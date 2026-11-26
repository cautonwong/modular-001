# 板级存储与设备状态所有权（IDIS，剩余批）

2026-11-24。基线 main 805e4e10。范围：两个 IDIS target 各一份的剩余
board-owned 状态，共 20 个符号：

- `app/board_ports.c`：`flash_sector`、`flash_store`、`uart_fifo`、
  `uart_fifo_buffer`、`key_event`
- `board/board_eeprom.c`：`DEVICE_ERR`、`error_streak`、`eeprom_storage`、
  `eeprom`
- `board/board_esam.c`：`esam_storage`、`esam`
- `board/board_flash.c`：`flash_storage`、`flash`
- `board/board_rn8209.c`：`storage`、`dev`（B3 残余）
- `board/spi_driver.c`：`esam_dma`、`esam_dma_ready`、`esam_tx_stage`、
  `esam_rx_sink`、`esam_tx_used`

审计没有发现此清单之外的 board-owned 可变状态：board 目录其余静态都是
`const` 配置/表；`baremetal.c` 的 `systick_ticks` 是 app 层状态，不在本
设计范围；board_ports 当前没有 `rtc_seconds`。`DEVICE_ERR` 是清单里唯一
外部链接的全局（`board_eeprom.h` 有 `extern`），其余是文件级 static。

B1/B2/B3 已把 transport SoC、`uart_dev` 和 `uart_ready` 放进
`nd_idis_board_state_t`；本设计沿用同一根上下文，不改变已完成的边界。

## 1. 访问矩阵

行号按 `idis_ip_meter`；freertos 副本同构。`ISR` 指真正在中断里执行的
访问；任务列包含启动、存储回调和后台任务。

| 符号 | ISR | 任务 | 顺序/生命周期 | volatile | 引用 |
| --- | --- | --- | --- | --- | --- |
| `flash_sector[4096]` | 无 | `nd_sector_store_init` 把它作为扇区工作区（初始化 198-200）；sector store 的 read/write/erase 回调在任务里通过它读写 | 初始化时绑定一次，之后由 sector store 独占 | 否（容器） | 声明 62；使用 200 |
| `flash_store` | 无 | `board_flash_port` 返回其 port（186）；`board_flash_init` 初始化（198） | 启动时初始化一次 | 否 | 声明 171；186、198 |
| `uart_fifo[3]` | 读写：UART RX/TX ISR 通过 `uart_dev->rx_fifo/tx_fifo` 指针访问 | 初始化 230-232；`board_ir_read` 读红外 FIFO（242-245）；timer 回调经同一指针 | 缓冲地址在 `uart_chl_initialize` 前绑定；ISR 收到 descriptor 后只经指针访问 | fifo 的 `rd`/`wr` 字段 volatile，容器不是 | 声明 65；217-218、230-232、242-245；ISR 见 `uart_driver.c` 61/75/81 |
| `uart_fifo_buffer[3][128]` | 同上，经 fifo 的 `buf_ptr` 间接访问 | `fifoBuf_init` 绑定（230-232） | 静态存储，地址不变 | 否 | 声明 66；230-232 |
| `key_event` | 写：`board_pmu_isr` 251-258（PARA 当前为 NULL，注册 298-303） | 读/清：`board_key_event_take` 260-265；main 的按键处理 | PMU 中断注册后可能随时写；任务读取并清零 | **是** | 249、256、262-263 |
| `DEVICE_ERR` | 无 | `note_result` 15/22；唯一读写文件是 board_eeprom.c，头文件只声明 extern | 第一次 EEPROM 访问后持续存在 | 否 | 8、15、22；board_eeprom.h:9 |
| `error_streak` | 无 | `note_result` 14、18-21 | 随每次总线结果增减；到阈值置 DEVICE_ERR | 否 | 9、14、18-21 |
| `eeprom_storage` | 无 | `board_eeprom` 懒初始化（74-78） | 首次访问 EEPROM 时初始化一次 | 否 | 57、76 |
| `eeprom` | 无 | 懒初始化指针（74-78） | 首次访问后非空 | 否 | 58、74-78 |
| `esam_storage` | 无 | `board_esam` 懒初始化（72-75） | 首次访问 ESAM 时一次 | 否 | 57、73 |
| `esam` | 无 | 懒初始化指针（72-75） | 首次访问后非空；当前无调用者 | 否 | 58、72-75 |
| `flash_storage` | 无 | `board_flash` 懒初始化（50-55） | 首次访问 flash 时一次 | 否 | 35、53 |
| `flash` | 无 | 懒初始化指针（50-55） | 首次访问后非空 | 否 | 36、50-55 |
| `storage`（rn8209） | 无 | `board_rn8209_init` 初始化（146） | bind 时初始化一次（B1 起） | 否 | 130、146 |
| `dev`（rn8209） | 无 | `board_rn8209_init` 写（146）；`board_rn8209` 读（151） | bind 后非空 | 否 | 131、146、151 |
| `esam_dma` | 间接：DMA ISR 用的是 board state 的 `spi_dma`（B1），不是这个对象；其 op/sem 由传输路径使用 | `esam_flush` 初始化/传输（234-238）；bind 写 `hw_ctx`（344） | 首次 flush 懒初始化；bind 先写 hw_ctx | 内部 `tx_active/rx_active/busy` 字段 volatile | 218、234-238、344 |
| `esam_dma_ready` | 无 | 首次 flush 置 1（233-235） | 0→1 一次 | 否 | 219、233、235 |
| `esam_tx_stage[256]` | 无 | 填 stage（310）、传输（237） | 任务串行使用；flush 前清 used | 否 | 220、237、310 |
| `esam_rx_sink[256]` | 无 | 传输接收缓冲（238） | 任务串行使用 | 否 | 221、238 |
| `esam_tx_used` | 无 | 226、232、310-314 | 0..chunk，flush 清零 | 否 | 222、226、232、310-314 |

