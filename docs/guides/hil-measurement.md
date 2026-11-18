# 硬件在环测量清单

本文把 2026-11-17 交接文档第 5 节列出的五项硬件测量写成可执行步骤：中断延迟、DMA 通道竞争、UART 字节间隔、I2C 恢复、WFI 看门狗余量。每项给出测量点、仪器、构建模式、判定口径和要保存的证据。host 侧的验证分层见 `docs/guides/verification.md`；本文不重复那一层，也不把仿真结果当成硬件结果。

行号在提交 `942290d4` 上核对。代码移动后先更新行号再测量。测量用的 GPIO 翻转是临时补丁，不提交；把补丁、trace 和记录一起归档。

## 1. 前置条件

### 1.1 硬件

- 三个 target 各跑一遍。IDIS 两个 target 可以复用同一块板，但要分别烧录。`targets/stategrid_meter/board`、`targets/idis_ip_meter/board`、`targets/idis_ip_meter_freertos/board` 是复制关系。`board_transport.c`、`board_transport_soc.c`、`spi_driver.c`、`uart_driver.c`、`iic_driver.c`、`board_esam.c`、`board_rn8209.c` 在 `942290d4` 上除 include 路径外逐字节相同；其他 board 文件可能有差异，测量前按本产品的 board 重新核对。一个产品测得的结果不能外推到另一个产品。
- 逻辑分析仪：至少 6 通道，采样率不低于 24 MS/s，测量中断延迟时能分辨 1 µs 以内的边沿。需要连续采集 24 小时，或者支持分段采集。
- 示波器：3 通道。I2C 用 SCL、SDA、电源三个通道；UART 用其中 2 个通道确认波形。
- USB 转 UART：接调试串口，保存运行日志。
- 可调直流电源：记录每次运行的电压。
- 跳线：用于把 I2C SDA 拉低，制造总线卡死。
- ESAM 卡、RN8209 计量输入源、645/698 通信对端。没有对端时，UART 字节间隔只能测发送侧。

### 1.2 探针点

| 信号 | 位置 | 用于 |
|---|---|---|
| UART3 TX、RX | RN8209 插座 | 2、4 |
| UART0、UART1、UART2 TX、RX | 645/698 通道连接器 | 4 |
| SPI2 SCK、MOSI、MISO | ESAM 插座 | 3 |
| ESAM CS | GPIOC0（`targets/idis_ip_meter/board/esam_pins.c:5`） | 3 |
| I2C SCL | GPIOC4（`targets/idis_ip_meter/board/iic_pins.c:47`） | 5 |
| I2C SDA | GPIOC5（`targets/idis_ip_meter/board/iic_pins.c:31`） | 5 |
| EEPROM 电源 | GPIOC6（`targets/idis_ip_meter/board/iic_pins.c:5`） | 5 |
| 标记 GPIO | 板上空闲引脚，临时初始化为输出 | 2、3、6 |
| 看门狗喂狗 | `nd_board_watchdog_kick`（`targets/idis_ip_meter/board/board_transport_soc.c:10`）内临时翻转 | 6 |

标记 GPIO 要选一个在测量期间不被产品使用的引脚。每次翻转用 `targets/idis_ip_meter/board/pin_ops.h:26` 的 `nd_pin_write`，只写位带地址，不要调用带循环的延时函数。

### 1.3 固件变体

| 变体 | 构建定义 | 用于 |
|---|---|---|
| POLL | 不定义，默认 `ND_BOARD_MODE_POLL`（`targets/idis_ip_meter/board/board_transport.h:18`） | 全部项的对照 |
| UART-IRQ | `-DND_BOARD_UART_MODE=1` | 2、4、6 |
| SPI-DMA | `-DND_BOARD_SPI_MODE=2` | 3、6 |
| IRQ-DMA | 同开 UART-IRQ 和 SPI-DMA | 2、3、4、6 |
| IIC-STUB | `-DND_BOARD_IIC_MODE=2` | 只确认失败返回，不测时序 |
| FreeRTOS | CMake 自动定义 `ND_BOARD_USE_FREERTOS=1`（`targets/idis_ip_meter_freertos/build/CMakeLists.txt:22`） | 2 到 6 |

