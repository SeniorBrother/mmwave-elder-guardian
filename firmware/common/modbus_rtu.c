/**
 * @file    modbus_rtu.c
 * @brief   Modbus RTU 从站协议栈实现（纯 C，跨平台）
 */
#include <string.h>
#include "modbus_rtu.h"

uint16_t modbus_crc16(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;
    int i, b;
    if (data == 0) return 0;
    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (b = 0; b < 8; b++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;     /* 发送时低字节在前 */
}

void modbus_regs_init(modbus_regs_t *r)
{
    if (r == 0) return;
    memset(r, 0, sizeof(*r));
    /* 默认配置 */
    r->holding[MB_HOLD_NODE_ADDR]    = 1;       /* 从站地址 1 */
    r->holding[MB_HOLD_HR_MAX]       = 110;     /* 心率上限 */
    r->holding[MB_HOLD_TEMP_MAX_X10] = 380;     /* 38.0℃ */
    r->holding[MB_HOLD_CO_MAX]       = 80;      /* CO 上限 */
}

void modbus_load_data(modbus_regs_t *r, const guardian_data_t *d)
{
    int t10, e10;
    if (r == 0 || d == 0) return;

    t10 = (int)(d->body_temp * 10.0f + (d->body_temp >= 0.0f ? 0.5f : -0.5f));
    e10 = d->env_temp * 10;

    r->input[MB_IN_HEART_RATE]    = (uint16_t)d->heart_rate;
    r->input[MB_IN_RESP_RATE]     = (uint16_t)d->resp_rate;
    r->input[MB_IN_PEOPLE]        = (uint16_t)d->people;
    r->input[MB_IN_FALL]          = (uint16_t)d->fall;
    r->input[MB_IN_BODY_TEMP_X10] = (uint16_t)t10;
    r->input[MB_IN_ENV_TEMP_X10]  = (uint16_t)e10;
    r->input[MB_IN_ENV_HUMI]      = (uint16_t)d->env_humi;
    r->input[MB_IN_CO]            = (uint16_t)d->co;
    r->input[MB_IN_ALARM_STATUS]  = (uint16_t)(d->alarm_status & 0xFFFF);
    r->input[MB_IN_FW_VERSION]    = d->fw_version;
}

/* 追加 CRC 到响应尾部，返回新长度 */
static int mb_append_crc(uint8_t *resp, int len)
{
    uint16_t crc = modbus_crc16(resp, len);
    resp[len++] = (uint8_t)(crc & 0xFF);        /* 低字节在前 */
    resp[len++] = (uint8_t)((crc >> 8) & 0xFF);
    return len;
}

/* 生成异常响应，返回帧长 */
static int mb_exception(uint8_t addr, uint8_t fc, uint8_t code,
                        uint8_t *resp, int resp_cap)
{
    if (resp_cap < 5) return 0;
    resp[0] = addr;
    resp[1] = (uint8_t)(fc | 0x80);
    resp[2] = code;
    return mb_append_crc(resp, 3);
}

/* 读寄存器通用处理（0x03/0x04），返回响应长度 */
static int mb_read_regs(uint8_t addr, uint8_t fc, const uint16_t *src, int src_count,
                        const uint8_t *req, uint8_t *resp, int resp_cap)
{
    uint16_t start = (uint16_t)((req[2] << 8) | req[3]);
    uint16_t qty   = (uint16_t)((req[4] << 8) | req[5]);
    int i, len, byte_count;

    if (qty == 0 || qty > 125) {
        return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_VALUE, resp, resp_cap);
    }
    if ((int)start + qty > src_count) {
        return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_ADDR, resp, resp_cap);
    }
    byte_count = qty * 2;
    if (resp_cap < 3 + byte_count + 2) return 0;

    resp[0] = addr;
    resp[1] = fc;
    resp[2] = (uint8_t)byte_count;
    len = 3;
    for (i = 0; i < qty; i++) {
        uint16_t v = src[start + i];
        resp[len++] = (uint8_t)(v >> 8);        /* 高字节在前 */
        resp[len++] = (uint8_t)(v & 0xFF);
    }
    return mb_append_crc(resp, len);
}

