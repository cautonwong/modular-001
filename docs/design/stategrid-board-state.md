# stategrid_meter 板级状态所有权

2026-11-25。基线 main e6e20bbf。范围：stategrid_meter（第三个电表 target）的
board-owned 可变状态、driver_entry 的 ISR 胶水，以及中断分发表。只做盘点与
设计，不改产品代码。模板沿用 `docs/design/board-storage-state.md`（IDIS 两
target），结论按 stategrid 自己的结构：context 从 `nd_memory` 资源 arena 分配，
不是静态根。

## 1. 盘点

`tools/mutable_globals_baseline.txt` 里本单元涉及的可变全局：

`targets/stategrid_meter/board/*` 共 24 条：

| 文件 | 符号 |
| --- | --- |
| `board_eeprom.c` | `DEVICE_ERR`、`error_streak`、`eeprom_storage`、`eeprom` |
| `board_esam.c` | `esam_storage`、`esam` |
| `board_flash.c` | `flash_storage`、`flash` |
| `board_rn8209.c` | `uart_ready`、`storage`、`dev` |
| `board_transport_soc.c` | `uart_rx[6]`、`uart_tx[6]`、`spi_dma`、`spi_dma_ready` |
| `spi_driver.c` | `esam_dma`、`esam_dma_ready`、`esam_tx_stage[256]`、`esam_rx_sink[256]`、`esam_tx_used` |
| `uart_driver.c` | `uart_dev[7]`、`uart_tx_fifo[7]`、`uart_rx_fifo[7]` |
| `cpu_v85xxp_isr.c` | `Isr_Function[IRQ_NUM]` |

`app/driver_entry.c` 的板级/ISR 胶水是 7 条（不是 8 条；`wplse_light_*` 一族是
4 个符号）：

- `fifo_buf[32]`、`irq_fifo`（21-22 行）
- `meter_cover_cnt`（23 行）
- `wpulse_light_flag`、`wpulse_light_open`、`wpulse_light_open_flag`、
  `wplse_light_width`（56-59 行）

清单外确认：`board/relay.c`、`board_io.c`、`*_pins.c`、`iic_driver.c` 只有
const 配置和函数；`relay.c` 的脉冲收回用 SOC 定时器，没有文件级状态。
`infra/soc/v85xxp/v8500/time_link.c` 的 `tick_timer_list` 是 SOC 级单例，属
SOC 所有权，不在本单元。LCD/display 的 app 全局（`display.c`、
`display_control_adapter.c`）和计量 legacy 全局（`electrical_measurement.c`、
`metering_energy_adapter.c` 等）是 app 状态，不在本单元；其中
`electrical_measurement.c` 的 `over_Imax` 被 TMR3 ISR 读取，列在残余风险。

## 2. 访问矩阵

行号按当前树。`ISR` 指真正在中断上下文执行；中断入口到回调的链路单独列出。
`SYSTICK` 表示回调由 SysTick 中断里的 `ndv_timer_tick` 调用（`time_link.c:130`
向 `IRQ_SYSTICK` 注册）。

