/**
 * @file    test_main.c
 * @brief   firmware/common 业务层单元测试（gcc 直接编译运行）
 *
 * 编译（Linux/WSL）：
 *   gcc -Wall -Wextra -std=c99 -I.. test_main.c ../[模块].c -o run_tests
 *   或直接: bash build_and_test.sh
 * 运行：
 *   ./run_tests
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "guardian_data.h"
#include "radar_proto.h"
#include "fall_algo.h"
#include "alarm_rule.h"
#include "onenet_json.h"
#include "modbus_rtu.h"
#include "guardian_app.h"
#include "motion_link.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond, name) do { \
    if (cond) { g_pass++; printf("[PASS] %s\n", name); } \
    else      { g_fail++; printf("[FAIL] %s  (%s:%d)\n", name, __FILE__, __LINE__); } \
} while (0)

/* 构造一帧合法雷达数据帧 */
static void make_radar_frame(uint8_t *f, uint8_t type, uint8_t cmd, uint8_t data)
{
    f[0] = 0x53; f[1] = 0x59; f[2] = type; f[3] = cmd;
    f[4] = 0x00; f[5] = 0x00; f[6] = data;
    f[7] = radar_checksum(f, RADAR_FRAME_LEN);
    f[8] = 0x54; f[9] = 0x43;
}

static void test_radar(void)
{
    uint8_t f[RADAR_FRAME_LEN];
    radar_evt_t evt;

    /* 心率帧 0x85/0x02 = 82 */
    make_radar_frame(f, RADAR_TYPE_HEART, 0x02, 82);
    CHECK(radar_frame_valid(f, RADAR_FRAME_LEN) == 1, "radar: 合法帧校验通过");
    CHECK(radar_parse_frame(f, RADAR_FRAME_LEN, &evt) == 1, "radar: 解析返回1");
    CHECK(evt.type == RADAR_EVT_HEART_RATE && evt.value == 82, "radar: 心率=82");

    /* 呼吸帧 0x81/0x02 = 18 */
    make_radar_frame(f, RADAR_TYPE_RESP, 0x02, 18);
    radar_parse_frame(f, RADAR_FRAME_LEN, &evt);
    CHECK(evt.type == RADAR_EVT_RESP_RATE && evt.value == 18, "radar: 呼吸=18");

    /* 存在帧 0x80/0x01 = 1 */
    make_radar_frame(f, RADAR_TYPE_STATUS, 0x01, 0x01);
    radar_parse_frame(f, RADAR_FRAME_LEN, &evt);
    CHECK(evt.type == RADAR_EVT_PRESENCE && evt.value == 1, "radar: 有人");

    /* 运动帧 0x80/0x02 = 2 */
    make_radar_frame(f, RADAR_TYPE_STATUS, 0x02, 0x02);
    radar_parse_frame(f, RADAR_FRAME_LEN, &evt);
    CHECK(evt.type == RADAR_EVT_MOTION && evt.value == 2, "radar: 剧烈运动");

    /* 破坏校验和 -> 非法 */
    make_radar_frame(f, RADAR_TYPE_HEART, 0x02, 82);
    f[7] ^= 0xFF;
    CHECK(radar_frame_valid(f, RADAR_FRAME_LEN) == 0, "radar: 校验和错误被拒");
    CHECK(radar_parse_frame(f, RADAR_FRAME_LEN, &evt) == 0, "radar: 非法帧解析返回0");

    /* 开启心跳命令帧 */
    uint8_t cmd[RADAR_FRAME_LEN];
    radar_build_enable_heartbeat(cmd);
    CHECK(cmd[0]==0x53 && cmd[1]==0x59 && cmd[2]==0x85 && cmd[7]==0x3D, "radar: 开启命令帧正确");
    radar_build_disable_heartbeat(cmd);
    CHECK(cmd[6]==0x00 && cmd[7]==0x3C, "radar: 关闭命令帧正确");
}

