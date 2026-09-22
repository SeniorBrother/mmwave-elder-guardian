# firmware/common —— 跨平台业务层（纯 C，可单元测试）

与芯片无关的核心业务逻辑，STM32 与 ESP32-S3 两套 BSP 共用。所有模块为纯 C99，
不依赖 HAL/ESP-IDF，可在 PC 上用 gcc 直接编译测试。

## 模块

| 文件 | 职责 |
|------|------|
| `guardian_data.h` | 监测数据模型 `guardian_data_t` + 报警位图定义 |
| `radar_proto.c/h` | R60ABD1 雷达帧解析（校验和/事件分发/使能命令） |
| `fall_algo.c/h` | 摔倒检测（心率+呼吸滑动窗口方差突变，整数实现无 FPU 依赖） |
| `alarm_rule.c/h` | 报警判定（阈值可调，返回报警位图） |
| `onenet_json.c/h` | OneNet 数据点上报 JSON 组包（体温不依赖 %f） |
| `modbus_rtu.c/h` | Modbus RTU 从站协议栈（CRC16 / 6 功能码 / 寄存器映射） |
| `guardian_app.c/h` | 业务编排器：把上述模块串成端到端逻辑，两套任务都调它 |
| `motion_link.c/h` | 分层双主控（AI大脑↔运动小脑）UART 协议编解码，见 docs/mobile-extension.md |
| `bsp.h` | 板级支持包抽象接口（各平台在 BSP/ 实现） |
| `CMakeLists.txt` | 让本目录作为 ESP-IDF 组件被复用（Keil 侧忽略） |
| `test/` | gcc 单元测试 |

## 单元测试

```bash
cd test
bash build_and_test.sh
# 或手动: gcc -Wall -Wextra -std=c99 -I.. test_main.c ../*.c -o run_tests && ./run_tests
```

当前覆盖：雷达帧解析/校验、摔倒边沿触发、报警组合、OneNet JSON、
Modbus CRC16 与读写/异常、guardian_app 端到端、motion_link 协议 round-trip。**62 项全部通过**。

## 设计原则

- 业务层只依赖 `bsp.h` 抽象接口，不出现任何 HAL_/esp_ 调用。
- 所有对外函数做 NULL 检查，返回明确成功/失败。
- 数据结构集中，多线程访问由调用方（任务）用互斥量保护。
