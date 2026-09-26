# Keil MDK 本机工程首次配置备忘（STM32F103 / 本工程）

> 适用场景：在本机首次打开 / 配置 `project/firmware/stm32f103` 的**已有 Keil 工程**。
> 目的：把本次踩过的坑与验证过的解法固化，避免重复排查。
> 状态：串口 ISP 烧录已通（LED 闪烁正常）；ST-Link SWD 烧录**挂起未解决**（见第 4 节登记）。

---

## 0. 本机环境事实（先记牢，别按默认路径猜）

| 项 | 值 |
|---|---|
| Keil 安装路径 | **`D:\work\diy\app\keil_v5`**（非默认 `C:\Keil_v5`）；UV4 在 `...\UV4\UV4.exe` |
| PACK 本地仓库 | **`D:\work\diy\app\keil_v5\ARM\PACK`**（内含 `.Download` / `.Web`） |
| 板载 Type-C | **CH340 串口**（COM 口：打印日志 / 串口 ISP 烧录），**不是调试器** |
| 板载调试器 | **无**。SWD 调试必须外接探针 |
| 目标芯片 | STM32F103RC（256KB Flash / 48KB RAM，High-Density） |

> 找 Keil 安装路径的可靠办法：Keil 运行时用任务管理器/`Get-Process UV4` 看进程路径；
> 默认路径和注册表卸载项在本机都查不到，别浪费时间。

---

## 1. 打开已有工程（不要新建工程）

直接双击 `USER/LCD.uvprojx`。新建工程只在你想要全新目录结构时才做，且要避开 RTE 重复 startup 的坑。

魔术棒（Alt+F7）逐页核对：

- **Device**：`STMicroelectronics → STM32F1 Series → STM32F103 → STM32F103RC`。
  若报 `Cannot select device` → 缺 DFP，走第 2 节。
- **Target**：Xtal `8.0`；IROM1 `0x08000000 / 0x40000`；IRAM1 `0x20000000 / 0xC000`；勾 **Use MicroLIB**。
- **C/C++**：
  - Define = `USE_STDPERIPH_DRIVER,STM32F10X_HD`
  - Include Paths 追加 `..\..\common`（**相对 `USER/` 目录**，即 `project/firmware/common`；别写成 `..\common`）。
- **Output**：勾 **Create HEX File**（串口 ISP 烧录要用 hex）。
- **加 Group**：`common` / `App` / `BSP` / `FreeRTOS`，各自 Add Files 加 `.c/.s`。
- **RTE（Manage Run-Time Environment）里 `CMSIS→CORE`、`Device→Startup` 一律不勾**：
  工程已自带 `startup_stm32f10x_hd.s` / `core_cm3.c` / `system_stm32f10x.c`，重复加会符号冲突。

---

## 2. 器件包 DFP：`Cannot select device` 的根因与绕开 Pack Installer 的解法

### 现象
- 打开工程报 `Cannot select device(STM32F103RC:STMicroelectronics), Device not found`。
- Pack Installer 卡在 `Read Pack descriptions… / 81%` 无响应，双击 `.pack` 也卡（因为双击会唤起同一个坏掉的 Pack Installer）。

### 根因（两条，都已验证）
1. 本机 PACK 仓库里**一个 STM32 器件包都没解压**（`ARM\PACK` 下只有 `.Download`、`.Web`，无 `Keil\STM32F1xx_DFP`）。
2. Pack Installer 启动要解析 `.Web` 下**几千个 `.pdsc`** 索引，本机性能/网络下直接卡死 → GUI 不可用。

### 解法：手动下载 + 手动解压，全程不用 Pack Installer
1. **拼直链下载 `.pack`**（不走全量索引，单文件 GET）：
   - 读包描述：`https://www.keil.com/pack/Keil.STM32F1xx_DFP.pdsc`
   - 取字段：`<url>=https://www.keil.com/pack/`、`<name>=STM32F1xx_DFP`、最新 `<release version>=2.4.1`
   - 按 CMSIS-Pack 规则拼：`<url> + Keil. + <name> + . + <version> + .pack`
     → `https://www.keil.com/pack/Keil.STM32F1xx_DFP.2.4.1.pack`
   - 实测约 47.9MB，ZIP 魔数 `PK\x03\x04` 完好。
