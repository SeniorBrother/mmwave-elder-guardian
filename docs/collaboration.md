# 两人协同开发分工与协作流程

> 配套文档：开发计划见 [roadmap.md](roadmap.md)，可认领任务清单见 [task-board.md](task-board.md)，移动版见 [mobile-extension.md](mobile-extension.md)。
> 本文解决"两个人怎么分工、怎么并行、怎么不打架"。
>
> **⚠️ 计划调整**：**QT5 上位机** 与 **OTA 升级** 已移出当前排期（目录保留，后期阶段再加）。因此当前两条切片都以**固件 + OneNet 云端**为主，本地上位以 **LVGL 触控屏**为准（不再依赖 QT5）。

## 一、分工模型：垂直功能切片（两人都贯通上下）

不做"一人只碰硬件、一人只碰云"的横向切分。改为**按功能垂直切片**：
每人 owns 一条从 **传感器/固件 → 协议(common) → 上位机/云** 的完整链路，
因此**两人都会写底层固件，也都会写上位机/云端**，各自负责一部分完整功能。

三条基本原则：
1. **契约层共同所有**：`project/firmware/common/`（数据模型 + 协议编解码 + 业务规则，当前 62 项测试全过）是两人唯一的强耦合点，改动需双方评审 + 过测试。
2. **按功能切片、按文件归属**：每条切片在各层只碰自己的文件，天然减少 git 冲突。
3. **交叉评审 + 里程碑结对**：强制两人都读对方领域代码，集成点一起调，逐步都变全栈。

## 二、功能切片与归属

### 切片 甲 —— 生命体征监护 + OneNet 云端远程查看

| 层 | 负责内容 | 主要文件 |
|----|----------|----------|
| 固件(下) | 雷达驱动、雷达 FreeRTOS 任务、ESP8266/WiFi + MQTT 上云发布 | `project/firmware/stm32f103/`(雷达/WiFi BSP + Task_Radar/Task_Net) |
| 协议(中) | 雷达解析、摔倒、监护字段 | `common/radar_proto.*`、`fall_algo.*`、`guardian_data.h` |
| 云(上) | OneNet 产品/物模型/可视化网页 + 手机远程查看 | OneNet 平台、`docs/onenet.md` |
| 硬件 | 雷达、ESP8266/WiFi 部分的接线调试 | `project/hardware/wiring.md`(相关段) |

### 切片 乙 —— 环境安全 + LVGL 本地交互 + RS485 本地总线

| 层 | 负责内容 | 主要文件 |
|----|----------|----------|
| 固件(下) | 温湿度/体温/CO 传感器、LVGL 触控本地界面、报警声光输出、RS485 从站 | `project/firmware/stm32f103/`(传感器/LVGL/RS485 BSP) |
| 协议(中) | 报警规则、Modbus 从站 | `common/alarm_rule.*`、`modbus_rtu.*` |
| 本地(上) | LVGL 显示页 + 触控"解除报警/SOS"、本地按键 | `project/firmware/stm32f103/UI/` |
| 硬件 | 传感器、RS485、屏、声光的接线调试 | `project/hardware/`(相关段) |

> 这样：甲写雷达固件 + OneNet 上云/网页；乙写传感器/LVGL/RS485 固件 + 本地交互。**两人都跨了驱动→common→云端/本地全链路。**
> （QT5 上位机、OTA、移动版 AI 语音/电机为**保留项**，后期阶段再分工，见 task-board 文末。）

### 共同所有（改动需双方评审 + 过测试）

- `project/firmware/common/` 全部（尤其跨切片共用的 `guardian_data.h`、`guardian_app.*`、`bsp.h`）
- `docs/modbus-map.md`、`docs/mobile-extension.md` 里的**协议与寄存器定义**

> 规则：改 common 或协议表 → 先说一声 → 跑 `bash project/firmware/common/test/build_and_test.sh` → 双方确认 → 才合并。

## 三、Git 协作流程

- **分支**：`main`(稳定) ← `dev`(集成) ← `feature/<切片>-<模块>`。
  例：`feature/jia-radar-driver`、`feature/yi-modbus-master`、`feature/yi-ota-panel`。
