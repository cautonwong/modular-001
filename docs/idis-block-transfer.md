# IDIS object_list block transfer 设计

状态：设计草案（2026-11-16），未实现。对应 `/home/edge/IDIS_PLAN_2026-11-15.md` 的 T4。

## 1. 问题

`nd_idis_registry` 可以生成并加载 354 条对象。association 的 object_list 属性（class 15，
attribute 2）按 COSEM 元素编码后约 9.9 KiB。当前可用的主机编码缓冲是 4 KiB
（`targets/host/idis_host_main.c` 的 audit 路径），IDIS target 还没有定义传输层
APDU 缓冲；没有 block transfer 时，全量 object_list 一次装不下。

目标：只给 object_list 增加 GET block transfer，不引入通用块传输（GBT）、不改变 SET/ACTION。

## 2. 协议形状

采用 Green Book Ed.10 的 LN referencing 经典块传输（Table 81、Table 133）。

GET 请求（APDU 前缀）：

```
C0 01   NORMAL   invoke-id-and-priority, class-id(u16), LN(6), attribute-id(u8), [selector]
C0 02   NEXT     invoke-id-and-priority, block-number(u32)
```

GET 响应（APDU 前缀）：

```
C4 01   NORMAL   invoke-id, result
C4 02   DATABLOCK invoke-id, last-block(u8), block-number(u32), result
                  result CHOICE: [0] raw-data(octet string) | [1] data-access-error
```

规则：

- 第一次 NORMAL GET 命中需要分块的对象时，服务器返回 `C4 02`，`last-block=0`，
  `block-number=1`，数据从偏移 0 开始。
- 客户端发 `C0 02` + 期望的 block-number；服务器校验号码，返回对应块。
- 最后一块 `last-block=1`，之后清除该 association 的传输状态。
- 号码不符、没有活动传输、或传输已结束：返回 Data-Access-Error 的
  `data-block-number-invalid`，不返回数据。
- 收到新的 NORMAL GET 时，终止旧传输并从头开始。

## 3. 数据形状

消费者定义块来源，DLMS 层不认识 object_list：

```c
typedef struct {
    const uint8_t *data;
    uint16_t length;
    uint32_t number;   /* 从 1 开始 */
    int last;
} nd_dlms_block_t;

typedef struct {
    void *ctx;
    /* 把第 number 块写入 buffer；返回 0 成功，-1 无此块。 */
    int (*read)(void *ctx, uint32_t number, uint8_t *buffer,
                size_t capacity, nd_dlms_block_t *out);
} nd_dlms_block_source_t;
```

每个 association 增加固定大小的传输状态（不分配堆）：

```c
typedef struct {
    const nd_dlms_block_source_t *source;
    uint32_t next_number;
    uint32_t total_blocks;
    uint8_t active;
} nd_dlms_block_state_t;
```

状态归属 `nd_dlms_assoc_t`，在 release、AARQ 重连和错误时清零。`read` 的 `buffer`
直接指向 APDU 发送缓冲，避免第二份 1 KiB RAM。

## 4. 接入点

- `components/protocol/dlms`：新增 GET 请求解析、GET 响应编码和块状态机；
  `nd_dlms_server_get` 保持不变，NORMAL 路径先完整编码到临时 writer，写不下时
  改用块来源。
- `targets/idis_ip_meter`：为 association object_list 提供一个块来源，按 354 条
  注册表按需编码第 N 块；不需要一次性构造 9.9 KiB。
- 两个 IDIS target 的 `/cosem/` 副本必须同步（`check:idis-twins`）。

## 5. 预算

| 项 | 估算 | 依据 |
|---|---|---|
| 每 association 状态 | 16 B | 上述结构，1 个 association |
| 块缓冲 | 0 B | 复用 APDU 发送缓冲 |
| 编解码 + 状态机 ROM | 0.6–1.0 KiB | 11 个新函数，无表 |
| FreeRTOS ROM 余量 | 2136 B（收工实测） | 实现后必须重新测 |

块大小取 `min(1024, tx_capacity - header)`。若复用失败、必须新增 1 KiB 缓冲，
FreeRTOS 余量只剩约 1.1 KiB，此时把块大小降到 512 B 或先做 ROM 回收。

## 6. 测试矩阵

单元测试：

1. 1 块、2 块、最后一块的 `last` 标志。
2. 块号从 1 开始，连续请求拼装后与一次性编码逐字节相同。
3. 越界块号、未开始就 NEXT、最后一块之后再来 NEXT。
4. 新 NORMAL GET 中止旧传输。
5. 354 条注册表的块数、每块长度和总长度。

验收：

- `idis.object-list-block`：通过 host runner 的 GET 路径请求全量 object_list，
  校验条目数与首尾条目。
- 回归：没有块传输的普通对象仍走 NORMAL 响应。

## 7. 明确不做

- 通用块传输（GBT）、流式、窗口、块号确认。
- SET/ACTION 的块传输。
- 为 object_list 之外的属性自动启用分块；先只处理显式注册的块来源。