int modbus_slave_process(uint8_t slave_addr, modbus_regs_t *regs,
                         const uint8_t *req, int req_len,
                         uint8_t *resp, int resp_cap)
{
    uint16_t crc_recv, crc_calc;
    uint8_t  addr, fc;
    uint16_t start, value, qty;
    int i, len, byte_count;

    if (regs == 0 || req == 0 || resp == 0) return 0;
    if (req_len < 4 || resp_cap < 8) return 0;      /* 最短帧: addr+fc+crc(2) */

    /* 1. 校验 CRC */
    crc_recv = (uint16_t)(req[req_len - 2] | (req[req_len - 1] << 8));
    crc_calc = modbus_crc16(req, req_len - 2);
    if (crc_recv != crc_calc) return 0;             /* CRC 错，静默丢弃 */

    addr = req[0];
    fc   = req[1];

    /* 2. 地址过滤：本机地址或广播(0)。广播不应答。 */
    if (addr != slave_addr && addr != 0) return 0;
    if (addr == 0) {
        /* 广播仅执行写操作，不回响应（此处简化：广播写保持寄存器） */
    }

    switch (fc) {
    /* ---- 0x01 读线圈 ---- */
    case MB_FC_READ_COILS: {
        uint16_t q = (uint16_t)((req[4] << 8) | req[5]);
        start = (uint16_t)((req[2] << 8) | req[3]);
        if (q == 0 || (int)start + q > MB_COIL_COUNT)
            return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_ADDR, resp, resp_cap);
        byte_count = (q + 7) / 8;
        if (resp_cap < 3 + byte_count + 2) return 0;
        resp[0] = addr; resp[1] = fc; resp[2] = (uint8_t)byte_count;
        for (i = 0; i < byte_count; i++) resp[3 + i] = 0;
        for (i = 0; i < (int)q; i++) {
            if (regs->coils[start + i]) resp[3 + i / 8] |= (uint8_t)(1 << (i % 8));
        }
        len = 3 + byte_count;
        if (addr == 0) return 0;
        return mb_append_crc(resp, len);
    }

    /* ---- 0x03 读保持寄存器 ---- */
    case MB_FC_READ_HOLDING:
        if (addr == 0) return 0;
        return mb_read_regs(addr, fc, regs->holding, MB_HOLDING_REG_COUNT, req, resp, resp_cap);

    /* ---- 0x04 读输入寄存器 ---- */
    case MB_FC_READ_INPUT:
        if (addr == 0) return 0;
        return mb_read_regs(addr, fc, regs->input, MB_INPUT_REG_COUNT, req, resp, resp_cap);

    /* ---- 0x05 写单线圈 ---- */
    case MB_FC_WRITE_SINGLE_COIL: {
        start = (uint16_t)((req[2] << 8) | req[3]);
        value = (uint16_t)((req[4] << 8) | req[5]);
        if (start >= MB_COIL_COUNT)
            return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_ADDR, resp, resp_cap);
        if (value != 0x0000 && value != 0xFF00)
            return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_VALUE, resp, resp_cap);
        regs->coils[start] = (value == 0xFF00) ? 1 : 0;
        /* 原样回显请求帧（含 CRC） */
        if (resp_cap < req_len) return 0;
        memcpy(resp, req, (size_t)req_len);
        if (addr == 0) return 0;
        return req_len;
    }

    /* ---- 0x06 写单寄存器 ---- */
    case MB_FC_WRITE_SINGLE_REG: {
        start = (uint16_t)((req[2] << 8) | req[3]);
        value = (uint16_t)((req[4] << 8) | req[5]);
        if (start >= MB_HOLDING_REG_COUNT)
            return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_ADDR, resp, resp_cap);
        regs->holding[start] = value;
        if (resp_cap < req_len) return 0;
        memcpy(resp, req, (size_t)req_len);
        if (addr == 0) return 0;
        return req_len;
    }

    /* ---- 0x10 写多寄存器 ---- */
    case MB_FC_WRITE_MULTI_REG: {
        start = (uint16_t)((req[2] << 8) | req[3]);
        qty   = (uint16_t)((req[4] << 8) | req[5]);
        byte_count = req[6];
        if (qty == 0 || byte_count != qty * 2 || (int)start + qty > MB_HOLDING_REG_COUNT)
            return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_ADDR, resp, resp_cap);
        if (req_len < 7 + byte_count + 2)
            return mb_exception(addr, fc, MB_EX_ILLEGAL_DATA_VALUE, resp, resp_cap);
        for (i = 0; i < (int)qty; i++) {
            regs->holding[start + i] =
                (uint16_t)((req[7 + i * 2] << 8) | req[8 + i * 2]);
        }
        /* 响应: addr + fc + start + qty + crc */
        if (resp_cap < 8) return 0;
        resp[0] = addr; resp[1] = fc;
        resp[2] = req[2]; resp[3] = req[3];
        resp[4] = req[4]; resp[5] = req[5];
        if (addr == 0) return 0;
        return mb_append_crc(resp, 6);
    }

    /* ---- 不支持的功能码 ---- */
    default:
        if (addr == 0) return 0;
        return mb_exception(addr, fc, MB_EX_ILLEGAL_FUNCTION, resp, resp_cap);
    }
}
