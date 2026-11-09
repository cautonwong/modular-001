# Blade 四层架构

状态：草案，等待勘察报告校订。
工作分支：`refactor/ddd-clean-arch`，合入 `modular`，不碰 `main`。

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

## 2. 数据形状优先

先定数据，再定接口。

- 聚合根是一个**不透明句柄**。定义留在 `.c` 文件里，公开头文件只有前置声明。
- 消费者拿不到结构体布局，因此不能 `offsetof`、不能直接改字段。
- 持久化交给聚合自己的 `serialize` / `deserialize`，不由 target 头文件用宏拼字段。
- 有界上下文之间只通过显式参数和返回值交换**值对象**，不共享可变状态。

当前的主要违规，也是重构的靶心：

1. `struct mbase_control_block_t` 定义在 `nidavellir/include/applications.h`，约 330 行，把计量、费率、事件、显示参数、上报配置、继电器状态混在一个聚合根里。
2. `instances/stategrid_meter/port.h` 用宏（`U_data_fields`、`I_data_fields`、`static_start`、`dyn_cache_start`）把 target 字段注入 domain 聚合根。domain 反向依赖了 target。
3. 持久化布局依赖对聚合根做 `offsetof`，字节布局与结构体字段顺序强绑定。
4. `applications.h` 是一个约 32 KB 的全局 extern 清单，应用通过它直接看见实现。

## 3. 有界上下文

| 上下文 | 职责 | 现状来源 |
|---|---|---|
| metering | 电能累计、瞬时量、需量 | `packages/meterbase/{energy,demand,electrical_measurement}.c` |
| tariff | 费率、阶梯、时段、电价 | `packages/meterbase/{touFee,gs_month_bill,gs_step_bill}.c` |
| events | 事件检测与记录 | `packages/meterbase/gs_*_event.c` |
| freeze | 冻结与记录游标 | `packages/meterbase/gs_{freeze,record}.c` |
| load_control | 继电器与负荷控制 | `packages/meterbase/relay_control.c` |
| display | 显示模型与轮显 | `packages/meterbase/{display,gs_backlight_ctrl}.c` |
| security | 安全策略（ESAM、密钥、权限） | `packages/{esam,crypto}` |
| upgrade | 升级流程 | `packages/upgrade` |
| protocol | 698 / 645 / DLMS 报文语义 | `packages/{dl698,dl645,dlms}`，属于 application 的适配器层 |

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
| 4 | `domain/events` + `domain/freeze` | Unity 测试 |
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

## 9. 待确认

1. 存量设备的 EEPROM 字节布局是否必须保持完全兼容？如果必须，`serialize` 需要复刻当前 `offsetof` 布局并加版本号。
2. `applications/` 下的协议应用是否允许保留独立可执行/可部署形态，还是统一收敛到 `application/` 分层？
3. `packages/kernel` 的 pipeline/filter/message 机制是保留为 infra 的消息总线，还是用更简单的接口替换？
