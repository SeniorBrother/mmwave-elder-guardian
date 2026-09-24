# 分阶段实施路线（新手友好，先模块后整合）

> 两人如何分工、并行与 Git 协作，见 [collaboration.md](collaboration.md)。可认领任务见 [task-board.md](task-board.md)。移动版扩展见 [mobile-extension.md](mobile-extension.md)。目录结构权威定义见 [directory-structure.md](directory-structure.md)。
>
> **⚠️ 计划调整**：**QT5 上位机** 与 **OTA 升级（IAP Bootloader + 云 FOTA）** 已移出当前排期，目录保留（`project/host-app/`、`project/firmware/stm32f103/Bootloader/`），后期阶段再启用。远程查看暂以 **OneNet 网页/手机** 为准（不依赖 QT5）。

## 当前排期（阶段 0 → 8）

| 阶段 | 目标 | 关键动作 | 提交示例 |
|------|------|----------|----------|
| 0 环境 | 工具链就绪 | 标准库(SPL)模板工程+Keil（`STM32F10X_HD`）；HSE8M→72M、点灯(PA8)+USART1 printf | `chore: init toolchain` |
| 1 FreeRTOS | RTOS 调度 | 移植 FreeRTOS 内核，`xTaskCreate`+`vTaskDelay` 周期打印 | `feat(rtos): 任务调度` |
| 2 LVGL+触控 | 界面显示 | 移植 LVGL，跑通 2.8寸 ILI9341 + XPT2046 触摸按钮 | `feat(lvgl): 移植LVGL+触控` |
| 3 传感器 | 环境采集 | DHT11(PB9) → MLX90614(I2C1 扫0x5A) → MQ7(PA1 预热+基准) | `feat(sensor): 传感器驱动` |
| 4 雷达 | 生命体征 | USART3(PB10/11) 看 53 59 帧 → `radar_proto` 解析；收不到心率发使能命令 | `feat(radar): R60ABD1协议解析` |
| 5 上云 | 远程查看 | ESP8266(USART2) → MQTT 登录 → 订阅 → 发布 JSON；OneNet 网页可视化 | `feat(net): OneNet MQTT上云` |
| 6 整合 | 多任务 | 驱动拆进任务，LVGL 绑定数据，`alarm_rule`+`fall_algo` 接入声光报警 | `feat(core): 任务整合` |
| 7 RS485 | 本地总线 | UART5+MAX485(DE/RE=PC3) → `modbus_rtu` 从站 → `tools/modbus_test.py` 轮询 → 多节点地址 | `feat(modbus): RS485从站` |
| 8 联调 | 整体 | 调摔倒阈值，OneNet 可视化发布，本地声光/按键闭环 | `chore: integration` |
| 9 方案B(可选) | ESP32-S3 移植 | ESP-IDF 工程 + 分区表 + 复用 common + BSP 重写 | `feat(esp32s3): 目标移植` |

## 保留（后期阶段，暂不排期）

| 阶段 | 目标 | 关键动作 |
|------|------|----------|
| R1 QT5 上位机 | PC 看板 | Qt5 Widgets/Charts/SerialBus：dashboard + Modbus 主站 + OneNet 客户端 |
| R2 OTA(A) | STM32 升级 | IAP Bootloader + Flash 分区 + 向量表偏移 + W25Q64 + OneNet FOTA |
| R3 OTA(B) | 原生升级 | ESP32-S3 `esp_https_ota` 双分区 + 自动回滚 |
| R4 移动版 | 移动+AI语音 | 见 [mobile-extension.md](mobile-extension.md)（分层双主控 + motion_link + ESP-SR） |

## 已完成（脚手架交付，现位于 examples/ 与 project/）

- 仓库三区结构（`project/` 正式代码 · `examples/ai-scaffold/` AI 参考 · `docs/` 文档）+ git + README/LICENSE/.gitignore
- `project/firmware/common/` 业务层全部实现并**通过 62 项单元测试**（含 motion_link）：
  - `guardian_data.h` 数据模型 · `radar_proto.c` 雷达协议解析 · `fall_algo.c` 摔倒检测（滑动窗口方差）
  - `alarm_rule.c` 报警规则 · `onenet_json.c` OneNet JSON 组包 · `modbus_rtu.c` Modbus RTU 从站栈
  - `motion_link.c` 双主控协议 · `guardian_app.c` 业务编排器 · `bsp.h` 跨平台 BSP 抽象接口
- `examples/ai-scaffold/`：STM32（app_tasks 6 任务骨架 + BSP + Bootloader IAP）、ESP32-S3（app_main + BSP + partitions + CMake + sdkconfig）、QT5 上位机骨架、hardware（接线/BOM/RS485）、tools（Token/Modbus 脚本）
- `project/hardware/`、`project/tools/` 已从示例复制为起步资料

## 下一步（需硬件在手）

1. 用**标准库模板工程**起步（详见 `project/firmware/stm32f103/README.md`，不用 HAL），把 `project/firmware/common/` 纳入 Keil，逐阶段填 BSP。
2. 按阶段 0→8 逐个模块点亮，每步用 USART1 串口日志验证。
3. 多房间部署时，为每个节点设不同 Modbus 地址（保持寄存器 0x0000）。
