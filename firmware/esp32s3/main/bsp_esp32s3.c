/**
 * @file    bsp_esp32s3.c
 * @brief   方案B BSP 实现骨架（ESP32-S3 + ESP-IDF driver）
 *
 * 脚手架：TODO 处填入 ESP-IDF 驱动调用。RS485 用 UART 硬件方向控制自动切换 DE。
 * OTA 用 esp_https_ota（见 components/ota）。引脚见 docs/wiring-esp32s3.md。
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "bsp.h"

/* 引脚（示例，避开 strapping GPIO0/45/46 与 USB GPIO19/20） */
#define PIN_RADAR_UART      UART_NUM_1
#define PIN_RS485_UART      UART_NUM_2
#define PIN_RS485_DE        GPIO_NUM_16
#define PIN_MQ7_ADC         GPIO_NUM_4      /* ADC1_CH3 */

/* 成帧缓冲（被 app_main.c extern 引用） */
volatile uint8_t  g_radar_buf[64];
volatile uint16_t g_radar_frame_len   = 0;
volatile uint8_t  g_radar_frame_ready = 0;
volatile uint8_t  g_rs485_buf[64];
volatile uint16_t g_rs485_frame_len   = 0;
volatile uint8_t  g_rs485_frame_ready = 0;

void bsp_init(void)
{
    /* TODO: uart_driver_install + uart_param_config for radar(115200)/rs485(9600)
       uart_set_pin(...) 映射 TX/RX
       RS485: uart_set_mode(PIN_RS485_UART, UART_MODE_RS485_HALF_DUPLEX)
              并把 DE 脚交给 UART 自动控制: gpio_matrix_out(PIN_RS485_DE, ...)
       adc / i2c(MLX90614) / spi(LCD+touch) 初始化 */
}

void bsp_uart_send(int uart_id, const uint8_t *data, int len)
{
    uart_write_bytes((uart_id == 1) ? PIN_RADAR_UART : PIN_RS485_UART, data, (size_t)len);
}

void bsp_rs485_init(void)    { /* TODO: 见 bsp_init 的 RS485 部分 */ }
void bsp_rs485_set_tx(void)  { gpio_set_level(PIN_RS485_DE, 1); }
void bsp_rs485_set_rx(void)  { gpio_set_level(PIN_RS485_DE, 0); }
void bsp_rs485_send(const uint8_t *data, int len)
{
    uart_write_bytes(PIN_RS485_UART, data, (size_t)len);
    uart_wait_tx_done(PIN_RS485_UART, pdMS_TO_TICKS(100));  /* 等发送完成再切回接收 */
    bsp_rs485_set_rx();
}

float bsp_read_body_temp(void) { /* TODO: MLX90614 via i2c */ return 0.0f; }
int   bsp_read_dht11(int *temp, int *humi) { (void)temp;(void)humi; /* TODO: rmt/单总线 */ return -1; }
int   bsp_read_mq7(void) { /* TODO: adc1_get_raw + 减基准换算 */ return 0; }
void  bsp_mq7_calibrate_base(void) { /* TODO */ }

void bsp_buzzer_set(int on) { (void)on; /* TODO: gpio_set_level(BUZZER, on) */ }
void bsp_led_set(int idx, int on) { (void)idx;(void)on; /* TODO */ }
int  bsp_key_read(int idx) { (void)idx; /* TODO: gpio_get_level + 去抖 */ return 0; }

int bsp_wifi_connect(const char *ssid, const char *pass)
{
    (void)ssid; (void)pass;
    /* TODO: esp_wifi STA 模式连接（ESP32-S3 内置 WiFi，无需 AT 指令） */
    return -1;
}
int bsp_mqtt_connect(void) { /* TODO: esp_mqtt_client */ return -1; }
int bsp_mqtt_publish(const char *topic, const char *payload)
{ (void)topic;(void)payload; /* TODO: esp_mqtt_client_publish */ return -1; }
int bsp_mqtt_connected(void) { return 0; }

void bsp_display_init(void) { /* TODO: SPI LCD(ILI9341) + LVGL 移植, 可用 PSRAM 做大 draw buffer */ }
void bsp_display_flush(int x1, int y1, int x2, int y2, const uint16_t *color_p)
{ (void)x1;(void)y1;(void)x2;(void)y2;(void)color_p; /* TODO */ }
int  bsp_touch_read(int *x, int *y) { (void)x;(void)y; /* TODO: XPT2046 */ return 0; }

int bsp_ota_begin(uint32_t fw_size) { (void)fw_size; /* TODO: esp_ota_begin */ return -1; }
int bsp_ota_write(const uint8_t *data, int len) { (void)data;(void)len; /* TODO: esp_ota_write */ return -1; }
int bsp_ota_finish(void) { /* TODO: esp_ota_end + esp_ota_set_boot_partition */ return -1; }

uint32_t bsp_millis(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
void bsp_delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
void bsp_reboot(void) { esp_restart(); }
