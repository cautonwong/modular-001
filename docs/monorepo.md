# Blade 组件化 monorepo 顶层设计

状态：2026-11-10 草案。平台目录和可见性以 `docs/platform-layout.md` 为准，产品线扩展步骤见 `docs/guides/product-platform-extension.md`。

本文回答三个问题：

1. `applications/` 下的协议应用是否保留独立可执行形态。结论是收敛，不保留独立可执行。
2. `packages/kernel` 的 pipeline/filter/message 是保留还是替换。结论是替换成更简单的类型化消息总线和通道端口。
3. 现有可移植性设计是否太窄。结论是太窄，需要从“单相电表 + 698 协议”提升为“计量设备与通信模块的组件化 monorepo”。

## 1. 目标

把仓库做成可复用的、组件化的、大规模 monorepo，支持：

- 计量设备：国网单相电表、国网三相电表、水表、燃气表、热表等。
- 通信模块：载波模块、RF 模块、双模模块等。
- 协议：DL/T 698.45、DL/T 645、DLMS/COSEM、Modbus、厂商私有协议。
- 平台：多 SoC、多 board、多 RTOS。

复用的单位是组件，不是文件。一个组件必须有公开契约、独立测试、明确依赖和可替换实现。

## 2. 为什么现有设计不够

`docs/portability.md` 解决的是“同一套电表代码换 SoC、board、RTOS、domain profile”。它仍然假设：

- 产品一定是电表。
- 一定有电能、需量、费率、继电器这些概念。
- 协议适配器只有 698/645/DLMS 三种。
- 应用入口和协议栈是绑在一起的。

水表、燃气表没有电能和继电器，但有流量、阀门、脉冲、温度、压力。通信模块没有计量域，只有通道、网络、安全、升级。把这些差异硬塞进一个电表 domain，会得到大量条件分支和空实现。

正确做法是把“计量设备”和“通信模块”都看成产品装配，把领域能力、协议、平台拆成组件。

## 3. 组件模型

组件分五类。

| 类别 | 职责 | 例子 |
|---|---|---|
| core | 与业务无关的基础类型和工具 | result、handle、allocator、集合、字节序、CRC |
| domain | 可复用的业务能力 | 累计量、需量、定价、日历、事件、阀门/继电器控制 |
| application | 用例编排和端口 | 计量用例、上报用例、冻结用例 |
| protocol | 协议编解码和会话 | dl698、dl645、dlms、modbus |
| services | 基础设施服务实现 | 存储、安全、通信通道、显示、时钟 |
| platform | SoC、board、RTOS 适配 | v85xxp、stategrid 单相板、baremetal |

每个组件目录至少包含：

```
components/<category>/<name>/
  include/           公开头文件，唯一允许被外部 include 的入口
  src/               实现
  port/              消费者定义的端口，或端口的使用说明
  test/              组件自己的 host 测试
  component.yaml     名称、版本、依赖、支持平台、owner
```

组件契约：

- 公开头只暴露不透明句柄、值对象和函数，不暴露结构体布局。
- 组件不读全局可变状态。实例状态由调用方持有并传入。
- 组件之间只通过公开头和端口交互，不 include 对方 `src/`。
- 组件不直接 include `infra/soc`、`targets/<product>/board`、`infra/rtos` 或 `products/`。
- 持久化格式属于产品适配层，不进入 domain 组件的值对象。
- 每个组件有独立测试目标，可以单独编译和运行。

## 3.1 遗留隔离

旧 kernel 的 pipeline、filter、message 和全局 fifo 不删除，但被标记为 legacy，只允许 legacy 组件使用。

- `infra/kernel` 和 `nidavellir/include/kernel` 的组件清单标记 `legacy: true`。
- `application/adapters/dl698`、`dl645`、`application/apps` 在完成协议组件迁移前标记 legacy。
- `tools/check_components.py` 禁止 core、domain、application、protocol、services 组件 include `kernel/` 或 `nidavellir/`。
- 新组件只能依赖公开组件和端口，不能依赖 legacy 头。
- 遗留代码在阶段 5 删除，届时 legacy 清单和 `kernel_core.h` 一起移除。

