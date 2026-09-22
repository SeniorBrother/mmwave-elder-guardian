# 分阶段实施路线（新手友好，先模块后整合）

> 两人如何分工、并行与 Git 协作，见 [collaboration.md](collaboration.md)。移动版扩展见 [mobile-extension.md](mobile-extension.md)。

| 阶段 | 目标 | 关键动作 | 提交示例 |
|------|------|----------|----------|
| 0 环境 | 工具链就绪 | CubeMX+Keil(方案A) / ESP-IDF(方案B)；点灯+串口日志 | `chore: init toolchain` |
| 1 FreeRTOS | RTOS 调度 | 建两个任务分别闪灯/打印 | `feat(rtos): 双任务调度` |
| 2 LVGL+触控 | 界面显示 | 移植 LVGL，跑通 2.8寸屏 + XPT2046 触摸按钮 | `feat(lvgl): 移植LVGL+触控` |
| 3 传感器 | 环境采集 | DHT11 → MLX90614(IIC扫0x5A) → MQ7(预热+基准) | `feat(sensor): 传感器驱动` |
| 4 雷达 | 生命体征 | USB-TTL 直连看 53 59 帧 → 接 MCU 解析；收不到心率发使能命令 | `feat(radar): R60ABD1协议解析` |
| 5 上云 | 远程查看 | WiFi → MQTT 登录(20 02) → 订阅(90 03) → 发布 JSON | `feat(net): OneNet MQTT上云` |
| 6 整合 | 多任务 | 驱动拆进任务，LVGL 绑定数据，接入报警逻辑 | `feat(core): 任务整合` |
| 7 QT5 | 上位机 | PC 看板对接 OneNet/Modbus | `feat(host): QT5看板` |
| 8 OTA(A) | STM32升级 | IAP Bootloader + Flash分区 + 向量表偏移 + W25Q64 + FOTA | `feat(ota): IAP+FOTA` |
| 9 ESP32-S3 | 移植B | ESP-IDF 工程 + 分区表 + 复用 common + BSP 重写 | `feat(esp32s3): 目标移植` |
| 10 OTA(B) | 原生升级 | esp_https_ota 双分区 + 自动回滚 | `feat(ota): esp_https_ota` |
| 11 RS485 | 本地总线 | MAX485 点对点 → Modbus 从站栈 → QT5 主站轮询 → 多节点地址 | `feat(modbus): RS485从站` |
| 12 联调 | 整体 | 调摔倒阈值，OneNet 可视化 + QT5(双源+OTA面板) 发布 | `chore: integration` |

## 已完成（本次交付）

- 仓库骨架 + git + README/LICENSE/.gitignore
- `firmware/common/` 业务层全部实现并**通过 52 项单元测试**：
  - `guardian_data.h` 数据模型
  - `radar_proto.c` 雷达协议解析（帧校验/事件分发/使能命令）
  - `fall_algo.c` 摔倒检测（滑动窗口方差，整数实现）
  - `alarm_rule.c` 报警规则
  - `onenet_json.c` OneNet JSON 组包（体温不依赖 %f）
  - `modbus_rtu.c` Modbus RTU 从站栈（CRC16/6功能码/寄存器映射）
  - `guardian_app.c` 业务编排器（把上述模块串成端到端逻辑）
  - `bsp.h` 跨平台 BSP 抽象接口
- STM32 方案A：`app_tasks.c` 6 任务骨架 + `bsp_stm32f103.c` + `bootloader.c` IAP 骨架
- ESP32-S3 方案B：`app_main.c` + `bsp_esp32s3.c` + `partitions.csv` + CMake + sdkconfig.defaults
- QT5 上位机：Modbus 主站 + OneNet 客户端 + 看板 + OTA 面板 + 主窗口脚手架
- 硬件/文档：接线表、BOM、RS485 拓扑、Modbus 寄存器表、雷达协议、OneNet 部署、FOTA
- 工具：OneNet Token 生成脚本、Modbus 测试脚本

## 下一步（需硬件在手）

1. 用 CubeMX 生成方案A 工程，把 `common/` 与 `BSP/` 纳入 Keil，逐阶段填 BSP 的 TODO。
2. 按阶段 1→12 逐个模块点亮，每步用串口日志验证。
3. 多房间部署时，为每个节点设不同 Modbus 地址（保持寄存器 0x0000）。
