/**
 * @file    bsp_stm32f103.c
 * @brief   方案A BSP 实现骨架（STM32F103RCT6 + HAL + CubeMX）
 *
 * 说明：本文件是脚手架。带 CubeMX 生成的句柄(huartX/hspiX/hi2cX/hadc1)后，
 *       把各处 TODO 替换为实际 HAL 调用即可编译。引脚见 hardware/wiring.md。
 *
 * 关键：RS485 半双工方向切换必须等"发送完成(TC)"再拉低 DE/RE，
 *       否则最后一字节会被截断。
 */
#include "bsp.h"
#include <string.h>

/* --- CubeMX 生成的外部句柄 --- */
/* extern UART_HandleTypeDef huart1;   // 调试
   extern UART_HandleTypeDef huart2;   // ESP8266
   extern UART_HandleTypeDef huart3;   // 雷达
   extern UART_HandleTypeDef huart5;   // RS485 (PC12/PD2)
   extern SPI_HandleTypeDef  hspi1;    // 触控屏
   extern SPI_HandleTypeDef  hspi2;    // W25Q64
   extern I2C_HandleTypeDef  hi2c1;    // MLX90614
   extern ADC_HandleTypeDef  hadc1;    // MQ7 (PA1)
*/

/* --- RS485 方向脚 (PC3) --- */
#define RS485_DE_HIGH()   do { /* HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);   */ } while (0)
#define RS485_DE_LOW()    do { /* HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); */ } while (0)

/* --- 蜂鸣器 (PA8, 三极管驱动) --- */
#define BEEP_ON()         do { /* HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);   */ } while (0)
#define BEEP_OFF()        do { /* HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET); */ } while (0)

/* ============ 串口成帧（中断收字节 + 定时器空闲成帧） ============
 * 沿用参考工程套路：UART RX 中断每收一字节喂定时器，20ms 无新字节触发溢出置成帧标志。
 * 下面缓冲区/标志被 app_tasks.c 通过 extern 引用。
 */
volatile uint8_t  g_radar_buf[64];
volatile uint16_t g_radar_frame_len   = 0;
volatile uint8_t  g_radar_frame_ready = 0;
static   uint16_t s_radar_idx = 0;

volatile uint8_t  g_rs485_buf[64];
volatile uint16_t g_rs485_frame_len   = 0;
volatile uint8_t  g_rs485_frame_ready = 0;
static   uint16_t s_rs485_idx = 0;

/* 在 HAL_UART_RxCpltCallback / 字节中断里调用（雷达 USART3） */
void bsp_radar_rx_byte(uint8_t b)
{
    if (g_radar_frame_ready) { s_radar_idx = 0; }   /* 上帧未处理，重置 */
    if (s_radar_idx < sizeof(g_radar_buf)) g_radar_buf[s_radar_idx++] = b;
    /* TODO: 清定时器计数并启动 (TIM3->CNT=0; TIM3->CR1|=1;) */
}
/* 在定时器溢出中断(TIM3)里调用：20ms 无新字节 -> 成帧 */
void bsp_radar_frame_done(void)
{
    g_radar_frame_len   = s_radar_idx;
    g_radar_frame_ready = 1;
    s_radar_idx = 0;
    /* TODO: 关闭定时器 (TIM3->CR1 &= ~1;) */
}

void bsp_rs485_rx_byte(uint8_t b)
{
    if (g_rs485_frame_ready) { s_rs485_idx = 0; }
    if (s_rs485_idx < sizeof(g_rs485_buf)) g_rs485_buf[s_rs485_idx++] = b;
    /* TODO: 喂 Modbus 帧间隔定时器(3.5字符时间) */
}
void bsp_rs485_frame_done(void)
{
    g_rs485_frame_len   = s_rs485_idx;
    g_rs485_frame_ready = 1;
    s_rs485_idx = 0;
}

/* ==================== BSP 接口实现 ==================== */

