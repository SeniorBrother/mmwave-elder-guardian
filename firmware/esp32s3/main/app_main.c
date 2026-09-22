/**
 * @file    app_main.c
 * @brief   方案B ESP32-S3 (ESP-IDF) 入口与任务，复用 firmware/common 业务层。
 *
 * 与 STM32 方案的差别：WiFi/BLE 内置（无需 ESP8266），OTA 走原生 esp_https_ota
 * 双分区 + 自动回滚；RS485 可用 UART 硬件方向控制。业务逻辑完全一致。
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"

#include "bsp.h"
#include "guardian_app.h"
#include "onenet_json.h"

static const char *TAG = "guardian";

static guardian_app_t   g_app;
static SemaphoreHandle_t g_mutex;

/* RS485/雷达 成帧缓冲（由 UART 事件任务填充，见 bsp_esp32s3.c） */
extern volatile uint8_t  g_radar_buf[64];
extern volatile uint16_t g_radar_frame_len;
extern volatile uint8_t  g_radar_frame_ready;
extern volatile uint8_t  g_rs485_buf[64];
extern volatile uint16_t g_rs485_frame_len;
extern volatile uint8_t  g_rs485_frame_ready;

#define ONENET_PID "8w7dxp3Pj8"
#define ONENET_DEV "dev1"
#define WIFI_SSID  "your-ssid"
#define WIFI_PASS  "your-pass"

static void lock(void)   { xSemaphoreTake(g_mutex, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(g_mutex); }

static void task_radar(void *arg)
{
    (void)arg;
    for (;;) {
        if (g_radar_frame_ready) {
            uint8_t tmp[64]; uint16_t len = g_radar_frame_len;
            g_radar_frame_ready = 0;
            if (len <= sizeof(tmp)) {
                memcpy(tmp, (const void *)g_radar_buf, len);
                lock(); app_on_radar_frame(&g_app, tmp, len); unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void task_sensor(void *arg)
{
    (void)arg;
    int t, h;
    vTaskDelay(pdMS_TO_TICKS(20000));   /* MQ7 预热 */
    bsp_mq7_calibrate_base();
    for (;;) {
        float body = bsp_read_body_temp();
        bsp_read_dht11(&t, &h);
        int co = bsp_read_mq7();
        lock(); app_on_sensors(&g_app, body, t, h, co); unlock();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void task_alarm(void *arg)
{
    (void)arg;
    for (;;) {
        lock();
        app_update(&g_app);
        int buzz = app_buzzer_should_on(&g_app);
        if (bsp_key_read(0)) app_clear_alarm(&g_app);
        if (bsp_key_read(1)) app_trigger_sos(&g_app);
        unlock();
        bsp_buzzer_set(buzz);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void task_net(void *arg)
{
    (void)arg;
    char topic[64], payload[256];
    onenet_build_post_topic(ONENET_PID, ONENET_DEV, topic, sizeof(topic));
    while (bsp_wifi_connect(WIFI_SSID, WIFI_PASS) != 0) vTaskDelay(pdMS_TO_TICKS(3000));
    while (bsp_mqtt_connect() != 0) vTaskDelay(pdMS_TO_TICKS(2000));
    for (;;) {
        if (bsp_mqtt_connected()) {
            int n; lock(); n = app_build_cloud_json(&g_app, payload, sizeof(payload)); unlock();
            if (n > 0) bsp_mqtt_publish(topic, payload);
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

static void task_modbus(void *arg)
{
    (void)arg;
    for (;;) {
        if (g_rs485_frame_ready) {
            uint8_t req[64], resp[64]; uint16_t len = g_rs485_frame_len;
            g_rs485_frame_ready = 0;
            if (len <= sizeof(req)) {
                memcpy(req, (const void *)g_rs485_buf, len);
                int n; lock();
                n = app_handle_modbus(&g_app, req, len, resp, sizeof(resp));
                if (n > 0) app_apply_coils(&g_app);
                unlock();
                if (n > 0) { bsp_rs485_set_tx(); bsp_rs485_send(resp, n); }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static void mark_app_valid(void)
{
    /* OTA 新固件启动后确认有效，取消回滚（防变砖关键步骤） */
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t st;
    if (esp_ota_get_state_partition(running, &st) == ESP_OK) {
        ESP_LOGI(TAG, "OTA partition state: %d", st);
        if (st == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "OTA app marked valid, rollback cancelled");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "mmwave-elder-guardian ESP32-S3 booting");
    bsp_init();
    bsp_display_init();
    mark_app_valid();

    g_mutex = xSemaphoreCreateMutex();
    app_init(&g_app, 1);

    xTaskCreate(task_radar,  "radar",  4096, NULL, 5, NULL);
    xTaskCreate(task_sensor, "sensor", 4096, NULL, 4, NULL);
    xTaskCreate(task_alarm,  "alarm",  4096, NULL, 6, NULL);
    xTaskCreate(task_net,    "net",    6144, NULL, 4, NULL);
    xTaskCreate(task_modbus, "modbus", 4096, NULL, 5, NULL);
    /* LVGL 主循环任务在 UI/lvgl_port.c 创建 */
}
