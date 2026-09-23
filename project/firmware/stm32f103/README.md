# STM32F103RCT6 固件工程（方案A）

> 目标板：**绿深 STM32F103RCT6 系统板（LQFP64）**，板载 CH340G+Type-C、8MHz 晶振、SWD。
> 本文是 M1 的 CubeMX 配置清单 + Keil 接入 `common` 共享库的步骤。
> 共享库在 `../common/`（已过 62 项单测），本工程直接复用它。

## 一、板载资源（据原理图确认）

| 资源 | 引脚 | 说明 |
|------|------|------|
| 调试串口 USART1 | PA9=TX / PA10=RX | 经板载 **CH340G + Type-C** 出，printf 直接用，无需外接 USB-TTL |
| SWD 下载/调试 | PA13=SWDIO / PA14=SWCLK | 接 ST-Link |
| HSE 晶振 | OSC_IN/OSC_OUT | **8MHz**，PLL×9 → 72MHz |
| 用户 LED（LED2） | **PA8** | PA8→R11→GND，**高电平点亮**；LED1 是电源指示 |
| 按键 | PA0(WK_UP) / PC8 / PC9 / RESET | 均按下接地，需上拉 |
| BOOT0 / BOOT1 | 跳线 JP3/JP5 | 正常从 Flash 启动：BOOT0=0 |

> ⚠️ **引脚冲突提醒**：早期方案把 **PA8 分配给蜂鸣器**，但本板 PA8 已接板载 LED2。
> M1 先用 PA8 点灯；后续蜂鸣器改到 JP1/JP3 引出的其它空闲引脚（如 PB0/PB1/PC3 等，接线前用万用表确认未被占用）。

## 二、CubeMX 配置清单（M1）

新建工程 → 选 **STM32F103RCT6** → 进入 Pinout & Configuration：

### 1. System Core
- **SYS** → Debug = **Serial Wire**（保留 PA13/PA14 给 SWD）。
- **RCC** → High Speed Clock(HSE) = **Crystal/Ceramic Resonator**；LSE 先不启用（暂不用 RTC）。

### 2. 串口
- **USART1** → Mode = **Asynchronous**（默认 PA9/PA10）。参数：**115200, 8-bit, None, 1 Stop**。用途：调试 printf。
- **USART3** → Mode = **Asynchronous**（默认 **PB10=TX / PB11=RX**）。参数按**雷达手册**设置（R60ABD1 常见 256000 或 115200，先查手册）。用途：接毫米波雷达。
  - 在 **NVIC Settings** 勾 **USART3 global interrupt**（接收用）。
  - 成帧方式二选一（M2 再细化）：① 定时器 20ms 空闲判定；② USART IDLE 线中断。M1 先能收到字节并打印即可。

### 3. GPIO
- **PA8** → **GPIO_Output**，User Label 填 `LED`（M1 点灯用）。

### 4. Clock Configuration（时钟树）
- PLL Source Mux = **HSE**；PLLMul = **×9**；
- SYSCLK = **72MHz**，AHB Prescaler = /1（72M），APB1 = /2（36M），APB2 = /1（72M），ADC Prescaler = /6（12M）。
- 直接点 **Resolve Clock Issues** 让 CubeMX 自动填也行。

### 5. Middleware → FREERTOS
- Interface = **CMSIS_V2**。
- 默认会建 `defaultTask`；M1 先用它跑点灯+printf，M2 起再按 `examples/ai-scaffold/firmware/stm32f103/App/app_tasks.c` 拆多任务。

### 6. Project Manager
- Project Name：`guardian_f103`（自定）。
- **Toolchain / IDE = MDK-ARM，Version = V5**。
- Code Generator：勾 **Generate peripheral initialization as a pair of '.c/.h' files per peripheral**；保留 "Do not generate the main()" 默认。
- **GENERATE CODE** → 用 Keil 打开生成的 `.uvprojx`。

## 三、Keil 接入 common 共享库

1. **加源文件**：在 Keil 里新建 Group `common`，把 `../common/` 下所有 `.c` 加入
   （radar_proto / fall_algo / alarm_rule / onenet_json / modbus_rtu / motion_link / guardian_app）。
   > 若 M1 只想先跑通，可只加 `guardian_app.c` 及其依赖，其余按需增量加入。
2. **加头文件路径**：Options for Target → C/C++ → **Include Paths** 添加 `../common`。
3. **编译器**：common 是纯 C（仅用 `stdint.h/string.h`），AC5/AC6 均可编译；建议 AC6 并开 C99。
4. **printf 重定向到 USART1**（AC6 示例，放在 `usart.c` 或新建 `retarget.c`）：
   ```c
   #include "usart.h"
   #include <stdio.h>
   int fputc(int ch, FILE *f) {
       HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
       return ch;
   }
   ```
   Keil 里 Options → Target → 勾 **Use MicroLIB**（否则 printf 需额外重定向 `_sys_*`）。
5. **下载/调试**：Options → Debug → 选 **ST-Link Debugger** → Settings → Port=**SW**，Flash Download 勾 Reset and Run。

## 四、M1 验收（对应 task-board）

- [ ] 生成工程能编译通过、能用 ST-Link 下载。
- [ ] **点灯**：`defaultTask` 里每 500ms 翻转 PA8（`HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8)`）。
- [ ] **printf**：串口助手（115200）看到周期性日志，如 `hello guardian, tick=%lu`。
- [ ] **FreeRTOS**：确认调度器在跑（一个任务里 vTaskDelay 周期打印即可）。
- [ ] **雷达 USART3 成帧**：接雷达后能收到字节流并打印原始帧（帧头 0x53 0x59）；M2 再交给 `radar_proto` 解析。

> 点灯 + printf 示例（放在 `defaultTask` 的 while 里）：
> ```c
> HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
> printf("hello guardian, tick=%lu\r\n", (unsigned long)osKernelGetTickCount());
> osDelay(500);
> ```

## 五、后续里程碑会新增的 CubeMX 配置（先了解，暂不配）

- **M2**：I2C1(PB6/PB7, MLX90614)、ADC1_IN1(PA1, MQ7)、DHT11 单总线(PB9)、UART5(PC12/PD2, RS485)+PC3(DE/RE)。
- **M3**：USART2(PA2/PA3, ESP8266 WiFi) + OneNet 上云。
- **SPI 屏（LVGL 本地交互）**：SPI1 + 触控（乙负责）。
- **OTA/Bootloader、QT5 上位机**：已**退出当前排期**，目录保留，后期阶段再加（见 `../../../docs/task-board.md` 末尾"保留（后期阶段）"节）。

## 六、参考

- 示例脚手架（任务/BSP/Bootloader 骨架）：`../../../examples/ai-scaffold/firmware/stm32f103/`
- 雷达协议：`../../../docs/radar-protocol.md`
- Modbus 寄存器表：`../../../docs/modbus-map.md`
