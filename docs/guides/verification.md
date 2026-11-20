# 验证与约束

目标：把能机器检查的约束写进代码和 CI，让后续功能工作只剩“各就各位、填空”。
每一层都给出可重跑的证据，不依赖人的记性。

## 1. 编译期约束

- `tools/check-arm.sh` 对 `domain/`、`application/` 用
  `-Wall -Wextra -Werror -Wmissing-prototypes -Wshadow -Wundef
  -Wstrict-prototypes` 按 Cortex-M0 交叉编译。
- `host:strict` 对共享主机代码再加 `-Wshadow -Wundef -Wstrict-prototypes
  -Wvla -Wformat=2 -Wnull-dereference -Wduplicated-cond -Wlogical-op
  -Wjump-misses-init`，测试夹具额外关闭只会误报的
  `-Wmissing-prototypes`、`-Wmissing-braces`、
  `-Wmissing-field-initializers`。
- `tools/check-legacy.sh` 保留 `-w` 压厂商噪声，但用
  `-Werror=implicit-function-declaration` 把隐式声明变成错误，并编译
  `stategrid_meter_entry.c` 等旧源文件。
- 数据形状用 `_Static_assert` 钉死，放在类型定义旁边：
  - `targets/idis_ip_meter/cosem/nd_idis_registry.h`：注册表条目、RAM 镜像、
    EEPROM 地址空间。
  - `targets/idis_ip_meter/cosem/nd_idis_profile_table.h`：表项数不超过聚合容量。
  - `domain/common/allocator.h`：`ND_ALLOC_ALIGNMENT`；每个通过分配器获得的
    聚合在自己的 `.c` 里断言 `_Alignof` 不超过该值。
  - `targets/stategrid_meter/persistence/legacy_energy_layout.h`：掉电保存区
    偏移不变。
- 固件 ROM 预算由 `build/idis.ld` 的链接断言和
  `tools/check_idis_firmware.py` 的 ROM load span 共同守住。`rom=` 是
  `.isr_vector`、`.text`、`.ARM.exidx` 和 `.data` 的 LMA 跨度，含 section
  间的对齐填充，等于链接产物的 `.bin` 大小，不是 section 大小之和。CI 矩阵的
  `[target:irq-rom] rom=` 仍按 section 大小之和打印，比 load span 小 1–2 B；
  以 checker 的 load span 为准。

### 加一条新约束

1. 能编译期判断的，写成 `_Static_assert`，消息写清“为什么”。
2. 只能运行期判断的，在最早的可信边界检查并返回错误码；不要静默截断。
3. 约束涉及硬件布局时，同时更新对应检查脚本和验收用例。

## 2. 静态分析

`host:analyzer` 用 GCC `-fanalyzer` 对生产库做路径敏感、单编译单元内的跨函数分析：

```sh
cmake -S tests -B build/ci-analyzer -G Ninja -DBLADE_BUILD_SIM=OFF \
  "-DCMAKE_C_FLAGS=-Wall -Wextra -Werror -fanalyzer"
cmake --build build/ci-analyzer --target \
  blade_domain blade_components blade_idis_skeleton blade_infra \
  blade_target_stategrid blade_application blade_target_host
```

测试夹具不参与这一层：fake 端口的空指针/未初始化告警是噪声。生产库当前零告警，
所以它是硬门禁。

`tools/check_param_tables.py` 用编译后的 `firmware_sim` 符号大小校验
`mapper_698.c` 的 `parameter_item[]`。直写行要求 `len` 不超过数组大小；长度前缀行
要求首字节不超过数组大小减一。CI 的 `check:param-tables` 在 host build 之后运行，
自检用 `--self-test`。

`host:conversion` 用 `-Wconversion -Wsign-conversion -Wcast-qual
-Wdouble-promotion -Wfloat-conversion` 构建生产库，不启用 `-Werror`。
`tools/check_conversion.py` 按文件和告警类别计数，对比
`tools/conversion_baseline.txt`。新增类别或计数上升会失败；修掉告警后用
`--update` 下调基线。

`tools/check_dead_code.py` 按文件统计 `#if 0` 块，对比
`tools/dead_code_baseline.txt`。新增块或计数上升会失败；删掉死代码后用
`--update` 下调基线。

## 3. 运行时验证