共享点：

- `uart_fifo`/`uart_fifo_buffer` 是 UART ISR 与任务共享的环形缓冲；
  ISR 通过 `uart_dev` 里的指针访问，descriptor 已在 B2 固定，移动缓冲
  本身不会改变同步语义。
- `key_event` 是唯一带 `volatile` 的 ISR/task 标志；迁移后字段必须保持
  `volatile`，ISR 仍通过注册的 para 获得地址。
- `DEVICE_ERR` 虽然名字像宏，实际是全局变量；这是清单里唯一需要删除
  `extern` 的符号。

## 2. 所有权与聚合

### 2.1 推荐拆分

进 `nd_idis_board_state_t`（每 target）的 17 个符号：

- 存储对象：`flash_store`、`eeprom_storage`、`eeprom`、`flash_storage`、
  `flash`、`esam_storage`、`esam`、rn8209 的 `storage`/`dev`
- 错误/事件：`DEVICE_ERR`、`error_streak`、`key_event`
- SPI ESAM 暂存：`esam_dma`、`esam_dma_ready`、`esam_tx_stage`、
  `esam_rx_sink`、`esam_tx_used`

留在 `board_ports.c` 的 3 个硬件缓冲：

- `flash_sector[4096]`：sector store 的擦除/写工作区，属于 flash 端口
  的板级内存。
- `uart_fifo[3]`、`uart_fifo_buffer[3][128]`：UART 硬件 FIFO，地址通过
  `uart_dev` 指针交给 ISR；main.c 已注明 board-owned。

这三个不放进 root 的理由：它们是 DMA/ISR 直接寻址的板级缓冲，生命周期
与 board 端口绑定，不参与产品状态；移动只增加 root 体积，不减少指针
解引用。为了让“board_ports 只剩一个所有者”，把三者合并成一个
board-owned 聚合：

```c
static struct {
    uint8_t flash_sector[BOARD_FLASH_SECTOR];
    struct fifo_t uart_fifo[3];
    uint8_t uart_fifo_buffer[3][BOARD_UART_FIFO_BYTES];
} board_buffers;
```

这样 mutable 计数从每 target 3 条降为 1 条。