IDIS 裸机构建：

```sh
cmake -S targets/idis_ip_meter/build -B build/hil-idis -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DND_BOARD_UART_MODE=1 -DND_BOARD_SPI_MODE=2 -DND_BOARD_IIC_MODE=0
cmake --build build/hil-idis
```

FreeRTOS 构建把 `-S` 换成 `targets/idis_ip_meter_freertos/build`。三个模式通过 cache 变量同时进入 board 和 app 目标（`targets/idis_ip_meter/build/CMakeLists.txt:66` 到 `:74`）。

stategrid 没有 CMake 固件构建。它用 IAR 工程 `targets/stategrid_meter/iar for arm/VG_DTZY.ewp`，工程里的三个模式固定为 0（`targets/stategrid_meter/iar for arm/VG_DTZY.ewp:229` 到 `:231`，以及 `:1300` 到 `:1302`）。所以 stategrid 只跑 POLL 变体；IRQ/DMA 只有 `tools/check-board-modes.sh:90` 到 `:99` 的交叉编译证据，没有固件。

`tools/check-board-modes.sh` 编译 15 个 board 和模式组合。它只证明能编译，不产生任何时序。

### 1.4 每次运行的记录

每个 trace 或日志旁边放一份 `run.md`，至少写：

- 提交 SHA、构建目录、`ND_BOARD_UART_MODE`、`ND_BOARD_SPI_MODE`、`ND_BOARD_IIC_MODE`、`ND_BOARD_USE_FREERTOS`。
- 板号、芯片批次、工具链版本、电源电压、环境温度、运行时长。
- 探针点和标记 GPIO 的引脚号。
- 被测产品的 target 路径。

证据按 `<sha>-<target>-<variant>-<measurement>-<yyyymmdd>` 命名。逻辑分析仪保存原始文件；示波器保存截图和 CSV。

## 2. 中断延迟

**代码路径。** UART RX 中断从 `UART3_IRQHandler` 进入。IDIS 的向量入口在 `infra/soc/v85xxp/v8500/nd_v8500_irq.c:64`，经 `uart_irq`（`:33`）和 RX 分支（`:39`）调用注册表，注册在 `targets/idis_ip_meter/board/board_transport_soc.c:50`，最终进入 `uart_rx_isr`（`:20`）。DMA 中断从 `DMA_IRQHandler`（`nd_v8500_irq.c:98`）进入，经 `IRQ_DMA` 注册（`board_transport_soc.c:185`）调用 `spi_dma_isr`（`:156`）。

stategrid 的入口在自己的文件里：`targets/stategrid_meter/board/cpu_v85xxp_isr.c:301`（UART0）、`:393`（UART3）、`:218`（DMA），之后走同一套 `Isr_Function` 分发表。

**测量点。** 在以下函数的第一条语句之前临时翻转标记 GPIO 置高，函数返回前拉低：

- `targets/idis_ip_meter/board/board_transport_soc.c:20` 的 `uart_rx_isr`。
- `targets/idis_ip_meter/board/board_transport_soc.c:30` 的 `uart_tx_isr`。
- `targets/idis_ip_meter/board/board_transport_soc.c:156` 的 `spi_dma_isr`。

同时把 UART RX 探针接到同一个逻辑分析仪，触发条件设为 RX 起始位下降沿。延迟是 RX 边沿到标记 GPIO 上升沿的时间。脉冲宽度是 ISR 执行时间。GPIO 写入本身约几个周期，作为固定偏置记录，不要从结果里减。

**仪器。** 逻辑分析仪，2 个通道（RX 和标记 GPIO）。FreeRTOS 下额外测一次高优先级任务运行时的最坏值。

**构建模式。** UART-IRQ 或 IRQ-DMA，IDIS 裸机和 FreeRTOS 各一次。POLL 没有可测的 ISR。DMA 中断在 SPI-DMA 或 IRQ-DMA 下测。