`host:strict` 用 AddressSanitizer 和 UndefinedBehaviorSanitizer 重跑全部单元测试，
`-fno-sanitize-recover=all`，任何越界、未定义行为都会让 CI 失败。它已经抓到过：

- `nd_demand_t` 要求 8 字节对齐，而测试用的 fake 分配器只对齐到 4 字节；
- `nd_arena_init` 现在把基址对齐到 `ND_ALLOC_ALIGNMENT`，并覆盖错位基址耗尽
  容量的边界；
- `nd_ic_mbus_client_set` 的内层 `channel` 遮蔽外层变量；
- `nd_ic_association` 的 `generate_certificate_request` 把未初始化的栈缓冲当公钥；
- `components/core/fifo/fifo.c` 的 `fifoBuf_getByteCount` 从未写输出参数，且无人调用。

`host:sim-san` 用 ASan 和 UBSan 重跑 `firmware_sim`、stategrid 验收矩阵，以及两个
IDIS 验收套件（用同一个 sanitizer 版 `idis_host_runner`）。它关闭 alignment 检查：
host 指针是 8 字节，legacy 布局按 4 字节假设，对齐告警描述的是仿真器而不是目标。
它已经抓到：

- `mapper_698.c` 把 `131 << 24` 当有符号 `int` 移位；
- `nd_calendar_total_day` 和 `GetTotalDay` 用未校验的月份索引 13 项表；
- `resource_tool.c` 按 160 字节请求读 128 字节栈缓冲；
- 事件 OAD 列表、分钟冻结深度、备用时区表的初始数组短于参数块字段，
  `para_init` 越界读相邻 rodata；
- `GET_f_Other` 解引用空的 698 上下文；
- `nd_idis_single_action_load` 接受持久化记录里超过 8 个时段的 `time_count`；
- `ctl_11_read.c` 用 `unsigned long` 接收 4 字节 DI，LP64 host 上高 4 字节未初始化。

## 4. 有界穷举与性质测试

对状态空间小、输入长度有界的纯函数，用穷举代替抽样。当前覆盖：

- `tests/unit/test_axdr.c`
  - 所有 65536 个两字节输入：`nd_axdr_skip_value` 和 `nd_axdr_get_length`
    不越界，`pos <= len`。
  - 恰好 16 层嵌套通过，超过 16 层被拒绝（防栈递归）。
  - 0x83 三字节长度超过 `UINT16_MAX` 时拒绝，不静默截断。
- `tests/unit/test_calendar.c`
  - 2000-2099 的每一天：`total_day` 单调递增且 `from_total_day` 往返一致。
  - 记录两位年份公式在 2100 年回绕的既有约束。
- `targets/idis_ip_meter/tests/unit/test_idis_registry.c`
  - 有效 blob 的每个字节位置取遍 0..255，除原值外全部被校验和/格式检查拒绝。
- `targets/idis_ip_meter/tests/unit/test_idis_single_action.c`
  - 持久化记录的 `time_count` 取遍 0..8 通过，9..255 全部拒绝。
- `tests/unit/test_bitarithm.c`
  - 全部 16 位输入与参考实现逐位比较 `msb`、`lsb` 和置位数。
- `tests/unit/test_pulse_store_properties.c`
  - unary lane 的两个字节位置取遍 0..255：重启恢复的计数等于连续清零位，
    第一个置位之后不再计数（TLA+ 前缀不变量的实现侧）。
  - 三个 marker 的 16 字节各取一个 bit 翻转（384 个样本）：CRC 失败的 slot
    被跳过，replay 总数保持 6 且不重复计数。
- `targets/idis_ip_meter/tests/unit/test_profile_store.c`
  - 元数据前 32 字节逐字节取遍 0..255，初始化、条目数和索引读取在任意损坏下不越界。
- `targets/idis_ip_meter/tests/unit/test_idis_profile_window.c`
  - 128 条记录的整表 GET 在窗口关闭和开启下返回逐字节相同的响应；后备读从
    128 次整槽读降到 4 次扇区填充，即触达的扇区数。
- `targets/stategrid_meter/tests/unit/test_mbase_search_counts.c`
  - 64 slot 的 mbase 环按序号（mode 1）和时间（mode 2）各查一次。计数趟只付
    mark 搜索，实测 15 / 19 次 4 字节 mark 读，上界 30（2 次 smoke 加每趟最多
    7 个窗口、每窗口一次 mid 加一次边界读）；每条记录再付一次整槽读。
