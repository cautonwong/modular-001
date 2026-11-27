# ADR-0004: IDIS FreeRTOS 线程模型

日期：2026-11-27

## Status

Accepted，仅适用于 `targets/idis_ip_meter_freertos`。stategrid 是裸机，不在
本决定范围。决定已落地，由 `check:rtos-model` 和 `check:rtos` 守护。

## Context

IDIS FreeRTOS target 需要同时满足三件事。计量和通信要在 10 ms tick 上推进；
EEPROM 和 flash 的保存是长阻塞操作，不能放在中断或主循环里；掉电安全要求
保存相对对象修改是原子的。`docs/design/freertos-thread-model.md` 记录了当时
的选择和依据。

执行上下文只有三类。app task 优先级 3，用 `vTaskDelayUntil` 每 10 ms 运行，
是对象集的唯一修改者，负责 `idis_app_tick`、on_second 的 drain、对象读写和
维护帧。worker task 优先级 4，阻塞在保存队列上，只在 app 发送保存请求后运行
`idis_save_persisted`，执行 EEPROM/flash 持久化。idle 不喂狗，不做业务
（`docs/design/freertos-thread-model.md` §1、§2）。

FreeRTOS 配置和任务常量被 `tools/check_rtos_model.py` 冻结。配置包括
`configTICK_RATE_HZ = 100u`、`configUSE_PREEMPTION = 1`、
`configUSE_TIME_SLICING = 0`、`configMAX_PRIORITIES = 5`、
`configUSE_TASK_NOTIFICATIONS = 0`、`configUSE_MUTEXES = 0`、
`configUSE_TIMERS = 0`、`configTOTAL_HEAP_SIZE = 12 KiB`、
`configCHECK_FOR_STACK_OVERFLOW = 2`、`configSUPPORT_STATIC_ALLOCATION = 0`、
`configUSE_TICKLESS_IDLE = 0`、`configMAX_SYSCALL_INTERRUPT_PRIORITY = 0xC0u`。
任务常量包括 `APP_TASK_PRIORITY = 3u`、worker 优先级 4、
app 栈 768 words（3 KiB）、worker 栈 512 words（2 KiB）、
`SAVE_QUEUE_DEPTH = 1u`、`SAVE_CRITICAL_TIMEOUT_MS = 2000`
（`docs/design/freertos-thread-model.md` §1.1、§2、§3）。

已知证据缺口同样影响这个决定。栈高水位没有实测，canary 只覆盖任务栈，不覆盖
ISR/MSP；运行统计和 trace 关闭；看门狗最坏喂食间隔要 HIL 实测
（`docs/design/freertos-thread-model.md` §2、§5、§6）。

## Options

1. 在中断里做持久化。否决。IR UART ISR 只读字节、写 rx FIFO、清标志，不碰
   flash/EEPROM；计量和秒中断只置状态，持久化在任务里做
   （`docs/design/freertos-thread-model.md` §1）。
2. 用 workqueue 承载持久化。否决，FreeRTOS 没有原生 workqueue，长阻塞操作
   用独立线程更直接（`docs/design/freertos-thread-model.md` §7）。
3. 用 task notification 代替队列和信号量。否决，实测多 1,088 B ROM，
   文档记录当时余量 3,604 B，先留给功能（`docs/design/freertos-thread-model.md`
   §7 第 1 条）。
4. 用 mutex 保护共享资源。现在不选，因为 app 是对象唯一所有者，worker 只在
   app 发送后运行，今天没有两个任务同时改同一对象集
   （`docs/design/freertos-thread-model.md` §3、§4）。
5. worker 低于 app 优先级，用快照或把保存搬回 app 并分段。保留为 HIL 之后的
   备选；当前 worker 高一级是为了让保存相对对象修改原子
   （`docs/design/freertos-thread-model.md` §2、§7 第 6 条）。
6. 静态分配和 tickless idle。暂缓。静态分配要 RAM 布局和 ROM 账；tickless
   是功耗指标，要 HIL 量测（`docs/design/freertos-thread-model.md` §7 第 2、
   3 条）。

## Decision

- 两个任务。app 优先级 3，每 10 ms 运行，是对象集的唯一所有者；worker
  优先级 4，阻塞在保存队列上，负责 EEPROM/flash 持久化。idle 不做业务，
  不喂狗（`docs/design/freertos-thread-model.md` §1、§2）。
- 保存队列深度 1。保存是幂等的，写当前状态，重复请求合并成一次，丢请求不丢
  信息。这是合并式背压，不是无策略丢弃
  （`docs/design/freertos-thread-model.md` §3）。
- 不使用 task notification、workqueue、mutex 和软件定时器。`save_done` 二值
  信号量只做完成通知；`critical_pending` 由 app 置位、worker 清除，同一时刻
  只有一个写者（`docs/design/freertos-thread-model.md` §3、§7）。
