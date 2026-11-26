# Blade 四层架构

状态：草案，等待勘察报告校订。
工作分支：`main`。

## 1. 四层与依赖规则

```
targets/       组合根。唯一同时看见其余三层的目录。
    |
    v
application/   用例 + 协议适配器。只依赖 domain 的公开契约。
    |
    v
domain/        聚合、聚合根、值对象、领域服务、有界上下文。纯 C，无平台依赖。
    ^
    |
infra/         SOCes / boards / interrupt / RTOSes / HALes。实现上层定义的端口。
```

依赖方向只向内。允许的 include 关系：

| 源层 | 允许 include |
|---|---|
| domain | domain |
| application | domain, application |
| infra | domain, application, infra |
| targets | domain, application, infra, targets |

其他方向一律视为架构错误，由 `tools/check-layers.sh` 拦截。

## 1.1 components 是 provider 层，不是 domain

`components/` 放可复用的协议、服务和基础设施能力。它不是领域模型，也不合并进 `domain/`。

- `domain` 只放聚合、聚合根、值对象、领域服务和有界上下文。纯 C，不依赖协议、通道、缓冲或 IP。
- `core` 组件无依赖。`protocol` 和 `services` 组件的目标是只依赖 `domain`、`core`、`services`，不依赖 `application`、`infra` 或 `targets`。
- `tools/check_components.py` 对 `#include` 和 `dependencies:` 使用同一张 `FORBIDDEN` 表。`protocol`/`services` 不能依赖 `application`、`infra`、`platform`、`legacy` 和 `product`；`services` 只能依赖 `domain`、`core`、`services`，不能依赖 `protocol`。`dependencies:` 里的名字先解析到组件清单的 `layer` 再判定。`--self-check` 每次运行都在临时树里构造负例（protocol include application、protocol 以 `../../../` 相对路径 include application、protocol 依赖 application、services 依赖 protocol）和一个干净组件。include 先按仓库根相对路径归类；不在已知前缀下时按包含文件所在目录解析成仓库相对路径再归类，组件自身路径、仓库外路径和系统头跳过。`protocol.dl698` wrapper 已删除，698 状态机留在 `application/adapters/dl698`，由 target 的组合根驱动。
- `tools/check_layers.py` 只管四层，不扫描 `components/`。组件方向以 `check_components.py` 为准。
- `tools/check_product_callers.py` 只检查固件产品，产品集合由 `check_product_component_usage.py` 从 target 的 CMake `PRODUCT` 或 IAR 项目派生。产品闭包 = `product.yaml` 解析出的组件 `.c` 加该产品 target 目录下的 `.c`，扣除 `tests` 和构建目录。闭包内每个非 product 层组件的公开函数（组件根和 `include/` 下的声明）至少要有一个同一闭包内的调用点；调用方必须直接或传递 include 组件路径下声明该函数的头，函数定义本身、组件内部调用、其它产品、`tests/`、`formal/` 和 `targets/host` 都不 credit。基线为空：无 caller 的选中组件直接失败，只能用删清单或收窄 `sources:` 解决。非固件产品（`comm_plc`、`comm_rf`、`meter_gas`、`meter_water`）是 source assembly，没有 target 源也没有链接，caller 不在闭包内是职责边界；它们由 `check_product_component_usage.py` 证明源可解析，不由本检查判定。`--self-check` 每次运行在临时树里证明同产品 caller 通过，跨产品、仅自调用、仅测试/host caller 失败。
- `tools/check_product_component_usage.py` 用构建产物逐源检查产品链接覆盖。固件产品从 `targets/` 派生，不靠手写清单：有 `app/` 或 `build/` CMake 的 target 是固件，用 `set(PRODUCT ...)` 指定产品；有 `iar for arm` `.ewp` 的 target 是固件，按产品选中的组件路径归属产品（组件 `paths` 指向 target 目录本身也算选中该 target）。固件 target 没有产品、同一产品归属两个固件 target、或给固件产品登记 `non_firmware` 都失败；没有固件 target 的产品必须登记 `non_firmware`。stategrid 的证据是 `check-firmware-link.py` 把 IAR EWP 源清单编译并交给 `arm-none-eabi-gcc` 链接后写出的 `--manifest`；它记录的是链接输入（编译并传给 ld 的翻译单元），不是符号存活：`-Wl,--gc-sections` 可能丢掉某个源的全部 section。当前 231 个产品源中有 22 个在 `firmware.elf` 里没有任何已定义全局符号存活（`arm-none-eabi-nm --defined-only`）；要求符号存活会把它们变成 22 条例外，所以门禁只声明链接输入，符号存活是残余。两个 IDIS 产品取 `build/ci-*/build.ninja` 唯一的可执行链接语句，链接对象必须有编译规则。没有 `.c` 的组件按 header-only 通过。每个被选中组件的解析源清单（`product.yaml` 的 `sources:` 可把组件收窄到具体 `.c`；没有 `sources:` 的组件贡献 `paths` 下全部源，扣除 `exclude` 和 `tests/`）都必须是该产品链接的输入；新增未链接源、链接对象缺编译规则、产品无链接证据都失败。反查规则同样生效：被选中组件交付的源若出现在链接里，必须在产品解析清单中；仓库内每个链接输入必须落在某个组件的 `paths` 下，`infra/third_party/` 和仓库外路径是构建输入，不要求清单声明。全局 accounting 规则：某个固件产品选中的组件，其每个 `.c` 必须被至少一个固件产品选中，或被 `tests/CMakeLists.txt` 编译，否则失败。`protocol.dlms` 的四个 `nd_dlms_*.c` 由测试清单保有，`core.types` 的 `bitarithm.c`/`list.c`/`rtc.c` 由 stategrid 的全量选择保有。非固件产品按 `non_firmware` 登记原因，并核对选中组件的 `.c` 都在 `product_manifest` 源清单里；`meter_gas` 和 `meter_water` 没有固件镜像，登记为 manifest-only。`tools/product_link_baseline.txt` 现在只登记 `non_firmware`。自检每次运行都证明部分链接会在新源上失败、反查规则拒绝被选中组件未声明的链接源和无组件声明的链接输入、accounting 规则拒绝无落点的组件源、注释掉的 `test` 条目被忽略，vendored 输入和测试编译的源通过，固件产品从 CMake `PRODUCT` 和 IAR 组件路径派生，无产品 target 和双 target 产品都失败。残余：EWP 里 `fr_698_comm.c` 和 `resource_access.c` 都定义 `nd_get_index`，dl698 那个在 `WIN32_U_TEST` 下编译为空；检查不解析预处理条件。
- `tools/check_mutable_globals.py` 扫描受管层的 `.c` 和 `.h`，规则相同：`static` 声明，含函数内 `static` 局部；以及位于行首、花括号深度为 0 的非 `static` 文件作用域声明。可变指针、数组和对象计数；`const` 对象、`const` 指针、`extern` 声明、`typedef`、函数原型和函数定义不计数，IAR 的 `@` 位置后缀先去掉。`tools/mutable_globals_baseline.txt` 按“路径 名字 行号”每行一条：同一文件里两个同名全局是两条，删一个再在另一行加一个会同时报新条目和过期条目；移动行号也会失败，直到 `--update`。当前条数和文件数只以 `tools/mutable_globals_baseline.txt` 和 `python3 tools/check_mutable_globals.py` 的输出为准，本文不重复数字。基线只降不升：新增条目必须先在代码里消除，或按 `--update` 流程写明理由更新基线；过期条目会让检查失败。扫描跳过 `tests/`、`infra/third_party`、`infra/soc` 下的 `vendor/` 子树、`infra/security/crypto` 和构建目录。剩余盲区：宏展开产生的声明看不到，宏可能带参数，安全展开要逐文件的预处理标志；`#if` 两个分支都参与解析；函数内 `static` 仍按债务计数；非 `static` 函数局部靠行首缩进和花括号深度排除。
- `components/core` 和 `components/services` 是模板。`components/protocol` 现在只有 DLMS；698 状态机在 `application/adapters/dl698`，由 stategrid target 驱动。两个 IDIS 产品的选择已与构建对齐：除被 CMake 过滤的 `application.metering`、`application.adapters.dl645`、`application.adapters.dl698` 和 `core.buf` 外，`application.ports` 也已移除；没有 IDIS 翻译单元 include 它们的头，CMake 也把它们过滤出链接。`protocol.dlms` 的固件链接只编 `src/nd_axdr.c`，其余 `src/nd_dlms_*.c` 只在 host 测试里编译。两个 IDIS 产品的 `sources:` 只声明 `core.types` 的 `utils.c` 和 `protocol.dlms` 的 `src/nd_axdr.c`，链接输入与 ROM 不变。DLMS 的四个 test-only 源和 `core.types` 的 `bitarithm.c`/`list.c` 由 `tests/CMakeLists.txt` 编译，`rtc.c` 由 stategrid 的全量 `core.types` 选择保有；非固件产品也按 `non_firmware` 登记原因。`tools/product_link_baseline.txt` 不再有 source 例外。
- stategrid_meter 的 RAM 预算和 OOM 覆盖也编在代码里。`targets/stategrid_meter/app/ram_budget.h` 定义 24 KiB arena、2 KiB 元数据区、cache 2108 B、mbase 3 KiB，并按 `STG_698_LINK_COUNT` 推导 698 块；共享分配入口 `__nd_malloc`/`__nd_calloc` 按 4 字节取整，元数据、cache 和 698 块的分配项按同样方式取整；mbase 三区按 `offsetof` 的原始大小放置，不经过分配器。断言分两层：每个块内部先算自己的占用（元数据 = context + 4 个 `block_device_t` + 17 个 `resource_block_t`；cache = RTC 4 + key 8 + `cache_max` + 每 698 通道 8；mbase = static + cache + persistent 三区；698 = 通道数 × 单通道）并断言不超过块大小，再断言四个块之和不超过 arena。host 当前：元数据 1968/2048 B、cache 2084/2108 B（余 24 B）、698 17136 B、mbase 1361/3072 B；四块按块大小合计 24364 B（2048 + 2108 + 17136 + 3072），arena 总余量 212 B（不是某个块的 headroom；内部 used 之和为 22549 B）。Cortex-M0 用 arm-none-eabi-gcc 的 sizeof：元数据 1420/2048 B、mbase 1321/3072 B、698 16572 B，总余量 776 B；IAR 实编译仍未跑。运行时每个 `add_block`/`add_inside_block`/`add_device`/`register_parameter`/`register_log`/`register_map` 结果都检查，`add_inside_block` 拒绝 `offset + size > block->size`，`stategrid_context_create` 的第一个分配也检查，所以溢出在启动时失败而不是静默重叠；`add_block`/`add_inside_block`/`register_parameter`/`register_log` 返回资源块描述符而不是可能为 0 的存储地址，EEPROM/flash 首块地址 0 不再被误判为失败；`register_map` 本来就返回 `resource_map_t` 描述符。`stategrid_meter_entry.c` 的 698 通道表与计数不一致编译失败，所以加第四个通道必须先改计数，而计数一变预算断言就重算：当前产品只有 RS485-1、PLC 和 IR 三个 698 通道，第四个不是产品配置。`__no_init nd_memory[STG_RAM_ARENA_SIZE]`、`stategrid_context_create` 和 host 模拟的 arena 默认值共用这组宏。ACC-BOOT-002 先跑一次正常启动，从模拟器的 `[sim] mallocs=N` 读共享分配入口的调用总数，再对 1..N 逐个 `--fail-malloc`，断言退出非 0、`init failed`、无 `done`；当前正常启动 `mallocs=49`（3 次 context create、7 次 `add_block`、3 次 `add_inside_block`、2 次 `add_device`、7 次 `register_*`、27 次 `nd_malloc`），测试同时断言总数不低于 49，掉回旧的 27 会失败。元数据断言里的设备数和块数由 `tools/check_ram_budget_counts.py` 校验：`stategrid_context_create` 各分配一个设备和根块，`add_device` 和 `register_map` 各分配一个设备，`add_block`、`add_inside_block`、`register_parameter`、`register_log` 各分配一个块，映射依据是 `resource_tool.c` 里这些函数实际调用的 `__nd_malloc`，`check:ram-budget-counts` 在 CI 里跑；盲区：号码随工具链布局变化，文档数字只是当前编译输出。

