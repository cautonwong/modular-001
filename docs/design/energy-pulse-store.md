# 能量脉冲持久化（port 与引擎层）

状态：E1 实现完成。E2a 已把 journal 接到 IDIS 裸机、FreeRTOS 与 host；
stategrid 已绑定双 slot snapshot（见 8.1）。

参考输入：2026-11-19 的能量脉冲 arena 合成。基线取 candidate 2（引擎与崩溃
协议），嫁接 candidate 3 的两级端口和 candidate 1 的 program granularity。
设计包不随仓库提交；冲突时以本文已落地的决定和提交为准。

本文不复制候选文档，只记录已落地的决定、接口、协议与数字。

## 1. 选择与嫁接

基线选 candidate 2。理由：掉电边界是介质的物理性质。NOR 的一个 bit 只能
从 1 程序到 0，清除的 bit 天然构成前缀；base 先写、journal 后清的提交顺序让
每次崩溃最多丢一个在途脉冲，而不是丢一个保存窗口。

两处嫁接：

1. candidate 3 的两级端口。domain metering 定义消费端口 `nd_pulse_store`
   （`append`、`replay`、`base`、`commit`），引擎只依赖 block 端口
   `nd_block_ops_t`（`read`、`program`、`erase`、`size`、`erase_unit`）。
   设备差异留在 binding。
2. candidate 1 的 program granularity。默认 `ND_PULSE_PROGRAM_GRANULARITY = 1`，
   每次 `append` 一次 byte program，崩溃最多丢 1 个已接受脉冲；配置成 8 时
   最多缓冲 7 个已接受脉冲，换成每 8 个脉冲一次 program。

与 synthesis 的路径差异：synthesis 写的是 `application/ports/energy_pulse.h`，
本仓库的消费者是 `domain/metering`，而 `domain` 不能反向 include `application`
（`tools/check_layers.py` 拦截）。端口按"消费者定义"放在
`domain/metering/pulse_store.h`，与 AGENTS.md 的端口规则一致。

不在本层：domain metering 的 `bind_pulses` / `commit` / 持久化状态 API，
target 组合根，base 的 legacy adoption 接线，验收用例。domain metering 现有
公开 API 未改。

## 2. 分层与端口

```
domain/metering             nd_energy_set_t, pulse_lane.h, pulse_store.h (port A)
        ^
infra/storage               pulse_store.c (engine + dispatch), pulse_snapshot.c
                            block.h (port B), block_memory.c (RAM/file)
        ^
infra/drivers/storage/block eeprom_block, flash_block (port B bindings)
```

domain 只看见 `void *` 不透明句柄、lane 字节和 base blob；它不知道 sector、
erase、CRC、generation、marker 或 `nd_energy_set_t` 的序列化。infra 可以
include domain（`check_layers.py` 允许），实现方向是 infra → domain。

### 2.1 消费端口（port A）

`domain/metering/pulse_store.h`：

```c
typedef struct nd_pulse_store nd_pulse_store_t;

typedef enum {
    ND_PULSE_OK = 0,
    ND_PULSE_EMPTY = 1,
    ND_PULSE_FULL = 2,
    ND_PULSE_ERROR = -1,    /* base(): non-blank region, no valid base */
    ND_PULSE_IO_ERROR = -2  /* base(): device read failed, region unknown */
} nd_pulse_status_t;

typedef void (*nd_pulse_replay_fn)(void *ctx, uint8_t lane, uint32_t count);

nd_pulse_status_t nd_pulse_store_append(nd_pulse_store_t *store, uint8_t lane);
nd_pulse_status_t nd_pulse_store_replay(nd_pulse_store_t *store,
                                        nd_pulse_replay_fn sink, void *ctx);
nd_pulse_status_t nd_pulse_store_base(nd_pulse_store_t *store, uint8_t *buffer,
                                      size_t capacity, size_t *length);
nd_pulse_status_t nd_pulse_store_commit(nd_pulse_store_t *store,
                                        const uint8_t *blob, size_t length);
```

契约：

- `append`：`OK` 表示已接受。granularity 为 1 时返回即持久；为 N 时最多
  N-1 个已接受脉冲可能随崩溃丢失。`FULL` 表示什么都没写，commit 后重试同一
  脉冲。`ERROR` 表示持久状态未知，不要重试同一脉冲；消费者仍按物理脉冲记账，
  在下个安全点 commit。
- `replay`：只读扫描上次 commit 以来的 journal，按 lane 给出计数。语义上是
  boot 恢复，不参与运行期热路径。设备读错误返回 `ERROR`。
- `base`：复制上次 commit 的 blob。`EMPTY` 表示从未 commit 过，消费者需要
  从 legacy 记录迁移并 commit 一次。`ERROR` 表示读操作全部成功、但 region
  非空且没有有效 base（撕裂/外来记录）。snapshot 引擎把这个状态当作显式
  commit 可以覆盖的已知状态，写 generation 1；journal 引擎的 commit 需要
  已知有效 base，遇到此状态返回 `ERROR`，不能覆盖，需维护（§4.2）。
  `IO_ERROR` 表示设备读失败、region 状态未知：消费者必须重试，不能当作
  `EMPTY` 或 `ERROR` 去采纳 legacy 记录，否则错误在 commit 前消失时会把旧值
  写成真实 base 的下一代。`FULL` 时 `*length` 是所需容量。
- `commit`：blob 对引擎不透明，必须已包含上次 commit 以来 `append` 接受的全部
  脉冲。base 落盘成功后重置 journal；若重置失败，下一次 `append` 重新初始化，
  本次仍返回 `OK`（base 已经可信）。base 的三次 program 都成功但回读失败或
  记录无效时，generation 已经前移且 journal 标为待重建，本次返回 `ERROR`；
  磁盘 base 与 RAM generation 不允许不一致。base 扫描本身遇到设备读错误时
  不写，generation 只来自一次完成的扫描；状态码按引擎不同：snapshot 返回
  `IO_ERROR`，journal 返回 `ERROR`。journal 的 commit 只拒绝“非空白但无有效
  base”的撕裂状态；至少一个 slot 非空白且没有有效 base 时返回 `ERROR`，不写
  generation 1。两个 slot 全为 0xFF（`EMPTY`，出厂新表）不是撕裂：generation
  从 0 开始，commit 写 generation 1。

### 2.2 block 端口（port B）

`infra/storage/block.h`：

```c
typedef struct {
    void *ctx;
    int (*read)(void *ctx, uint32_t offset, uint8_t *buffer, size_t length);
    int (*program)(void *ctx, uint32_t offset, const uint8_t *buffer,
                   size_t length);
    int (*erase)(void *ctx, uint32_t offset, uint32_t length);
    uint32_t size;
    uint32_t erase_unit;
} nd_block_ops_t;
```

- NOR：`program` 只能 1→0，`erase_unit = 4096`，`erase` 后读 0xFF。
- EEPROM / RAM / file：`erase_unit = 1`，`erase` 用 0xFF 覆盖。EEPROM 驱动
  自己拆页并回读校验。
- 引擎只写 `old & ~mask`，所以撕裂的 program 最多让目标 bit 停留在 1，不会
  破坏其它 bit。

### 2.3 lane 编码（消费侧）

`domain/metering/pulse_lane.h`：

```c
#define ND_PULSE_DIRECTIONS 2u
#define ND_PULSE_TARIFFS 12u
#define ND_PULSE_LANES (ND_PULSE_DIRECTIONS * ND_PULSE_TARIFFS)

uint8_t nd_energy_pulse_lane(nd_energy_direction_t direction, uint8_t tariff);
nd_energy_direction_t nd_energy_pulse_lane_direction(uint8_t lane);
uint8_t nd_energy_pulse_lane_tariff(uint8_t lane);
```

编码是 `lane = (tariff - 1) * 2 + direction`，低 bit 是方向。tariff 0 和
大于 12 折到 1，与 `energy.c` 的 `valid_tariff` 共用同一个 inline
`nd_energy_pulse_tariff`，避免两处漂移。方向只有两条 unary lane；费率靠
marker 时间线重建，不占独立 lane。

## 3. 引擎状态机

`infra/storage/pulse_store.c` 是唯一持有 journal 几何、marker 时间线、
generation 和提交顺序的模块。句柄以 `nd_pulse_store_t port` 为首成员，公开
函数只做 ops 派发。

```
NO_BASE   两个 base slot 的 header 全为 0xFF   append -> ERROR；调用者先 commit
INVALID   base 有效，journal header 不匹配  append -> ensure_journal（erase，
          当前 generation                   写并校验两个 header）后 ACTIVE
ACTIVE    header 有效，cursor < capacity    append 写 bit；marker_pending
                                          时先重写并校验 marker
FAULT     设备读错误，或 base 非空但无有效   append/base/replay -> ERROR；
          slot（撕裂/外来记录）              base() 区分：撕裂为 ERROR，
                                             设备读错误为 IO_ERROR；
                                             journal commit 返回 ERROR，
                                             需要已知有效 base
```

