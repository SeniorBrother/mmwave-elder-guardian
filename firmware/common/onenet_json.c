/**
 * @file    onenet_json.c
 * @brief   OneNet 数据点上报 JSON 组包实现
 *
 * 为避免部分嵌入式 newlib-nano 未开启浮点 printf，体温用整数拆分为
 * "整数.一位小数" 的形式手工格式化，不依赖 %f。
 */
#include <stdio.h>
#include "onenet_json.h"

/* 把 float 摄氏度格式化为 "xx.y"（一位小数），返回写入长度 */
static int fmt_temp1(float v, char *out, int out_sz)
{
    int t10, ip, fp;
    t10 = (int)(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f));  /* 四舍五入到0.1 */
    ip  = t10 / 10;
    fp  = t10 % 10;
    if (fp < 0) fp = -fp;
    return snprintf(out, (size_t)out_sz, "%d.%d", ip, fp);
}

int onenet_build_dp_json(const guardian_data_t *d, int msg_id, char *out, int out_sz)
{
    char temp_str[16];
    int  n;

    if (d == 0 || out == 0 || out_sz <= 0) return -1;
    fmt_temp1(d->body_temp, temp_str, (int)sizeof(temp_str));

    n = snprintf(out, (size_t)out_sz,
        "{\"id\":%d,\"dp\":{"
        "\"HeartRate\":[{\"v\":%d}],"
        "\"DHT11_T\":[{\"v\":%d}],"
        "\"DHT11_H\":[{\"v\":%d}],"
        "\"MLX90614\":[{\"v\":%s}],"
        "\"MQ7\":[{\"v\":%d}],"
        "\"People\":[{\"v\":%d}],"
        "\"Fall\":[{\"v\":%d}],"
        "\"RespiratoryRate\":[{\"v\":%d}]}}",
        msg_id,
        d->heart_rate, d->env_temp, d->env_humi, temp_str,
        d->co, d->people, d->fall, d->resp_rate);

    /* snprintf 返回"本应写入的长度"，若 >= out_sz 说明被截断 */
    if (n < 0 || n >= out_sz) return -1;
    return n;
}

int onenet_build_sub_topic(const char *pid, const char *dev, char *out, int out_sz)
{
    int n;
    if (pid == 0 || dev == 0 || out == 0 || out_sz <= 0) return -1;
    n = snprintf(out, (size_t)out_sz, "$sys/%s/%s/%s", pid, dev, ONENET_TOPIC_SUB_SUFFIX);
    if (n < 0 || n >= out_sz) return -1;
    return n;
}

int onenet_build_post_topic(const char *pid, const char *dev, char *out, int out_sz)
{
    int n;
    if (pid == 0 || dev == 0 || out == 0 || out_sz <= 0) return -1;
    n = snprintf(out, (size_t)out_sz, "$sys/%s/%s/%s", pid, dev, ONENET_TOPIC_POST_SUFFIX);
    if (n < 0 || n >= out_sz) return -1;
    return n;
}