合并 `components` 到 `domain` 会把协议、通道、缓冲和 IP 放进模型层，破坏依赖方向和 `check_components.py` 的边界。

## 2. 数据形状优先

先定数据，再定接口。

- 聚合根是一个**不透明句柄**。定义留在 `.c` 文件里，公开头文件只有前置声明。
- 消费者拿不到结构体布局，因此不能 `offsetof`、不能直接改字段。
- 持久化交给聚合自己的 `serialize` / `deserialize`，不由 target 头文件用宏拼字段。
- 有界上下文之间只通过显式参数和返回值交换**值对象**，不共享可变状态。

当前的主要违规，也是重构的靶心：

1. `struct mbase_control_block_t` 曾定义在旧 kernel 的 `applications.h`，约 330 行，把计量、费率、事件、显示参数、上报配置、继电器状态混在一个聚合根里。2026-11-10 已移到 `targets/stategrid_meter/persistence/legacy_control_block.h`，聚合边界仍未拆开。
2. `instances/stategrid_meter/port.h` 用宏（`U_data_fields`、`I_data_fields`、`static_start`、`dyn_cache_start`）把 target 字段注入 domain 聚合根。domain 反向依赖了 target。
3. 持久化布局依赖对聚合根做 `offsetof`，字节布局与结构体字段顺序强绑定。
4. `applications.h` 是一个约 32 KB 的全局 extern 清单，声明 70 个符号，而实现散落在
   旧的应用与软件包目录。kernel 伞头无条件 include 它，于是 kernel 对外宣称了一批
   自己并不实现的符号。2026-11-10 已删除该伞头，声明按 owner 分散到 context、infra
   和 target 头，该伞头不再 include 它。
