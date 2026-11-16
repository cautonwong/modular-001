# 记录存储端口设计

## 1. 背景

`domain/events/legacy/gs_record.h` 声明了一批 target 侧记录存储函数。两个
application 适配器直接 include 它：

- `application/adapters/dl698/fr_698_connect.c`
- `application/adapters/dl698/fr_axdr.c`

这违反依赖方向。application 应该依赖消费者定义的端口，target 实现端口。
`gs_record.h` 目前还被 18 个 target 文件使用，不能直接删除。

## 2. 现有数据形状

`record_storage_id_info_t` 是记录存储的地址和长度描述。

```c
struct record_storage_id_info_t {
    uint32_t save_index;
    uint32_t sequence_index;
    uint32_t capture_index;
    uint32_t period_index;
    uint32_t depth_index;
    uint16_t capture_len;
    uint16_t period_len;
    uint16_t depth_len;
    uint32_t rpt_flag;
    uint32_t rpt_mode;
    uint32_t *acc_time;
    const uint32_t *record_head;
};
```

`format_conversion_t` 描述记录在协议格式和存储格式之间的转换函数。application
适配器用到的操作包括：

| 操作 | 用途 |
|---|---|
| `get_record_id_info` | 由 OAD 解析存储地址和长度 |
| `check_record_table_oad__support` | 判断 OAD 是否有记录表 |
| `record_data_package` | 读取记录并打包成 698 响应 |
| `record_package_01` | 打包 645/698 记录数据 |
| `record_change_format` | 协议格式和存储格式互转 |
| `select_record_time_check` | 时间选择条件校验 |
| `record_response_null` | 空记录响应 |

## 3. 端口草案

application 定义值对象和端口，target 实现，domain/legacy 删除。

```c
/* application/ports/record_storage.h */
typedef struct nd_record_info {
    uint32_t save_index;
    uint32_t sequence_index;
    uint32_t capture_index;
    uint32_t period_index;
    uint32_t depth_index;
    uint16_t capture_len;
    uint16_t period_len;
    uint16_t depth_len;
    uint32_t rpt_flag;
    uint32_t rpt_mode;
    const uint32_t *record_head;
} nd_record_info_t;

typedef struct nd_record_storage_port {
    void *ctx;
    int (*info)(void *ctx, uint32_t oad, nd_record_info_t *out);
    int (*read)(void *ctx, uint32_t oad, uint32_t index, uint8_t *out,
                size_t capacity, size_t *length);
    int (*supports)(void *ctx, uint32_t oad);
} nd_record_storage_port_t;
```

格式转换函数是编解码逻辑，优先放到 application 的 codec 层，不放进端口。
`acc_time` 指针和可变记录头需要在实现里转成值对象，避免 application 拿到
target 内部地址。

## 4. 迁移步骤

1. 盘点 `fr_axdr.c` 和 `fr_698_connect.c` 实际使用的字段和函数，按读取、写入、
   支持判断分组。
2. 在 `application/ports/record_storage.h` 定义值对象和端口。
3. 在 target 的 `gs_record` 适配器实现端口，把 mbase 地址和环形区细节留在
   target。
4. 迁移两个 application 适配器到端口，保留现有响应格式。
5. 把仍属于 target 的声明移到 target 头，删除 `domain/events/legacy/gs_record.h`。
6. 加 host 测试，用 fake 端口验证打包和校验逻辑。

## 5. 门禁

- `check_platform_boundary.py` 无新增 baseline。
- `check_components.py` 通过，application 不再 include target 头。
- stategrid `firmware_sim` 和 `firmware_acceptance` 通过。
- 记录读取和冻结的现有验收用例保持绿色。

## 6. 未决问题

- 记录存储地址是持久化布局的一部分，端口不能暴露偏移。
- 645 和 698 的打包差异是否放进同一个端口，还是分成两个消费者端口。
- `acc_time` 是可写指针，需要确认是否由 target 独有。
- 块传输和分帧长度由谁决定。
