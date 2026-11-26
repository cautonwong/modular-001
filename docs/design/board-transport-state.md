# 板级传输状态所有权（IDIS）

2026-11-24。基线 main ea34a823。范围：两个 IDIS target 的板级传输状态，
每 target 一份，共 8 个符号：

- `targets/idis_ip_meter/board/board_transport_soc.c`：`uart_rx`、`uart_tx`、
  `spi_dma`、`spi_dma_ready`
- `targets/idis_ip_meter/board/uart_driver.c`：`uart_dev`、`uart_tx_fifo`、
  `uart_rx_fifo`
- `targets/idis_ip_meter/board/board_rn8209.c`：`uart_ready`

freertos target 是同一份文件的副本，行号和访问方式相同；差异只在定时
回调的运行上下文（见 1.2）。只读设计，不改产品 C、配置、测试或 mutable
基线。`board_ports.c` 的硬件缓冲（`uart_fifo` 等）已注明 board-owned，
不在本单元；`tick_timer_list` 和 stategrid 的同名副本也不在范围。

## 1. 状态与访问矩阵

### 1.1 逐符号

行号按 `idis_ip_meter`；freertos 副本相同。

| 符号 | 存储 | ISR 访问 | 任务访问 | 顺序/生命周期 | volatile | 引用 |
| --- | --- | --- | --- | --- | --- | --- |
| `uart_rx[6]` | `board_transport_soc.c` 静态数组 | 写：`uart_rx_isr` 收字节到 ring（head/buf） | 读：`nd_board_uart_irq_read`/`_rx_done`；写：pop 推进 tail | 仅 IRQ lane 编译；init 清零后注册 ISR，再使能接收 | 字段 `head`/`tail` 与 op 的 `done`/`status` 是 volatile，数组本身不是 | 声明 17；ISR 20-27（26）；init 44；read 97；status 109 |
| `uart_tx[6]` | 同上 | 写：`uart_tx_isr` 置完成 | 写：`nd_board_uart_irq_write` reset；读：`_tx_done` take | 同上；一次一字节，ISR 完成 | op 字段 volatile，数组本身不是 | 声明 18；ISR 30-35（34）；init 45；reset 80；take 89 |
| `spi_dma` | 同上，单实例 | 读：ISR 判断 `tx_active`/`rx_active`；调用 `nd_board_spi_dma_tx_done`/`_rx_done` | 无 | 仅 SPI DMA lane 编译；ISR 在首次 start 时注册 | 字段 volatile | 声明 119；ISR 156-172（163/168/170/171）；无其他读写的生产路径 |
| `spi_dma_ready` | 同上，单字节 | 无 | 读/写：`nd_board_spi_dma_hw_start` 首次注册 ISR 后置 1 | 0→1 一次，之后只读；注册完成后才使能 DMA 中断 | 否；单写者且在事件前完成 | 声明 120；读 179；写 186 |
| `uart_dev[7]` | `uart_driver.c` 静态数组 | 读写：`UartReceiveHandle`/`UartTransmissionHandle` 的 `rx_time_out`、`tx_time_out`、fifo 指针、回调 | 写：`uart_chl_initialize` 填字段；读：`uart_chl_open` 读 `chl_type`；timer 回调读写超时并收发 | init 写完全部字段后注册 ISR 和 timer；timer 的 `tickpara` 就是 `&uart_dev[index]` | **否**（字段均非 volatile） | 声明 41；ISR 52-89（61/62/73/75/77/83/85）；init 148-204（158-167、187-191、193-199）；timer 92-146（98-142）；open 208-243（216） |
| `uart_tx_fifo[7]` | 同上 | 无 | 无 | 仅声明，无任何引用 | 否 | 声明 42 |
| `uart_rx_fifo[7]` | 同上 | 无 | 无 | 仅声明，无任何引用 | 否 | 声明 43 |
| `uart_ready` | `board_rn8209.c` 静态 int | 无 | 读/写：`bus_write` 首次调用 `uart_prepare` 置 1，之后只读 | rn8209 总线任务路径；无 ISR | 否；单任务调用 | 声明 23；写 28；读 38 |

