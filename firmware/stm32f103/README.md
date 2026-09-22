# firmware/stm32f103 —— 方案 A（STM32F103RCT6）

主控 STM32F103RCT6（256KB Flash / 48KB RAM），框架 STM32CubeMX + Keil MDK5 + HAL + FreeRTOS(CMSIS-RTOS v2) + LVGL v8.3，外挂 ESP8266 上云。

## 目录

```
stm32f103/
├── App/
│   └── app_tasks.c        FreeRTOS 6 任务（雷达/传感器/报警/网络/Modbus/GUI），调用 common 业务层
├── BSP/
│   └── bsp_stm32f103.c    bsp.h 接口的 HAL 实现骨架（含串口成帧、RS485 方向切换）
├── Bootloader/
│   └── bootloader.c       IAP 引导骨架（Flash 分区、跳转 APP、W25Q64 搬运）
└── (待生成) Core/ Drivers/ Middlewares/ PROJECT_MDK/ *.ioc
```

## 构建步骤

1. 打开 STM32CubeMX，新建工程选 STM32F103RCT6：
   - 时钟 72MHz；SYS 选 SWD（释放 PB3/PB4）。
   - 使能 USART1(调试)/USART2(ESP8266)/USART3(雷达)/UART5(RS485)。
   - 使能 SPI1(触控屏)/SPI2(W25Q64)/I2C1(MLX90614)/ADC1(PA1 MQ7)。
   - `Middleware` → 勾选 FreeRTOS(CMSIS v2)；LVGL 可用内置包或手动移植。
   - 引脚对照 `hardware/wiring.md`。
2. 生成 Keil 工程后，把以下文件加入工程：
   - `../common/*.c`（业务层，纯 C，直接编译）
   - `App/app_tasks.c`、`BSP/bsp_stm32f103.c`
   - LVGL 源码 + `lv_conf.h` + `lv_port_disp/indev`
3. 把 `bsp_stm32f103.c` 里的 TODO 替换为实际 HAL 调用（句柄名对齐 CubeMX 生成）。
4. 在 CubeMX 生成的 `main.c` 里，`MX_FREERTOS_Init` 或默认任务中调用 `app_tasks_create()`。
5. 编译下载，串口1(115200) 看日志。

## OTA（IAP）要点

- APP 工程 IROM 起始改 `0x08008000`，代码里设 `SCB->VTOR = 0x08008000`。
- Bootloader 单独工程烧在 `0x08000000`，永不被覆盖。
- 固件下载到 W25Q64 缓存，校验 CRC32 通过后置升级标志，复位由 Bootloader 搬运。
- 详见 `docs/fota.md`。

## 关键坑

- PB3/PB4 需关 JTAG 只留 SWD 才能当普通 IO。
- RS485 发送后必须等 TC（发送完成）再拉低 DE/RE，否则截断末字节。
- ESP8266 必须 5V 供电且电流足，只支持 2.4G WiFi。