static void test_fall(void)
{
    fall_detector_t d;
    int i, detected = 0;

    fall_init(&d);
    /* 先喂 10 次平稳数据填满窗口 */
    for (i = 0; i < 10; i++) fall_check(&d, 70, 16);
    /* 平稳数据不应触发 */
    CHECK(fall_check(&d, 71, 16) == 0, "fall: 平稳数据不触发");

    /* 制造剧烈波动：窗口内已有平稳值，插入极端值 */
    fall_init(&d);
    for (i = 0; i < 9; i++) fall_check(&d, 70 + (i % 2) * 40, 16 + (i % 2) * 10);
    /* 第10次填满后，再来一个大幅偏离值 */
    for (i = 0; i < 4; i++) {
        if (fall_check(&d, 160, 30)) detected = 1;
        if (fall_check(&d, 45, 8))   detected = 1;
    }
    CHECK(detected == 1, "fall: 剧烈波动触发摔倒");

    /* isqrt 正确性 */
    CHECK(fall_isqrt(0) == 0 && fall_isqrt(100) == 10 && fall_isqrt(99) == 9, "fall: isqrt");
}

static void test_alarm(void)
{
    guardian_data_t d;
    alarm_threshold_t t;
    alarm_threshold_default(&t);
    guardian_data_reset(&d);

    CHECK(t.heart_rate_max == 110 && t.co_max == 80, "alarm: 默认阈值");

    /* 正常 */
    d.heart_rate = 80; d.body_temp = 36.5f; d.co = 10;
    CHECK(alarm_evaluate(&d, &t) == ALARM_NONE, "alarm: 正常无报警");

    /* 心率超限 */
    d.heart_rate = 120;
    CHECK(alarm_evaluate(&d, &t) & ALARM_HEART_RATE, "alarm: 心率超限");

    /* 体温 + 摔倒 组合 */
    d.heart_rate = 80; d.body_temp = 39.0f; d.fall = 1;
    uint32_t s = alarm_evaluate(&d, &t);
    CHECK((s & ALARM_BODY_TEMP) && (s & ALARM_FALL), "alarm: 体温+摔倒组合");
    CHECK(alarm_is_active(s) == 1, "alarm: is_active");

    /* SOS */
    d.body_temp = 36.0f; d.fall = 0; d.fall_sos = 1;
    CHECK(alarm_evaluate(&d, &t) & ALARM_SOS, "alarm: SOS");
}

static void test_onenet(void)
{
    guardian_data_t d;
    char buf[256];
    guardian_data_reset(&d);
    d.heart_rate = 82; d.resp_rate = 18; d.people = 1; d.fall = 0;
    d.body_temp = 36.5f; d.env_temp = 25; d.env_humi = 60; d.co = 12;

    int n = onenet_build_dp_json(&d, 123, buf, sizeof(buf));
    CHECK(n > 0, "onenet: JSON 生成成功");
    CHECK(strstr(buf, "\"HeartRate\":[{\"v\":82}]") != NULL, "onenet: 含心率字段");
    CHECK(strstr(buf, "\"MLX90614\":[{\"v\":36.5}]") != NULL, "onenet: 体温格式化为36.5");
    CHECK(strstr(buf, "\"RespiratoryRate\":[{\"v\":18}]") != NULL, "onenet: 含呼吸字段");
    CHECK(buf[0] == '{' && buf[n-1] == '}', "onenet: JSON 首尾括号");

    /* 缓冲不足应返回 -1 */
    char small[8];
    CHECK(onenet_build_dp_json(&d, 1, small, sizeof(small)) == -1, "onenet: 缓冲不足返回-1");

    /* 主题 */
    char topic[64];
    onenet_build_post_topic("8w7dxp3Pj8", "dev1", topic, sizeof(topic));
    CHECK(strcmp(topic, "$sys/8w7dxp3Pj8/dev1/dp/post/json") == 0, "onenet: 发布主题");
    onenet_build_sub_topic("8w7dxp3Pj8", "dev1", topic, sizeof(topic));
    CHECK(strcmp(topic, "$sys/8w7dxp3Pj8/dev1/#") == 0, "onenet: 订阅主题");
}