两个死数组不迁移、不保留：B2 直接删除。它们的 BSS 是
`2 × 7 × sizeof(struct fifo_t)`，按 4 字节对齐的 fifo_t（12 B）计算为
每 target 168 B。

### 1.2 上下文差异与已存在的共享

- `uart_rx`/`uart_tx`/`spi_dma`/`spi_dma_ready` 只在
  `ND_BOARD_UART_MODE == IRQ` / `ND_BOARD_SPI_MODE == DMA` 时编译。
  poll lane 没有这些静态，但 mutable 基线按源码计数，二者都要走批次。
- `uart_dev` 的 timer 回调 `uart_time_process` 在 bare 目标由 SysTick ISR
  调用（`ndv_timer_tick`），在 freertos 目标由 `app_task` 调用
  （`ndv_timer_tick` 在任务里）。同一份字段在两个产品里分别处于
  ISR 和任务上下文，这是本单元最重要的边界。
- `uart_dev` 的 ISR/任务共享字段当前**没有 volatile**，也没有临界区。
  U3-B 不改这个语义（见 5 残余），迁移只换存储位置。
- `spi_dma` 的 `tx_active`/`rx_active`/`busy` 在当前生产路径里从未被置位：
  `nd_board_spi_dma_hw_start` 只配置 DMA，不填 `spi_dma`。ISR 因此总是走
  “完成函数提前返回 + stop”的分支，组件层 `nd_board_spi_dma_t` 的完成
  等待会超时。这是既有行为；U3-B 必须原样保留，不在本单元修。
- `uart_chl_open` 当前没有调用者；保留接口语义，迁移时不改它的行为。

## 2. 所有权与聚合

### 2.1 方案 A（推荐）：board state 进 target 根上下文

每个 target 增加 board 私有的聚合类型（例如
`nd_idis_board_state_t`，放在该 target 的 `board/board_state.h`）：

- 成员：`uart_rx[6]`、`uart_tx[6]`、`spi_dma`、`spi_dma_ready`、
  `uart_dev[7]`、`uart_ready`（死数组删除）。
- 实例嵌进 target 根上下文：bare 的 `nd_idis_app_context_t`（main.c）和
  freertos 的 `nd_idis_app_context_t`（nd_idis_app_context.h）。不新增
  文件级可变全局；`g_idis_app` 仍是唯一根。
- 组合根在启动时调用一次 `board_bind(&root->board)`（或等价的 init），
  在 `board_uart_init`、任何 IRQ 使能之前完成。board 层各模块在 init 时
  收到指针，之后不再保存全局指针。
- ISR 拿上下文：`Isr_handle_regsit` 的 `para` 传**指向 state 内对应
  descriptor 的指针**，不再传下标。descriptor 内保留 `mcu_uart_index`
  等字段，ISR 单次 cast 后使用；注册在 ISR 使能之前完成，指针此后不变。
- timer：`tickpara` 直接传 `&state->uart_dev[index]`，与今天的
  `&uart_dev[index]` 等价；回调签名不变。
- rn8209：总线回调的 `ctx` 从 NULL 改为 `&state->rn8209`，`uart_ready`
  成为该对象字段；回调改用它，不再读文件级静态。

mutable 计数：三批全部完成后 196 → 180（16 条移除，0 条新增）。
可行性：board 层文件不包含 target 根头，只接收 `nd_idis_board_state_t *`；
类型定义在 board 自己的头里，target 组合根反过来 include 它，方向与现有
`board_ports.h` 一致。

### 2.2 方案 B（备选）：一个 board-owned 静态聚合

如果评审要求 board 层不依赖 target 根上下文，可把唯一实例放在
`board_ports.c`（已是 board-owned 状态文件）：

- `static nd_idis_board_state_t g_board;`，`board_bind()` 用它的地址；
  其余上下文传递方式同 A。
- mutable 计数：196 − 16 + 2 = 182。每 target 保留 1 个静态聚合，
  理由是 board 层与 target 根解耦；代价是两个 target 各多一条可审的
  board-owned 全局。
- 推荐 A：根上下文已经存在，board state 是产品状态的一部分，A 不新增
  静态聚合；B 只作为 board 层不能反向依赖 target 时的退路。

### 2.3 不新增的可变全局

- 不引入“指向上下文的静态指针”。需要上下文的地方由 init 参数或
  descriptor 指针携带。
