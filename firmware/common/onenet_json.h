/**
 * @file    onenet_json.h
 * @brief   OneNet 多协议接入(MQTT) 数据点上报 JSON 组包（纯 C，跨平台）
 *
 * 物模型标识符必须与 OneNet 产品定义完全一致（区分大小写）：
 *   HeartRate / DHT11_T / DHT11_H / MLX90614 / MQ7 / People / Fall / RespiratoryRate
 * 上报主题：$sys/{产品ID}/{设备名}/dp/post/json
 */
#ifndef ONENET_JSON_H
#define ONENET_JSON_H

#include <stdint.h>
#include "guardian_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OneNet 连接与主题（实际部署替换为自己的产品参数） */
#define ONENET_MQTT_IP          "183.230.40.96"
#define ONENET_MQTT_PORT        1883

/* 组一个主题字符串：$sys/{pid}/{dev}/{suffix} */
#define ONENET_TOPIC_POST_SUFFIX    "dp/post/json"
#define ONENET_TOPIC_SUB_SUFFIX     "#"

/**
 * @brief 组数据点上报 JSON（dp/post/json 格式）
 * @param d      监测数据
 * @param msg_id 消息自增 id
 * @param out    输出缓冲
 * @param out_sz 缓冲大小（建议 >= 256）
 * @return 写入长度（不含结束符），<0 表示缓冲不足
 */
int onenet_build_dp_json(const guardian_data_t *d, int msg_id, char *out, int out_sz);

/**
 * @brief 组订阅主题：$sys/{pid}/{dev}/#
 */
int onenet_build_sub_topic(const char *pid, const char *dev, char *out, int out_sz);

/**
 * @brief 组发布主题：$sys/{pid}/{dev}/dp/post/json
 */
int onenet_build_post_topic(const char *pid, const char *dev, char *out, int out_sz);

#ifdef __cplusplus
}
#endif

#endif /* ONENET_JSON_H */
