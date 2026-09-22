/**
 * @file    bsp.h
 * @brief   板级支持包(BSP)抽象接口 —— STM32F103 与 ESP32-S3 各自实现
 *
 * 业务层(guardian_app / common 模块)只依赖本头文件的抽象接口，
 * 不直接触碰 HAL / ESP-IDF，从而实现"一套业务、两套 BSP"的跨平台复用。
 *
 * 每个平台在 firmware/<target>/BSP/ 下提供 bsp.c 实现下列函数。
 */
#ifndef BSP_H
#define BSP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 生命周期 ---------------- */
void bsp_init(void);                 /* 时钟/引脚/外设/RTOS 之前的一切初始化 */

/* ---------------- 串口(雷达/WiFi/调试) ---------------- */
/* 发送一帧字节（阻塞或 DMA 由平台决定） */
void bsp_uart_send(int uart_id, const uint8_t *data, int len);

/* ---------------- RS485 (半双工) ---------------- */
void bsp_rs485_init(void);
void bsp_rs485_set_tx(void);         /* DE/RE 拉高，进入发送 */
void bsp_rs485_set_rx(void);         /* DE/RE 拉低，进入接收 */
void bsp_rs485_send(const uint8_t *data, int len);  /* 发送后需等TC再切回rx */

/* ---------------- 传感器 ---------------- */
float bsp_read_body_temp(void);      /* MLX90614, ℃ */
int   bsp_read_dht11(int *temp, int *humi);  /* 返回0成功 */
int   bsp_read_mq7(void);            /* 已做基准补偿的 0~100 相对浓度 */
void  bsp_mq7_calibrate_base(void);  /* 开机预热后采基准 */

/* ---------------- 输出(声光) ---------------- */
void bsp_buzzer_set(int on);         /* 蜂鸣器 */
void bsp_led_set(int idx, int on);   /* LED: 0运行 1上传 2雷达 */

/* ---------------- 按键 ---------------- */
int  bsp_key_read(int idx);          /* 0=K1解除 1=K2 SOS，返回1表示按下 */

/* ---------------- 网络(上云) ---------------- */
int  bsp_wifi_connect(const char *ssid, const char *pass);   /* 返回0成功 */
int  bsp_mqtt_connect(void);
int  bsp_mqtt_publish(const char *topic, const char *payload);
int  bsp_mqtt_connected(void);

/* ---------------- 显示(LVGL) ---------------- */
void bsp_display_init(void);
void bsp_display_flush(int x1, int y1, int x2, int y2, const uint16_t *color_p);
int  bsp_touch_read(int *x, int *y);  /* 返回1表示按下 */

/* ---------------- OTA ---------------- */
int  bsp_ota_begin(uint32_t fw_size);
int  bsp_ota_write(const uint8_t *data, int len);
int  bsp_ota_finish(void);           /* 校验通过置升级标志 */

/* ---------------- 系统 ---------------- */
uint32_t bsp_millis(void);           /* 毫秒计时 */
void     bsp_delay_ms(uint32_t ms);
void     bsp_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */
