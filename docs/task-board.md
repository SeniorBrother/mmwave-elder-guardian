# 任务看板 (Task Board)

> 配套：分工模型见 [collaboration.md](collaboration.md)，阶段路线见 [roadmap.md](roadmap.md)，移动版见 [mobile-extension.md](mobile-extension.md)。
> 用法：认领任务后把 `[ ]` 改成 `[x]`；每条对应一个 `feature/<切片>-<模块>` 分支 + 一次 PR（交叉评审）。
> 标记：🅰 = 切片甲（生命体征 + OneNet 云端远程查看） 🅱 = 切片乙（环境安全 + LVGL 本地交互 + RS485 本地总线） 🤝 = 两人共同/结对
>
> **⚠️ 计划调整（当前阶段）**：**QT5 上位机** 与 **OTA 升级（IAP Bootloader + 云 FOTA）** 已移出当前排期，
> 目录结构保留（`project/host-app/`、`project/firmware/stm32f103/Bootloader/`），见文末「保留（后期阶段）」。

图例：`[ ]` 待办 · `[~]` 进行中 · `[x]` 完成

---

## M0 准备（🤝 一起，第 1 天）

- [x] 🤝 冻结 common 契约：过一遍 `guardian_data.h` / `modbus-map.md` / `motion_link.h`，字段无异议
- [ ] 🤝 建 `dev` 分支，约定 PR 交叉评审 + Conventional Commits
- [x] 🤝 统一环境：WSL/gcc 跑通 `project/firmware/common/test/build_and_test.sh`（基线 62 项全过）
- [x] 🤝 认领切片：甲=A / 乙=B
- [ ] 🅰 装齐工具链：Keil MDK5 + 标准库模板（STM32F10x_StdPeriph_Lib V3.5）+ ST-Link 驱动（CubeMX 可选，仅作引脚/时钟规划）
- [ ] 🅱 装齐工具链：Keil MDK5 + 标准库模板（+ ESP-IDF 备用）

---

## M1 基础（固件起框架）★汇合：点灯+printf+FreeRTOS 跑通

### 🅰 切片甲（详见 `project/firmware/stm32f103/README.md`，**标准库 SPL，不用 HAL**）
- [ ] 固件：用标准库模板建 STM32F103RCT6 工程（`STM32F10X_HD`、HSE 8M→72M）
- [ ] 固件：common + BSP 纳入 Keil，编译通过、ST-Link 下载成功
- [ ] 固件：PA8 点灯 + USART1 printf 日志（板载 CH340/Type-C）
- [ ] 固件：移植 FreeRTOS 内核，任务调度跑通（vTaskDelay 周期打印）
- [ ] 固件：雷达 USART3(PB10/PB11) 接收中断成帧，打印原始帧（帧头 0x53 0x59）

### 🅱 切片乙
- [ ] 固件：LVGL v8.3 移植 + ILI9341/XPT2046 触控点亮（SPI1）
- [ ] 固件：DHT11 驱动（PB9）+ 串口验证
- [ ] 固件：MLX90614 驱动（I2C1 PB6/PB7，先扫描确认 0x5A）
- [ ] 固件：MQ7 ADC 采集（PA1）+ 开机基准零点补偿

---

## M2 感知与本地总线 ★汇合：RS485 有线自测通过

### 🅰 切片甲
- [ ] 固件：`radar_proto` 接入 → 心率/呼吸/存在/姿态更新到 `guardian_app`
- [ ] 固件：收不到心率时发 `EnableHeartbeatMonitor()` 使能
- [ ] 固件：`fall_algo` 摔倒检测接入（滑动窗口方差）

### 🅱 切片乙
- [ ] 固件：RS485(UART5 PC12/PD2 + MAX485，DE/RE=PC3) 半双工收发（TC 后切 DE）
- [ ] 固件：`modbus_rtu` 从站接入 → `app_handle_modbus` + `app_apply_coils`
- [ ] 固件：LVGL 本地显示页（心率/呼吸/体温/温湿度/CO/报警状态）
- [ ] 🤝 联调：用 `project/tools/modbus_test.py`（PC + USB-RS485）轮询真机寄存器，与设备屏/串口一致

---

## M3 上云与报警 ★汇合：真机数据上云联调

