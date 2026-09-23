/**
 * @file    guardian_app.c
 * @brief   平台无关业务编排器实现
 */
#include "guardian_app.h"
#include "radar_proto.h"
#include "onenet_json.h"

void app_init(guardian_app_t *a, uint8_t modbus_addr)
{
    if (a == 0) return;
    guardian_data_reset(&a->data);
    fall_init(&a->fall);
    alarm_threshold_default(&a->thresh);
    modbus_regs_init(&a->mb);
    a->mb_addr   = (modbus_addr == 0) ? 1 : modbus_addr;
    a->mb.holding[MB_HOLD_NODE_ADDR]   = a->mb_addr;
    a->mb.holding[MB_HOLD_HR_MAX]      = (uint16_t)a->thresh.heart_rate_max;
    a->mb.holding[MB_HOLD_TEMP_MAX_X10]= (uint16_t)(a->thresh.body_temp_max * 10.0f);
    a->mb.holding[MB_HOLD_CO_MAX]      = (uint16_t)a->thresh.co_max;
    a->msg_id    = 0;
    a->buzzer_on = 0;
}

int app_on_radar_frame(guardian_app_t *a, const uint8_t *frame, int len)
{
    radar_evt_t evt;
    if (a == 0) return RADAR_EVT_NONE;
    if (!radar_parse_frame(frame, len, &evt)) return RADAR_EVT_NONE;

    switch (evt.type) {
    case RADAR_EVT_PRESENCE:
        a->data.people = evt.value;
        break;
    case RADAR_EVT_MOTION:
        a->data.motion = evt.value;
        break;
    case RADAR_EVT_RESP_RATE:
        a->data.resp_rate = evt.value;
        break;
    case RADAR_EVT_HEART_RATE:
        a->data.heart_rate = evt.value;
        /* 有心率(>40)辅助判定有人 */
        if (evt.value > 40) a->data.people = 1;
        break;
    default:
        break;
    }

    /* 每次拿到心率+呼吸跑一次摔倒检测（边沿触发） */
    if (evt.type == RADAR_EVT_HEART_RATE || evt.type == RADAR_EVT_RESP_RATE) {
        if (fall_check(&a->fall, a->data.heart_rate, a->data.resp_rate)) {
            a->data.fall = 1;
        }
    }
    return (int)evt.type;
}

void app_on_sensors(guardian_app_t *a, float body_temp, int env_temp,
                    int env_humi, int co)
{
    if (a == 0) return;
    a->data.body_temp = body_temp;
    a->data.env_temp  = env_temp;
    a->data.env_humi  = env_humi;
    a->data.co        = co;
}

uint32_t app_update(guardian_app_t *a)
{
    uint32_t status;
    if (a == 0) return ALARM_NONE;

    /* 从 Modbus 保持寄存器同步可能被主站/触控改过的阈值 */
    a->thresh.heart_rate_max = (int)a->mb.holding[MB_HOLD_HR_MAX];
    a->thresh.body_temp_max  = (float)a->mb.holding[MB_HOLD_TEMP_MAX_X10] / 10.0f;
    a->thresh.co_max         = (int)a->mb.holding[MB_HOLD_CO_MAX];

    status = alarm_evaluate(&a->data, &a->thresh);
    a->data.alarm_status = status;
    a->buzzer_on = app_buzzer_should_on(a);

    modbus_load_data(&a->mb, &a->data);
    return status;
}

int app_handle_modbus(guardian_app_t *a, const uint8_t *req, int req_len,
                      uint8_t *resp, int resp_cap)
{
    if (a == 0) return 0;
    return modbus_slave_process(a->mb_addr, &a->mb, req, req_len, resp, resp_cap);
}

void app_apply_coils(guardian_app_t *a)
{
    if (a == 0) return;
    if (a->mb.coils[MB_COIL_CLEAR_ALARM]) {
        app_clear_alarm(a);
        a->mb.coils[MB_COIL_CLEAR_ALARM] = 0;   /* 自动复位为脉冲 */
    }
    if (a->mb.coils[MB_COIL_SOS]) {
        app_trigger_sos(a);
        a->mb.coils[MB_COIL_SOS] = 0;
    }
}

int app_build_cloud_json(guardian_app_t *a, char *out, int out_sz)
{
    if (a == 0) return -1;
    a->msg_id++;
    if (a->msg_id > 1000000) a->msg_id = 1;
    return onenet_build_dp_json(&a->data, a->msg_id, out, out_sz);
}

void app_clear_alarm(guardian_app_t *a)
{
    if (a == 0) return;
    a->data.fall     = 0;
    a->data.fall_sos = 0;
    fall_reset_status(&a->fall);
}

void app_trigger_sos(guardian_app_t *a)
{
    if (a == 0) return;
    a->data.fall     = 1;
    a->data.fall_sos = 1;
}

int app_buzzer_should_on(const guardian_app_t *a)
{
    if (a == 0) return 0;
    /* 报警或 Modbus 强制线圈开启 -> 蜂鸣 */
    if (alarm_is_active(a->data.alarm_status)) return 1;
    if (a->mb.coils[MB_COIL_BUZZER]) return 1;
    return 0;
}
