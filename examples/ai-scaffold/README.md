# examples/ai-scaffold —— AI 生成的示例脚手架（备用参考）

> ⚠️ 这里**不是**你们的正式项目代码，只是 AI 生成的**参考实现/抄写起点**。
> 正式代码请写在 `../../project/`。本目录可随时删改，不影响 project。

## 内容

```
ai-scaffold/
├── firmware/
│   ├── stm32f103/   App/app_tasks.c(FreeRTOS 6任务骨架) · BSP/bsp_stm32f103.c(带TODO) · Bootloader/bootloader.c(IAP骨架)
│   └── esp32s3/     main/app_main.c · main/bsp_esp32s3.c · partitions.csv · CMakeLists.txt · sdkconfig.defaults
├── host-app/        QT5 上位机脚手架: mainwindow / dashboard / modbusmaster / onenetclient / otapanel
├── hardware/        wiring.md(接线表) · bom.md(物料清单) · rs485-bus.md(总线拓扑)
└── tools/           onenet_token.py(Token生成) · modbus_test.py(Modbus主站测试)
```

## 使用说明

- **可直接拿去用**：`tools/` 里的两个脚本、`hardware/` 里的接线表与 BOM。
- **参考抄写**：`firmware/` 与 `host-app/` 是带 TODO 的骨架，展示如何调用共享库 `common/`。
- **路径提示**：示例里的 `#include`、ESP-IDF `EXTRA_COMPONENT_DIRS` 是相对**它原位置**写的；
  共享库 `common/` 现已在 `../../project/firmware/common/`。若要编译示例，请把引用路径指过去，
  或直接把需要的文件复制进 `project/` 再改。
- 共享库 `common/`（雷达/摔倒/报警/OneNet/Modbus/motion_link，62 项单测通过）不在这里，
  已作为共享库放在 `project/firmware/common/`。

## 与正式项目的关系

| | examples/ai-scaffold | project/ |
|---|---|---|
| 性质 | AI 参考示例 | 你们的正式代码 |
| common 库 | 无（已移走） | `project/firmware/common/`（复用+改） |
| 是否交付 | 否 | 是 |
