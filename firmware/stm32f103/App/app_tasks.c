/**
 * @file    app_tasks.c
 * @brief   STM32F103RCT6 方案A：FreeRTOS 任务实现（调用 common 业务层 + BSP）
 *
 * 依赖：STM32CubeMX 生成的 FreeRTOS(CMSIS-RTOS v2) + HAL。
 * 本文件展示任务如何组织，真正的引脚/外设细节在 BSP/bsp_stm32f103.c。
 *
 * 任务清单：
 *   Task_Radar   解析雷达帧 -> app_on_radar_frame
 *   Task_Sensor  周期读传感器 -> app_on_sensors
 *   Task_GUI     LVGL 刷新 + 触控
 *   Task_Net     WiFi/MQTT 上云 -> app_build_cloud_json + bsp_mqtt_publish
 *   Task_Alarm   报警评估 -> app_update + 蜂鸣器/LED
 *   Task_Modbus  RS485 收帧 -> app_handle_modbus -> 回发
 */
#include "cmsis_os2.h"
#include "bsp.h"
#include "guardian_app.h"

/* 全局业务上下文（受 g_data_mutex 保护） */
static guardian_app_t  g_app;
static osMutexId_t     g_data_mutex;

/* 串口成帧标志（由各 UART 空闲/定时器中断置位，见 BSP） */
extern volatile uint8_t g_radar_frame_ready;
extern volatile uint16_t g_radar_frame_len;
extern volatile uint8_t g_radar_buf[64];

extern volatile uint8_t g_rs485_frame_ready;
extern volatile uint16_t g_rs485_frame_len;
extern volatile uint8_t g_rs485_buf[64];

/* OneNet 产品参数（替换为自己的） */
#define ONENET_PID   "8w7dxp3Pj8"
#define ONENET_DEV   "dev1"
#define WIFI_SSID    "your-ssid"
#define WIFI_PASS    "your-pass"

static void lock(void)   { osMutexAcquire(g_data_mutex, osWaitForever); }
static void unlock(void) { osMutexRelease(g_data_mutex); }

/* ---------------- Task_Radar ---------------- */
static void Task_Radar(void *arg)
{
    (void)arg;
    for (;;) {
        if (g_radar_frame_ready) {
            uint8_t  tmp[64];
            uint16_t len = g_radar_frame_len;
            g_radar_frame_ready = 0;
            if (len <= sizeof(tmp)) {
                memcpy(tmp, (const void *)g_radar_buf, len);
                lock();
                app_on_radar_frame(&g_app, tmp, len);
                unlock();
                bsp_led_set(2, 1); bsp_delay_ms(20); bsp_led_set(2, 0);
            }
        }
        osDelay(5);
    }
}

/* ---------------- Task_Sensor ---------------- */
static void Task_Sensor(void *arg)
{
    (void)arg;
    int t, h;
    bsp_delay_ms(20000);            /* MQ7 预热 */
    bsp_mq7_calibrate_base();
    for (;;) {
        float body = bsp_read_body_temp();
        bsp_read_dht11(&t, &h);
        int co = bsp_read_mq7();
        lock();
        app_on_sensors(&g_app, body, t, h, co);
        unlock();
        osDelay(2000);
    }
}

/* ---------------- Task_Alarm ---------------- */
static void Task_Alarm(void *arg)
{
    (void)arg;
    for (;;) {
        lock();
        app_update(&g_app);
        int buzz = app_buzzer_should_on(&g_app);
        /* 本地按键：K1 解除，K2 SOS */
        if (bsp_key_read(0)) app_clear_alarm(&g_app);
        if (bsp_key_read(1)) app_trigger_sos(&g_app);
        unlock();
        bsp_buzzer_set(buzz);
        bsp_led_set(0, 1);          /* 运行灯心跳 */
        osDelay(200);
        bsp_led_set(0, 0);
        osDelay(200);
    }
}

/* ---------------- Task_Net ---------------- */
static void Task_Net(void *arg)
{
    (void)arg;
    char topic[64], payload[256];
    onenet_build_post_topic(ONENET_PID, ONENET_DEV, topic, sizeof(topic));

    while (bsp_wifi_connect(WIFI_SSID, WIFI_PASS) != 0) osDelay(3000);
    while (bsp_mqtt_connect() != 0) osDelay(2000);

    for (;;) {
        if (bsp_mqtt_connected()) {
            int n;
            lock();
            n = app_build_cloud_json(&g_app, payload, sizeof(payload));
            unlock();
            if (n > 0) {
                bsp_mqtt_publish(topic, payload);
                bsp_led_set(1, 1); bsp_delay_ms(80); bsp_led_set(1, 0);
            }
        }
        osDelay(10000);             /* 长期部署建议 10~30s 一次 */
    }
}

/* ---------------- Task_Modbus (RS485) ---------------- */
static void Task_Modbus(void *arg)
{
    (void)arg;
    for (;;) {
        if (g_rs485_frame_ready) {
            uint8_t req[64], resp[64];
            uint16_t len = g_rs485_frame_len;
            g_rs485_frame_ready = 0;
            if (len <= sizeof(req)) {
                memcpy(req, (const void *)g_rs485_buf, len);
                int n;
                lock();
                n = app_handle_modbus(&g_app, req, len, resp, sizeof(resp));
                if (n > 0) app_apply_coils(&g_app);
                unlock();
                if (n > 0) {
                    bsp_rs485_set_tx();
                    bsp_rs485_send(resp, n);   /* 内部等 TC 完成后切回 rx */
                }
            }
        }
        osDelay(2);
    }
}

/* ---------------- Task_GUI (LVGL) ---------------- */
extern void lvgl_loop(void);   /* 在 UI/lvgl_port.c 实现: lv_timer_handler() + 绑定数据 */
static void Task_GUI(void *arg)
{
    (void)arg;
    bsp_display_init();
    for (;;) {
        lock();
        /* 把 g_app.data 传给 LVGL 控件刷新（见 UI 层实现） */
        unlock();
        lvgl_loop();
        osDelay(30);            /* ~33fps */
    }
}

/* ---------------- 创建所有任务 ---------------- */
void app_tasks_create(void)
{
    g_data_mutex = osMutexNew(NULL);
    app_init(&g_app, 1);        /* Modbus 从站地址=1，多节点时改这里 */

    osThreadNew(Task_Radar,  NULL, NULL);
    osThreadNew(Task_Sensor, NULL, NULL);
    osThreadNew(Task_Alarm,  NULL, NULL);
    osThreadNew(Task_Net,    NULL, NULL);
    osThreadNew(Task_Modbus, NULL, NULL);
    osThreadNew(Task_GUI,    NULL, NULL);
}

/* 供 UI 层只读访问业务数据 */
const guardian_app_t *app_get(void) { return &g_app; }
guardian_app_t *app_get_mut(void)   { return &g_app; }
osMutexId_t app_mutex(void)         { return g_data_mutex; }
