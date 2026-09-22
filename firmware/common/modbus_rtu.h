/**
 * @file    modbus_rtu.h
 * @brief   Modbus RTU 从站协议栈（纯 C，跨平台，不含硬件收发）
 *
 * 职责：给入一帧完整请求（含 CRC），校验地址与 CRC，执行功能码，产出响应帧。
 * 硬件收发（UART + RS485 方向切换）由各平台 BSP 负责。
 *
 * 支持功能码：
 *   0x01 读线圈            0x05 写单线圈
 *   0x03 读保持寄存器      0x06 写单寄存器
 *   0x04 读输入寄存器      0x10 写多寄存器
 */
#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>
#include "guardian_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 功能码 */
#define MB_FC_READ_COILS            0x01
#define MB_FC_READ_HOLDING          0x03
#define MB_FC_READ_INPUT            0x04
#define MB_FC_WRITE_SINGLE_COIL     0x05
#define MB_FC_WRITE_SINGLE_REG      0x06
#define MB_FC_WRITE_MULTI_REG       0x10

/* 异常码 */
#define MB_EX_ILLEGAL_FUNCTION      0x01
#define MB_EX_ILLEGAL_DATA_ADDR     0x02
#define MB_EX_ILLEGAL_DATA_VALUE    0x03
#define MB_EX_SLAVE_DEVICE_FAILURE  0x04

/* 寄存器/线圈数量（可按需扩展） */
#define MB_INPUT_REG_COUNT      10  /* 只读：传感器数据，见寄存器映射 */
#define MB_HOLDING_REG_COUNT    8   /* 读写：配置（阈值/节点地址等） */
#define MB_COIL_COUNT           4   /* 线圈：解除报警/SOS/蜂鸣器/LED */

/* 输入寄存器地址（与 docs/modbus-map.md 一致） */
#define MB_IN_HEART_RATE        0x0000
#define MB_IN_RESP_RATE         0x0001
#define MB_IN_PEOPLE            0x0002
#define MB_IN_FALL              0x0003
#define MB_IN_BODY_TEMP_X10     0x0004
#define MB_IN_ENV_TEMP_X10      0x0005
#define MB_IN_ENV_HUMI          0x0006
#define MB_IN_CO                0x0007
#define MB_IN_ALARM_STATUS      0x0008
#define MB_IN_FW_VERSION        0x0009

/* 线圈地址 */
#define MB_COIL_CLEAR_ALARM     0x0000  /* 主站写1：解除报警 */
#define MB_COIL_SOS             0x0001  /* 主站写1：触发SOS测试 */
#define MB_COIL_BUZZER          0x0002  /* 蜂鸣器强制开关 */
#define MB_COIL_LED             0x0003  /* 指示灯强制开关 */

/* 保持寄存器地址（配置） */
#define MB_HOLD_NODE_ADDR       0x0000  /* 本节点 Modbus 地址 */
#define MB_HOLD_HR_MAX          0x0001  /* 心率报警上限 */
#define MB_HOLD_TEMP_MAX_X10    0x0002  /* 体温报警上限(x10) */
#define MB_HOLD_CO_MAX          0x0003  /* CO报警上限 */

/**
 * @brief Modbus 从站数据区（寄存器 + 线圈）
 */
typedef struct {
    uint16_t input[MB_INPUT_REG_COUNT];
    uint16_t holding[MB_HOLDING_REG_COUNT];
    uint8_t  coils[MB_COIL_COUNT];
} modbus_regs_t;

/**
 * @brief 计算 Modbus RTU CRC16（多项式 0xA001，初值 0xFFFF）
 */
uint16_t modbus_crc16(const uint8_t *data, int len);

/**
 * @brief 初始化寄存器区（清零 + 默认配置）
 */
void modbus_regs_init(modbus_regs_t *r);

/**
 * @brief 把监测数据刷新到输入寄存器（体温/室温放大10倍存整数）
 */
void modbus_load_data(modbus_regs_t *r, const guardian_data_t *d);

/**
 * @brief 处理一帧 Modbus RTU 请求，生成响应
 * @param slave_addr 本机从站地址
 * @param regs       寄存器区
 * @param req        请求帧（含 CRC）
 * @param req_len    请求长度
 * @param resp       [out] 响应帧缓冲（含 CRC，由调用方发送）
 * @param resp_cap   响应缓冲容量
 * @return >0 响应帧长度（需回发）；0 无需响应（广播/地址不符/CRC错）
 */
int modbus_slave_process(uint8_t slave_addr, modbus_regs_t *regs,
                         const uint8_t *req, int req_len,
                         uint8_t *resp, int resp_cap);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RTU_H */