**判定。** 仓库没有中断延迟预算，第一次测量设定基线。基线必须记录：至少 1000 个样本的最大值、中位数、最大值发生时的后台负载。物理上限是一个字符时间：UART3 按 `targets/idis_ip_meter/board/board_rn8209.c:20` 的 4800 × 1.676191 bps 配置，一个字符约 1.4 到 1.5 ms（推算）。延迟加上 ISR 时间超过这个值，UART 数据寄存器可能在 ISR 读取之前被下一个字节覆盖。DMA 侧还要求中断在 `ND_BOARD_SPI_TIMEOUT_US`（20000 µs，`board_transport.h:58`）以内完成一次唤醒，否则整个分块按超时失败（`board_transport.c:250`）。

**证据。** 逻辑分析仪原始文件、CSV、通道设置截图；`run.md`；样本数和最大值所在的 trace 片段。

## 3. DMA 通道竞争

**代码路径。** SPI2 的 TX 用 DMA 通道 2，RX 用通道 3（`board_transport_soc.c:116` 和 `:117`）。一次分块从 `nd_board_spi_dma_start`（`targets/idis_ip_meter/board/board_transport.c:183`）进入 `nd_board_spi_dma_hw_start`（`board_transport_soc.c:175`），两个通道分别完成，`spi_dma_isr`（`:156`）调用 `tx_done` 和 `rx_done`（`board_transport.c:206` 和 `:218`），两个都完成后唤醒等待者。ESAM 的请求帧在 `targets/idis_ip_meter/board/spi_driver.c:306` 起按 256 字节暂存，满一块调用 `esam_flush`（`:223`）并进入 `nd_board_spi_dma_transfer`（`:236`）。CS 释放和 `bus_delay` 也会触发 flush（`spi_driver.c:274`、`targets/idis_ip_meter/board/board_esam.c:29`、`:52`）。

计量侧同时有 UART3 的 RN8209 读写（`targets/idis_ip_meter/board/board_rn8209.c:31` 和 `:62`），以及每 10 ms 一次的 SysTick。竞争发生在两个 DMA 通道之间、DMA 与 CPU 之间、以及 DMA 中断与 UART 中断之间。

**测量点。** 标记 GPIO 在 `nd_board_spi_dma_hw_start` 入口置高，在 `spi_dma_isr` 两个通道都完成后拉低。逻辑分析仪同时接 SPI2 的 SCK、MOSI、MISO 和 CS，以及 UART3 TX、RX。

**仪器。** 逻辑分析仪，至少 6 通道。运行 30 分钟连续流量。

**构建模式。** SPI-DMA 或 IRQ-DMA，UART 分别用 POLL 和 IRQ，IDIS 裸机和 FreeRTOS 各一次。

**判定。** 每个 256 字节分块必须在 `ND_BOARD_SPI_TIMEOUT_US`（20000 µs，`targets/idis_ip_meter/board/board_transport.h:58`）内完成，`nd_board_spi_dma_transfer` 不得返回 `ND_BOARD_TIMEOUT`（`board_transport.c:250`）。计量读取不得返回错误（`board_rn8209.c:79`）。`busy` 在传输期间为 1，第二次启动必须被拒绝（`board_transport.c:186` 到 `:188`），trace 里不得出现两个分块重叠。仓库没有吞吐预算，第一次测量记录每个分块的最大耗时、从 hw_start 到唤醒的最大时间、以及计量读取的最大延迟，作为基线。

**证据。** 逻辑分析仪原始文件和 CSV；超时或重入时的 trace；`run.md`；计量读取的错误计数。

## 4. UART 字节间隔

**代码路径。** 分两类，测量时分开记录：

- RN8209 计量口在 UART3，波特率 4800 × 1.676191（`board_rn8209.c:20`）。发送在 `bus_write`（`board_rn8209.c:31`），接收在 `bus_read`（`:62`）。IRQ 模式下接收字节由 `uart_rx_isr`（`board_transport_soc.c:20`）填入 128 字节环（`board_transport.h:42`），环满时保留最旧、丢弃最新（`board_transport.c:129`）。
- 645/698 通道在 UART0、UART1、UART2。UART0 和 UART1 为 9600 bps，UART2 为 1200 bps（`targets/idis_ip_meter/app/board_ports.c:57` 到 `:68`）。发送由 `uart_time_process`（`targets/idis_ip_meter/board/uart_driver.c:92`）按 10 ms 节拍推进，`uart_driver.c:128` 到 `:143` 只有在 `tx_time_out` 归零时才写下一个字节。这就是要测的间隔来源。节拍来自 `SysTick_Handler`（`targets/idis_ip_meter/app/baremetal.c:13`）或 FreeRTOS 的 app task（`targets/idis_ip_meter_freertos/app/freertos.c:102`）。