- `tools/fuzz_idis_eeprom.py`
  - 用 sanitizer 版 host runner 播种 EEPROM，按 512 字节步长翻转一个字节，
    128 个样本必须退出 0。这是确定性采样，不是随机 fuzz。

### 加一个有界穷举测试

- 输入空间要能算清上界，写进测试名或注释。
- 循环里只断言不变量，不要依赖随机数。
- 在 `host:strict` 下跑，让 sanitizer 兜住越界。

## 4.1 形式化检查

`tools/check_formal.sh` 跑四组检查。本地缺工具时跳过，GitHub workflow 安装后
用 `--strict` 运行。Frama-C 从 `FRAMA_C` 或 PATH 查找；Lean 装在 `~/.elan/bin`
时脚本自己把它加进 PATH；TLA+ 的 jar 从 `TLA2TOOLS`、
`formal/tla/tla2tools.jar` 或 `/tmp/tla2tools.jar` 依次查找，不提交 jar。
每个 CBMC stage 钉住 property 数，每条 Eva harness 钉住语句覆盖数和
Assertions/Preconditions 总数，所以计数口径必须稳定：CI 用 CBMC 5.12（jammy
包，下载时校验 sha256）和 Frama-C 33.0，版本不符即失败。

| 工具 | 方法 | 当前覆盖 |
| --- | --- | --- |
| CBMC | 有界模型检验，按 harness 声明的路径和展开深度枚举 | A-XDR、脉冲 header、profile slot、两个 store |
| Frama-C Eva | 抽象解释，无外部 prover，小输入空间用状态分裂加循环展开枚举 | 两个 store 的请求范围、内存安全、窗口状态和设备错误路径 |
| Frama-C WP | 演绎验证，需要 why3 和外部 prover | 当前不可作门禁：bundle 带 why3 运行时（`lib/why3/why3.cmxs`），但没有外部 prover（Alt-Ergo/Z3/CVC5），`frama-c -wp` 只能用内置 Qed 证平凡目标，store 契约无法验证 |
| TLA+ | 显式状态模型检验 | block transfer、脉冲 journal、profile GET |
| Lean | 交互式定理证明（算术模型） | FIFO、脉冲、profile ring、mbase |

- `formal/cbmc/axdr_length.c`：四个符号字节输入，CBMC 证明长度解码不越界，
  0x81/0x82/0x83 的消耗字节数正确，保留形式被拒绝。
- `formal/cbmc/axdr_skip.c`：符号化的定长标签和缓冲区长度，CBMC 证明完整值
  消耗固定字节数，截断值返回错误，位置始终在缓冲区内。
- `formal/cbmc/pulse_headers.c`：符号化的存储后端和代际号（`--nondet-static`），
  CBMC 证明 base 记录和 journal header 的解析不越界，magic、version、长度或
  代际不匹配时不接受，接受时字段与 CRC 一致。
- `formal/cbmc/profile_slot.c`：直接编译 `nd_profile_store.c`，`--nondet-static`
  让每个 slot 的 magic、version、length 和存储 CRC 九个决策字节符号化。CBMC
  证明 `read_slot` 和 `nd_profile_store_read` 拒绝错误版本、`1..slot_size-20`
  之外的长度、未知 magic 和 CRC 不匹配；接受记录的 payload 不超过
  `slot_size - 20` 和调用者 capacity，payload 拷贝与存储字节一致，store 读取
  不越界，窗口外索引在触碰 store 之前被拒绝。
- `formal/cbmc/byte_store.c`：直接编译 `byte_store.c`，8 字节后端和请求符号化。
  CBMC 证明越界读写在触碰目标或后端之前返回 -1，in-range 读只写 length 字节
  且返回存储字节，in-range 写只改目标范围。
- `formal/cbmc/sector_store.c`：直接编译 `sector_store.c`，8 字节设备、2 个
  4 字节扇区，后端和请求符号化。设备按 NOR 语义建模：擦除置 0xFF，program 只清位
  （`memory &= buffer`），所以删除空白检查或读改写分支后，重写非空扇区会被证明
  失败。CBMC 证明越界的读、写和越界或未对齐的擦除返回 -1 且不改后端；窗口读
  （填窗和第二次命中）与直接读一致；写保留范围外字节，含跨扇区和重写扇区；擦除
  只把请求的扇区置 0xFF；写和擦除都使窗口失效。设备读、擦、写各有一个始终失败的
  回调；窗口填充和直接读、擦除、写各分支的错误返回都被断言为 -1，擦除失败还
  断言后端不变。
