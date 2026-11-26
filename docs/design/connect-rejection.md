# CONNECT 拒绝规则（698）

2026-11-24。适用于 `application/adapters/dl698/fr_698_connect.c` 的
`fr_handle_connect`。B5 修复了失败认证的回滚顺序。

## 规则

1. 合法认证机制：`ConnectMechanismInfo::=CHOICE` 的四个选择，
   `[0] NullSecurity`、`[1] PasswordSecurity`、`[2] SymmetrySecurity`、
   `[3] SignatureSecurity`，即 `connect_mode < Security_MAX`（0..3）。
2. 非法机制：`connect_mode >= Security_MAX`（4..255，含 4；CHOICE 没有 `[4]`）
   一律拒绝，不进入安全分发。
3. 通配服务器地址：解析时快照的 `ctx->sa_type_raw == SA_TYPE_GENERAL`
   一律拒绝。代码原注释即“通配地址 拒绝服务”。帧层
   `fr_apdu_effective_check` 会把 `ctx->SA` 的类型位掩掉（回帧地址规范化），
   所以拒绝判断必须用掩码前的快照，不能改掩码的全局语义。组地址和广播
   地址的处理不在本记录范围，保持现状，另行评估。
4. 拒绝应答：写一个完整的 CONNECT-Response，连接响应对象
   `ConnectResponseInfo { 认证结果 ConnectResult, 认证附加信息 OPTIONAL }`
   的认证结果取“其他错误”(255)，附加信息写 0。响应中的协商参数取**服务端
   当前 cfg 值**，不采用请求里的期望值。实现是
   `__connect_reject` helper，与安全函数的应答布局一致。
5. 状态：非法/通配路径不写 `cfg`（请求中的 mode、一致性块、帧尺寸和超时
   都不落地），不更新 `nd_mb_clientaddr`、连接状态表和 ESAM 超时，不建立
   应用连接。因为 cfg 未被改写，这里没有额外的回滚动作；认证失败路径仍走
   B5 的整份 cfg 快照/恢复。
6. 通配 SA 的帧行为：若正式标准确认该地址应在
   链路层静默丢弃，只改 helper 的调用点，安全结论（拒绝、不建连）不变。
   该点为 TBD，需要标准原文确认。

## 条款依据与置信度

- 建立应用连接服务（CONNECT-Request / CONNECT-Response）、
  `ConnectMechanismInfo` 的四个选择、`ConnectResult` 枚举
  （允许建立应用连接(0)、密码错误(1)、对称解密错误(2)、非对称解密错误(3)、
  签名错误(4)、协议版本不匹配(5)、其他错误(255)）：按 DL/T 698.45 的
  服务与数据类型定义确认，置信度高。
- 2026 版对应章节 §7.4.3 建立应用连接（Tables 41/42 与密钥协商）。
  2017 版章节号未在仓库内核对（仓库没有标准原文，验收表也没有 698 一致性
  用例，见 `docs/firmware-acceptance-spec-notes.md`），引用前需对照正式版。
- 通配/广播地址的条款号未确认，置信度低；规则来自代码既有意图和点对点
  应用连接的语义。

## 帧层“交 handler 拒绝”契约

`ConnectMechanismInfo` 的未知 choice（如 4/5）无法确定机制负载长度，帧层
不能可靠定位时间标签。`get_CONNECT_Request_len` 对未知 choice 返回
`FRM_LEN_DEFER`（`fr_698_internal.h`），语义只有一条：**长度不可判，交给
service handler 拒绝**，不表示任何服务或最大长度。帧层只在固定体
（`sizeof(struct connect_request_t)`，packed 38 字节，`_Static_assert` 钉住）
完整时才返回该值，否则返回 -1 走 ERROR_S。

`fr_timetag_check` 收到 `FRM_LEN_DEFER` 时把 `timeTag_valid` 置 0 并跳过
时间标签与长度等式；gatekeeper 只区分“哨兵→照常分发”和“其他负值→
ERROR_S”，不含 CONNECT 语义。handler 在读取机制负载前按
`connect_mode >= Security_MAX` 拒绝，因此拒绝路径不需要可信的时间标签。
这是有意的 fail-closed：拒绝不改状态，跳过时标校验没有副作用。

## 可选尾部边界

`fr_timeTag_optional` 的两处可选写入都以 `cache->wr < cache->buf_size` 为
前提：跟随上报标志字节，以及 11 字节时间标签或其 0x00 占位符。空间不足时
丢弃该可选尾部，不写占位符；`timeTag_valid` 分支还要满足
`apdu_cache_getFree(cache) > 11`，避免尾满时 `uint16_t` 下溢绕过检查。
`wr == buf_size` 时函数不再写任何字节，返回 1。回归见
`test_fr_698_time_tag_optional_drops_tail_at_buffer_end` 和
`_writes_nothing_when_full`。

## 应答兜底（gatekeeper）

分发 switch 之后以 `cache->wr` 为基线：CONNECT、GET、SET、ACTION 的 handler
没有让 wr 增长时，写一个 ERROR_S-Response 兜底；失败响应的起点单独记录为
响应头位置，不把 GET/SET/ACTION 的 OAD 数量字节当成响应头。RELEASE 的
handler 原地改 cache 最后两字节、wr 不变，是文档化例外；REPORT_Response 按
协议不响应，分发内提前返回；SECURITY_Response 自己写固定帧；default（不支持
的服务或时间标签解析失败）走同一个 helper，DAR 取 `ctx->err_n`（当前“服务
不支持”=2，DAR 映射 TBD）。helper 同时把 `ctx->service` 置为
ERROR_S_Response，否则 no-write 路径的 service 仍是原请求，帧会重新变成
5 字节。
最小 ERROR_S 帧为 `[0xEE, PIID, DAR]`，**不带跟随上报标志和时间标签**：
`fr_timeTag_optional` 对 ERROR_S_Response 提前返回，与 SECURITY_Response
一致。时间标签是否可选/必须仍是 TBD，需要标准原文；正式版若要求时间标签，
只在 helper 里追加，不改变“拒绝并回错”的结论。正常响应仍追加跟随上报
0x00 和可选 11 字节时间标签，`test_fr_698_frame.c` 有回归。

## 残余（另立单元，不在本批改）

1. `__common_process`（NullSecurity）恒写并返回“其他错误”(255)，明文连接
   实际永远失败；注释“不支持数字签名”与该函数不符。需产品确认明文连接
   是否禁用；若允许，认证结果应为 0。
2. `ConnectResult` 的“协议版本不匹配”(5) 没有实现：`pro_ver` 未校验。
3. 不支持服务的 DAR 目前沿用 `ctx->err_n`（2），是否映射到更准确的 DAR
   待产品/标准确认。

## 测试

`tests/unit/test_fr_698_connect_len.c`：
`test_698_connect_rejects_mechanism_four`、`_five`、`_general_address`
三条拒绝用例；断言响应末尾 `[255, 0]`、`wr` 增长、响应首字节 0x82、
cfg 的 mode 与一致性 canary 不变、clientaddr 与连接状态未写。
`_defers_unknown_security` 和 `_rejects_short_fixed_body` 钉住帧层契约。
B5 的 `test_fr_698_connect_failure_restores_connect_mode` 作为回滚回归保留。

`targets/stategrid_meter/tests/acceptance/test_dl698.py`：
ACC-698-014/015/016 走 firmware_sim 的 UART 0，覆盖 mode 4、mode 5 和
通配 SA。断言 CONNECT-Response 结果 255、协商参数等于当前 cfg（0x0200）、
不等于请求里的 0x0100，证明没有协商。
