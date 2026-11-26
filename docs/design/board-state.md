# Board 状态只读盘点（95 条，按 target / ISR 可见性）

日期：2026-11-24
基线：main acc7b487 的 `tools/mutable_globals_baseline.txt`。
口径：路径含 board/driver/baremetal/isr/soc 的全部条目，共 **95** 条
（不是早先估算的 70）；其中 target 侧 88 条，infra 共享 7 条。
ISR=是 表示该符号在同文件的 ISR 入口（`Isr_handle_regsit` 注册的函数或
`*_IRQHandler`）里被直接访问；间接调用和跨文件回调标“待核”。
只读，无代码改动。

## 批 1：idis_ip_meter bare 的 app 状态（6）

| 文件 | 符号 | ISR | 访问 | 备注 |
|---|---|---|---|---|
| app/baremetal.c | systick_ticks | 是（SysTick_Handler） | ISR 写 / 任务读 | volatile |
| app/board_ports.c | flash_sector | 否 | 任务 | EEPROM 扇区缓冲 |
| app/board_ports.c | uart_fifo | 否 | 任务 | 填充者不在本文件，待核 |
| app/board_ports.c | uart_fifo_buffer | 否 | 任务 | 同上 |
| app/board_ports.c | flash_store | 否 | 任务 | |
| app/board_ports.c | key_event | 是（board_pmu_isr） | ISR 写 / 任务读 | volatile |

## 批 2：idis_ip_meter bare 的 board 驱动（23）

| 文件 | 符号 | ISR | 访问 | 备注 |
|---|---|---|---|---|
| board/board_eeprom.c | DEVICE_ERR | 否 | 任务 | note_result |
| board/board_eeprom.c | error_streak | 否 | 任务 | |
| board/board_eeprom.c | eeprom_storage | 否 | 任务 | 存储对象 |
| board/board_eeprom.c | eeprom | 否 | 任务 | 设备对象 |
| board/board_esam.c | esam_storage | 否 | 任务 | |
| board/board_esam.c | esam | 否 | 任务 | |
| board/board_flash.c | flash_storage | 否 | 任务 | |
| board/board_flash.c | flash | 否 | 任务 | |
| board/board_rn8209.c | uart_ready | 否 | 任务 | bus_write/uart_prepare |
| board/board_rn8209.c | storage | 否 | 任务 | |
| board/board_rn8209.c | dev | 否 | 任务 | |
| board/board_transport_soc.c | uart_rx | 是（uart_rx_isr） | ISR+任务 RW | 无 volatile |
| board/board_transport_soc.c | uart_tx | 是（uart_tx_isr） | ISR+任务 RW | 无 volatile |
| board/board_transport_soc.c | spi_dma | 是（spi_dma_isr） | ISR RW | 无 volatile |
| board/board_transport_soc.c | spi_dma_ready | 否 | 任务 | DMA 完成标志，待核 |
| board/spi_driver.c | esam_dma | 否 | 任务 | DMA 回调可能间接访问，待核 |
| board/spi_driver.c | esam_dma_ready | 否 | 任务 | 同上 |
| board/spi_driver.c | esam_tx_stage | 否 | 任务 | |
| board/spi_driver.c | esam_rx_sink | 否 | 任务 | |
| board/spi_driver.c | esam_tx_used | 否 | 任务 | |
| board/uart_driver.c | uart_dev | 是（UartReceiveHandle/UartTransmissionHandle） | ISR+任务 RW | 无 volatile |
| board/uart_driver.c | uart_tx_fifo | 否 | 无直接访问 | 数组，可能宏访问/死代码，待核 |
| board/uart_driver.c | uart_rx_fifo | 否 | 无直接访问 | 同上 |

## 批 3：idis_ip_meter_freertos 的 app 状态（5）

| 文件 | 符号 | ISR | 访问 | 备注 |
|---|---|---|---|---|
| app/board_ports.c | flash_sector | 否 | 任务 | |
| app/board_ports.c | uart_fifo | 否 | 任务 | 与 bare 同一份副本 |
| app/board_ports.c | uart_fifo_buffer | 否 | 任务 | |
| app/board_ports.c | flash_store | 否 | 任务 | |
| app/board_ports.c | key_event | 是（board_pmu_isr） | ISR 写 / 任务读 | volatile |

## 批 4：idis_ip_meter_freertos 的 board 驱动（23）

与批 2 同构（文件是 bare 的副本，符号、访问和 ISR 结论一致）：
`board_eeprom.c` 4、`board_esam.c` 2、`board_flash.c` 2、`board_rn8209.c` 3、
`board_transport_soc.c` 4（uart_rx/uart_tx/spi_dma 是 ISR，spi_dma_ready 否）、
`spi_driver.c` 5（esam_* 任务）、`uart_driver.c` 3（uart_dev ISR；
uart_tx_fifo/uart_rx_fifo 无直接访问）。

## 批 5：stategrid_meter 的 app 状态（7）