## 4. 目录布局

```
components/
  core/
    result/
    collections/
    bytes/
  domain/
    quantity/          累计量、瞬时量、需量
    pricing/           费率、阶梯、分时
    calendar/
    events/
    actuator/          继电器、阀门、告警输出
    display/
  application/
    metering/
    reporting/
    freezing/
  protocol/
    dl698/
    dl645/
    dlms/
    modbus/
  services/
    storage/
    security/
    channel/
    time/
  platform/
    soc/<soc>/
    board/<board>/
    rtos/<rtos>/
products/
  meter_electricity_single_phase_cn/
  meter_electricity_three_phase_cn/
  meter_water/
  meter_gas/
  comm_plc/
  comm_rf/
tools/
tests/
docs/
```

`products/<product>/` 是组合根，包含 `product.yaml`、`main.c`、持久化适配、产品测试和构建文件。产品只装配组件，不实现业务规则。

## 5. 产品矩阵

| 产品 | domain 组件 | protocol 组件 | 平台重点 |
|---|---|---|---|
| 国网单相电表 | quantity、pricing、calendar、events、actuator、display | dl698、dl645 | 单相计量前端、继电器、LCD、ESAM |
| 国网三相电表 | quantity、pricing、calendar、events、actuator、display | dl698、dl645 | 三相计量前端、相序/缺相、继电器 |
| 水表 | quantity（体积）、pricing、calendar、events、actuator（阀门） | dlms、modbus、厂商协议 | 脉冲/流量输入、阀门、低功耗 |
| 燃气表 | quantity（体积）、pricing、calendar、events、actuator（阀门） | dlms、modbus、厂商协议 | 脉冲/流量输入、阀门、安全 |
| 载波通信模块 | 无计量域，可选 events | 698/645 透传、PLC 网络层 | PLC 调制解调、MAC、路由、升级 |
| RF 通信模块 | 无计量域，可选 events | 698/645 透传、RF 网络层 | RF 收发、MAC、路由、升级 |

通信模块不装配 metering/pricing/actuator，只装配 channel、protocol、security、storage、upgrade。产品矩阵是组件复用边界的验收标准。

## 6. 现有 domain 的组件化评估

现有 `domain/` 是组件的种子，但还不是完整组件。

| 现有模块 | 可复用性 | 需要补的边界 |
|---|---|---|
| `domain/common/calendar` | 高，电/水/气通用 | 已经比较独立，补组件元数据和测试目标 |
| `domain/tariff` | 中，电价语义强 | 抽象成 pricing，支持单一价、阶梯、分时、季节 |
| `domain/metering` 能量账本 | 中，电能语义强 | 抽象成 quantity/accumulator，支持 kWh、m3、脉冲 |
| `domain/freeze` | 高，周期快照通用 | 与具体记录布局解耦，记录 codec 留给产品 |
| `domain/events` | 高，事件去抖和持续时间通用 | 事件集合由产品 profile 选择 |
| `domain/report` | 高，上报策略与协议无关 | 通道和对象列表由产品适配器提供 |
| `domain/security` | 中，698 安全模式语义强 | 权限表和安全模式由产品参数配置 |
| `domain/load_control` | 中，继电器语义强 | 抽象成 actuator，支持继电器、阀门、告警输出 |
| `domain/display` | 中，电表页面强 | 页面模型和产品页面内容分离 |
| `domain/common/parameter_store` | 高，通用键值存储契约 | 保持为端口，不绑定 EEPROM 布局 |

结论：现有 domain 层是“可复用组件的候选”，不是已经可复用的组件。主要缺口是 legacy 类型、`mbase` 单体和单相电表假设。

## 7. 协议应用收敛

`application/apps/app_698_server`、`app_645_server`、`app_msta`、`app_dlms_client` 不再作为独立可执行或独立入口存在。它们收敛成 `components/protocol/<protocol>`。

统一协议组件接口：

