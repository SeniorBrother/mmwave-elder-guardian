# 目录结构定义（权威）

> 本文件是仓库目录结构的**唯一权威定义**。任何结构调整以本文为准，README/roadmap 中的目录描述为辅助说明。

## 顶层布局

```
mmwave-elder-guardian/
├── project/          【你们两人的真实项目代码 —— 独立工作区，正式产物】
├── examples/         【AI 生成的示例脚手架 —— 备用参考，非正式代码】
├── docs/             【方案/计划/协议文档 —— 共享参考】
├── README.md         项目总览
├── LICENSE
├── .gitignore
└── .gitattributes
```

三个区的边界与规则：

| 区 | 归属 | 用途 | 规则 |
|----|------|------|------|
| `project/` | 甲 + 乙 | 你们真正开发、要交付/烧录/上线的代码 | 正式提交；按 task-board 认领 |
| `examples/` | AI 参考 | 我生成的脚手架，供抄写/对照 | **只读心态**，不作为正式产物；可随时删改不影响 project |
| `docs/` | 共享 | 方案、协议、计划、分工文档 | 随实现同步更新 |

## project/ —— 你们的真实项目

```
project/
├── firmware/
│   ├── common/          ★共享契约库（已由 AI 实现并通过 62 项单测，你们复用+改）
│   │   ├── guardian_data.h      数据模型
│   │   ├── radar_proto.c/h      雷达协议解析
│   │   ├── fall_algo.c/h        摔倒检测
│   │   ├── alarm_rule.c/h       报警规则
│   │   ├── onenet_json.c/h      OneNet JSON 组包
│   │   ├── modbus_rtu.c/h       Modbus RTU 从站栈
│   │   ├── motion_link.c/h      双主控通信协议
│   │   ├── guardian_app.c/h     业务编排器
│   │   ├── bsp.h                板级抽象接口
│   │   ├── CMakeLists.txt       (ESP-IDF 组件注册)
│   │   └── test/                单元测试（gcc/WSL 可跑）
│   ├── stm32f103/       方案A 固件（你们自建，参考 examples 起步）
│   ├── esp32s3/         方案B / AI语音大脑（你们自建）
│   └── esp32/           小脑 ESP32-WROOM-32D 目标（你们自建）
├── host-app/            QT5 上位机（你们自建，参考 examples 起步）
├── hardware/            你们的接线表/BOM/原理图（可从 examples 复制起步）
└── tools/               你们的脚本（Token/打包/测试，可从 examples 复制起步）
```

**约定**：
- `common/` 是两人**共同所有**的契约层，改动需双方评审 + 过测试（见 collaboration.md）。
- `common/` 是纯 C、与硬件无关，`stm32f103 / esp32s3 / esp32` 三个目标都复用它，只各写自己的 BSP。
- 其余子目录由你们从零建立正式实现；`examples/` 里有对应脚手架可对照抄写。

## examples/ —— AI 示例脚手架（备用参考）

```
examples/
└── ai-scaffold/
    ├── firmware/
    │   ├── stm32f103/   App(任务骨架) / BSP(驱动骨架) / Bootloader(IAP骨架)
    │   └── esp32s3/     app_main / bsp / partitions.csv / CMake / sdkconfig.defaults
    ├── host-app/        QT5 上位机脚手架(mainwindow/dashboard/modbusmaster/onenetclient/otapanel)
    ├── hardware/        接线表 wiring.md / bom.md / rs485-bus.md（参考）
    ├── tools/           onenet_token.py / modbus_test.py（可直接拿去用）
    └── README.md        说明这是参考实现
```

**注意**：
- 示例脚手架里的 `#include "../common/..."`、ESP-IDF `EXTRA_COMPONENT_DIRS` 等相对路径，是相对**它自己原来的位置**写的；common 现已移到 `project/firmware/common/`，若要直接编译示例需把引用路径指过去。
- 示例仅作**参考/抄写起点**，不保证开箱即编译（BSP 多为带 TODO 的骨架）。

## docs/ —— 共享文档

```
docs/
├── directory-structure.md   本文（权威目录定义）
├── roadmap.md               分阶段实施路线
├── collaboration.md         两人分工与 Git 协作
├── task-board.md            可认领任务看板
├── mobile-extension.md      移动版（分层双主控 + AI 语音）
├── onenet.md                OneNet 部署
├── fota.md                  OTA/FOTA 升级
├── modbus-map.md            Modbus 寄存器映射
└── radar-protocol.md        R60ABD1 雷达协议
```

## 从示例起步的推荐动作

1. 把 `examples/ai-scaffold/hardware/*` 复制到 `project/hardware/` 作为接线/BOM 起点。
2. 把 `examples/ai-scaffold/tools/*` 复制到 `project/tools/`（Token/Modbus 脚本可直接用）。
3. 参考 `examples/ai-scaffold/firmware/stm32f103/BSP` 在 `project/firmware/stm32f103/` 写你们自己的 BSP（include 指向 `project/firmware/common/`）。
4. 参考 `examples/ai-scaffold/host-app/` 在 `project/host-app/` 建你们的 QT5 工程。
