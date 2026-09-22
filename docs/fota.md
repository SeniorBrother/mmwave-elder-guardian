# OTA / FOTA 远程升级

## 方案 A：STM32F103RCT6（自建 IAP + OneNet FOTA）

STM32F1 无原生 OTA，需自建 Bootloader + Flash 分区，并用 W25Q64 缓存固件防变砖。

### Flash 分区（256KB，高密度页 2KB）

| 区域 | 地址 | 大小 | 用途 |
|------|------|------|------|
| Bootloader | 0x08000000 | ~24-32KB | IAP 引导，永不被覆盖 |
| APP | 0x08008000 | ~200KB | 应用固件 |
| 参数区 | 末尾页 | 2KB | 版本/升级标志/CRC32/大小 |
| W25Q64 | 外部 SPI | 8MB | 下载缓存 + 上一版备份(回滚) |

### 升级流程

1. OneNet「远程升级」上传新 bin，绑定产品/设备，下发升级任务（携带固件 URL / 大小 / 校验值）。
2. 固件 `Task_OTA` 收到指令 → ESP8266 用 AT（`AT+CIPSTART`/HTTP）从 URL 分块下载写入 W25Q64。
3. 边下边算 CRC32，完成后与平台校验值比对，通过则在参数区置「待升级」标志。
4. 软复位 → Bootloader 检测标志 → 从 W25Q64 搬运到 APP 区 → 擦写后再校验 → 置 APP 有效 → 跳转 APP。
5. APP 启动后上报新版本号确认成功；失败/超时由 Bootloader 回退备份镜像。

### 关键坑

- **APP 必须设 `SCB->VTOR = 0x08008000`**，Keil APP 工程 IROM 起始改 0x08008000，否则中断全乱。
- Bootloader 跳转前设 MSP = APP 首字，跳 APP 复位向量（见 `Bootloader/bootloader.c`）。
- Bootloader 永不被覆盖；下载/搬运分阶段校验；断电重启可幂等重新搬运。

## 方案 B：ESP32-S3（ESP-IDF 原生 OTA）

- 分区表 `partitions.csv`：nvs / otadata / phy_init / ota_0 / ota_1 / storage(spiffs)。
- 用 `esp_https_ota` 从 OneNet FOTA URL 下载 → 写备用 OTA 分区 → 校验镜像 → `esp_ota_set_boot_partition()` → 重启。
- **自动回滚**：`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`，新固件启动后调用
  `esp_ota_mark_app_valid_cancel_rollback()`（见 `app_main.c` 的 `mark_app_valid()`），
  否则 panic/看门狗复位会自动回退旧分区，天然防变砖。

## OneNet FOTA 说明

对「自定义 MCU + 透传模组」，OneNet 通常在平台侧存储固件并下发**固件下载地址(URL)+校验**，
设备自行 HTTP 下载。ESP32-S3 走标准 `esp_https_ota` 更顺畅。

## QT5 上位机 OTA 面板

`host-app/src/otapanel.*` 提供选择本地 bin、填产品ID/设备名、触发下发、显示进度与设备版本。
实际上传/下发需对接 OneNet FOTA 的 REST API（在 `requestUpload` 信号的处理里实现）。
