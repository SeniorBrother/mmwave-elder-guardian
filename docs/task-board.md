# 任务看板 (Task Board)

> 配套：分工模型见 [collaboration.md](collaboration.md)，阶段路线见 [roadmap.md](roadmap.md)，移动版见 [mobile-extension.md](mobile-extension.md)。
> 用法：认领任务后把 `[ ]` 改成 `[x]`；每条对应一个 `feature/<切片>-<模块>` 分支 + 一次 PR（交叉评审）。
> 标记：🅰 = 切片甲（生命体征+云端远程） 🅱 = 切片乙（环境安全+本地总线+OTA/移动） 🤝 = 两人共同/结对

图例：`[ ]` 待办 · `[~]` 进行中 · `[x]` 完成

---

## M0 准备（🤝 一起，第 1 天）

- [ ] 🤝 冻结 common 契约：过一遍 `guardian_data.h` / `modbus-map.md` / `motion_link.h`，字段无异议
- [ ] 🤝 建 `dev` 分支，约定 PR 交叉评审 + Conventional Commits
- [ ] 🤝 统一环境：WSL/gcc 跑通 `firmware/common/test/build_and_test.sh`（基线 62 项全过）
- [ ] 🤝 认领切片：甲 / 乙（可对调）
- [ ] 🅰 装齐工具链：Keil + STM32CubeMX（+ ESP-IDF）
- [ ] 🅱 装齐工具链：Qt Creator + CMake（Qt SerialBus/Charts）+ ESP-IDF

---

## M1 基础（固件起框架 + 上位机起 UI）★汇合：冻结数据字段

### 🅰 切片甲
- [ ] 固件：CubeMX 生成 STM32F103RCT6 工程，common + BSP 纳入 Keil
- [ ] 固件：点灯 + 串口1 printf 日志（阶段0）
- [ ] 固件：FreeRTOS 双任务调度跑通（阶段1）
- [ ] 固件：雷达 USART3 串口成帧接收（IDLE/定时器），打印原始帧
- [ ] 上位机：QT5 工程骨架（CMake + Qt5 Widgets/Charts）
- [ ] 上位机：`dashboard` 用 mock 数据渲染 8 项卡片 + 曲线

### 🅱 切片乙
- [ ] 固件：LVGL v8.3 移植 + ILI9341/XPT2046 触控点亮（阶段2）
- [ ] 固件：DHT11 驱动 + 串口验证
- [ ] 固件：MLX90614 驱动（先 IIC 扫描确认 0x5A）
- [ ] 固件：MQ7 ADC 采集 + 开机基准零点补偿
- [ ] 上位机：OneNet 建产品 + 8 个物模型标识符
- [ ] 上位机：`onenetclient` RESTful 轮询骨架（mock/真值皆可）

---

## M2 感知与本地总线 ★汇合：RS485 有线联调

### 🅰 切片甲
- [ ] 固件：`radar_proto` 接入 → 心率/呼吸/存在/姿态更新到 `guardian_app`
- [ ] 固件：收不到心率时发 `EnableHeartbeatMonitor()` 使能
- [ ] 固件：`fall_algo` 摔倒检测接入（滑动窗口方差）
- [ ] 上位机：`dashboard` 从 mock 切到真值源，绑定字段

### 🅱 切片乙
- [ ] 固件：RS485(UART5+MAX485) 半双工收发（TC 后切 DE）
- [ ] 固件：`modbus_rtu` 从站接入 → `app_handle_modbus` + `app_apply_coils`
- [ ] 上位机：`modbusmaster` 经 USB-RS485 轮询真机输入寄存器
- [ ] 上位机：写线圈远程"解除报警/SOS"打通
- [ ] 🤝 联调：QT5 读回数据与设备屏幕/串口一致；断 WiFi 有线仍刷新

---

## M3 上云与报警 ★汇合：真机数据上云联调

### 🅰 切片甲
- [ ] 固件：ESP8266 连 2.4G WiFi（AT）→ TCP → MQTT 登录(20 02)
- [ ] 固件：`onenet_json` 组包 → 订阅(90 03) → 发布数据点
- [ ] 云：OneNet 可视化 View 拖仪表盘/卡片 + 过滤器脚本，发布手机网页
- [ ] 🤝 联调：OneNet 设备详情数据点刷新，标识符大小写核对

### 🅱 切片乙
- [ ] 固件：`alarm_rule` 阈值判定 → 蜂鸣器(三极管) + LED 声光报警
- [ ] 固件：本地按键 K1 解除 / K2 SOS
- [ ] 上位机：`dashboard` 报警条变红 + 声音 + 托盘通知
- [ ] 上位机：报警位图解析（心率/体温/CO/摔倒/SOS）

---

## M4 OTA 远程升级 ★汇合：整机升级 + 回滚联调

### 🅱 切片乙（OTA 主责）
- [ ] 固件：IAP Bootloader（Flash 分区 + 跳转 APP + `SCB->VTOR`）
- [ ] 固件：APP 工程 IROM 起始改 0x08008000
- [ ] 固件：W25Q64 驱动 + 固件缓存/CRC32 校验
- [ ] 固件：`Task_OTA` 收 OneNet FOTA 指令 → ESP8266 下载 → 置升级标志
- [ ] 云：OneNet FOTA 上传 bin + 下发升级任务

### 🅰 切片甲（上位机 + 版本）
- [ ] 上位机：`otapanel` 选 bin / 填产品ID设备名 / 触发下发
- [ ] 上位机：升级进度条 + 设备版本号显示（Modbus FwVersion 寄存器）
- [ ] 🤝 联调：升级成功版本更新；模拟下载中断电，重启回退不变砖
- [ ] （方案B 可选）：ESP32-S3 `esp_https_ota` 双分区 + 自动回滚

---

## M5 移动版（可选）★汇合：双主控通信联调

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
- [ ] 上位机：底盘遥控 + 状态页
- [ ] 🤝 联调：语音命令 → 底盘动作；急停(0x03) + 物理急停验证

---

## 贯穿性任务（随时）

- [ ] 🤝 每次改 common/协议 → 过 `build_and_test.sh`（当前 62 项）+ 双方评审
- [ ] 🅰🅱 各自模块补充单元测试到 `firmware/common/test/`
- [ ] 🤝 文档同步：接线/BOM/寄存器表随实现更新
- [ ] 🤝 硬件采购到货核对（见 `hardware/bom.md`）

## 里程碑进度速览

| 里程碑 | 甲 | 乙 | 汇合★ |
|--------|----|----|-------|
| M0 准备 | ☐ | ☐ | 冻结契约 |
| M1 基础 | ☐ | ☐ | 数据字段 |
| M2 感知/本地 | ☐ | ☐ | RS485 联调 |
| M3 上云/报警 | ☐ | ☐ | 真机上云 |
| M4 OTA | ☐ | ☐ | 升级+回滚 |
| M5 移动版 | ☐ | ☐ | 双主控通信 |
