/**
 * @file    guardian_data.h
 * @brief   跨平台共享的监测数据模型（纯 C，无硬件依赖）
 *
 * 所有 BSP（STM32 / ESP32-S3）采集到的数据统一填入 guardian_data_t，
 * 再由报警规则、OneNet JSON、Modbus 寄存器映射共同消费。
 * 多线程环境下对该结构的读写需用互斥量保护（见各平台 Task 实现）。
 */
#ifndef GUARDIAN_DATA_H
#define GUARDIAN_DATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 报警原因位图（alarm_evaluate 返回值 / Modbus AlarmStatus 寄存器） */
#define ALARM_NONE          (0u)
#define ALARM_HEART_RATE    (1u << 0)   /* 心率过快 */
#define ALARM_BODY_TEMP     (1u << 1)   /* 体温过高 */
#define ALARM_CO            (1u << 2)   /* 一氧化碳超标 */
#define ALARM_FALL          (1u << 3)   /* 摔倒 */
#define ALARM_SOS           (1u << 4)   /* 紧急求助 */

/* 运动状态（雷达 0x80/0x02 上报） */
typedef enum {
    MOTION_STILL   = 0,   /* 静止 */
    MOTION_MOVING  = 1,   /* 运动 */
    MOTION_VIOLENT = 2    /* 较剧烈运动 */
} motion_state_t;

/**
 * @brief 系统监测数据总集
 */
typedef struct {
    /* --- 来自 60G 毫米波雷达 R60ABD1 --- */
    int      heart_rate;    /* 心率, 0~100 次/分 */
    int      resp_rate;     /* 呼吸频率, 0~25 次/分 */
    int      people;        /* 人体存在: 0=无人 1=有人 */
    int      motion;        /* 运动状态, 见 motion_state_t */
    int      fall;          /* 摔倒: 0=正常 1=疑似摔倒 */
    int      fall_sos;      /* 紧急求助(SOS): 0=无 1=触发 */

    /* --- 来自环境/健康传感器 --- */
    float    body_temp;     /* MLX90614 红外体温, 摄氏度 */
    int      env_temp;      /* DHT11 室内温度, 摄氏度 */
    int      env_humi;      /* DHT11 室内湿度, %RH */
    int      co;            /* MQ7 一氧化碳相对浓度, 0~100% */

    /* --- 系统状态 --- */
    uint32_t alarm_status;  /* 当前报警位图, 见 ALARM_* */
    uint16_t fw_version;    /* 固件版本号, 例如 0x0102 = v1.2 */
    int      wifi_online;   /* WiFi/MQTT 是否在线: 0/1 */
    int      cloud_online;  /* OneNet 是否已连接: 0/1 */
} guardian_data_t;

/**
 * @brief 将数据模型清零为安全默认值
 */
static inline void guardian_data_reset(guardian_data_t *d)
{
    if (d == 0) return;
    d->heart_rate   = 0;
    d->resp_rate    = 0;
    d->people       = 0;
    d->motion       = MOTION_STILL;
    d->fall         = 0;
    d->fall_sos     = 0;
    d->body_temp    = 0.0f;
    d->env_temp     = 0;
    d->env_humi     = 0;
    d->co           = 0;
    d->alarm_status = ALARM_NONE;
    d->fw_version   = 0x0100;   /* v1.0 */
    d->wifi_online  = 0;
    d->cloud_online = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* GUARDIAN_DATA_H */
