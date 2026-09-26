# FreeRTOS V9.0.0 移植教程（STM32F103RCT6 / Keil MDK5）

> 面向嵌入式新手。每一步都给出：**做什么 → 为什么 → 不这样做会怎样**。
> 代码片段注释为中英双文。本文档基于本工程**实际移植过程**整理，含真实踩坑 FAQ。

---

## 一、环境标注

| 项 | 值 |
|---|---|
| 芯片 | STM32F103RCT6（Cortex-M3, 72MHz, 256KB Flash, 48KB RAM） |
| RTOS | FreeRTOS **V9.0.0** |
| 编译器 / IDE | Keil MDK5（**ARMCC v5**，即 AC5） |
| 移植层 | `portable/RVDS/ARM_CM3`（`port.c` + `portmacro.h`） |
| 内存管理 | `portable/MemMang/heap_4.c` |
| 外设库 | STM32F10x 标准外设库 SPL（startup 向量名形如 `SVC_Handler`） |

最终目录结构（本工程实际）：

```
stm32f103/FreeRTOS/
├── FreeRTOSConfig.h              <- 自写配置 self-written config
├── include/                      <- 内核头文件 kernel headers
├── tasks.c  queue.c  list.c      <- 内核源码 kernel sources
└── portable/
    ├── MemMang/heap_4.c          <- 内存管理 memory manager
    └── RVDS/ARM_CM3/
        ├── port.c                <- 移植层 port layer
        └── portmacro.h           <- 移植层宏 port macros
```

---

## 二、移植步骤

### 步骤 1：拷贝内核源码文件

**做什么**：从官方包 `FreeRTOS/Source/` 拷贝 `tasks.c`、`queue.c`、`list.c`、`timers.c` 与 `include/` 全部头文件到工程 `FreeRTOS/` 目录。

**为什么**：FreeRTOS 内核是纯 C 通用代码，与芯片无关；调度器在 `tasks.c`，队列在 `queue.c`，双向链表在 `list.c`（就绪表/延时表都靠它），软件定时器在 `timers.c`。缺任何一个，链接期都会报 undefined symbol。

**不这样做会怎样**：
- 缺 `list.c` → 链接报 `undefined symbol vListInsert` 等；
- 缺 `queue.c` → 信号量/互斥量（本质是队列）不可用；
- `timers.c` 仅当 `configUSE_TIMERS=1` 才需要；本工程 M1 设 0，可不加入工程组（文件可拷但不编译）。

### 步骤 2：选择移植层 portable/RVDS/ARM_CM3

**做什么**：只拷 `portable/RVDS/ARM_CM3/` 下的 `port.c` 与 `portmacro.h`；`portable/MemMang/` 下只取 `heap_4.c`。

**为什么选 RVDS 而非 GCC**：移植层是"编译器 + CPU"绑定的汇编/内联层。
- `RVDS/` = RealView Development Suite = **Keil ARMCC** 专用（AC5 汇编语法）；
- `GCC/` = GNU 汇编语法，给 GCC / armclang(AC6) 用；
- `ARM_CM3` = Cortex-M3 无 FPU 的上下文切换实现。
本工程用 Keil AC5 + Cortex-M3，故 RVDS/ARM_CM3 唯一正确。

**不这样做会怎样**：
- 误用 `GCC/ARM_CM3` + AC5 → 汇编语法不识别，编译报一堆 syntax error；
- 误把多个 port（如 RVDS 与 GCC）同时加入工程 → `xPortStartScheduler` 等重复定义 L6200E；
- 误用 `ARM_CM4F`（带 FPU 保存）在 M3 上 → 上下文切换访问不存在的 FPU 寄存器，HardFault。

### 步骤 3：编写 FreeRTOSConfig.h（关键宏含义）

**做什么**：新建 `FreeRTOSConfig.h`（放 `FreeRTOS/` 根，加入 Include Path）。关键宏：

```c
/* 内核主频(Hz)，用于计算 SysTick 重装值
   Core clock (Hz), used to compute SysTick reload value */
#define configCPU_CLOCK_HZ        ((uint32_t)72000000)

/* 每秒 tick 数 = 调度精度；1000 => 1ms
   Ticks per second = scheduling resolution; 1000 => 1ms */
#define configTICK_RATE_HZ        ((TickType_t)1000)

/* 可用优先级个数(0 .. N-1)
   Number of usable priorities (0 .. N-1) */
#define configMAX_PRIORITIES      5

/* 空闲任务栈大小，单位=字(4B)
   Idle task stack size, in words (4 bytes each) */
#define configMINIMAL_STACK_SIZE  ((uint16_t)128)

/* heap_4 总堆大小(RAM)；任务栈与内核对象都从这里分配
   Total heap for heap_4; task stacks & kernel objects come from here */
#define configTOTAL_HEAP_SIZE     ((size_t)(16 * 1024))

/* 允许调用 FreeRTOS API 的最高中断优先级(数值最小者)；
   数值 >= 此值的中断才可调 ISR-safe API
   Highest IRQ priority (lowest numeric) allowed to call FreeRTOS APIs;
   IRQs with numeric priority >= this may call ISR-safe APIs */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  (5 << 4)
```

