# 整机固件孪生 acceptance 测试方案

日期：2026-11-12
状态：6 个用例已实现并通过，方案随证据更新。

## 目标

在 `modular-001` 内建立一组 acceptance 测试，以 Linux 上的整机固件孪生
`firmware_sim` 为被测对象，从外部观察启动、调度、业务周期和持久化结果。
测试要能在本机重复运行，不依赖 IAR、真实表计或硬件在环设备。

## 完成谓词

- 一条命令能构建 `firmware_sim` 并跑完整 acceptance 套件。
- 用例只通过外部契约观察被测系统：进程退出码、trace、EEPROM/flash 镜像、
  以及后续要补的 LCD/继电器/通信捕获文件。
- 至少覆盖：冷启动与复位计数、RTC 秒推进、跨重启持久化、一次业务周期。
- 每个用例有独立的镜像目录，不共享 `backupfiles/` 状态。
- 方案、场景表、失败注入和已知盲区落盘到本文件及 `docs/firmware-sim.md`。

## 当前资产

- `tests/sim/sim_main.c`：MAP_32BIT 低 4 GiB 内存、真实 `create_instance`、
  真实 `baremetal_instance_entry_main`、文件后端 EEPROM/flash。
- `tests/sim/sim_trace.c`：每周期时间/trace、报文、filter、应用阶段日志。
- `tests/sim/sim_devices.c`：EEPROM 65535 字节、flash 8388607 字节，擦除值 0xFF。
- `tests/sim/sim_stubs.c`：MCU/板级入口的空实现，计量芯片返回零值。
- `tools/firmware_sim_manifest.py`：从 IAR `.ewp` 派生 172 个可移植源文件。
- `tests/CMakeLists.txt`：`firmware_sim` 目标和 3 周期 ctest 冒烟。

## 已知边界

- 没有 UART/PLC/红外报文注入，DL698/645 组帧和 mapper 请求路径未覆盖。
- 存储 pipeline `pl_resource_manager` 会执行，但记录内容还没有解码。
- 停电/上电状态机、ESAM、LCD 段码、继电器反馈、计量脉冲都没有真实模型。
- 主机 `struct tm` 布局与 ARM newlib 不同，mbase 持久化字节不能当作 ARM 镜像。
- `instance->ms` 由孪生注入，固件自身没有推进它。

## test-platform 调研结论

`/home/edge/test-platform` 是 pytest 原生平台。一个测试项就是一个 pytest 函数，
没有 testcase DSL；用例身份来自 `@case(id, clause, tags)`，选择器是
`model:`/`capability:`/`tag:`/`case:`，收集阶段就会校验未知选择器；判定分为
clean/failed/invalid/incomplete，对应退出码 0/1/2/3。证据是每次运行一个目录，
每台 DUT 一个 JSONL，记录原始帧、解码读数、判据和结论。

可以直接借的约定：稳定的 case id、clause/tag、`--select`、四档判定与退出码、
证据目录形状、acceptance 与 smoke 分开。不借的部分：manifest 三段的设备模型、
`CompositionRoot`、驱动注册表、`${PORT}` 就绪协议、1000 点历史窗口、pytest
插件本身。`firmware_sim` 没有传输总线，也不监听端口，硬套任务执行器只会得到
假的设备模型。

test-platform 的零设备覆盖记为 incomplete；本仓库把“选择器没选中用例”视为
配置错误，记为 invalid。两者都不是 clean。

## 方案

主仓库的 CI 不引入 pytest 或 PyYAML。acceptance 用 Python 标准库实现一个薄运行器，
保留 test-platform 的词汇和证据形状，不复制它的执行框架。

```text
tests/acceptance/
  run.py          # --sim --workdir --select --list --no-evidence
  sim.py          # run_sim(): 子进程、trace 解析、镜像读取
  cases.py        # @case(id, clause, tags) 注册表与选择器
  test_boot.py
  test_rtc.py
  test_persistence.py
  test_business_cycle.py
  test_flash.py
  test_determinism.py
```