| 符号 | 声明 | ISR / 任务 | volatile | 生命周期与顺序 | 引用 |
| --- | --- | --- | --- | --- | --- |
| `DEVICE_ERR` | board_eeprom.c:8 | 任务：`note_result` 11-24；消费者 `platform_adapter.c:32` | 否 | 首次 EEPROM 访问后持续；位 1 = 连续 5 次失败 | 8、15、22；board_eeprom.h:11 extern |
| `error_streak` | board_eeprom.c:9 | 任务：`note_result` 14、18-21 | 否 | 随每次总线结果增减 | 9、14、18-21 |
| `eeprom_storage` / `eeprom` | board_eeprom.c:57-58 | 任务：`board_eeprom` 懒初始化 72-79 | 否 | 首次访问一次；初始化时 `nd_board_iic_init` | 57-66 |
| `esam_storage` / `esam` | board_esam.c:57-58 | 任务：`board_esam` 懒初始化 70-76 | 否 | 首次 ESAM 调用一次 | 57-58、70-76 |
| `flash_storage` / `flash` | board_flash.c:35-36 | 任务：`board_flash` 懒初始化 51-58 | 否 | 首次 flash 访问一次 | 35-36、51-58 |
| `uart_ready` | board_rn8209.c:23 | 任务：`uart_prepare` 25-29、`bus_reset` 115-128 | 否 | 首次 rn8209 总线访问后置 1 | 23、28、41、119 |
| `storage` / `dev` | board_rn8209.c:120-121 | 任务：`board_rn8209` 133-139 | 否 | 首次计量读一次 | 120-121、133-139 |
| `uart_rx[6]` / `uart_tx[6]` | board_transport_soc.c:17-18 | UART IRQ：`uart_rx_isr` 20-27、`uart_tx_isr` 29-35（仅 `ND_BOARD_UART_MODE==IRQ`） | 内部 op 字段 | `nd_board_uart_irq_init` 37-62 初始化并注册；`para` 是通道下标 | 17-18、37-62、64-108 |
| `spi_dma` / `spi_dma_ready` | board_transport_soc.c:119-120 | DMA IRQ：`spi_dma_isr` 141-157（仅 `ND_BOARD_SPI_MODE==DMA`） | 内部 `tx_active/rx_active/busy` | 首次 `hw_start` 注册 ISR；每次传输复用 | 119-120、141-210 |
| `esam_dma`、`esam_dma_ready`、`esam_tx_stage[256]`、`esam_rx_sink[256]`、`esam_tx_used` | spi_driver.c:217-221 | 任务：`esam_flush` 223-244、`nd_board_spi_transfer` 305-338（仅 DMA 模式）；DMA ISR 操作的是 transport_soc 的 `spi_dma` | 否 | 首次 flush 懒初始化；任务串行暂存 | 217-244、305-338 |
| `uart_dev[7]` | uart_driver.c:41 | ISR：`UartReceiveHandle` 46-60、`UartTransmissionHandle` 62-84；SYSTICK：`uart_time_process` 92-146；任务：`uart_chl_initialize` 148-210 | `rx_time_out/tx_time_out` 不是 volatile | 初始化写一次，之后 ISR/timer 独占；回调 para 是 `&uart_dev[index]`（定时器）或下标（ISR） | 41、46-84、92-146、148-210、166-198 |
| `uart_tx_fifo[7]` / `uart_rx_fifo[7]` | uart_driver.c:42-43 | 无 | — | 从未被引用；`uart_chl_initialize` 用的是调用方传入的 fifo | 42-43（仅声明） |
| `Isr_Function[IRQ_NUM]` | cpu_v85xxp_isr.c:19 | 写：任务（各 init 经 `Isr_handle_regsit` 24-38）；读：全部 IRQHandler | 否 | 启动期注册为主；DMA ISR 在首次 SPI 传输时补注册；每项 12 B（fn+para+irq_index） | 19、112-116、163-181、218-223、271-280、288-299、301-506 |
| `fifo_buf` / `irq_fifo` | driver_entry.c:21-22 | 写：RTC ISR `RTC_Second` 51-54；PMU ISR `IO_Interrupt` 100、106；SYSTICK 定时回调 `rs485/plc/ir_receive_timeout` 179-192；读：任务 `irq_transform_to_instance` 321-334 | fifo 的 `rd`/`wr` 字段 volatile | boot `irq_system_init` 初始化；多生产者单消费者 | 21-22、51-54、100-106、179-192、303、327-334 |
| `meter_cover_cnt` | driver_entry.c:23 | 写：SYSTICK 定时回调 `IOState` 914-929；任务重置 `IOStateCheckInit` 961；读：任务 `IOStateCheck` 1031、1035 | 否 | 0..50 饱和计数 | 23、917-926、961、1031-1035 |
| `wpulse_light_open` | driver_entry.c:57 | PMU ISR 74；TMR3 ISR（`timing_ftr` 113-168）128-145 | 否 | PMU 沿置 0x5a，TMR3 消抖后清 | 57、74、128-145 |
| `wpulse_light_flag` / `wpulse_light_open_flag` / `wplse_light_width` | driver_entry.c:56、58-59 | TMR3 ISR 132-163 | 否 | TMR3 独占；两个 IRQ 同优先级不嵌套 | 56、58-59、132-163 |