- `formal/frama-c/byte_store_eva.c`：8 字节后端，`Frama_C_interval_split` 让每个
  offset/length 成为独立状态，`-eva-slevel 512 -eva-min-loop-unroll 8` 保持状态
  和循环索引精确。Eva 证明越界读写返回 -1 且不动目标或后端，in-range 读只写
  length 字节且返回存储字节，in-range 写只改请求范围。字节内容取固定模式：
  值域无法证明两个非单值抽象值相等，符号内容的证明由 CBMC harness 承担。
- `formal/frama-c/sector_store_eva.c`：8 字节 NOR、2 个 4 字节扇区，program 只
  清位（`backing &= buffer`），窗口开启，设备回调和确定性故障注入。Eva 证明
  0 alarm、所有逻辑性质成立，覆盖越界读写的拒绝、越界和未对齐擦除的拒绝、窗口
  填窗与命中、跨扇区读改写、擦除循环、设备错误路径（擦除失败的结果用断言检查），
  以及 `sector_size` 为 0 和 `sector_buffer` 为 NULL 的 init 拒绝；擦除后的窗口
  失效用断言检查，写后的窗口失效由 CBMC harness 证明（memcmp 的结果对值域不
  透明）。5 条语句保持未覆盖：三个 store 函数里的 `store->sector == NULL` 守卫
  子句（init 失败会清零设备半区，前面的回调 NULL 守卫先命中），以及 init 对
  非零非 2 的幂 `sector_size` 和 capacity 不是整扇区这两个拒绝分支。门禁钉住
  语句覆盖数和 Assertions/Preconditions 总数，删除检查调用会失败。
- `formal/tla/BlockTransfer.tla`：block transfer 状态机，TLC 检查已服务块是
  `1..next-1`、不重复、完成后关闭、新 NORMAL GET 从块 1 重新开始。
- `formal/tla/EnergyPulse.tla`：能量脉冲 journal 的崩溃协议。覆盖
  NO_BASE/INVALID/ACTIVE/FAULT、每方向 unary lane、marker 时间线、base 双 slot
  与 generation、journal header 代际。TLC 在 2 方向、每方向 3 bit、2 个 marker
  slot、granularity 2、2 代 base、每方向 4 个脉冲的界内穷举，检查恢复不重复计数、
  每次 commit 窗口内已接受丢失不超过 granularity、journal 只在代际匹配时计入、
  base generation 单调、marker 时间线有序、恢复的 bit 是连续前缀。
- `formal/tla/ProfileGet.tla`：profile GET 的两趟（`count_rows` 再
  `write_rows`）。在 6 slot、每 sector 2 slot 的 ring 上，append、reset 和两趟
  扫描交错，TLC 检查 `CountEqualsEmitted`（header 是已发行行数的上界，结束时
  相等）和 `GridStable`（每趟只读自己的 live window 快照）。文件显式建模了
  pass 之间的并发 append（`ConcurrentSpec`）：它违反 `CountEqualsEmitted`，
  因为当前两趟没有快照，`write_rows` 重新读取 `entries`，会发行比 count 多的行。
- `formal/lean/BladeFormal/Fifo.lean`：FIFO 容量算术模型，证明
  `used + free = capacity`（`used <= capacity` 时）和满时空闲为 0。
- `formal/lean/BladeFormal/Pulse.lean`：脉冲引擎的整数算术。证明 unary lane 的
  扫描只数连续清零位、扫描停在第一个置位、在 cursor 写位严格推进扫描；
  pending 缓冲严格小于 granularity，所以 `accepted - recovered <= granularity`；
  以及 wrap 擦除次数 `(pulses + capacity - 1) / capacity` 覆盖脉冲数且是最小值。
