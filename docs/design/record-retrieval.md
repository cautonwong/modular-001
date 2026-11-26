# 记录型数据的检索（策略、实现与门禁）

状态：P-a/P-b/P-c 已实现；P-e 形式化在跑。本文是决定记录，三种策略各自
单页见 `docs/design/record-strategies/`。

## 1. 问题与基线

IDIS 的 COSEM profile GET 在 1152 行 PQ 上要扫整环。修改前的成本（`nd_store_t`
的 read 调用数，一次调用是一次 MX25 片选窗口）：

| 查询 | 修改前 | 今天（P-a+P-c） | 窗口开启（P-b） |
| --- | --- | --- | --- |
| 全量 GET | 2304 | 1152 | 36 次扇区填充 |
| value-range GET，M=N | 6912 | 2304 | 72 次扇区填充 |
| value-range GET，M=1 | 4610 | 2304 | 72 次扇区填充 |
| entry-range GET，k 行 | 2k | k | 触达的扇区数 |

- P-a：每条记录一次 128 B 整槽读，替代 20 B 头加 payload 的两次读。
- P-c：write 趟匹配时读到的记录直接用于编码，value-range GET 从 3N 降到 2N。
- P-b：`nd_sector_store` 的读透窗口，复用它已有的 4 KiB 扇区缓冲；一次对齐
  填充服务 32 条记录。

## 2. 选择

- **IDIS 默认：cache-first（P-b），加 P-a/P-c 两个前置。** 理由：成本由块事务数
  主导；窗口把一次全扫变成扇区数的操作，+8 B RAM、无新缓冲、不改持久化字节、
  不改 append 路径。
- **stategrid 默认：mbase 直接源（现有的 log manager + mark search），同步单帧。**
  698 的成本是 CPU 和 A-XDR，不是 SPI；现有路径已经只读一次窗口。多帧续传等
  `GET-NEXT` 实现后再上 budgeted cursor。
- **fallback：线性扫描。** 它保留为真实实现，不是注释。

## 3. 门禁与未决

- **窗口在固件里默认关闭。** 开启条件：板上实测 `t(4 KiB) < 8 * t(148 B)`。
  若总线按字节计费，窗口与整槽读字节数相同，收益消失；此时整槽读仍然是默认。
  绑定点在 `board_flash_init`（`nd_sector_store_init` 的 `enable_window`）。
  IDIS host runner 的组合根（`targets/host/idis_host_main.c`）开启窗口，供端到端
  测试使用，固件组合根不变。
- **FreeRTOS 开启前必须有 store 级串行化。** save worker（APP+1）与查询路径
  共用 `flash_sector` 缓冲和 SPI 总线；现在没有锁。锁端口由 target 组合根提供，
  infra 不依赖 RTOS。当前窗口关闭，所以风险未生效。
- **整槽读把短记录多读最多 108 B**（固定读 `slot_size`），用字节换事务；同样受
  总线测量约束。
- **ROM**：P-a/P-c/P-b 的差值（−112 B、+48 B、+176..192 B）只是那次测量的
  记录。当前镜像的绝对占用和 103 KiB 余量不在本文复制：CI 的
  `<target>:firmware` 步骤运行 `tools/check_idis_firmware.py` 并打印 ELF
  section 实测值，以那份输出为准。