- 存储总线今天没有锁。约束是发送点纪律：worker 只在 app 主动发送保存请求后
  运行，app 不会在一次存储事务中途发送，所以两者不会真正并行
  （`docs/design/freertos-thread-model.md` §4）。
- 看门狗按任务喂。app 在 on_second 入口喂一次；worker 在持久化块组之间喂；
  `save_energy_book` 在 commit 前后各喂一次；启动扫描每个 store read 后喂。
  这不是集中监控（`docs/design/freertos-thread-model.md` §5）。
- 配置和任务常量由 `tools/check_rtos_model.py` 冻结。改值必须同时改
  `docs/design/freertos-thread-model.md` 和 checker
  （`docs/design/freertos-thread-model.md` 开头；AGENTS.md 规则表）。

## Consequences

- 长保存期间 app 不能运行。worker 高一级换保存原子性，代价是计量和通信延迟
  被拉长；文档把它记为有意的优先级换原子性，不是漏配
  （`docs/design/freertos-thread-model.md` §2）。
- 没有 mutex，就没有锁顺序和死锁问题。代价是发送点纪律只在单存储 actor 时
  成立。出现第二个存储 actor，例如查询 worker、block session 或扇区窗口，
  纪律失效，必须先有 store 级锁（`docs/design/freertos-thread-model.md` §4；
  `docs/design/record-retrieval.md` §3）。
- 看门狗不是集中监控。worker 卡在擦除中间时，app 被高优先级抢占，喂狗只靠
  worker 的块间喂；最坏擦除 45 到 400 ms，块间间隔是否小于 2 s 要 HIL 实测。
  4 KiB flash sector commit 典型 200 ms、最坏 1.6 s，与 2 s 看门狗同量级，
  HIL 要确认最坏擦除下不会先复位（`docs/design/freertos-thread-model.md` §5；
  `docs/design/energy-pulse-store.md` §10 第 3 条）。
- 栈大小来自链接期预算，不是实测高水位。溢出检测覆盖任务栈，不覆盖 ISR/MSP；
  栈水位、CPU 占用和队列水位仍是缺口
  （`docs/design/freertos-thread-model.md` §2、§6）。
- 不用 task notification 省下 1,088 B ROM；这个数字是文档记录的当时测量，
  不是当前余量（`docs/design/freertos-thread-model.md` §7）。
- 这个模型不适用于 stategrid。stategrid 是裸机，调度和喂狗由主循环负责
  （`docs/design/freertos-thread-model.md` 开头）。

## Evidence

- `docs/design/freertos-thread-model.md` §1 到 §7：执行上下文、配置冻结表、
  任务和栈、通信与背压、共享资源所有权、看门狗、指南 §13 清单状态、决策与
  后续。
- `docs/design/energy-pulse-store.md` §10 第 3 条：commit 时序、擦除时间、
  2 s 看门狗和 HIL 缺口。
- `docs/design/record-retrieval.md` §3：FreeRTOS 开启扇区窗口前必须有 store
  级串行化，锁端口由 target 组合根提供。
- 门禁：`check:rtos-model`（`tools/check_rtos_model.py` 冻结配置和任务常量）、
  `check:rtos`（`tools/check_rtos_targets.py` 管 target 副本和 include 边界）。
- 测试：IDIS 两个 target 的固件构建和验收 lane；本次分支 `./ci.sh` 输出。
- 运行时证据：没有 HIL。栈高水位、看门狗最坏间隔、CPU 占用和 trace 仍是
  缺口，本文不声称已验证（`docs/design/freertos-thread-model.md` §6）。

## Reversal conditions

- 出现第二个存储 actor，或开启扇区读窗口。store 级锁必须先落地，否则发送点
  纪律不再成立（`docs/design/record-retrieval.md` §3；
  `docs/design/freertos-thread-model.md` §4、§7 第 5 条）。
- HIL 证明 worker 的长保存拉长计量或通信，超出预算。改用快照加低优先级
  worker，或把保存搬回 app 并分段（`docs/design/freertos-thread-model.md`
  §7 第 6 条）。
- 栈高水位实测显示 768/512 words 不够或过多。调整栈并同步 checker；或者做
  静态分配，但要先有 RAM 布局和 ROM 账
  （`docs/design/freertos-thread-model.md` §7 第 1、2 条）。
- 新功能需要 task notification 或软件定时器，且 ROM 预算能支付实测增量。
  那时重开配置决定，同步文档和 checker
  （`docs/design/freertos-thread-model.md` §7）。
- 功耗目标要求 tickless idle。先做 HIL 量测，再改配置和文档
  （`docs/design/freertos-thread-model.md` §7 第 3 条）。
