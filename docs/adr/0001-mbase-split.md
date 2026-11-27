# ADR-0001: mbase 拆分，结构体不拆，边界隔离到 target 持久化和消费者端口

日期：2026-11-27

## Status

Accepted。决定已落地，由 `stategrid_meter:mbase`、`check:meter-record`、
`stategrid_meter:instance` 和验收用例守护。

## Context

`struct mbase_control_block_t` 是一条 1368 字节的遗留持久化记录，把计量、费率、
阶梯、事件、冻结、安全、上报、显示、负荷控制和运行状态混在一个布局里。
`sizeof` 为 1368，`persistent_section_start` 为 849，`active_p` 为 852，
`active_n` 为 904（`docs/mbase-accessors.md` §2）。迁移开始时，79 个文件引用
这个结构体，约 1460 处箭头访问，其中约 68 个文件参与构建；112 处 `offsetof`
分布在 24 个文件（`docs/mbase-accessors.md` §3）。结构体在 2026-11-10 从旧
kernel 的 `applications.h` 移到 `targets/stategrid_meter/persistence/legacy_control_block.h`，
但聚合边界仍未拆开（`docs/architecture.md` §2 第 1 条）。

两条硬约束决定了方案的形状：

1. 存量电表的持久化字节布局必须逐字节兼容（`docs/architecture.md` §2.1）。
   拆分结构体会改变字段顺序、宽度或对齐，违反这条约束。
2. domain 和 application 不得看到 `instance_t` 或 mbase 布局，也不能
   `offsetof` 或直接改字段（`docs/architecture.md` §2；AGENTS.md 架构约束）。

W5 之前，`meter_base_view.h` 位于 `application/adapters/dl698`，application
仍然知道 target 布局。W5 把该头移到 target，并在 application 定义
`application/ports/meter_base.h` 的 typed vtable，按 34 个 `nd_mb_*` 符号
每个一个函数指针，签名是真实访问形状（`docs/design/placement-seam.md`
C6.3 和 W5 行）。

## Options

1. 拆分结构体，按限界上下文各自建聚合和记录。否决，因为字节兼容是硬约束；
   拆分要改变持久化布局，并处理旧记录的版本迁移（`docs/architecture.md`
   §2.1）。
2. 保持结构体并把布局暴露给所有消费者。否决，因为 domain/application 会继续
   依赖 target 布局，`check_instance_boundary.py` 禁止这条边界
   （`docs/mbase-accessors.md` §4.1）。
3. 保持一条持久化记录，把它降级为 target 私有的持久化格式，对外按上下文提供
   不透明聚合句柄和消费者端口。选中。这是 `docs/mbase-accessors.md` §4.1 的
   方案。
4. 第一层访问器用 `.c` 函数句柄还是生成头文件宏。选宏，因为 616 次写和
   250 次取地址需要 lvalue，112 处 `offsetof` 和静态初始化需要常量表达式，
   宏不新增 IAR 源表条目（`docs/mbase-accessors.md` §4.3）。
5. W5 的跨层取值面用 field id 枚举加 switch 解释器还是 typed vtable。选 typed
   vtable，`const` 放 flash，没有枚举和 switch，避免把字段语义放进
   application（`docs/design/placement-seam.md` C6.3、§4）。

## Decision

- 不拆分 `mbase_control_block_t`。它归 `targets/stategrid_meter/persistence`
  私有，是持久化记录，不是领域聚合。
- 只有生成的 `persistence/meter_record.h` 和持久化适配器可以出现 `offsetof`、
  `sizeof`、整块 `memcpy` 和校验和计算（`docs/mbase-accessors.md` §4.3）。
  `tools/check_mbase_access.py` 统计直接字段访问和布局操作，allowlist 只减不增；
  P5 归零后 allowlist 为空（`docs/mbase-accessors.md` §5、§7）。
- domain 和 application 通过消费者定义的端口访问数据。P1 到 P5 的迁移没有新增
  领域端口，现有端口加生成宏已经满足跨层边界（`docs/mbase-accessors.md` §4.3、
  §5 P3）。
- W5 落地：`meter_base_view.h` 移到 target persistence，`application/ports/meter_base.h`
  定义 34 项 `const` typed vtable，target 为每项提供薄包装，组合根绑定一次，
  application 只 include 端口头。out-of-line 的 `nd_mb_*` wrapper 归
  application，签名不变（`docs/design/placement-seam.md` C6.3、W5 行）。
- 状态和电源的 wire 到语义映射留在 target。domain 只保留语义值，客户事件码
  `0xAA`/`0x55` 移到 target wire 头（`docs/design/placement-seam.md` C6.1、W5 行）。

## Consequences

- target 内部签名仍然出现 `struct mbase_control_block_t *`。硬边界是
  domain/application，由 `tools/check_instance_boundary.py` 强制
  （`docs/mbase-accessors.md` §4.3 末段）。
- 字段访问必须走生成宏。新增直接字段访问会让 `stategrid_meter:mbase` 失败；
  布局操作只剩生成的 persistence 头（`docs/mbase-accessors.md` §5 P5、§7）。
- 持久化布局随结构体定义和生成头一起演进。拆分结构体不再是可选项，除非字节
  兼容约束解除。
- W5 的 stategrid `.text` 实测增加 1,104 B，在 1,120 B 上限内；IDIS 四个镜像
  的 ROM 和 RAM 逐项不变（`docs/design/placement-seam.md` W5 行）。
- 残余：端口隐藏了 target 布局，但访问器仍是产品字段语义，不是 domain value
  object。value object 和 snapshot 留给后续，不阻塞本 wave
  （`docs/design/placement-seam.md` W5 行）。
- 迁移按 P0 到 P5 逐阶段完成，每阶段以 `./ci.sh` 全绿结束；P4 当时由 74 个
  主机测试和 205 源 ARM 编译保护，P5 把 allowlist 清空
  （`docs/mbase-accessors.md` §5）。

## Evidence

- `docs/mbase-accessors.md` §2 到 §5：硬约束、布局数字、两层接口、P0 到 P5
  完成状态、验证命令。
- `docs/design/placement-seam.md` C6.3 和 W5 行：34 项 typed vtable、文件移动、
  组合根绑定、+1,104 B 实测、IDIS 不变、残余。
- `docs/architecture.md` §2、§2.1、§4：数据形状、字节兼容硬约束、消费者定接口。
- 门禁：`stategrid_meter:mbase`（`tools/check_mbase_access.py`）、
  `check:meter-record`（`tools/gen_meter_record.py --check`）、
  `stategrid_meter:instance`（`tools/check_instance_boundary.py`）、
  `check:layers`、`check:components`。
- 测试：`test_meter_base_view` 覆盖 34 个 vtable 项的读写、索引、字节和 save
  映射；W5 的 firmware acceptance 覆盖 ACC-MET、ACC-EVT、ACC-LC
  （`docs/design/placement-seam.md` W5 行）。
- 本次分支的 `./ci.sh` 输出。

## Reversal conditions

- 字节兼容约束解除，或为现场表提供记录版本迁移和回读验证。那时可以按上下文
  拆成真正的聚合和记录。
- 某个跨层消费者需要业务行为或 domain value object、snapshot，typed accessor
  vtable 无法表达。那时按 placement-seam 的残余项引入 value object，并逐步
  退役字段语义端口。
- 第二个产品必须共享同一条 mbase 记录。target 私有归属不再成立，记录要移入
  共享持久化组件，并自带版本和迁移路径。
- 生成宏无法表达某类真实访问，例如需要运行期字段选择。那时先补生成器能力，
  仍不把布局暴露给 domain/application。