### 2.1 真实中断入口

`cpu_v85xxp_isr.c` 的向量函数是唯一入口；它们查 `Isr_Function[irq]` 并核对
`irq_index` 后调用回调：

- SysTick：`SysTick_Handler` 112-131 → `ndv_timer_tick`（time_link.c:130 于
  `IRQ_SYSTICK` 注册）→ 定时回调 `uart_time_process`、`IOState`、
  `rs485/plc/ir_receive_timeout`、`shut_pluse_driver`（relay.c:10-17）。
  SysTick handler 还直接读写 `relay_off_chk`/`relay_on_chk`（app 全局）。
- RTC：`RTC_IRQHandler` 271-280 → `Isr_Function[IRQ_RTC]` → `RTC_Second`。
- PMU：`PMU_IRQHandler` 288-299 → `Isr_Function[IRQ_PMU]` → `IO_Interrupt`。
- TMR3：`TMR3_IRQHandler` 163-177 → `Isr_Function[IRQ_TMR3]` → `timing_ftr`。
  G 批最初仍直接调用 `timing_ftr(NULL)`，而 `timing_ftr` 已改为需要 state，
  会直接返回；后续修正恢复表分发，`check:isr-para` 阻止向量处理给回调传裸
  NULL。
- DMA：`DMA_IRQHandler` 218-223 → `Isr_Function[IRQ_DMA]` → `spi_dma_isr`。
- UART0..5：`UARTn_IRQHandler` 301-... 分别分发 TX/RX →
  `UartTransmissionHandle` / `UartReceiveHandle`（POLL 产品的 legacy 栈）或在
  UART-IRQ 模式下被 `uart_tx_isr`/`uart_rx_isr` 覆盖（同一 `Isr_Function`
  槽位，后注册者生效）。

## 3. 存活分析

构建方式：`python3 tools/check-firmware-link.py --product
meter_electricity_single_phase_cn --elf <repo 外临时路径>`。该工具用 IAR 工程
的源列表和 include/define，以 GCC `-Os -ffunction-sections -fdata-sections`
加 `-Wl,--gc-sections` 链接 `targets/stategrid_meter/firmware/gnu/link.ld`
（FLASH 256K、RAM 32K）。这是 link-check 镜像，不是 IAR 产品构建；IAR 本机
没有，host 侧另有 `firmware_sim`。证据边界以此为准。

最终 GNU 镜像（PA 配置，`ND_BOARD_*_MODE=0`）：

- ROM = `.vectors` 64 + `.text` 204,743 + `.ARM.exidx` 8 + `.data` 24 =
  **204,839 B**，256 KiB = 262,144，余 **57,305 B**。
- RAM = `.data` 24 + `.bss` 29,856 = **29,880 B**，32 KiB = 32,768，余
  **2,888 B** 给栈和堆（链接脚本不单独保留栈段，`_estack` 是 RAM 顶端）。

`arm-none-eabi-nm` 符号存活：

- 活：`DEVICE_ERR`(1)、`error_streak`(1)、`eeprom_storage`(64)、`eeprom`(4)、
  `esam_storage`(64)、`esam`(4)、`flash_storage`(64)、`flash`(4)、
  `uart_ready`(4)、`storage`(64)、`dev`(4)、`uart_dev`(0x134)、
  `Isr_Function`(0x120)、`fifo_buf`(32)、`irq_fifo`(12)、
  `meter_cover_cnt`(1)、四个 `wpulse/wplse` 各 1。
- 被剥掉：`uart_tx_fifo`、`uart_rx_fifo`（从未被引用，连 POLL 镜像里都被
  `--gc-sections` 删除，各 7×`struct fifo_t`=7×12=84 B）。
- 编译期不存在：`uart_rx`/`uart_tx`（UART-IRQ 模式）、`spi_dma`/
  `spi_dma_ready`（SPI-DMA 模式）、`esam_dma*`（SPI-DMA 模式）。产品配置是
  POLL；`tools/check-board-modes.sh` 只做其余模式的编译检查，不链接。

