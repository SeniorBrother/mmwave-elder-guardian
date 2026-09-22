/**
 * @file    bootloader.c
 * @brief   方案A STM32F103RCT6 IAP 引导程序骨架（独立 Keil 工程，烧在 0x08000000）
 *
 * Flash 分区（F103RCT6 256KB，高密度页 2KB）：
 *   0x08000000  Bootloader      (本程序, ~24-32KB, 永不被覆盖)
 *   0x08008000  APP             (应用, ~200KB)
 *   末尾参数区   升级标志/版本/CRC32/大小
 *   W25Q64      下载缓存 + 备份镜像（断电保护 + 回滚）
 *
 * 启动逻辑：
 *   1. 读参数区升级标志
 *   2. 若置位 -> 从 W25Q64 搬运固件到 APP 区 -> 校验 CRC32 -> 清标志/置有效
 *   3. 校验 APP 有效性(栈顶指针在 RAM 范围 + magic/CRC)
 *   4. 有效则跳转 APP，否则停留/回滚
 *
 * 重要：APP 工程必须设置向量表偏移 SCB->VTOR = APP_ADDR，
 *       且 Keil 里 APP 的 IROM 起始地址改为 0x08008000。
 */
#include <stdint.h>
/* #include "stm32f1xx_hal.h" */

#define BOOT_ADDR       0x08000000u
#define APP_ADDR        0x08008000u
#define PARAM_PAGE      (0x08000000u + 256u*1024u - 2u*1024u)  /* 最后一页 */

/* W25Q64 布局（自定义） */
#define W25Q_DOWNLOAD   0x000000u   /* 下载缓存区 */
#define W25Q_BACKUP     0x200000u   /* 上一版备份区 */

/* 参数区结构（存于内部 Flash 最后一页或备份到 W25Q64） */
typedef struct {
    uint32_t magic;         /* 0x4752444E "GRDN" */
    uint32_t upgrade_flag;  /* 1=待升级 */
    uint32_t fw_size;
    uint32_t fw_crc32;
    uint16_t fw_version;
    uint16_t reserved;
} boot_param_t;

static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu, i; int b;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (b = 0; b < 8; b++)
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
    }
    return ~crc;
}

/* APP 是否可跳转：栈顶指针落在 SRAM(0x20000000~0x2000C000) 内 */
static int app_is_valid(uint32_t app_addr)
{
    uint32_t sp = *(volatile uint32_t *)app_addr;
    return (sp >= 0x20000000u && sp <= 0x2000C000u) ? 1 : 0;
}

static void jump_to_app(uint32_t app_addr)
{
    typedef void (*pFunction)(void);
    uint32_t      jump_stack = *(volatile uint32_t *)app_addr;
    pFunction     jump_reset = (pFunction)(*(volatile uint32_t *)(app_addr + 4));

    /* 关中断、去初始化外设、关 SysTick，避免跳转后中断错乱 */
    /* __disable_irq(); HAL_DeInit(); SysTick->CTRL = 0; */
    /* 设向量表偏移（也可在 APP 里设 SCB->VTOR = APP_ADDR） */
    /* SCB->VTOR = app_addr; */
    /* __set_MSP(jump_stack); __enable_irq(); */
    (void)jump_stack;
    jump_reset();
}

/* 从 W25Q64 搬运固件到内部 APP 区（分块读->擦->写） */
static int copy_fw_from_w25q64(uint32_t size)
{
    (void)size;
    /* TODO:
       1. 擦除 APP 区所有页 (FLASH_PageErase)
       2. 循环: W25Q64 读一块 -> HAL_FLASH_Program 写入 APP_ADDR+offset
       3. 边写边算 CRC32，与参数区 fw_crc32 比对
       返回 0 成功 */
    return -1;
}

int main(void)
{
    /* TODO: HAL_Init(); SystemClock_Config(); 初始化 W25Q64 SPI; 读参数区 */
    boot_param_t param;
    /* read_param_from_flash(&param); */
    param.magic = 0; param.upgrade_flag = 0; param.fw_size = 0; param.fw_crc32 = 0;

    if (param.magic == 0x4752444Eu && param.upgrade_flag) {
        if (copy_fw_from_w25q64(param.fw_size) == 0) {
            /* 搬运并校验成功：清升级标志，置有效 */
            /* clear_upgrade_flag(); */
        } else {
            /* 搬运失败：保持旧 APP 或从 W25Q64 备份区回滚 */
        }
    }

    if (app_is_valid(APP_ADDR)) {
        jump_to_app(APP_ADDR);
    }

    /* APP 无效：停留在此，可进入串口/UART 恢复模式重新下载 */
    for (;;) { /* TODO:  recovery over UART */ }
}

/* 供 APP 侧调用的 CRC（保持一致） */
uint32_t boot_crc32(const uint8_t *d, uint32_t n) { return crc32_calc(d, n); }