### 2.2 `const board_store` 与签名/调用点

`board_store` 是 `static const nd_store_t`（board_ports.c:173），`.ctx`
不能在运行期写。**E 批把 `board_store` 移进
`nd_idis_board_state_t`**，作为非 const 的 `nd_store_t store;`：

- `board_bind(&g_idis_app.board)` 一次填 `.ctx = state`、
  `.read = board_store_read`、`.write = board_store_write`。
- `board_store_port(state)` 返回 `&state->store`。
- `board_store_read/write` 的 `ctx` 是 state，内部调用
  `board_eeprom(state)`。
- 不保留 const 模板再拷贝：模板加一份运行期副本只会多一个同步点，
  而且 `board_store` 本来就不是 baseline 计数对象，搬进 root 不改变
  mutable 数。

各批签名与全部调用点（行号按 bare；freertos 的 main.c 行号略偏，同构）：

**E：EEPROM**

- `nd_store_t *board_store_port(nd_idis_board_state_t *state)`
  - board_ports.c:92（`board_calibration_load`）
  - main.c:226、279、324、442、586、693
- `void board_store_init(nd_idis_board_state_t *state)`
  - main.c:714
- `nd_eeprom_t *board_eeprom(nd_idis_board_state_t *state)`
- `board_measurement_init(nd_idis_board_state_t *state,
  board_measurement_calibration_t *calibration)` 也随 E 批改：它调用的
  `board_calibration_load` 需要把 state 传给 `board_store_port`。
  main.c:717 的调用改传 `&g_idis_app.board`。RN 批只再加 measurement
  read 的 ctx 和 rn8209 用法，不重复改这个签名。
- `board_calibration_load` 增加 state 参数，由 measurement init 传入；
  `board_store_init` 内把 store 端口绑到 state。

**F：FLASH**

- `nd_store_t *board_flash_port(nd_idis_board_state_t *state)`
  - main.c:426、444
- `void board_flash_init(nd_idis_board_state_t *state)`
  - main.c:715
- `nd_mx25l3206e_t *board_flash(nd_idis_board_state_t *state)`
- `nd_sector_store_init(&state->flash_store, state, ...)`；
  `flash_read/erase/write` 的 ctx 从 NULL 改为 state。
- `flash_store` 随本批进 state（原先放在 BP 会迫使 flash 端口在 F 批
  仍指向静态对象，分批不闭合）。

**S：ESAM（延后，未执行）**

- 目标接口 `nd_esam_t *board_esam(nd_idis_board_state_t *state)`；当前无
  调用者，且 `--gc-sections` 把 `board_esam.o` 整体剥掉：最终
  `idis.elf`/`idis_freertos.elf` 里没有 `board_esam`、`esam`、
  `esam_storage` 符号（`arm-none-eabi-nm` 复核；只剩 SPI 批的
  `esam_dma*`）。
- 把 `nd_esam_storage_t`（64 B union）和 4 B 指针移进 live root 会净增
  72 B BSS（四 lane 实测 +72：bare 36,173→36,245、IRQ 37,712→37,784；
  freertos 48,705→48,777、IRQ 50,244→50,316），等于把不在镜像里的功能
  实体化。RAM 余量 4,980 B 够，但不为一个无调用者的功能付。
- 触发条件：`board_esam` 第一个调用者出现时再迁，届时预算 +72 B BSS；
  实测数字见上，S 批延后记录在提交说明与验收证据中。
- SPI 批已提前把签名改成 `board_esam(state)`：四个 bus 回调要把 state
  转给 SPI API，ctx 仍为 NULL 会让 ESAM 静默不可用。S 的延后范围因此只剩
  搬 `esam_storage`/`esam` 进 root 并删 mutable 行，触发条件不变。

**SPI：ESAM 暂存**

- `nd_board_spi_bind(state)` 已存在，改为初始化 state 里的
  `esam_dma.hw_ctx = state`（DMA 模式）；`esam_dma`/`esam_dma_ready`/
  暂存区随首次 `esam_flush(state)` 懒初始化，`spi_driver.c` 不再有静态
  对象。