**为什么 / 不这样做会怎样**（逐宏）：
- `configCPU_CLOCK_HZ` 错（如写 8MHz 而实际 72MHz）→ SysTick 周期算错 → `vTaskDelay(500)` 实际变成数秒或数毫秒，节拍全乱。
- `configTICK_RATE_HZ` 太低（如 100）→ 调度粒度 10ms，短延时不精确；太高（如 10000）→ SysTick 中断过频，CPU 开销大。
- `configMAX_PRIORITIES` 太小 → 想建更高优先级任务时 `xTaskCreate` 行为异常/优先级被截断。
- `configMINIMAL_STACK_SIZE` 太小 → 空闲任务栈溢出，触发 stack overflow hook 或 HardFault。
- `configTOTAL_HEAP_SIZE` 超过可用 RAM → 与全局变量/栈重叠，运行期随机崩溃；太小 → `xTaskCreate` 返回 `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`，任务建不起来。
- `configMAX_SYSCALL_INTERRUPT_PRIORITY` 与 NVIC 分组不匹配 → 高优先级中断里调 API 破坏临界区 → HardFault / configASSERT 触发。

### 步骤 4：Keil 工程配置

**做什么**：
1. 新建 Group `FreeRTOS`，加入 5 个 .c：`tasks.c`、`queue.c`、`list.c`、`heap_4.c`、`port.c`。
2. 魔术棒 → C/C++ → Include Paths 追加（相对 `USER/`）：
   ```
   ..\FreeRTOS
   ..\FreeRTOS\include
   ..\FreeRTOS\portable\RVDS\ARM_CM3
   ```
3. 魔术棒 → Target → ARM Compiler 选 **Use default compiler version 5**（AC5）。

**为什么**：
- 第三条 Include Path 是让 `include/portable.h` 里的 `#include "portmacro.h"` 能找到 port 目录；第二条找内核头；第一条找自写的 `FreeRTOSConfig.h`。
- RVDS port 是 AC5 汇编语法，AC6(armclang) 不认。

**不这样做会怎样**：
- 漏第三条 → `cannot open source input file "portmacro.h"`（见 FAQ-1）；
- 漏 `heap_4.c` → 链接报 `undefined symbol pvPortMalloc`；
- 用 AC6 → port.c 汇编报错。

### 步骤 5：中断向量冲突处理（删 SPL 空壳）

**做什么**：打开 `USER/stm32f10x_it.c`，**删除/注释** `SVC_Handler`、`PendSV_Handler`、`SysTick_Handler` 三个空函数定义；**保留** `DebugMon_Handler` 及其余 handler。

**为什么**：启动文件向量表只认 `SVC_Handler/PendSV_Handler/SysTick_Handler` 这三个名字；FreeRTOS 的实现叫 `xPortSVCHandler/xPortPendSVHandler/xPortSysTickHandler`，靠 FreeRTOSConfig.h 里三个 `#define` 改名映射：

```c
/* 把 FreeRTOS 处理函数改名成 SPL 启动向量表的名字
   Rename FreeRTOS handlers to match SPL startup vector names */
#define vPortSVCHandler      SVC_Handler
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler
```

于是 port.c 已提供这三个符号；SPL 的空壳就是**第二个定义**。

**不这样做会怎样**：链接报
`L6200E: Symbol PendSV_Handler multiply defined (by port.o and stm32f10x_it.o)`（见 FAQ-2）。
注意：`stm32f10x_it.h` 里的**函数原型不用删**（原型不产生符号）。

### 步骤 6：NVIC 优先级分组 与 configMAX_SYSCALL_INTERRUPT_PRIORITY 的对应

**做什么**：`main()` 在启动调度器**之前**调用：

```c
/* 4 位全作抢占优先级，无子优先级；优先级数值 0..15 即抢占级
   All 4 bits as preemption priority, no sub-priority; numeric 0..15 = preempt level */
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
```

