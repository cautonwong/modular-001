# 端口绑定模板

端口表达业务需要的一个动作。它不是设备驱动模型。仓库里没有 device ops、驱动注册表或运行期设备工厂，也不为"一个版本服务多个产品"设计抽象。一个 board 服务一个产品，产品在构建期选定，组合根直接调用具体实现。

## 1. 什么时候需要端口

domain、application 或 components 需要一项平台能力，但不能 include 具体实现时。

例子。背光开关、继电器反馈、参数读写、存储读写、上报发送。

不需要端口的情况。能力只在一个 target 内部使用。那就直接调用，不要为单次使用造接口。

## 2. 生成器

```sh
python3 tools/new_port.py --capability backlight \
  --action "set(bool enable)" --action "read(uint8_t *buffer, uint16_t length)"
```

生成 `application/ports/backlight.h`，只包含能力结构体和动作函数指针。动作按 `name(type arg, ...)` 传入，返回值固定为 `int`。生成后写 target 适配表、在组合根把表传给消费者的构造函数、补假端口测试。生成器不产生 `.c`、绑定函数或全局 `bound`。

## 3. 端口头文件

位置 `application/ports/<能力>.h`。

```c
#ifndef ND_APPLICATION_PORTS_EXAMPLE_H
#define ND_APPLICATION_PORTS_EXAMPLE_H

#include <stdbool.h>
#include <stdint.h>

/* 业务需要的动作，不出现设备类型、寄存器名或厂商头。 */
typedef struct nd_example_port {
    void *ctx;
    int (*read)(void *ctx, uint8_t *buffer, uint16_t length);
    int (*write)(void *ctx, const uint8_t *buffer, uint16_t length);
} nd_example_port_t;

#endif
```

消费者在构造函数里接收 `const nd_example_port_t *`，存进自己的不透明结构体，调用 `port->read(port->ctx, ...)`。没有绑定函数，也没有文件级可变指针。头文件不声明包装函数。

规则。

- `ctx` 不透明。头文件不解释它指向什么。
- 函数按业务动作命名。不要 `dev_ops`、`ioctl`、`board_ops`。
- 返回值用 `int`，0 表示成功，负数表示失败。
- 头文件不 include `targets/` 或 `infra/soc`。
- 端口按业务能力增长，不按板子数量增长。

## 4. target 侧适配

位置 `targets/<t>/app/<能力>_adapter.c`。

```c
#include "application/ports/example.h"
#include "targets/<t>/board/board_io.h"

static int example_read(void *ctx, uint8_t *buffer, uint16_t length)
{
    (void)ctx;
    return board_example_read(buffer, length) == 0 ? 0 : -1;
}

static int example_write(void *ctx, const uint8_t *buffer, uint16_t length)
{
    (void)ctx;
    return board_example_write(buffer, length) == 0 ? 0 : -1;
}

const nd_example_port_t *target_example_port(void)
{
    static const nd_example_port_t port = {
        .ctx = NULL,
        .read = example_read,
        .write = example_write,
    };
    return &port;
}
```

适配函数只做参数转换和错误码转换。业务判断留在 domain。

## 5. 组合根

组合根把适配表传给消费者的构造函数，消费者自己保存指针。

```c
consumer_t *consumer = consumer_create(alloc, target_example_port());
```

端口在构造时注入，先于第一次使用。host 测试构造时传假端口即可，不需要运行期注入框架。

## 6. 测试

- 端口消费者把假端口传给构造函数做单元测试，放在 `tests/unit/test_<模块>.c`。
- 适配层由 target 验收覆盖，见 `docs/guides/acceptance-case.md`。
- 断言可观察的返回值，不断言内部调用顺序。

## 7. 检查清单

- [ ] 端口只表达业务动作，没有设备类型或厂商头。
- [ ] `ctx` 不透明，头文件独立可编译。
- [ ] 端口由组合根构造注入，先于第一次使用，没有绑定全局。
- [ ] 适配函数不含业务判断。
- [ ] 有假端口的单元测试。

## 8. 绑定现状与 no-legacy-bind 检查设计（2026-11-23）