```c
typedef struct nd_protocol nd_protocol_t;

nd_protocol_t *nd_dl698_create(const nd_protocol_config_t *config);
int nd_protocol_on_frame(nd_protocol_t *protocol, const nd_frame_t *frame);
int nd_protocol_tick(nd_protocol_t *protocol, uint32_t now_ms);
int nd_protocol_encode(nd_protocol_t *protocol, nd_frame_t *out);
void nd_protocol_destroy(nd_protocol_t *protocol);
```

协议组件不直接读引脚、不直接写 EEPROM、不直接操作显示。它通过 channel、storage、security、reporting 端口工作。产品 `main.c` 负责创建协议实例并绑定端口。

协议组件可以有独立 host 测试和可选的命令行仿真器，但不作为固件产品交付物。独立可执行形态只用于开发调试，不进入产品构建。

## 8. 替换 pipeline/filter/message

现有机制的问题：

- `pipeline_t`、`filter_t`、`message_t` 是一套通用图执行引擎，但路由和连接函数大多是空实现。
- 依赖全局 fifo、全局 pipeline id 和全局上下文 extern。
- 消息头有魔数、长度和校验，但没有背压、没有类型安全。
- 动态分配和全局状态让组件无法独立测试。
- 多数调用方只是“发一个事件”或“读一段字节”，不需要图。

替换方案分三层。

### 8.1 类型化事件总线

用于异步事件，例如秒变化、电源变化、按键、告警。

```c
typedef enum {
    ND_EVENT_TICK_SECOND,
    ND_EVENT_TICK_MINUTE,
    ND_EVENT_POWER_CHANGE,
    ND_EVENT_KEY,
    ND_EVENT_ALARM,
    ND_EVENT_TOPIC_COUNT
} nd_event_topic_t;

typedef struct {
    nd_event_topic_t topic;
    uint32_t timestamp_ms;
    union {
        struct { uint8_t key; uint8_t state; } key;
        struct { uint8_t on; } power;
        struct { uint32_t mask; } alarm;
    } data;
} nd_event_t;

typedef void (*nd_event_handler_t)(void *ctx, const nd_event_t *event);

int nd_bus_subscribe(nd_event_topic_t topic, nd_event_handler_t handler, void *ctx);
int nd_bus_publish(const nd_event_t *event);
```

约束：

- 固定主题枚举，固定大小负载，无 malloc。
- 固定大小订阅表，启动时注册，运行期不增删。
- 默认同步分发，按注册顺序调用。RTOS 需要异步时由 platform 层提供队列实现。
- 发布返回实际处理的订阅者数量，便于测试和诊断。

### 8.2 通道端口

用于字节流，替代 pipeline 的 stdin/stdout。

```c
typedef struct nd_channel {
    void *ctx;
    int (*read)(void *ctx, uint8_t *buf, size_t len);
    int (*write)(void *ctx, const uint8_t *buf, size_t len);
    int (*flush)(void *ctx);
} nd_channel_t;
```

UART、PLC、RF、红外都实现这个端口。协议组件只看见通道，不看见 fifo 和 pipeline。

缓冲和零拷贝策略见 `docs/buffer-strategy.md`。当前不把 lwIP pbuf 作为协议内部缓冲；IP 传输产品再把 lwIP 作为 infra 组件接入。

### 8.3 直接用例接口

同步操作直接调用 application 组件的函数，例如 `nd_metering_on_energy(...)`、`nd_reporting_add_oad(...)`。不经过消息总线，不构造 `message_t`。

调度器只做三件事：

1. 从 platform 取 tick。
2. 发布秒/分事件。
3. 驱动协议组件的 `tick` 和通道读写。

### 8.4 迁移顺序

1. 新增 `components/core/bus` 和 `components/services/channel`，先写 host 测试，不接线。
2. 用适配器把旧 `send_mesg`/`recv_mesg` 包到新总线，保持旧调用点可运行。
3. 迁移一个协议组件，建议 dl698，因为它已经有参数端口和上下文拆分。
4. 迁移一个应用用例，建议秒事件和上报触发。
5. 当旧调用点归零，删除 `pipeline_t`、`filter_t`、`message_t` 和全局 fifo/pipeline extern。
6. 用 lint 禁止新代码 include `pipeline.h` 和 `message.h`。