RAM 预算的工具化约束在 `targets/stategrid_meter/app/ram_budget.h`：

- context 在 `nd_memory` arena（`STG_RAM_ARENA_SIZE` 24 KiB）的第一段
  metadata block（2 KiB）里；`_Static_assert(STG_RAM_METADATA_USED <=
  STG_RAM_METADATA_SIZE)`。ARM 实测：`sizeof(stategrid_context_t)` 800，
  metadata used 1,420/2,048（余 628）；host（64 位）context 1,080，used
  1,968/2,048（余 80）。
- 四个 block（metadata 2,048 + cache 2,108 + 698 16,572 + mbase 3,072）合计
  23,800/24,576（ARM 余 776；host 合计 24,364，余 212）。
- board 聚合若按值进 context：ARM 656 B，host 856 B。ARM 自己的断言先过不去：
  metadata used 1,420−800+1,456=2,076>2,048（host 1,968−1,080+1,936=2,824）。
  要过断言得把 `STG_RAM_METADATA_SIZE` 升到 ≥2,832、`STG_RAM_ARENA_SIZE`
  升到 ≥25,360，nd_memory 增 784 B。见 4.1。

## 4. 所有权

### 4.1 board 聚合的落点

```c
typedef struct {
    nd_eeprom_storage_t eeprom_storage;
    nd_eeprom_t *eeprom;
    nd_esam_storage_t esam_storage;
    nd_esam_t *esam;
    nd_mx25l3206e_storage_t flash_storage;
    nd_mx25l3206e_t *flash;
    nd_rn8209_storage_t rn8209_storage;
    nd_rn8209_t *rn8209;
    int uart_ready;
    uint8_t DEVICE_ERR;
    uint8_t error_streak;
    struct uart_chl_init_t uart_dev[7];
    /* driver_entry 的 ISR 胶水，G 批迁入 */
    struct fifo_t irq_fifo;
    uint8_t irq_fifo_buf[32];
    uint8_t meter_cover_cnt;
    uint8_t wpulse_light_flag;
    uint8_t wpulse_light_open;
    uint8_t wpulse_light_open_flag;
    uint8_t wplse_light_width;
#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ
    nd_board_uart_rx_t uart_rx[ND_BOARD_UART_CHANNELS];
    nd_board_op_t uart_tx[ND_BOARD_UART_CHANNELS];
#endif
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    nd_board_spi_dma_t spi_dma;
    uint8_t spi_dma_ready;
    nd_board_spi_dma_t esam_dma;
    uint8_t esam_dma_ready;
    uint8_t esam_tx_stage[ND_BOARD_SPI_DMA_CHUNK];
    uint8_t esam_rx_sink[ND_BOARD_SPI_DMA_CHUNK];
    uint16_t esam_tx_used;
#endif
} stategrid_board_state_t;
```

聚合按批增字段（见第 5 节），POLL 配置下最终 ARM 656 B / host 856 B
（UART-IRQ/SPI-DMA 字段在 `#if` 里，按模式另计）。按值整体放进
arena 的 metadata block 会撞上 2 KiB 静态断言，并把共用常数推到吃掉栈余量的
位置，因此**推荐 Option B**：

- 根组合（`g` 级）：`stategrid_meter_entry.c` 里一个
  `static stategrid_board_state_t stategrid_board;`，boot 时
  `stategrid_board_init(&stategrid_board)`，在
  `stategrid_context_create` 之前。
- `stategrid_context_t` 增加一个指针成员
  `stategrid_board_state_t *board;`（host metadata 余 80−8=72 B，ARM 余
  628−4=624 B，两个断言都不动）。
- 所有 board 访问器收 `stategrid_board_state_t *`，组合根和 instance 侧传
  `ctx->board`。