5. 接口已经漂移：旧 698 server stub 的 `read_esam_data` 和
   `display_oad_to_id` 签名与旧 `applications.h` 的声明不一致，靠近似符号名链接。
6. target 关注点散在 `driver_entry.c`、`os_entry.c`、`port.h`、`targets/stategrid_meter/board/board_io.c`
   和 v85xxp arch 头里，没有单一组合根。

## 2.1 兼容复刻约束（硬约束）

存量电表的持久化字节布局必须逐字节兼容。

- 新 domain 聚合的结构体不与旧布局绑定。旧布局作为 target 侧的
  `persistence/legacy_<context>_record.h` 保留，字段顺序、宽度、对齐和 `port.h`
  注入的扩展字段都按原样复刻。
- domain 只提供字段读取与写入的语义接口。target 的序列化器把 domain 值写入旧偏移。
- 每个持久化记录带独立版本号。版本号变化时，读路径必须能做旧版本迁移。
- 验证方式是对同一组输入分别跑旧结构体和新序列化器，比较字节数组完全相等。
  这必须成为 `tests/` 里的回归用例。

已冻结的电能切片（`tools/legacy_probe.c` 实测）：

| 字段 | 偏移 |
|---|---|
| active_p[13] | 0 |
| active_n[13] | 52 |
| active_p_dec[13] | 104 |
| active_n_dec[13] | 130 |
| Energy_chksum | 156 |
| MonthEnegery_dec | 160 |
| MonthEnegery | 164 |
| UseEnergy_chksum | 168 |
| 切片大小 | 172 |