**测量点。** 逻辑分析仪接发送通道，测起始位到下一个起始位的时间。接收方向接对端发送和本机 RX，测同一个帧内的字节间隔。IRQ 模式下再测一次 `uart_rx_isr` 的入口时间，和中断延迟分开记录。

**仪器。** 逻辑分析仪或示波器。发送侧至少 1 通道，接收侧 2 通道。

**构建模式。** POLL、UART-IRQ、IRQ-DMA 三种，IDIS 裸机和 FreeRTOS 各一次。POLL 和 IRQ 的差别主要在接收路径；发送完成在 IRQ 模式改为 TX 中断完成位，但字节节拍仍由 `uart_time_process` 决定。两种都要各测一次确认。

**判定。** 仓库没有字节间隔预算。第一次测量必须记录每个通道的间隔最小值、最大值、中位数，以及一帧的完整时长。判定通过还需要对端设备的字节间超时值；这个值不在本仓库里，由产品线提供。没有对端超时值时，只能记录基线，不能判定通过。硬性要求是不丢字节：`board_rn8209.c:79` 的读取返回 0，接收环没有因为溢出丢字节。

**证据。** 逻辑分析仪原始文件；按通道分开的 CSV；对端超时值的出处；`run.md`。

## 5. I2C 恢复

**代码路径。** EEPROM 走 bit-bang，`nd_board_iic_write`（`targets/idis_ip_meter/board/iic_driver.c:163`）和 `nd_board_iic_read`（`:185`）。每个字节的 ACK 等待在 `iic_driver.c:111` 到 `:118`，最多 200 次轮询（`:113`），超时返回失败。`nd_board_iic_init`（`:148`）只把 SDA 和 SCL 拉高，没有 9 个时钟脉冲的总线恢复序列。EEPROM 侧的错误累计在 `targets/idis_ip_meter/board/board_eeprom.c:21`，连续 5 次失败置 `DEVICE_ERR` 的位 1（`board_eeprom.c:8`）。

IIC 的 IRQ 和 DMA 模式是失败桩（`iic_driver.c:224` 到 `:248`），`nd_board_iic_*` 直接返回 -1（`iic_driver.c:236` 和 `:247`）。这两个模式没有可测的时序。

**测量点。** 示波器接 GPIOC4（SCL）和 GPIOC5（SDA），探头地接板地。EEPROM 电源 GPIOC6 也接一个通道。正常读写在 `targets/idis_ip_meter/board/iic_driver.c:163` 的 `nd_board_iic_write` 和 `:185` 的 `nd_board_iic_read` 入口及返回前加标记 GPIO。

**故障注入。** 在 `nd_board_iic_read` 进行中用跳线把 SDA 拉到地，保持到传输返回失败；然后松开跳线，调用一次 `nd_board_iic_init`，再读同一地址。重复 20 次。第二种注入是在读的中途用 GPIOC6 断开 EEPROM 电源，再恢复。

**仪器。** 示波器，3 通道（SCL、SDA、电源或标记）。调试器读取 `DEVICE_ERR` 和 `error_streak`。

**构建模式。** 只有 POLL，IDIS 裸机和 FreeRTOS 各一次。IIC-STUB 只用于确认返回 -1，不测恢复。

**判定。** 传输必须返回，不能挂死；200 次轮询是仓库给出的上界（`iic_driver.c:113`）。恢复后第一次读取必须成功，`DEVICE_ERR` 的位 1 在第一次成功后清零（`board_eeprom.c:13` 到 `:16`）。仓库没有恢复时间预算，第一次测量记录：从故障注入到返回失败的时间、从松开跳线到第一次成功读取的时间、以及是否需要断开 EEPROM 电源才能恢复。如果 `nd_board_iic_init` 单独不能恢复，把这一点记为现状差距，不要写成通过。

