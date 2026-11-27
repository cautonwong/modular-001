# 运行预算：执行性能、功耗与唤醒、掉电安全

日期：2026-11-27
状态：三项预算有目标、测量方法和当前值；毫秒时延、休眠电流、真实保存窗口和喂狗间隔没有 HIL 证据，保持 pending。

本文定义三项预算。每项写目标、测量方法、基线、当前值和 HIL 到期日。
`tools/check_budgets.py` 校验表格，在 CI 里重跑测量，并把当前值写进日志。
预算先记录，不进硬门禁。阈值等 HIL 数据齐了再定。

## 1. 预算表

| ID | 目标 | 测量方法 | 基线 | 当前值 | HIL 到期日 |
| --- | --- | --- | --- | --- | --- |
| PERF | 输入到效果最多 1 个 pass；每 pass 14 个业务阶段各一次且顺序固定（ACC-PERF-001） | `python3 tools/check_budgets.py --sim build/firmware_sim` 跑 100 个模拟 pass 的停电上电场景，读 `[sim] stats`；pass 契约由 ACC-PERF-001 验收 | `e43b56f9`，ACC-PERF-001 通过；本单元只加测量，不改固件行为 | 100 pass；14 阶段/pass；输入到效果最多 1 pass；宿主墙钟见 CI 日志 | 2026-12-31（暂定，需产品确认） |
| POWER | 唤醒延迟最多 1 pass（上电到首个完整 pass）；休眠电流 TBD（needs product input） | sim 读 `[sim] stats wake-latency-passes`；休眠电流只能用电流表在 HIL 上测，sim 的 `cpu_sleep_manage` 是空桩 | `e43b56f9`，ACC-PERF-001 的上电与合闸复位 ≤ 1 pass | 唤醒 1 pass；休眠电流 sim 不可测，HIL pending | 2026-12-31（暂定，需产品确认） |
| FAILSAFE | 看门狗喂狗间隔小于 2 s（WDT 周期），余量待 HIL；保存窗口时间 TBD（needs product input） | sim 读 `[sim] stats power-off-entry-passes` 和 `watchdog-max-gap-passes`，另读启动行的 EEPROM margin；真实 EEPROM 写时间和喂狗间隔用 HIL | `e43b56f9`；看门狗每 pass 喂一次；掉电保存在 `PowerOff_Save_data` 里同步完成 | 掉电后 0 pass 进入低功耗；看门狗最大间隔 1 pass（1 个模拟秒）；EEPROM margin 4342 B；保存窗口和真实间隔 HIL pending | 2026-12-31（暂定，需产品确认） |

没有 HIL 证据前，三项都不算已验证。HIL 到期日是暂定值，产品确认后改。

## 2. 测量命令与原始输出

```sh
cmake -S tests -B build -G Ninja
cmake --build build --target firmware_sim
python3 tools/check_budgets.py --sim build/firmware_sim
```

checker 生成固定场景，跑 100 个模拟 pass：

```text
at 60 power off
at 80 power on
```

2026-11-27 的原始输出：

```text
[sim] eeprom-used=44810 snapshot-region=49152 margin=4342
[sim] stats cycles=100 stages=1106 stage-max=14 wake-latency-passes=1 power-off-entry-passes=0 watchdog-kicks=100 watchdog-max-gap-passes=1 stack-high-water-bytes=2568
```

字段含义：

- `cycles`：跑完的循环 pass 数。
- `stage-max`：单个 live pass 的业务阶段数，当前是 14。
- `wake-latency-passes`：`power on` 场景 pass 到首个完整 pass 之间的 pass 数。
- `power-off-entry-passes`：`power off` 场景 pass 到首个没有 live 阶段的 pass 之间的 pass 数。
- `watchdog-max-gap-passes`：相邻两次 `mcu_clear_watchdog` 之间的最大 pass 数。
- `stack-high-water-bytes`：宿主栈涂色测得的最大使用字节数，ASan 构建输出 0。

`check_budgets.py` 另记 `host-wall-ms`，只作宿主观测，不是固件时间。
CI 的 `check:budgets` 复用这份输出并把 `budget:` 行写进日志。

## 3. 逐项说明

### 3.1 PERF 代码执行性能

目标依据是 `docs/firmware-acceptance-matrix.md` 的 ACC-PERF-001 行，用例在
`targets/stategrid_meter/tests/acceptance/test_timing_budget.py`。这条 P0 用例
已经约束输入到带周期戳的效果最多 1 个 pass，每 pass 14 个阶段各一次、顺序固定。
本预算记录同一组数字，不新增期望。

