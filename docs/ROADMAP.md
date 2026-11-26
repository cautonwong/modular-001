# 下一阶段路线图

日期：2026-11-25
基线：`1287f017`。数字以 `./ci.sh` 和各 checker 的输出为准，本文只记当前值。

## 1. 目标

把 `modular-001` 从“按目录分层”推进到“按限界上下文和端口组织”的可复用组件仓库：

- domain 只表达业务规则，不暴露 `mbase_control_block_t`、`instance_t` 的布局。
- application 只依赖消费者定义的端口，不依赖具体存储、协议或板级实现。
- target 只做组合根、硬件适配和持久化兼容。
- product 清单驱动构建和组合，而不是只做说明。
- 所有迁移保持旧持久化字节兼容。

硬约束：只做本地提交，不 push 远端。

## 2. 当前基线（2026-11-25）

- 58 个组件清单；7 个产品（3 个固件、4 个非固件）；164 个单测文件。
- `./ci.sh`：166 个 host ctest（164 单测 + 2 仿真）、strict 164；IDIS 验收
  两个 target 各 16 条；stategrid 验收 143 行 / 109 例。
- 形式化：6 个 CBMC harness、2 个 Frama-C/Eva harness、4 个 TLA+ spec、
  6 个 Lean 模块，`formal: ok`。
- 可变全局 117 条（40 文件，28 在头文件）；product-link 源例外 0 条；
  reachability 例外 0 条；`shortcut:` 7 条。
- IDIS ROM（pinned Arm GNU 14.3.rel1，103 KiB = 105,472 B）：
  裸机 POLL 95,344、裸机 IRQ 97,948、FreeRTOS POLL 99,624、
  FreeRTOS IRQ 102,432；最紧余量 3,040 B。数字是 ROM load span（含 section
  间对齐填充），等于链接产物的 `.bin` 大小。
- `check_layers`、`check_components`、`check_platform_boundary` 全 0。

2026-11-25 更新：components 58；host 166（164 单测 + 2 仿真），strict 164；
product-link 源例外 0（产品级 `sources:` 选择 + 全局 accounting 规则，见
`docs/product-link-audit.md`）；reachability 0；ROM load span
95,344 / 97,948 / 99,624 / 102,432，最紧余量 3,040 B。

## 3. 已完成（从待办中删除）

- P0 收尾迁移：组件归属、依赖闭环、严格 ARM 编译、业务规则迁 domain、
  adapter 警告抑制删除。
- P0.3 reachability 例外清零：`services.channel` 及 `infra.channel` 只有
  host 测试调用，组件、适配器、测试、IAR 源表和产品选择一并删除。
- P0.1 产品级源选择：两个 IDIS 产品用 `product.yaml` 的 `sources:` 只选
  固件链接的源，product-link 源例外 14 → 0；test-only 源由全局 accounting
  规则保有，反查规则拒绝未声明的链接输入，旧 premise 机制删除。
- P1 核心模型解耦：`instance_t` 删除；计量端口改 `nd_energy_ledger_t`；
  `mbase` 边界隔离（结构体拆分仍待决策）。
- P2.1 产品清单驱动构建：`product.yaml` 经 `tools/product_manifest.py` 生成
  确定性源清单；固件链接与 `.ewp` 源表双向一致。
- P2.2 兜底组件拆除：legacy 容器按目录拆开；`legacy.*`、`nidavellir/`、
  `infra.os.baremetal` 已删除。
- P3.3 持续集成：`./ci.sh`、验收矩阵、形式化、ROM/RAM 断言、lint 棘轮。
- P3.4 固件链接：链接脚本、启动文件、向量表和 section 断言已接入；
  真机运行仍未验证。

## 4. P0 近期（守住绿色 + 还债）

1. **烧可变全局：117 → 0。** 分布：stategrid 50、infra 46、host 9、
   IDIS 5、IDIS FreeRTOS 4、components 3。先从 `application/ports/*.c`
   的三个 `bound` 标志开始，改为组合根一次性绑定；每批跑
   `check_mutable_globals.py`，增长需要 `mutable_globals_exceptions.txt`。
2. **ROM 账。** IRQ 余量 3,040 B，新固件功能先看 IRQ 列；数字以
   `tools/check_idis_firmware.py` 的 ELF 输出为准。

完成标准：待办清零或明确记录理由，CI 保持全绿。

## 5. P1 中期（产品化）

1. **IAR `.ewp` 由清单生成。** CI 校验生成结果与仓库一致，手写工程退役。
2. **端口绑定检查自动化。** 每个端口在组合根恰好绑定一次；漏绑/多绑失败。
   先出设计，再进 CI。
3. **698 客户端 `forward` 落地或删除。** 有真实消费者就绑定，没有就删路径。
4. **启动/运行时验证。** 补全向量表和 runtime init，加“复位→初始化→主循环”
   仿真测试；真机验证仍属 HIL。
5. **持久化契约测试。** 每个持久化块加字节级 golden test + 版本字段 +
   迁移策略；不改旧字节布局。

## 6. P2 远期（可信度与规模）

1. **HIL 自动化。** ESAM MAC/RN、上报确认、掉电重启、IR 光口、MX25
   wait_ready、释放耗时/WDT 时序；见 `MODULAR-001-HIL-2026-11-20.md`。
2. **形式化扩面。** CBMC 扩到 645 地址/密码解析，TLA+ 扩到冻结/回补状态机，
   Lean 扩到循环日志容量。Frama-C Eva 已覆盖两个 store；WP 的 Phase 0 玩具
   测试（2026-11-23）只在真目标上 valid，假目标全部 Timeout/Unknown，因此
   drop（`docs/design/wp-decision.md`）；store 契约仍不闭合，能量累加的 WP
   证明留到 prelude 收窄或 prover 升级后再试。
3. **性能/功耗预算。** sim 加周期计数、唤醒延迟、`-fstack-usage` 静态栈、
   调度抖动阈值，达标后进 CI 当硬门禁。
4. **第二个 SoC/板型。** 用真实第二产品验证端口抽象；同时处理 `mbase`
   拆分决策门。

## 7. 决策门与阻塞

- `mbase_control_block_t` 拆分：需明确同意后才启动；当前只做边界隔离。
- IAR 实编译、HIL：环境不具备。
- Frama-C WP：Phase 0 已 drop。真目标在 z3 4.8.12、Alt-Ergo 2.0.0/2.6.5、
  CVC5 1.2.1/1.3.1 上都 valid，但假目标全是 Timeout/Unknown，拿不到 invalid；
  卡点是 WP 的量化算术 prelude。bundle 带 why3 运行时和 prover 检测数据但不
  带 prover。不进 CI，当前 CI 只跑 Eva；证据与计数见
  `docs/design/wp-decision.md`。store 历史证据 26/46（12 Timeout、8 Unknown）。
- 103 KiB 是自限预算，不是硬件限制；若确有 100 KiB 约束再回头砍 IRQ 传输。
- 威胁模型：用户决定暂时跳过；光口是唯一物理门禁，前端必须置
  `remote_client=1` 且不得把 release 暴露为 COSEM 方法。

## 8. 验证入口

- `./ci.sh`、`tools/check_formal.sh`、两个 IDIS target 的验收 runner、
  stategrid 验收矩阵。
- 验证分层与盲区见 `docs/guides/verification.md`；设计与决定见
  `docs/design/`。
