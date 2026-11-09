# Blade DDD / Clean Architecture 交接文档

更新于 2026-11-09。新会话从这里接着干。

## 1. 目标

把 Blade 嵌入式电表工程重构为四层 DDD / Clean Architecture。

不可违背的原则：

1. user alloc。所有资源都由调用方提供，不只 RAM，还包括持久化块、队列、定时器、IO 句柄、ID 段。
2. 聚合、聚合根、有界上下文。
3. 消费者定接口。上层声明自己需要的端口，下层实现。
4. 组合根。只有 target 同时看见 domain、application、infra。
5. 不透明指针。聚合结构体定义留在 `.c`，公开头文件只有前置声明。

硬约束：存量电表的持久化字节布局必须逐字节兼容。

构建约束：目标代码用 `arm-none-eabi-gcc`，测试用宿主 `gcc` 加 Unity。

## 2. 仓库与分支

- 工作副本：`/tmp/blade-refactor`
- 工作分支：`refactor/ddd-clean-arch`
- 发布仓库：`https://github.com/cautonwong/modular-001`，`main` 分支
- 源仓库：`https://github.com/nanshan0708/Blade`，`main` 从未改动
- 当前远端 `main` 头：`9d4ed9b3`

新会话建议直接克隆发布仓库：

```bash
git clone https://github.com/cautonwong/modular-001.git
cd modular-001
```

`/tmp/blade-refactor` 是临时目录，重启可能丢失，但所有提交都已推到 `modular-001`。

## 3. 当前目录结构

```
domain/        领域层，纯 C，不依赖平台
  common/      allocator, store, parameter_store, calendar
  metering/    energy book, metering aggregate, demand, legacy/
  tariff/      tariff lookup, legacy/
  events/      duration, over_current, cover_open, power_off, generic,
               neutral_current, legacy/
  load_control/ load control aggregate, legacy/
  freeze/      legacy/
  display/     legacy/
application/   应用层
  metering/    one-second use case
  adapters/    dl698, dl645, dlms (staging)
  apps/        legacy applications (staging)
  upgrade/     legacy upgrade (staging)
  meterbase/   unbuilt legacy application files
infra/         基础设施层
  kernel/      fifo, list, bitarithm, runtime
  memory/      arena, byte_store, parameter_store
  soc/         cm0, v85xxp
  board/       eeprom, rnorflash, rn8209, relay, uart, iic, spi, esam698
  os/          baremetal
  security/    esam, crypto
  boot/        bootloader
targets/       组合根
  host/        host composition root
  stategrid_meter/ legacy entry plus persistence adapters
  msta/        msta instance
tests/         unit, support, third_party/unity, legacy
nidavellir/    legacy SDK headers and docs
docs/          architecture and this handoff
tools/         checks and probes
```

依赖方向只向内。`targets -> application -> domain <- infra`。
接口由消费者声明，infra 在组合根里实现。

## 4. 已验证的东西

- 25 个 Unity 测试二进制，全绿。
- 旧工程 ARM GCC 编译基线 `123/123`。
- 层依赖检查通过。
- 新代码 Cortex-M0 编译检查通过。
- 快速旧代码检查通过。

### 4.1 验证命令

```bash
# 宿主测试
cd tests
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure

# 全量旧工程 ARM GCC 编译基线，解析 IAR .ewp
python3 tools/legacy_baseline.py

# 快速旧代码子集
./tools/check-legacy.sh

# 层依赖规则
./tools/check-layers.sh

# 新代码 Cortex-M0 编译
./tools/check-arm.sh
```

预期输出：

- `ctest` 显示 `100% tests passed, 0 tests failed out of 25`
- `legacy_baseline.py` 显示 `sources=123 ok=123 failed=0 missing_files=0`
- 三个脚本退出码为 0

### 4.2 工具说明

- `tools/legacy_baseline.py` 解析 `targets/stategrid_meter/iar for arm/VG_DTZY.ewp`，收集源文件、include 路径和宏定义，用 ARM GCC 逐个编译，按错误归类。每次移动目录后都要跑。
- `tools/legacy_probe.c` 从旧头文件实测持久化偏移。修改旧结构体时重跑，并更新 `targets/stategrid_meter/persistence/legacy_energy_record.h` 的静态断言。
- `tools/check-legacy.sh` 是快速子集，适合频繁迭代。
- `tools/check-layers.sh` 检查 include 方向和 domain 内的直接分配。
- `tools/check-arm.sh` 检查新代码在 Cortex-M0 下可编译且不依赖宿主 libc。

## 5. 关键设计决策

### 5.1 四层

DDD 是四层。`targets` 是组合根，不是层。物理目录用 `domain / application / infra / targets`。

### 5.2 不透明聚合与 user alloc

示例：`domain/metering/metering.h` 只前置声明 `metering_state_t`。定义在 `.c`。
创建和销毁都接收 `nd_allocator_t`。domain 不调用 `malloc`。
`infra/memory/arena.c` 实现分配端口，返回清零内存，最近分配的块可以回收。

### 5.3 消费者定接口

已有端口：

- `domain/common/allocator.h`：`nd_allocator_t`
- `domain/common/store.h`：`nd_store_t`，按字节偏移读写
- `domain/common/parameter_store.h`：`nd_parameter_store_t`，按 16 位 ID 读写记录

infra 实现这些端口。target 在组合根里注入。

### 5.4 持久化兼容

电能切片是 172 字节，冻结在 `targets/stategrid_meter/persistence/legacy_energy_record.h`。
偏移由 `_Static_assert` 锁定。
实测值：active_p 0，active_n 52，active_p_dec 104，active_n_dec 130，energy_checksum 156，month_energy_dec 160，month_energy 164，use_energy_checksum 168。
该切片在旧持久化块内的起始偏移是 3。
校验和与 `reserved` 填充字由持久化层保留，codec 不覆盖。

