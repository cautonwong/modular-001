# 下一阶段路线图

日期：2026-11-20
基线：`c9f7214d`。数字以 `./ci.sh` 和各 checker 的输出为准，本文只记当前值。

## 1. 目标

把 `modular-001` 从“按目录分层”推进到“按限界上下文和端口组织”的可复用组件仓库：

- domain 只表达业务规则，不暴露 `mbase_control_block_t`、`instance_t` 的布局。
- application 只依赖消费者定义的端口，不依赖具体存储、协议或板级实现。
- target 只做组合根、硬件适配和持久化兼容。
- product 清单驱动构建和组合，而不是只做说明。
- 所有迁移保持旧持久化字节兼容。

硬约束：只做本地提交，不 push 远端。

## 2. 当前基线（2026-11-20）

- 60 个组件清单；7 个产品（3 个固件、4 个非固件）；156 个单测文件。
- `./ci.sh`：158 个 host ctest（156 单测 + 2 仿真）、strict 156；IDIS 验收
  两个 target 各 16 条；stategrid 验收 134 行 / 105 例。
- 形式化：6 个 CBMC harness、2 个 Frama-C/Eva harness、4 个 TLA+ spec、
  6 个 Lean 模块，`formal: ok`。
- 可变全局 248 条（76 文件，28 在头文件）；product-link 源例外 22 条；
  reachability 例外 1 条；`shortcut:` 7 条。
- IDIS ROM（pinned Arm GNU 14.3.rel1，103 KiB = 105,472 B）：
  裸机 POLL 94,968、裸机 IRQ 97,452、FreeRTOS POLL 99,188、
  FreeRTOS IRQ 101,868；最紧余量 3,604 B。数字是 ROM load span（含 section
  间对齐填充），等于链接产物的 `.bin` 大小。
- `check_layers`、`check_components`、`check_platform_boundary` 全 0。

## 3. 已完成（从待办中删除）

- P0 收尾迁移：组件归属、依赖闭环、严格 ARM 编译、业务规则迁 domain、
  adapter 警告抑制删除。
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

1. **烧可变全局：248 → 0。** 分布：stategrid 83、IDIS FreeRTOS 50、
   IDIS 45、infra/boot 40、host 9、其余 21。先从 `application/ports/*.c`
   的三个 `bound` 标志开始，改为组合根一次性绑定；每批跑
   `check_mutable_globals.py`，增长需要 `mutable_globals_exceptions.txt`。
2. **清 product-link 源例外：22 → 0。** 真死源删除，清单漏选补
   `product.yaml`，保留项必须理由成立。拒绝 stale 例外。
3. **清 reachability 例外：1 → 0。** 当前只有 `services.channel`：
   补生产调用者或删除组件，二选一。
4. **ROM 账。** IRQ 余量 3,604 B，新固件功能先看 IRQ 列；数字以
   `tools/check_idis_firmware.py` 的 ELF 输出为准。

完成标准：四项清零或明确记录理由，CI 保持全绿。

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
   Lean 扩到循环日志容量。Frama-C Eva 已覆盖两个 store；WP 需要 why3 和
   prover，当前 bundle 没有，能量累加的 WP 证明留到工具就位后。
3. **性能/功耗预算。** sim 加周期计数、唤醒延迟、`-fstack-usage` 静态栈、
   调度抖动阈值，达标后进 CI 当硬门禁。
4. **第二个 SoC/板型。** 用真实第二产品验证端口抽象；同时处理 `mbase`
   拆分决策门。

## 7. 决策门与阻塞

- `mbase_control_block_t` 拆分：需明确同意后才启动；当前只做边界隔离。
- IAR 实编译、HIL：环境不具备。
- Frama-C WP：官方 bundle 没有 why3，也没有系统 prover；演绎证明（含能量
  累加）留到 why3/prover 就位后，当前 CI 只跑 Eva。
- 103 KiB 是自限预算，不是硬件限制；若确有 100 KiB 约束再回头砍 IRQ 传输。
- 威胁模型：用户决定暂时跳过；光口是唯一物理门禁，前端必须置
  `remote_client=1` 且不得把 release 暴露为 COSEM 方法。

## 8. 验证入口

- `./ci.sh`、`tools/check_formal.sh`、两个 IDIS target 的验收 runner、
  stategrid 验收矩阵。
- 验证分层与盲区见 `docs/guides/verification.md`；设计与决定见
  `docs/design/`。