| 文件 | 符号 | ISR | 访问 | 备注 |
|---|---|---|---|---|
| app/driver_entry.c | fifo_buf | 否 | 任务 | irq_system_init |
| app/driver_entry.c | irq_fifo | 是（IO_Interrupt/RTC_Second） | ISR 写 / 任务读 | 中断队列，无 volatile |
| app/driver_entry.c | meter_cover_cnt | 否 | 任务/定时钩子 | IOState 经 tm.tickfunc，待核 |
| app/driver_entry.c | wpulse_light_flag | 是（IO_Interrupt/timing_ftr） | ISR+任务 RW | 无 volatile |
| app/driver_entry.c | wpulse_light_open | 是（IO_Interrupt/timing_ftr） | ISR+任务 RW | 无 volatile |
| app/driver_entry.c | wpulse_light_open_flag | 是（timing_ftr） | ISR RW | 无 volatile |
| app/driver_entry.c | wplse_light_width | 是（timing_ftr） | ISR RW | 无 volatile |

## 批 6：stategrid_meter 的 board 驱动（24）

`board_eeprom.c` 4、`board_esam.c` 2、`board_flash.c` 2、`board_rn8209.c` 3、
`board_transport_soc.c` 4、`spi_driver.c` 5、`uart_driver.c` 3：结论与批 2 相同
（transport 的 uart_rx/uart_tx/spi_dma 和 uart_driver 的 uart_dev 是
ISR 可见；其余任务态）。另加：

| 文件 | 符号 | ISR | 访问 | 备注 |
|---|---|---|---|---|
| board/cpu_v85xxp_isr.c | Isr_Function | 是（各 *_IRQHandler） | 任务写 / ISR 读+执行 | 注册表，无 volatile |

## 批 7：infra 共享（7，单列）

| 文件 | 符号 | ISR | 访问 | 备注 |
|---|---|---|---|---|
| boot/App/Source/target_isr.c | Ir_Cou | 待核 | 无直接访问 | 老 ISR 残余，待核 |
| soc/.../nd_v8500_irq.c | isr_table | 是（isr_dispatch） | 任务写 / ISR 读 | 注册表，无 volatile |
| soc/.../time_link.c | tick_timer_list | 是（ndv_timer_tick） | 任务写 / ISR 读 | SysTick 里 tick，无 volatile |
| soc/.../v8500_for_single.c | UART_list | 否 | 任务 | mcu_uart_* |
| soc/.../v8500_for_single.c | poweroff_test | 否 | 任务 | ac_power_off_monitor |
| soc/.../v8500_for_single.c | test | 否 | 任务 | 同上（两条同名条目） |

## 风险与拆批建议

- 主要风险是 ISR 共享的状态，但先证明再改：wpulse 与两张函数指针表经证明
  不需要 volatile，tick_timer_list 需要临界区——见下一节。迁移顺序仍先 ISR
  相关项，再任务态存储对象；每批保持“一个文件的符号一起走”。
- 跨文件间接访问的待核项：`uart_fifo`（谁填）、`spi_dma_ready`、
  `esam_*`（DMA 回调）、`uart_tx_fifo`/`uart_rx_fifo`（是否有宏访问或已死）、
  `meter_cover_cnt`（定时钩子是否在 ISR 内）、`Ir_Cou`。
- 建议批次：1）bare app（6）→ 2）bare drivers（23）→ 3）freertos app（5）
  → 4）freertos drivers（23）→ 5）stategrid app（7）→ 6）stategrid drivers
  （24）→ 7）infra 共享（7）。ISR 共享项先按下一节的结论处理，再排迁移。

## Batch A 证据与结论（只读）

对象：`wpulse_light_flag` / `wpulse_light_open` / `wpulse_light_open_flag` /
`wplse_light_width`（driver_entry.c:56-59）、`Isr_Function`
（cpu_v85xxp_isr.c:19）、`isr_table`（nd_v8500_irq.c:11）、
`tick_timer_list`（time_link.c:17）。结论：七个符号都不该只加 volatile，
A 不写产品代码，ROM/RAM 增量为 0。

- wpulse 四个：uint8_t，只在 ISR 里访问——IO_Interrupt（PMU ISR）写
  `wpulse_light_open`，timing_ftr（TMR3 ISR）读写另外三个；任务上下文没有
  访问。目标没有 NVIC_SetPriority 调用，PMU/TMR3 同为默认优先级；Cortex-M0
  同优先级中断不互相抢占，访问是顺序的。结论：不需要 volatile，约束是
  ISR-only 状态机。IRQ lane：stategrid 没有 IRQ lane，driver_entry.c 不在
  host 单测中，真并发证据留 HIL 残余。
- Isr_Function / isr_table：函数指针表，Isr_handle_regsit 整结构赋值、ISR
  读取并调用。volatile 不能把多字段拷贝变原子，还会把函数指针读取变成
  volatile 访问，语义错误。约束是“注册必须在对应 IRQ 使能之前，之后只读”：
  driver_entry 的 RTC/PMU/TMR3 注册（:305-315）在 __enable_irq（:317）之前；
  ndv_timer_init 先注册 SYSTICK 再 cm0_systick_init(1)；IDIS 的 DMA 注册在
  NVIC_EnableIRQ 之前。IDIS 的 UART/PMU 注册路径尚未逐条审计，列为待核。
- tick_timer_list：任务侧 create/start/stop/start_one 写
  timeout/dlyctr/stoped，SysTick ISR 的 ndv_timer_tick 递减 dlyctr 并回调；
  真正共享，volatile 只解决可见性，不解决多字段一致性与 lost update。需要
  关中断临界区或重构为 ISR 只递减、任务消费；另立单元并需要 IRQ lane/HIL
  证据，不在 A。
- irq_fifo 仍归 B。
