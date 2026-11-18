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
  `tools/check_idis_firmware.py` 的 ELF section 实测共同守住。

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
- `targets/idis_ip_meter/tests/unit/test_profile_store.c`
  - 元数据前 32 字节逐字节取遍 0..255，初始化、条目数和索引读取在任意损坏下不越界。
- `tools/fuzz_idis_eeprom.py`
  - 用 sanitizer 版 host runner 播种 EEPROM，按 512 字节步长翻转一个字节，
    128 个样本必须退出 0。这是确定性采样，不是随机 fuzz。

### 加一个有界穷举测试

- 输入空间要能算清上界，写进测试名或注释。
- 循环里只断言不变量，不要依赖随机数。
- 在 `host:strict` 下跑，让 sanitizer 兜住越界。

## 4.1 形式化检查

`tools/check_formal.sh` 跑三组检查。本地缺工具时跳过，GitHub workflow 安装后
用 `--strict` 运行。

- `formal/cbmc/axdr_length.c`：四个符号字节输入，CBMC 证明长度解码不越界，
  0x81/0x82/0x83 的消耗字节数正确，保留形式被拒绝。
- `formal/cbmc/axdr_skip.c`：符号化的定长标签和缓冲区长度，CBMC 证明完整值
  消耗固定字节数，截断值返回错误，位置始终在缓冲区内。
- `formal/tla/BlockTransfer.tla`：block transfer 状态机，TLC 检查已服务块是
  `1..next-1`、不重复、完成后关闭、新 NORMAL GET 从块 1 重新开始。
- `formal/lean/BladeFormal/Fifo.lean`：FIFO 容量算术模型，证明
  `used + free = capacity`（`used <= capacity` 时）和满时空闲为 0。

边界：CBMC 只证明 harness 声明的输入长度和展开深度；TLA+ 是协议模型，不是 C
实现；Lean 是算术模型，尚未做 C 细化。

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
