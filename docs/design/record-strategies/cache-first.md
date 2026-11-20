# 策略：对齐读透窗口（cache-first）

状态：**已实现**（P-b，`nd_sector_store`）。固件默认关闭，等总线测量和
FreeRTOS store 锁；host 测试已证明填充次数。

## 问题

一次 1152 行 GET 的事务数由记录数决定（每条一次 128 B 读）。总线如果按事务
计费（片选、命令、驱动开销），减少事务数就是减少停顿；如果按字节计费，
窗口不赚。

## 算法

- 在 `nd_sector_store` 里加一个读透窗口，复用调用方已有的 `sector` 缓冲
  （IDIS 是 4 KiB `flash_sector`），新增 `window_base` 和 `window_valid`。
- 请求完全落在一个对齐且完整的扇区内时：窗口未命中就一次性读入整扇区，再从
  内存拷贝所需字节；否则直接后备读。
- 每次 write/erase 在用设备之前把窗口置无效，避免读到旧字节。
- 跨扇区、超过 `sector_size`、零长度、尾部不满一个扇区的请求全部旁路。

## 接口

不新增端口：`nd_store_t` 的 read 语义不变，`nd_sector_store_init` 增加一个
`enable_window` 参数。组合根（`board_flash_init`）决定开关。

## 成本

- 全扫 1152 槽：36 次 4096 B 填充（host 实测并断言）。
- 全量 GET 36 次、value-range GET 72 次（count 趟加 write 趟）。
- RAM +8 B（结构 44→52 B）；ROM +176..192 B（关闭时分支仍在）；append 路径
  字节不变；无持久化变更。
- 字节数：一次扫描约 148 KiB，与逐条整槽读相同；短记录场景会多读。

## 门禁与失败模式

- **默认关闭。** 开启条件：板上实测 `t(4 KiB) < 8 * t(148 B)`。字节计费时
  收益消失，整槽读仍是默认。
- **FreeRTOS 需要 store 锁。** 窗口和 save worker 共用缓冲与 SPI；现在没有
  串行化。锁由组合根注入，infra 不依赖 RTOS。
- 读失败会让窗口失效并返回错误；下一次读重新填充。
- 事件日志、镜像等其它 `nd_store` 用户只在小读时受益，大读旁路。

## 目标组合

IDIS：`board_flash_init` 传 `enable_window = 0`（当前）。host 测试直接开启。
stategrid：不使用（mbase 是 RAM 映射，没有块事务问题）。

## 测试

- `test_sector_store_window`：1152 槽全扫 36 次 4096 B 填充；单记录 1 次填充；
  写失效；跨扇区旁路；关闭时逐次后备读。
- `test_profile_*` 在窗口开/关两种配置下重跑。
- 形式化（P-e）：Lean 的填充界 `ceil(n * slot / sector)`，CBMC 的填充边界。