- 不把 board state 拆成多个互相引用的静态对象；一个 target 一个聚合，
  避免第二份所有权。
- 本设计不改 `board_ports.c` 的 `uart_fifo`/`uart_fifo_buffer`，它们仍是
  board-owned 缓冲，通过指针挂在 `uart_dev` 上。

## 3. 迁移批次

每批都是独立 commit，可单独回滚；每批结束都要求完整门禁。

### B1：transport SoC 状态

- 符号：`uart_rx`、`uart_tx`、`spi_dma`、`spi_dma_ready`（4 × 2 = 8 条）。
- 文件（每 target）：`board_transport_soc.c`、`board_transport.c/h`
  （SoC hw 上下文参数、必要时给 `nd_board_spi_dma_t` 增加 hw 上下文）、
  `uart_driver.c` 的 `nd_board_uart_*` 包装、`board_ports.c` 的 bind、
  `board_state.h`、根上下文（main.c 或 nd_idis_app_context.h）。
- 迁移后 mutable：188。
- 验证：host:strict（board_transport 单测加 context 传递）；poll 构建 +
  acceptance；IRQ lane 构建 + `firmware-irq`/`irq-symbols`/`irq-rom`；
  mutable checker。

#### B1 实施记录（2026-11-24）

- 状态：已实现，按方案 A。四个符号进 `nd_idis_board_state_t`，嵌入两个
  IDIS target 的根上下文；`board_bind(&root.board)` 在启动时调用一次。
- UART ISR para 改为 `&state->uart[index]`（descriptor 含 index、rx、tx）；
  `board_rn8209_init(state)` 把 state 放进 rn8209 的 bus.ctx，总线回调把它
  传给 `nd_board_uart_*`。SPI 的 `nd_board_spi_dma_t` 增加 `hw_ctx`，
  `nd_board_spi_bind(state)` 把它存进 `esam_dma` 对象；hw start 用该 ctx，
  ISR para 是 `&state->spi_dma`。board 层没有新增全局指针。
- `spi_dma` 的 inert 完成路径原样保留；`uart_dev`/`uart_tx_fifo`/
  `uart_rx_fifo`/`uart_ready` 未动。
- 未绑定时 `nd_board_uart_irq_init/write/read/tx_done/rx_done` 与
  `nd_board_spi_dma_hw_start` 收到 NULL ctx 会失败；`tx_enable/rx_enable`
  不依赖 state，仍直接操作硬件。组合根保证 `board_bind` 在任何 UART/DMA
  使用之前调用一次。
- mutable：196 → 188（-8），无 exceptions。
- 实测（IRQ lane，同一工具链）：
  - ROM：bare 97,610 B（+112 B），freertos 102,099 B（+120 B）；
    103 KiB 余量 7,862 B / 3,373 B。
  - `.bss`：bare 37,694 B（+31 B），freertos 50,226 B（+31 B）。
  - poll lane 也构建通过；board 静态状态随根上下文搬家产生对齐增量。
    修正：`spi_dma_ready` 最初留在 `#if` 外，POLL 根结构多 8 B
    （600 对 592，root 对象 0x258 对 0x250）。后续把它移进 DMA 守卫；
    CI 实测（`check_idis_firmware.py`）：poll BSS bare 36,192→36,184
    （−8）、freertos 48,724→48,716（−8）；IRQ 37,728/50,260 不变；
    poll ROM +4 B（对齐），IRQ 不变。
- host 夹具：`tests/unit/test_idis_board_transport_context.c`，正面验证
  bound ctx 到达 hw start，反面验证未绑定 dma 不会继承别的 ctx，以及
  bind 先于 init 时 hw_ctx 保留。

### B2：uart_driver 状态

- 符号：`uart_dev` 迁移；`uart_tx_fifo`、`uart_rx_fifo` 删除（3 × 2 = 6 条）。
- 文件（每 target）：`uart_driver.c/h`、`board_state.h`、`board_ports.c`；
  descriptor 化 `uart_dev`，ISR para 改为 descriptor 指针，timer tickpara
  指向同一 descriptor。
- 迁移后 mutable：182。
- 验证：同 B1，加 uart 通道 init/timer 的 host 单测（可测部分）；
  IRQ lane；poll acceptance 行为不变。