### 🅰 切片甲
- [ ] 固件：ESP8266(USART2 PA2/PA3) 连 2.4G WiFi（AT）→ TCP → MQTT 登录
- [ ] 固件：`onenet_json` 组包 → 订阅 → 发布数据点
- [ ] 云：OneNet 建产品 + 8 个物模型标识符
- [ ] 云：OneNet 可视化 View 拖仪表盘/卡片 + 过滤器脚本，发布手机网页远程查看
- [ ] 🤝 联调：OneNet 设备详情数据点刷新，标识符大小写核对

### 🅱 切片乙
- [ ] 固件：`alarm_rule` 阈值判定 → 蜂鸣器(三极管，改到非 PA8 空闲脚) + LED 声光报警
- [ ] 固件：本地按键 解除(PC8) / SOS(PC9) / WK_UP(PA0)
- [ ] 固件：LVGL 报警页变红 + 触控"解除报警/SOS"按钮
- [ ] 固件：报警位图解析（心率/体温/CO/摔倒/SOS）驱动本地声光

---

## M4 移动版（可选）★汇合：双主控通信联调

### 🅱 切片乙（小脑·运动）
- [ ] 固件：ESP32-WROOM-32D 目标（`set-target esp32` + 4MB `partitions.csv`）
- [ ] 固件：四轮电机 PWM（MCPWM/LEDC + TB6612）开环转动
- [ ] 固件：编码器 PCNT/定时器闭环 + 速度 PID
- [ ] 固件：小脑 `motion_link` 从机（收速度指令 / 回底盘状态）

### 🅰 切片甲（大脑·AI语音）
- [ ] 固件：ESP32-S3 加 `model` 分区 + 开 ESP-SR/PSRAM 的 sdkconfig
- [ ] 固件：I2S 麦(INMP441) + AFE + WakeNet 唤醒
- [ ] 固件：MultiNet 命令词 → 意图映射到 `motion_link` 主机指令
- [ ] 固件（可选）：云端 ASR/LLM/TTS 自由对话链路
- [ ] 🤝 联调：语音命令 → 底盘动作；急停(0x03) + 物理急停验证

---

## 贯穿性任务（随时）

- [ ] 🤝 每次改 common/协议 → 过 `build_and_test.sh`（当前 62 项）+ 双方评审
- [ ] 🅰🅱 各自模块补充单元测试到 `project/firmware/common/test/`
- [ ] 🤝 文档同步：接线/BOM/寄存器表随实现更新
- [ ] 🤝 硬件采购到货核对（见 `project/hardware/bom.md`）

---

## 保留（后期阶段，暂不排期，目录已留）

> 以下功能本轮不做，目录结构保留，下个阶段再启用。

### QT5 上位机（`project/host-app/` 保留）
- [ ] 上位机：QT5 工程骨架（CMake + Qt5 Widgets/Charts/SerialBus）
- [ ] 上位机：`dashboard` 看板曲线 + 报警弹窗（Modbus/OneNet 双数据源）
- [ ] 上位机：`modbusmaster` 经 USB-RS485 有线轮询（断网可用）
- [ ] 上位机：`onenetclient` RESTful 拉云端历史数据
- [ ] 上位机：底盘遥控 + 状态页（移动版）

### OTA 远程升级（`project/firmware/stm32f103/Bootloader/` 保留）
- [ ] 固件：IAP Bootloader（Flash 分区 + 跳转 APP + `SCB->VTOR`），APP 起始改 0x08008000
- [ ] 固件：W25Q64 驱动 + 固件缓存/CRC32 校验
- [ ] 云：OneNet FOTA 上传 bin + 下发升级任务
- [ ] 固件：`Task_OTA` 收 FOTA 指令 → ESP8266 下载 → 置升级标志 → 重启升级
- [ ] 联调：升级成功版本更新；下载中断电重启回退不变砖
- [ ] （方案B 可选）：ESP32-S3 `esp_https_ota` 双分区 + 自动回滚

## 里程碑进度速览

| 里程碑 | 甲 | 乙 | 汇合★ |
|--------|----|----|-------|
| M0 准备 | ☑ | ☑ | 冻结契约(部分) |
| M1 基础 | ☐ | ☐ | 点灯+printf+FreeRTOS |
| M2 感知/本地 | ☐ | ☐ | RS485 自测 |
| M3 上云/报警 | ☐ | ☐ | 真机上云 |
| M4 移动版(可选) | ☐ | ☐ | 双主控通信 |
| 保留 QT5上位机 | — | — | 后期阶段 |
| 保留 OTA升级 | — | — | 后期阶段 |
