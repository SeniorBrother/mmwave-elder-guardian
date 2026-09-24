# STM32F103RCT6 固件工程（方案A · 标准库 SPL）

> 目标板：**绿深 STM32F103RCT6 系统板（LQFP64，256KB Flash = High-Density）**，板载 CH340G+Type-C、8MHz 晶振、SWD。
> **开发库：STM32F10x 标准外设库（StdPeriph_Lib V3.5.0），不用 HAL。**
> 共享库在 `../common/`（已过 62 项单测，纯 C、与库无关），本工程直接复用它。

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

## 二、关于 CubeMX 的定位（标准库下）

- **CubeMX 不能生成标准库代码**（只出 HAL / LL）。所以本工程**不靠 CubeMX 生成代码**。
- CubeMX 可作为**可选的引脚/时钟规划器**：想核对某个外设引脚复用、时钟树时打开看看即可，不生成工程。
- 实际工程用一份**标准库模板**起步（三选一）：
  1. ✅ **板子资料盘自带的标准库例程**（粤嵌/绿深 F103RCT6 资料里通常有 `标准库例程`/`模板工程`）——最省事，引脚/时钟已适配本板；
  2. 正点原子 / 野火 的 **STM32F103 标准库模板**（成熟、注释全）；
  3. ST 官方 `STM32F10x_StdPeriph_Lib_V3.5.0` 里的 `Project/STM32F10x_StdPeriph_Template`。

## 三、标准库工程结构（Keil）

一份 SPL 工程典型结构（用模板即可，无需手搭库文件）：

```
Libraries/
├── CMSIS/                 core_cm3.h/.c, system_stm32f10x.c/.h,
│   └── startup_stm32f10x_hd.s      ← 256KB Flash 用 hd(High-Density) 启动文件
└── STM32F10x_StdPeriph_Driver/
    ├── inc/  stm32f10x_rcc.h, _gpio.h, _usart.h, _misc.h, _tim.h, _spi.h, _i2c.h, _adc.h ...
    └── src/  对应 .c
User/                      main.c, stm32f10x_conf.h, stm32f10x_it.c/.h, 你的 bsp/
```

**Keil 关键设置**（Options for Target）：
- **C/C++ → Define**：`USE_STDPERIPH_DRIVER, STM32F10X_HD`
  （`STM32F10X_HD` = 高密度，对应 F103RC 的 256KB Flash；务必选对，否则中断向量表错位）
- **C/C++ → Include Paths**：加 `Libraries\CMSIS`、`...\StdPeriph_Driver\inc`、`User`、以及 **`../common`**
- **Target**：芯片选 STM32F103RC；Flash/RAM 起始 `0x08000000 / 0x20000000`，Size `0x40000 / 0xC000`
- **Debug**：ST-Link，Port=**SW**，Flash Download 勾 **Reset and Run**
- 建议勾 **Use MicroLIB**（printf 重定向简单）

## 四、时钟（72MHz）

标准库模板的 `system_stm32f10x.c` 里，打开对应宏即可（`SystemInit()` 在启动文件里已自动调用）：
```c
#define SYSCLK_FREQ_72MHz  72000000   // HSE 8MHz × PLL9 = 72MHz
```
- AHB=72M，APB1=36M（/2），APB2=72M（/1），ADC=12M（APB2/6）。
- 若模板默认跑 HSI 8M，务必确认切到 HSE×9=72M（串口波特率才准）。

## 五、M1 三段最小代码（标准库 API）

### 1. LED（PA8）+ USART1 printf
```c
#include "stm32f10x.h"
#include <stdio.h>

static void LED_Init(void) {
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    g.GPIO_Pin   = GPIO_Pin_8;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);
}
static void USART1_Init(void) {
    GPIO_InitTypeDef g; USART_InitTypeDef u;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    g.GPIO_Pin = GPIO_Pin_9;  g.GPIO_Mode = GPIO_Mode_AF_PP;      g.GPIO_Speed = GPIO_Speed_50MHz; GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = GPIO_Pin_10; g.GPIO_Mode = GPIO_Mode_IN_FLOATING;                                 GPIO_Init(GPIOA, &g);
    u.USART_BaudRate = 115200; u.USART_WordLength = USART_WordLength_8b;
    u.USART_StopBits = USART_StopBits_1; u.USART_Parity = USART_Parity_No;
    u.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &u); USART_Cmd(USART1, ENABLE);
}
/* printf 重定向（MicroLIB）*/
int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}
```

