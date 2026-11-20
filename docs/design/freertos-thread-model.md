# IDIS FreeRTOS 线程模型（记录与评审）

状态：2026-11-20，`c9f7214d` 基线。适用于 `targets/idis_ip_meter_freertos`；
stategrid 是裸机，不在本文范围。依据是《RTOS 编程中的线程使用模型》工程
实践指南 v2.2 的 §1、§5、§6.5、§7、§9、§13；本文只记录本项目怎么选、
为什么、哪些没做。

`tools/check_rtos_model.py` 把下面表格里的配置和任务常量冻结成门禁；任何
漂移都会让 `check:rtos-model` 失败，必须先改本文和 checker。

## 1. 执行上下文

| 上下文 | 做什么 | 约束 |
| --- | --- | --- |
| IR UART ISR | 读字节、写 rx FIFO、清标志 | 不阻塞、不调用非 ISR-safe API；不碰 flash/EEPROM |
| 计量/秒中断 | 秒计数、脉冲累计 | 只置状态；持久化在任务里做 |
| app task（优先级 3） | 10 ms tick：`idis_app_tick`、on_second 的 drain、对象读写、维护帧 | 唯一的对象修改者；调度的主循环 |
| worker task（优先级 4） | `idis_save_persisted`：EEPROM/flash 持久化 | 只在 app 发送保存请求后运行；长阻塞（擦除 45–400 ms） |
| idle | FreeRTOS 空闲 | 不喂狗、不做业务 |

FreeRTOS 数值越大优先级越高；`configUSE_TIME_SLICING = 0`，同优先级不轮转。

### 1.1 配置冻结

| 宏 | 值 | 理由 |
| --- | --- | --- |
| `configTICK_RATE_HZ` | 100u | 10 ms tick；功耗与响应折中 |
| `configUSE_PREEMPTION` | 1 | 优先级抢占模型 |
| `configUSE_TIME_SLICING` | 0 | 不依赖未配置的轮转 |
| `configMAX_PRIORITIES` | 5 | 三级任务加 idle，留余量 |
| `configUSE_TASK_NOTIFICATIONS` | 0 | ROM 实测 1,088 B，先留给功能 |
| `configUSE_MUTEXES` | 0 | 今天无共享可变资源 |
| `configUSE_TIMERS` | 0 | 不用软件定时器 |
| `configTOTAL_HEAP_SIZE` | (12u * 1024u) | 两个任务、一个队列、一个信号量 |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | 上下文切换时查 canary；两 lane +72 B |
| `configSUPPORT_STATIC_ALLOCATION` | 0 | 已知缺口，见 §7 |
| `configUSE_TICKLESS_IDLE` | 0 | 低功耗后续 |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | 0xC0u | Cortex-M0 内核临界区上限 |

任务常量同样冻结：`APP_TASK_PRIORITY 3u`、`WORKER_TASK_PRIORITY
(APP_TASK_PRIORITY + 1u)`、`APP_TASK_STACK_WORDS 768u`、worker 栈 512 words、
`SAVE_QUEUE_DEPTH 1u`。

## 2. 任务、栈与时间属性

| 任务 | 优先级 | 栈 | 触发/周期 | 阻塞点 | WCET |
| --- | --- | --- | --- | --- | --- |
| app | 3 | 768 words = 3 KiB | `vTaskDelayUntil` 10 ms | 队列发送（非阻塞）、临界保存信号量（≤2000 ms） | 未实测 |
| worker | 4（`APP_TASK_PRIORITY + 1`） | 512 words = 2 KiB | 保存队列 | 队列接收（无限期） | 未实测；单次保存含多个持久化块 |
| idle | 0 | `configMINIMAL_STACK_SIZE` 128 words | 调度器 | 无 | 无 |

worker 比 app 高一级是有意的：保存必须相对对象修改原子。代价是长保存期间
app 不能运行，计量/通信延迟被拉长；这是 §7.4 意义上的优先级换原子性，不是
漏配。栈大小来自链接期预算，**没有** `uxTaskGetStackHighWaterMark` 实测；
溢出检测已开（`configCHECK_FOR_STACK_OVERFLOW = 2`，canary，两 lane +72 B），
只覆盖任务栈，ISR/MSP 栈不在检测范围；high-water 仍是已知缺口（§9.4/§13）。

## 3. 通信与背压

- 保存队列深度 1（`SAVE_QUEUE_DEPTH = 1`）。保存是幂等的“写当前状态”，
  重复请求合并成一次，丢了请求不会丢信息。这是 §6.5 的“合并/只保留最新值”
  背压，不是无策略丢弃。
- `save_done` 二值信号量只做完成通知；`critical_pending` 标记只由 app 任务
  置位、由 worker 在 `idis_save_persisted` 里清。app 只在等待前/等待中读它，
  同一时刻只有一个写者，因此“标志 + 信号量”没有 §12.17 的检查/等待窗口。
- 临界保存等待上限 `SAVE_CRITICAL_TIMEOUT_MS = 2000`；超时返回 -1，调用方
  继续走电源故障路径，但保存可能没完成。这个上限与 2 s 看门狗同量级，HIL
  要确认最坏擦除下不会先复位。