- RAM 净变化≈0：每批只把该批静态搬进聚合，例如 E 的 70 B 静态换成 70 B
  字段、T 的 `uart_dev` 308 B 换成 308 B 字段；最终 656 B 聚合替换原有
  635 B 活静态（4 个 64 B storage + 4 个指针 + 2 个错误字节 + `uart_ready`
  4 + `uart_dev` 308 + driver_entry 49 = 635），加对齐后净 +21 B；context
  里的 4 B/8 B 指针落在已有 metadata 余量内（ARM 1,424/2,048、host
  1,976/2,048），不新增 block。栈余量约 2,867 B（2,888−21）。

Option A（按评审意见把聚合按值嵌入 `stategrid_context_t`）不可直接落地：
ARM context 800+656=1,456，metadata used 2,076>2,048，先撞静态断言；host
context 1,080+856=1,936，metadata used 2,824。要过断言必须把
`STG_RAM_METADATA_SIZE` 升到 2,832、`STG_RAM_ARENA_SIZE` 升到 25,360
（host 四块和 25,148 ≤ 25,360；ARM 四块和 24,584），nd_memory +784 B；
被搬走的 635 B 静态同时离开 .bss，净 BSS +149（784−635），栈余量约
2,739 B（若只按 arena 增长粗算是 2,104 B），`ram_budget.h` 三条断言同时改。
设计把 A 列为备选，默认按 B 拆批次；如需 A，先单独改预算常数并复测。

### 4.2 `DEVICE_ERR` 的消费者边界

`platform_adapter.c` 的 `platform_device_error(ctx)` 是 application
`nd_platform_port_t` 的实现，ctx 是 `stategrid_context_t *`。删除
`board_eeprom.h:11` 的 `extern uint8_t DEVICE_ERR`，改为 board 访问器：

```c
uint8_t nd_board_device_error(const stategrid_board_state_t *state);
```

`platform_device_error` 用 `instance->board` 取位 1；`note_result` 改为
`state->DEVICE_ERR`/`state->error_streak`。平台端口本身不变，consumer 仍是
application 端口，适配器只换取值来源。sim 的 `sim_stubs.c:182`
`uint8_t DEVICE_ERR = 0;` 删除或替换为访问器。

### 4.3 ISR para 与 `Isr_Function` 表

- board 回调的 para 改成指向 state 内字段（如 `&state->key_event`），DMA/UART
  的 para 指向 state 内的通道项（`&state->uart_irq[index]` 形状），ISR 里
  `para` 不再编码下标。UART-IRQ 模式下 `uart_rx_isr`/`uart_tx_isr` 的 para 从
  `(void*)index` 改为指向 `state->uart_rx[index]` 的包装项，避免"下标 + 全局
  state"。
- `Isr_Function` 留在 `cpu_v85xxp_isr.c`：Cortex-M0 的向量函数没有参数，
  分发表是硬件入口的一部分，不能经参数注入 state。它保持可变（注册期写），
  归 board ISR 层所有；每一项的 `para` 指向 board state。若以后要移走，需要
  向量函数改成 `syscall`/固定地址表，超出本单元。mutable 冻结条目不增长，
  不新增例外。

## 5. 分批

每批一个 commit，独立回滚；每批跑 `./ci.sh`、`check:board-modes`、验收矩阵
（`tests/acceptance` 的 `firmware_sim` + `firmware_acceptance`）和
`check:doc-evidence`。mutable 146 → 117：板级 23 条 + driver_entry 7 条 = 30
条移除，Option B 新增 1 条组合根聚合 `static stategrid_board_state_t
stategrid_board;`（类比 `g_idis_app`，mutable 检查器可见，frozen 加一行），
净 −29；`Isr_Function` 保留不动。

`stategrid_board_state_t` **按批增字段**，不是 E 批一次放整个聚合：E 批加
EEPROM 的 4 个字段（64 B storage + 4 B 指针 + 2 B 错误计数 = 70 B），F 加 flash（68 B），S 加 esam（68 B），SPI 加
7 个（全在 `#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA` 里，POLL 下 0 B），
T 加 `uart_dev`（308 B）并删两个死 fifo，RN 加 rn8209 的 3 个（72 B），G 加
driver_entry 的 7 个（49 B）；对齐后最终 656 B（ARM）/856 B（host）。每批
BSS 预期：静态换成等大字段，净 0（加聚合内对齐 ≤8 B）；E 批若一次放整个
聚合会先 +588 B，直接击穿 ±8 与栈余量，禁止。