## 9. 与四层规则的关系

四层不再作为顶层目录的硬划分，而是组件内部的依赖规则。

| 组件类别 | 允许依赖 |
|---|---|
| core | core |
| domain | core、domain |
| application | core、domain、application |
| protocol | core、domain、application、protocol |
| services | core、domain、application、services |
| platform | core、application 的端口、platform |
| products | 全部，负责装配 |

`tools/check_layers.py` 升级为组件依赖检查，读取 `component.yaml`，按组件而不是按目录前缀判断。

## 10. 迁移路线

阶段 0：现状冻结

- 保持当前 144/144 编译链接和 34/34 主机测试。
- 记录每个 domain 模块的公开 API 和隐含依赖。

阶段 1：组件元数据

- 给现有 domain 模块补 `component.yaml` 和公开头。
- 建立组件依赖检查，先只报告不拦截。

阶段 2：core 和 bus

- 落地 `components/core/{result,bytes,collections}`。
- 落地 `components/core/bus` 和 `components/services/channel`，带 host 测试。
- 旧 kernel 机制保持运行。

当前进度：`components/core/bus/nd_bus` 和 `components/services/channel/nd_channel` 已落地，包含 `component.yaml`、host 测试和独立构建目标。总线是固定主题、固定订阅表、同步分发、无 malloc。通道是非阻塞字节端口，提供 read、write、flush。`infra/channel/fifo_channel` 已把旧 FIFO 包装成通道端口。秒变化和电源变化已经向总线发布，旧 pipeline 路径保持不变。domain、application、infra 的关键模块和 `products/` 下的产品描述已经补上组件元数据，`tools/check_components.py` 会检查组件 include 方向和产品引用。`protocol.dl698` 组件已经接管三个 698 通道的收发，收到完整帧时发布 `ND_EVENT_DL698_FRAME`，旧 rx pipeline 已从 recipe 移除。dl698 已不再引用 `message_t`，改用私有的 `nd_dl698_msg_t`，主动上报直接调用组件发送，tx pipeline 已删除。`protocol.dlms` 已移到 `components/protocol/dlms`，仍是未构建的 legacy 组件；`protocol.dl645` 也已完成私有消息类型和通道组件，645 产品可以直接实例化；当前单相产品没有 645 context，因此尚未接线。dlms 适配器已移除 `message_t` 并通过独立语法检查，但 DLMS 仍不在产品构建里。`components/core/buf` 提供链式 `nd_frame`/`nd_block`/`nd_cursor`，698 的分段重组已经改用块链，当前帧节点指向现有 APDU 存储。缓冲策略见 `docs/buffer-strategy.md`，当前不引入 lwIP pbuf。

阶段 3：协议收敛

- 把 698/645/DLMS 应用入口改成协议组件。
- 用 channel 端口替代 pipeline 输入输出。
- 产品 `main.c` 只装配，不实现协议。

阶段 4：domain 泛化

- `metering` 抽象成 quantity/accumulator。
- `tariff` 抽象成 pricing。
- `load_control` 抽象成 actuator。
- 水表、燃气表用同一套组件装配。

阶段 5：删除旧机制

- 删除 pipeline/filter/message 和全局 fifo/pipeline extern。
- 删除 `nidavellir.h` 和 `kernel_core.h` 兼容层。
- 组件依赖检查改为硬失败。

阶段 6：产品矩阵

- 新增水表、燃气表、PLC 模块、RF 模块各一个最小产品。
- 用产品矩阵证明组件复用，而不是靠文档声明。

## 11. 验收标准

- 新增一个水表产品只新增 `products/meter_water` 和必要的 platform 适配，不修改电表 domain。
- 新增一个 PLC 模块产品不装配 metering/pricing/actuator。
- 新增一个协议只新增 `components/protocol/<protocol>`，不改 domain。
- 任意组件可以单独编译和运行测试。
- domain 和 application 组件的 include 图里没有 platform、products 或具体 SoC。
- 旧持久化字节回归测试持续通过，直到产品明确升级存储版本。
- `pipeline_t`、`filter_t`、`message_t` 在迁移完成后从仓库消失。
