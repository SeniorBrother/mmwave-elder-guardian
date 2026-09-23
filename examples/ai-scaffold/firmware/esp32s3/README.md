# firmware/esp32s3 —— 方案 B（ESP32-S3, ESP-IDF）

主控 ESP32-S3（双核 LX7@240MHz，自带 WiFi/BLE，N16R8 = 16MB Flash/8MB PSRAM），框架 ESP-IDF v5.x + FreeRTOS + LVGL。省去外挂 ESP8266，OTA 走原生双分区 + 自动回滚。

## 目录

```
esp32s3/
├── main/
│   ├── app_main.c          入口 + 5 任务（雷达/传感器/报警/网络/Modbus），复用 common 业务层
│   ├── bsp_esp32s3.c       bsp.h 接口的 ESP-IDF driver 实现骨架
│   └── CMakeLists.txt
├── components/             lvgl / radar / onenet / ota / modbus（按需扩展）
├── partitions.csv          双 OTA 分区表（ota_0/ota_1 + storage）
├── sdkconfig.defaults      PSRAM / 分区表 / OTA 回滚 / FreeRTOS 配置
└── CMakeLists.txt          顶层，EXTRA_COMPONENT_DIRS 引入 ../common
```

## 构建步骤

```bash
# 安装 ESP-IDF v5.x 后
. $IDF_PATH/export.sh
cd firmware/esp32s3
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

`../common` 通过顶层 `CMakeLists.txt` 的 `EXTRA_COMPONENT_DIRS` 作为组件引入，
业务逻辑与 STM32 方案完全一致，只需实现 `bsp_esp32s3.c` 里的 TODO。

## 关键点

- **OTA 防变砖**：`sdkconfig.defaults` 已开 `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`；
  新固件启动后 `app_main.c` 的 `mark_app_valid()` 会调 `esp_ota_mark_app_valid_cancel_rollback()`。
- **RS485**：用 `uart_set_mode(UART_MODE_RS485_HALF_DUPLEX)` 让硬件自动控制 DE，省去手动切方向。
- **引脚**：避开 strapping GPIO0/45/46 与 USB GPIO19/20；MQ7 用 ADC1(GPIO1~10)。
- **LVGL**：可借 PSRAM 开大 draw buffer，界面更流畅；支持更大分辨率屏。

## 与方案 A 的差异

| 项 | 方案A STM32 | 方案B ESP32-S3 |
|----|-------------|----------------|
| WiFi | 外挂 ESP8266 | 内置 |
| OTA | 自建 IAP + W25Q64 | 原生双分区 + 回滚 |
| 框架 | CubeMX+Keil+HAL | ESP-IDF |
| 算力/内存 | 72MHz/256KB/48KB | 240MHz双核/16MB/+8MB PSRAM |
| 业务层 | `common/`（共用） | `common/`（共用） |