static void test_modbus(void)
{
    modbus_regs_t r;
    guardian_data_t d;
    uint8_t req[32], resp[64];
    int rl, n;

    modbus_regs_init(&r);
    guardian_data_reset(&d);
    d.heart_rate = 82; d.resp_rate = 18; d.body_temp = 36.5f; d.people = 1;
    d.fw_version = 0x0102;
    modbus_load_data(&r, &d);

    CHECK(r.holding[MB_HOLD_NODE_ADDR] == 1, "modbus: 默认从站地址=1");
    CHECK(r.input[MB_IN_HEART_RATE] == 82, "modbus: 心率入寄存器");
    CHECK(r.input[MB_IN_BODY_TEMP_X10] == 365, "modbus: 体温x10=365");
    CHECK(r.input[MB_IN_FW_VERSION] == 0x0102, "modbus: 版本入寄存器");

    /* CRC16 已知向量: 01 03 00 00 00 0A -> CRC = C5 CD */
    uint8_t crcvec[6] = {0x01,0x03,0x00,0x00,0x00,0x0A};
    uint16_t crc = modbus_crc16(crcvec, 6);
    CHECK((crc & 0xFF) == 0xC5 && ((crc >> 8) & 0xFF) == 0xCD, "modbus: CRC16标准向量");

    /* 读输入寄存器 0x04: addr=1 fc=04 start=0 qty=10 */
    req[0]=0x01; req[1]=0x04; req[2]=0x00; req[3]=0x00; req[4]=0x00; req[5]=0x0A;
    crc = modbus_crc16(req, 6);
    req[6]=crc&0xFF; req[7]=crc>>8;
    n = modbus_slave_process(1, &r, req, 8, resp, sizeof(resp));
    CHECK(n == 3 + 20 + 2, "modbus: 读输入寄存器响应长度");
    CHECK(resp[1] == 0x04 && resp[2] == 20, "modbus: 字节数=20");
    /* 心率在第0寄存器, 高字节在前 -> resp[3]=0 resp[4]=82 */
    CHECK(resp[3] == 0 && resp[4] == 82, "modbus: 响应含心率82");

    /* 写单线圈 0x05: 解除报警 coil0 = ON(0xFF00) */
    req[0]=0x01; req[1]=0x05; req[2]=0x00; req[3]=0x00; req[4]=0xFF; req[5]=0x00;
    crc = modbus_crc16(req, 6); req[6]=crc&0xFF; req[7]=crc>>8;
    n = modbus_slave_process(1, &r, req, 8, resp, sizeof(resp));
    CHECK(n == 8 && r.coils[MB_COIL_CLEAR_ALARM] == 1, "modbus: 写线圈解除报警");

    /* 写单寄存器 0x06: 心率上限=120 */
    req[0]=0x01; req[1]=0x06; req[2]=0x00; req[3]=0x01; req[4]=0x00; req[5]=0x78;
    crc = modbus_crc16(req, 6); req[6]=crc&0xFF; req[7]=crc>>8;
    n = modbus_slave_process(1, &r, req, 8, resp, sizeof(resp));
    CHECK(n == 8 && r.holding[MB_HOLD_HR_MAX] == 120, "modbus: 写单寄存器阈值=120");

    /* 地址不符 -> 不响应 */
    req[0]=0x02; req[1]=0x04; req[2]=0x00; req[3]=0x00; req[4]=0x00; req[5]=0x01;
    crc = modbus_crc16(req, 6); req[6]=crc&0xFF; req[7]=crc>>8;
    n = modbus_slave_process(1, &r, req, 8, resp, sizeof(resp));
    CHECK(n == 0, "modbus: 地址不符不响应");

    /* CRC 错 -> 丢弃 */
    req[0]=0x01; req[1]=0x04; req[2]=0x00; req[3]=0x00; req[4]=0x00; req[5]=0x01;
    req[6]=0x00; req[7]=0x00;
    n = modbus_slave_process(1, &r, req, 8, resp, sizeof(resp));
    CHECK(n == 0, "modbus: CRC错误丢弃");

    /* 非法功能码 -> 异常响应 */
    req[0]=0x01; req[1]=0x07; req[2]=0x00; req[3]=0x00;
    crc = modbus_crc16(req, 4); req[4]=crc&0xFF; req[5]=crc>>8;
    n = modbus_slave_process(1, &r, req, 6, resp, sizeof(resp));
    CHECK(n == 5 && resp[1] == (0x07|0x80) && resp[2] == MB_EX_ILLEGAL_FUNCTION,
          "modbus: 非法功能码异常响应");

    /* 越界地址 -> 异常响应 */
    req[0]=0x01; req[1]=0x04; req[2]=0x00; req[3]=0x63; req[4]=0x00; req[5]=0x01;
    crc = modbus_crc16(req, 6); req[6]=crc&0xFF; req[7]=crc>>8;
    n = modbus_slave_process(1, &r, req, 8, resp, sizeof(resp));
    CHECK(n == 5 && resp[2] == MB_EX_ILLEGAL_DATA_ADDR, "modbus: 越界地址异常");
    (void)rl;
}