`FULL` 不是存储状态，是 `cursor == capacity` 或 marker slot 用尽时 `append`
的返回值。

- `base()` 的三态：两个 slot 都空白 -> `EMPTY`；扫描完成且至少一个 slot 非空
  但没有有效 slot -> `ERROR`（已知撕裂状态；snapshot 的显式 commit 可用
  generation 1 覆盖，journal 的 commit 需要有效 base，见 §4.2）；任一读操作
  失败 -> `IO_ERROR`（region 未知，重试，不采纳 legacy）。`select_base` 已经
  选出有效 slot 后，再读 header/blob/CRC 失败也返回 `IO_ERROR`：那是一次读
  不一致，不是撕裂区域，不能触发 legacy adoption。

- 首次操作懒执行 `attach`：选 base、校验两个 journal header、扫描 bit 前缀、
  扫描 marker 恢复 `marker_count` 和当前 tariff。
- `select_base` 逐 slot 区分三种结果：有效、空白（header 全 0xFF）、非空但
  无效。有有效 slot 时取 generation 最大者；全部有效 slot 缺失且至少一个
  非空时返回 `ERROR`，只有两个 slot 都空白才是 `EMPTY`。撕裂的 base 永远
  不会伪装成首次启动。
- `ensure_journal` 只允许在 INVALID 时执行。此时 journal 里的 bit 要么属于旧
  epoch（base 已覆盖），要么来自崩溃的 commit（base 已覆盖），erase 不会丢失
  已提交的计数。
- FAULT 不自动 erase。读错误可能是瞬时的，erase 会把可读数据抹掉。base 扫描
  失败时 journal commit 返回 `ERROR`，generation 不前进；journal 需要维护，
  不能自愈。只有 snapshot 引擎会在已知撕裂（`ERROR`）时用 generation 1
  覆盖（§4.3）。

## 4. 存储布局

所有整数字段（magic、version、bytes、generation、CRC）在介质上按
little-endian 排列，所以文档里的 magic `"NDPJ"`/`"NDPB"`/`"NDPS"` 落到介质上
是字节 `"JPDN"`/`"BPDN"`/`"SPDN"`；`tests/unit/test_pulse_store*.c` 的 golden
逐字节钉住这个顺序。

### 4.1 journal region

region 内共享一份元数据，两个方向各有自己的 bit 区：

```
+0      16   header direction 0: magic "NDPJ", version, bytes, generation, crc
+16     16   header direction 1: 同上
+32    512   marker 32 x 16 bytes，append-only
+544    B0   direction 0 的 unary bits，LSB first
+544+B0 B0   direction 1 的 unary bits
```

- 每方向 bit 容量 `C = (region_bytes - 544) / 2 * 8`。
- 一个 marker 记录两个方向当时的 cursor 和 tariff，所以一条 marker 流就够；
  candidate 2 每个 lane 各放一份 marker 的写法没有沿用。
- region 必须按 `erase_unit` 对齐且长度是其整数倍；NOR 用 4 KiB 扇区。
- 已知值：IDIS 裸机 16,384 B → 每方向 7,920 B = 63,360 bits。stategrid
  6,144 B → 每方向 2,800 B = 22,400 bits（EEPROM，`erase_unit = 1`）。

### 4.2 base 记录

两个交替 slot，各 `base_capacity` 字节（IDIS 1,024 B）：

```
+0      12   magic "NDPB", version, bytes(blob 长度 <= 65535), generation
+12     N    blob（引擎不解释；IDIS 是 nd_energy_set_t 的 836 B）
+12+N   4    CRC-32（覆盖前 12+N 字节）
```

`nd_pulse_store_config_t.base_block` 可选，`NULL` 表示与 journal 同一设备。
IDIS 裸机把 journal 放 flash、base 放 EEPROM，所以组合根分别构造两个 block 端口。

`generation` 每次 commit 加一，slot = generation & 1。恢复时选 CRC 正确且
generation 最大的 slot；新 slot 撕裂时旧 slot 仍完整。

- 两个 slot 的 header 全为 0xFF 才算 `EMPTY`。任一 slot 非空但无法通过
  magic/version/长度/CRC 校验，且没有另一个有效 slot 时，`base()` 返回
  `ERROR`（已知撕裂状态），**不会**被当成首次启动而触发 legacy adoption。
  journal 的 commit 在此状态下返回 `ERROR`，不写 generation 1：提交需要
  扫描至少选出一个有效 slot，损坏区需要维护。
- 任一读操作失败时 `base()` 返回 `IO_ERROR`，commit 返回 `ERROR`：region
  状态未知，调用者重试。读错误不是 `EMPTY`，也不是可被显式 commit 覆盖的
  `ERROR`。
- 一个 slot 读失败时扫描立即失败，即使另一个 slot 有效也不自动回退。
  restore 保持 pending 并置 `storage_fault`，需要维护；这是 fail-closed，
  不是掉电丢失。
- 单个空白 slot 加单个撕裂 slot 也是 `ERROR`；valid slot 加撕裂 slot 则
  使用 valid slot，撕裂 slot 只是上一次提交的残留。

### 4.3 snapshot 记录

snapshot fallback 保留整记录重写语义，在同一 block 端口上实现 port A。两个
交替 slot，每个 slot 长度是 4 字节对齐的 `12 + blob_bytes + 4`：

```
+0     12   header: magic "NDPS", version, bytes, generation
+12     N   blob
+12+N   4   CRC-32（覆盖 header + blob）
```

- `generation` 每次 commit 加一，slot = `generation & 1`，从 generation 1
  开始。恢复时两个 slot 都验 magic、version、长度、CRC 和 generation，取
  generation 最大的有效 slot。
- 最新 slot 撕裂（写了一半或 CRC 错）时回退到另一个 slot，也就是上一代
  commit。两个 slot 都非空但都没有有效记录时，`base()` 返回 `ERROR`（已知
  撕裂状态），**不会**因错误而回退旧 legacy 记录。任一读操作失败时返回
  `IO_ERROR`，调用者重试，旧 legacy 记录同样不被采纳。一个 slot 读失败时
  扫描立即失败，即使另一个 slot 有效也不自动回退；restore 保持 pending 并
  置 `storage_fault`，需要维护。
- 两个 slot 全为 0xFF 时 `base()` 返回 `EMPTY`。消费者在 `EMPTY` 时读取旧
  172 字节记录一次并 commit 成 generation 1；之后旧记录只作为降级镜像。
  `ERROR` 时也做一次显式恢复，但 `IO_ERROR` 不恢复。`commit` 在扫描遇到
  设备读错误时返回 `IO_ERROR` 且不写；撕裂（`ERROR`）时用 generation 1
  覆盖。这是 snapshot 独有的自愈；journal 在同样状态下拒绝 commit（§4.2）。
- `append` 返回 `OK` 但不写任何东西，`replay` 不产生 segment，丢失窗口等于
  commit 间隔，不是单个脉冲。`commit` 先写 header、再写 blob、最后写 CRC，
  然后回读校验；校验失败返回 `ERROR`，另一个 slot 的上一代记录仍然有效。
- 该格式不再兼容旧 snapshot 的 magic 0x454E4531。stategrid 之前没有绑定
  snapshot，所以没有已部署数据；旧 172 字节记录在 0x450A+3 由第一个
  generation 迁移，保留供降级读取。

## 5. 协议

### 5.1 append

```
append(lane):
    校验 lane < 24
    attach；NO_BASE/FAULT -> ERROR
    INVALID -> ensure_journal；仍失败 -> ERROR
    direction = lane % 2; tariff = lane / 2 + 1
    if tariff != current_tariff or marker_pending:
        marker slot 用尽 -> FULL
        flush 两个方向已缓冲的 bit          # 旧费率脉冲必须停在 marker 之前
        program marker {cursor[2], tariff, crc} 并回读校验
        失败：不消耗 slot，marker_pending = 1，ERROR，不写 bit
        成功：marker_pending = 0，marker_count++，current_tariff = tariff
    if cursor[direction] == C: FULL
    在 shadow 中清掉 cursor+pending 位置的 bit，pending++
    达到 granularity 或字节写满 -> flush
```

`flush` 写一个 byte 并回读，只有回读等于目标值才推进 cursor、清 pending。
失败的 program 不推进 cursor，重试写同一个位置，所以不会出现空洞或重复计数。

