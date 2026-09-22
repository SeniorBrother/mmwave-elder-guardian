/**
 * @file    fall_algo.c
 * @brief   摔倒检测算法实现（整数运算，无 FPU 依赖）
 */
#include "fall_algo.h"

int fall_isqrt(int x)
{
    int r = 0;
    if (x < 0) return 0;
    /* 逐次逼近整数平方根：满足 r*r <= x 的最大整数 r */
    while ((r + 1) * (r + 1) <= x) {
        r++;
    }
    return r;
}

static int abs_int(int v)
{
    return (v < 0) ? -v : v;
}

void fall_init(fall_detector_t *d)
{
    fall_config_t cfg;
    if (d == 0) return;
    cfg.hr_std_min     = 15;
    cfg.rr_std_min     = 4;
    cfg.hr_dev_percent = 20;
    fall_init_cfg(d, &cfg);
}

void fall_init_cfg(fall_detector_t *d, const fall_config_t *cfg)
{
    int i;
    if (d == 0) return;
    for (i = 0; i < FALL_WINDOW; i++) {
        d->heartRate[i] = 0;
        d->respRate[i]  = 0;
    }
    d->hrIndex = 0;
    d->hrReady = 0;
    d->rrIndex = 0;
    d->rrReady = 0;
    d->lastFallStatus = 0;
    if (cfg != 0) {
        d->cfg = *cfg;
    } else {
        d->cfg.hr_std_min     = 15;
        d->cfg.rr_std_min     = 4;
        d->cfg.hr_dev_percent = 20;
    }
}

void fall_reset_status(fall_detector_t *d)
{
    if (d == 0) return;
    d->lastFallStatus = 0;
}

/* 计算窗口标准差（整数） */
static int window_stddev(const int *buf, int n, int *out_mean)
{
    int sum = 0, mean, var_sum = 0, i, diff;
    for (i = 0; i < n; i++) sum += buf[i];
    mean = sum / n;
    for (i = 0; i < n; i++) {
        diff = buf[i] - mean;
        var_sum += diff * diff;
    }
    if (out_mean) *out_mean = mean;
    return fall_isqrt(var_sum / n);
}

int fall_check(fall_detector_t *d, int currentHR, int currentRR)
{
    int hrMean = 0, rrMean = 0, hrStd, rrStd, hrDev;

    if (d == 0) return 0;

    /* 1. 存入滑动窗口（环形） */
    d->heartRate[d->hrIndex] = currentHR;
    if (++d->hrIndex >= FALL_WINDOW) { d->hrIndex = 0; d->hrReady = 1; }
    d->respRate[d->rrIndex] = currentRR;
    if (++d->rrIndex >= FALL_WINDOW) { d->rrIndex = 0; d->rrReady = 1; }

    /* 2. 数据不足时不判断，避免开机误报 */
    if (!d->hrReady || !d->rrReady) return 0;

    /* 3. 计算心率、呼吸标准差 */
    hrStd = window_stddev(d->heartRate, FALL_WINDOW, &hrMean);
    rrStd = window_stddev(d->respRate,  FALL_WINDOW, &rrMean);

    /* 4. 突变判定：波动剧烈 + 当前心率偏离均值超过阈值百分比 */
    hrDev = abs_int(currentHR - hrMean);
    if (hrStd > d->cfg.hr_std_min &&
        rrStd > d->cfg.rr_std_min &&
        hrDev > (hrMean * d->cfg.hr_dev_percent / 100)) {
        if (d->lastFallStatus == 0) {   /* 边沿触发，防重复报警 */
            d->lastFallStatus = 1;
            return 1;
        }
        return 0;
    } else {
        d->lastFallStatus = 0;
        return 0;
    }
}
