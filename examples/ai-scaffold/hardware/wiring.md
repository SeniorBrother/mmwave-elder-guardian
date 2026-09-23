# 接线表（方案 A：STM32F103RCT6）

> 供电约定：ESP8266 / 雷达 / MQ7 / MLX90614 用 5V；TFT 屏、MAX485(3.3V版)、W25Q64 用 3.3V。所有模块 GND 共地。

## 主控引脚分配

| 模块 | 模块引脚 | STM32 引脚 | 供电 | 说明 |
|------|----------|-----------|------|------|
| TFT 2.8寸 ILI9341 | VCC | 3.3V | 3.3V | 电源 |
|  | GND | GND | — | 地 |
|  | CS | PA4 | — | LCD 片选 |
|  | RESET | PB1 | — | 复位 |
|  | DC/RS | PB0 | — | 数据/命令 |
|  | SDI/MOSI | PA7 | — | SPI1 MOSI |
|  | SCK | PA5 | — | SPI1 SCK |
|  | LED/BL | PB12 | — | 背光 |
|  | SDO/MISO | PA6 | — | SPI1 MISO(触摸共用) |
| 触摸 XPT2046 | T_CS | PB13 | — | 触摸片选 |
|  | T_IRQ | PB14 | — | 触摸中断 |
| ESP8266 | RXD | PA2 (USART2_TX) | — | 接单片机发送 |
|  | TXD | PA3 (USART2_RX) | — | 接单片机接收 |
|  | VCC | 5V | 5V | 峰值电流大，务必5V |
|  | GND | GND | — | |
| 60G 雷达 R60ABD1 | VCC | 5V | 5V | |
|  | GND | GND | — | |
|  | RXD | PB10 (USART3_TX) | — | |
|  | TXD | PB11 (USART3_RX) | — | |
| MLX90614 | VCC | 5V | 5V | |
|  | GND | GND | — | |
|  | SCL | PB6 (I2C1_SCL) | — | |
|  | SDA | PB7 (I2C1_SDA) | — | |
| DHT11 | VCC | 3.3/5V | — | |
|  | GND | GND | — | |
|  | DAT | PB9 | — | 单总线 |
| MQ7 | VCC | 5V | 5V | 加热型，需预热 |
|  | GND | GND | — | |
|  | AO | PA1 (ADC1_CH1) | — | 模拟输出 |
| MAX485 (RS485) | VCC | 3.3V | 3.3V | 3.3V 版本 |
|  | GND | GND | — | |
|  | DI | PC12 (UART5_TX) | — | 发送数据入 |
|  | RO | PD2 (UART5_RX) | — | 接收数据出 |
|  | DE+RE | PC3 | — | 方向控制(并接) |
|  | A | 总线A | — | 差分正 |
|  | B | 总线B | — | 差分负 |
| W25Q64 (OTA) | VCC | 3.3V | 3.3V | |
|  | GND | GND | — | |
|  | CS | PB8 | — | |
|  | SCK | PB3 | — | SPI2 (关JTAG释放) |
|  | MISO | PB4 | — | SPI2 |
|  | MOSI | PB5 | — | SPI2 |
| 蜂鸣器 | + | VCC | — | 经 SS8050 三极管驱动 |
|  | - | PA8→三极管 | — | 高电平响，反并 1N5819 |
| LED | 运行 | PC13 | — | |
|  | 上传 | PC14 | — | |
|  | 雷达 | PC15 | — | |
| 按键 | K1(解除) | PC0 | — | 上拉输入 |
|  | K2(SOS) | PC1 | — | 上拉输入 |
| 调试 | USB-TTL RX | PA9 (USART1_TX) | — | printf 日志 |
|  | USB-TTL TX | PA10 (USART1_RX) | — | |
| 下载 | SWDIO/SWCLK | PA13/PA14 | — | ST-Link |

## 供电注意

1. ESP8266 必须 5V 且电流充足，否则"AT 能响应、一连路由器就重启"。
2. 雷达接 5V，走线短、远离 ESP8266 天线，减少射频干扰。
3. MQ7 加热型，上电预热 20s~几分钟，代码开机采基准 `MQ7_base` 做零点补偿。
4. 蜂鸣器等感性/大电流负载不要与传感器共用同一供电支线，避免拉低电压导致复位。
5. PB3/PB4 默认是 JTAG 引脚，需在代码里 `JTAG_Set(SWD_ENABLE)` 关闭 JTAG 只保留 SWD 才能当普通 IO。

## 方案 B：ESP32-S3 引脚

ESP32-S3 GPIO 可自由映射，见 [docs/wiring-esp32s3.md]（实现阶段定稿）。要点：
- 避开 strapping 脚 GPIO0/45/46 与 USB 脚 GPIO19/20。
- MQ7 接 ADC1 通道（GPIO1~10），避免 ADC2（WiFi 工作时不可用）。
- RS485 用 UART2，DE 脚可用 `uart_set_mode(UART_MODE_RS485_HALF_DUPLEX)` 硬件自动控制。
- 雷达/MLX90614/DHT11 均为 3.3V 电平，与 ESP32-S3 直连更友好（雷达 VCC 仍可 5V，看模块）。