marker 失败不消耗 slot：下一次 `append`（即使费率与当前值相同）先在同一
slot 重写并回读校验 marker，成功后才追加 bit。program 已落盘但回读失败的
marker 可能领先 RAM 一个 slot；由于失败后不再追加 bit，已记录的脉冲仍按旧
费率切分，重试成功后该 slot 被新 marker 覆盖。这样既不会留下 0xFF 空洞让
`scan_markers` 提前停止，也不会把后续旧费率脉冲错归到新费率。

### 5.2 commit 与恢复

```
commit(blob):
    attach；base 读错误，或非空白但无有效 base（撕裂）-> ERROR（不推进，不写）
    两个 slot 全空白（EMPTY，出厂新表）-> generation 0，继续写 generation 1
    next = generation + 1; slot = next & 1
    写 base header、blob、CRC；任一 program 失败 -> ERROR（不推进）
    回读并按 CRC + generation 校验：
      回读成功且 generation == next -> generation = next
      回读成功但 generation != next -> ERROR（generation 不推进，journal 不动）
      回读设备错误，或回读成功但记录无效（magic/CRC/长度错）
          -> generation = next，重置 cursor/pending/shadow/marker，
             journal 标 INVALID，ERROR
    重置 cursor/pending/shadow/marker_count/tariff/marker_pending
    erase journal region
    写并校验两个 journal header（generation = next）
    erase/header 失败 -> state = INVALID，返回 OK（base 已可信）
```

snapshot 的 commit 状态码不同：base 扫描读错误返回 `IO_ERROR`，已知撕裂
（`ERROR`）时写 generation 1 覆盖（§4.3）。

- base 先于 erase：base 可信之前 journal 不被触碰。
- base 的三次 program 都返回成功即视为已提交。回读失败（设备读错误）或回读
  成功但记录无效（magic/长度/CRC 错）都不能把 generation 退回：否则磁盘 base
  是 next，而 RAM 仍按旧 epoch 追加，重启后新 base 胜出、旧 epoch 的 journal
  整体被忽略，失败 commit 之后接受的脉冲全部丢失。两种情况下 generation 都
  前移、journal 标 INVALID，下一次 `append` 按 next 重建 journal；磁盘 base 与
  RAM generation 不再可能不一致。只有"回读成功、记录有效但 generation 不是
  next"这一种不一致会拒绝推进并返回 `ERROR`，此时 journal 保持不变。
- generation 在 erase 前推进：即使 base 已落盘而 erase 失败，旧 journal 的
  epoch 不匹配，恢复时整体忽略，不会把旧 bit 记到新 base 上。
- 两个 header 都必须匹配 base generation，才允许 replay 和 append；只有
  header 0 有效时不会出现"一个方向可写、另一个方向不可写"的混合状态。

```
restore:
    选最高 valid generation 的 base slot
    两个 slot 全 blank -> EMPTY；非空且无有效 slot -> ERROR
    任一读操作失败 -> IO_ERROR，消费者重试且不采纳 legacy
    有有效 slot 且两个 header 均匹配 generation 且 CRC 正确 -> 扫描
    否则 INVALID，replay 返回 0 段，下一次 append 重建 journal
    扫描每个方向的 bit 前缀 -> cursor + shadow
    扫描 marker：统计已消耗 slot 数（含坏 CRC），取最后一个 tariff 有效的
    marker 作为当前 tariff（修复点）
    replay 每个方向：按 marker 的 cursor 切段，对每段调用一次
    nd_energy_set_add_pulses；marker CRC 错误或 cursor 越界的段跳过
```

修复点（candidate 2 的 `restore` 遗漏）：恢复必须同时还原 `marker_count` 和
当前 tariff。否则重启后第一次费率变化会从 slot 0 开始写，覆盖已经存在的
marker，时间线被破坏。`test_pulse_store_marker.c` 在每次重启后检查 slot 0
字节不变、同费率不再写 marker。

### 5.3 program granularity

`nd_pulse_store_config_t.granularity` 取 1..8，默认宏
`ND_PULSE_PROGRAM_GRANULARITY = 1`。

- N > 1 时 `append` 只更新 RAM 里的 shadow 和 pending，返回 `OK`。
- pending 达到 N 或当前字节写满（bit 7）时一次 program。
- 费率变化先 flush 两个方向，再写 marker，旧费率的 bit 不会被算进新费率。
- commit 不 flush：blob 按契约已包含 pending 的已接受脉冲，base 落盘后
  pending 直接丢弃；崩溃则最多丢 N-1 个已接受脉冲。
- 代价：崩溃边界从 1 变成 N-1；收益：同一 byte 的 program 次数从 8 次降到
  1 次，若 flash 的 endurance 按 byte program 而不是 erase 计，寿命乘 8。

## 6. 崩溃矩阵

记真实物理脉冲数为 `N`，恢复计数为 `R`。"丢失"按已接受脉冲计。所有行的
double count 都是 0；`R <= attempted` 由 bit 前缀和"每个物理脉冲最多一个 bit"
保证，`R <= N` 随之成立。TLA+ 模型（`formal/tla/EnergyPulse.tla`）确认了更强的
口径：没有返回 `OK` 的在途脉冲也可能被恢复计数，所以无重复计数的正确表述是
`recovered <= attempted`，不是 `recovered <= accepted`；`accepted - recovered <=
granularity` 只在单个 commit/attach 窗口内成立，跨多次掉电时累计的 accepted
缺口无界。

| # | 崩溃点 | 介质状态 | 恢复规则 | 最多丢失 |
| --- | --- | --- | --- | --- |
| 1 | pulse program 撕裂 | 目标 bit 读 0 或 1 | 扫描停在第一个 1 | 1 |
| 2 | pulse program 驱动报错，bit 未写 | bit 仍为 1，cursor 未推进 | 下个脉冲重试同一 bit | 1 |
| 3 | program 成功但回读失败 | bit 已为 0，cursor 未推进 | 重试同一位置（NOR 幂等） | 0（该脉冲算入） |
| 4 | marker 撕裂 | slot CRC 错 | 跳过该 slot；新费率下无 bit | 1（写出错的脉冲） |
| 5 | marker slot 用尽 | 32 个 slot 都消耗 | `FULL`，commit 后重试 | 0（commit 后） |
| 6 | capacity 用尽 | 该方向 `cursor == C` | `FULL`，commit 后重试 | 0（commit 后） |
| 7 | base 写撕裂 | 旧 slot CRC 有效，新 slot 无效 | 旧 base + 完整 journal | 0 |
| 8 | base 校验通过，erase 前崩溃 | 新 base gen+1，journal gen 旧 | 忽略 journal | 0 |
| 9 | erase 撕裂 | 部分 sector 旧数据 | header 不匹配，重初始化 | 0 |
| 10 | header 写撕裂 | 新 base gen+1，header 无效 | INVALID，重初始化 | 0 |
| 11 | 两个 header 之间崩溃 | header 0 有效、header 1 无效 | 不允许 append；重初始化 | 0 |
| 12 | 一个 base slot 不可读 | 另一个 slot 有效 | 扫描失败 -> `IO_ERROR`；不自动用另一个 slot，restore 保持 pending 并置 `storage_fault`，需维护 | 故障，不是掉电 |
| 13 | 两个 base slot 都无效 | 全 0xFF，或至少一个非空无效 | 全 0xFF -> `EMPTY`；非空无效 -> `ERROR`：stategrid（snapshot）显式恢复一次，IDIS（journal）commit 拒绝；读失败 -> `IO_ERROR`，不采纳 | 故障，不是掉电 |
| 14 | journal 读错误 | 扫描截断 | INVALID/FAULT，不 replay | 未读部分（故障） |
| 15 | granularity N 崩溃 | 最多 N-1 个已接受 bit 只在 RAM | 扫描已写 bit | N-1 |
| 16 | snapshot 整记录撕裂 | magic 或 CRC 失败 | 整条记录丢失 | 上一个 commit 间隔 |
| 17 | base program 成功但回读失败或 CRC 不符（运行期，不掉电） | 新 base gen+1，journal header 仍是旧 generation | generation 前移，journal 标 INVALID，`ERROR`；下次 append 按新 generation 重建 | 0（按 program 成功即持久） |
| 18 | marker program 报错，未写入（运行期） | 该 slot 仍为 0xFF | 不消耗 slot，`marker_pending`，`ERROR`；重试写同一 slot | 0（该脉冲未被接受，调用者账面并 commit） |
| 19 | marker 已落盘但回读失败（运行期） | slot 可能是新 marker | 不消耗 slot，阻塞后续 append；重试同一 slot 校验成功后才继续 | 0（失败后无旧费率脉冲落在 marker 之后） |
| 20 | base 扫描读错误（运行期） | 介质内容不变 | `base()` 返回 `IO_ERROR`；调用者重试、不 commit、不采纳 legacy。错误消失后下一次读到真实 base | 0 |