| 批次 | 符号 | mutable 后 | 主要文件 | 验证 |
| --- | --- | --- | --- | --- |
| E：EEPROM/DEVICE_ERR | `DEVICE_ERR`、`error_streak`、`eeprom_storage`、`eeprom` | 143 | board_eeprom.c/h、board_state.h、storage_device.c/h、platform_adapter.c、resource_tool.c、stategrid_meter_entry.c、driver_entry.c | 存储读写的 host/仿真路径、验收、mutable |
| F：FLASH | `flash_storage`、`flash` | 141 | board_flash.c/h、board_state.h、storage_device.c、driver_entry.c | 同上 |
| S：ESAM | `esam_storage`、`esam` | 139 | board_esam.c/h、board_state.h、security/esam_pro.c | 698 ESAM 用例、验收 |
| SPI：ESAM DMA | `esam_dma`、`esam_dma_ready`、`esam_tx_stage`、`esam_rx_sink`、`esam_tx_used`、`spi_dma`、`spi_dma_ready` | 132 | spi_driver.c/h、board_transport_soc.c、board_transport.h、board_state.h、board_flash.c、board_esam.c | SPI-DMA 模式编译、UART-IRQ 模式编译、POLL 链接、验收 |
| T：transport/UART | `uart_dev`、`uart_rx`、`uart_tx`；删除死代码 `uart_tx_fifo`、`uart_rx_fifo` | 127 | uart_driver.c/h、board_transport_soc.c、board_state.h、driver_entry.c、stategrid_meter_entry.c | UART-IRQ 模式编译、POLL 链接、UART 验收 |
| RN：rn8209 | `uart_ready`、`storage`、`dev` | 124 | board_rn8209.c/h、board_state.h、metering/electrical_measurement.c、rn8209 单测 | 计量验收、poll/IRQ 构建 |
| G：driver_entry/ISR 胶水 | `irq_fifo_buf`、`irq_fifo`、`meter_cover_cnt`、`wpulse_light_flag`、`wpulse_light_open`、`wpulse_light_open_flag`、`wplse_light_width` | 117 | driver_entry.c/h、uart_driver.c/h、board_state.h、stategrid_meter_entry.c、tests/sim/sim_stubs.c | PMU/RTC/定时器路径、验收、mutable |

排序 E → F → S → SPI → T → RN → G：store 端口先闭合；SPI 先把 hw 层签名改
成带 state（POLL 产品不受影响，DMA 模式保持 inert）；transport 批删死 fifo；
G 最后处理 ISR para 和 SysTick 定时回调。

每批签名与调用点（行号按当前树，freertos 无关，只一个 target）：

- **E**：`board_eeprom(state)`；`board_eeprom_init(state)` 不需要单独函数
  （保持懒初始化）。调用点：`storage_device.c:11、23`（见下）、
  `stategrid_meter_entry.c:747`、`driver_entry.c:404`。
  `nd_storage_read/write/erase/size` 增加 `stategrid_context_t *instance`
  参数（消费者是 resource tool）：`resource_tool.c` 32 处、`metering_energy_adapter.c:53`
  （ctx 现在把 device 名塞进 `uintptr`，改为 `struct { ctx; device; }`）、sim
  `sim_devices.c:71-110`、`test_mbase_search_counts.c:44-77`、
  `metering_energy_harness.h:173`。
  E 可拆两步落地：E0 只给 `nd_storage_read/write/erase/size` 加
  `stategrid_context_t *instance` 并改上述 32 处、适配器 ctx 和 stub（纯签名
  线程，行为与 mutable 都不变）；E1 再搬 EEPROM 状态和 `board_eeprom(state)`。
  不拆也能一次完成，但 diff 更大。E0 实测：只改调用约定，BSS 不变；ROM
  一次性 +312 B（legacy link 镜像 204,839→205,151；产品闭包 208,472→208,784）
  已由评审接受。E1 的预算基线从 E0 提交之后起算。