- **提交规范**（Conventional Commits）：`feat(radar): ...` / `fix(modbus): ...` / `docs: ...`。
- **交叉评审（强制）**：功能分支 → dev 走 PR，**由另一人 review**；甲的固件 PR 由乙审、乙的上位机 PR 由甲审，逼着两人都读懂对方领域。
- **防冲突**：
  1. 按"切片 + 文件归属"改，甲不碰 `modbusmaster.*`、乙不碰 `dashboard.*`。
  2. common 层改动串行化（一次只一人改，改完即合）。
  3. 频繁 pull/rebase dev，别攒大改动。
- **禁止**：直接推 main；提交编译产物（已 gitignore `*.bin/*.exe/build/` 等）。

## 四、并行泳道与里程碑（每个里程碑两人都跨层）

★ = 必须一起的汇合/联调点。可见每个里程碑里，**两人都同时有"下"和"上"的活**。

| 里程碑 | 切片 甲 | 切片 乙 | 汇合★ |
|--------|---------|---------|-------|
| M1 基础 | 固件:CubeMX工程/点灯/FreeRTOS/雷达串口 | 固件:LVGL移植触控/传感器驱动 | ★点灯+printf+FreeRTOS |
| M2 感知与本地 | 固件:雷达解析+摔倒接入 common | 固件:RS485从站+LVGL显示页 | ★RS485 有线自测 |
| M3 上云与报警 | 固件:MQTT上云 & 云:OneNet可视化网页 | 固件:报警声光+本地按键+LVGL报警页 | ★真机数据上云联调 |
| M4 移动版(可选) | 固件:大脑AI语音(ESP-SR/云)+motion_link主机 | 固件:小脑四轮电机+编码器PID+motion_link从机 | ★双主控通信联调 |

> QT5 上位机、OTA 升级为**保留项**（后期阶段），不在当前泳道。

**解耦技巧（让并行真正跑起来）**：
- 硬件没到手时，用 common 层在 PC 上跑 **mock 数据源**（参考 `project/tools/modbus_test.py` 思路做假设备），先把上云/本地全流程打通。
- 每人的"固件侧"和"上位机侧"用同一份 common 协议对接——协议先冻结，自己两端就能并行推进。

## 五、轮换与结对（让两人都变全栈）

- **里程碑结对**：每个 ★ 汇合点两人一起联调，天然接触对方模块。
- **中期轮换（可选）**：M3 之后可交换一个小模块（如甲接一次 Modbus、乙接一次雷达），补齐对方领域经验。
- **交叉评审**已是常态化的"互相学习"机制。

## 六、当前基线（交接起点）

- 仓库 `mmwave-elder-guardian` 已初始化，Monorepo 骨架完整。
- `project/firmware/common/` 业务层**全部实现并通过 62 项单元测试**（雷达/摔倒/报警/OneNet/Modbus/编排/motion_link）。
- STM32/ESP32-S3 BSP、QT5 上位机为带 TODO 的脚手架，位于 `examples/ai-scaffold/` 作参考。
- 硬件/文档/工具齐备（接线表、BOM、RS485 拓扑、Token 脚本、Modbus 测试脚本）。

## 七、建议的第一步（本周）

1. 两人一起**冻结 common 契约**：过 `guardian_data.h`、`modbus-map.md`、`motion_link.h`，确认字段无异议。
2. 认领切片：甲=生命体征+OneNet云端远程，乙=环境安全+LVGL本地交互+RS485本地总线（可对调）。
3. 各自把固件侧起起来（详见 `project/firmware/stm32f103/README.md`）：
   - 甲：CubeMX 生成工程跑通点灯(PA8)/printf(USART1)/FreeRTOS/雷达 USART3 成帧。
   - 乙：LVGL 移植 + 传感器驱动（DHT11/MLX90614/MQ7）。
4. 环境统一：都用 WSL 或本机 gcc 跑 `build_and_test.sh`，约定"改 common 必过测试"。
5. 建 `dev` 分支与 PR 交叉评审流程，跑通第一次协作合并。

## 八、环境备注

- common 层单元测试用 gcc（推荐 WSL，Windows 侧 MinGW 若缺 cc1 改用 WSL）：
  ```bash
  cd project/firmware/common/test && bash build_and_test.sh
  ```
- 固件：Keil MDK5 + STM32CubeMX(STM32) / ESP-IDF(ESP32-S3、ESP32)。两人都装 CubeMX+Keil。QT5(Qt Creator) 待后期上位机阶段再装。
