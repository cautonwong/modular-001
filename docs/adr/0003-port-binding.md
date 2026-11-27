# ADR-0003: 端口由消费者定义，void *ctx 不透明，组合根绑定一次

日期：2026-11-27

## Status

Accepted。决定已落地，由 `check:layers`、`check:components` 和两个
`port-bindings` stage 守护。

## Context

架构要求依赖方向向内。domain 和 application 不能 include `infra/soc`、
`targets/` 或具体板级头，也不能出现具体 SoC、board 或 RTOS 的条件编译
（AGENTS.md 架构约束；`docs/architecture.md` §1）。需要平台能力时，消费者
声明自己需要的接口，target 在组合根实现并注入。

仓库明确不采用设备驱动模型。没有 `device_ops` 表，没有驱动注册表，没有运行期
设备工厂。一个 board 服务一个产品，产品在构建期选定，组合根直接调用具体实现
（`docs/guides/port-binding.md` 开头；AGENTS.md）。这条前提决定端口是业务
动作的集合，不是设备操作的通用接口。

旧实现用绑定函数和文件级可变指针。`nd_*_bind()` 把实现写进一个 `bound`
全局，`exactly-once` 只能靠运行期检查。R4、R5a、R5b 逐步删除了这些绑定，
改成组合根在构造或 attach 时填端口值（`docs/guides/port-binding.md` §8、
§8.2）。迁移后有 0 个旧绑定符号，`tools/port_bindings.txt` 无行，
`tools/port_binding_exceptions.txt` 已删除。

## Options

1. 设备驱动模型，`device_ops` 表加运行期注册。否决，因为一个 board 只服务
   一个产品，产品在构建期选定，运行期选择没有消费者
   （`docs/guides/port-binding.md` §1；AGENTS.md）。
2. 绑定函数加文件级 `bound` 全局。已删除。它把 `exactly-once` 变成运行期
   性质，需要可变全局，并存在未绑定窗口。R4、R5a、R5b 的迁移和
   `port-bindings` stage 就是为了消除这条路径（`docs/guides/port-binding.md`
   §8、§8.1）。
3. 消费者定义端口结构体，构造时注入，`ctx` 不透明。选中。新端口按这个模板
   生成；legacy 根 context 用 `stategrid_*_attach` 填端口值，同样只绑定一次
   （`docs/guides/port-binding.md` §3 到 §5、§8.2）。
4. 通用 `ioctl` 或 field id 解释器。否决。端口按业务动作命名，不用设备类型、
   寄存器名或厂商头；数据面端口用真实 typed 签名，不做枚举和 switch
   （`docs/guides/port-binding.md` §3；`docs/design/placement-seam.md` C6.3）。

## Decision

- 端口由消费者定义，接口头放在消费者自己的组件里。domain、application 或
  components 需要平台能力时，在自己的头文件里声明端口
  （`docs/guides/port-binding.md` §1；`docs/architecture.md` §4）。
- 端口是一个结构体，含不透明 `void *ctx` 和每个业务动作一个函数指针。
  普通能力端口用生成器模板，动作返回 `int`，0 表示成功，负数表示失败；
  数据面端口保留真实 typed 签名，例如 `application/ports/meter_base.h` 的
  34 项 vtable（`docs/guides/port-binding.md` §3；`docs/design/placement-seam.md`
  C6.3）。
- `ctx` 不透明。端口头不解释它指向什么，也不 include `targets/` 或
  `infra/soc`（`docs/guides/port-binding.md` §3）。
- target 适配器只做参数转换和错误码转换，业务判断留在 domain
  （`docs/guides/port-binding.md` §4）。
- 组合根在构造时把端口传给消费者，先于第一次使用。消费者把端口指针存进自己的
  不透明结构体。没有绑定函数，没有文件级可变端口指针
  （`docs/guides/port-binding.md` §5、§8.2）。