- 没有 mutex（`configUSE_MUTEXES = 0`），因为今天没有两个任务同时改同一
  对象集：app 是对象唯一所有者，worker 只在 app 发送后运行。没有 workqueue；
  长阻塞的持久化用独立线程（§5.4）。

## 4. 共享资源与所有权

- 对象集（COSEM 对象、能量账、事件）归 app 任务；worker 读取它做持久化。
- EEPROM 和 MX25 flash 总线由 app 的查询路径和 worker 的保存路径共用。
  当前没有锁：worker 只在 app 主动 `xQueueSend` 之后才被唤醒，app 不会在
  一次存储事务中间发送，因此两者不会真正并行。这是“发送点纪律”，不是
  锁保证。
- 一旦出现第二个存储 actor（查询 worker、block session、开启扇区窗口），
  这个纪律失效；必须先有 store 级锁，再开窗口。见
  `docs/design/record-retrieval.md` §3。

## 5. 看门狗

- app 在 `on_second` 入口喂一次；worker 在 `save_persisted` 的持久化块组之间
  喂（时钟/能量/需量/日历…，然后单动作/监视/限幅/镜像/设置，最后事件）；
  `save_energy_book` 在 commit 前后各喂一次；启动扫描每个 store read 后喂。
- 这是按任务的“活着就喂”，不是指南 §9.1 的集中监控。两个任务都喂，任一
  卡死仍会停喂；但 worker 卡在擦除中间时，app 被高优先级抢占，喂狗只靠
  worker 的块间喂。最坏擦除 45–400 ms（datasheet），块间间隔是否小于 2 s
  要 HIL 实测；`docs/design/energy-pulse-store.md` §10 与 HIL 手册已记录。

## 6. 对指南 §13 清单的状态

| 清单项 | 状态 | 说明 |
| --- | --- | --- |
| ISR 短小、只做立即的事 | 是 | IR ISR 只写 FIFO；持久化在任务 |
| ISR 只用 ISR-safe API | 是 | `board_ir_read` 的 ISR 只写 FIFO |
| 硬实时路径在硬件/ISR | 是 | 计量脉冲在中断累计，线程只消费 |
| 每个线程有拆分理由 | 是 | app=对象所有者；worker=长阻塞持久化 |
| 优先级有依据、方向正确 | 是 | FreeRTOS 大数优先；worker 高一级换原子性 |
| 栈实测 + 溢出检测 | 部分 | canary 只覆盖任务栈，ISR/MSP 不在内；high-water 未测 |
| 无忙等高优先级线程 | 是 | worker 阻塞在队列；app 用 `vTaskDelayUntil` |
| 无 workqueue/阻塞 handler | 是 | 不用 workqueue；长阻塞用独立线程 |
| 通信所有权清晰 | 是 | 对象 app 所有；保存请求合并 |
| 队列满策略与计数 | 部分 | 深度 1 合并；没有丢弃计数（保存幂等，不需要） |
| 互斥用 mutex、锁顺序 | 不适用 | 今天无 mutex；未来 store 锁要按 §7.2 选型 |
| 阻塞等待带超时 | 是 | 临界保存 2000 ms；队列接收无限期（worker 的常驻等待） |
| 无轮询式同步 | 是 | 队列/信号量；10 ms 是周期任务不是忙等 |
| 无“标志+原语”窗口 | 是 | 见 §3，单写者论证 |
| 单一所有者/仲裁 | 是（今天） | 存储总线靠发送点纪律；未来加锁 |
| 管理线程优先级不低于客户端 | 是 | worker 高于 app |
| 异步链路容量/背压/计数 | 部分 | 容量 1 + 合并；无计数，因为幂等 |
| 硬件隐性阻塞源已审查 | 是 | flash 擦除、SPI 事务在文档与 HIL 清单 |
| 看门狗集中监控 | 部分 | 两任务各自喂；HIL 测最坏间隔 |
| 运行统计（CPU/栈/队列） | **缺口** | `configGENERATE_RUN_TIME_STATS=0`，无水位 |
| trace/测量验证 deadline | **缺口** | 只有 host 仿真；无目标 trace |

## 7. 决策与后续

- **不做**：任务通知（ROM 实测 1,088 B，当前余量 3,604 B，先留给功能）、
  workqueue（FreeRTOS 无原生实现；长阻塞用独立线程更合适）、mutex（无共享
  可变资源）。
- **后续（需要设计/ROM/HIL）**：
  1. 栈：加 high-water 采样（调试构建或启动日志），先量再定；溢出检测已开。
  2. 静态分配：`xTaskCreateStatic` + 静态队列，去掉 12 KiB heap 的创建失败
     路径；需要 RAM 布局与 ROM 账。
  3. tickless idle 与唤醒合并：功耗指标，需 HIL 量测。
  4. 运行统计：CPU 占用、栈水位、队列水位、丢弃计数；先设计再进固件。
  5. store 级锁：第二个存储 actor 或扇区窗口开启前必须落地。
  6. worker 优先级重审：若 HIL 证明长保存拉长计量/通信超出预算，改用快照 +
     低优先级 worker，或把保存搬回 app 任务并分段。
- 门禁：`tools/check_rtos_model.py` 冻结配置与任务常量；`check:rtos` 继续管
  目标副本和 include 边界。