**为什么 / 对应关系**：
- 分组 4 使"优先级数值 = 抢占级"，0~15 线性可比；
- `configMAX_SYSCALL_INTERRUPT_PRIORITY = (5<<4)` 约定：**数值 5~15 的中断允许调用 FreeRTOS ISR-safe API**；数值 0~4 保留给"比内核临界区还关键、不受 FreeRTOS 屏蔽"的中断（这些中断里**禁止**调 FreeRTOS API）。
- 内核自身使用的 SysTick/PendSV 放在最低（`configKERNEL_INTERRUPT_PRIORITY = 15<<4`）。

**不这样做会怎样**：
- 不设分组（默认分组 0 有子优先级位）→ 优先级比较含子优先级位，与 config 宏的"纯抢占"假设不符 → 临界区屏蔽错乱 → HardFault 或死锁；
- 在数值 <5 的中断里调 `xQueueSendFromISR` 等 → 破坏临界区 → 随机崩溃。

### 步骤 7：main() 最小任务 + 启动调度器

**做什么**：

```c
#include "FreeRTOS.h"
#include "task.h"

/* LED 翻转任务：每 500ms 翻转一次
   LED toggle task: flip every 500ms */
static void Task_Led(void *pv)
{
    (void)pv;                          /* 未用参数 unused parameter */
    for(;;)
    {
        LED = ~LED;                    /* 翻转 LED toggle LED */
        vTaskDelay(pdMS_TO_TICKS(500));/* 阻塞 500ms block 500ms */
    }
}

/* 日志任务：每 1s 打印调度 tick
   Log task: print scheduler tick every 1s */
static void Task_Log(void *pv)
{
    uint32_t n = 0;                    /* 计数器 counter */
    (void)pv;
    for(;;)
    {
        printf("[M1] sched tick %lu\r\n", (unsigned long)n++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* 钩子：malloc 失败 / 栈溢出（config 开启后必须提供）
   Hooks: malloc failed / stack overflow (required when enabled in config) */
void vApplicationMallocFailedHook(void){ for(;;); }
void vApplicationStackOverflowHook(TaskHandle_t x, char *name)
{ (void)x; (void)name; for(;;); }

int main(void)
{
    delay_init();                      /* 仅调度器启动前可用 only valid before scheduler starts */
    uart_init(115200);                 /* 串口 printf 通道 UART for printf */
    LED_Init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); /* 见步骤6 see step 6 */

    xTaskCreate(Task_Led,  "led", 128, NULL, 2, NULL); /* 栈128字 优先级2 stack 128 words, prio 2 */
    xTaskCreate(Task_Log,  "log", 256, NULL, 1, NULL); /* 栈256字(printf吃栈) stack 256 words */

    vTaskStartScheduler();             /* 启动调度器，不再返回 start scheduler, never returns */

    for(;;);                           /* 理论上到不了 unreachable in theory */
}
```

**为什么**：
- 任务栈单位是**字(4B)**；`printf` 带格式转换吃栈深，日志任务给 256 字更稳。
- `vTaskStartScheduler()` 之后 CPU 交给调度器，`main` 的后续代码不再执行。
- 两个 hook 因 config 开了 `configUSE_MALLOC_FAILED_HOOK=1` 与 `configCHECK_FOR_STACK_OVERFLOW=2` 才必须提供；不开可不写。

**不这样做会怎样**：
- 不调 `vTaskStartScheduler` → 任务永远不跑；
- 栈给太小 + printf → 触发 stack overflow hook / HardFault；
- 调度器启动后还调 `delay_ms` → 见 FAQ-3。

---

## 三、通用移植说明（换芯片 / 换编译器 / 换内核版本）

### 3.1 换 Cortex-M4 / M7
- **M4 带 FPU** → `portable/RVDS/ARM_CM4F`（上下文切换会保存 FPU 寄存器）。
- **M4 不用 FPU** → 仍建议 `ARM_CM4F`（或确认内核未使能 FPU 时用 `ARM_CM3` 亦可，但首选 CM4F）。
- **M7** → FreeRTOS 9.0 的 `RVDS/` 下**没有** CM7 目录；改用 `portable/GCC/ARM_CM7/...`（配 GCC 或 AC6），或升级 FreeRTOS 到 V10+（RVDS/ARM_CM7 或 ARM_CLANG 支持更全）。
- **M0/M0+** → `portable/RVDS/ARM_CM0`。
原则：**port 目录 = 编译器族 + 核心(+FPU)**，三者必须同时匹配。