- `formal/lean/BladeFormal/ProfileRing.lean`：profile ring 和两趟 GET 的算术。
  证明 `nd_profile_store_read` 的环索引到物理 slot 映射在 live window 上单射，
  `entries + sector_slots <= phys` 时 append 前的 spare sector 擦除不碰 live
  记录；连续扫描 128 字节 slot 的填充数是 `ceil(records * 128 / 4096)`，1152 行
  最坏 37 个 sector；value-range GET 是每 entry 两次读（count 一趟加 write
  一趟），与 `test_profile_value_range_get_read_counts` 的 3 行 6 次读一致。
- `formal/lean/BladeFormal/MbaseSearch.lean`：stategrid 698 mbase 的 mark 搜索
  和记录索引。`SearchStep` 建模 `search_mid_way` 的继续探测：窗口两端只向内
  移动，一次探测至少减半，所以 n 个 slot 的窗口最多探测
  `ceil(log2 n) + 1` 次（`markProbe_bound`）；64 slot 的实例是 7 次。`recordIndex_injective`
  复用 `add_mod_injective`，证明 `(read_ptr + i) % span` 在 `i < span` 上单射。

边界：CBMC 只证明 harness 声明的输入长度和展开深度；Eva 只覆盖 harness 枚举的
offset/length 和具体字节内容，且 `-eva-min-loop-unroll` 的界与 harness 的固定
容量绑定；TLA+ 是协议模型，不是 C
实现，且 `EnergyPulse` 用 3 bit 的抽象 lane（小于一个字节），C 的 byte-end
flush 不参与；granularity ≤ 容量时两边的 flush 规则都是 pending ≥
granularity；Lean 是算术模型，尚未做 C 细化。脉冲模型不覆盖“设备报错后引擎
继续运行”的路径，那条路径由 `tests/unit/test_pulse_store_crash.c` 的故障注入
覆盖。`ProfileGet` 的并发 append 缺口是实现缺口，不是模型缺口：pass 之间没有
快照，`ConcurrentSpec` 的 TLC 反例是 header 与已发行行数不等的真实行为，修实现
时需要同一快照或 pass 间校验；`profile_slot.c` 的 payload 内容取固定模式，
越界只由长度和 capacity 决定，符号化 108 字节 payload 会让 CRC 公式不可解。
`byte_store.c` 和 `sector_store.c` 的 CBMC proof 用 8 字节后端；sector harness
固定 2 个 4 字节扇区，`nd_sector_store_init` 在运行期检查扇区大小非零且为 2 的
幂、capacity 为整扇区倍数、扇区缓冲非空，非法几何会把设备半边清零并让端口调用
返回 -1；host 用例覆盖接受和四种拒绝。Eva 的两个 harness 用同一几何，sector
harness 另执行一次零扇区大小的拒绝调用；byte harness 的字节内容取固定模式，
sector harness 只在具体字节上断言窗口值，符号内容仍由 CBMC 证明。官方 Frama-C
bundle 带 why3 运行时（`lib/why3/why3.cmxs`），但没有外部 prover
（Alt-Ergo/Z3/CVC5），所以 `-wp` 只剩内置 Qed 能证平凡目标，store 契约无法验证，
暂不进 CI；能量累加的 WP 证明留到外部 prover 就位后。负例证据在
`formal/frama-c/negative/`：放宽 byte 写保护、删除擦除的窗口失效各留一份带
Frama-C 版本头的输出，验证器分别以 `[eva:alarm]` 行和 invalid 断言拒绝；同目录
的 10-alarm fixture 在门禁启动时自检，证明 0 alarm 计数是锚定的。
`MbaseSearch` 是搜索协议模型，`test_mbase_search_counts` 在
`nd_storage_read` 边界计数；两者都不穿过 `GET_5_Frozen`/`GET_3_Event` 的
legacy 编码路径，那条路径没有干净接缝。

## 5. 当前边界

- 所有验证是 host GCC + Cortex-M0 交叉编译 + 链接检查；没有 IAR 实编译、
  真机、HIL 或现场抓包。硬件在环的五项测量按
  [硬件在环测量清单](hil-measurement.md) 执行；测量结果补进对应产品的记录，
  不能从仿真推导。
- `host:analyzer` 只覆盖生产库；`host:strict` 覆盖单元测试，不含
  `firmware_sim`；整机仿真和 IDIS host runner 由 `host:sim-san` 用 ASan 和
  UBSan 覆盖。
- 穷举只证明声明的界内行为；界外的规格问题仍需要产品确认，例如厂商 double
  精度、IDIS EEPROM 实际容量。
