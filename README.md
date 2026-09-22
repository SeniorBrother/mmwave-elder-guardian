# mmwave-elder-guardian

STM32 / ESP32-S3 独居老人居家监护系统 —— 60G 毫米波雷达非接触测呼吸心率 + 摔倒报警 + OneNet 云平台远程查看 + RS485/Modbus RTU 本地总线 + QT5 上位机看板 + OTA 远程升级。

> 免责声明：本项目仅用于学习 / 竞赛 / 个人监护原型，**非医疗级、不可量产**。摔倒判定为教学级生理信号突变检测，实际部署需人工确认以降低误报。毫米波雷达家用合规性请自行确认。

## 功能特性

- 非接触生命体征：60GHz 毫米波雷达 R60ABD1 直出呼吸频率、心率、人体存在、运动姿态。
- 摔倒报警：雷达姿态为主 + 心率/呼吸滑动窗口方差突变为辅，声光报警。
- 环境与健康：MLX90614 红外体温、DHT11 温湿度、MQ7 一氧化碳。
- 本地交互：2.8 寸 ILI9341 + XPT2046 触控屏（LVGL），触控解除报警 / SOS / 阈值设置。
- 云端远程：ESP8266 / ESP32-S3 内置 WiFi → MQTT → OneNet，可视化网页手机远程查看 + FOTA 升级。
- 本地有线：RS485 + Modbus RTU，QT5 上位机有线轮询（断网可用），支持多房间节点一主多从。
- 上位机：QT5 桌面看板（QtCharts 曲线 + 仪表盘 + 报警弹窗 + OTA 面板）。

## 双主控方案

| 方案 | 主控 | 框架 | WiFi | OTA |
|------|------|------|------|-----|
| A | STM32F103RCT6 (256KB/48KB) | STM32CubeMX + Keil + HAL + FreeRTOS + LVGL | 外挂 ESP8266 | 自建 IAP Bootloader + W25Q64 缓存 |
| B | ESP32-S3 (N16R8) | ESP-IDF v5.x + FreeRTOS + LVGL | 内置 WiFi/BLE | 原生双分区 + 自动回滚 |

两方案共用 `firmware/common/` 业务层（雷达协议、摔倒算法、报警规则、OneNet JSON、Modbus RTU 从站栈、LVGL UI），仅 BSP 分芯片实现。

## 目录结构

```
mmwave-elder-guardian/
├── firmware/
│   ├── common/       跨平台业务逻辑(纯C, 可单元测试)
│   │   └── test/     gcc 单元测试
│   ├── stm32f103/    方案A: BSP / App / Bootloader
│   └── esp32s3/      方案B: main / components / partitions.csv
├── host-app/         QT5 PC 上位机看板
├── hardware/         接线表 / BOM / RS485 总线拓扑
├── docs/             OneNet 部署 / FOTA / Modbus 寄存器表 / 雷达协议 / 教程 / 移动版扩展
└── tools/            Token 生成 / 固件打包 / Modbus 测试脚本
```

## 快速开始（common 层单元测试）

common 层是与硬件无关的纯 C 代码，可用 gcc 直接编译测试：

```bash
cd firmware/common/test
gcc -Wall -Wextra -I.. test_main.c ../*.c -o run_tests
./run_tests
```

## 分阶段实施路线

见 [docs/roadmap.md](docs/roadmap.md)。核心顺序：环境 → FreeRTOS → LVGL 触控 → 传感器 → 雷达 → 上云 → 整合 → QT5 → OTA → ESP32-S3 → RS485/Modbus → 联调。

两人协同开发的分工与 Git 协作流程见 [docs/collaboration.md](docs/collaboration.md)，可认领的任务清单见 [docs/task-board.md](docs/task-board.md)。

## 技术栈

C · FreeRTOS · LVGL v8.3 · STM32 HAL / ESP-IDF · MQTT · OneNet · Modbus RTU · QT5 (QtCharts / QtSerialBus)

## 扩展：移动版（移动底盘 + AI 语音交互）

后期可扩展为可移动 + 带 AI 语音交互的巡逻/跟随式监护机器人，采用**分层双主控**架构（AI 大脑 ESP32-S3/SBC + 运动控制小脑 MCU），双主控间用 `motion_link` UART 协议通信。完整方案（引脚预算、四轮电机、ESP-SR 语音链路、分区表改法、追加 BOM、实施步骤）见 [docs/mobile-extension.md](docs/mobile-extension.md)。