void bsp_init(void)
{
    /* TODO: CubeMX 已在 main() 里 MX_xxx_Init(); 这里做额外 IO/中断优先级配置 */
    RS485_DE_LOW();   /* 默认接收 */
}

void bsp_uart_send(int uart_id, const uint8_t *data, int len)
{
    (void)uart_id; (void)data; (void)len;
    /* TODO: 按 uart_id 选 huart2(ESP8266)/huart3(雷达)，HAL_UART_Transmit() */
}

void bsp_rs485_init(void) { RS485_DE_LOW(); }
void bsp_rs485_set_tx(void) { RS485_DE_HIGH(); }
void bsp_rs485_set_rx(void) { RS485_DE_LOW(); }

void bsp_rs485_send(const uint8_t *data, int len)
{
    RS485_DE_HIGH();
    /* TODO: HAL_UART_Transmit(&huart5, (uint8_t*)data, len, 100); 阻塞发送 */
    /* TODO: 等发送完成 TC 标志: while(__HAL_UART_GET_FLAG(&huart5, UART_FLAG_TC)==RESET); */
    RS485_DE_LOW();   /* TC 之后再切回接收，避免截断末字节 */
}

float bsp_read_body_temp(void)
{
    /* TODO: MLX90614 软件/硬件 I2C 读取，raw*0.02-273.15，中位数滤波 */
    return 0.0f;
}

int bsp_read_dht11(int *temp, int *humi)
{
    (void)temp; (void)humi;
    /* TODO: 单总线时序读 DHT11 */
    return -1;
}

int bsp_read_mq7(void)
{
    /* TODO: HAL_ADC 多次平均采集 PA1，减基准，线性换算 0~100 */
    return 0;
}

void bsp_mq7_calibrate_base(void)
{
    /* TODO: 预热后采基准值保存到静态变量 */
}

void bsp_buzzer_set(int on) { if (on) BEEP_ON(); else BEEP_OFF(); }

void bsp_led_set(int idx, int on)
{
    (void)idx; (void)on;
    /* TODO: idx 0=PC13运行 1=PC14上传 2=PC15雷达 */
}

int bsp_key_read(int idx)
{
    (void)idx;
    /* TODO: 读 PC0(K1)/PC1(K2)，去抖 */
    return 0;
}

int bsp_wifi_connect(const char *ssid, const char *pass)
{
    (void)ssid; (void)pass;
    /* TODO: ESP8266 AT 指令连 2.4G WiFi (AT+CWJAP) */
    return -1;
}
int bsp_mqtt_connect(void)      { /* TODO: TCP + MQTT_Connect */ return -1; }
int bsp_mqtt_publish(const char *topic, const char *payload) { (void)topic; (void)payload; return -1; }
int bsp_mqtt_connected(void)    { return 0; }

void bsp_display_init(void)  { /* TODO: ILI9341 初始化 + LVGL 移植 (lv_port_disp/indev) */ }
void bsp_display_flush(int x1, int y1, int x2, int y2, const uint16_t *color_p)
{ (void)x1;(void)y1;(void)x2;(void)y2;(void)color_p; /* TODO: SPI DMA 刷屏 + lv_disp_flush_ready */ }
int  bsp_touch_read(int *x, int *y) { (void)x;(void)y; /* TODO: XPT2046 读坐标 */ return 0; }

int bsp_ota_begin(uint32_t fw_size) { (void)fw_size; /* TODO: 擦 W25Q64 缓存区 */ return -1; }
int bsp_ota_write(const uint8_t *data, int len) { (void)data;(void)len; /* TODO: 写 W25Q64 + 累算CRC32 */ return -1; }
int bsp_ota_finish(void) { /* TODO: 校验通过->参数区置升级标志 */ return -1; }

uint32_t bsp_millis(void) { /* TODO: return HAL_GetTick(); */ return 0; }
void bsp_delay_ms(uint32_t ms) { (void)ms; /* TODO: HAL_Delay(ms); */ }
void bsp_reboot(void) { /* TODO: NVIC_SystemReset(); */ }