费率表按参数 ID 解析。ID 冻结在 `legacy_tariff_ids.h`。

### 5.5 迁移暂存区

以下目录暂存未完成的 legacy 代码，检查和测试显式跳过：

- `domain/metering/legacy`
- `domain/tariff/legacy`
- `domain/events/legacy`
- `domain/freeze/legacy`
- `domain/load_control/legacy`
- `domain/display/legacy`
- `application/apps`
- `application/adapters`
- `application/upgrade`
- `application/meterbase`

跳过名单在 `tools/check_layers.py` 的 `STAGING` 和 `tools/check-arm.sh` 的 `skipped` 里。
每迁移完一个目录，就从名单里删掉。

## 6. 已完成的工作

1. 四层骨架、Unity、CMake、层检查、ARM 检查。
2. 分配端口、arena、byte store、parameter store。
3. 计量领域：电能账本、不透明聚合根、需量、字节兼容 codec 和 store。
4. 日历与费率查找，包括环形区间、节假日、周休日、日时段、费率选择。
5. 负荷控制聚合，包括命令状态机、计时器、按键、LED、误动作检测。
6. 事件去抖原语，以及过流、开盖、掉电、计量芯片故障、继电器误动作、RTC 故障、电源异常、零线电流异常。
7. 旧事件调用点全部接到新模块，旧的 `event_duration_pro` 已删除。
8. host 组合根，一个秒循环串起计量、需量、费率、负荷控制和持久化。
9. 应用层一秒用例，target 只负责装配。
10. 目录迁移：kernel、drivers、boot、baremetal、esam、crypto、upgrade、meterbase、协议包、applications、instances 全部归位。
11. 旧工程 ARM GCC 编译基线从 11 个文件扩到全工程 123 个文件。

## 7. 未完成与下一步

建议顺序：

1. 重写 `freeze` 上下文，`domain/freeze/legacy/gs_freeze.c`。
2. 收掉掉电和电源异常的下电路径，它们仍直接操作活动标志。
3. 迁移旧调用方到新聚合：计量能量、需量、费率查找、负荷控制。迁移后删除 legacy 文件和转发头。
4. 处理死文件。清单见第 9 节。
5. 把 `nidavellir/include` 的公共头归入各层，删除旧伞头 `applications.h`。
6. 给 ARM 基线补链接步骤，目前只验证到编译。
7. 把 `application/apps`、`application/adapters`、`application/upgrade` 改成真正的适配器。
8. 清理根目录失效的 solution 文件。

## 8. 已知问题与风险

1. 旧代码质量低，已知缺陷：
   - `demand.c` 滑差寄存器从不写入，平均值指针类型错误。
   - `electrical measurement.c` 与 `electrical_measurement.c` 是重复文件，前者未编译。
   - `display.c` 使用文件级 `struct instance_t*` 全局变量，多实例失效。
   - `resource.c` 的枚举定义在结构体使用之后，本身编不过。
   - `tasklet.c` 主体在 `#if 0` 里。
   - `iv/iv_data.c` 有 `mallos` 拼写错误和全角逗号。
   - crypto 缺整个 `inc/` 目录。
2. 旧注释多为 GBK 编码，改动时注意不要破坏编码。
3. 部分文件名带空格，例如 `electrical measurement.c`、`gs_power_abnormal_event .c`，脚本和命令要加引号。
4. MSVC `.vcxproj` 多数已失效，IAR `.ewp` 是当前可信构建。ARM GCC 基线是新验证手段。
5. 没有链接基线。编译通过不代表能链接成固件。
6. 持久化兼容是硬约束。任何改动旧结构体字段顺序的行为都要先跑 `tools/legacy_probe.c` 并更新静态断言。
7. `/tmp/blade-refactor` 可能丢失，发布仓库是唯一可靠副本。

## 9. 死文件清单

已确认不在 IAR 工程里，但暂时保留：

- `domain/metering/legacy/demand.c`
- `domain/metering/legacy/electrical measurement.c`
- `domain/metering/legacy/iv/iv_data.c`
- `domain/tariff/legacy/gs_month_bill.c`
- `domain/tariff/legacy/gs_step_bill.c`
- `domain/display/legacy/display.c`
- `application/meterbase/unbuilt/factory.c`
- `application/meterbase/unbuilt/parainit.c`
- `application/meterbase/unbuilt/tasklet.c`
- `application/meterbase/unbuilt/meterbase.h`

删除前先确认没有外部引用。用户对删除死文件尚未表态。

## 10. 新会话启动清单

1. 克隆或进入仓库。
2. 读 `docs/architecture.md` 和 `.audit/ddd-refactor.todo.md`。
3. 跑一遍全部验证命令，确认基线。
4. 从第 7 节第一项开始。
5. 每完成一个单元，更新 `.audit/ddd-refactor.todo.md`，提交，推送 `modular001`。

推送命令：

```bash
git push modular001 main
```

如果本地分支不是 `main`，用：

```bash
git push modular001 refactor/ddd-clean-arch:main
```

## 11. 关键提交

```
9d4ed9b3 Finish the event migration and remove event_duration_pro
f1b49ccd Route the legacy event call sites through the new domain modules
895ccf13 Rewrite the over-current, cover-open and power-off events
9b6eabd6 Extract the event debounce state machine
a4ac0b40 Split meterbase into bounded-context legacy directories
9973867f Move test packages and kernel build files out of packages
91980208 Move applications into application/apps
96e23734 Stage meterbase under domain
d30b7296 Move instances to targets
8ca68ab6 Move protocol packages into application/adapters
1af091c3 Make the strict ARM check state the legacy skip
99356b4b Move OS, security, upgrade and boot into their layers
```
