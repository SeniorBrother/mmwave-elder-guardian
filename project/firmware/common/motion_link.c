/**
 * @file    motion_link.c
 * @brief   双主控运动控制协议实现（小端序列化 + 累加和校验）
 */
#include <string.h>
#include "motion_link.h"

uint8_t ml_checksum(const uint8_t *data, int len)
{
    uint32_t sum = 0;
    int i;
    if (data == 0) return 0;
    for (i = 0; i < len; i++) sum += data[i];
    return (uint8_t)(sum & 0xFF);
}

/* 组装一帧：head+type+len+payload+checksum，返回总长或 <0 */
static int build_frame(uint8_t *out, int cap, uint8_t type,
                       const uint8_t *payload, uint8_t plen)
{
    int total = ML_OVERHEAD + plen;
    if (out == 0 || cap < total || plen > ML_MAX_PAYLOAD) return -1;
    out[0] = ML_HEAD0;
    out[1] = ML_HEAD1;
    out[2] = type;
    out[3] = plen;
    if (plen) memcpy(&out[4], payload, plen);
    out[4 + plen] = ml_checksum(out, 4 + plen);
    return total;
}

static void put_i16(uint8_t *p, int16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);          /* 小端：低字节在前 */
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
static int16_t get_i16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

int ml_encode_velocity(uint8_t *out, int cap, const ml_velocity_t *v)
{
    uint8_t p[6];
    if (v == 0) return -1;
    put_i16(&p[0], v->vx);
    put_i16(&p[2], v->vy);
    put_i16(&p[4], v->wz);
    return build_frame(out, cap, ML_CMD_VELOCITY, p, sizeof(p));
}

int ml_encode_motor_pwm(uint8_t *out, int cap, const ml_motor_pwm_t *m)
{
    if (m == 0) return -1;
    return build_frame(out, cap, ML_CMD_MOTOR_PWM, (const uint8_t *)m->pwm, 4);
}

int ml_encode_enable(uint8_t *out, int cap, uint8_t enable)
{
    return build_frame(out, cap, ML_CMD_ENABLE, &enable, 1);
}

int ml_encode_chassis(uint8_t *out, int cap, const ml_chassis_t *c)
{
    uint8_t p[8];
    if (c == 0) return -1;
    put_i16(&p[0], c->x_mm);
    put_i16(&p[2], c->y_mm);
    put_i16(&p[4], c->heading);
    p[6] = c->battery;
    p[7] = c->fault;
    return build_frame(out, cap, ML_RSP_CHASSIS, p, sizeof(p));
}

int ml_encode_vitals(uint8_t *out, int cap, const ml_vitals_t *v)
{
    uint8_t p[6];
    if (v == 0) return -1;
    p[0] = v->heart_rate;
    p[1] = v->resp_rate;
    p[2] = v->people;
    p[3] = v->fall;
    p[4] = v->body_temp_x10_lo;
    p[5] = v->body_temp_x10_hi;
    return build_frame(out, cap, ML_RSP_VITALS, p, sizeof(p));
}

int ml_decode(const uint8_t *frame, int len, ml_frame_t *out)
{
    if (frame == 0 || out == 0) return 0;
    if (len < ML_OVERHEAD) return 0;
    if (frame[0] != ML_HEAD0 || frame[1] != ML_HEAD1) return 0;

    uint8_t plen = frame[3];
    if (plen > ML_MAX_PAYLOAD) return 0;
    if (len < ML_OVERHEAD + plen) return 0;

    /* 校验和覆盖 head..payload */
    if (frame[4 + plen] != ml_checksum(frame, 4 + plen)) return 0;

    out->type = frame[2];
    out->len  = plen;
    if (plen) memcpy(out->payload, &frame[4], plen);
    return 1;
}

int ml_parse_velocity(const ml_frame_t *f, ml_velocity_t *v)
{
    if (f == 0 || v == 0) return 0;
    if (f->type != ML_CMD_VELOCITY || f->len != 6) return 0;
    v->vx = get_i16(&f->payload[0]);
    v->vy = get_i16(&f->payload[2]);
    v->wz = get_i16(&f->payload[4]);
    return 1;
}

int ml_parse_motor_pwm(const ml_frame_t *f, ml_motor_pwm_t *m)
{
    if (f == 0 || m == 0) return 0;
    if (f->type != ML_CMD_MOTOR_PWM || f->len != 4) return 0;
    memcpy(m->pwm, f->payload, 4);
    return 1;
}

int ml_parse_enable(const ml_frame_t *f, uint8_t *enable)
{
    if (f == 0 || enable == 0) return 0;
    if (f->type != ML_CMD_ENABLE || f->len != 1) return 0;
    *enable = f->payload[0];
    return 1;
}

int ml_parse_chassis(const ml_frame_t *f, ml_chassis_t *c)
{
    if (f == 0 || c == 0) return 0;
    if (f->type != ML_RSP_CHASSIS || f->len != 8) return 0;
    c->x_mm    = get_i16(&f->payload[0]);
    c->y_mm    = get_i16(&f->payload[2]);
    c->heading = get_i16(&f->payload[4]);
    c->battery = f->payload[6];
    c->fault   = f->payload[7];
    return 1;
}

int ml_parse_vitals(const ml_frame_t *f, ml_vitals_t *v)
{
    if (f == 0 || v == 0) return 0;
    if (f->type != ML_RSP_VITALS || f->len != 6) return 0;
    v->heart_rate      = f->payload[0];
    v->resp_rate       = f->payload[1];
    v->people          = f->payload[2];
    v->fall            = f->payload[3];
    v->body_temp_x10_lo= f->payload[4];
    v->body_temp_x10_hi= f->payload[5];
    return 1;
}
