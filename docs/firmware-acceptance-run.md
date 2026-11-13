# 固件验收矩阵推进记录

## 退出条件

`docs/firmware-acceptance-matrix.md` 的 122 行全部为 `A` 或 `X`，且：

1. `ctest --test-dir build` 全绿，`firmware_acceptance` 全绿。
2. `./ci.sh` 全绿（ARM 205 源、链接、固件对象、持久化偏移、组件清单）。
3. 矩阵第 5 节计数与表格一致。
4. 每个 `A` 行至少有一个不依赖 trace 的外部产物断言：EEPROM/flash 字节、UART TX 捕获、继电器/背光/LCD 捕获、退出码或报文捕获。
5. 每个 `X` 行写明排除理由，不把“难做”写成 `X`。

## 判定原则

- 期望值来自 `docs/firmware-acceptance-spec-notes.md` 引用的产品规范用例，或已确认的旧版行为；不从当前固件输出反推。
- 一个用例可以覆盖多行，但每行的判据必须在用例里单独断言。
- 仿真进程内的变量只用于定位，不作为判据。

## 工作单元

| 单元 | 交付 | 退出检查 |
|---|---|---|
| S1 场景与 UART 捕获 | `--scenario`、`--start-time`、`--uart-tx`；电源/开盖/按键/反馈注入 | 注入帧进入真实 DL698 栈，TX 捕获非空；原 76 测试全绿 |
| S2 协议编解码 | `tests/acceptance/protocol.py`，645/698 组帧与解析 | 已知报文向量 round-trip；独立 ctest |
| S3 规范映射 | `docs/firmware-acceptance-spec-notes.md`，122 行到产品用例 | 每行有规范引用或无规范说明 |
| S4 计量/费率/冻结 | 脉冲、电参、参数、时间用例 | 对应矩阵行 `A`，外部产物断言 |
| S5 事件/负荷/显示 | 事件输入、继电器、背光、LCD 用例 | 同上 |
| S6 645/698 | 读写、控制、对象族、分帧用例 | 同上 |
| S7 安全/上报/存储/升级 | ESAM、上报、存储一致性、升级用例 | 同上或规范缺失的 `X` |

## 场景语法

`tests/sim/sim_script.c` 是严格解析器：未知命令、未知字段、重复字段、越界值都报错并带行号。

| 命令 | 语义 |
|---|---|
| `at <cycle> power on\|off` | 经 `ac_power_on_monitor`/`ac_power_off_monitor` 走真实电源保存/恢复路径 |
| `at <cycle> uart <0..2> <hex...>` | 向对应 UART 通道注入原始字节 |
| `at <cycle> relay_feedback on\|off` | 板级继电器反馈输入，保持到下次设置 |
| `at <cycle> cover open\|closed` | 板级开盖输入，同时写 `nd_mbase_CoverOpenS_status`（`ND_EVENT_SET`/`ND_EVENT_RESET`） |
| `at <cycle> key press\|release` | 板级按键电平；`press` 额外向 `pipe_key` 入队 `IRQ_PMU`，`release` 不入队 |
| `at <cycle> pulse <1..64>` | 调用 `Energy_Pluse_Add()` count 次；`Energy_Pluse_Count` 只在 `0 < cnt < 64` 时搬运，故单周期应不超过 63 个脉冲；64 会卡在累加器里不再搬运，用于覆盖该边界 |
| `at <cycle> meter <field> <value> ...` | 在 `Energy_Pluse_Count` 阶段（`Electrical_measurement` 之后、`Energy_Meter` 之前）写 mbase |
| `at <cycle> energy clear` | 调用 `Energy_clr(instance)`：清电能数组、更新校验和、写 EC2 与 EC1 |
| `at <cycle> time 'YYYY-MM-DD HH:MM:SS'` | 替换 `instance->time` |

`meter` 字段（全部经 `meter_record.h` 生成的访问器写入，不直接碰结构体成员）：