该切片在旧持久化块内的起始偏移是 3。校验和与 `reserved` 填充字由持久化层负责，
codec 不覆盖它们。<br>
对应实现：`targets/stategrid_meter/persistence/legacy_energy_record.h` 和
`legacy_energy_codec.c`，偏移由 `_Static_assert` 锁死。


## 2.2 资源所有权现状

1. 三套分配并存，且都没有释放：libc `malloc`（生产代码 8 处，仅 1 处 `free`）、
   `nd_malloc` 的 bump 区（不释放）、`user_malloc`/`power_safe_malloc` 空桩。
2. `nd_malloc` 清零，`nd_calloc` 不清零，名字与 libc 语义相反。
3. 计量芯片校准状态、继电器状态、UART 缓冲等是文件级可变全局变量，
   第二实例无法创建。

新的资源契约只保留一条路径：聚合构造时注入分配端口，返回清零内存，释放带 size。

## 2.3 可移植性

整体组件化 monorepo 设计见 `docs/monorepo.md`，平台可移植性细节见 `docs/portability.md`。核心约定：

- 产品 = domain profile + protocol adapters + platform(soc + board + rtos)。
- 平台差异放在 `infra/soc/<soc>`、`targets/<product>/board`、`infra/rtos/<rtos>`。
- domain 和 application 不出现具体 SoC、board、RTOS 的 include 或条件编译。
- 端口由消费者定义，target 组合根负责绑定。
- 新增 SoC、board、RTOS 或 domain 只新增目录和组合根，不改已有业务规则。