第 2 节起的新端口用构造注入，没有绑定函数。platform、record_storage 和
resource_access 已迁移为构造注入；application/ports 里已没有绑定函数和
绑定全局。R4 删除了 resource_access 的 12 个旧包装、`nd_resource_bind` 与
文件级 `bound`，`resource_tool_attach` 只填 `ctx->resource`。R5a 删除了 timer
和 backlight 的 `nd_*_bind` 与文件级 `bound`，R5b 删除了 relay_status 和
metering_input 的同一套旧绑定；四个 domain 端口全部改为组合根填 context：

| 端口 | 全局 | 绑定函数 | 调用者 | 状态 |
| --- | --- | --- | --- | --- |
| `application/ports/resource_access.c` | 无（R4 删除） | 无（R4 删除） | `resource_tool_attach` 只填 `ctx->resource` | 已迁移 |
| `domain/common/timer.c` | 无（R5a 删除） | 无（R5a 删除） | `stategrid_timer_attach` 只填 `ctx->timer` | 已迁移 |
| `domain/display/backlight.c` | 无（R5a 删除） | 无（R5a 删除） | `stategrid_backlight_attach` 只填 `ctx->backlight` | 已迁移 |
| `domain/load_control/relay_status.c` | 无（R5b 删除） | 无（R5b 删除） | `stategrid_relay_status_attach` 只填 `ctx->relay_status` | 已迁移 |
| `domain/metering/metering_input.c` | 无（R5b 删除） | 无（R5b 删除） | `stategrid_metering_input_attach` 只填 `ctx->metering_input` | 已迁移 |

R5b 之后没有任何产品保留旧绑定，`tools/port_bindings.txt` 无行，
`tools/port_binding_exceptions.txt` 已删除：两个 stage 都走 0 行路径，
任何 `nd_`、`stategrid_`、`dl698_` 或 `resource_tool_` 前缀的 `*_bind` 函数符号即失败。record_storage 的
10 个使用点（`fr_645.c`、`fr_698_connect.c`、`fr_axdr.c`，3 文件）已改为在
调用点经 `nd_fr698_record_storage(instance)` / `nd_fr645_record_storage(instance)`
取端口，`nd_record_storage_bind` 与两处可变全局（`record_storage.c:bound`、
`record_storage_adapter.c:port`）已删除。

### 8.1 no-legacy-bind 检查

目标：旧绑定既不能漏绑/多绑，也不能在迁移完成后悄悄回流。数据来源是链接
后的固件 ELF，不是源码文本：

1. 表 `tools/port_bindings.txt` 有该产品的行时，用 `objdump -d` 取直接
   `bl`（ARM）/ `call`（host）边，按行统计指向该 bind 的调用数；0 次、
   >1 次、调用者不在允许列表、或只有间接调用都失败。
2. 产品没有行时（R5b 后表为空），用 `objdump -t` 扫 ELF 定义的函数符号；
   任何以 `nd_`、`stategrid_`、`dl698_` 或 `resource_tool_` 开头、以 `_bind`
   结尾的函数符号失败。
3. `--self-check` 覆盖 0 次、多次、调用者不符、scratch 重复绑定、仅间接
   调用、空表 + 四类前缀各自的合成 `*_bind`（必须失败）、空表 + 非前缀
   `*_bind`（必须通过）。

已落地：`check:port-bindings-sim` 检查 host `firmware_sim`，
`meter_electricity_single_phase_cn:port-bindings` 在 `./ci.sh` 里检查 stategrid
的 ARM ELF（由 `tools/check-firmware-link.py --elf` 保留）。R5b 后两个 stage
都走 0 行路径，打印 `0-bind: no guarded *_bind symbol`。旧 ELF 会被新扫描判失败：
R5b 前的 stategrid ELF 实测报 `nd_relay_status_bind` 和
`nd_metering_input_bind`，更早的 ELF 报 `nd_resource_bind`。

这个检查只覆盖旧绑定端口；新端口没有绑定函数，`exactly-once` 由构造注入
在类型上成立（消费者只收到一个指针），不需要运行期检查。

### 8.2 最小 DI seam（P0-2 第一批）

做法按端口逐个迁移，顺序 platform → record_storage → resource_access。
三个端口都已迁移：

- 消费者函数接收 `const nd_<能力>_port_t *`，存进自己的不透明结构体；
  删除 `bound` 和 `*_bind()`。