- **F**：`board_flash(state)`；调用点 `storage_device.c:14、26、34`、
  `driver_entry.c:408`。
- **S**：`board_esam(state)`。`esam_pro_handle` 的原分发把 `esam_hal` 写死
  传 0，本批改为传 `stategrid_context_board(instance)`，handler 从 esam_hal
  取回 board 状态后再调 `board_esam`；调用点 `security/esam_pro.c:76、102、
  172、177`；sim `sim_stubs.c:197`。`stategrid_context_board(void *)` 的声明
  放在 `board_state.h`、实现放 `stategrid_context.c`：esam_pro.c 直接 include
  `stategrid_context.h` 会拉进 `fr_698.h` 的 `enum sec`，与其本地同名 enum
  在 GNU link 下冲突。这不是层间倒置——访问器只做 void* → board 指针转换。
- **SPI**：`nd_board_spi_flush/select/transfer(state, ...)`；
  `nd_board_spi_dma_hw_start/stop/wait_idle(state, ...)`（transport_soc 的
  `spi_dma` 进 state）；调用点 `board_flash.c` 的 bus 回调、`board_esam.c`
  的四个 bus 回调、`board_transport.c` 的 DMA 包装；NULL state 的返回语义
  与 IDIS SPI 批一致。
- **T**：所有 `nd_board_uart_*` 加 state 首参（POLL 分支忽略 state）；
  `uart_chl_initialize(state, chl)`、`uart_chl_open(state, type)`；legacy
  UART ISR 的 para 是 `&state->uart_dev[index]`，`UartReceiveHandle/
  TransmissionHandle` 和 `uart_time_process` 从该项取；UART-IRQ 传输的 ISR
  para 是每通道 `{state,index}` 记录（不是裸下标，也不是文件级指针），数组
  在 `#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ` 下。`board_rn8209` 在本批
  提前取 `board_rn8209(stategrid_board_state_t *state)` 签名（config 局部
  拷贝、ctx=state），storage/dev/uart_ready 仍静态，RN 批只搬字段、不再改
  签名。调用点：`driver_entry.c:296、441、615、657-658`、
  `stategrid_meter_entry.c:241`、`electrical_measurement.c`（18 个已带
  instance 的函数把 instance 传给 4 个静态 helper）、`board_rn8209.c:27`。
  删除 `uart_tx_fifo`/`uart_rx_fifo` 时同步 `tools/dead_code_baseline.txt`
  （若有条目）和 mutable 文件。
- **RN**：`board_rn8209(state)`；调用点 `electrical_measurement.c:60-204`
  （legacy 文件，函数已带 instance 或可沿调用链传）、`test_rn8209c_get.c:239`
  stub、`sim_devices.c:198`。
- **G**：`void board_irq_bind(stategrid_board_state_t *state)` 替代
  `irq_system_init(void)`；`RTC_Second/IO_Interrupt/timing_ftr` 的 para 就是
  board state，中断里直接写 state 字段；`uart_chl_init_t` 的
  `CallBackSendPara` 更名 `CallBackPara`，接收超时回调与发送回调共用同一个
  instance para（`rs485/plc/ir_receive_timeout` 经 `stategrid_context_board`
  取 state）；`IOState` 的 `tm.tickpara` 是 state；`irq_transform_to_instance`
  仍从 instance 取 `board`，队列存储 `irq_fifo`/`irq_fifo_buf` 在 state 里；
  `stategrid_board_init` 对整个聚合 memset，覆盖全部字段（含条件字段），
  不再逐字段列举。`Isr_Function` 留在 `cpu_v85xxp_isr.c`，是硬件分发表，
  不搬。G 表里的 7 个文件级静态全部消失，mutable 终值 117。

每批预算与停止线（以 `check-firmware-link.py` 的 GNU ELF 为准，POLL 产品）：

