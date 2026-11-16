# AGENTS.md

本文件只写需要判断力、且暂时无法自动化的约束。能写成检查的，一律进工具和 CI。
规则集中在这里，其他文档只链接，不复制。

## 投入顺序

按信任金字塔，自下而上投入：

1. 确定性代码约束。编译器告警、`_Static_assert`、检查脚本、依赖方向、链接断言。
2. 验证。运行时验证、有界穷举、不变量检查。
3. 文字规则。只保留需要判断力的部分。

修 agent 的问题时，优先把它变成确定性检查，而不是在这里加一行。确需判断力的
才写下来，并说明为什么无法自动化。

## 元规则

- 规则少而高信号。不为一次性 bug 添加永久规则，agent 会把小修正过度泛化。
- 修复必须消除整类问题，禁止留 workaround。无法根因修复时停下来问人。
- 代码库是 agent 的输入。发现反模式、过时代码或变通写法，先清理再扩展，
  否则会被下一个 agent 当作既定模式复制。
- 验证是审查的对象。没有验证证据不算完成。
- 规则集中在本文件，其他文档只链接，避免规则漂移。

## 完成定义

完成 = 静态分析通过 + 测试通过 + 运行时验证证据。证据至少一种：

- `./ci.sh` 全绿输出。
- `host:strict`、`host:analyzer`、`host:sim-san` 的 ctest 或告警输出。
- target 验收 `verdict=clean` 和 evidence 目录。
- ELF section 实测数字，以及 IDIS 的 100 KiB 链接断言。
- `[clean-clone] ok`。

没有证据不算完成。验证覆盖用户视角。本项目用验收矩阵当 feature map：

- stategrid 的 122 行矩阵和 `tools/check_acceptance_matrix.py`。
- IDIS 两个 target 的 Python 验收用例和 `tests/acceptance/runner.py`。

形式化工具已进 CI。`formal/cbmc` 证明 A-XDR 长度解码和定长跳过的边界，
`formal/tla` 检查 block transfer 状态机，`formal/lean` 证明 FIFO 容量算术。
本地缺工具时 `tools/check_formal.sh` 跳过，GitHub workflow 安装 CBMC、Java/TLA+
和 Lean 后按 `--strict` 运行。关键算法仍优先加属性测试或小输入空间穷举。

每个产品 target 必须有一个 `tests/acceptance` 验收入口。入口里的 case 数按矩阵，
不限制为一条。共享用例用 `CASES` 指向同一份目录，不复制。缺入口或 `CASES`
指向不存在的目录时 `ci_matrix.py` 直接失败。

## 架构约束

- 依赖方向按层。`domain` 不依赖平台；`application` 依赖 `domain`；`components`
  提供协议和服务；`infra` 放 SoC、驱动、板级和 RTOS；`targets` 是产品组合根。
  门禁是 `check:layers` 和 `check:components`。
- 限界上下文之间通过消费者定义的端口交互。端口用 `void *ctx` 不透明上下文，
  在组合根绑定一次。没有设备驱动模型、`device_ops` 表或驱动注册表。
- 目录结构表达边界。能用依赖图、include 方向、命名和清单检查强制的，不在本文
  重复。见 `docs/platform-layout.md`、`docs/guides/component-standard.md`、
  `docs/guides/port-binding.md`。
- 产品构建的 domain 源列表从 `product.yaml` 经
  `tools/product_manifest.py --cmake` 解析，CMake 不再 GLOB 目录。清单里没有的
  组件不会进固件；加组件先改 `product.yaml`。
- 不新增可变全局。端口和聚合通过构造注入。public 头必须独立可编译。

## 项目约束

- C11，Cortex-M0，裸机和 FreeRTOS 两种构建。domain 和 application 必须能按
  Cortex-M0 严格编译，不能引入 host libc。
- IDIS 两个 target 的 ROM 必须小于 100 KiB，链接脚本有断言。FreeRTOS 余量约
  2 KiB，任何固件功能先做 ROM 账。
- 代码执行性能、功耗与唤醒、掉电安全是三大优先指标。掉电安全路径的改动需要
  真机或 HIL 证据；没有硬件时不要盲改。
- `test-platform` 验收是 must。新功能同时补验收用例或说明为什么不能补。
- 本地提交，不 push 远端。

## 迁移清单

代码落地、自动化就位后删除对应条目。

- [x] include 边界与分层方向。`tools/check_layers.py`、`tools/check_components.py`。
- [x] 组件命名、版本、层和路径。`tools/check_component_standard.py`。
- [x] 函数原型、Cortex-M0 严格编译。`tools/check-arm.sh`。
- [x] 禁用可变全局和跨层内核类型。`tools/check_target_kernel.py`、
  `tools/check_platform_boundary.py`。
- [x] 参数表长度与数组大小。`tools/check_param_tables.py`。
- [x] 转换告警棘轮。`tools/check_conversion.py` 和 `host:conversion`。
- [x] 死代码棘轮。`tools/check_dead_code.py`。
- [x] 运行时验证。`host:strict`（ASan/UBSan）、`host:sim-san`（整机仿真和 IDIS
  host runner）、stategrid 91 条验收、IDIS 各 11 条验收、EEPROM 损坏采样。
- [x] 形式化检查。`tools/check_formal.sh`：CBMC（A-XDR）、TLA+（block
  transfer）、Lean（FIFO 容量）。GitHub workflow 安装工具后按 `--strict` 跑。
- [ ] 并发与性能预算的自动阈值。当前只有 ROM/RAM 断言和验收，没有调度时序预算。
- [ ] IAR 实编译和 HIL。环境不具备。

## 链接

- 验证分层与盲区。`docs/guides/verification.md`。
- 平台布局与门禁。`docs/platform-layout.md`。
- 组件标准。`docs/guides/component-standard.md`。
- 端口写法。`docs/guides/port-binding.md`。
- 验收用例。`docs/guides/acceptance-case.md`。
- block transfer 设计。`docs/idis-block-transfer.md`。