第 17-20 行是运行期设备错误，不是掉电点；其余行是掉电崩溃点。行 17 的前提
是 program 返回成功即视为持久，与 `block_ops` 的契约一致；若介质会谎报
program 成功，任何软件都无法在不成功回读的情况下区分。行 20 的调用者规则
是 R1 的根因修复：设备读错误不能与撕裂 region 共用 `ERROR`，否则错误在
commit 前消失时，legacy 值会被写成真实 base 的下一代。

对照当前实现：IDIS 裸机与 FreeRTOS 都绑 journal。journal 是脉冲的持久化机制，
append 成功即写入 journal（granularity 8 时最多 7 个 bit 仍在该字节的 RAM
pending），所以 300 s 保存（`nd_idis_energy_save`）不写 base、也不比较 book。
`nd_idis_energy_commit` 只在 journal 装不下 book 时调用：FULL、append ERROR
后的重试、寄存器清零/设总，以及掉电时 `energy_appends_pending` 为真。该
标志是粗粒度闩锁：每秒出现脉冲就置位，任何一次成功 commit 后清除，但
`nd_idis_energy_pulse` 为 FULL 内部 commit 后不清它（该接口不回报内部是否
commit）。因此掉电时可能多一次 commit：journal 已被内部 commit 折叠、标志
仍为 1，于是再写一次 base 并 erase journal。每次掉电最多多这一次，换取组合
根不必跟踪引擎内部提交，也不会漏掉真正未提交的脉冲；让
`nd_idis_energy_pulse` 回报内部 commit 会改动四个调用点共用的 twin API，
收益只是省掉一次罕见写，不做。host 每次运行结束 commit 一次。300 s 保存
不再是 commit 边界；掉电丢失窗口仍只由 granularity 决定（最多 N-1 个已接受
脉冲），与两次 commit 间隔的时间无关。

## 7. 磨损模型

输入：

| 输入 | 值 | 来源 |
| --- | --- | --- |
| meter constant | 2000 pulses/kWh | `targets/idis_ip_meter/app/main.c` |
| 10 kW | 5.56 pulses/s，480,000/day | 推导 |
| 41 kW | 22.8 pulses/s，1,968,000/day | 推导 |
| journal region | 16 KiB，4 x 4 KiB sector | 设计 |
| 每方向容量 | 63,360 bits | 计算 |
| 每次 commit 的 sector erase | 4 个扇区各 1 次 | 设计 |
| flash endurance | 100,000 erase cycles | 数据手册典型值，待确认 |
| EEPROM endurance | 100k / 1M / 4M byte writes | 数据手册变体，待确认 |
| IDIS commit 触发 | 仅当 journal 装不下 book：FULL、append 错误重试、寄存器清零/设总、掉电且 `energy_appends_pending` 为真（粗粒度闩锁，内部 FULL commit 后仍可能多一次）；300 s 保存不写 | `nd_idis_energy_commit` |
| 修复前 IDIS 定时 commit | 288 次/day | `EEPROM_PERIOD = 300` |

最坏情况全部脉冲落在同一个方向（两个方向平摊时 wrap 数减半）：

| 项目 | 10 kW | 41 kW |
| --- | --- | --- |
| wrap/day（480k / 1.968M ÷ 63,360） | 7.58 | 31.06 |
| 每扇区 erase/day | 7.58 | 31.06 |
| 扇区寿命 @100k | 36.2 年 | 8.8 年 |
| base 每 slot 写/day（隔次 commit） | 3.79 | 15.53 |
| base 寿命 @100k | 72.3 年 | 17.6 年 |
| base 寿命 @1M | 723 年 | 176 年 |
| 修复前 IDIS 定时 commit @100k | 0.95 年 | 0.95 年 |
| 修复后 IDIS，无脉冲 | 0 次 erase/day | 0 次 erase/day |
| 修复后 IDIS，持续脉冲 | 7.58 次 erase/day | 31.06 次 erase/day |
| snapshot @200 脉冲保存 @100k | 0.11 年 | 0.028 年 |

- journal 的磨损由 erase 主导，不由 1 byte program 主导；每个脉冲一次
  program，每 63,360 个脉冲一次四扇区 erase。
- commit 由 journal 容量驱动，不由 book 差异或定时器驱动：
  `nd_idis_energy_save` 不 commit；`nd_idis_energy_commit` 在 FULL、append
  错误重试、寄存器清零/设总和掉电有未提交 append 时调用。持续脉冲下
  erase/day 等于 wrap/day，300 s 检查本身不写 flash。
- base 的 EEPROM 磨损从"每 300 s 整记录重写"降到"每次 wrap 一次交替 slot"。
- 若 MX25 的 endurance 按 byte program 计，granularity 8 把最坏 program 次数
  除以 8；崩溃边界升到 7 个脉冲。

实测磨损（`targets/idis_ip_meter/tests/unit/test_idis_energy_commit.c`，16 KiB
journal、4 x 4 KiB sector、granularity 1、计数 block）：288 个 300 s 窗口各追加
1 个脉冲并保存 = 0 次 erase，重启后 288 个脉冲全部从 journal 恢复。追加到
63,360（每方向容量）后的第 63,361 个脉冲触发 FULL，恰好 1 次 commit = 1 次
erase（16,384 B，即 4 个 sector erase）。仅追加 2 个脉冲后的掉电 commit = 1
次 erase；寄存器清零即使 book 已等于 base 也 commit 1 次，journal 清空后
replay 不复活旧电量。修复前同一 288 窗口用例测到 288 次 erase。

实测（本机 Zephyr `arm-none-eabi-gcc 14.3.0`，Cortex-M0，`-Os`）：
`pulse_store.c` text 2,534 B，`pulse_snapshot.c` 434 B，
`block_ops.c` 360 B。FreeRTOS 的 GNU 14.3.rel1 ROM 余量以 CI 的
`check_idis_firmware.py` 为准。

## 8. target 绑定表

| target | 绑定 | journal / base | 掉电窗口 | 备注 |
| --- | --- | --- | --- | --- |
| IDIS 裸机 | flash journal + EEPROM base | flash 0xA0000 起 16 KiB；base 0xE800/0xEC00 各 1 KiB | 7 个脉冲（granularity 8） | profile 区结束于 640 KiB，image 区从 1 MiB 开始，中间空闲；MX25 的 byte-program endurance 未确认，用 8 保守 |
| IDIS FreeRTOS | flash journal + EEPROM base（与裸机同绑定） | flash 0xA0000 起 16 KiB；base 0xE800/0xEC00 各 1 KiB | 7 个脉冲（granularity 8） | 与裸机共用 twin 源码和适配器；ROM 以 `check_idis_firmware.py` 的 ELF 输出为准 |
| stategrid | 双 slot snapshot（不绑 journal） | EEPROM 0xC000 起两个 188 B slot（172 B blob + 12 B header + 4 B CRC）；旧 172 B 记录在 0x450A+3 只做首次迁移 | commit 间隔；默认 `EC_SaveDiff=200` = 2.00 kWh，约 120/492 次/天（10/41 kW） | 6 KiB EEPROM journal 在 100k 下只有 0.22-1.42 年，不可行；region 高水位实测 0xAF0A，margin 4342 B，boot guard 见 8.1 |
| host | file block | flash 文件 0xA0000 16 KiB journal；EEPROM 文件 0xE800/0xEC00 base | 1 个脉冲 | `block_file` 跨进程保留 journal 和 base |

组合根只构造端口并在启动时绑定一次；端口表用 `void *ctx`，没有设备驱动模型、
`device_ops` 或驱动注册表。

### 8.1 E2b 修正（stategrid）

E2b 的只读设计推翻了两条早期假设，先记录再实现：

- **6 KiB EEPROM journal 不可行。** 按 meter_const 2000/3200、10/41 kW、每方向
  22,400 bits、每 wrap 最坏 9 次 byte write、100k/1M/4M endurance 计算：
  100k 下 0.22-1.42 年，1M 下 41 kW 只有 2.17-3.46 年。收入表计不能用。
- **`EC_SaveDiff = 200` 不是 200 个脉冲，是 2.00 kWh。** 当前 172 B 记录在
  10/41 kW 下每天写 120/492 次，100k 寿命 2.28/0.56 年，1M 22.8/5.57 年。
  §7 的 "200 脉冲保存" 模型只适用于 host 的 `save_step`，不适用于 stategrid。
  E2b 结论：绑 snapshot（180 B，CRC + 端口），磨损与当前路径等价，不缩小
  丢失窗口；journal 保持可替换，等产品确认 endurance/负载/更大区域。
