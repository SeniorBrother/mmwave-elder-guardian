/**
 * @file    alarm_rule.h
 * @brief   报警判定规则（纯 C，跨平台）
 *
 * 任意一项超限即报警，返回报警原因位图（见 guardian_data.h 的 ALARM_*）。
 */
#ifndef ALARM_RULE_H
#define ALARM_RULE_H

#include <stdint.h>
#include "guardian_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 报警阈值（可通过 LVGL 设置页 / Modbus 保持寄存器在线修改） */
typedef struct {
    int   heart_rate_max;   /* 心率上限, 默认 110 次/分 */
    float body_temp_max;    /* 体温上限, 默认 38.0 ℃ */
    int   co_max;           /* 一氧化碳上限, 默认 80% */
} alarm_threshold_t;

/**
 * @brief 载入默认阈值（心率110 / 体温38 / CO80）
 */
void alarm_threshold_default(alarm_threshold_t *t);

/**
 * @brief 根据数据与阈值评估报警状态
 * @return 报警原因位图（ALARM_NONE 表示正常）
 */
uint32_t alarm_evaluate(const guardian_data_t *d, const alarm_threshold_t *t);

/**
 * @brief 是否处于报警状态（位图非零）
 */
int alarm_is_active(uint32_t alarm_status);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_RULE_H */
