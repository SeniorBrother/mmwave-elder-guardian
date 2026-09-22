/**
 * @file    fall_algo.h
 * @brief   摔倒/突发异常检测算法（心率+呼吸滑动窗口方差突变，纯 C 整数实现）
 *
 * 教学级"生理信号突变检测"，非医疗级摔倒判定。实际产品应以雷达姿态感知为主、
 * 本算法为辅，并加入持续时间与人工确认机制降低误报。
 */
#ifndef FALL_ALGO_H
#define FALL_ALGO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FALL_WINDOW     10      /* 滑动窗口长度（最近10次采样） */

/* 可调阈值（默认沿用参考工程经验值） */
typedef struct {
    int hr_std_min;         /* 心率标准差阈值, 默认 15 */
    int rr_std_min;         /* 呼吸标准差阈值, 默认 4  */
    int hr_dev_percent;     /* 当前心率偏离均值百分比阈值, 默认 20 */
} fall_config_t;

typedef struct {
    int heartRate[FALL_WINDOW];
    int hrIndex;
    int hrReady;
    int respRate[FALL_WINDOW];
    int rrIndex;
    int rrReady;
    int lastFallStatus;     /* 上次摔倒状态, 用于边沿触发防重复 */
    fall_config_t cfg;
} fall_detector_t;

/**
 * @brief 初始化检测器（清零 + 载入默认阈值）
 */
void fall_init(fall_detector_t *d);

/**
 * @brief 用自定义阈值初始化
 */
void fall_init_cfg(fall_detector_t *d, const fall_config_t *cfg);

/**
 * @brief 喂入一次新的 (心率, 呼吸) 采样，返回是否检测到摔倒（边沿触发）
 * @return 1 = 本次判定为疑似摔倒（且上次不是），0 = 否
 */
int fall_check(fall_detector_t *d, int currentHR, int currentRR);

/**
 * @brief 清除摔倒锁存状态（解除报警后调用，允许下次再次触发）
 */
void fall_reset_status(fall_detector_t *d);

/**
 * @brief 整数平方根（内部方差计算用，便于无 FPU 平台）
 */
int fall_isqrt(int x);

#ifdef __cplusplus
}
#endif

#endif /* FALL_ALGO_H */