- **journal 的 1 脉冲保证在 append 之后，不在脉冲端子。** ISR 只累加
  `ec_pluse_cnt`，下一秒任务才 drain；掉电最多丢 1 秒的脉冲，41 kW/3200 下
  最多 36 个，cap 64。不要把 target 表里的 1 个脉冲写成端子级承诺。
- **0xC000 区域已实测并在启动时保留。** `firmware_sim` 在组合根打印
  `[sim] eeprom-used=44810 snapshot-region=49152 margin=4342`：allocator 高
  水位 `0xAF0A`（44,810 B，event 区末尾），snapshot 区 `0xC000..0xC177`
  （两个 188 B slot），与高水位之间余 4,342 B，远低于 `BOARD_EEPROM_SIZE`
  0xFFFF，也低于 registry 约定的 `0xD800`。组合根在每次启动时调用
  `get_device_used()`，一旦未来 `add_block` 越过 `0xC000` 直接 fail boot；
  同时断言旧记录偏移 `persistent_eprom + 3 == 0x450D`。这不是只读设计，是
  可执行的启动门禁。
- **实现默认（产品可覆盖）。** E2b 按以下默认落地，产品确认后可在组合根改，
  不改持久化格式：
  1. 双 slot snapshot，`0xC000` 起，slot stride 188 B，generation + CRC；
     boot 取最高有效 generation，最新 slot 撕裂回退上一代；丢失窗口等于
     commit 间隔（`EC_SaveDiff`，默认 2.00 kWh），不是单脉冲。
  2. 不实现 flash journal（磨损 5.5-8.8 年 @41 kW/100k，partition/watchdog
     未验证）。
  3. `EC_SaveDiff` 语义和 512 B 持久化块/1368 B 控制块布局不变。迁移期保留
     旧 `Save_EC2` 镜像写入，便于降级；确认不需要降级后可删除，届时 EEPROM
     写次数减半（每次 commit 188 B，不再是旧记录 + snapshot 双写）。
  4. `PowerOff_Save_data` 在 `Energy_Meter_POWEROFF` 之后显式 commit 一次：
     188 B，跨 2 个 128 B 页写并回读校验，比旧的 512 B 持久块写小。掉电保持
     时间仍以真机/HIL 证据为准，本文不把它当成已验证。
- 仍待产品确认：EEPROM endurance（100k/1M/4M）、最大持续负载和 meter_const
  （2000/3200）、`0xC000` 的物理 EEPROM/bootloader 归属、`EC_SaveDiff` 是
  2.00 kWh 还是 200 脉冲、可接受的突发丢失窗口、以及 41 kW 下 10 年寿命是否
  硬要求。若 10 年是硬要求，要么 1M+ 且 `EC_SaveDiff=1000`，要么更大且实测
  过的 flash journal；当前双 slot snapshot 不缩小丢失窗口。

## 9. legacy adoption 与 CRC

- 新的 base 记录带 CRC-32（header + blob），恢复时两个 slot 都校验 CRC 和
  generation。当前 `nd_idis_energy_store.c:33-35` 只检查 magic，新 base 路径
  不允许只凭 magic 接受数据。
- journal 引擎对 base 使用同一区分：两个 slot 的 header 全 0xFF 才是 `EMPTY`；
  任一 slot 非空但无效且没有有效 slot 时 `base()` 返回 `ERROR`；任一读操作
  失败返回 `IO_ERROR`。消费者只在 `EMPTY` 时读旧记录做一次显式迁移。
  `ERROR` 时 `nd_idis_energy_adopt` 也会尝试显式恢复，但 journal 的 commit
  需要已知有效 base，会拒绝写 generation 1 并返回 -1；此时 store 需要维护，
  不能自愈。`IO_ERROR` 不采纳旧记录：region 未知，重试即可，IDIS 的
  `nd_idis_energy_load`/`nd_idis_energy_adopt` 都返回 -1。IDIS 的普通 load
  不在 `ERROR` 时静默采纳，由 `nd_idis_energy_adopt` 显式处理。
- 首次升级时 `base()` 返回 `EMPTY`：组合根读旧记录（magic 0x454E4531 +
  `nd_energy_set_t`，无 CRC），把 set 作为 generation 1 的 base commit 一次，
  之后不再读旧记录。旧记录保留不删，允许降级读回升级前的快照，但降级会丢掉
  journal 里的脉冲。E2a 的 `nd_idis_energy_load` 在 `EMPTY` 时做这次迁移；
  每次都先写并校验新 base，再重放 journal。
- E2a 的接线在 adoption 路径上先写并校验新 base，不允许只凭 magic 接受数据；
  旧记录只作为迁移来源。
- stategrid 的 snapshot 用新格式（magic `NDPS`、版本、长度、generation、
  CRC），旧固件不能读这些 slot；旧 172 字节记录仍保留在 0x450A+3，作为降级
  镜像和首次迁移来源。没有已部署的旧 snapshot 数据，因为 stategrid 之前只有
  设计没有绑定。
- stategrid 迁移镜像旧固件的 `Check_EC_recover`：仅当 EEPROM 的
  `ec_save_success == 0x5AA5` 时，0x450A+3 的掉电记录（EC1）才是权威来源；
  标志已清说明它在上一次启动被消费过，此时优先 parameter_1 的 EC2 双备份
  （energy 160 B + month 12 B，各带项校验和），EC2 无效再回退 EC1。**energy
  与 month 两个半记录独立校验、独立采纳**：EC2 的 month 项撕裂时，有效
  energy 项仍然被采纳（ECC/校验和按半记录算）；反之亦然。每个 EC2 半记录
  读取失败时先重试一次；参数管理器把项校验和错误和总线错误报成同一种失败，
  重试用来吸收瞬时总线错误。EC2 energy 半记录持续读失败时不再无条件按缺失项
  回退有效的 EC1 半记录。power-off 标志已清时 EC2 才是较新的来源：首次启动
  （`EMPTY`）回退会把总电量静默退回上次掉电值，所以 restore 返回 -1、保持
  `restore_pending` 并置 `storage_fault`。snapshot 撕裂（`ERROR`）时 legacy
  mirror 是恢复来源，仍用 EC1 恢复并 commit generation 1，但 restore 返回 1，
  适配器置 `storage_fault`：恢复值可能退回上次掉电值。EC1 也没有有效 energy
  时两个来源都不可用，仍按既有的从零开始。EC2 month 半记录持续读失败仍按缺失
  项处理。两个来源都空白或损坏才从零开始。标志读失败、或 EC1 读失败且两个
  半记录无法都从 EC2 得到，按存储故障返回 -1。
- 两个 slot 都非空且无有效记录（`ERROR`）时不再永久拒绝持久化：损坏区是
  已知状态，snapshot 的显式 commit 可以 generation 1 覆盖它，所以
  `legacy_energy_restore` 走与 `EMPTY` 相同的显式恢复：采纳旧镜像、commit、
  成功后才 `metering_import`。设备读错误返回 `IO_ERROR`，`restore` 直接
  返回 -1：不采纳旧镜像、不 commit，恢复留在重试窗口。早期实现把读错误也
  归入 `ERROR`，并假设"那次 commit 会失败"；实测不成立：slot header 读失败
  一次后错误在 commit 前消失，commit 成功，旧镜像被写成真实 base 的下一代。
  现在端口层分开 `IO_ERROR` 与 `ERROR`，且 commit 的 generation 只来自一次
  完成的 base 扫描。
- `restore_pending` 期间 book 未恢复：不 export 到 mbase、不 sync legacy
  view、不写 EC2/EC1、不接受 commit，清零/设总返回 -1。窗口内的脉冲只记在
  RAM：stategrid 绑的是 snapshot 端口，`append` 不落盘也不写 journal
  （`pulse_snapshot.h` 明说 append 不推进），所以掉电会丢这段窗口，恢复成功
  后把它们按方向+费率一次性加到恢复出的 book 上（durable base + replay）。
  **重放基线是恢复出的 book，不是恢复前的 RAM book**，所以不会双计。如果
  将来换成会持久 append 的 journal 端口，`replay` 已经计入这些脉冲，那时
  必须停止 pending 累加（`restore_pending` 只在恢复失败窗口内为真，正常
  append 路径不在窗口内），否则会双计。恢复后的合并 book 立即 commit 一次。
- 掉电保存时若 `restore_pending` 仍为真，`PowerOff_Save_data` 不从 0 覆盖
  EEPROM 的 legacy 电能区，也不置 `0x5AA5`：它先把旧电能区读回 RAM，再整块
  写回，保留上一份可信镜像。若这次读回本身失败（`nd_get_offset` 返回 < 0），
  它跳过整块镜像写，而不是把零 RAM book 写回 durable 镜像。
