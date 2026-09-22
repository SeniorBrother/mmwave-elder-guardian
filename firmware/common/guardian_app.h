/**
 * @file    guardian_app.h
 * @brief   平台无关的业务编排器 —— 把雷达解析、摔倒检测、报警判定、
 *          Modbus 寄存器刷新、OneNet JSON 组包统一封装成一组纯 C 接口。
 *
 * 两套 BSP（STM32 / ESP32-S3）的 FreeRTOS 任务只需调用这里，
 * 无需重复实现业务逻辑，保证行为一致、可单元测试。
 */
#ifndef GUARDIAN_APP_H
#define GUARDIAN_APP_H

#include <stdint.h>
#include "guardian_data.h"
#include "fall_algo.h"
#include "alarm_rule.h"
#include "modbus_rtu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    guardian_data_t    data;        /* 当前监测数据 */
    fall_detector_t    fall;        /* 摔倒检测器 */
    alarm_threshold_t  thresh;      /* 报警阈值 */
    modbus_regs_t      mb;          /* Modbus 寄存器区 */
    uint8_t            mb_addr;     /* 本节点 Modbus 从站地址 */
    int                msg_id;      /* OneNet 上报自增 id */
    int                buzzer_on;   /* 当前蜂鸣器状态缓存 */
} guardian_app_t;

/**
 * @brief 初始化业务上下文（默认阈值、Modbus地址1、清零数据）
 */
void app_init(guardian_app_t *a, uint8_t modbus_addr);

/**
 * @brief 处理一帧雷达数据：解析事件 + 更新数据 + 摔倒检测
 * @return 解析出的事件类型（RADAR_EVT_NONE 表示无有效事件/非法帧）
 */
int app_on_radar_frame(guardian_app_t *a, const uint8_t *frame, int len);

/**
 * @brief 更新环境/健康传感器数据
 */
void app_on_sensors(guardian_app_t *a, float body_temp, int env_temp,
                    int env_humi, int co);

/**
 * @brief 周期评估报警 + 刷新 Modbus 寄存器
 * @return 报警位图（ALARM_NONE 表示正常）
 */
uint32_t app_update(guardian_app_t *a);

/**
 * @brief 处理一帧 Modbus RTU 请求，生成响应
 * @return 响应长度（>0 需回发；0 无响应）
 */
int app_handle_modbus(guardian_app_t *a, const uint8_t *req, int req_len,
                      uint8_t *resp, int resp_cap);

/**
 * @brief 组 OneNet 数据点上报 JSON（自增 msg_id）
 * @return JSON 长度，<0 失败
 */
int app_build_cloud_json(guardian_app_t *a, char *out, int out_sz);

/**
 * @brief 解除报警（清 fall/sos、清对应线圈）
 */
void app_clear_alarm(guardian_app_t *a);

/**
 * @brief 触发 SOS 求助
 */
void app_trigger_sos(guardian_app_t *a);

/**
 * @brief 根据 Modbus 主站写入的线圈，同步解除报警/SOS 动作
 *        （在 app_handle_modbus 之后调用，把线圈语义落到业务）
 */
void app_apply_coils(guardian_app_t *a);

/**
 * @brief 期望的蜂鸣器状态（报警或强制线圈开启时为1）
 */
int app_buzzer_should_on(const guardian_app_t *a);

#ifdef __cplusplus
}
#endif

#endif /* GUARDIAN_APP_H */
