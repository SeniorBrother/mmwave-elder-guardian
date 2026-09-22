/**
 * @file    radar_proto.h
 * @brief   海凌科 R60ABD1 60GHz 毫米波雷达串口协议解析（纯 C，跨平台）
 *
 * 帧格式（定长 10 字节）：
 *   [0]=0x53 [1]=0x59 [2]=类型 [3]=子命令 [4..5]=保留 [6]=数据 [7]=校验和 [8]=0x54 [9]=0x43
 *   校验和 = (字节0~6 累加) & 0xFF
 *
 * 本模块不依赖任何硬件：喂入完整一帧字节，返回解析出的事件。
 */
#ifndef RADAR_PROTO_H
#define RADAR_PROTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RADAR_FRAME_LEN     10      /* R60ABD1 定长帧 */
#define RADAR_HEAD0         0x53    /* 帧头字节1 */
#define RADAR_HEAD1         0x59    /* 帧头字节2 */
#define RADAR_TAIL0         0x54    /* 帧尾字节1 */
#define RADAR_TAIL1         0x43    /* 帧尾字节2 */

/* 消息类型（帧第 3 字节 / index 2） */
#define RADAR_TYPE_STATUS   0x80    /* 人体存在 / 运动 */
#define RADAR_TYPE_RESP     0x81    /* 呼吸 */
#define RADAR_TYPE_SLEEP    0x84    /* 睡眠 / 在离床 */
#define RADAR_TYPE_HEART    0x85    /* 心率 / 心跳 */

/* 解析出的事件类型 */
typedef enum {
    RADAR_EVT_NONE = 0,
    RADAR_EVT_PRESENCE,     /* 人体存在, value: 0=无人 1=有人 */
    RADAR_EVT_MOTION,       /* 运动状态, value: 0静止 1运动 2剧烈 */
    RADAR_EVT_RESP_RATE,    /* 呼吸频率, value: 0~25 */
    RADAR_EVT_HEART_RATE,   /* 心率,     value: 0~100 */
    RADAR_EVT_IN_BED,       /* 在离床,   value: 平台定义 */
    RADAR_EVT_SLEEP_STATE,  /* 睡眠状态, value: 0无 1浅睡 2深睡 */
    RADAR_EVT_HR_ABNORMAL   /* 心率参数异常 */
} radar_evt_type_t;

typedef struct {
    radar_evt_type_t type;
    int value;
} radar_evt_t;

/**
 * @brief 计算 R60ABD1 帧校验和（字节0~6 累加取低8位）
 */
uint8_t radar_checksum(const uint8_t *frame, int len);

/**
 * @brief 校验一帧是否合法（长度/帧头/帧尾/校验和）
 * @return 1 合法, 0 非法
 */
int radar_frame_valid(const uint8_t *frame, int len);

/**
 * @brief 解析一帧雷达数据，输出事件
 * @param frame 帧缓冲（至少 RADAR_FRAME_LEN 字节）
 * @param len   帧长度
 * @param evt   [out] 解析结果；type==RADAR_EVT_NONE 表示该帧无需处理
 * @return 1 帧合法并已解析（evt 有效，可能为 NONE）；0 帧非法
 */
int radar_parse_frame(const uint8_t *frame, int len, radar_evt_t *evt);

/**
 * @brief 生成"开启生命体征(心跳/呼吸)检测"命令帧
 * @param out 输出缓冲（至少 RADAR_FRAME_LEN 字节）
 * @return 写入的字节数（=RADAR_FRAME_LEN）
 */
int radar_build_enable_heartbeat(uint8_t *out);

/**
 * @brief 生成"关闭生命体征检测"命令帧
 */
int radar_build_disable_heartbeat(uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_PROTO_H */