### 3.2 换 GCC 编译器
- 选 `portable/GCC/ARM_CM3`（或对应核心目录）。
- Keil 若切到 **AC6(armclang)**：9.0 无 ARM_CLANG 目录，AC6 兼容 GNU 汇编 → 用 `GCC/ARM_CM3`；或升级内核用官方 ARM_CLANG port。
- Makefile/CMake 工程同理，把 port 源文件与对应 include 目录换掉即可。

### 3.3 换其他 FreeRTOS 内核版本
以**新版本自带 Demo 的 FreeRTOSConfig.h 为基线**再改，不要直接沿用旧文件。可能变化的宏：
- V9+ 新增静态分配：`configSUPPORT_STATIC_ALLOCATION`（若启用需提供 `vApplicationGetIdleTaskMemory` 等）。
- V10+ Cortex-M4F/M33 新增：`configENABLE_FPU`、`configENABLE_MPU`、`configENABLE_TRUSTZONE`。
- V10.4+ 栈溢出检测头文件改名：`StackMacros.h` → `stack_macros.h`（hook 名不变）。
- 旧宏（本教程列的 6 个）大多保持兼容，但**默认值/含义微调**要以新版注释为准。

---

## 四、本次移植实际遇到的问题与解决（FAQ）

### FAQ-1：`cannot open source input file "portmacro.h"`
- **根因**：`portable/RVDS/ARM_CM3/` 目录里**只拷了 port.c，漏拷 portmacro.h**（文件不在磁盘上）；或该目录没加进 Include Path。`include/portable.h` 第 94 行 `#include "portmacro.h"` 依赖 Include Path 定位 port 目录。
- **修法**：① 从官方包/本机已有源码补拷 `portmacro.h` 到 `portable/RVDS/ARM_CM3/`；② Include Path 确认含 `..\FreeRTOS\portable\RVDS\ARM_CM3`。二者缺一不可。

### FAQ-2：`L6200E: Symbol SVC_Handler/PendSV_Handler/SysTick_Handler multiply defined (by port.o and stm32f10x_it.o)`
- **根因**：SPL 的 `stm32f10x_it.c` 自带这三个空壳定义；而 port.c 经 FreeRTOSConfig.h 的 `#define` 改名后也产出同名符号 → 一个符号两个定义。
- **修法**：删除 `stm32f10x_it.c` 中这三个函数**定义**（保留 `DebugMon_Handler`）；`stm32f10x_it.h` 的原型可留。让 port.c 独占这三个向量符号。

### FAQ-3：SysTick 与正点原子 delay.c 冲突
- **现象/根因**：`SYSTEM/delay.c` 的 `delay_ms/delay_us` 靠配置 SysTick 重载值并轮询；FreeRTOS 启动时**重新接管 SysTick** 作为 tick 源 → 调度器启动后 delay.c 的假设失效，`delay_ms` 时长错乱甚至卡死。
- **处理策略**：`delay_init()/delay_ms` **只在 `vTaskStartScheduler()` 之前**的初始化阶段使用；调度器启动后一切延时统一 `vTaskDelay(pdMS_TO_TICKS(x))`。不要在任务里混用 delay_ms。

### FAQ-4：源码编码（GBK）导致中文注释乱码 / 编译报 missing closing quote
- **根因**：本工程含中文的 .c/.h 为 **GBK**；若被外部编辑器存成 **UTF-8**，Keil(ARMCC) 按 GBK 读时，中文尾字节会"吞掉"紧随的闭引号 `"`，引发 `#8 missing closing quote` 及连锁 `too few arguments`。且 GB2312 字库按 GBK 字节查表，UTF-8 即使编译过屏上中文也乱。
- **预防**：含中文源文件**一律保存为 GBK/GB2312**；用 VS Code 等编辑时先看右下角编码，`Save with Encoding → GB2312/GBK`；教程/协作代码片段注释用 ASCII 或中英双文且在 Keil 内手打中文；切勿把文件转存 UTF-8。

---

## 五、验证清单

- [ ] **编译**：Rebuild all → 0 error（链接无 L6200E / undefined symbol）。
- [ ] **串口**：115200 打开 CH340 COM，复位后见 `[M1] boot, starting FreeRTOS...` 随后 `[M1] sched tick 0/1/2...` **每秒递增** = tick 与调度正常。
- [ ] **LED**：按 **500ms** 周期翻转 = Task_Led 在跑且 vTaskDelay 精度正确。
- [ ] **双任务交替**：tick 打印（优先级1）与 LED 翻转（优先级2）同时持续 = 抢占式多任务切换正常。

四项全过 = FreeRTOS V9.0.0 在本机/本板移植成功，可进入业务任务开发。