- platform 已迁移：`stategrid_context_t` 持有 `nd_platform_port_t` 值，组合根
  `stategrid_platform_attach` 填表，目标侧用 `stategrid_platform_port(ctx)` 取
  端口。698 适配栈经 `fr_698_env.h` 的 `nd_fr698_platform(instance)` 在调用点
  取端口，不改 698 调用链。11 个调用点已改，`nd_platform_bind` 与两处可变
  全局已删除，冻结表删掉 platform 行。
- record_storage 已迁移：`stategrid_context_t` 持有 `nd_record_storage_port_t`
  值，`stategrid_record_storage_attach` 只填实例字段，目标侧用
  `stategrid_record_storage_port(ctx)` 取端口；698 栈经
  `nd_fr698_record_storage(instance)`、645 栈经
  `nd_fr645_record_storage(instance)` 在调用点取端口。10 个调用点已改，
  `nd_record_storage_bind` 与两处可变全局已删除，冻结表只剩 resource_access 行。
- resource_access 是 legacy 内核接口（accessor 已带 `void *ctx`），迁移最大，
  最后做。R0 已落地：`stategrid_context_t` 持有 `nd_resource_port_t` 值，
  `resource_tool_attach` 填 `ctx->resource` 后再调用 `nd_resource_bind`；目标侧
  用 `stategrid_resource_port(ctx)` 取端口，`nd_fr698_resource_access(instance)` /
  `nd_fr645_resource_access(instance)` 两个 accessor 由 R0 提供；698 侧在
  R0/R1 尚未被引用（ARM GC 丢弃，host sim 保留符号）。
- R1 已落地：mapper_645 的 120 个调用点（12 文件、65 个函数）在函数入口提升
  一次 `const nd_resource_port_t *port = nd_fr645_resource_access(instance);`，
  每个调用点改用 `resource_access.h` 新增的端口优先入口
  `nd_<动作>_port(port, ...)`。入口先查空端口和空回调（返回 -1），再调
  `port->fn(port->ctx, ...)`。旧 `nd_<动作>` 包装、`nd_resource_bind` 和
  `bound` 留到 R4，继续服务 mapper_645 以外的调用点；ARM ELF 上
  `nd_resource_bind` 仍恰好 1 次（调用者 `resource_tool_attach`）。ARM 账：
  text +560 B（ELF 保留的 7 个入口 190 B、65 次 hoist 调 accessor 约 260 B、
  链接 accessor 12 B，其余为寄存器压力与对齐；逐个调用点取 accessor 的写法
  更贵），data 和 bss 不变。未绑定 instance 的调用现在返回 -1，不再回落到
  旧全局 `bound`；组合根在调用前填 `ctx->resource`。
- R2 已落地：mapper_698 的 193 处文本调用点（19 文件、49 个函数，其中 2 处
  是注释里的旧示例）在函数入口提升一次
  `const nd_resource_port_t *port = nd_fr698_resource_access(instance);`，每个
  调用点改用端口优先入口。mapper_698 只用 6 个动作（get_index、set_index、
  set_offset、log_output、get_index_e2p、set_index_e2p），R1 已全部提供，R2
  没有新增入口。旧 `nd_<动作>` 包装、`nd_resource_bind` 和 `bound` 留到 R4；
  ARM ELF 上 `nd_resource_bind` 仍恰好 1 次（调用者 `resource_tool_attach`）。
  ARM 账：text 207,072 → 207,656 B（+584 B，其中 accessor 12 B、49 次 hoist
  约 570 B；调用点首参替换不增加指令，余量为对齐），data 584 B 和 bss
  30,212 B 不变。