- **并发两趟的计数一致性（已修）**：原实现 count 趟和 write 趟是两次独立扫描，
  两趟之间 append 会让数组头计数与行数/行集不一致，reset 则会让 header 计数
  存活而 emit 为空。现在 `nd_ic_profile_generic_get` 在 parse_selection 之前
  取一次 `nd_profile_store_snapshot`（head/entries/next_sequence 的 RAM 快照），
  parse_selection 与两趟扫描都只用快照值，不再重读活 entries。每次读由快照
  计算物理槽，并校验记录头 sequence 等于
  `snapshot.next_sequence - snapshot.entries + index`。快照之后 append 若
  没有触达快照窗口，响应与快照完全一致；若满了环并覆盖旧槽，sequence 不
  匹配，GET 返回 operation-not-possible，客户端重试即可。append 在 head
  扇区对齐时先擦整个 head 扇区再写记录，擦除会让该扇区落在快照读窗口内的
  槽变成空槽，不要求满环；reset 在 head 写一条 reset mark 并推进 sequence，
  head 扇区对齐时同样先擦整个 head 扇区，`read_at` 按非记录 kind 拒绝
  空槽。
  reset mark 或擦除产生的空槽在快照读窗口之外时，快照内的旧记录仍物理
  存在，读到的仍是快照时刻的一致视图；在窗口内时，当前趟尚未读到的槽令
  GET 返回 operation-not-possible，当前趟已发行的槽不会被重读，响应保持
  快照。
  value-range 选择时两趟都会重读每个槽，所以 count 趟读过不保护 emit 趟，
  只有 emit 趟已经越过的槽才不受影响。非对齐 head 的 mark 走 sector store 的
  read-modify-erase-write：它重写整个扇区缓冲，其他槽字节不变；只有写窗口
  内掉电会丢数据，顺序一致性模型不覆盖掉电路径。窗口内的槽被后续 append
  复用同样由 sequence 拒绝。快照只占 RAM，不增加 store 读
  （P-a/P-c 的读计数不变）。TLA+ `formal/tla/ProfileGet.tla` 的 SnapshotSpec
  证明 `CountEqualsEmitted`；Reset 在模型里写 head 槽（mark 不可读），head
  对齐时擦除整个扇区。Append 按实现建模：head 对齐时先擦整个扇区再写记录，
  该扇区其余槽置空。cfg 用每扇区 2 槽的实例让擦除覆盖 head 之外的槽；旧写法
  只写 head 槽，会留下已被擦除却仍可读的槽；临时探针（未入库）能在旧写法上复现“GET 完成
  时发行了已擦除记录”的假完成，新模型无此路径。窗口外、窗口内当前趟未读槽、
  当前趟已发行槽三种情况都在证明范围内。旧的两趟并发 spec 保留为反例文档
  （header 1、append 一条匹配记录、emit 2 行）。
  第二个 actor（save worker、查询 worker 或 block session）现在可以与
  **attribute-2 GET** 以及 class-7 methods 3/4（`get_buffer_by_range` /
  `get_buffer_by_index`）重叠而不产生不一致响应。methods 3/4 进入动作时
  取一次同样的 `nd_profile_store_snapshot`：数组头与每一行都从快照计算，
  `get_buffer_by_index` 的上界用 `snapshot.entries`，行读走
  `nd_profile_store_read_at`，不再按行读实时 entries。没有新增 TLA+ 模型：
  快照之后越界、覆盖或擦空快照窗口的 append 由 `read_at` 拒绝（空槽或
  sequence 不匹配），即 `SnapshotSpec` 的读拒绝不变量，动作路径直接复用；
  被拒时返回 operation-not-possible，客户端重试。

## 4. 698 的映射与取舍

- 选择器：记录索引、时间区间、数据项、collection/report 组合、多帧。
- 后端：`mbase_control_block_t` 内存映射，`log_manager_t` + `search_mid_way`
  负责 mark 查找，行从 `(read_ptr + i) % span` 直接读；成本在 CPU 和编码。
- **本次不改 698 的代码。** 没有测到的 IO 症状；`get_3_Event.c` 1416 行等
  legacy 路径没有干净接缝；为单一实现加端口是投机抽象。策略文档记录了将来的
  cursor 接法，等 `GET-NEXT` 续传落地再实现。stategrid 验收 92 条继续覆盖
  现有行为。运行期上界在 log manager 的 `nd_storage_read` 边界计数，形式化在
  `BladeFormal/MbaseSearch.lean`；两者都绕开 GET 的 legacy 编码层。

## 5. 可替换性落在哪

- `nd_store_t`（block 端口）是扇区窗口与其后备读的接缝，组合根在
  `board_flash_init` 选择开关。