- 非脉冲变更（全清零、月度清零、设置总电量）走 book 聚合，落盘成功才对外
  返回成功；commit 失败返回 -1 并置存储故障位。645 电表清零、645 参数初始
  化、698 ACTION 0x4300 方法 3/4 都把 `meter_clr()` 的失败转成错误应答（645
  错误字节 01、698 DAR 255），不再 ACK 成功。月度清零只清 book 的 `month`，
  再 `sync_legacy_view` 并 commit；直接改 mbase 字段会被下一次 pulse 的
  export 覆盖。
- `storage_fault` 是锁存位：`Energy_Meter` 在 `report_2015state_judge` 之前
  跑，成功不再清它；只有上报路径（`nd_platform_device_error_reported`）确认
  已上报才清，保证同一秒内的恢复不会吞掉存储器故障事件。
- IDIS 的 `nd_pulse_snapshot` 用法（IDIS 测试里）在空白区域返回 `EMPTY`，
  在外来/撕裂记录上返回 `ERROR`，在读失败上返回 `IO_ERROR`。
  `nd_idis_energy_adopt` 只对 `EMPTY` 或 `ERROR` 做一次显式迁移，写并校验
  generation 1；普通 load 不会在 `ERROR` 时静默采纳旧记录，`IO_ERROR` 一律
  返回 -1 等重试。E2a 的 FreeRTOS 不绑 snapshot（ROM 不够）。

## 10. 待拍板的产品决定

1. IDIS FreeRTOS 的 ROM。E2a 曾因为 pinned 余量只保留旧整记录；当前 FreeRTOS
   target 已与裸机同绑 journal（见 §8 与 `targets/idis_ip_meter_freertos/app/main.c`），
   ROM 门槛以 CI 的 `check_idis_firmware.py` ELF 输出为准。
2. MX25L3206E 的 endurance 口径。未确认按 byte program 计，所以 IDIS 裸机
   已用 granularity 8，把掉电边界从 1 改成 7；确认按 erase 计后可以回到 1。
3. commit 时序。4 KiB sector erase 典型 200 ms、最坏 1.6 s，看门狗 2 s。
   commit 必须排在一次 tick 之后，绝不能放进 brownout 路径；真机/HIL 证据
   目前没有。E2a 的裸机在 on_second 末尾处理持久化失败重试，FreeRTOS 走
   worker 任务。寄存器清零是例外：它在 COSEM action 里同步 commit，并在前后
   feed 看门狗，因为清零必须在报成功前落盘；这是罕见的配置操作，不在
   brownout 路径。
4. stategrid：已按 8.1 的默认绑定双 slot snapshot；0xC000 由启动 high-water
   guard 保留，6 KiB EEPROM journal 不可行。
5. base slot 与 registry blob 的边界、旧记录保留多久。
6. E2 的 domain metering API：`bind_pulses`、`commit`、持久化状态/事件。
   非脉冲变更里的寄存器清零已在 IDIS 绑定 `save_energy` 回调并同步 commit；
   总电量设置在 IDIS 尚未实现，落地时必须同样先 commit 再对外报成功。
7. stategrid 的 EEPROM endurance、最大持续负载和 `EC_SaveDiff` 语义；决定
   snapshot 的 2 kWh 突然丢失窗口是否可接受。
8. journal 的端子级窗口：ISR/second-task drain 最多 1 秒/64 个脉冲。若产品
   要求端子级 1 脉冲，必须改 drain 路径，不只是换存储引擎。

## 11. 测试计划与证明目标

全部是 host 测试，`tests/CMakeLists.txt` 编译。断言的是消费者可观察的行为：
replay 计数、写入次数、重启后的恢复值。

| 文件 | 用例 | 证明的 rubric 项 |
| --- | --- | --- |
| `tests/unit/test_pulse_store.c` | 首次启动 EMPTY、lane/方向拆分、费率分段、跨重启的半字节、FULL 后 commit 重试、granularity 1/8、base 容量、参数校验、lane 编码 | 消费端口契约正确；granularity 1 的 1 脉冲边界；candidate 3 端口嫁接 |
| `tests/unit/test_pulse_store_crash.c` | 对固定场景的每个 program/erase 做 fail-before 和 fail-after 扫描，逐个重启；`R + 1 >= accepted` 且 `R <= attempted`；无故障恢复全部 | 崩溃矩阵的掉电边界：最多丢 1 个已接受脉冲，0 double count |
| `formal/tla/EnergyPulse.tla` | 每次写可撕裂；blank/torn base、verify 失败 epoch、marker 未消费与同 slot 重试；不变量 `gen = BestGen`、`NO_BASE` 仅全 blank、pending 时 bit lane 不追加 | 崩溃与运行期设备错误的协议不变量 |
| `tests/unit/test_pulse_store_wear.c` | 计数 block 跑 1,000,000 脉冲：16 次 commit、64 个扇区 erase、1,000,096 次 program | 第 7 节的磨损数字（erase 由 wrap 主导，base 与 marker 的额外写可数） |
| `tests/unit/test_pulse_store_marker.c` | 重启后 slot 0 字节不变、同费率不写 marker、每个 marker slot 后重启仍正确 | synthesis must-fix：`restore` 恢复 `marker_count` 和当前 tariff |
| `tests/unit/test_pulse_store_faults.c` | 单 blank+单撕裂、双撕裂均为故障而全 blank 为 EMPTY；base 读错误返回 `IO_ERROR` 且下一次读到真实 base；commit 写成功但回读失败后 epoch 前移且后续脉冲不丢；marker 未写入不消耗 slot；已落盘未验证的 marker 不错归旧费率脉冲 | 运行期设备错误不冒充正常状态；读错误与撕裂 region 分开；无脉冲丢失、无费率错归 |
| `tests/unit/test_pulse_store_snapshot.c` | snapshot 空白/容量、399 个脉冲在上次 commit 200 后重启丢 199、同样场景 journal 丢 0、两者差值 199 | fallback 的 199 脉冲丢失窗口显式化；journal 为 1 |
| `tests/unit/test_pulse_store_properties.c` | lane 前两字节 0..255 共 512 次重启；3 个 marker 的 16 字节各翻一个 bit，共 384 个样本 | 恢复计数等于连续清零前缀；CRC 失败 slot 被跳过，replay 总数不变且不重复（TLA+ 不变量的实现侧） |
| `tests/unit/test_pulse_store_snapshot.c` | 双 slot 选最高 generation、最新 slot 撕裂回退、唯一 slot 撕裂为故障、外区记录为故障、单次 commit 写 3 段共 188 B、snapshot 与 journal 的 199 脉冲窗口差 | 双 slot 协议、CRC 门禁、每次 commit 的 EEPROM 写计数 |
| `tests/unit/test_energy_snapshot_adoption.c` | EMPTY 按 0x5AA5 标志选 EC1/EC2、标志已清优先 EC2、EC1 撕裂回退 EC2、空白来源从零、双 slot 撕裂后显式恢复 generation 1、EC2 energy 半记录有效而 month 项撕裂时仍采纳 energy、EC1 读失败且 EC2 不能补全两半时返回故障、commit 往返并递增 generation、最新撕裂回退、base 扫读瞬断不采纳 legacy 且不写下一代、错误消失后重试读到真实 base、EC2 energy 半记录瞬断重试一次、首次启动持续失败时不回退 EC1 并保持故障、撕裂时持续失败仍恢复 mirror 但返回 degraded | 消费者侧 adoption、半记录独立采纳与有界重试、torn 恢复、读错误不可采纳、generation 排序 |
| `targets/stategrid_meter/tests/unit/test_metering_energy_{retry,power_cut,clear_fault,month_clear,fault_latch,storage_fault}.c` | 首次恢复读失败后重试且不提交零 book；窗口内 40 步脉冲在恢复后只加一次（5→45）且窗口内不 export 到 mbase；窗口内掉电只恢复 durable book（5）；窗口内清零被拒；commit 失败时 clear/set_total 返回 -1 且旧快照不变；月度清零后下一 pulse 从零累计、重启读回清零值；存储故障锁存到上报后才清；单 slot 持续读故障保持 fail-closed 并上报 storage_fault；首次启动 EC2 energy 持续读故障保持 restore_pending 直到介质恢复；撕裂恢复回退 mirror 时上报 storage_fault | 适配器的恢复/持久化契约、pending 重放基线、故障上报、无静默回退 |
| `targets/idis_ip_meter/tests/unit/test_idis_energy_commit.c` | 288 个 300 s 窗口各 1 个脉冲并保存 = 0 次 erase，重启恢复全部；journal wrap 时恰好 1 次 commit（4 sector erase）；掉电有 append 才 commit；清零即使 book 等于 base 也 commit | journal 是持久化机制；periodic save 不写，commit 只由容量和非脉冲变更驱动 |
| `targets/idis_ip_meter/tests/unit/test_idis_energy_pulse_rate.c` | `INT32_MAX`/`INT32_MIN` 功率被拒绝且 fraction 不变；41 kW 在 2000/3200 常数下为 22/36 脉冲；分数进位与 NULL 参数 | 磨损修复不能让饱和标定变成每脉冲一次 flash program |
| `targets/idis_ip_meter/tests/unit/test_idis_register.c` | 能量寄存器清零调用 `save_energy` 一次，清零成功才报成功；回调失败返回错误 | 非脉冲变更先 commit 再报成功 |
| `targets/idis_ip_meter/tests/acceptance/test_idis_acceptance.py` | `idis.boot-rejects-bad-energy-base`：book 长度不匹配的 base 让 host 启动失败，而不是运行零 book | 启动错误不被吞掉；坏 store 不静默清零 |
| `targets/stategrid_meter/tests/acceptance/test_energy_snapshot.py` | ACC-STO-007..016：29 步丢失窗口、30 步提交不重复计数、撕裂回退、双撕裂后显式恢复、EC2 迁移来源、结算日月度清零跨秒/重启、EC2 半记录撕裂、645/698 清零与 645 底度设置 commit 失败返回错误应答且重启值不变、恢复 pending 时清零不覆盖旧电能记录 | 产品可观察的丢失窗口、回退、迁移来源、清零与底度设置路径、故障上报 |
| `formal/tla/Snapshot.tla` | 每次 header/blob/CRC 写都可撕裂；Crash/Resume；Inv：恢复代次相邻、值不混、已有 commit 后必回退到上一代 | 双 slot 提交/恢复协议 |
| `formal/lean/BladeFormal/Snapshot.lean` | encode/decode 双射、172 B 字段跨度、checksum 封印/拒绝/篡改检测 | 172 B base codec 与 CRC 门禁 |
| `tools/check_components.py`、`tools/check_layers.py` | CI 门禁 | 端口在 domain、引擎在 infra、绑定在 infra/drivers，include 方向正确 |