- R3 批次 1 已落地：app 层 11 文件、61 个函数、138 个活动调用点改为在函数入口
  提升一次 `const nd_resource_port_t *port = nd_fr698_resource_access(instance);`
  后调用端口优先入口。文件：`metering/electrical_measurement.c`（15 函数）、
  `freeze/legacy/gs_freeze.c`（11）、`events/legacy/gs_record.c`（8）、
  `adapters/fr_645_report.c`（8）、`metering/legacy/energy.c`（5）、
  `apps/app_meter_base_entry.c`（3）、`events/legacy/gs_meter_program_event.c`
  （4）、`events/legacy/gs_rtc_err_event.c`（3）、`stategrid_meter_entry.c`（2）、
  `tariff/legacy/touFee.c`（1）、`application/adapters/dl698/fr_axdr.c`（1）。
  批次 1 的实际调用点需要 R1 曾以“无调用者”删掉的 4 个入口
  （`log_clean`、`log_append`、`log_append_revise`、`get_offset`），本次按真实
  调用者补回，入口仍是先查空端口和空回调再 `port->fn(port->ctx, ...)`；没有
  新增无调用者的入口。旧 `nd_<动作>` 包装、`nd_resource_bind` 和 `bound` 留到
  R4。ARM 账：text 207,656 → 207,936 B（+280 B，含 61 次 hoist 与 4 个入口），
  data 584 B 和 bss 30,212 B 不变；ELF 335,956 字节、233 个对象；
  `nd_resource_bind` 仍恰好 1 次（调用者 `resource_tool_attach`）。
- R3 批次 2 已落地：5 个普通文件共 7 个活动调用点、7 个函数在函数入口提升一次
  accessor 后改调端口优先入口（`display.c` 的 Key_Display、Get_DisplayPage；
  `meter_base_adapter.c` 的 2 个保存函数；`relay_control_adapter.c` 的
  save_relay_state；`esam_status_adapter.c` 的 nd_esam_status_meter_no_update；
  `fr_645.c` 的 Password_Check，用 `nd_fr645_resource_access(instance)`）。
  3 个回调适配文件的 5 个仅拿 `ctx` 的函数也迁移：注册侧把不透明 `ctx` 槽
  改成 `&instance->resource`（`dl698_parameter_attach` 的 `port.ctx`、
  `metering_energy_ledger_from_instance` 的 `parameter_store.ctx`、
  `tariff_lookup_reload` 构造 store 处的一次 `stategrid_context_t` 转换），
  回调里 `ctx` 直接当 `const nd_resource_port_t *` 调端口优先入口。不需要全局、
  service locator，也不改消费者接口。4 处注释
  里的旧示例（`gs_freeze.c` 1、`gs_record.c` 2、`fr_645_report.c` 1）改为
  端口优先写法。ARM 账：text 207,936 → 207,904 B（-32 B），data 584 B 和
  bss 30,212 B 不变；ELF 335,812 字节、233 个对象；`nd_resource_bind` 仍
  恰好 1 次（调用者 `resource_tool_attach`）。
- 批次 2 之后，产品代码里旧 `nd_<动作>` 文本调用为 0。旧包装、
  `nd_resource_bind` 和 `bound` 留到 R4，只为 R4 清单服务：
  `test_mbase_search_counts.c` 的 2 处 `nd_log_output`、`fr_698_comm.c` 的
  `WIN32_U_TEST` 旧定义、IDIS 两个 `legacy_stubs.c` 的 `nd_get_index`；清单
  不变。`test_metering_energy_storage_fault.c` 的 `nd_resource_bind` 夹具也
  保留到 R4，参数备份读取改由 harness 实例的 `resource` 字段提供端口。
- R4 已落地：删除 12 个旧 `nd_<动作>` 包装、`nd_resource_bind` 和文件级
  `bound`；`resource_tool_attach` 只填 `ctx->resource`。`test_mbase_search_counts.c`
  的 2 处 `nd_log_output` 改为 `nd_log_output_port(&instance.resource, ...)`，
  读计数断言不变（15/19 mark reads，slot 读仍一记录一次）。
  `test_metering_energy_storage_fault.c` 去掉 `nd_resource_bind(NULL)` 和两处
  `nd_resource_bind(&test_port)`，只保留 `harness.instance.resource = test_port`。
  `fr_698_comm.c` 删除 `WIN32_U_TEST` 的 `nd_get_index`/`nd_set_index` 桩和
  声明。mutable globals 243 → 242。ARM 账：text 207,904 → 207,880 B（-24 B），
  data 584 B 不变，bss 30,212 → 30,204 B（-8 B），exidx 8 B；ROM 208,496 →
  208,472 B（-24 B）；ELF 335,812 → 335,716 字节、233 个对象。IDIS 的
  `legacy_stubs.c` 在 R5b 只做可达性分析，不改：两个 target 各一个
  `nd_get_index` 桩（同文件还定义 `nd_meter_status_metertype` /
  `nd_meter_status_emu_error_set`），无调用者、被 `--gc-sections` 丢弃、
  最终 ELF 无符号，IDIS 也不链接 `application/ports`。后续 legacy-stubs 单元
  已删除两个 `legacy_stubs.c` 和 `APP_SOURCES` 引用；四个 IDIS ELF（两个
  target × POLL/IRQ）均无这些符号，ROM 四个 lane 不变（95,068 / 97,552 /
  99,288 / 101,968 B）。在有 IDIS 消费者之前不给它接真实端口。
