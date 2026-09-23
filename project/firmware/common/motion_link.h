/**
 * @file    motion_link.h
 * @brief   分层双主控"大脑(AI)<->小脑(运动控制)"UART 通信协议（纯 C，可单测）
 *
 * 帧格式：
 *   [0] 0xAA  [1] 0x55  [2] type  [3] len  [4..4+len-1] payload  [4+len] checksum
 *   checksum = (sum of bytes[0 .. 4+len-1]) & 0xFF
 *
 * 见 docs/mobile-extension.md 第二节。
 */
#ifndef MOTION_LINK_H
#define MOTION_LINK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML_HEAD0        0xAA
#define ML_HEAD1        0x55
#define ML_OVERHEAD     5       /* head(2)+type(1)+len(1)+checksum(1) */
#define ML_MAX_PAYLOAD  16
#define ML_MAX_FRAME    (ML_OVERHEAD + ML_MAX_PAYLOAD)

/* 指令类型 */
typedef enum {
    ML_CMD_VELOCITY   = 0x01,   /* 大脑->小脑: 速度指令 vx,vy,wz */
    ML_CMD_MOTOR_PWM  = 0x02,   /* 大脑->小脑: 4 电机直驱 pwm[-100..100] */
    ML_CMD_ENABLE     = 0x03,   /* 大脑->小脑: 0停 1使能 2急停 */
    ML_RSP_CHASSIS    = 0x10,   /* 小脑->大脑: 底盘状态 */
    ML_RSP_VITALS     = 0x11    /* 小脑->大脑: 监护数据透传 */
} ml_type_t;

/* 使能状态 */
#define ML_ENABLE_STOP   0
#define ML_ENABLE_RUN    1
#define ML_ENABLE_ESTOP  2

/* ---- 结构化负载（小端序列化） ---- */
typedef struct { int16_t vx, vy, wz; } ml_velocity_t;      /* mm/s, mm/s, mdeg/s */
typedef struct { int8_t pwm[4]; } ml_motor_pwm_t;          /* -100..100 */
typedef struct {
    int16_t x_mm, y_mm, heading;   /* 里程与朝向(mdeg) */
    uint8_t battery;               /* 电量 % */
    uint8_t fault;                 /* 故障位图 */
} ml_chassis_t;
typedef struct {
    uint8_t heart_rate, resp_rate, people, fall;
    uint8_t body_temp_x10_lo;      /* 体温 x10, 拆两字节 */
    uint8_t body_temp_x10_hi;
} ml_vitals_t;

/* ---- 编码：返回帧长度，<0 失败 ---- */
int ml_encode_velocity(uint8_t *out, int cap, const ml_velocity_t *v);
int ml_encode_motor_pwm(uint8_t *out, int cap, const ml_motor_pwm_t *m);
int ml_encode_enable(uint8_t *out, int cap, uint8_t enable);
int ml_encode_chassis(uint8_t *out, int cap, const ml_chassis_t *c);
int ml_encode_vitals(uint8_t *out, int cap, const ml_vitals_t *v);

/* ---- 解码 ---- */
typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t payload[ML_MAX_PAYLOAD];
} ml_frame_t;

/**
 * @brief 校验并拆出一帧（不解释 payload）
 * @return 1 合法, 0 非法(帧头/长度/校验错)
 */
int ml_decode(const uint8_t *frame, int len, ml_frame_t *out);

/* 便捷 payload 解释器（解码成功后调用） */
int ml_parse_velocity(const ml_frame_t *f, ml_velocity_t *v);
int ml_parse_motor_pwm(const ml_frame_t *f, ml_motor_pwm_t *m);
int ml_parse_enable(const ml_frame_t *f, uint8_t *enable);
int ml_parse_chassis(const ml_frame_t *f, ml_chassis_t *c);
int ml_parse_vitals(const ml_frame_t *f, ml_vitals_t *v);

/* 计算校验和（内部/测试用） */
uint8_t ml_checksum(const uint8_t *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_LINK_H */