- `nd_profile_store_read` 是记录级访问器，窗口开或关都不改调用方。
- COSEM 与 698 的消费者各自持有所需的选择语义；没有把线协议类型跨过端口。
- 查询游标（cursor-first）的端口**暂不实现**：只有一个实现时的抽象是债务，等
  延迟响应传输和第二实现出现再引入。

## 6. 验证

- 运行期：`test_profile_store_read_count`（1 次整槽读、CRC 失败关闭）、
  `test_idis_profile_path` 的计数断言（value-range 2N、全量 N、entry k、动作
  逐行 1 次）和五个并发用例（count 趟末或动作行读中注入 append：未满环时
  header 与 emitted 相等，满环覆盖快照槽时 GET 返回 operation-not-possible，
  methods 3/4 返回快照行；method 3 的行读撞上被移动的槽时返回
  operation-not-possible；按端口契约，错误结果的 writer 内容未定义，调用方
  丢弃并重新 init 后重发得到干净响应）、
  `test_sector_store_window`（1152 槽 36 次填充、写失效、跨扇区旁路）、
  `test_idis_profile_window`（128 槽整表 GET 在窗口开关下响应逐字节相同，后备读
  128 → 4 次扇区填充）、`test_mbase_search_counts`（64 槽 mbase 环按序号和时间
  选择：计数趟 15 / 19 次 mark 读，每条记录一次整槽读）。
- 验收：IDIS 两个 target 和 stategrid 的验收套件按 runner 输出逐个 case 为
  `verdict=clean`；条数以 CI 日志和 evidence 目录为准，不在本文复制。验收不新增
  GET 用例：host runner 的验收语言不含 COSEM 请求，行集与编码由
  `test_idis_profile_path` 逐字节覆盖；验收继续覆盖采集与持久化。
- 形式化（P-e）：`formal/lean/BladeFormal/ProfileRing.lean`（槽映射单射、备用
  扇区不相交、连续扫描填充界、两趟读预算）、`formal/lean/BladeFormal/MbaseSearch.lean`
  （698 mark 搜索的 `ceil(log2 n) + 1` 探测界、窗口单调收缩、记录索引
  `(read_ptr + i) % span` 单射）、`formal/cbmc/profile_slot.c`
  （编译真实的 `nd_profile_store.c`，证明槽头校验与 payload 拷贝边界）、
  `formal/tla/ProfileGet.tla`（SnapshotSpec 证明 `CountEqualsEmitted`；append
  与 reset 在 head 扇区对齐时的整扇区擦除都建模；旧的两趟并发 spec 保留为
  反例文档）；都接进 `tools/check_formal.sh`
  （Lean 由 lake 的模块 glob 枚举）。`ProfileGet.tla` 把 sequence 抽象为
  无界 Natural，实现是 16 位 modulo 65536；模型不覆盖“精确 65,536 次 append
  后旧槽 sequence 再次等于预测值”的别名情形。模型还把扇区擦除和记录写当成
  原子一步，擦除进行中的并发读、擦除/写窗口内掉电和擦除时序不在证明范围。
  两者都是已知残余。

## 7. profile 存储的 pre-v4 升级路径与启动扫描预算

- v4 布局（commit `ab9bcd9c`）移动了 22 个 region base 中的 19 个。pre-v4
  镜像仍留在旧 base；`nd_profile_store_init` 在 v4 ring 为空时探测冻结的旧
  base，表在 `targets/idis_ip_meter/cosem/nd_idis_profile_table.c` 的
  `legacy_base_offsets`（以及 FreeRTOS twin），是只读参考数据，不改持久化
  字节。探测只在 v4 ring 为空时运行；已经写有 v4 数据的 region 按 v4 处理，
  不再回扫旧 base，避免把 v4 记录误判成旧镜像。代价是 buggy build 跑过、
  已经写入 v4 数据的混合状态不会被识别。