sim 的循环 pass 是唯一自动时间单位。`tests/sim/sim_trace.c` 把
`instance->ms` 设成 `cycle_index * 1000`，固件自己从不推进毫秒时基
（`docs/firmware-sim.md` 的固件侧发现 1）。所以 sim 只能约束 pass 数，不能约束
毫秒。每秒任务周期数、WCET 和调度抖动要用目标上的 DWT 或逻辑分析仪，列在 HIL
到期日。

基线和当前值相同，因为本单元只加测量，不改固件行为。宿主墙钟随机器和负载变化，
只在 CI 日志里记录，不当预算值。

### 3.2 POWER 功耗与唤醒

唤醒延迟目标来自 ACC-PERF-001 的上电恢复和合闸复位断言，都是 ≤ 1 pass。
sim 实测 1 pass。这是 pass 数，不是毫秒。真实唤醒时间取决于 PMU 和时钟启动，
要 HIL 测。

休眠电流在仓库里没有 stategrid 产品数字。`docs/plans/zw6b-ftu-performance.md`
有 FTU 产品的 ≤200 µA，那是另一个产品，不能搬。sim 的 `cpu_sleep_manage` 是
空桩（`tests/sim/sim_stubs.c`，`docs/firmware-sim.md` 的覆盖矩阵），测不了电流。
目标标 TBD，等产品给数字；当前值标 HIL pending。

### 3.3 FAILSAFE 掉电安全

看门狗周期是 2 s。`infra/soc/v85xxp/v8500/v8500_for_single.c` 调
`WDT_SetPeriod(WDT_2_SECS)`，`infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc/lib_wdt.h`
定义 `WDT_2_SECS`。`targets/stategrid_meter/app/stategrid_meter_entry.c` 的
`event_tick` 每个 pass 调一次 `mcu_clear_watchdog`。sim 把这个调用计成喂狗，
测得最大间隔 1 个 pass，即 1 个模拟秒。真实最坏间隔包含 EEPROM 擦写和长上报
循环，sim 的 EEPROM 延时是空操作（`tests/sim/sim_devices.c`），所以真实余量
要 HIL 测。sim 证明的是固件每 pass 都喂狗，不是硬件时序。

保存窗口：`PowerOff_Save_data` 在 `power_tick` 里同步执行，然后
`ac_power_status` 才变成 `AC_POWER_OFF`。sim 观察到掉电当 pass 就进入低功耗，
即 0 pass。真实窗口是从掉电检测到最后一个 EEPROM 写完成，取决于 EEPROM 页写
时间和保持电容；仓库里没有 stategrid 的 hold-up 数字。目标标 TBD，等产品给
hold-up 时间。

EEPROM margin 是启动时打印的 `snapshot-region - eeprom-used`，当前 4342 B。
`stategrid_meter_entry.c` 的 `energy_region_free` 在 `eeprom_used >=
STG_ENERGY_SNAPSHOT_OFFSET` 时拒绝启动，所以 margin 必须非负。这是存储余量，
不是时间窗口；它由启动 guard 执行，不由预算阈值执行。

### 3.4 栈 high-water

`tests/sim/sim_main.c` 在跑固件前给主线程未用栈涂 `0x5a`，跑完后找最深的被覆盖
字节。当前 2568 B。这个数包含 sim 的 stdio trace 和分配器走的宿主 libc 帧，
是仿真代理，不是 Cortex-M0 的栈，不能拿来定目标栈大小。目标上的运行时栈水位
仍然缺测量，IDIS 的 `docs/design/freertos-thread-model.md` 也记录同一个缺口。
sim 记录这个基线，让更深的调用链表现为数字变化；CI 记录，不设阈值。ASan 构建
用假栈帧，输出 0，所以测量命令固定用普通 host 构建。

## 4. 不变量的执行者

| 不变量 | 执行 |
| --- | --- |
| 三项预算都在表里，五个字段非空 | `tools/check_budgets.py` 解析表格，缺行或缺字段失败 |
| 测量命令存在并输出数字 | `check:budgets` 跑 `firmware_sim`，缺字段、0 栈或未测到唤醒/掉电失败 |
| CI 日志有当前值 | `check_budgets.py` 打印 `budget:` 行，`./ci.sh` 日志保留 |
| 没有 HIL 不声称已验证 | `check_budgets.py` 拒绝当前值列里的 `verified` 和 `已验证` |
| 文档不引用临时路径 | `check:doc-evidence` |

记录值和测量值必须一致。`check_budgets.py` 比对第 2 节的 `[sim] stats` 行和本次
测量，pass 类数字不一致就失败。栈数字依赖宿主 libc，只记录不比对。

## 5. 非目标

- 不做 HIL 测量，没有硬件。
- 不改调度器和固件行为。
- 不设硬门禁。预算先记录；阈值等 HIL 数据齐了再定。