- API 增加 state 参数（NULL state：transfer 返回 -1，select/flush 为
  无害 no-op）：
  ```c
  void nd_board_spi_select(nd_idis_board_state_t *state, nd_board_spi_t bus,
                           int active);
  void nd_board_spi_flush(nd_idis_board_state_t *state, nd_board_spi_t bus);
  int nd_board_spi_transfer(nd_idis_board_state_t *state, nd_board_spi_t bus,
                            const uint8_t *tx, uint8_t *rx, uint16_t len);
  ```
  `nd_board_spi_init(bus)` 只初始化 FLASH，保持无 state。
- 调用点：`board_flash.c` 的 `bus_select`/`bus_transfer` 把 bus ctx
  （state）传入；`board_esam.c` 的 `bus_transfer`/`bus_cs`/`bus_power`/
  `bus_delay` 同样从 ctx 取 state；`tests/unit/test_idis_board_flash_state.c`
  的 stub 同步签名。state 只经参数传递，不落文件级 static。
- `esam_dma`、`esam_tx_stage`/`esam_rx_sink`（两个 256 B 缓冲）在最终
  镜像里是活的（`arm-none-eabi-nm` 可见），搬进 root 的 BSS 净变化
  仍≈0，计划不变。

**RN：rn8209 残余与 measurement**

- `board_measurement_init(state, calibration)` 的签名已在 E 批改完；
  本批把内部的 `board_rn8209(state)` 接到 state，并迁移 `storage`/`dev`。
- measurement 端口不能只放 calibration：在 root context 增加
  ```c
  typedef struct {
      board_measurement_calibration_t *calibration;
      nd_idis_board_state_t *state;
  } board_measurement_ctx_t;
  ```
  并放一个 `board_measurement_ctx_t measurement_ctx` 字段。
  - main.c:358-359 的 `composition.measurement.ctx` 指向它；
  - main.c:540、575 的 `board_measurement_read` 直接调用改传
    `&g_idis_app.measurement_ctx`；
  - `board_measurement_read` 从 ctx 取 calibration 和 state，再调用
    `board_rn8209(state)`；
  - main.c:365、449 的 average/registry 仍使用同一个 measurement 端口，
    不需要改。
- `board_rn8209_init(state)` 已由 `board_bind` 调用；measurement init 内
  的 `board_rn8209(state)` 保留，以维持原有懒初始化语义。

**BP：board_ports**

- `void board_gpio_init(nd_idis_board_state_t *state)`：main.c:715；NULL
  state 直接返回（不注册）；PMU ISR 注册（board_ports.c 的
  `Isr_handle_regsit` 调用）的 `.para = (void *)&state->key_event`。
- `board_pmu_isr(void *param)` 把 param 当 `volatile uint8_t *`，NULL
  守卫后写 1；`key_event` 字段保持 `volatile uint8_t`。
- `int board_key_event_take(nd_idis_board_state_t *state)`：main.c:521；
  NULL 返回 0；读后清零 state 字段。
- 3 个硬件缓冲合并成一个 board-owned 静态聚合（不进 root），用命名类型
  让 mutable 检查器看得见，计数 3→1：
  ```c
  typedef struct {
      uint8_t flash_sector[BOARD_FLASH_SECTOR];
      struct fifo_t uart_fifo[3];
      uint8_t uart_fifo_buffer[3][BOARD_UART_FIFO_BYTES];
  } board_buffers_t;

  static board_buffers_t board_buffers;
  ```
  `board_flash_init` 传 `board_buffers.flash_sector`；
  `uart_channel_init`/`board_uart_init`/`board_ir_read` 用
  `board_buffers.uart_fifo*`。地址仍固定，ISR/timer 持有的指针不变。

`board_uart_init(state)` 已在 B2 接收 state，本设计不再改。

### 2.3 可变全局预期