- 探测到镜像返回 `ND_PROFILE_STORE_LEGACY_IMAGE`，store 进入 hold：append
  拒绝写入。组合根（两个 target 的 `app/main.c`）显式选择 fail closed
  （`reset_legacy = 0`）：保留旧镜像，不自动 reset、不自动迁移，全部
  profile 和 event log 仍然注册。任一 profile 探测到旧镜像时 hold 全部
  profile，因为 v4 region 与旧 base 有重叠（例如旧 PQ_LOG 记录落在新 COMM
  region 内），只 hold 单个 profile 挡不住别的 ring 覆盖旧数据。
- hold 是表级状态，释放也必须是表级动作。class-7 method 1 通过
  `nd_idis_profiles_reset` 复位单个 profile；任一 store 仍处于 hold 时直接
  返回 operation-not-possible，不写 mark、不擦扇区。原因是 per-store 释放
  会把该 store 的 v4 ring 打开，而 ring 的扇区可能覆盖别的 store 的旧
  base：BILLING region `[0, 8192)` 包含旧 LOAD_PROFILE_1 元数据（4096），
  只释放 Billing 后它的下一次扇区擦除就会毁掉仍被保护的 LP1 镜像。
- 维护释放入口（R3-B/C）：**不是 COSEM 方法**。class-7 method 1（reset）按 DLMS
  定义没有参数，合规前置端会拒绝携带 RELG 的 ACTION；因此确认值不进线上
  APDU，method 1 本身恢复为纯 reset。本地工厂工具校验操作员输入的 A-XDR
  double-long-unsigned `0x52454C47`（ASCII "RELG"）后，直接调用
  `nd_cosem_port_release_legacy(objects, sap, confirmation)`。入口要求
  management subject（SAP 1）且 `remote_client == 0`，与
  `nd_cosem_port.c` 的 local-only 脚本表门禁同一写法：pre-established
  （SAP 102）、public（16）、CIP（103）和远程关联一律 access-denied。
  SAP 1 是 management 的证据在 `nd_cosem_port.c` 的 `access_allowed`、
  `test_idis_access.c` 的 `test_management_sap_can_act` 和
  `docs/idis/idis-pack3-plan.md` 第 5.2 节的关联表。无 hold 时是 no-op，
  不碰任何 ring，也不写 marker。
- 外部触发（R3-A）：本地光口是出厂固件上的可达传输。`board_uart_init`
  初始化 UART 通道 2（IR，1200 baud，8E1），ISR 把字节写进 `board_ports.c`
  的 rx fifo；组合根每秒 drain 一次，只有 IR 通道被绑定，RS485 和 module
  通道不喂维护解析器，物理本地口就是第一道门禁。
- 帧格式（固定 8 字节）：`0x68 | length=5 | command=1 | confirmation
  (4 字节 big-endian) | XOR(length..confirmation)`。长度、命令或 checksum
  不符就丢弃；噪声之后遇到下一个 `0x68` 重新同步。帧里的 confirmation
  原样交给 `nd_cosem_port_release_legacy`，错误值由入口的确认值门禁拒绝。
  解析器是 `components/services/maintenance`（services 层，无依赖、无全局
  状态，状态在组合根 aggregate 里）。
- 帧内超时（R5）：组合根每轮 drain 之后调用 `nd_maintenance_abort` 丢掉
  未完成的半帧，所以一次 drain 就是 inter-byte timeout。上一秒收到的合法
  前缀（例如 `68 05 01 52 45`）不会被下一秒的无关字节
  （`4C 47 18`）补全。1200 baud 8E1（每字节 11 bit）下一个 8 字节帧约 73.3 ms，
  远小于 1 s；只有跨 drain 边界到达的帧会被丢弃，操作员重发即可。
- FIFO 溢出的 fail-closed 行为（R5，显式记录，不新增状态）：IR rx fifo 为
  128 B，`fifoBuf_putByte` 在满时覆盖最旧字节。1200 baud 下 128 B 约
  1.17 s，而单个合法帧约 73.3 ms，正常帧不会触发溢出。持续噪声导致溢出时
  只会丢字节；长度、命令或 XOR 校验失败后解析器丢弃该帧并在下一个 `0x68`
  重同步。所以溢出最多让一次 release 丢失，不会让半帧或乱序字节被接受；
  当前没有溢出计数或事件，这是有意保留的 fail-closed 行为。