- R5a 已迁移 timer 和 backlight：`stategrid_context_t` 分别持有
  `nd_timer_port_t` 和 `nd_backlight_port_t` 值，`stategrid_timer_attach` /
  `stategrid_backlight_attach` 接收实例并只填字段，目标侧用
  `stategrid_timer_port(ctx)` / `stategrid_backlight_port(ctx)` 在调用点取端口
  （`touFee.c` 1 处、`display.c` 2 处、`display_control_adapter.c` 1 处）。
  两处 `bound`、两个静态 port 和两个 `nd_*_bind` 已删除；静态 port 是
  `static const`，本来就不在 mutable 计数里，所以 mutable globals 242 → 240
  （只减两个 bound），冻结表删掉两行。ARM 账：text 207,880 不变，bss
  30,204 → 30,196（−8），ELF 335,716 → 335,400（−316），ROM 208,472 不变；
  context +16 B 在 arena 内。
- R5b 已迁移 relay_status 和 metering_input：`stategrid_context_t` 持有
  `nd_relay_status_port_t relay_status` 和 `nd_metering_input_port_t
  metering_input` 值，两个 stategrid adapter 接收实例并只填字段；wrapper
  改为显式端口首参（`nd_relay_status_is_off(port)`、
  `nd_metering_input_read(port, input, value)`），消费者在调用点用
  `stategrid_relay_status_port(instance)` / `stategrid_metering_input_port(instance)`
  取端口（`runstatus.c` 1 处、`electrical_measurement.c` 2 处）。两处
  `bound`、两个静态 port 和两个 `nd_*_bind` 已删除，mutable globals
  240 → 238，冻结表和 `tools/port_binding_exceptions.txt` 一并删除，
  `tools/port_bindings.txt` 无行，任何四类前缀的 `*_bind` 符号失败。ARM 账：
  text 207,880 → 207,864（−16），data 584 不变，bss 30,196 → 30,188（−8，
  两个 bound 指针），ROM 208,472 → 208,456（−16），ELF 335,400 → 335,064
  （−336）、233 个对象不变；两个端口值在 arena 内的 context 里。
- P1 已落地：组合根适配函数统一从 `*_bind` 改名为 `*_attach`，即
  `stategrid_{platform,record_storage,timer,backlight,relay_status,metering_input}_attach`、
  `dl698_parameter_attach` 和 `resource_tool_attach`；调用点
  （`stategrid_meter_entry.c`、`tests/sim/sim_main.c`、
  `test_mbase_search_counts.c`）与文档同步。8.1 的 0 行扫描收紧到
  `nd_`、`stategrid_`、`dl698_` 和 `resource_tool_` 四类前缀，self-check
  为每个前缀加合成负例并要求非前缀 `sim_uart_bind` 通过。改名纯机械：
  text 208,048、data 544、bss 30,168 不变，ELF 334,884 → 334,896（+12 B，
  仅符号名变长）。
- 迁移前先落 8.1 的检查，保证旧绑定在过渡期不漂移。

### 8.3 根 context（IDIS）

IDIS 的可变状态收在一个根 context `g_idis_app` 里
（`targets/idis_ip_meter*/app/main.c`）：port 表、对象聚合、引擎存储、脉冲
累加和循环标志都在这个聚合内，`board_ports.c` 的硬件缓冲和存储属于另一个
翻译单元。两个 IDIS target 都已落地。
`tools/mutable_globals_baseline.txt` 里每个 IDIS 的 main.c 路径应恰好只有
一条 `g_idis_app`。该项没有 checker，由 verifier 验收时核对。