#### B2 实施记录（2026-11-24）

- 状态：已实现。`uart_dev[7]` 进 `nd_idis_board_state_t`；ISR 和 timer 的
  `para/tickpara` 都是 `&state->uart_dev[index]`，descriptor 自带
  `mcu_uart_index`；`uart_chl_initialize/open` 增加 state 参数，
  board_ports 把根上下文的 board 传给 `board_uart_init`。
- 头文件循环：`uart_driver.h` 改为前向声明 `struct nd_idis_board_state`
  并去掉 board_state.h include；`board_state.h` include `uart_driver.h`
  取得 `struct uart_chl_init_t` 和 `MCU_UART_NUM`。两个头可独立编译。
- `uart_tx_fifo`/`uart_rx_fifo` 删除；删除前全 target grep 只有声明。
- `uart_chl_open` 的无用局部 `tm` 删除；接口/行为不变（该函数当前无
  调用者）。
- mutable：188 → 182（-6），无 exceptions。
- 实测（IRQ lane，同一工具链）：
  - ROM：bare 97,642 B（+32 B），freertos 102,127 B（+28 B）；
    103 KiB 余量 7,830 B / 3,345 B。
  - `.bss`：bare 37,698 B（+4 B），freertos 50,230 B（+4 B）。
  - “每 target −168 B”没有出现在最终 ELF：两个死数组本来就没有被引用，
    `-fdata-sections` + `--gc-sections` 已在链接时丢弃；删除只清理对象/
    源码，链接后 BSS 的 +4 B 来自 `uart_dev` 搬进根上下文的对齐。
- host 夹具：`tests/unit/test_idis_uart_descriptor.c`，覆盖 initialize 把
  descriptor 绑到 timer/ISR、RX ISR 用 descriptor、timer 收发用
  descriptor、open 扫描根状态、NULL state 失败。

### B3：rn8209 uart_ready

- 符号：`uart_ready`（1 × 2 = 2 条）。
- 文件（每 target）：`board_rn8209.c`、`board_state.h`、`board_ports.c`
  （总线 ctx 绑定）。
- 迁移后 mutable：180。
- 验证：测量/计量 acceptance、poll/IRQ 构建、mutable checker。

#### B3 实施记录（2026-11-24）

- 状态：已实现。`uart_ready` 成为 `nd_idis_board_state_t` 的字段；
  `uart_prepare` 置位，`bus_write` 读取，bus_read/bus_reset 增加
  `state == NULL` 的失败返回。rn8209 的 `storage`/`dev` 两个 static
  保留为残余，不在本批。
- mutable：182 → 180（-2），无 exceptions。
- 实测（IRQ lane，同一工具链）：
  - ROM：bare 97,658 B（+16 B），freertos 102,143 B（+16 B）；
    103 KiB 余量 7,814 B / 3,329 B。
  - `.bss`：bare 37,694 B（-4 B），freertos 50,226 B（-4 B）。
  - poll/IRQ 两 lane 均构建通过；两个 IDIS 验收 16/16。

批次顺序 B1 → B2 → B3：B1/B2 都动 `uart_driver.c` 的包装签名，先做 B1
把上下文入口定下来；B3 独立，可提前，但按文件风险放在最后。

### 3.1 host / IRQ lane 证据

每批需要的可执行证据：

- `host:strict`：board_transport 组件与可 host 化的 descriptor 逻辑；
  新行为（上下文传递）要有正向和负向夹具，不能只改签名。
- `host:sim-san` + IDIS host runner：两个 IDIS target 的 acceptance
  16 条保持 clean，证明 poll lane 业务行为不变。
- target 构建：poll 和 IRQ 两种 lane 都要过；`irq-symbols` 证明
  transport 符号仍在，`irq-rom` 提供实测 ROM。
- `check:mutable-globals`：每批的 baseline/frozen 差必须是该批删除的
  行，不出现“顺手”新增。

### 3.2 无 HIL 的证据边界

IRQ lane 只证明编译、链接、符号和 ROM；host 仿真没有 IRQ 注入模型，
acceptance 全在 poll lane。以下没有证据，必须留给 HIL：

