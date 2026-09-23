/**
 * @file    alarm_rule.c
 * @brief   报警判定规则实现
 */
#include "alarm_rule.h"

void alarm_threshold_default(alarm_threshold_t *t)
{
    if (t == 0) return;
    t->heart_rate_max = 110;    /* 静息心率过快 */
    t->body_temp_max  = 38.0f;  /* 发烧 */
    t->co_max         = 80;     /* 煤气浓度超标（相对浓度%） */
}

uint32_t alarm_evaluate(const guardian_data_t *d, const alarm_threshold_t *t)
{
    uint32_t status = ALARM_NONE;
    if (d == 0 || t == 0) return ALARM_NONE;

    if (d->heart_rate > t->heart_rate_max)  status |= ALARM_HEART_RATE;
    if (d->body_temp  > t->body_temp_max)   status |= ALARM_BODY_TEMP;
    if (d->co         > t->co_max)          status |= ALARM_CO;
    if (d->fall)                            status |= ALARM_FALL;
    if (d->fall_sos)                        status |= ALARM_SOS;

    return status;
}

int alarm_is_active(uint32_t alarm_status)
{
    return (alarm_status != ALARM_NONE) ? 1 : 0;
}