当前落地：`application/ports/platform` 承接看门狗和事件输出；旧 kernel 伞头、`kernel/instance.h`、`kernel/pipeline.h` 和旧 kernel 源文件已删除。`stategrid_context_t` 持有产品状态、FIFO 和资源链表，`resource_tool.c` 只接受这个 context。协议适配器通过显式 FIFO/context 参数和 `nd_dl698_msg_t`/`nd_dl645_msg_t` 工作；645 的消息类型在 `application/adapters/dl645/fr_645_message.h`，698 的消息类型在 `application/adapters/dl698/fr_698_message.h`。645 和 698 的 wrapper 都已删除，698 的接收/发送循环由 stategrid 组合根驱动。组件侧已有 `core.types`、`core.bus`、`protocol.dlms`、组件清单和 `tools/check_components.py`，产品描述在 `products/` 下。

## 2.4 公开 struct 棘轮

公开头里的 `typedef struct { ... } name_t;` 把字段布局交给所有消费者，消费者就能
`offsetof`、直接改字段，类型也就再也收不回不变量。判定分两类：

- 聚合：有生命周期（`*_create` / `*_destroy`），或必须维护不变量。定义放 `.c`，
  公开头只留 `typedef struct tag name_t;`。
- DTO：纯数据记录，没有生命周期，不维护不变量。可以留在公开头。

`tools/check_public_structs.py` 扫描 `domain/**/*.h` 和 `application/**/*.h`，按花括号
配对识别定义，注释和前置声明不计；CI 门禁是 `check:public-structs`。
`tools/public_struct_baseline.txt` 每行 `path name`，条目数和文件数以
`python3 tools/check_public_structs.py` 的 ok 行为准。基线是迁移债务，不是许可。

政策：

- 棘轮只减不增。聚合改成不透明时，同一 commit 删掉基线行。
- 新聚合必须不透明，不允许先进基线。
- 检查器分不出聚合和 DTO。新增 DTO 是显式例外，必须单独评审并写明为什么不能
  做成不透明；基线增行只能来自这个决定。
- 默认模式拒绝新定义和没有对应定义的基线条目，两者都提示 `--update`。
  `--self-check` 在临时树里证明新增和过期两个负例都失败。
- 盲区：扫描读原始头文本，不跑预处理器，宏生成的 typedef 看不到。

## 3. 有界上下文

| 上下文 | 职责 | 现状来源 |
|---|---|---|
| metering | 电能累计、瞬时量、需量 | `domain/metering/{energy,demand,measurement_average}.h` |
| tariff | 费率、阶梯、时段、电价 | `domain/tariff/{tariff,activity_calendar}.h` |
| events | 事件检测与记录 | `domain/events/*` |
| freeze | 冻结与记录游标 | `domain/freeze/{schedule,capture}.h` |
| load_control | 继电器与负荷控制 | `domain/load_control/{load_control,disconnect}.h` |
| display | 显示模型与轮显 | `domain/display/*` |
| security | 安全策略（ESAM、密钥、权限） | `domain/security/*` |
| upgrade | 升级流程 | `targets/stategrid_meter/upgrade` |
| protocol | 698 / 645 / DLMS 报文语义 | `application/adapters/{dl645,dl698}`；DLMS 在 `components/protocol/dlms`，未进产品构建 |

`protocol` 不是领域上下文。它把线上字节翻译成领域值对象，放在 `application/adapters/`。

## 4. 消费者定接口

端口由使用方声明。domain 需要什么，就在自己的头文件里声明；infra 在组合根里实现。

示例：计量上下文只需要时钟和存储，不要 kernel。

```c
/* domain/metering/ports.h */
typedef struct metering_clock_port {
    void *ctx;
    uint32_t (*now_sec)(void *ctx);
} metering_clock_port_t;

typedef struct metering_store_port {
    void *ctx;
    int (*read)(void *ctx, uint32_t offset, uint8_t *buf, size_t len);
    int (*write)(void *ctx, uint32_t offset, const uint8_t *buf, size_t len);
} metering_store_port_t;
```

目标目录里写 `metering_clock_port_t` 的具体实现，注入给聚合根。domain 头文件不 include 任何 infra 头文件。