static void test_app(void)
{
    guardian_app_t a;
    uint8_t frame[RADAR_FRAME_LEN];
    uint8_t req[32], resp[64];
    uint16_t crc;
    int n;

    app_init(&a, 1);
    CHECK(a.data.heart_rate == 0 && a.mb_addr == 1, "app: 初始化");

    /* 喂一帧心率=82 -> data.heart_rate 更新且判定有人 */
    frame[0]=0x53; frame[1]=0x59; frame[2]=RADAR_TYPE_HEART; frame[3]=0x02;
    frame[4]=0; frame[5]=0; frame[6]=82;
    frame[7]=radar_checksum(frame, RADAR_FRAME_LEN); frame[8]=0x54; frame[9]=0x43;
    n = app_on_radar_frame(&a, frame, RADAR_FRAME_LEN);
    CHECK(n == RADAR_EVT_HEART_RATE, "app: 雷达心率事件");
    CHECK(a.data.heart_rate == 82 && a.data.people == 1, "app: 心率更新+判定有人");

    /* 传感器更新 */
    app_on_sensors(&a, 36.5f, 25, 60, 12);
    CHECK(a.data.body_temp > 36.4f && a.data.env_temp == 25, "app: 传感器更新");

    /* 正常时评估无报警 */
    CHECK(app_update(&a) == ALARM_NONE, "app: 正常无报警");
    CHECK(a.mb.input[MB_IN_HEART_RATE] == 82, "app: 刷新Modbus寄存器");

    /* 触发SOS -> 报警位含 SOS，蜂鸣应开启 */
    app_trigger_sos(&a);
    uint32_t s = app_update(&a);
    CHECK((s & ALARM_SOS) && app_buzzer_should_on(&a) == 1, "app: SOS触发报警+蜂鸣");

    /* 解除报警 -> 恢复正常 */
    app_clear_alarm(&a);
    CHECK(app_update(&a) == ALARM_NONE, "app: 解除报警后恢复");

    /* Modbus 主站写线圈0(解除报警) 经 app 落地业务 */
    app_trigger_sos(&a);
    app_update(&a);
    req[0]=0x01; req[1]=0x05; req[2]=0x00; req[3]=0x00; req[4]=0xFF; req[5]=0x00;
    crc=modbus_crc16(req,6); req[6]=crc&0xFF; req[7]=crc>>8;
    n = app_handle_modbus(&a, req, 8, resp, sizeof(resp));
    CHECK(n == 8, "app: Modbus写线圈有响应");
    app_apply_coils(&a);
    CHECK(a.data.fall == 0 && a.data.fall_sos == 0, "app: 线圈解除报警落地");

    /* 云上报 JSON 自增 id */
    char buf[256];
    int len1 = app_build_cloud_json(&a, buf, sizeof(buf));
    CHECK(len1 > 0 && strstr(buf, "\"id\":1") != NULL, "app: 云JSON id自增");
}

