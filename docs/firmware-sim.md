# Linux 整机固件孪生

日期：2026-11-12
状态：可运行；覆盖 MSG_SECOND 主路径，硬件外设与通信注入仍是桩。

## 目标

把 `stategrid_meter` 的固件源码编成 Linux 进程，从真实的
`baremetal_instance_entry_main` 启动，跑真实的实例、资源管理、pipeline 调度和
`app_meter_base_entry` 秒任务；EEPROM/flash 用文件镜像，MCU 和板级外设用桩。
它不是产品固件，也不是字节级仿真器，而是运行路径和业务逻辑的孪生。

## 构建与运行

```sh
cmake -S tests -B build -G Ninja
cmake --build build --target firmware_sim
./build/firmware_sim --cycles 60 --verbose --trace /tmp/firmware.trace
```

默认镜像写在 `backupfiles/firmware_eeprom.bin`（65535 字节）和
`backupfiles/firmware_flash.bin`（8388607 字节），首次打开全部填 `0xFF`。
`--eeprom`、`--flash` 可以指向独立场景目录。`--verbose` 才输出报文、filter 和
应用阶段；不加时只输出每周期的时间行。

ctest 里有一个 3 周期冒烟：`ctest -R firmware_sim`。acceptance 套件在
`tests/acceptance/`，通过外部契约检查启动、RTC、持久化、业务周期和 flash，
运行 `ctest -R firmware_acceptance`。方案和用例表在
`docs/firmware-acceptance-plan.md`。

gprof 版本：

```sh
cmake -S tests -B build-prof -G Ninja -DBLADE_FIRMWARE_SIM_PROFILE=ON
cmake --build build-prof --target firmware_sim
cd build-prof
./firmware_sim --cycles 60 --eeprom /tmp/e.bin --flash /tmp/f.bin
gprof ./firmware_sim gmon.out > profile.txt
```

## 结构

| 位置 | 作用 |
|---|---|
| `tests/sim/sim_main.c` | 用 `mmap(MAP_32BIT)` 申请低 4 GiB 的 24 KiB 实例区；调用真实 `create_instance` 和真实入口 |
| `tests/sim/sim_trace.c` | 每周期注入 RTC 事件、设置 `instance->ms`、输出 trace；提供 filter/报文/阶段钩子 |
| `tests/sim/sim_devices.c` | 文件后端 EEPROM/flash `driver_ops_t`，以及返回零读数的 RN8209 桩 |
| `tests/sim/sim_stubs.c` | MCU、看门狗、IRQ、继电器、LCD、ESAM 等板级入口 |
| `tests/sim/include/sim_compat.h` | 保 `pid_t` 为 `uint8_t` 的 glibc 兼容层、CMSIS intrinsic 空实现、IAR 关键字 |
| `tests/sim/include/core_cm0.h` | 屏蔽真实 CMSIS 的 Cortex-M0 汇编 |
| `tools/firmware_sim_manifest.py` | 从 IAR `.ewp` 派生 172 个可移植源文件，排除 board/SOC/driver_entry |
| `tests/CMakeLists.txt` | `firmware_sim` 目标和 ctest 冒烟 |

低 4 GiB 不是随便选的。资源管理器把地址存进 `uint32_t`：
`create_instance` 的 `first_addr`、`add_inside_block` 的返回值和
`block_device_t.first_addr` 都是 32 位（`resource_tool.h:34`、
`resource_tool.c:317`）。如果实例区落在高地址，`create_instance` 会把截断后的
地址交给 `memset`。`MAP_32BIT` 让固件的 32 位寻址假设在 x86-64 上仍然成立。

## 运行路径

60 个模拟秒的 gprof 调用计数：

| 阶段 | 调用 | 说明 |
|---|---:|---|
| `create_instance` | 1 | 真实资源管理器 |
| `add_block` / `add_inside_block` | 7 / 3 | RAM 块、698 块、mbase 块 |
| `add_device` | 2 | EEPROM、flash |
| `register_parameter` / `register_log` / `register_map` | 3 / 3 / 1 | 参数区、事件区、698 map |
| `nd_get_block` | 3 | 启动恢复 static 区 |
| `nidavellir_run_baremetal` | 60 | 每个模拟秒一圈 |
| `app_meter_base_entry` | 60 | `pl_meter_base` 唯一 filter |
| `nd_sim_stage` | 840 | 14 个应用阶段 × 60 秒 |
| `legacy_energy_checksum_span` | 180 | 电能校验/保存路径 |
| `store_read` / `nd_store_write` | 392 / 8 | 文件后端真实读写 |

启动阶段走的是 `baremetal_instance_entry_main`
（`targets/stategrid_meter/app/stategrid_meter_entry.c:333`）：创建 recipe、注册
RAM/EEPROM/flash 块、创建三个 698 通道、分配 mbase 三区、恢复 static 区、
递增复位次数。之后每圈由 `sim_trace.c` 往 `instance->pipe_rtc` 写 `IRQ_RTC`，
`fr_sw_rtc` 把它变成 `MSG_SECOND` 投给 `pid_meter_base`，
`nidavellir_run_baremetal`（`targets/stategrid_meter/app/rtos/nidavellir.c:411`）
调用 `app_meter_base_entry`（`targets/stategrid_meter/app/apps/app_meter_base_entry.c:221`）。

`MSG_SECOND` 实际执行的阶段：