- ISR 与任务在真实中断时序下的交错、优先级和临界区；
- UART 溢出、DMA 完成、双通道同时活动的行为；
- 低功耗唤醒与 ISR 的交互。

因此每批只能声明“结构迁移 + poll 行为不变 + IRQ lane 可链接可测
ROM”，不能声明 ISR 运行时已验证。

## 4. ROM / RAM 预算

当前基线（IRQ lane，ea34a823）：ROM 97,500 B（bare）/ 101,980 B
（freertos）；BSS 45,879 B / 50,195 B。103 KiB = 105,472 B，余量
7,972 B / 3,492 B。

预计增量（每 target，均为保守上限，实施时实测替换）：

| 批次 | ROM 预计 | ROM 最坏 | RAM/BSS | 说明 |
| --- | --- | --- | --- | --- |
| B1 | +64 B | +128 B | 0 | 上下文参数和 ISR descriptor 解引用 |
| B2 | +64 B | +128 B | −168 B | 删死数组；`uart_dev` 换位置不增减 |
| B3 | +16 B | +32 B | 0 | 单字段与 ctx 传递 |
| 合计 | +144 B | +288 B | −168 B | freertos 余量仍 ≥ 3,204 B |

注意：

- 上面的 RAM 净变化按“字段搬家”计算；如果 descriptor 需要额外索引字段，
  按每 target +4~8 B 预留，仍远低于余量。
- poll lane 的 ROM/BSS 与 IRQ lane 不同，每批用 `check_idis_firmware.py`
  的 poll/irq 两个 ELF 分别测，不以本表代替实测。
- 103 KiB 是 ROM 断言，不是估算目标；任何一批实测超过预计上限就停下
  重新 scope，而不是压缩属性或跳过 IRQ lane。

## 5. 失败模式、回滚与行为边界

失败模式：

- init 顺序错误：上下文未 bind 或字段未清零就注册 ISR / 使能中断。
  约束：bind 和字段初始化在 `board_uart_init` 内、任何 IRQ 使能前完成。
- ISR para 类型错误：把 index 和 descriptor 指针混用会读写错误地址。
  约束：每个 ISR 只有一个 cast 入口，descriptor 内含 `mcu_uart_index`；
  编译期类型检查，运行期不做额外分支。
- 并发语义被无意改变：新增锁、critical section 或 volatile 会改变时序。
  约束：三批不新增锁、不改字段类型、不改 IRQ 优先级。
- SPI DMA 的既有 inert 完成路径被“顺手修好”，导致行为变化。约束：
  保留 `spi_dma` 从未置位的语义，单独记录为残余。
- 死数组删除后仍有隐藏引用。约束：删除前做全 target 引用检查，
  构建后检查符号/链接，BSS 实测减少 168 B。

回滚：

- 每批一个 commit；回滚该 commit 即回到上一批状态，mutable
  baseline/frozen 的对应行随 commit 一起回退。
- 不涉及持久化格式、协议或数据迁移；回滚不需要清 EEPROM/flash。
- 批次之间没有数据依赖，B3 可独立先回滚。

不改 behavior 的边界：

- 同一批 buffer 指针（`board_ports.c` 的 `uart_fifo`）、同一组回调、
  同一 IRQ 编号、同一 poll/IRQ 编译选择。
- 不改变收发顺序、超时时长、回调时机和中断优先级；只改变状态的存储
  位置和取上下文的路径。
- poll lane 的 acceptance 必须逐条与迁移前一致；IRQ lane 的符号和
  ROM 断言保持通过。

## 6. 残余（不在 U3-B 修）

1. `uart_dev` 的 ISR/任务共享字段无 volatile、无临界区；bare 的 timer
   回调在 SysTick ISR、freertos 在任务，语义本身需要单独的并发单元。
2. `spi_dma` 的完成路径从未填入 active 标志，组件层 DMA 等待可能超时；
   需要单独确认是死代码、未启用功能还是缺陷。
3. `board_rn8209.c` 的 `storage`/`dev` 两个静态仍在；本单元只要求
   `uart_ready`，它们留在原地，计入 mutable。
4. `uart_chl_open` 无调用者；本单元保持接口，不删除。
5. stategrid 的同名 board 文件是另一份副本，不在两个 IDIS target 的
   范围内。