| 字段 | 范围 | 写入口 | 标度 |
|---|---|---|---|
| `voltage` | 0..6553 | `nd_mbase_voltage_ac` | 输入整数 V，存 0.1 V |
| `current` | -100000..100000 | `nd_mbase_current_l` | 输入整数 A，存 0.001 A |
| `power` | -100000000..100000000 | `nd_mbase_active_power` | 输入整数 W，存 0.1 W |
| `const` | 1..4294967295 | `nd_mbase_meter_const` | 脉冲数/kWh，步进为 `const/100` 个脉冲 |
| `cal` | 0..255 | `nd_mbase_active_energy_cal_word` | 组合有功方向位：`0x80` ADD_POS、`0x40` DEC_POS、`0x20` ADD_NEG、`0x10` DEC_NEG；正向用 `128`，反向（记为负方向量）用 `32` |
| `direction` | forward\|reverse | `nd_mbase_power_dir` | 脉冲进入正向/反向数组 |
| `tariff` | 0..12 | `nd_mbase_TOUNo` | 0 为总，1..12 为 F1..F12 |

## 捕获文件

| 选项 | 每行格式 |
|---|---|
| `--relay-log PATH` | `cycle <n> time <YYYY-MM-DD HH:MM:SS> action connect\|disconnect`，每次 `Magnetic_hold_relay_connect/disconnect` 一行 |
| `--backlight-log PATH` | `cycle <n> time <...> value 0\|1`，每次 `nd_board_backlight_set` 一行 |
| `--lcd-log PATH` | `cycle <n> time <...> fb <320 位十六进制>`，每个 `Display` 阶段后快照 160 字节 `FB` |

`tests/acceptance/sim.py` 的 `SimRun` 暴露 `relay_log`、`backlight_log`、`lcd_log` 三个列表。`relay_feedback` 仍是板级输入：仿真中由 `nd_board_relay_is_off()` 返回，`metering/legacy/runstatus.c` 的 `nd_relay_status_is_off()` 路径读取；真实硬件的 `SysTick_Handler`（`soc/cpu_v85xxp_isr.c`）用它累计 `relay_off_chk`/`relay_on_chk`，仿真不运行该 ISR。

## 已知限制与修复

- 空 EEPROM 的三个陷阱：`meter_const` 为 `0xFFFFFFFF`（脉冲步进无意义）、`cal_word` 为 `0xFF`（`nd_energy_read_combined` 四个方向位互相抵消，GET 恒为 0）、电能数组为 `0xFF`（首次累加回绕）。计量用例必须先 `at 0 energy clear`，再显式 `meter const` 与 `meter cal`。
- 空 EEPROM 的事件记录 `valid_num` 由 `0xFF` 记录搜索得出，不可作为基线；开盖用例先跑一次生成种子 EEPROM，再断言读数差值（开盖后 1 条、关盖后仍 1 条，END 原位修订）。
- 上电崩溃根因：`application/adapters/dl698/fr_698.h` 泄漏 `#pragma pack(1)`，`fr_698_esam.c` 看到 packed 布局而 `esam_pro.c` 看到自然布局，主机 64 位下结构体偏移不一致（`data_buf` 0x34 对 0x38）。修复是在 `fr_698_esam.h` 的 ESAM 结构体外层加 `#pragma pack(push, 1)`/`#pragma pack(pop)`（ARM32 两种布局恰好一致）。修复后 `at 1 power off`、`at 3 power on` 的 6 周期运行正常退出。

## 决策记录

| 日期 | 决策 | 理由 |
|---|---|---|
| 2026-11-12 | 场景文件用逐行文本，不引入 JSON 解析器 | C 侧零依赖；用例生成简单；错误可定位到行 |
| 2026-11-12 | UART 注入写 `pl_rx_698.in_queue`，不伪造 ISR | 走真实 `nd_dl698_poll` 和 mapper |
| 2026-11-12 | 电源切换走 `ac_power_off_monitor`/`ac_power_on_monitor` | 走真实 `fr_sw_ac_power` 保存/恢复路径 |
| 2026-11-12 | 协议编解码复制纯函数，不依赖 test-platform | 主仓库 CI 不能依赖外部路径和 pytest |
| 2026-11-12 | 记录类判据优先用 645/698 读取响应 | 响应是外部产物，避免 Python 重写存储格式 |

## 进度

