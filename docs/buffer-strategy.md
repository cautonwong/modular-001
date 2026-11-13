# 协议缓冲与 pbuf 策略

状态：2026-11-10 草案。

## 结论

不把 lwIP 的 `pbuf` 直接作为 698、645 和 DLMS 的内部类型，但多层分帧和分块确实需要链式缓冲能力。

建议分两步：

1. 保留 `nd_channel` 作为传输层字节流边界，现有零拷贝 FIFO 路径继续使用。
2. 在协议层定义传输无关的 `nd_frame`、`nd_block` 和 `nd_buf_chain`。`pbuf` 可以作为链式缓冲的一个后端，但不是协议组件必须依赖的类型。

如果产品已经有 lwIP，并且需要 IP 传输，`pbuf` 是合适的链式缓冲实现。没有 lwIP 的产品不为了 698/645 单独移植 pbuf。

## 多层协议与缓冲分层

698 和 DLMS 都不是单层协议。

| 层 | 698 | DLMS | 缓冲形态 |
|---|---|---|---|
| 传输层 | UART、PLC、红外、TCP | 串口、HDLC、TCP/UDP | 字节流，`nd_channel` |
| 链路层 | 0x68 帧、长度、控制、地址、HCS/FCS | HDLC 帧、地址、控制、FCS | 小帧，`nd_frame`，可引用 RX 缓冲 |
| 应用层 | APDU、分帧上报、分块 | APDU、块传输、分块 | 可能跨多帧，`nd_block` 或 `nd_buf_chain` |

链路层帧通常很小，698 约 544 字节，DLMS HDLC 更小。应用层 APDU 或块传输可以大于单帧，需要跨帧重组。

因此不能只用“一个连续帧缓冲”概括全部缓冲需求。正确的零拷贝路径是：

1. 链路层在接收缓冲里原地解析，产出一个 `nd_frame`，不复制帧内容。
2. 应用层把多个 `nd_frame` 链接成 `nd_block`，只在必须连续时线性化。
3. 加密、整体 CRC、整块压缩这类必须连续访问的操作，才调用 `nd_block_linearize` 一次。
4. 协议解析器使用游标在链上读取，不把整块复制到固定数组。

当前 698 的分段重组把数据复制进 `context_698_t.apdu_d_rx`，DLMS 也类似。这是后续可以用 `nd_buf_chain` 替换的复制点，不是现在就必须完成的重写。

## 理由

1. 这些协议不是 IP 包协议，但也不是单层字节流。链路层帧小，应用层块可能跨帧。缓冲契约必须同时支持连续帧和链式块，不能只按“小帧”设计。
2. 现有路径已经零拷贝到链路帧边界。`nd_dl698` 和 `nd_dl645` 通过 `nd_fifo_channel_fifo` 直接使用 UART FIFO，不复制整帧；`nd_channel` 只在组件边界表达读写语义。应用层分块重组仍然复制，这是 `nd_buf_chain` 要解决的问题。
3. `pbuf` 解决的是链式包和引用计数，不解决环形缓冲回绕。UART FIFO 是环形缓冲，要把它变成 `pbuf` 仍然需要自定义 `pbuf` 或线性化，零拷贝收益没有想象中大。
4. `pbuf` 带入 lwIP 的内存池、`mem_malloc`、引用计数、`SYS_ARCH_PROTECT` 和配置。目标 RAM 只有 24 KB 级的固定 arena，增加一套网络缓冲生命周期会挤压现有 698/645/DLMS 上下文。
5. 组件契约不应该绑定 lwIP 类型。domain 和 protocol 组件如果直接使用 `struct pbuf`，未来换网络栈或换传输都要改协议代码。

## 什么时候 pbuf 合适

- 产品需要 DLMS over IP、698 over TCP、Modbus TCP 或以太网升级。
- 系统已经引入 lwIP，IP 层和网络缓冲生命周期本来就要管理。
- 需要跨多个网络层分片、聚合或引用同一块数据。
- 需要统一管理多段应用层块，并且团队接受 lwIP 的引用计数和内存池模型。

在这些场景里，`pbuf` 可以作为 `nd_buf_chain` 的后端，但协议组件仍然只看见 `nd_frame`、`nd_block` 和游标，不直接操作 `struct pbuf`。

## 建议的缓冲契约

缓冲按层定义，不按传输实现定义。

```c
typedef struct nd_frame {
    uint8_t *data;
    size_t len;
    struct nd_frame *next;
    void *owner;
    void (*release)(struct nd_frame *frame);
} nd_frame_t;

typedef struct nd_block {
    nd_frame_t *first;
    size_t total_len;
} nd_block_t;

typedef struct nd_cursor {
    nd_frame_t *frame;
    size_t offset;
} nd_cursor_t;
```

- `nd_frame` 表示链路层帧，可以引用 RX 缓冲，也可以指向静态缓冲。
- `nd_block` 表示应用层 APDU 或块传输，由多个 `nd_frame` 链接而成。
- `nd_cursor` 提供跨帧的顺序读取，协议解析器不需要先线性化。
- `nd_block_linearize` 只在加密、整块 CRC 等必须连续访问时调用。

`nd_buf_chain` 可以用不同实现：

- 连续静态缓冲。
- UART FIFO 的连续窗口。
- 固定大小的帧池。
- lwIP `pbuf` 适配器。
- DMA 描述符适配器。

这样 pbuf 是一个可替换后端，而不是协议层的硬依赖。

## 迁移建议

1. 保持当前 `nd_channel` 和零拷贝 FIFO 通道。
2. 先建立 `nd_frame`、`nd_block`、`nd_cursor` 的契约和 host 测试，不急着接协议。
3. 698/645 的链路帧继续用现有 FIFO 窗口，不引入 pbuf。
4. DLMS 产品化时先用同样的通道和上下文，不引入 pbuf。
5. 当应用层分块重组成为实际瓶颈时，用 `nd_block` 链替换 `apdu_d_rx` 复制。
6. 只有新增 IP 传输产品时，才在 `infra/net/lwip` 中加入 lwIP，并实现 `nd_buf_chain` 到 `pbuf` 的适配。
7. 如果实测发现某条路径复制开销显著，再替换该路径的缓冲，不整体重写协议。

## 当前落地

`components/core/buf` 已经提供最小链式缓冲：

- `nd_frame_t` 表示一帧，可以引用现有缓冲，并带 release 回调。
- `nd_block_t` 把多帧链接成应用层块。
- `nd_cursor_t` 支持跨帧读取和跳过，不需要先线性化。
- `nd_block_copy` 只在调用方明确需要连续数据时复制。
- `nd_block_truncate` 支持分段乱序或重发时的块回退。

组件有 host 测试，覆盖跨帧读、跨帧跳过、容量截断、截断回退、释放回调和非法链入。

698 的分段重组已经改用 `nd_block`：`context_698_t` 里的 `rx_block` 链接最多 6 个段帧，段帧指向现有 `apdu_d_rx` 存储。当前仍是连续 APDU 存储上的链式视图，下一步可以把段帧指向独立接收缓冲，并让网关用 `nd_cursor` 解析，从而去掉整块复制。DLMS 的分段重组尚未切换。

## 验收标准

- 协议组件不 include lwIP 头。
- `pbuf` 只出现在 `infra/net/lwip` 适配层。
- 698/645 的链路帧零拷贝路径没有新增整帧复制。
- 应用层分块使用 `nd_block` 和 `nd_cursor`，只在必须连续时线性化。
- 新增 IP 产品只新增传输组件和产品组合根，不修改协议解析器。