## 5. 资源分配（user alloc，不限 RAM）

所有资源由调用方提供，domain 不调用 `malloc`。

```c
/* domain/common/allocator.h */
typedef struct nd_allocator nd_allocator_t;

void *nd_alloc(nd_allocator_t *a, size_t size);
void *nd_calloc(nd_allocator_t *a, size_t count, size_t size);
void  nd_dealloc(nd_allocator_t *a, void *p, size_t size);
```

资源不只有 RAM。持久化块、队列、定时器、IO 句柄、ID 段都走同一个资源提供者。

```c
/* domain/common/resources.h */
typedef enum {
    ND_RES_RAM,
    ND_RES_PERSIST_BLOCK,
    ND_RES_QUEUE,
    ND_RES_TIMER,
    ND_RES_IO_HANDLE,
    ND_RES_ID_RANGE
} nd_resource_kind_t;

typedef struct nd_resources nd_resources_t;

void *nd_res_acquire(nd_resources_t *r, nd_resource_kind_t kind,
                     const nd_resource_req_t *req);
void  nd_res_release(nd_resources_t *r, nd_resource_kind_t kind, void *handle);
```

`nd_allocator_t` 只是 `nd_resources_t` 在 RAM 上的特例，实现放 infra，注入放 targets。

每个聚合的构造签名固定为：

```c
metering_state_t *metering_create(nd_resources_t *res, const metering_config_t *cfg);
```

没有全局变量。每个实例独立创建，因此可重入、可多实例。

## 6. 组合根

`targets/<product>/main.c` 是唯一装配点。

```c
int main(void)
{
    nd_resources_t *res = target_resources_create(&target_memory_map);
    infra_t *infra = infra_create(res, &board_stategrid_meter);
    metering_state_t *metering = metering_create(res, &metering_cfg);
    application_t *app = application_create(res, metering, infra_ports(infra));

    target_wire_pipelines(app, infra);
    return target_run(app);
}
```

组合根负责：

1. 声明内存与持久化布局。
2. 用具体 infra 实现上层端口。
3. 创建聚合根与用例。
4. 注册消息管道、设备、映射表。
5. 启动 RTOS 或前后台主循环。

## 7. 迁移单元

每个单元都以“能跑测试”结束。

| 单元 | 内容 | 验证 |
|---|---|---|
| 0 | `tests/` + Unity + CMake | `ctest` 通过（已完成） |
| 1 | `domain/common`、allocator/resources 端口 | host 单元测试 |
| 2 | `domain/metering` 不透明聚合 + 序列化 | Unity 测试 + ARM 编译 |
| 3 | `domain/tariff` | Unity 测试 |
| 4 | `domain/events` + `domain/freeze` + `domain/report` + `domain/security` | Unity 测试 |
| 5 | `domain/load_control` | Unity 测试 |
| 6 | `application/adapters` 协议迁移 | 报文样本测试 |
| 7 | `infra` 目录重排 + 资源提供者实现 | ARM 编译 + 层检查 |
| 8 | `targets/stategrid_meter` 组合根 | 层检查 + 链接 |
| 9 | 删除旧接口（`applications.h` extern、`port.h` 字段宏、单块聚合根） | 层检查 + 全量构建 |

## 8. 验证手段

1. `tests/` 用 Unity，host gcc 跑逻辑测试。
2. `arm-none-eabi-gcc` 编 Cortex-M 目标代码，确认没有 host libc 依赖。
3. `tools/check-layers.sh` 静态检查 include 方向、domain 内的 `malloc`、全局变量。
4. 设备端字节布局回归：序列化后的字节必须与既有 EEPROM 布局对齐。

## 8.1 构建

新增的 `domain/`、`application/`、`infra/`、`targets/` 以 CMake + `arm-none-eabi-gcc` 为准。
现有 MSVC `.vcxproj` 和 IAR `.ewp` 大量指向移动前的旧路径，其中
`msta.vcxproj`、`msta_test.vcxproj`、`app_meterbase_baremetal.vcxproj` 已经是失效引用。
旧工程文件在和旧目录一起删除的单元里统一处理，不作为新代码的验证依据。


## 9. 待确认

1. 存量设备的 EEPROM 字节布局是否必须保持完全兼容？如果必须，`serialize` 需要复刻当前 `offsetof` 布局并加版本号。
