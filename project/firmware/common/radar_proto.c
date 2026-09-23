/**
 * @file    radar_proto.c
 * @brief   R60ABD1 60GHz 毫米波雷达协议解析实现（纯 C，跨平台）
 */
#include "radar_proto.h"

uint8_t radar_checksum(const uint8_t *frame, int len)
{
    uint32_t sum = 0;
    int i;
    /* 校验和覆盖 index 0~6（即帧第 1~7 字节），index 7 存放校验和本身 */
    int n = (len < 7) ? len : 7;
    if (frame == 0) return 0;
    for (i = 0; i < n; i++) {
        sum += frame[i];
    }
    return (uint8_t)(sum & 0xFF);
}

int radar_frame_valid(const uint8_t *frame, int len)
{
    if (frame == 0 || len < RADAR_FRAME_LEN) return 0;
    if (frame[0] != RADAR_HEAD0 || frame[1] != RADAR_HEAD1) return 0;
    if (frame[8] != RADAR_TAIL0 || frame[9] != RADAR_TAIL1) return 0;
    if (frame[7] != radar_checksum(frame, len)) return 0;
    return 1;
}

int radar_parse_frame(const uint8_t *frame, int len, radar_evt_t *evt)
{
    if (evt == 0) return 0;
    evt->type  = RADAR_EVT_NONE;
    evt->value = 0;

    if (!radar_frame_valid(frame, len)) return 0;

    /* frame[2]=类型, frame[3]=子命令, frame[6]=数据 */
    switch (frame[2]) {
    case RADAR_TYPE_STATUS:                     /* 0x80 存在 / 运动 */
        switch (frame[3]) {
        case 0x01:                              /* 人体存在 */
            evt->type  = RADAR_EVT_PRESENCE;
            evt->value = (frame[6] == 0x01) ? 1 : 0;
            break;
        case 0x02:                              /* 运动状态 */
            evt->type  = RADAR_EVT_MOTION;
            evt->value = frame[6];
            break;
        default:
            break;
        }
        break;

    case RADAR_TYPE_RESP:                       /* 0x81 呼吸 */
        switch (frame[3]) {
        case 0x02:                              /* 呼吸频率值 0~25 */
            evt->type  = RADAR_EVT_RESP_RATE;
            evt->value = frame[6];
            break;
        default:
            break;
        }
        break;

    case RADAR_TYPE_SLEEP:                      /* 0x84 在离床 / 睡眠 */
        switch (frame[3]) {
        case 0x01:                              /* 在床/离床 */
            evt->type  = RADAR_EVT_IN_BED;
            evt->value = frame[6];
            break;
        case 0x02:                              /* 睡眠状态 0/1/2 */
            evt->type  = RADAR_EVT_SLEEP_STATE;
            evt->value = frame[6];
            break;
        default:
            break;
        }
        break;

    case RADAR_TYPE_HEART:                      /* 0x85 心率 / 心跳 */
        switch (frame[3]) {
        case 0x02:                              /* 心率值 0~100 */
            evt->type  = RADAR_EVT_HEART_RATE;
            evt->value = frame[6];
            break;
        case 0x05:                              /* 心率参数异常 */
            evt->type  = RADAR_EVT_HR_ABNORMAL;
            evt->value = frame[6];
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    return 1;   /* 帧合法（即便 evt->type 为 NONE） */
}

int radar_build_enable_heartbeat(uint8_t *out)
{
    /* 53 59 85 00 00 01 01 3D 54 43 —— 开启心跳/生命体征检测 */
    static const uint8_t cmd[RADAR_FRAME_LEN] = {
        0x53, 0x59, 0x85, 0x00, 0x00, 0x01, 0x01, 0x3D, 0x54, 0x43
    };
    int i;
    if (out == 0) return 0;
    for (i = 0; i < RADAR_FRAME_LEN; i++) out[i] = cmd[i];
    return RADAR_FRAME_LEN;
}

int radar_build_disable_heartbeat(uint8_t *out)
{
    /* 53 59 85 00 00 01 00 3C 54 43 —— 关闭生命体征检测 */
    static const uint8_t cmd[RADAR_FRAME_LEN] = {
        0x53, 0x59, 0x85, 0x00, 0x00, 0x01, 0x00, 0x3C, 0x54, 0x43
    };
    int i;
    if (out == 0) return 0;
    for (i = 0; i < RADAR_FRAME_LEN; i++) out[i] = cmd[i];
    return RADAR_FRAME_LEN;
}