```
SecTick_Task
Rtc_Prossecc
TouFeePro
Electrical_measurement
Energy_Pluse_Count
Energy_Meter
freeze_pro
event_pro
Key_Display
Display
backlight_pro
do_relay
relay_led_ctl
IOStateCheck
```

3600 个模拟秒（1 小时）无崩溃退出；EEPROM 有 183 个非 `0xFF` 字节，flash 有 6 个。
跨小时的时间推进已验证。

## 覆盖矩阵

| 层 | 状态 |
|---|---|
| 实例、资源块、设备注册、参数/日志/map 注册 | 真实 |
| recipe、pipeline、filter 调度、消息 FIFO | 真实 |
| `app_meter_base_entry` 秒任务及领域调用 | 真实 |
| EEPROM/flash 读写 | 真实，文件后端 |
| 电能持久化校验和编码 | 真实 |
| RTC 秒事件 | 注入 |
| `instance->ms` | 注入，每模拟秒 +1000 |
| MCU 时钟、看门狗、NVIC、低功耗 | 空桩 |
| UART/PLC/红外传输 | 只调用 `nd_dl698_poll`，没有字节注入 |
| 计量芯片 | 返回零读数，没有电压/电流/脉冲 |
| ESAM | `openM_ioctl` 返回 -1 |
| LCD | 写进程内 `sim_lcd`，不校验段码 |
| 继电器/背光/按键 | 板级桩，逻辑状态仍会流转 |
| 停电/上电状态机 | 监控函数恒 0，只跑上电分支 |
| 存储 pipeline `pl_resource_manager` | 已执行；3600 秒内 1 次 filter，记录内容未解码 |
| ThreadX 入口 `os_entry.c` | 不编译；入口签名与 baremetal 版本不一致 |

## 固件侧发现

1. `instance->ms` 在启动时置 0，之后没有 SysTick 推进
   （`stategrid_meter_entry.c:355`）。`dl698_now_ms` 因此恒为 0。孪生自己注入
   毫秒时间，说明这是裸机固件的真实缺口，不是仿真器造出来的。
2. WIN32 的 `main` 不创建实例，`instance` 保持 NULL，随后被
   `baremetal_instance_entry_main` 解引用。旧的 Windows 仿真路径不可用。
3. `pl_rx_698` 的 `filter_n` 为 0，`pipeline->on_work` 不会执行；698 组帧完全靠
   `nd_dl698_poll`。只看 pipeline trace 会误以为接收路径没跑。
4. `COMMTAB.Addr` 和 `DISPLAYTAB.Sign` 原来用 `unsigned int` 存指针，64 位主机
   无法把数组地址当常量初始化。已改为 `uintptr_t`
   （`application/adapters/dl645/645id_table.h:111`、
   `targets/stategrid_meter/app/display.c:266`）；32 位 ARM 布局不变。
5. `Rtc_Prossecc` 在每秒第 20 秒调用 `read_mcu_rtc` 对时
   （`targets/stategrid_meter/app/tariff/legacy/touFee.c:321`）。主机桩必须让时间
   前进，否则秒数会卡在 20。
6. `nidavellir_run_baremetal` 无条件调用看门狗、IRQ 泵、电源监控和
   `cpu_sleep_manage`；WIN32 之外没有宿主保护。孪生用桩承接。
7. `resource_tool.h` 的 `#pragma pack(4)` 没有复位，泄漏到后续所有头文件。
   `stategrid_meter_entry.c` 和 `nd_dl698.c` 因此对
   `nd_dl698_config_t` 得出 44 和 48 两个大小，`nd_dl698_create` 返回 NULL，
   `dl698_component_count` 为 0。已在 `circular_info_t` 后补 `#pragma pack()`，
   698 通道恢复为 3 个；三个持久化结构的大小不变。

## 已知盲区与下一步

按对 acceptance 的价值排序：

1. 通信注入：把字节写进 `instance->pipe_uart[chl]`，补
   `IRQ_UART_RX_TM_OUT` 超时标记，抓 TX fifo。这样才能覆盖 645/698 组帧、
   mapper 请求和主动上报。
2. 存储 pipeline：记录消息已经会到达 `pl_resource_manager`，下一步是注入指定
   记录并解码 `sync_data_record` 的落盘结果。
3. 停电/上电：让 `ac_power_off_monitor`/`ac_power_on_monitor` 可脚本化，
   覆盖 `PowerOff_Save_data` 和 `MSG_POWERON` 恢复。
4. 计量芯片模型：按场景给电压、电流、功率、脉冲；否则电能、需量、事件阈值
   都只能在零值附近打转。
5. 观测面：继电器状态、LCD 段码、UART TX、ESAM 响应目前没有可断言的输出。
6. 时间模型：`ndv_timer_tick`、定时器到期、跨天/跨月冻结还没有驱动。
7. 字节兼容：主机 `struct tm` 与 newlib 布局不同，孪生 EEPROM 不能当作 ARM
   镜像；需要固定布局或独立的字节级 fixture。

## 变更边界

所有固件改动都有 `ND_FIRMWARE_SIM` 保护，ARM 构建不受影响：
`csstack_base_init` 的空实现、入口循环的停止钩子、filter/报文 trace、
应用阶段 trace、`display.c` 的 `sim_lcd`。`uintptr_t` 两处是无条件但 32 位
布局等价的移植性修正。全量 `ci.sh` 通过（75/75，205/205，207 objects）。
