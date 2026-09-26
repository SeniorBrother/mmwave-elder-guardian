#include "FreeRTOS.h"
#include "task.h"
#include "delay.h"
#include "usart.h"
#include <stdio.h>
#include "led.h"
#include  "lcd_init.h"
#include "lcd.h"

/* ... existing includes: delay.h sys.h usart.h stdio.h led.h lcd_init.h lcd.h ... */

static void Task_Led(void *pv)
{
    (void)pv;
    for(;;)
    {
        LED = ~LED;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void Task_Log(void *pv)
{
    uint32_t n = 0;
    (void)pv;
    for(;;)
    {
        printf("[M1] sched tick %lu\r\n", (unsigned long)n++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* hooks required because configUSE_MALLOC_FAILED_HOOK=1 / CHECK_FOR_STACK_OVERFLOW=2 */
void vApplicationMallocFailedHook(void)
{
    for(;;);
}
void vApplicationStackOverflowHook(TaskHandle_t x, char *name)
{
    (void)x; (void)name;
    for(;;);
}

int main(void)
{
    delay_init();
    uart_init(115200);
    printf("\r\n[M1] boot, starting FreeRTOS...\r\n");
    LED_Init();
    LCD_Init();
    LCD_Fill(0,0,LCD_W,LCD_H,WHITE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  /* must match configMAX_SYSCALL_INTERRUPT_PRIORITY */

    xTaskCreate(Task_Led,  "led", 128, NULL, 2, NULL);
    xTaskCreate(Task_Log,  "log", 256, NULL, 1, NULL);  /* 256 words: printf(float) needs stack */

    vTaskStartScheduler();

    for(;;);  /* never reached */
}