**证据。** 示波器截图和 CSV（含故障和恢复段）；20 次试验的返回码和恢复时间表；`DEVICE_ERR` 读数；`run.md`。

## 6. WFI 看门狗余量

**代码路径。** 看门狗在 `mcu_init` 里按 2 秒打开：`WDT_SetPeriod(WDT_2_SECS)` 和 `WDT_Enable()`（`infra/soc/v85xxp/v8500/v8500_for_single.c:438` 和 `:439`），周期常量在 `infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc/lib_wdt.h:22`。SysTick 按 `cpu_clk / 100` 编程（`v8500_for_single.c:52`），一次唤醒 10 ms（`targets/idis_ip_meter/board/board_transport.h:52`）。

裸机 IDIS 有两个 WFI 点：

- 传输等待：`nd_board_op_wait`（`targets/idis_ip_meter/board/board_transport.c:64` 起）每步先 `nd_board_watchdog_kick()`（`:71`），再 `__WFI()`（`:72`）。
- 主循环空闲：`targets/idis_ip_meter/app/main.c:573` 的 `wfi`。主循环只在秒变化时进入 `on_second`，喂狗在 `targets/idis_ip_meter/app/main.c:472`，所以两次喂狗之间最长约 1 秒。

FreeRTOS IDIS 没有 `board_transport.c` 的 WFI 分支（`board_transport.c:49` 走信号量）。`configUSE_TICKLESS_IDLE` 为 0（`targets/idis_ip_meter_freertos/app/FreeRTOSConfig.h:38`），端口层不执行 WFI，空闲任务空转。喂狗在每秒一次的 `on_second`（`targets/idis_ip_meter_freertos/app/main.c:483`）和存储操作里。

stategrid 的睡眠路径在 `targets/stategrid_meter/app/driver_entry.c:585` 和 `:797` 先调用 `WDT_Disable()`，再进入低功耗。正常唤醒路径不会再跑 `mcu_init`；`WDT_Enable` 只在 `v8500_for_single.c:439` 出现。测量要记录唤醒后的 `WDTEN`，不要假设看门狗在睡眠后仍然开着。

**测量点。** 在 `nd_board_watchdog_kick`（`board_transport_soc.c:10`）和每个直接调用 `mcu_clear_watchdog` 的关键点临时翻转标记 GPIO。逻辑分析仪按低采样率连续采集 24 小时。调试器在睡眠前后读 `WDT_GetCounterValue()`（`infra/soc/v85xxp/v8500/vendor/Lib_Driver/src/lib_wdt.c:83`）和 `PMU_WDTEN`。

**仪器。** 逻辑分析仪，1 通道，24 小时；调试器。

**构建模式。** IDIS 裸机 POLL、IDIS 裸机 IRQ-DMA、IDIS FreeRTOS 各一次。stategrid 单独一次，重点在睡眠前和唤醒后。

**判定。** 硬上界是 2 秒：任意两次喂狗之间的间隔必须小于 2 秒，且 24 小时内不得发生看门狗复位。余量等于 2 秒减去实测最大间隔。仓库没有余量预算，第一次测量记录最大间隔、间隔分布、以及最大间隔发生在哪个路径，作为基线。如果 stategrid 唤醒后 `WDTEN` 为 0，记录“看门狗在首次睡眠后关闭”，并说明余量只覆盖首次睡眠之前；这不能算通过。

**证据。** 24 小时逻辑分析仪文件或喂狗时间戳 CSV；复位原因日志；睡眠前后的 `WDTEN` 和计数器读数；`run.md`。

## 7. 结果归档

每项测量至少留下：

- 原始 trace 或波形文件。
- 导出的 CSV 和截图。
- 带构建定义和运行条件的 `run.md`。
- 第一次测量的基线值，写进对应产品的测量记录。

基线确定后，在后续固件提交上重跑同一清单。间隔变大或延迟变高时，先查代码和构建定义，再改基线。