`PowerOff_Save_data` 的 pending 分支在 `nd_get_offset` 读失败时跳过整块镜像写；
继续写块会把零 RAM book 覆盖到 durable 镜像。firmware_sim 只注入 EEPROM 写故障，
没有读故障缝隙，该分支不可验收，靠代码审查保持。

验证证据（E1 收工）：

- `build/e1` ctest 138/138 通过；其中 pulse 用例 5 个文件、20 个 Unity case。
- `build/e1-strict`（`-Wall -Wextra -Werror -Wshadow ...` + ASan/UBSan）构建
  通过，pulse 用例 5/5 通过。
- `check_components.py`、`check_layers.py`、`check_component_standard.py`、
  `check_platform_boundary.py`、`check_drivers.py`、
  `check_domain_contracts.py`、`check_mutable_globals.py`、
  `check_externs.py`、`check_dead_code.py` 全绿。

验证证据（E2a 收工）：

- host ctest 138/138 通过；`test_idis_energy_store` 5 个 Unity case 覆盖
  base 往返、legacy adoption、journal 重放、FULL 后 commit 重试、无 CRC 旧
  记录不静默接受。
- IDIS 验收 12 条全过，新增 `idis.energy-journal-power-cut`：130 s 无最终
  保存后重启，`energy_raw` 从 20650 恢复到 20675，0 丢失。
- ARM ROM（本机 Zephyr 14.3.0）：裸机 95355 -> 98896 B；FreeRTOS IRQ
  102184 -> 102232 B。
- ARM ROM（pinned GNU 14.3.rel1）：裸机 98088 -> 101628 B，余 3844 B；
  FreeRTOS IRQ 104916 -> 104964 B（+48 B），余 508 B；FreeRTOS poll
  102276 B，余 3196 B。
- `check_idis_twins.py`、`check-arm.sh`、`check_idis_firmware.py`、
  `check_layers.py`、`check_components.py`、`check_component_standard.py`、
  `check_mutable_globals.py`、`check_domain_contracts.py` 全绿。

验证证据（F1 收工）：

- `./tools/check_formal.sh` 为 `FORMAL_EXIT:0`、`formal: ok`：
  `formal/cbmc/pulse_headers.c` 直接编译 `pulse_store.c`，
  `--nondet-static --unwind 16` 下 1,714 checks、0 failed；
  `EnergyPulse.tla` 在 cfg 界内 TLC 生成 13,948,710 states、1,764,080 distinct、
  depth 29、无错误；`Pulse.lean` 与 `Fifo.lean` 的 `lake build` 成功并实际产出
  `.olean`。
- host ctest 139/139；strict ASan/UBSan 下 6 个 pulse 测试文件全过。
- 修复了一个验证基础设施 bug：`formal/lean/lakefile.lean` 的 `lean_lib` 没有
  globs，`lake build` 实际不编译任何模块，Lean 门禁此前是空的；现用
  `@[default_target]` + `.submodules` 注册，`Fifo.lean` 的证明同步修正。
- 盲区：TLA+ 只覆盖崩溃路径，设备报错后继续运行的路径由 crash 故障注入测试
  覆盖；矩阵第 14 行（journal 读错误截断扫描）和第 16 行（snapshot 整记录撕裂）
  没有直接运行时测试。

验证证据（E2b 收工，stategrid 双 slot snapshot）：

- region 证明：`firmware_sim` 启动 trace
  `[sim] eeprom-used=44810 snapshot-region=49152 margin=4342`，即 allocator
  高水位 `0xAF0A`、snapshot 区 `0xC000..0xC177`，余 4,342 B，远低于
  `BOARD_EEPROM_SIZE` 0xFFFF 和 registry 约定 `0xD800`。启动 guard 在
  `get_device_used()` 越过 `0xC000` 或旧记录偏移不再是 `0x450D` 时直接
  fail boot。
- host ctest 146/146 通过；strict ASan/UBSan 144/144 通过；新增
  `test_energy_snapshot_adoption` 5 个 Unity case、`test_pulse_store_snapshot`
  9 个 case（含单次 commit 3 段 188 B 写计数）。
- stategrid 验收 98 条全过，新增 ACC-STO-007/008/009：
  - 29 步未 commit 后直接掉电，重启读回 0；30 步跨阈值 commit，重启读回
    3000，再走 1 步为 3100，无重复计数。
  - 最新 slot blob 撕裂，重启回退上一代（200→100 的上一代）。
  - 双 slot 撕裂，读回 0，旧 172 B 记录字节不变（不被采纳）。
  - ACC-STO-001、ACC-POWER-001、ACC-MET-008 同时断言旧镜像与 snapshot。
- formal：`./tools/check_formal.sh` 为 `formal: ok`。`Snapshot.tla` 在
  MaxGen=3 下 TLC 生成 543 states / 160 distinct、无错误；把 slot 选择改成
  固定 slot 0 后模型报 `Inv is violated`，证明 fallback invariant 有效。
  `BladeFormal/Snapshot.lean` 编译通过：encode/decode 双射、172 B 字段跨度、
  checksum 封印/拒绝/篡改检测。
- GNU 链接实测（`check-firmware-link.py --product
  meter_electricity_single_phase_cn`，pinned GNU 14.3.rel1，Cortex-M0）：
  `.text` 202,960 -> 205,288 B（+2,328），`.data` 584 -> 584（0），
  `.bss` 30,156 -> 30,172（+16）；链接对象 229 -> 234；ELF 335,376 B。
- 门禁：`check-legacy.sh`、`check-persistence.sh`、`check-layers.sh`、
  `check_components.py`、`check_component_standard.py`、
  `check_mutable_globals.py`（基线只改行号）、`check_dead_code.py`、
  `check_acceptance_matrix.py`、`check_product_component_usage.py`（stategrid
  部分）、`check_param_tables.py`、`check_ram_budget_counts.py` 全绿。
- 盲区：IAR 实编译和 HIL 掉电保持时间未验证；EEPROM endurance、最大持续
  负载和 `EC_SaveDiff` 语义仍是产品输入。

验证证据（磨损、启动与脉冲速率修复）：

- `test_idis_energy_commit`（5 个 Unity case）：288 个 300 s 窗口各追加 1 个
  脉冲并保存 = 0 次 erase、重启后 288 个脉冲全部恢复；第 63,361 个脉冲在
  FULL 时 commit 1 次（16,384 B，4 个 sector）；掉电有 append 时 commit 1
  次；寄存器清零即使 book 已等于 base 也 commit 1 次。修复前同一 288 窗口
  用例测到 288 次 erase。