### 2. 雷达 USART3（PB10/PB11）+ 接收中断
```c
static void USART3_Radar_Init(void) {
    GPIO_InitTypeDef g; USART_InitTypeDef u; NVIC_InitTypeDef n;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    g.GPIO_Pin = GPIO_Pin_10; g.GPIO_Mode = GPIO_Mode_AF_PP;      g.GPIO_Speed = GPIO_Speed_50MHz; GPIO_Init(GPIOB, &g);
    g.GPIO_Pin = GPIO_Pin_11; g.GPIO_Mode = GPIO_Mode_IN_FLOATING;                                 GPIO_Init(GPIOB, &g);
    u.USART_BaudRate = 256000;            /* ← 按 R60ABD1 手册，常见 256000 或 115200 */
    u.USART_WordLength = USART_WordLength_8b; u.USART_StopBits = USART_StopBits_1;
    u.USART_Parity = USART_Parity_No; u.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART3, &u);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    n.NVIC_IRQChannel = USART3_IRQn; n.NVIC_IRQChannelPreemptionPriority = 1;
    n.NVIC_IRQChannelSubPriority = 0; n.NVIC_IRQChannelCmd = ENABLE; NVIC_Init(&n);
    USART_Cmd(USART3, ENABLE);
}
/* stm32f10x_it.c 里 */
void USART3_IRQHandler(void) {
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        uint8_t b = USART_ReceiveData(USART3);
        /* 存入成帧缓冲，M2 交给 common/radar_proto 解析 */
    }
}
```

### 3. FreeRTOS（用内核 API，不是 CMSIS_V2）
标准库工程一般直接移植 **FreeRTOS 内核**（`xTaskCreate / vTaskDelay`），不用 CubeMX 的 CMSIS_V2 封装：
```c
#include "FreeRTOS.h"
#include "task.h"
static void TaskLed(void *arg) {
    for (;;) {
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);       /* 点灯（高电平亮→按实际接法调） */
        printf("hello guardian, tick=%lu\r\n", (unsigned long)xTaskGetTickCount());
        vTaskDelay(pdMS_TO_TICKS(500));
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
int main(void) {
    SystemInit();            /* 72MHz */
    LED_Init(); USART1_Init(); USART3_Radar_Init();
    xTaskCreate(TaskLed, "led", 256, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;);
}
```
> ⚠️ **SysTick 冲突（标准库+FreeRTOS 常见坑）**：FreeRTOS 会占用 SysTick 作系统心跳。
> 若你还想要 `delay_us()` 微秒延时，**不要**再抢 SysTick，改用一个 TIM（如 TIM2）做 us 时基；
> 毫秒级延时统一用 `vTaskDelay()`。移植 FreeRTOS 时把 `port.c`/`heap_4.c` 加进工程，`FreeRTOSConfig.h` 里 `configCPU_CLOCK_HZ` 设 72000000。

## 六、Keil 接入 common 共享库

1. 新建 Group `common`，把 `../common/` 下所有 `.c` 加入（radar_proto / fall_algo / alarm_rule / onenet_json / modbus_rtu / motion_link / guardian_app）。M1 可只加需要的。
2. Include Paths 加 `../common`（见第三节）。
3. common 是纯 C（仅 `stdint.h/string.h`），AC5/AC6 均可；与标准库共存无冲突（common 不含任何 HAL/SPL 依赖）。
4. BSP 层（`bsp_stm32f103.c`）用 **SPL API** 实现 `bsp.h` 里的接口（`bsp_uart_send`→`USART_SendData` 循环、`bsp_buzzer_set`→`GPIO_SetBits` 等）。

## 七、M1 验收（对应 task-board）

- [ ] 标准库模板工程能编译通过、ST-Link 下载成功（Define 里 `STM32F10X_HD` 正确）。
- [ ] **点灯**：任务里每 500ms 翻转 PA8（`GPIO_SetBits/ResetBits`）。
- [ ] **printf**：串口助手（115200）看到周期日志；波特率准 → 说明时钟确实是 72MHz。
- [ ] **FreeRTOS**：`vTaskStartScheduler()` 后任务在跑（`xTaskGetTickCount` 递增）。
- [ ] **雷达 USART3 成帧**：接雷达后中断能收到字节流并打印原始帧（帧头 0x53 0x59）；M2 再交 `radar_proto` 解析。

## 八、后续里程碑会用到的 SPL 外设（先了解）

- **M2**：I2C1(PB6/PB7, MLX90614)、ADC1(PA1, MQ7)、DHT11 单总线(PB9, GPIO+定时器读时序)、UART5(PC12/PD2, RS485)+PC3(DE/RE, `USART_SendData` 后等 `USART_FLAG_TC` 再拉低 DE)。
- **M3**：USART2(PA2/PA3, ESP8266 WiFi) + OneNet 上云。
- **LVGL 屏**：SPI1 + XPT2046 触控（乙负责）。
- **OTA/Bootloader、QT5 上位机**：已**退出当前排期**，目录保留（见 `../../../docs/task-board.md` 文末"保留"）。

## 九、参考

- 标准库获取：板子资料盘 `标准库例程` / ST `STM32F10x_StdPeriph_Lib_V3.5.0` / 正点原子·野火 F103 模板。
- 示例脚手架（HAL 版，仅作结构参考，API 需换成 SPL）：`../../../examples/ai-scaffold/firmware/stm32f103/`
- 雷达协议：`../../../docs/radar-protocol.md` · Modbus 寄存器表：`../../../docs/modbus-map.md`