- 可达性是确定性门禁：`tools/check_idis_firmware.py` 除符号表外，还要求
  `board_ir_read` 和 `nd_maintenance_feed` 在 `main`、`idis_app_tick` 或
  `on_second` 里有直接 BL。`on_second` 当前被内联进前两者；允许它作为
  调用者，编译器选择不内联时门禁不会误报。门禁还要求
  `nd_cosem_port_release_legacy` 从 `maintenance_release`、`main` 或
  `idis_app_tick` 被直接调用。只有符号存在不再算通过。
- ROM（Arm GNU 14.3.Rel1，HEAD 04003461 对比本次）：baremetal poll/irq
  各 +336 B，FreeRTOS poll/irq 各 +320 B，RAM 各 +8 B（parser 状态）。
  R3-B 的 release marker 另加 +120/+112 B ROM、+16 B RAM；最终数字以 CI
  `<target>:firmware` 的 `tools/check_idis_firmware.py` 输出为准。
- 释放动作是 operator-confirmed 的破坏性操作，把 22 个 profile ring 全部
  reset：每个 ring 的 entries 清零、写一条 reset mark、清掉 hold，ring 内
  已写入的 v4 记录随 reset 丢弃。reset 只擦当前 head 扇区，pre-v4 旧字节
  不会立即擦除；reset mark 让下次启动不再探测旧 base，旧字节从此不受保护，
  会被后续 ring 覆盖。标准 event log 和其他 event log 也是 profile，条目
  一并清零；EEPROM 里的费率、日历、能量账本和参数不受影响。任一 ring
  reset 失败时把全部 store 重新 hold 并返回 operation-not-possible，避免
  半释放状态下已释放的 ring 擦掉仍被保护的旧镜像；操作员可重试。
- 22 环循环的 watchdog（R5）：`on_second` 只在入口 feed 一次，而 release
  的 22 次 ring reset 每次都可能擦一个扇区（典型 45 ms，datasheet 上限
  400 ms），累计可达 1.1-1.6 s 甚至约 9 s，接近或超过 2 s 看门狗。release
  现在在每次 `nd_profile_store_reset` 之后调用组合根注入的同一个 watchdog
  feed（`profiles->watchdog`，与启动扫描的回调相同），所以相邻两次 feed
  之间最多一个 ring 的擦除加写入。`test_release_feeds_watchdog_between_rings`
  断言 feed 次数不少于 ring 数，且相邻 feed 之间最多 2 次 store 事务。
  设备侧最坏擦除时间仍待 HIL。
- 释放进度必须跨重启保存（R3-B）：第一个 ring reset 之后，前 N 个 ring
  已写 reset mark，旧 base 探测被关闭；若后面的 ring 失败，RAM 里的 re-hold
  在重启后会丢失，掉电等于静默完成破坏性释放。因此在第一个 ring reset
  之前，先在一个专用 flash marker 扇区写入一条 ACTIVE 记录；22 个 ring
  全部完成后擦除该扇区。启动时扫描 marker，任何 ACTIVE 记录都 hold 全部
  store（并把 `reset_legacy = 1` 的自动 reset 压住），组合根照常 raise
  event 12、host runner 打印 `legacy_held=1`，操作员必须重试。marker 在
  ACTIVE 期间只追加不擦除：失败重试先追加一条 ACTIVE，再执行 reset，最后
  擦除扇区，所以重试本身不会抹掉失败状态。写入或擦除失败都 re-hold 全部
  store 并返回 operation-not-possible；boot 读 marker 失败让 init 返回 -1，
  与其它 store 读错误同样 fail closed。