- 全部批次完成后：180 → 146（S 延后）；S 日后落地才到 142。
  - E/F/SPI/RN/BP 移除 15 个符号 × 2 target = 30 条（含
    `flash_store`，不含 `board_store`：后者是 const，不在 baseline 计数）；
    S 若执行再 −4。
  - board_ports 的 3 个缓冲从 3 条 static 合并成 1 条，每 target 再
    −2，共 −4。
- `board_store` 从 const 静态搬进 root 不改变计数；不新增模板副本。
- 不新增文件级指针、锁或第二份所有者；state 仍是 `g_idis_app.board`。
- `DEVICE_ERR` 的 extern 删除后，头文件不再暴露可变对象。

## 3. 分批

每批一个 commit，独立回滚；每批都跑完整门禁。

| 批次 | 符号 | mutable 后 | 主要文件 | 验证 |
| --- | --- | --- | --- | --- |
| E：EEPROM | `DEVICE_ERR`、`error_streak`、`eeprom_storage`、`eeprom`；`board_store` 搬进 state；measurement init 加 state | 172 | board_eeprom.c/h、board_ports.c/h、board_state.h、main.c | host:strict（EEPROM 单测）、poll/IRQ 构建、验收 |
| F：FLASH | `flash_storage`、`flash`、`flash_store` | 166 | board_flash.c/h、board_ports.c/h、board_state.h、main.c | 同上，加 sector store 相关单测 |
| S：ESAM（延后） | `esam_storage`、`esam` | 延后；若执行 162 | board_esam.c/h、board_state.h | 不执行；最终 ELF 无 board_esam 符号（GC），移入 live root 实测 +72 B BSS |
| SPI：ESAM 暂存 | `esam_dma`、`esam_dma_ready`、`esam_tx_stage`、`esam_rx_sink`、`esam_tx_used` | 156 | spi_driver.c/h、board_state.h、board_ports.c | host 的 board_transport/SPI 可测部分、IRQ lane、验收 |
| RN：rn8209 与 measurement | `storage`、`dev`；measurement read 的 calibration+state ctx | 152 | board_rn8209.c/h、board_state.h、board_ports.c/h、main.c | 计量验收、poll/IRQ |
| BP：board_ports | `key_event`，并把 3 个缓冲合并为 1 个聚合 | 146 | board_ports.c/h、board_state.h、main.c | host:strict、PMU/按键路径、验收、mutable |

顺序 E → F → SPI → RN → BP：E/F 先把 store/flash 端口和主调用点改完；
S 延后到 `board_esam` 首个调用者出现；SPI 涉及 DMA 完成路径（保持既有
inert 语义）；RN 引入 measurement ctx；BP 最后处理 ISR para 和缓冲聚合。

每批证据：

- `host:strict`：现有 EEPROM/flash/sector store/board_transport 单测；
  新 API 的正反夹具（state NULL 时访问器失败）。
- poll 和 IRQ 两 lane 构建；`firmware-irq`、`irq-symbols`、`irq-rom`。
- 两个 IDIS target 验收 16/16。
- `check:mutable-globals` 的差只包含该批删除的行；`check:doc-evidence`。
- 回滚：revert 该批 commit；不涉及持久化格式，无需清 EEPROM/flash。

## 4. ROM / RAM 预算

当前（IRQ lane，main 805e4e10）：ROM 97,658 / 102,143 B，BSS
37,694 / 50,226 B；103 KiB = 105,472 B，余量 7,814 / 3,329 B。

E 批实测：IRQ ROM 97,790 / 102,283 B（+132 / +140 B，设计预计 worst
+96 B）；IRQ BSS 37,712 / 50,244 B（+18 / +18 B，设计预计 ±8 B）。
增量来自 state 间接和 store ctx 守卫，没有新功能。

对象和缓冲只是从静态搬到 root 或合并，RAM 净变化应接近 0。剩余各批按
每批 ±8 B、累计 worst +43 B 约束。ROM：