每次运行的证据写在 `build/acceptance/evidence/<UTC-stamp>/`：
`<case-id>.jsonl`、`summary.json`、`report.md`、`run-scope.json`。

运行方式：

```sh
cmake --build build --target firmware_sim
python3 tests/acceptance/run.py --sim build/firmware_sim
```

ctest 增加 `firmware_acceptance`，直接调用同一个运行器。退出码：
0 clean，1 failed，2 invalid（缺二进制、选择器写错、没有用例），
3 incomplete（选了但没跑完）。skip 记为 invalid，不记 failed。

每个用例有自己的临时目录，镜像不共享。断言只看外部契约：退出码、
`[sim]` trace 的稳定子集、EEPROM/flash 文件、跨进程重启后的变化。
不按调用顺序或内部结构断言。

## 用例表

| id | 观察 | 通过条件 |
|---|---|---|
| `ACC-BOOT-001` | 空白镜像冷启动 3 秒 | 退出 0；3 条周期时间从 23:59:50 起逐秒；EEPROM 65535、flash 8388607 字节；复位计数 0；flash 未被写 |
| `ACC-RTC-001` | 35 秒跨午夜 | 每秒恰好 +1 秒；最后到 00:00:24；00:00:20 的硬件对时后时钟继续走 |
| `ACC-PERSIST-001` | 同一镜像连续启动 3 次 | 复位计数 0→1→2；每次追加一条开机事件，前一条不被覆盖；前 2 KiB 参数副本逐字节不变；非 0xFF 字节增加；flash 不变 |
| `ACC-CYCLE-001` | 1 个 `MSG_SECOND` | 14 个业务阶段按固定顺序各跑一次；`app_meter_base` filter 返回 0；收到 RTC 消息；没有 `NVIC_SystemReset` |
| `ACC-FLASH-001` | 3600 秒整点 | 时间到 00:59:49；首个 flash 写入落在 `0x1AE000`；EEPROM 至少 180 个非 0xFF 字节 |
| `ACC-DETERMINISM-001` | 两次独立 65 秒空白启动 | 周期序列、EEPROM、flash 完全一致；复位计数都为 0 |

复位计数和 flash 首块的偏移是当前持久化布局的一部分。写成 `sim.py`/
`test_flash.py` 里的命名常量。布局变化会让 acceptance 明确失败，而不是被测试
悄悄跟随。

全量业务能力矩阵在 `docs/firmware-acceptance-matrix.md`。矩阵按域拆成 122 行，
每行有稳定 ID、验收判据、状态、优先级和阻塞项。当前有 6 行到 A，其余是
C/D/E；后续按矩阵补观测面和激励，而不是按文件补测试。

## 验证

```sh
cmake --build tests/build --target firmware_sim
ctest --test-dir tests/build -R firmware_acceptance --output-on-failure
```

2026-11-12 结果：6/6 pass，verdict=clean，总耗时约 0.7 秒；ctest 76/76。
失败注入已验证：未知选择器、缺失二进制返回 2；单用例选择返回 0。

## 还未覆盖

- 通信注入、记录消息注入、停电/上电、计量脉冲和 ESAM 仍走桩，见
  `docs/firmware-sim.md` 的“已知盲区”。
- 真实主机 `struct tm` 字节兼容不在 acceptance 断言内，避免把主机布局当成
  ARM 产品契约。
- 继电器、LCD 段码、UART TX 还没有可断言的捕获面；补这些观测面之前不写对应用例。

## 过程记录

- 2026-11-12：整机孪生已可启动并跑完 3600 个模拟秒；开始 acceptance 方案调研。
- 2026-11-12：test-platform 调研完成；决定标准库薄运行器 + 借用其词汇与证据形状。
- 2026-11-12：6 个用例实现并接入 ctest；`firmware_acceptance` 绿。
- 2026-11-12：acceptance 调试中确认 `resource_tool.h` 的 pack 泄漏，补
  `#pragma pack()` 后 698 通道从 0 恢复为 3；全量 CI 仍绿。