2. **解压进本地仓库**（`.pack` 就是 ZIP；`.pdsc` 必须直接位于版本层）：
   ```
   D:\work\diy\app\keil_v5\ARM\PACK\Keil\STM32F1xx_DFP\2.4.1\Keil.STM32F1xx_DFP.pdsc
   ```
   PowerShell 一条命令：
   ```powershell
   Add-Type -AssemblyName System.IO.Compression.FileSystem
   [System.IO.Compression.ZipFile]::ExtractToDirectory($pack路径, $上述2.4.1目录)
   ```
   （或重命名 `.pack`→`.zip` 后右键解压，把内容放进 `2.4.1\`。）
3. **完全退出 UV4 再重开**（uVision 启动时扫描 PACK 目录注册器件）。

### 验证
- Device 页出现 `STM32F1 Series → STM32F103RC`，报错消失。
- Debug → Settings → **Flash Download** 见 `STM32F10x High-density` 算法。

### 注意（high-density 算法的坑）
- 算法显示 **512k / 08000000–0807FFFF**，但 F103RC 实际 **256K**。该算法向上兼容可用，但：
  - **烧录选 `Erase Sectors`，别选 `Erase Full Chip`**（Full Chip 会按 512K 擦不存在的页）。
  - Target 页 **IROM1 Size 保持 `0x40000`**，别链接出超 256K 的镜像。

---

## 3. 下载/调试通道：分清两条链路（最易误判）

| 链路 | 物理接口 | 能力 | 本机状态 |
|---|---|---|---|
| 串口 | Type-C → **CH340** → USART | 打印日志 + **串口 ISP 烧录** | ✅ 已通（LED 闪烁即此通道烧录） |
| SWD | 外接探针 → PA13/PA14 | 烧录 + **断点调试** | ⚠️ 挂起（见第 4 节） |

- **只插 Type-C 就点 Keil Load 必报 `No Debug Unit Device found`**——因为 Type-C 是串口不是调试器，本机又无板载调试器。这不是故障，是通道选错。
- 串口 ISP 烧录流程（无探针时的主力）：BOOT0=1 → 复位 → 工具（CubeProgrammer/FlyMcu/stm32flash）连 CH340 的 COM 烧 hex → BOOT0=0 → 复位运行。前提：CH340 接 USART1（F103 ROM bootloader 只认 USART1）。
- 要断点调试才需要外接 ST-Link/J-Link/DAPLink，接 PA13(SWDIO)/PA14(SWCLK)/GND(/3V3)。

---

## 4. ST-Link SWD 烧录：已知问题登记（本次挂起，后续再战）

**已验证 OK：**
- 驱动装 **STSW-LINK009**（最新 v2.0.2，管理员安装）；设备管理器见 `STM32 STLink` 无感叹号。
- Keil 能读到 **SW-DP IDCODE `0x1BA01477`**（ARM CoreSight SW-DP）= 探针↔芯片 SWD 物理链路通。

**未解决现象：**
- Load 报 `Error: Flash Download failed - Target DLL has been cancelled`；
- 并伴随残留弹窗 `CMSIS-DAP - Cortex-M Error: No Debug Unit Device found`（说明某次 Load 实际用的驱动仍是 CMSIS-DAP，驱动未正确切换/保存）。

**下次再战的排查清单（按序）：**
1. Debug 页下拉确为 `ST-Link Debugger`，且 ST-Link Settings 里点**"确定"**保存（消除 CMSIS-DAP 残留）。
2. Flash Download 页：算法在列 + RAM for Algorithm `0x20000000 / 0x1000` + 勾 Reset and Run + Erase Sectors。
3. Connect 改 **under Reset**；SWD 时钟降 **1 MHz**。
4. 关闭占用 ST-Link 的程序（CubeProgrammer / OpenOCD / 其它 Keil 实例）。
5. 用 STM32CubeProgrammer 交叉验证：能烧=纯 Keil 配置问题；提示读保护/RDP=芯片保护，Option Bytes 改 Level 0（全片擦除）后再试。

**当前 workaround：Type-C 串口 ISP 烧录（已可用），不影响 M1 推进。**

---

## 5. 一键自检清单（换机器 / 重配时照做）

1. UV4 打开 `USER/LCD.uvprojx`，Device 页 = STM32F103RC 且不报 device not found → 否则走第 2 节装 DFP。
2. `Rebuild all` → 0 error。
3. 烧录二选一：有探针走 ST-Link Load；无探针走串口 ISP 烧 `OBJ/LCD.hex`。
4. 现象验证：LED / 1.8寸屏 / 串口 115200 任一正常 = 链路通。