| 批次 | ROM 预计 | 最坏 | BSS |
| --- | --- | --- | --- |
| E | 实测 +132 / +140 B | +96 B（已超） | 实测 +18 B |
| F | +40 B | +80 B | ±8 B |
| S | 延后（未执行） | — | 若执行实测 +72 B |
| SPI | +32 B | +64 B | ±8 B |
| RN | +40 B | +80 B | 实测 +12 B（state +72、measurement_ctx +8、删静态 −68） |
| BP | +32 B | +64 B | ±8 B |
| 合计 | — | 140 + 288 = 428 B | 实测 35 B（18+0+5+12），BP ±8 → worst ≤43 B；S 若执行另 +72 B |

freertos 累计 worst 余量：3,329 − 428 ≈ 2,901 B。RAM 余量：freertos IRQ
BSS 50,261 B + 8 KiB 栈 ≤ 64 KiB，余 5,035 B。E 的超出已由评审接受，
不压缩；后续批次仍按“超过本批 worst 就停下重新 scope”执行（F 的 worst
是 +80 B，BP 的 BSS 停止线是 ±8 B）。103 KiB 是链接断言，不靠估算：
每批用 `check_idis_firmware.py` 的 poll/irq ELF 测 ROM/BSS。

## 5. 失败模式、回滚与边界

失败模式：

- 绑定/懒初始化顺序：访问器在 state 未 bind 时被调用。约束：访问器对
  NULL state 返回 NULL/-1；main 的 `board_bind` 在第一次存储/设备访问前。
- ISR para 类型：`key_event` 的 para 从 NULL 改为字段地址，写错会踩
  相邻字段。约束：注册点只有一个，ISR 只通过该地址写一个 volatile 字节。
- 存储回调的 ctx：`nd_store_t.ctx`、sector store ctx 从 NULL 改为 state；
  漏改会让回调拿到 NULL。约束：每批编译 + host 存储单测覆盖读写路径。
- `board_store` 的 const：它不能原地写 `.ctx`；E 批必须同时把对象搬进
  state 并改完全部 `board_store_port` 调用点。只改 `board_eeprom(state)`
  会留下 NULL ctx。保留 const 模板再拷贝不作为方案。
- E 批的顺序依赖：`board_calibration_load` 调 `board_store_port`，所以
  measurement init 的 state 参数必须在 E 批一起改；否则中间批次不编译。
  RN 批只补 measurement read 的 ctx。
- measurement ctx：RN 批必须把 calibration 和 state 一起打包进
  `board_measurement_ctx_t`；只改 calibration 指针会让
  `board_measurement_read` 拿不到 state，或把 state 当 calibration 读。
  直接调用点和 composition port 的 `.ctx` 必须同一批改。
- 缓冲区合并：字段名变化导致漏改引用。约束：编译期符号检查，全 target
  grep 旧名；合并只在 BP 一批完成。
- 驱动对象搬家：懒初始化指针未清空或重复初始化。约束：state 零初始化，
  首次访问仍走同一 lazy 分支；不改初始化时机。
- ESAM 的 inert DMA 完成路径：保持现状，不在本单元修复。

回滚：每批一个 commit；revert 即回到上一批；mutable baseline/frozen 行
随 commit 回退。不涉及持久化数据、协议或 EEPROM 布局。

不改 behavior 的边界：

- 同一组缓冲地址、回调、初始化时机和失败返回；只是存储位置/参数传递。
- 不新增锁、临界区或 volatile（`key_event` 的 volatile 是既有语义，只
  搬家）。
- poll lane 行为与验收逐条不变；IRQ lane 继续通过构建/符号/ROM 证据。

无 HIL 的证据边界：IRQ lane 只证明编译、链接、符号和 ROM；host 仿真
没有中断/EEPROM I2C/SPI DMA 时序模型。PMU ISR 与任务对 `key_event` 的
真实交错、EEPROM 错误重试、flash 擦写时序和 DMA 完成仍需 HIL；本设计
只能声明结构迁移和 poll 行为不变。

## 6. 残余

- `esam_dma` 的 inert 完成路径仍是既有行为，单独确认是死代码还是缺陷。
- `board_esam` 当前无调用者；迁移只保证编译/链接，不新增使用。
- `baremetal.c` 的 `systick_ticks`、stategrid 的同名 board 副本不在本
  单元范围。