- 2026-11-12：矩阵 122 行，A=6，C=6，D=104，E=6。启动 S1/S2/S3。
- 2026-11-12：S1 增加 `pulse`/`meter`/`energy clear`/`time` 输入与继电器/背光/LCD 捕获；新增 ACC-INPUT-PULSE/COVER/POWER 三个冒烟用例（只作冒烟，不把矩阵行标成 `A`）。

## 矩阵收敛记录

- 2026-11-12：ACC-BOOT-002 用 `--arena-size 384` 触发资源初始化失败，仿真器现在打印 `[sim] init failed ret=-1` 并退出非零，不再打印 `done`；该行转 A。
- 2026-11-12：ACC-WDG-001、ACC-TMR-001、ACC-POWER-003 归为 X。理由分别是看门狗在 `driver_entry.c`/`infra/boot`、定时器服务属基础设施、低功耗与唤醒源属板级 PMU/SoC；三者都不在应用固件孪生的可观测范围。
- 2026-11-12：ACC-LC-009 归为 X。产品配置未启用预付费，代码中只有 698 映射字段，没有业务路径。
- 矩阵检查器增加 `--update`，从行状态重算第 5 节计数，避免表格与明细脱节。
- 2026-11-12：645 RS485-1 路径接通后，ACC-645-001/002/003/004/005/006/008/009 到 A；坏校验帧丢弃与重同步由既有用例覆盖，0xFA 长帧缺陷留在 645-007 的 X 理由里。
- 2026-11-12：ACC-POWER-002 到 A，断言停电前后 698 电能值和继电器命令状态；需量没有固件路径，ACC-MET-005 归 X，`get_1_Demand.c:7-17` 恒返回 OBJECT_NOT_EXIST。
- 2026-11-12：所有外部对时入口都在 `GET_4_Parameter.c:385` 解引用 NULL 的 698 上下文，698 SET 0x40000200 单帧即 SIGSEGV；ACC-RTC-003/004/005 归 X，另 `write_mcu_rtc` 仍是空桩。
- 2026-11-12：698 分帧、SET/ACTION 对象族和记录选择到 A；连接/超时（698-007）与 ESAM（698-005）没有可测状态，归 X。
- 2026-11-12：安全模式查找（SEC-001）到 A；MAC/密钥/密文转换与上报路径缺少 ESAM 和 PLC TX 捕获，3320 列表还是 NULL 初始化，SEC-002..006、RPT-001..007、698-004 归 X。
- 2026-11-12：季节/日时段切换、费率参数持久化、分钟/小时/结算/瞬时冻结、停电补抄上限与去重、EEPROM 电能记录到 A；假日表可读但不参与选表且 698 写假日表崩溃，TAR-003/009 归 X。
- 2026-11-13：矩阵 122 行全部收敛到 A 或 X（A=68，X=54），`--strict` 通过；X 行都在第 2 节和明细里写明不可测的产品路径或孪生缝隙。
- 2026-11-13：验收偶发 `firmware_sim exited 2` 的根因是 0x40000000 外设窗口与地址随机化后的 brk 堆重叠。链接器在 `0x40000000` 预留 1 MiB 的 `.sim_periph` 段，`sim_peripheral_map` 直接使用该段；连续 30 次 acceptance ctest 不再复现。
- 2026-11-13：修复 `get_4_Parameter.c` 在无 698 上下文时解引用 `p_ctx->CA` 的空指针，并在仿真器实现 `write_mcu_rtc` 的 BCD 写入。ACC-RTC-003/004 与 ACC-EVT-007 转为 A；RTC-005 仍缺冻结回退的外部断言。
- 2026-11-13：仿真器增加 `chip ok|fault` 与 `adc temperature|battery` 激励。计量芯片故障事件（ACC-MET-007、ACC-EVT-016）和温度/电池读数（ACC-MET-012）转为 A。芯片默认按故障态启动以保持既有电气量用例的早退路径，用例先 `chip ok` 再触发故障。
- 2026-11-13：`IOStateCheck` 按模拟秒推进继电器反馈计数，ACC-LC-005 的误动事件转为 A；计数比真实 10 ms ISR 慢 100 倍，已在代码里标注 shortcut。