- marker 的擦除账（R5 更正）：512 个 8 B slot 限制的是**同一次 release 内**
  的失败重试次数，不是成功 release 的次数。每次成功 release 都擦除 marker
  扇区两次：激活时若没有 ACTIVE 记录先擦一次（有 ACTIVE 则只追加），完成
  后清除时再擦一次。另外 22 个 ring 各自 reset 都会写一条 reset mark：
  `nd_sector_store` 的写是读-改-写，目标 slot 非空（内容不同且不是 0xFF）时
  先擦整个扇区再回写，slot 为空时只 program；head 落在扇区边界时 reset 还会
  先显式擦一次 head 扇区（随后 mark 写的是空 slot，不再触发 RMW erase）。
  所以每个 ring reset 最多擦一个扇区，但不是只在扇区边界才擦。失败重试每次
  追加一条 ACTIVE
  记录并消耗一个 slot；slot 用尽后激活返回错误并保持全部 store held。因此
  marker 的 erase 次数随成功 release 次数线性增长，**不能**按“一次 append
  对应一次 release、512 次 release 擦一次”估算寿命。
- marker 的持久化布局：flash 偏移 `0x9F000` 起 4 KiB（
  `ND_IDIS_RELEASE_MARKER_OFFSET/BYTES`），位于 640 KiB profile flash 预算
  的最后一个扇区。最后一个 profile region 结束于 `0x9B000`，`0x9B000` 到
  `0x9F000` 的 16 KiB 仍是空闲；`nd_idis_profile_table.c` 的
  `_Static_assert` 把“最后一个 region 在 marker 之前”和“marker 在 640 KiB
  之内”钉死，任何 region 增长都会编译失败。EEPROM 布局不变：IDIS 的 demand
  maxima 在 `EEPROM_MAX_DEMAND`（0x0C00），与 marker 无关；stategrid 的
  36 B `persistent_blank_space`（28 B 已被 demand maxima 占用）属于另一个
  产品和另一个介质，这里不碰。
- 迁移尚未实现；init 的 `reset_legacy = 1` 策略保留在 boot gate 里，产品
  组合根仍用 `reset_legacy = 0`，不自动 reset；marker ACTIVE 时该策略也被
  压住，boot 不替操作员完成半途的释放。
- hold 可观测：组合根在绑定 event 后检查 `nd_idis_profiles_legacy_held`，
  有 hold 就 raise event 12（program memory error，profile status 置
  ERR|DNV）。event 状态（`EEPROM_EVENT`）保存 latest code 和计数；标准
  event log profile 本身处于 hold，append 被拒，所以日志条目数为 0 是预期
  现象，不是未注册。host runner 另外打印 `legacy_held=0/1`，验收直接断言
  释放前后的状态。
- 设备错误与无效槽内容分开：recover 或 legacy 探测期间任何 store read 失败
  都让 init 返回 -1。瞬时 SPI 错误不再被当作"空 store"，否则 head 会越过
  活记录，或首次 append 擦掉正在使用的扇区。启动必须把 init 失败当成失败：
  `nd_idis_profile_table_boot` 只在 22 个 profile 全部注册时返回 0；组合根
  返回非零退出 `main`，看门狗随后重启并重试扫描。继续运行会得到
  `profiles->count == 0`，所有 class-7 对象未定义、两个 event log 为空。
- 启动扫描（空白 flash 的最坏情况，host 实测）：9339 次 store 事务 /
  716,580 字节 = 4960 次 128 B 整槽读 + 22 次 32 B legacy 元数据读 +
  3845 次 20 B legacy 头读 + 512 次 8 B release marker 扫描。每次 store
  read 后调用组合根注入的 watchdog feed，最大间隔 128 B。按 brief 的
  5 us/B bit-bang 模型，无 feed 时 3.583 s，超过 2 s 看门狗；有 feed 时
  最大间隔 0.640 ms。host 侧整次扫描约 0.17 ms（byte store），它只证明
  事务数和 feed 覆盖。**设备侧真实启动时间仍需 HIL 实测**，host 数字不是
  设备数字。
- `test_boot_scan_feeds_watchdog_within_budget` 断言每个 store read 都有
  feed、最大间隔不超过 `PROFILE_SLOT_BYTES`，并打印事务数、字节数和 host
  时间。

## 8. 验证补充

- `test_profile_store`：legacy 镜像 hold 到 reset；recover 期间的读错误
  全失败和中途失败都让 init 返回 -1。