- `test_idis_energy_pulse_rate`（3 个 case）：`INT32_MAX`/`INT32_MIN` 被拒绝
  且 fraction 不变；41 kW 在 2000/3200 常数下为 22/36 脉冲；分数进位正确。
- `test_idis_register`（12 个 case，新增 1 个）：能量寄存器清零调用
  `save_energy` 一次；回调失败返回 `ND_ERR_OPERATION_NOT_POSSIBLE`。
- 验收 `idis.boot-rejects-bad-energy-base`：修复前 host 以零 book 启动并输出
  `energy_raw=20025`；修复后 exit 1 且 stderr 为
  `cannot load the energy book`。
- `./ci.sh`（pinned GNU 14.3.rel1，`FREERTOS_KERNEL_DIR` 指向外部 checkout）：
  host ctest 148/148、strict 146/146、sim-san 2/2；IDIS 验收 13 个 case x
  4 个配置全 `verdict=clean`；EEPROM 损坏采样 128 个 offset 全过。
- ROM（`check_idis_firmware.py` ELF，pinned GNU 14.3.rel1）：裸机
  95,132 -> 95,480 B（+348），FreeRTOS 99,288 -> 99,640 B（+352）；
  103 KiB 断言余量 9,992 / 5,832 B。
- 盲区：IAR 实编译和 HIL 掉电保持时间未验证；41 kW 上限来自 §7 的磨损模型，
  不是产品配置的 Imax，产品确认后改 `ND_IDIS_ENERGY_MAX_POWER_W`。

验证证据（E3 修正：半记录采纳、restore_pending 与清零失败上报）：

- 先红后绿（host ctest，修复前实测）：
  - `test_energy_snapshot_adoption::test_torn_ec2_month_keeps_the_ec2_energy_half`
    `Expected 5 Was 1`（EC2 energy 有效 5.00 被 month 项撕裂一起丢弃）。
  - 同文件 `test_torn_store_recovers_the_legacy_mirror_once`
    `Expected 0 Was -1`（双 slot 撕裂永久拒绝持久化）。
  - `test_metering_energy_retry::test_failed_restore_is_retried_and_pulses_apply_once`
    `Expected 0 Was 40`（pending 窗口把零 book + 40 步导出到 mbase）。
  - `test_metering_energy_fault_latch::test_storage_fault_stays_latched_after_a_later_success`
    `Expected 1 Was 0`（同一秒内的成功清掉了未上报的故障）。
  - 验收 ACC-STO-009/012 在旧固件上分别报
    `the damaged region must recover the mirror` 和
    `the valid EC2 energy half must survive the torn month half`；
    ACC-STO-013/014 因旧固件没有 `eeprom on|off` 场景命令而 exit 2。
- 修复后：host ctest 全绿；stategrid 验收 103 条 `verdict=clean`
  （新增 ACC-STO-012/013/014，ACC-STO-009 改为显式恢复语义）。
- pending 脉冲的持久性口径（见 §9）：snapshot 的 `append` 不落盘，窗口内
  脉冲只存在 RAM；掉电会丢窗口，恢复后按“恢复出的 book”为基线一次性重放，
  不会与任何 durable 记录双计。`test_metering_energy_power_cut` 证明掉电后
  只读回 durable 的 5 步，不是 5+40。

验证证据（R1/M5：base 读错误与 EC2 半记录重试）：

- 先红后绿（host ctest，修复前实测，clean 树）：
  - `test_energy_snapshot_adoption::test_transient_base_read_error_never_adopts_legacy`
    `Expected 4294967295 Was 2`（slot 1 header 瞬断后仍采纳旧值并写成 slot 0 的
    generation 2）。
  - 同文件 `test_base_read_error_retries_and_loads_the_real_base`
    `Expected -1 Was 0`（首次恢复成功采纳了旧值，错误消失后没有重试语义）。
  - 同文件 `test_transient_ec2_energy_read_failure_is_retried`
    `Expected 5 Was 2`（EC2 energy 半记录只读一次，回退到陈旧 EC1）。
  - 同文件 `test_persistent_ec2_energy_read_failure_stays_bounded`
    `Expected 2 Was 1`（失败半记录没有重试；R4 改为
    `..._fails_closed`，首次启动不再回退 EC1）。
- `test_pulse_store_faults::test_base_read_error_is_io_not_torn`：`base()` 返回
  `ND_PULSE_IO_ERROR`，下一次读返回 `ND_PULSE_OK` 和真实 base。
- 修复后：host ctest 156/156；strict ASan/UBSan 154/154；sim-san 2/2；
  stategrid 验收 103 条 `verdict=clean`；IDIS 两个 target 各 14 条、各 4 个
  lane 全 `verdict=clean`；EEPROM 损坏采样 128 个 offset 全过。
- `./tools/check_formal.sh` 为 `formal: ok`：EnergyPulse.tla 2,422,465 states /
  448,327 distinct、depth 28；Snapshot.tla 543 / 160、depth 18。端口状态码不在
  TLA 模型里，读错误语义由上面的 host 用例覆盖。
- GNU 14.3.0（Zephyr SDK 1.0.1，Cortex-M0）IRQ lane ELF 实测：裸机
  97,756 -> 97,768 B（+12，103 KiB 余 7,704）；FreeRTOS 102,120 -> 102,136 B
  （+16，余 3,336）。链接断言全过。
- 门禁：`check-layers.sh`、`check_components.py`、`check:conversion`、
  `check:dead-code`、`check:mutable-globals`、`check:public-structs`、
  `check:doc-evidence`、`check:acceptance-matrix`、`check:idis-twins` 全绿。

验证证据（R4：contract 对齐与故障可见性）：

- 契约：`pulse_store.h` 和本文把 snapshot 与 journal 分开。snapshot 的显式
  commit 可以用 generation 1 覆盖撕裂区；journal 的 commit 需要已知有效 base，
  base 扫描读错误或双 slot 非空但无效都返回 `ERROR`（双 slot 全空是 `EMPTY`，
  snapshot 仍写 generation 1）。commit 状态码随引擎不同：
  snapshot 读错误返回 `IO_ERROR`，journal 返回 `ERROR`（§2.1、§4.2、§4.3、
  §5.2）。
- 先红后绿（host ctest，修复前实测，clean 树）：
  - `test_energy_snapshot_adoption::test_persistent_ec2_energy_read_failure_fails_closed`
    `Expected -1 Was 0`（首次启动 EC2 energy 持续读失败仍采纳旧 EC1）。
  - `test_metering_energy_storage_fault::test_persistent_ec2_energy_read_fault_stays_pending_until_recovered`
    `Expected 0 Was 2`（适配器静默采纳 EC1 的 2 步）。
- 修复后行为：
  - 首次启动（`EMPTY`）EC2 energy 持续读失败：restore 返回 -1，不采纳、不
    commit，`restore_pending` 保持，适配器置 `storage_fault`；介质恢复后重试
    读到 EC2 的 5 步。
  - 撕裂 snapshot（`ERROR`）：仍用 EC1 mirror 恢复并 commit generation 1，
    restore 返回 1，适配器置 `storage_fault`（总电量可能退回上次掉电值）。
  - 单 slot 持续读故障且另一个 slot 有效：扫描失败，不自动回退；meter 读 0、
    clear/set 返回 -1、`restore_pending` 保持、`storage_fault` 置位，介质
    恢复后才读到 valid slot。
- 修复后验证：host ctest 157/157；strict ASan/UBSan 155/155；sim-san 2/2；
  stategrid 验收 105 条 `verdict=clean`；IDIS 两个 target 各 15 条、各 4 个
  lane 全 `verdict=clean`；EEPROM 损坏采样通过。新增
  `test_metering_energy_storage_fault` 3 个 case，
  `test_energy_snapshot_adoption` 增至 14 个 case。
- ELF（GNU 14.3.0 Zephyr SDK，Cortex-M0，`-Os`，
  `check-firmware-link.py --product meter_electricity_single_phase_cn --keep`
  后 `arm-zephyr-eabi-size -A`）：stategrid `.text` 202,599 -> 202,691（+92），
  `.data` 64 -> 64，`.bss` 30,000 -> 30,000，ROM 202,727 -> 202,819（+92）。
  IDIS 的编译源文件不在本次 diff 内（`pulse_store.h` 只有注释变化），CI
  实测 ROM：裸机 poll/IRQ 95,472 / 97,956 B，FreeRTOS poll/IRQ
  99,640 / 102,328 B，103 KiB 链接断言全过。
- 门禁与构建：`./ci.sh` 全绿；`check-layers.sh`、`check_components.py`、
  `check:conversion`、`check:dead-code`、`check:mutable-globals`、
  `check:public-structs`、`check:doc-evidence`、`check:acceptance-matrix`、
  `check:idis-twins`、`check:product-link` 全绿。
