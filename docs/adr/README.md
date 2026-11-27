# 架构决策记录

本目录保存已经落地的核心架构决定。每份 ADR 记录决定时的背景、备选方案、
选择、后果、证据和回退条件。ADR 不复述设计细节，细节留在链接的文档和门禁里。

状态只有三种：

- `Proposed`：已提出，未落地。
- `Accepted`：已落地，由文字或门禁守护。
- `Superseded`：被后续 ADR 取代，文件保留。

## 索引

| ID | 决定 | 状态 | 摘要 |
| --- | --- | --- | --- |
| [ADR-0001](0001-mbase-split.md) | mbase 拆分 | Accepted | 不拆 1368 字节持久化结构体，降级为 target 私有记录，跨层走消费者端口；W5 把 `meter_base_view` 移到 target，application 定义 34 项 typed vtable |
| [ADR-0002](0002-rom-budget-103-kib.md) | IDIS ROM 预算 103 KiB | Accepted | 两个 IDIS target 的四个镜像以 103 KiB 为上限，链接断言加 ELF checker 强制，`ROM_BYTES` 是唯一来源，自限预算不是硬件限制 |
| [ADR-0003](0003-port-binding.md) | 端口绑定 | Accepted | 消费者定义端口，`void *ctx` 不透明，组合根构造注入或 attach 一次；没有 `device_ops`、驱动注册表和绑定全局 |
| [ADR-0004](0004-rtos-thread-model.md) | IDIS FreeRTOS 线程模型 | Accepted | app 任务拥有对象，worker 任务做长阻塞持久化，保存队列深度 1，无 mutex，配置和任务常量由 checker 冻结 |

## 维护规则

- 每份 ADR 必须包含 `Status`、`Context`、`Options`、`Decision`、
  `Consequences`、`Evidence`、`Reversal conditions` 七节。
- 本索引必须列出 `docs/adr/` 下每个 ADR 文件，文件和索引一一对应。新增 ADR
  时同时改索引。
- `Evidence` 只引用仓库内路径、门禁名或 CI 工件。数字必须有来源，不写推测值。
- 决定变化时新增一份 ADR，并把旧文件状态改为 `Superseded`。不要改写已接受的
  决定，回退条件写在原文件里。
- 架构入口 `docs/architecture.md` 第 10 节链接到本索引和四份 ADR。