- `test_idis_profile_table`：旧 PQ base（167936）的镜像在 v4 表初始化时被
  探测到，retain 策略下所有 profile 仍注册且 append 被拒；reset 策略解除；
  空白 flash 的整表启动扫描的 feed 计数与间隔预算。新增：
  `test_reset_refused_while_legacy_image_is_held` 让 Billing 的 head 停在
  4096 扇区，断言 class-7 method 1 被拒且旧 LP1 元数据逐字节不变；
  `test_reset_failure_is_reported` 让擦除失败，断言 method 1 返回
  operation-not-possible；`test_boot_refuses_zero_profiles` 用全失败读的
  store 断言 boot gate 返回 -1；`test_legacy_hold_is_observable` 断言
  `nd_idis_profiles_legacy_held` 从 0 变 1。R3 新增：
  `test_release_legacy_resumes_appends` 在 4096 放旧镜像后断言 public、
  pre-established 和 CIP SAP 返回 access-denied、错误确认值返回
  access-denied 且 hold 不变、management SAP 1 加正确确认值返回 OK、hold
  清零、append 恢复、重新 init 后仍不重新 hold；
  `test_release_without_hold_keeps_profile_records` 断言无 hold 时释放是
  no-op，已有 billing 记录不被删除；`test_release_resets_every_profile_store`
  先写入一条 billing 记录，再由别处的旧镜像触发 hold，断言释放把这条非
  legacy ring 的记录也清掉；`test_release_failure_reholds_every_store` 让
  第一次 ring 擦除失败，断言返回 operation-not-possible 且全部 store 重新
  hold，重试后释放成功。R4 新增：
  `test_release_failure_at_last_ring_survives_reboot` 让第 22 个 ring 的
  擦除失败，重启后仍断言 `legacy_held=1` 且 append 被拒；
  `test_release_failure_mid_ring_survives_reboot` 对第 11 个 ring 做同样
  断言（修复前两者重启后都是 0）；`test_release_refused_for_remote_management`
  断言 management SAP 1 加 `remote_client=1` 被拒；
  `test_profile_method1_cannot_release` 断言携带 RELG 的 class-7 method 1
  不释放整表、hold 不变。R5 新增：
  `test_release_feeds_watchdog_between_rings` 断言 22 次 ring reset 之间都有
  feed（feed 数不少于 ring 数，相邻 feed 最多 2 次 store 事务）；
  `test_marker_full_refuses_release` 填满 512 条 ACTIVE 记录，断言 release
  返回 -1 且全部 store 保持 held；`test_torn_marker_record_is_not_active`
  断言不完整 ACTIVE 记录（第二个字不是补码）本身不触发 hold，真实旧镜像
  释放时该扇区被擦除；`test_failed_clear_scan_reports_after_release_complete`
  让清除后的证明扫描读失败，断言 release 返回 -1、22 个 ring 已清空、
  marker 已空白，重试幂等成功。最后一条固定了“release 可能已经完成却仍
  报告失败并重新 hold”的语义：操作员重试即可，不会重复破坏其它数据。
- 验收 `idis.legacy-profile-hold`（`targets/idis_ip_meter/tests/acceptance`）：
  在 `flash.bin` 的 4096 写入 pre-v4 LOAD_PROFILE_1 元数据，host runner 连续
  启动两次，断言 `profiles=22`、`event=12`（hold 可观测）、`legacy_held=1`、
  `billing=0` 和 `event_log=0`（append 被拒），并逐字节核对旧元数据未被
  覆盖。