- legacy 根 context 用 attach 填端口值，而不是绑定函数。
  `stategrid_context_t` 持有端口值，`stategrid_*_attach` 只填字段，调用点用
  `stategrid_*_port(ctx)` 取端口；IDIS 把 port 表收在根 context `g_idis_app`
  里（`docs/guides/port-binding.md` §8.2、§8.3）。
- `exactly-once` 由构造注入在类型上成立。新端口没有绑定函数，也不需要运行期
  检查。旧端口的回归由链接后 ELF 的符号扫描拦住：`port_bindings.txt` 有行时
  比对直接 `bl`/`call` 边，0 次、多次、调用者不符或仅间接调用失败；无行时
  任何 `nd_`、`stategrid_`、`dl698_` 或 `resource_tool_` 前缀的 `*_bind`
  符号失败（`docs/guides/port-binding.md` §8.1）。

## Consequences

- 绑定不再产生可变全局。R4 把 mutable globals 从 243 降到 242，R5a 从 242
  降到 240，R5b 从 240 降到 238（`docs/guides/port-binding.md` §8.2）。
- 端口值放进根 context。R5b 的实测是 text 207,880 到 207,864 B（减 16 B），
  bss 30,196 到 30,188 B（减 8 B，两个 bound 指针），ELF 335,400 到 335,064 B，
  233 个对象不变；端口值在 arena 的 context 内（`docs/guides/port-binding.md`
  §8.2）。
- 漏绑和多绑是编译期或构造错误，不是运行期状态。旧绑定的回归仍有 ELF 扫描
  兜底，但只覆盖 legacy 前缀；新端口不需要运行期检查
  （`docs/guides/port-binding.md` §8.1 末段）。
- 端口数量按业务能力增长，不按板子数量增长。加 board 或 SoC 只加 target
  目录和组合根，不改消费者接口（`docs/guides/port-binding.md` §3；
  `docs/architecture.md` §2.3）。
- 端口头必须独立可编译，公开头不得新增可变 struct；`check:public-structs`
  和组件标准继续约束这两点（AGENTS.md 架构约束）。
- 数据面端口会带来一次调用间接。W5 的 vtable 是 `const`，放 flash，34 项
  vtable 144 B，计入 W5 的 +1,104 B ROM 账（`docs/design/placement-seam.md`
  W5 行）。

## Evidence

- `docs/guides/port-binding.md` §1 到 §8：端口模板、适配器、组合根、迁移表、
  no-legacy-bind 检查设计、根 context。
- `docs/architecture.md` §4、§6：消费者定接口、组合根职责。
- `AGENTS.md` 架构约束：没有设备驱动模型、`device_ops` 表或驱动注册表；端口用
  `void *ctx` 不透明上下文，在组合根绑定一次。
- 门禁：`check:layers`、`check:components`、`check:port-bindings-sim`、
  `meter_electricity_single_phase_cn:port-bindings`。
- `tools/port_bindings.txt` 无行；`tools/port_binding_exceptions.txt` 已删除；
  两个 stage 打印 `0-bind: no guarded *_bind symbol`
  （`docs/guides/port-binding.md` §8.1）。
- 测试：端口消费者在 `tests/unit/test_<模块>.c` 里传假端口；target 适配层由
  target 验收覆盖（`docs/guides/port-binding.md` §6）。
- R4、R5a、R5b 的 mutable 和 ARM 账见 `docs/guides/port-binding.md` §8.2。
- 本次分支的 `./ci.sh` 输出。

## Reversal conditions

- 一个产品必须在运行期支持多个 board，或运行期选择驱动。构建期选定和直接
  调用不再成立，需要运行期选择机制；这会推翻一个 board 服务一个产品的前提
  （`docs/guides/port-binding.md` 开头；AGENTS.md）。
- 同一个产品需要同一能力的两个不同实现同时存在。单个根 context 的 attach
  不够，消费者要按实例接收端口或索引表。
- 端口需要表达的行为无法用业务动作加 typed 签名描述，例如需要运行期字段
  选择。先扩展生成器或端口形状，仍不引入 `device_ops` 或 field id 解释器。
