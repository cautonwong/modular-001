# 策略：预算化可恢复游标（cursor/scheduling-first）

状态：设计完成，未实现。只采用它的 snapshot 过期规则和后续 `GET-NEXT` 续传
构想；per-pass 上界依赖尚不存在的传输能力，不作为当前默认。

## 问题

即使每次查询只有 36 次填充，一次 GET 仍然在同一调用里跑完。要做“每次主循环
最多 B 次读”的硬上界，查询必须可恢复，且传输必须能把请求挂着。

## 算法

1. `begin(selection) -> cursor`、`step(cursor, budget) -> {rows, more}`、
   `abort(cursor)`。每次 step 最多 B 次 reader read，状态存在调用方持有的
   不透明块里。
2. 选择语义协议无关：scope（all/entry window/last-n/time window）、最多两个
   列值过滤、投影列、排序。消费者提供 `matches`/`on_count`/`on_row`/
   `on_done`/`on_error` 回调。
3. 快照：游标建立时记下 `{head, entries, sequence}`；每次读之前用
   `sequence delta > phys - entries - sector_slots` 判定环是否已经回收了
   未读槽，过期就 abort 并返回协议错误。
4. count 与 emit 分开：先数一遍发布 count，再逐块 emit；块之间允许 append。

## 接口

`begin/step/abort` 加策略表；同步路径等于预算无限的同一个游标。TLA+ 模型有
`BudgetBound` 和 `NoReadAfterTouch`。

## 成本

- RAM 约 820 B/活动会话（一个槽、单关联），不新增缓冲；ROM 0.6-1.2 KiB 估计。
- 每行仍是 1-2 次 reader read，不减少总量，只把它摊到多个 pass。
- 满环后任何 append 都可能擦掉未读扇区，所以 60 s 采集周期内必须结束，
  否则 abort。

## 为什么不是默认

- **上界是条件的。** NORMAL GET 在游标拿到 count 之前不能返回，块源要能返回
  `NOT_READY`；延迟响应和 `NOT_READY` 都不存在。没有它们，GET 处理器会一次跑
  完游标，上界消失。
- **SPI 总线没有串行化。** 查询路径与 save worker 共用总线，需要事务级锁。
- **放不下。** 820 B/会话加 0.6-1.2 KiB ROM，对比 pinned FreeRTOS 588 B 余量
  （当时）和约 7.5 KiB RAM 余量。
- 设计自带的示例还引入了 `static nd_axdr_writer_t block_out`，违反不新增可变
  全局。

## 值得保留的部分

- **snapshot 过期规则**：cache-first 的多块响应一旦引入，需要它来防止环回收
  造成 count 与行不一致。
- **698 `GET-NEXT` 续传**：多帧响应复用同一游标，不再整段重扫；等
  `__fr_handle_get_next_request` 落地后再实现。

## 测试（设计）

- 每 step 的 reader read 计数 ≤ B；cursor 单调；count == 已 emit 数。
- 环回收、reset、RTC 回拨、掉电后游标失效。
- 698 event/frozen/collection/多帧的续传用例。
- 形式化：TLA+ 的 `QueryCursor.tla`（预算、快照、abort 契约）。