- 验收 `idis.legacy-release`：同一个旧镜像，第一次启动确认 hold
  （`legacy_held=1`、`event=12`、`billing=0`、`event_log=0`）；第二次启动带
  `--release-legacy 0x52454C47`，runner 作为本地 front end 以 management
  SAP 1 调用 `nd_cosem_port_release_legacy`，断言 `maintenance=0`、
  `legacy_held=0`，并且 `billing` 和 `event_log` 都增长（release -> records
  resume）；第三次启动不带释放参数，断言 `legacy_held=0` 且记录仍在，证明
  释放跨重启保持。
  **未覆盖**：host 的 file store 不能注入启动扫描或 ring 擦除的故障，“释放
  失败跨重启 re-hold”只能在 `test_idis_profile_table` 用 fault store 验证；
  target 的 `main.c` 调用点和看门狗复位路径需要 ARM 编译加真机，host 不
  代替。host runner 走 `nd_idis_profiles_init`（configs），target 走
  `nd_idis_profile_table_init`，两者共用 marker 与 hold 逻辑，但扫描错误
  注入只在 unit gate 上覆盖。host runner 的维护入口就是本地工厂入口；普通
  method 1 在 hold 期间被拒仍由
  `test_reset_refused_while_legacy_image_is_held` 覆盖，不在验收里重复。
- 验收 `idis.legacy-uart-release`：同一个旧镜像，第一次启动确认
  `legacy_held=1` 且 append 被拒；第二次用 `--uart-input` 喂入 checksum
  错误的帧，断言 `maintenance=-1`、`legacy_held=1`、`billing=0`；第三次
  喂入正确帧 `68050152454C4718`，断言 `maintenance=0`、`legacy_held=0`，
  `billing` 和 `event_log` 增长；第四次启动断言释放跨重启保持。
  `test_maintenance` 覆盖帧接受、checksum 拒绝、半帧在 drain 结束时被
  `nd_maintenance_abort` 丢弃（后半帧不能补全）、abort 后新帧可重新开始、
  噪声后重同步和消费者失败。host 侧证明的是解析器和 object-model 路径；
  target 的 IR fifo/ISR 传输、光层丢字节和每秒 drain 的边界仍需 HIL 证据。

## 9. 威胁模型与 HIL 待测项（R5 记录，不改代码）

**门禁链。** 本地 IR 光口是唯一物理入口；`nd_cosem_port_release_legacy`
要求 management subject（SAP 1）且 `remote_client == 0`；确认值
`0x52454C47`（ASCII "RELG"）是公开常量，不是密钥。安全性来自物理本地
访问加 management 关联，不来自确认值的保密。

**前端契约（固件无法代替）。** 任何网络关联必须在 `nd_idis_objects_t`
里设置 `remote_client = 1`，并且任何前端都**不能**把
`nd_cosem_port_release_legacy` 暴露成 COSEM method。今天产品代码没有
任何地方设置 `remote_client`（字段保持 0），所以一个忘记设置的网络前端
会被入口当作本地客户端；这是前端集成契约的缺口，固件没有可用的关联
身份来补齐它。HIL/集成验收必须用一条远程 management SAP 1 的 release
尝试证明 access-denied。

**HIL 清单（没有硬件不改阈值）。**
- MX25L3206E `wait_ready` 固定 `ND_MX_STATUS_POLLS = 2000` 次轮询，必须
  覆盖 45-400 ms 的 sector erase 上限；ring reset、marker activate/clear
  和 ring append 的扇区擦除都走它。先测轮询周期与 400 ms 的余量，**不要
  在没有硬件证据时改这个阈值**。
- IR ISR/光接收层：1200 baud 8E1、128 B FIFO、每秒 drain、满时覆盖最旧
  字节。host 只证明解析器和 abort 策略，ISR 优先级、光层丢字节和边界
  时序要 HIL。
- release 时长：22 个 ring reset 加 marker activate/clear 两次扇区擦除；
  按典型 45 ms/erase 约 1.1-1.6 s，按 datasheet 上限 400 ms 约 9 s。本轮
  已让每个 ring reset 后 feed watchdog，但 2 s WDT 下最坏时序要 HIL 确认。
- WDT 时序：`on_second` 入口 feed 一次；R5 之前 22 环循环中间没有 feed。
  HIL 要测最坏擦除时间、相邻 feed 间隔和真实复位行为。
- `remote_client` 集成：任何网络关联路径必须置 1；用远程 management
  SAP 1 的 release 尝试验证。