static void test_motion_link(void)
{
    uint8_t buf[ML_MAX_FRAME];
    ml_frame_t f;
    int n;

    /* 速度指令 round-trip（含负值） */
    ml_velocity_t vin = {-120, 200, -4500}, vout;
    n = ml_encode_velocity(buf, sizeof(buf), &vin);
    CHECK(n == ML_OVERHEAD + 6, "motion: 速度帧长度");
    CHECK(ml_decode(buf, n, &f) == 1 && f.type == ML_CMD_VELOCITY, "motion: 速度帧解码");
    CHECK(ml_parse_velocity(&f, &vout) == 1 &&
          vout.vx == -120 && vout.vy == 200 && vout.wz == -4500, "motion: 速度round-trip");

    /* 电机直驱 */
    ml_motor_pwm_t min = {{80, -80, 100, -100}}, mout;
    n = ml_encode_motor_pwm(buf, sizeof(buf), &min);
    CHECK(ml_decode(buf, n, &f) == 1 && ml_parse_motor_pwm(&f, &mout) == 1 &&
          mout.pwm[0] == 80 && mout.pwm[1] == -80 &&
          mout.pwm[2] == 100 && mout.pwm[3] == -100, "motion: 电机PWM round-trip");

    /* 急停 */
    uint8_t en = 0;
    n = ml_encode_enable(buf, sizeof(buf), ML_ENABLE_ESTOP);
    CHECK(ml_decode(buf, n, &f) == 1 && ml_parse_enable(&f, &en) == 1 &&
          en == ML_ENABLE_ESTOP, "motion: 急停指令");

    /* 底盘状态 round-trip */
    ml_chassis_t cin = {1500, -2000, 17999, 88, 0x03}, cout;
    n = ml_encode_chassis(buf, sizeof(buf), &cin);
    CHECK(ml_decode(buf, n, &f) == 1 && ml_parse_chassis(&f, &cout) == 1 &&
          cout.x_mm == 1500 && cout.y_mm == -2000 && cout.heading == 17999 &&
          cout.battery == 88 && cout.fault == 0x03, "motion: 底盘状态round-trip");

    /* 监护透传 */
    ml_vitals_t gin = {82, 18, 1, 0, 0x6D, 0x01}, gout;  /* 体温 0x016D=365 */
    n = ml_encode_vitals(buf, sizeof(buf), &gin);
    CHECK(ml_decode(buf, n, &f) == 1 && ml_parse_vitals(&f, &gout) == 1 &&
          gout.heart_rate == 82 &&
          (gout.body_temp_x10_lo | (gout.body_temp_x10_hi << 8)) == 365,
          "motion: 监护透传round-trip");

    /* 校验和错误应被拒 */
    n = ml_encode_velocity(buf, sizeof(buf), &vin);
    buf[n - 1] ^= 0xFF;
    CHECK(ml_decode(buf, n, &f) == 0, "motion: 校验错拒绝");

    /* 帧头错误应被拒 */
    n = ml_encode_velocity(buf, sizeof(buf), &vin);
    buf[0] = 0x00;
    CHECK(ml_decode(buf, n, &f) == 0, "motion: 帧头错拒绝");

    /* 缓冲不足 */
    CHECK(ml_encode_velocity(buf, 4, &vin) == -1, "motion: 缓冲不足返回-1");
}

int main(void)
{
    printf("==== mmwave-elder-guardian common layer tests ====\n");
    test_radar();
    test_fall();
    test_alarm();
    test_onenet();
    test_modbus();
    test_app();
    test_motion_link();
    printf("==== result: %d passed, %d failed ====\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