- ROM：E0 实测 +312 B，E1 实测 +172 B（产品闭包 +176 B），F（FLASH 状态）
  实测 +32 B（产品闭包 +32 B），S（ESAM 状态）实测 +108 B（产品闭包 +104 B），
  SPI（ESAM DMA/暂存）实测 +36 B（BSS 0，POLL 字段编译期不存在），T
  （transport/UART）实测 +288 B（产品闭包 +288 B，BSS −8），RN（rn8209
  状态）实测 +0 B（`.text`/`.data`/`.bss` 与 T 逐 section 一致），G
  （driver_entry/ISR 胶水）实测 +52 B（BSS +8，49 B 静态换成 56 B 字段，
  余差是对齐）。超过单批上限的批次均由评审接受、不再压缩。最终累计
  312 + 176 + 32 + 108 + 36 + 288 + 0 + 52 = 1,004 B。legacy link 镜像
  余量 57,305 → 56,305 B（G 后实测）。预算基线从 G 提交之后起算。
- BSS：每批 ±8 B（对象搬家净 0，只有对齐）；合计 ≤ +32 B。当前余 2,888 B
  （含栈）。
- 任何一批让 `ram_budget.h` 的 metadata/arena 断言需要改常数，或让死对象进
  live state，立即停，重新 scope。
- 回滚：revert 该批 commit；不涉及持久化格式，不需清 EEPROM/flash。

## 6. 风险与残余

- **栈余量 2,888 B**：32 KiB RAM 的硬约束；每批 BSS 停止线按 ±8 B，任何
  arena/metadata 常数改动都要先复测栈余量。Option A 的净代价约 +149 B BSS
  （栈余量 2,739 B），另需改三条预算断言。
- **死代码实体化**：`uart_tx_fifo`/`uart_rx_fifo` 从未被引用、当前被
  `--gc-sections` 剥掉；T 批删除而不是搬进 state（搬进去是 168 B RAM）。
  UART-IRQ/SPI-DMA 字段必须放在与原 `#if` 相同的条件下，POLL 镜像不涨。
- **ISR/task 共享**：`irq_fifo` 有 RTC、PMU、SysTick 三个 ISR 生产者，任务侧
  单消费者；`uart_dev` 被 UART ISR、SysTick 定时器和初始化任务触碰；
  `meter_cover_cnt` 在 SysTick 回调里写、任务里读。迁移只换存储位置，不加锁、
  不改 volatile 语义；真机交错仍要 HIL。
- **双重 UART 栈**：legacy `uart_chl_initialize` 与 `nd_board_uart_irq_init`
  注册同一组 UART IRQ 槽位，后注册者生效。POLL 产品只有第一套；UART-IRQ 模式
  没有产品配置，T 批保留现状并在设计里标注，不顺带合并两套栈。
- **TMR3/PMU 表分发（已修）**：G 批把 `timing_ftr`/`IO_Interrupt` 改成接收
  state，但向量处理仍传 NULL，两个中断入口会直接返回。后续修正让
  `TMR3_IRQHandler` 和 `PMU_IRQHandler` 走 `Isr_Function` 表分发，注册的
  para 就是 board state；`check:isr-para` 阻止向量处理给回调传裸 NULL。
  真机时序仍要 HIL。
- **app 全局的 ISR 触点**：`relay_off_chk`/`relay_on_chk`（SysTick handler
  直接读写，app 全局）、`over_Imax`（TMR3 ISR 读）、`ec_pluse_cnt`
  （`Energy_Pluse_Add` 在 TMR3 ISR 里写）。不在本单元，列作后续 app 状态
  盘点输入。
- **验收与证据边界**：stategrid 的验收是 `firmware_sim` + `firmware_acceptance`
  （host，board/SOC 源被 stubs 替换，ISR 不执行），加 GNU link-check 镜像的
  ROM/BSS/符号。没有 IAR 构建和 HIL；中断时序、真实 EEPROM/flash、掉电安全
  仍需 HIL。`check:board-modes` 只编译不链接。
- **单实例约束**：`stategrid_meter_entry.c` 和 sim 都只创建一个 instance；
  Option B 的静态 board 聚合继承了这一点（与 IDIS 的 `g_idis_app.board`
  相同），多实例不是当前产品承诺。
- **LCD/display 与计量 legacy 全局**不在本单元；`display.c`、
  `electrical_measurement.c`、`metering_energy_adapter.c` 等 45 条 app 状态
  另行盘点。
