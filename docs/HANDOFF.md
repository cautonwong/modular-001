# Blade DDD / Clean Architecture 交接文档

更新于 2026-11-10。新会话从这里接着干。

## 1. 目标

把 Blade 嵌入式电表工程重构为四层 DDD / Clean Architecture。

不可违背的原则：

1. user alloc。所有资源都由调用方提供，不只 RAM，还包括持久化块、队列、定时器、IO 句柄、ID 段。
2. 聚合、聚合根、有界上下文。
3. 消费者定接口。上层声明自己需要的端口，下层实现。
4. 组合根。只有 target 同时看见 domain、application、infra。
5. 不透明指针。聚合结构体定义留在 `.c`，公开头文件只有前置声明。

硬约束：存量电表的持久化字节布局必须逐字节兼容。

构建约束：目标代码用 `arm-none-eabi-gcc`，测试用宿主 `gcc` 加 Unity。

## 2. 仓库与分支

- 工作副本：`/home/edge/modular-001`
- 工作分支：`main`（本项目直接在 `main` 上累积提交）
- 发布仓库：`https://github.com/cautonwong/modular-001`，`main` 分支
- 源仓库：`https://github.com/nanshan0708/Blade`，`main` 从未改动
- 本地 `main` 已包含本轮提交；用户明确要求只 commit，不 push 远端

新会话建议直接克隆发布仓库：

```bash
git clone https://github.com/cautonwong/modular-001.git
cd modular-001
```

`/tmp/blade-refactor` 是临时目录，重启可能丢失，但所有提交都已推到 `modular-001`。

## 3. 当前目录结构

```
domain/        领域层，纯 C，不依赖平台
  common/      allocator, store, parameter_store, calendar
  metering/    energy book, metering aggregate, demand, legacy/
  tariff/      tariff lookup, legacy/
  events/      duration, over_current, cover_open, power_off, generic,
               neutral_current, legacy/
  load_control/ load control aggregate, legacy/
  freeze/      legacy/
  display/     legacy/
application/   应用层
  metering/    one-second use case
  adapters/    dl698, dl645 (layer-clean)
  apps/        legacy applications (staging)
  upgrade/     legacy upgrade
infra/         基础设施层
  kernel/      fifo, list, bitarithm, runtime
  memory/      arena, byte_store, parameter_store
  soc/         cm0, v85xxp
  os/          baremetal
  security/    esam, crypto
  boot/        bootloader
targets/       组合根
  host/        host composition root
  stategrid_meter/ legacy entry, board/, rtos/, soc/, security/, persistence adapters
  msta/        msta instance
tests/         unit, support, third_party/unity, legacy
nidavellir/    legacy SDK headers and docs
docs/          architecture and this handoff
tools/         checks and probes
```

依赖方向只向内。`targets -> application -> domain <- infra`。board、具体 RTOS 入口和产品专用 SoC 胶水代码放在 `targets/<product>`，由组合根直接选择和调用。domain、application 和共享组件不 include 平台头。

## 4. 已验证的东西

- 73 个 Unity 测试二进制，全绿。
- 旧工程 ARM GCC 编译基线 `205/205`。
- 旧工程整体重定位链接，内部未定义符号为 0。
- 层依赖检查通过。
- 新代码 Cortex-M0 编译检查通过。
- 快速旧代码检查通过。

### 4.1 验证命令

```bash
# 宿主测试
cd tests
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure

# 全量旧工程 ARM GCC 编译基线，解析 IAR .ewp
python3 tools/legacy_baseline.py

# 快速旧代码子集
./tools/check-legacy.sh

# 层依赖规则
./tools/check-layers.sh

# 新代码 Cortex-M0 编译
./tools/check-arm.sh
```

预期输出：

- `ctest` 显示 `100% tests passed, 0 tests failed out of 73`
- `legacy_baseline.py` 显示 `sources=205 ok=205 failed=0 missing_files=0` 和 `link_undefined=0`
- `check-firmware-link.py` 显示 `firmware link ok`
- `check_components.py` 显示 `components ok (59 manifests)`
- 其余检查脚本退出码为 0

### 4.2 工具说明

- `tools/legacy_baseline.py` 解析 `targets/stategrid_meter/iar for arm/VG_DTZY.ewp`，收集源文件、include 路径和宏定义，用 ARM GCC 逐个编译，再把全部目标文件重定位链接，用 `nm -u` 检查内部未定义符号，按错误归类。每次移动目录后都要跑。
- `tools/legacy_probe.c` 从旧头文件实测持久化偏移。修改旧结构体时重跑，并更新 `targets/stategrid_meter/persistence/legacy_energy_record.h` 的静态断言。
- `tools/check-legacy.sh` 是快速子集，适合频繁迭代。
- `tools/check-layers.sh` 检查 include 方向和 domain 内的直接分配。
- `tools/check-arm.sh` 检查新代码在 Cortex-M0 下可编译且不依赖宿主 libc。
- `tools/check-firmware-link.py` 按产品源清单编译并链接，同时要求与 EWP 源表双向一致，链接出 V85XXP 最终 ELF。
- `tools/check-persistence.sh` 断言旧持久化结构大小和偏移不变。
- `tools/check_instance_boundary.py` 禁止 domain/application 看到 instance 和 mbase 布局。
- `tools/check_components.py` 校验组件归属、include 依赖和层方向；include 按仓库内 IAR 工程的搜索路径解析。

## 5. 关键设计决策

### 5.1 四层

DDD 是四层。`targets` 是组合根，不是层。物理目录用 `domain / application / infra / targets`。

### 5.2 不透明聚合与 user alloc

示例：`domain/metering/metering.h` 只前置声明 `metering_state_t`。定义在 `.c`。
创建和销毁都接收 `nd_allocator_t`。domain 不调用 `malloc`。
`infra/memory/arena.c` 实现分配端口，返回清零内存，最近分配的块可以回收。

### 5.3 消费者定接口

已有端口：

- `domain/common/allocator.h`：`nd_allocator_t`
- `domain/common/store.h`：`nd_store_t`，按字节偏移读写
- `domain/common/parameter_store.h`：`nd_parameter_store_t`，按 16 位 ID 读写记录

infra 实现这些端口。target 在组合根里注入。

### 5.4 持久化兼容

电能切片是 172 字节，冻结在 `targets/stategrid_meter/persistence/legacy_energy_record.h`。
偏移由 `_Static_assert` 锁定。
实测值：active_p 0，active_n 52，active_p_dec 104，active_n_dec 130，energy_checksum 156，month_energy_dec 160，month_energy 164，use_energy_checksum 168。
该切片在旧持久化块内的起始偏移是 3。
校验和与 `reserved` 填充字由持久化层保留，codec 不覆盖。

费率表按参数 ID 解析。ID 冻结在 `legacy_tariff_ids.h`。

### 5.5 迁移暂存区

`tools/check_layers.py` 的 STAGING 现在为空。`application/adapters/dl645` 和 `dl698` 已去掉 kernel/target include，层检查按 application 规则扫描。ARM 检查仍跳过 adapters，因为部分旧头文件不能单独做语法检查。

`domain/*/legacy` 现在只保留接口头文件，实现在 `targets/stategrid_meter/*/legacy`。

## 6. 已完成的工作

1. 四层骨架、Unity、CMake、层检查、ARM 检查。
2. 分配端口、arena、byte store、parameter store。
3. 计量领域：电能账本、不透明聚合根、需量、字节兼容 codec 和 store。
4. 日历与费率查找，包括环形区间、节假日、周休日、日时段、费率选择。
5. 负荷控制聚合，包括命令状态机、计时器、按键、LED、误动作检测。
6. 事件去抖原语，以及过流、开盖、掉电、计量芯片故障、继电器误动作、RTC 故障、电源异常、零线电流异常。
7. 旧事件调用点全部接到新模块，旧的 `event_duration_pro` 已删除。
8. host 组合根，一个秒循环串起计量、需量、费率、负荷控制和持久化。
9. 应用层一秒用例，target 只负责装配。
10. 目录迁移：kernel、drivers、boot、baremetal、esam、crypto、upgrade、meterbase、协议包、applications、instances 全部归位。
11. 旧工程 ARM GCC 编译基线从 11 个文件扩到全工程 137 个文件，并加入整体链接检查。
12. 冻结上下文的周期、月、结算、瞬时触发、上电补冻结和序号规则已重写为 `domain/freeze/{schedule,backfill,sequence}`，旧调用点已接入。
13. 掉电、电源异常的下电路径改走 `nd_event_duration_start`，不再直接操作活动标志。
14. 费率查找已迁移：`targets/stategrid_meter/app/tariff_lookup.c` 通过 `nd_tariff_table_t` 选择日表和费率，`tariff_lookup_eeprom.c` 提供 EEPROM 参数存储；`GetFeeTab`/`GetTOUFee` 已接入，旧 `FeeTable_CmpFind` 已删除。
15. 负荷控制已迁移：`targets/stategrid_meter/app/relay_control_adapter.c` 把旧控制块、继电器输出、LED、事件和 645 上报桥接到 `load_control_state_t`；`do_relay`/`relay_led_ctl` 已接入，`Relay_Sec_Task` 已删除。
16. 计量能量已迁移：`targets/stategrid_meter/app/metering_energy_adapter.c` 在旧电能数组和 `metering_state_t` 之间导入导出；`Energy_Meter`、停电刷新、清零、设置和两个 int64 取数函数已改用聚合。电能账本现在在零脉冲调用时也会进位，组合取数先合并原始整数和脉冲再换算，与旧公式一致。
17. 死文件核查：10 个文件都不在 IAR 工程且无编译引用。删除了 7 个重复或未使用文件，保留 `demand.c` 和两个 bill 文件等产品范围确认。`application/meterbase` 暂存目录已清空。
18. 用户确认保留需量和月/阶梯结算。三处文件留在 staging，当前固件没有接线：`get_1_Demand.c` 返回 `OBJECT_NOT_EXIST`，也没有账单处理函数调用两个 bill 文件。恢复功能时先迁进对应限界上下文。
19. 需量应用层：`application/metering/demand_registry.c` 用固定数组持有多路需量对象、功率来源和输出 OAD，替代旧 `demand.c` 的链表和 malloc 接线。4 个主机测试。旧 `demand.c` 仅作为 staging 参考保留。
20. `applications.h` 第一次拆分：35 个函数声明移到 metering、tariff、load control、freeze、events、display 六个上下文头文件；单体 `mbase_control_block_t` 移到 `nidavellir/include/kernel/legacy_control_block.h`。伞头从 708 行降到 282 行，`tools/legacy_probe.c` 的持久化偏移不变。
21. 内核解耦：`nidavellir.h` 不再 include `applications.h`。资源块 ID、设备枚举、继电器反馈阈值和按键延时宏移到 `infra/kernel/resource_blocks.h`、`infra/board/relay_config.h`、`infra/kernel/key_config.h`。44 个 application/target 调用方改为显式 include `applications.h`，137/137 编译链接通过。
22. 记录组迁移：`applications.h` 里的记录结构体、枚举和 13 个函数声明移到 `domain/events/legacy/gs_record.h`，头文件补齐 `stdint.h` 和前置声明。伞头从 282 行降到 213 行。
23. 删除 `applications.h`：77 个 include 点改为具体 owner 头。剩余声明分别归入 `app_config.h`、`app_entry.h`、`driver_entry.h`、`mapper_698/resource_map.h`、`fr_698_esam.h`、`v85xxp_system.h`、`devices.h`、`ctl_other.h`。`pin_function_def.h` 补 `type_def.h`，修复 `BIT` 未定义。`check_layers.py` 明确把 `application/adapters` 和 `application/upgrade` 列为 staging。伞头已从仓库删除，137/137 编译链接通过。
24. dl698 第一步解耦：`fr_698_comm.c` 改用通用 `nd_get`/`nd_set`/`nd_action`，不再 include target 的 `resource_map.h`。硬件 ESAM 实现从 `application/adapters/dl698/fr_698_esam.c` 移到 `infra/security/esam/fr_698_esam.c`，应用侧头文件保留为消费者声明。IAR 工程路径已更新。
25. dl698 时间与安全参数：`dl698_time.c` 用日历领域函数重写日期检查、时间间隔和纪元换算，修掉了通配日、闰日、24 点/60 分/60 秒和单位 0 乘法等旧缺陷，6 个主机测试。`parameter_port` 把显式安全模式、安全使能和上报参数从 EEPROM 地址计算里拿出来，`dl698_parameter_adapter.c` 在启动时绑定；旧的不可达冻结周期读取已删除。140/140 编译链接通过。
26. dl698 上报参数：跟随上报、上报通道、主动上报和四路上报列表全部走参数端口，读写共用同一套键。`get_3320_save_addr` 已删除，`get_3_Event.c` 改用共享列表访问器。
27. 上报列表模型：`report_list.c` 用固定 60 项数组表达旧布局（1 字节计数加 4 字节小端 OAD），提供解析、编码、查找、删除和头插。更新、最新跟随项、确认回写、主动上报过滤和 3011 存在性检查已改写到模型上，不再用指针搬移持久化缓冲。7 个主机测试，33/33 通过。
28. 单体控制块访问器：`application/adapters/dl698/meter_base_view.h` 用带类型的指针访问器暴露 dl698 需要的字段，`targets/stategrid_meter/app/meter_base_adapter.c` 负责绑定旧控制块并保存两个持久化切片。`fr_698.c`、`fr_698_connect.c`、`fr_698_security.c`、`fr_report.c` 不再 include `kernel/legacy_control_block.h`，也不再直接访问 `mbase_cb->`/`ctrl_block->` 字段。142/142 编译链接通过，偏移探针不变。
29. 可移植性第一批接缝：新增 `application/ports/platform.{h,c}`，把看门狗和 PLC 事件输出收成平台端口；`targets/stategrid_meter/app/platform_adapter.c` 绑定 v85xxp 实现。`domain/common/status.h` 提供与平台无关的电源和事件状态值。dl698 目录不再直接 include `port.h`、`v85xxp_system.h`、`cpu_surport.h`、`pin_function_def.h`、`lib_clk.h`、`lib_uart.h` 或 `infra/kernel/resource_blocks.h`。顶层设计写入 `docs/portability.md`，覆盖多 SoC、多 board、多 RTOS、多 domain 的目录、端口、组合根和迁移顺序。144/144 编译链接通过，34/34 主机测试通过。
30. `nidavellir.h` 拆分：原 783 行伞头变成 11 行兼容包装，内容拆到 `kernel/kernel_core.h`、`kernel/instance.h`、`kernel/message.h`、`kernel/object.h`、`kernel/channel.h`、`kernel/pids.h`、`kernel/resource_ids.h`。`context_698_t` 移到 `application/adapters/dl698/fr_698_context.h`，`context_645_t` 移到 `application/adapters/dl645/fr_645_context.h`。dl698 和协议适配器不再 include `nidavellir.h`，应用入口和 645 调用方改为显式包含上下文头。新头文件都能独立编译，`instance_t` 和 `message_t` 的布局与拆分前逐字节一致，144/144 编译链接通过，34/34 主机测试通过。
31. `kernel_core.h` 拆分：从 450 行降到 62 行的兼容包头，内容拆到 `kernel/tasklet.h`、`kernel/rtc.h`、`kernel/log_object.h`、`kernel/device.h`、`kernel/environment.h`、`kernel/ipc_globals.h`、`kernel/serial.h`、`kernel/work_type.h`、`kernel/resource_map.h`、`kernel/flags.h`。`recipe_t` 保留原来的 pack(1) 布局。`instance_t`、`message_t`、`tasklet_t`、`rtc_context_t`、`dev_info_t`、`recipe_t`、`irp_t` 的大小和关键偏移与拆分前一致，144/144 编译链接通过，34/34 主机测试通过。
32. 顶层设计升级为组件化 monorepo，写入 `docs/monorepo.md`。协议应用收敛到 `components/protocol`，不再保留独立可执行形态。pipeline/filter/message 计划替换为类型化事件总线、通道端口和直接用例接口，分阶段迁移后删除旧机制。产品矩阵扩展到单相/三相电表、水表、燃气表、载波模块和 RF 模块。现有 domain 模块被定义为组件候选，还需要补公开契约、去除 legacy 类型和单相假设。
33. 组件化第一批落地：`components/core/bus/nd_bus` 提供固定主题枚举、固定订阅表、同步发布、无 malloc；`components/services/channel/nd_channel` 提供非阻塞字节通道端口，包含 read、write、flush。两个组件都有 `component.yaml` 和 host 测试，`blade_components` 已接入 CMake，IAR 工程新增 `components` 组。当前 7 个 bus 测试和 4 个 channel 测试通过，146/146 编译链接通过，36/36 主机测试通过。组件尚未接到固件，下一步做旧 `send_mesg`/`recv_mesg` 的适配层。
34. 组件化第二批：domain、application、infra 的关键模块补了 `component.yaml`，`products/` 下建立单相电表、水表、燃气表、PLC 模块和 RF 模块的产品描述。新增 `tools/check_components.py`，检查组件元数据、组件 include 方向和产品引用的组件是否存在，并接入 `tools/check-legacy.sh`。`infra/kernel/nidavellir.c` 在秒变化和电源变化时向 `nd_bus` 发布事件，旧 pipeline 路径保持不变。新增 `infra/channel/fifo_channel.c`，把旧 FIFO 包装成 `nd_channel` 端口，带 host 测试。当前 147/147 编译链接通过，37/37 主机测试通过。
35. dl698 协议组件迁移：新增 `components/protocol/dl698/nd_dl698.{h,c}`，用 `nd_channel` 接管三个 698 通道的收发。组件通过 `nd_fifo_channel_fifo` 零拷贝访问现有 UART FIFO，不新增 RAM。三个 rx pipeline 从 recipe 移除，三个 tx pipeline 改成只保留 `in_queue` 的路由管道，由组件轮询 `send_mesg` 写入的消息并调用 `fr_tx_698_entry`。收到完整帧时向 `nd_bus` 发布 `ND_EVENT_DL698_FRAME`。148/148 编译链接通过，37/37 主机测试通过。
36. 移除 698 对 `message_t` 的依赖：新增 `fr_698_message.h`，定义 698 私有的 `nd_dl698_msg_t`，dl698 所有收发、处理和回调签名改用它，源码里不再出现 `message_t`、`send_mesg`、`recv_mesg`。主动上报改为通过 `context_698_t.send` 直接调用 `nd_dl698_send`，tx pipeline 从 recipe 删除，`pl_tx->in_queue` 不再分配。rx pipeline 只保留 context 和 FIFO 供上下文查找，内核循环通过无 filter 判断跳过。148/148 编译链接通过，37/37 主机测试通过。
37. 迁移 dl645：新增 `fr_645_message.h`，定义 `nd_dl645_msg_t`，布局与旧 `message_t` 头部一致，保证 master 路径写入 `instance->cache` 的偏移不变。dl645 源码不再出现 `message_t`、`send_mesg`、`recv_mesg`；master 转发改为 `context_645_t.forward` 回调，tx 入口不再从 FIFO 读消息。新增 `components/protocol/dl645/nd_dl645.{h,c}`，用 `nd_channel` 零拷贝访问现有 FIFO，轮询 `fr_rx_645_entry` 和 `fr_gatekeeper_645_server`，收到完整帧时发布 `ND_EVENT_DL645_FRAME`。645 组件暂未接入选定的单相产品，因为该产品的 recipe 没有 645 context；645 产品实例化组件即可。149/149 编译链接通过，37/37 主机测试通过。
38. 迁移 dlms：新增 `fr_dlms_message.h`，定义只含 len/ctrl 的 `nd_dlms_msg_t`，`pkg_dlms.c` 和 `user_information.c` 不再出现 `message_t`。修正 DLMS 的 include 路径、内部声明和 `fifoBuf_putByte/putData` 调用，补上安全上下文 extern，删除未在 IAR 和 vcxproj 中使用、且与 `pkg_dlms.c` 重复的 `application_context_name.c`。两个 DLMS 源文件现在可以单独通过语法检查，但 DLMS 仍不在产品构建里，暂无运行时接线。缓冲策略写入 `docs/buffer-strategy.md`：当前不引入 lwIP pbuf 作为协议内部缓冲，IP 产品再把 lwIP 作为 infra 传输组件接入。
39. 多层协议缓冲组件：新增 `components/core/buf/nd_buf.{h,c}`，提供 `nd_frame`、`nd_block`、`nd_cursor`。帧可以引用现有缓冲并带 release 回调，块由多帧链接而成，游标支持跨帧读取和跳过，只有显式 `nd_block_copy` 才线性化。7 个 host 测试覆盖跨帧读、跳过、容量截断、释放和非法链入。当前 150/150 编译链接通过，38/38 主机测试通过。协议解析器尚未切换到该组件，698/DLMS 的分段重组仍复制到固定数组。
40. 698 分段重组改用 `nd_block`：`context_698_t` 增加 `rx_block` 和 6 个 `rx_frame` 节点。首帧、中间帧、末帧都通过 `nd_block_append` 追加，末帧序号回退时用 `nd_block_truncate` 截断，`apdu_rx_len` 由块总长同步。帧节点指向现有 `apdu_d_rx` 存储，不新增分片缓冲；当前仍是连续 APDU 视图上的链，下一步可以把帧节点指向独立接收缓冲并用 `nd_cursor` 解析。150/150 编译链接通过，38/38 主机测试通过。
41. 基础设施状态端口：新增 `infra/board/meter_status.h`，由消费者声明表类型、计量芯片故障状态和 ESAM 电源状态；`targets/stategrid_meter/app/meter_status_adapter.c` 在旧控制块上实现。`infra/board/ndv_RN8209.c`、`infra/security/esam/esam_pro.c`、`infra/kernel/nidavellir.c` 不再 include `kernel/legacy_control_block.h`，也不再直接读写 `mbase_cb` 的这些字段。151/151 编译链接通过，38/38 主机测试通过。
42. ESAM 状态端口：新增 `infra/security/esam/esam_status.h`，声明计量芯片异常、密钥状态、认证开关、表号更新和认证超时挂起；`targets/stategrid_meter/app/esam_status_adapter.c` 在旧控制块上实现，表号更新仍在变化时保存到原来的静态 EEPROM 切片。`infra/security/esam/fr_698_esam.c` 不再 include `kernel/legacy_control_block.h`，也不再直接访问 `mbase_cb`；ESAM 电源开关和关闭延时继续走 `meter_status` 端口。152/152 编译链接通过，38/38 主机测试通过。
43. 基础设施目标头清理：删除未在 IAR 工程中的 Windows 专用 `infra/board/esam698_win.c`。新增 `infra/board/meter_config.h`，把表类型、计量校准和自热补偿参数 ID 固定为 `ND_*_PARAM_ID`。`infra/board/ndv_RN8209.c` 和 `infra/kernel/nidavellir.c` 不再 include `targets/stategrid_meter/app/port.h`，`fr_698_esam.c` 不再 include `cpu_surport.h`，改用 `domain/common/status.h` 的 `ND_AC_POWER_ON`。`infra/` 下已没有 `port.h` 引用。152/152 编译链接通过，38/38 主机测试通过。
44. 引脚宏迁移：`targets/stategrid_meter/pin_function_def.h` 移到 `infra/board/stategrid_meter/pin_function_def.h`，23 个消费文件的 include 和 IAR 工程路径同步更新，旧路径删除。文件内容未改，板级引脚仍直接暴露 GPIO 寄存器宏；下一步按外设拆成语义 board 端口。152/152 编译链接通过，38/38 主机测试通过。
45. 语义 board 端口：新增 `infra/board/stategrid_meter/board_io.{h,c}`，提供背光、PLC 事件输出、继电器 LED、继电器反馈、表盖输入、按键输入和 RS485 接收使能，以及 ADC 和 UART 通道常量。`display.c`、`gs_backlight_ctrl.c`、`electrical_measurement.c`、`runstatus.c`、`driver_entry.c`、`mapper_698.c`、`platform_adapter.c`、`relay_control_adapter.c` 和 `nidavellir.c` 改走语义接口。`fr_645_report.c` 的 PLC 事件改用 `application/ports/platform.h`。`infra/soc/v85xxp/cpu_v85xxp_isr.c` 和 ESAM 安全代码也不再 include 引脚头。现在只有 board 驱动和 `board_io.c` 使用原始引脚宏。153/153 编译链接通过，38/38 主机测试通过。
46. board 驱动内部收口：`board_io` 增加 EEPROM 电源和 IIC 线、SPI Flash 电源和 SPI 线、RS485 方向、继电器线圈、RN8209 复位和 TX 线、ESAM 片选和电源接口。`iic_driver.c`、`spi_driver.c`、`ndv_Norflash.c`、`uart_driver.c`、`relay.c`、`ndv_RN8209.c` 和 `esam698*.c` 全部改走这些接口。这一步后只有 `board_io.c` include `pin_function_def.h`，其他源码不再出现原始引脚宏。153/153 编译链接通过，38/38 主机测试通过。
47. 引脚头折叠：删除 `infra/board/stategrid_meter/pin_function_def.h`。`board_io.c` 直接持有位带地址、寄存器置位/清零、位读写辅助函数和全部板级引脚操作，IAR 工程里的旧头文件条目也已移除。现在仓库里没有单独的引脚宏头，只有 `board_io.c` 接触 GPIO 寄存器。153/153 编译链接通过，38/38 主机测试通过。
48. 应用端口第一批：新增 `domain/display/backlight.h` 和 `domain/load_control/relay_status.h`，分别表达背光开关和继电器反馈。`application/apps/display.c`、`domain/display/legacy/gs_backlight_ctrl.c` 和 `domain/metering/legacy/runstatus.c` 不再 include `board_io.h`。target 的 `backlight_adapter.c` 和 `relay_status_adapter.c` 在组合根里绑定到 `board_io`，绑定发生在 `dev_init` 之前，保证初始化期间显示和继电器状态可用。各加一个 host 测试。157/157 编译链接通过，40/40 主机测试通过。
49. 计量输入端口：新增 `domain/metering/metering_input.h`，表达温度和电池两路输入，返回值仍是旧 ADC 原始值。`domain/metering/legacy/electrical_measurement.c` 不再 include `board_io.h`，也不再直接调用 `mcu_adc_start_once` 和 `read_mcu_adc_measure_value`。target 的 `metering_input_adapter.c` 负责把端口映射到 `ND_ADC_CHANNEL_TEMPERATURE` 和 `ND_ADC_CHANNEL_BATTERY`，并在组合根绑定。159/159 编译链接通过，41/41 主机测试通过。
50. 线级引脚模块拆分：`board_io.h` 只保留背光、PLC 事件、继电器 LED 和反馈、表盖、按键这些语义操作。新增私有头 `iic_pins`、`spi_pins`、`uart_pins`、`rn8209_pins`、`relay_pins`、`esam_pins`，对应驱动只 include 自己需要的私有头。公共头不再暴露 IIC、SPI、UART、RN8209、ESAM 和继电器线圈的线级函数。`ND_ADC_CHANNEL_*` 移到 `meter_config.h`，`ND_UART_*` 移到 `uart_driver.h`。165/165 编译链接通过，41/41 主机测试通过。
51. 平台边界收敛：`infra/board` 合并到 `targets/stategrid_meter/board`。依赖 board 的 `nidavellir.c`、`time_link.c`、`cpu_v85xxp_isr.c`、`esam_pro.c`、`fr_698_esam.c` 一并移到 target 的 `rtos`、`soc`、`security` 子目录。新增 `tools/check_platform_boundary.py`，只允许 target 和平台实现目录 include SoC、board、RTOS、Zephyr 头，并把 application/domain 里的 legacy 依赖计为 staging 债。当前 116 条 staging include/调用，非 staging 违规为 0。165/165 编译链接通过，41/41 主机测试通过。
52. application/apps 平台依赖清理：产品专用的 `app_meter_base_entry.c` 和 `display.c` 移到 `targets/stategrid_meter`，`app_msta` 移到 `targets/msta/apps`。`app_698_server` 测试用例、`test_app_meter_base_entry.c` 和 `test_resource.c` 移到 `tests/legacy`。application/apps 里不再有平台 include 或 RTOS 调用。staging 依赖从 116 条降到 18 条，剩余集中在 `application/adapters/dl645/fr_645.c` 和 domain legacy。165/165 编译链接通过，41/41 主机测试通过。
53. staging 清零：新增 `domain/common/runtime` 和 `domain/common/timer` 端口，target 分别绑定到 `mcu_delay_us`/`mcu_clear_watchdog` 和 `ndv_timer_start_one`。`gs_record` 的记录消息改为 target 实现的 `nd_record_message_send`。`electrical_measurement.c` 移到 target。domain 里的 event、freeze、tariff、metering 代码改用 `ND_EVENT_*`、`ND_AC_POWER_ON` 和领域端口。平台边界检查 staging 从 18 条降到 0，非 staging 仍为 0。170/170 编译链接通过，43/43 主机测试通过。
54. 内核头边界 ratchet：`tools/check_platform_boundary.py` 扩展到 `nidavellir.h`、`kernel/*` 和 `nidavellir/*`。当前 80 条 legacy kernel 头依赖记录在 `tools/platform_boundary_baseline.txt`，新增违规直接失败。主要债务在 `domain/events/legacy`、dl698/dl645 适配器和 `application/apps`。170/170 编译链接通过，43/43 主机测试通过。
55. dl645/dl698 第一批：`fr_645_report.c` 移到 target，dl645 的 channel/flag/object 常量本地化，dl698 去掉未使用的 object 和 `kernel/channel.h`。kernel baseline 从 62 条降到 55 条，剩余 dl645 4 条（instance、legacy control block、pids、fifo），dl698 9 条。170/170 编译链接通过，43/43 主机测试通过。
56. dl645 归零：新增 `fr_645_env` 访问器，target 用 `fr_645_env_adapter.c` 提供旧控制块字段、APDU cache、密码索引和 pipeline 视图。`fr_645.c` 不再 include instance、legacy control block 或 pids。`fr_645_context.h` 的 APDU FIFO 改成私有 `fr_645_fifo_t`，操作语义按原 kernel FIFO 复制。dl645 的平台边界条目为 0，整体 baseline 从 55 条降到 51 条。171/171 编译链接通过，43/43 主机测试通过。
57. dl698 归零：新增 `fr_698_env` 访问器，target 用 `fr_698_env_adapter.c` 提供时间、APDU cache、电源状态、复位标志、断电计时和 pipeline 视图。dl698 适配器不再 include `kernel/instance.h`，`fr_698_context.h` 不再 include pids，pid 常量本地化。`protocol.dl698` 组件增加 `now_ms` 回调，不再直接读 instance。整体 baseline 从 51 条降到 41 条。172/172 编译链接通过，43/43 主机测试通过。
58. domain/events/legacy 归零：11 个事件/记录实现文件移到 `targets/stategrid_meter/app/events/legacy`，IAR 工程路径同步更新。原因是这些实现直接操作单体控制块、EEPROM 记录地址和 kernel 消息，属于产品目标代码，不属于可复用领域层。`domain/events/legacy` 只保留 `events_applications.h` 和 `gs_record.h` 两个接口头。该目录从 STAGING 和 ARM 跳过名单移除。平台边界 baseline 从 41 条降到 19 条。172/172 编译链接通过，43/43 主机测试通过。
59. 剩余 19 条归零：display、freeze、load_control、metering、tariff 的 9 个 legacy 实现文件移到 `targets/stategrid_meter/<context>/legacy`，IAR 工程和 `tools/check-legacy.sh` 路径同步。dlms 增加本地 `struct pipeline_t` 视图，不再 include kernel/instance；`nd_dl645` 也改为本地 pipeline 视图，并用 `now_ms` 回调读取时间。domain 下的 legacy 目录只剩接口头，全部从 STAGING 和 ARM 跳过名单移除。平台边界 baseline 从 19 条降到 0。172/172 编译链接通过，43/43 主机测试通过。
60. fr_report 和 fr_698_security 局部重写：新增 `domain/report/report_policy`，把上报标识匹配、通道使能、2015 掩码、3011 主动上报条件和 3320 属性变体收成纯函数；新增 `domain/security/security_mode`，把安全模式表和权限判断从适配器移出，并集中明文读 OAD 白名单。适配器改为调用这两个领域模块，删掉重复表、死代码和 `#if 0` 样本。新增 10 个主机测试。174/174 编译链接通过，45/45 主机测试通过。
61. 3011/3320 组帧与重发状态机拆分：`domain/report/report_retry` 管理主动上报命令位、重试延迟/次数、PIID 和上电/掉电窗口判断；`fr_report_frames.c` 只负责 3011 和 3320/2015 的 APDU 组帧；`fr_report.c` 保留列表维护、应答处理和调用编排。重试状态机有 8 个主机测试。176/176 编译链接通过，46/46 主机测试通过。
62. ESAM RN/MAC 流程拆分：`fr_698_esam_flow.c` 负责 SID_MAC、RN、RN_MAC、SID 四种验证处理、SID 解析和 ESAM 结果判定，`fr_698_security.c` 只保留安全传输参数解析、请求权限检查和编码入口。177/177 编译链接通过，46/46 主机测试通过。
63. ESAM 输入解析和结果映射拆分：`fr_698_esam_input.c` 负责公共输入初始化、SID/octet 解析、临时缓冲绑定和 RN 保存；`domain/security/esam_result` 负责 ESAM 状态与验证结果映射，并显式保留 `enforce_mac` 开关。适配器现在传 1，MAC 不匹配会拒绝验证；没有 MAC 字段的 RN 流程不误判。179/179 编译链接通过，47/47 主机测试通过。
64. fr_report_frames 编码拆分：`report_encoder.c` 提供 3011 头/RCSD 列表、普通上报头、3320 数组项、2015 位串项、长度回填和边界检查，`fr_report_frames.c` 只保留数据采集、ESAM 尾部和缓存拷贝。新增 4 个编码测试。180/180 编译链接通过，48/48 主机测试通过。
65. ESAM MAC 校验打开：`fr_698_esam_flow.c` 对 RN_MAC 等带 MAC 的验证调用改为 `enforce_mac = 1`，只有输入和输出 MAC 都存在且长度足够时才比较；纯 RN 流程没有 MAC 字段，不会误拒绝。ESAM 结果映射测试覆盖强制模式。180/180 编译链接通过，48/48 主机测试通过。
66. ESAM 主机侧兼容性验证：新增 `docs/esam-compatibility.md`，覆盖 ESAM 状态错误、SID/SID_MAC、RN_MAC 正确和错误、纯 RN、MAC 缺失或过短。`nd_esam_mac_optional_matches` 已由主机测试覆盖，当前 48/48 通过。真实 ESAM、误码 MAC、断线重连和掉电重启仍需硬件验证。
67. ESAM RN/MAC 尾部编码：`report_encoder` 增加 `nd_report_encoder_rn_mac_tail`，统一生成 `0x02 0x0c`、12 字节随机数、`0x04` 和 4 字节 MAC。ESAM 异常时 MAC 填 `0xFF`，正常时取 ESAM 返回的第 13 到 16 字节。3011 和普通上报都改用该函数，删掉两处重复的指针搬移。新增 3 个尾部编码测试。180/180 编译链接通过，48/48 主机测试通过。
68. 698 权限检查拆分：新增 `domain/security/security_request`，把应用链接、身份认证、红外认证、终端认证超时、广播/组地址和广播校时权限判断收成纯函数。`fr_698_security.c` 只负责填请求状态并映射返回码。新增 7 个权限测试。181/181 编译链接通过，49/49 主机测试通过。
69. 698 安全编码拆分：`fr_698_esam_encode.c` 负责安全响应的 ESAM 调用和尾部拼装，`security_encoder.c` 只负责 `01 00 04 MAC` 或 `00` 尾部的纯编码和边界检查。`fr_698_security.c` 不再包含编码实现。新增 3 个编码测试。183/183 编译链接通过，50/50 主机测试通过。
70. fr_698.c 帧层拆分：`fr_698_frame.c` 承接帧头/帧尾打包、matcher、时间标签、地址处理、APDU 检查和长度检查；`fr_698.c` 缩减到 746 行，保留分帧、发送、gatekeeper 和入口。新增 `fr_698_internal.h` 管理适配器内部原型。184/184 编译链接通过，50/50 主机测试通过。
71. gatekeeper 和分帧状态机拆分：`fr_698_segment.c` 承接接收块重组、分帧处理、扰码和分帧发送；`fr_698_gatekeeper.c` 承接服务检查、上下文清理、服务端和客户端 gatekeeper。`fr_698.c` 只剩发送处理和两个入口，170 行。186/186 编译链接通过，50/50 主机测试通过。
72. 发送状态机拆分：`fr_698_tx.c` 承接发送分段、连接参数选择、帧打包调用和 `fr_tx_698_entry`。`fr_698.c` 只剩 `fr_rx_698_entry`，36 行。187/187 编译链接通过，50/50 主机测试通过。
73. 客户端转发和协议状态拆分：`fr_698_client.c` 承接客户端 gatekeeper，删除其中整段失效的 `#if 0` 转发代码；`fr_698_state.c` 承接 `fr_ctx_clear` 和 `fr698_inital`。`fr_698_gatekeeper.c` 从 389 行降到 284 行。189/189 编译链接通过，50/50 主机测试通过。
74. 客户端转发回调：`context_698_t` 增加 `forward` 回调。客户端 gatekeeper 解析 service/choice/PIID 后，把剩余 APDU 复制到 `ctx->apdu_d_rx`，填充 `nd_dl698_msg_t` 并调用回调；没有绑定回调时仍按原逻辑丢弃。目标侧还没有注册具体转发目标。189/189 编译链接通过，50/50 主机测试通过。
75. DLMS 迁移到协议组件：`application/adapters/dlms` 移到 `components/protocol/dlms`，组件名改为 `protocol.dlms`。新增本地 `nx_dlms.h`，不再通过 `kernel/object.h` 取得结构定义。DLMS 仍未进入产品构建，组件清单保留 `legacy: true`。`application/adapters` 的 staging 现在只剩 dl645 和 dl698。189/189 编译链接通过，50/50 主机测试通过。
76. dl645/dl698 迁移到 application 规则：新增 `application/ports/fifo.h` 作为 FIFO 消费接口，`infra/kernel/fifo.h` 改为引用它。dl645 去掉 `utils.h`、`resource_blocks.h` 和 target 头依赖，`645id_table.h` 移到 adapter 目录；dl698 全部改用 `application/ports/fifo.h`，`fr_698_get` 改用 `nd_fr698_apdu_cache` 访问器。`check_layers.py` 的 STAGING 清空。189/189 编译链接通过，50/50 主机测试通过。ARM 检查仍跳过 adapters。
77. 协议报文主机测试：`__matcher_645` 和 `__matcher_698` 从各自适配器文件中移出，新增 `fr_645_matcher` / `fr_698_matcher`。主机测试直接喂入真实 645 和 698 帧，覆盖正常帧、645 校验和错误和 698 HCS 错误。191/191 编译链接通过，52/52 主机测试通过。
78. 698 组帧主机测试：`fr_698_pack.c` 承接 698 帧头、APDU 字节和帧尾打包，matcher 之外再拆出一个可独立测试的模块。主机测试校验头长度、控制字、地址、APDU 数据和 `0x16` 帧尾。192/192 编译链接通过，53/53 主机测试通过。
79. 645 组帧主机测试：`fr_645_pack.c` 承接 645 帧头、APDU、数据域和帧尾打包，包含本地 FIFO 弹出和校验和回调。主机测试校验地址域、控制字、长度、校验和、`0x16` 帧尾，以及 APDU 的扰码。193/193 编译链接通过，54/54 主机测试通过。
80. 698 APDU 解析测试：`axdr_decode`、`axdr_len_encode` 和 `axdr_get_len` 移到 `fr_axdr_codec.c`，与协议上下文解耦。主机测试覆盖短/长长度形式、长度编码边界、标量、字符串、位串和数组 TLV 长度。194/194 编译链接通过，55/55 主机测试通过。
81. 645 数据域解析测试：`fr_645_parse_91_data` 负责从 91 数据域解析 LSB ID、长度和 payload 偏移，`fr_645_decode_91_from_meter` 改为调用它。主机测试覆盖 ID 字节序、payload 长度和偏移。195/195 编译链接通过，56/56 主机测试通过。
82. adapters 进入 ARM 检查：`tools/check-arm.sh` 不再跳过 `application/adapters`，domain 仍用 `-Wall -Wextra -Werror`，adapters 用 `-fpermissive -w` 加显式 stdint/string 预包含做语法和 Cortex-M0 编译检查。195/195 编译链接通过，56/56 主机测试通过。
83. adapter 警告收紧：去掉 `-w`、`-fpermissive` 和强制预包含，改为在 `-Wall -Wextra -Werror` 下只抑制未使用参数/变量、符号比较、缺花括号、隐式声明、未使用函数、type-limits 和恒真比较这些 legacy 类别。`fr_axdr.h` 补 `stdint.h` 后头文件可独立检查。195/195 编译链接通过，56/56 主机测试通过。
84. adapter 零警告抑制：逐项删除上述抑制。删除未使用静态函数和局部变量；修正 698 服务端 `DIR` 判断（`(ctrl & 0x80) == 1` 恒假，响应帧此前不会被丢弃）；645 的 `unsigned >= 0` 恒真分支展开；698 一致性块补内层花括号；隐式声明改为 `fr_698_env.h` 中消费者侧原型，并把两个读取 `mesg->id` 的目标钩子改为协议本地消息类型；未使用参数在定义中去名。现在 domain 和 application 共用同一套严格 `-Wall -Wextra -Werror` Cortex-M0 检查。195/195 编译链接通过，56/56 主机测试通过。
85. 698 帧解析测试：`fr_698_frame.c` 纳入主机测试，覆盖服务端拒绝 DIR=1 响应帧并消费该帧、接受单地址请求并映射 485-1 通道、通配地址匹配后规范化为台账地址、广播地址只接受 ACTION、通配地址在 APDU 检查后屏蔽为单地址。57/57 主机测试通过。
86. 698 分帧解析测试：`fr_698_segment.c` 纳入主机测试，覆盖 APDU 数据长度随地址长度变化、扰码只在控制字置位时执行、首帧累计并回确认帧、乱序帧清空重装、发送确认帧序号递增。58/58 主机测试通过。
87. 645 地址解析测试：抽出 `fr_645_addr.c`，用 `fr_645_addr_all` 和 `fr_645_addr_match` 替代 `Addr_9_9`、`Addr_A_A`、`Cmp_MeterAddr`，并迁移 `fr_645.c` 与 `ctl_other.c` 的全部调用。主机测试覆盖全 99/AA 地址、精确匹配、尾部 AA 通配填充、非尾部 AA 不填充、非通配模式不改写地址。59/59 主机测试通过，196/196 编译链接通过。
88. 698 连接请求解析测试：`fr_698_connect.c` 纳入主机测试，覆盖空安全、密码安全、对称安全和未知安全选择下的 APDU 偏移计算。60/60 主机测试通过。
89. 698 GET 请求解析测试：`fr_698_get.c` 纳入主机测试，覆盖单个 OAD、OAD 列表、记录选择、记录列表、空选择器拒绝、GetNext 和未知 choice。61/61 主机测试通过。
90. 698 SET/ACTION 请求解析测试：`fr_698_set.c` 纳入主机测试，覆盖单个 OAD、SET 列表、ACTION 列表的选择器和值长度、未知 choice。62/62 主机测试通过。
91. 698 客户端转发测试：`fr_698_client.c` 纳入主机测试，覆盖 DIR=0 请求帧丢弃、分帧帧忽略、响应帧经 `forward` 回调和无回调时排空 payload。63/63 主机测试通过。
92. 645 密码校验测试：抽出 `fr_645_password.c`，`Password_Check` 改为调用 `fr_645_password_matches`。主机测试覆盖忽略首字节、后三字节匹配和失配。64/64 主机测试通过，197/197 编译链接通过。
93. 落位清理：删除未进入任何工程的 `targets/stategrid_meter/app/apps/legacy/app_698_server/stub.c`，以及 `domain/events/legacy/gs_record.h` 中已无定义的 `event_duration_pro` 原型。事件和冻结的 legacy 实现全部留在 `targets/stategrid_meter/{events,freeze}/legacy`，domain 侧只保留接口头。64/64 主机测试通过，197/197 编译链接通过。
94. 组件归属闭环：给 `targets/stategrid_meter` 根文件、`targets/msta`、`targets/host`、`nidavellir/include`、`infra/boot`、`infra/os/baremetal` 补组件清单，`tools/check_components.py` 新增“每个 .c/.h 必须属于某个组件路径”和“include 其他组件必须声明 dependencies”两项检查。组件清单从 27 增至 33，654 个源文件全部有归属，检查通过。
95. 组件依赖图补真：`resource_access.h` 从 `application/apps` 移到 `application/ports`，8 个 include 全部更新；dl645、dl698、application.metering、domain.load_control 的 `dependencies` 按真实 include 补齐。`check_components.py` 修掉相对路径解析漏洞后，跨组件未声明依赖从 44 条降为 0。64/64 主机测试、197/197 编译链接、ARM 与平台边界检查通过。
96. 开关误动作事件迁移：新增 `domain/events/switch_err.c`，把 hold 期间只累计、不推进去抖的规则从 `gs_relay_err_event.c` 移到 domain，legacy 只保留 mbase 和事件上报。4 个主机测试覆盖去抖启动、hold 累计、hold 未激活和 hold 期间结束。65/65 主机测试、198/198 编译链接通过。
97. MAC 挂起规则迁移：新增 `domain/events/mac_suspend.c`，把每日 200 次上限、挂起日期、跨日/零点/挂起日期变化清零和密码计数跨日重置判断移到 domain；legacy 只同步 mbase。5 个主机测试覆盖计数上限、零点清零、日期变化、挂起日期变化和同日保持。66/66 主机测试、199/199 编译链接通过。
98. 编程列表规则迁移：新增 `domain/events/program_list.c`，把 10 条上限计数、首条判断和 698/645 校时 OAD 判定移到 domain；`gs_meter_program_event.c` 只保留 52 字节 EEPROM 列表搬运。4 个主机测试覆盖计数归一、首条判断和两协议校时 OAD。67/67 主机测试、200/200 编译链接通过。
99. RTC 策略迁移：新增 `domain/events/rtc.c`，承接默认时间、每日 12:01:35 快照判断和上电回退/超时判定；`tm_to_mcu_rtc_bcd` 改用 `nd_calendar_to_rtc_bcd` 和 `nd_calendar_weekday`。3 个主机测试覆盖默认值、快照时刻、回退和 1000 天边界。68/68 主机测试、201/201 编译链接通过。
100. 冻结空间换算迁移：新增 `domain/freeze/storage.c`，把 flash 扇区向上取整加预留扇区、EEPROM 字节数加预留字节的规则从 `cal_record_size_sectors` 移到 domain；target 只负责读取深度和累加方案。4 个主机测试覆盖零深度、flash 取整、EEPROM 字节和大记录。69/69 主机测试、202/202 编译链接通过。
101. 冻结回滚判定迁移：`domain/freeze/sequence` 新增 `nd_freeze_rollback_required`，`record_rollback_after_set_time` 不再直接比较时间戳。新增 1 个边界测试，冻结序列测试共 7 个。69/69 主机测试、202/202 编译链接通过。
102. 上电事件恢复迁移：新增 `domain/events/power_event.c`，承接 0xAA 结束时间选择和停电累计时间合并清零；`event_PowerOn_pro` 只做记录查找、活动标志和上报。2 个主机测试覆盖结束时间选择与累计合并。70/70 主机测试、203/203 编译链接通过。
103. 记录捕获 OAD 映射迁移：新增 `domain/events/record_capture.c`，承接校时/编程/默认三类捕获对象映射和普通电能量 OAD 转高精度。`record_capture_change` 只把 record id 映射为类别。4 个主机测试覆盖起止时间、编程首条、默认映射和高精度转换。71/71 主机测试、204/204 编译链接通过。
104. 记录方案可见性迁移：`domain/events/record_capture` 新增 `nd_record_capture_visible`，承接只结束对象、事件起止时间、方案 0-7 分档和 MeterWr/SetTime 覆盖规则。`check_scheme` 只映射 record id。新增 4 个可见性测试，共 8 个。71/71 主机测试、204/204 编译链接通过。
105. 事件目录迁移：记录 id 和冻结 id 移到 `domain/events/event_id.h`，`targets/stategrid_meter/app/port.h` 改为 include 该头；新增 `domain/events/event_catalog.c`，承接事件 id 到 OAD、模式（冻结方案 0-7、发生 10、结束 11）的完整目录。`recv_record_process` 只调用 `nd_event_catalog_lookup`，删除 140 行 switch 和未使用的 `pairs_event`。5 个主机测试覆盖事件对、单事件、表组、冻结组和未映射 id。72/72 主机测试、205/205 编译链接通过。
106. 计量端口不透明化：`metering_energy_port.h` 只暴露 `nd_energy_ledger_t`，不再出现 `instance_t` 或 `mbase_control_block_t`；target 适配器提供 `metering_energy_ledger_from_instance/from_mbase`，legacy 调用点只负责换句柄。新增 `tools/check_instance_boundary.py`，禁止 domain/application include `kernel/instance.h`、`legacy_control_block.h` 或定义这两个布局。72/72 主机测试、205/205 编译链接、ARM/layers/components 通过。
107. 产品清单闭环：`check_components.py` 增加产品依赖闭包检查，五个 `product.yaml` 补齐缺失组件；新增 `tools/product_manifest.py`，按 product.yaml 解析组件闭包并生成确定性的 CMake 源清单。宿主构建新增 `products_check` 目标，每次构建校验产品清单。51 个组件清单，产品闭包检查通过。
108. target 兜底组件拆分：`targets/stategrid_meter` 根清单只列根目录 33 个文件，不再用宽路径加排除项；18 个子目录各自建立 `target.stategrid_meter.<dir>` legacy 清单。组件清单从 33 增至 51，所有源文件仍有归属。
109. 持续集成：新增 `ci.sh`，一次跑宿主构建和测试、旧工程编译链接基线、ARM/layer/component/instance/platform 边界以及产品闭包检查；新增 `.github/workflows/ci.yml` 在 Ubuntu 上安装 `cmake`、`ninja`、`gcc-arm-none-eabi` 后执行同一脚本。本地脚本全绿。
110. 固件最终链接：新增 `targets/stategrid_meter/firmware/gnu/{startup.c,link_stubs.c,link.ld}` 和 `tools/check-firmware-link.py`，按 EWP 源表加 `-Os`、函数/数据分段和 `--gc-sections`，用 V85XXP 的 256K flash / 32K RAM 映射链接出最终 ELF。207 个对象，`nm -u` 为 0。`link_stubs.c` 的 `__CALL` 和 CSTACK 符号只是链接桩；启动文件只有复位向量和默认异常处理，不是已验证的 IAR 运行时替代。
111. 协议样本库骨架：新增 `tests/corpus/README.md` 和 `tests/unit/test_protocol_corpus.c`，用原始字节回放 645/698 matcher，覆盖有效帧和坏校验帧。当前四条是回归种子，不是现场抓包；现场报文到位后按 README 格式追加。73/73 主机测试通过。
112. legacy 组件 include 检查：`check_components.py` 不再跳过 legacy 组件的依赖检查，只对 legacy 组件豁免层方向判定。补齐 protocol.dlms/dl645/dl698、infra.kernel、infra.memory、legacy.kernel_headers 的真实依赖声明。组件清单 52 个，未声明依赖 0。
113. 阶段收口：P1.1/P1.2 和 P3.3 完成；P2.1 完成产品闭包校验和源清单生成，但固件构建仍用 IAR 源表；P2.2 完成根清单拆分和全部组件 include/层方向检查；P2.3 适配器依赖已归位，698 客户端 `forward` 因无消费者仍未绑定；P3.1 需真机；P3.2 只有回归种子；P3.4 已能链接最终 ELF，但启动文件和运行时未验证。
114. 协议端口收口：新增 `application/ports/fifo_channel.h`，把 `nd_fifo_channel_t` 和 FIFO 通道接口从 infra 头移到消费者端口；`infra/channel/fifo_channel.h` 只转引端口。dl645/dl698/dlms 改 include `application/ports/fifo.h` 和 `fifo_channel.h`。`check_components.py` 删除 legacy 的层方向豁免，52 个组件全部检查 include 依赖和层方向，0 违规。
115. 产品清单驱动固件链接：`meter_electricity_single_phase_cn` 选择 39 个组件，覆盖 EWP 的全部 205 个源文件；`tools/check-firmware-link.py --product` 先按组件归属过滤 EWP 源表，任何未选中的源文件都会失败。CI 用该产品名链接，207 个对象（含启动和桩）通过。
116. 持久化探针进 CI：新增 `tools/check-persistence.sh`，编译并运行 `tools/legacy_probe.c`，断言结构体大小 1368、`persistent_section_start` 849、`active_p` 852、`active_n` 904。`ci.sh` 已调用。
117. 产品源清单锁定 IAR 源表：`product_manifest.component_sources` 只输出 `.c`，并修复 `exclude` 对显式文件路径不生效的问题；13 个组件清单补记 55 个未参与 VG_DTZY 构建的 owned 源文件（Windows 驱动、RTOS 内核、legacy 应用/需量/月结、测试用例等）。`check-firmware-link.py --product` 改为按 product.yaml 闭包编译源文件，并要求闭包与 `.ewp` 源表双向一致，只在一侧出现的源文件直接失败。CI 全绿：205 个源文件加启动和桩共 207 个对象，`nm -u=0`。
118. legacy 容器拆分：`infra.boot` 拆为 `infra.boot.app`、`infra.boot.drivers`、`infra.boot.hd`、`infra.boot.updata`；`legacy.kernel_headers` 拆为 `legacy.kernel_headers`（kernel 头）、`legacy.base_headers`、`legacy.dlms_headers`、`legacy.test_support`；`target.msta` 拆出 `target.msta.apps`。`infra.os.baremetal` 只有一个源文件，保持原组件。依赖按真实 include 声明，组件清单从 52 增至 59，其余检查项不变。
119. include 搜索路径检查：`check_components.py` 解析仓库内全部 `.ewp` 的 `CCIncludePath2`，按所属 IAR 工程的文件目录、include 路径、仓库根顺序解析 include；文件级 `exclude` 现在也从检查中跳过。补记真实依赖：dl645→domain.events/legacy.base_headers，dl698→dl645，infra.kernel/soc/security→legacy 头，infra.security→dl698；产品清单同步选择（electricity 42、comm_plc 12）。`legacy.kernel_headers` 通过 `port.h` 反向依赖 `target.stategrid_meter`，已显式声明并注释；尝试把 mbase 段标记宏移出 port.h 会让 40 个文件丢失传递 include，暂不拆分。

## 7. 未完成与下一步

建议顺序：

1. 按 `docs/esam-compatibility.md` 做真实硬件验证：正确 MAC 通过、篡改 MAC 被拒、纯 RN 通过、上报确认、断线重连和掉电重启。权限检查的广播校时和红外分支、以及安全响应尾部编码也需要真机回归。`fr_698.c` 现在已经很薄。协议层状态和客户端路径已分开，后续可以在目标组合根里注册 `forward`。
2. 协议解析主机测试已覆盖 645/698 matcher、pack、645 数据域与地址与密码、698 AXDR、698 帧检查、698 分帧、698 连接/GET/SET/ACTION 请求和 698 客户端转发。adapters 已进入 ARM 检查且不再抑制警告。剩余协议验证需要真机回归：698 服务端 DIR 过滤、目标钩子消息类型、ESAM MAC/RN 和分帧末帧。下一步按 `docs/ROADMAP.md` 走 P1：收窄 `instance_t` 和计量端口，`mbase` 拆分仍暂缓。
3. 清理根目录失效的 solution 文件。
4. 需量和月/阶梯结算的 legacy 实现已随本轮移到 `targets/stategrid_meter/*/legacy`，不在当前固件构建里。排期恢复时把语义迁进 `domain/metering/demand` 和 `domain/tariff`，不要直接复活旧实现。
5. 内核头还可以继续按 device、tasklet、rtc 拆小；当前 `kernel_core.h` 是兼容包。
6. 增加带链接脚本和启动文件的固件链接，以及真实硬件上的运行验证。
7. 下阶段局部重构清单已清空：事件 id 到 OAD 目录已迁入 `domain/events/event_catalog`。产品源清单已成为固件链接输入并与 `.ewp` 双向锁定，legacy 容器已按目录拆开；后续按 `docs/ROADMAP.md`：真机回归、由清单生成 IAR 工程或组合根、698 客户端消费者接入。

## 8. 已知问题与风险

1. 旧代码质量低，已知缺陷：
   - `demand.c` 滑差寄存器从不写入，平均值指针类型错误。
   - `resource.c` 的枚举定义在结构体使用之后，本身编不过。
   - `tasklet.c` 主体在 `#if 0` 里。
   - crypto 缺整个 `inc/` 目录。
2. 旧注释多为 GBK 编码，改动时注意不要破坏编码。
3. 部分文件名带空格，例如 `gs_power_abnormal_event .c`，脚本和命令要加引号。
4. MSVC `.vcxproj` 多数已失效，IAR `.ewp` 是当前可信构建。ARM GCC 基线是新验证手段。
5. 已有整体重定位链接检查，但没有带链接脚本和启动文件的固件链接；中断向量表、IAR 运行时和段布局仍未验证。
6. 持久化兼容是硬约束。任何改动旧结构体字段顺序的行为都要先跑 `tools/legacy_probe.c` 并更新静态断言。
7. 所有提交只在本地，用户要求不 push 远端；工作副本丢失则本轮提交全部丢失。
8. `domain/tariff/legacy/touFee.c` 现在从 legacy 暂存区 include `targets/stategrid_meter/app/tariff_lookup.h`，是阶段性反向依赖；调用方迁出 legacy 后删除。费率表和负荷控制状态都是文件级静态单实例，target 有组合根后再改为实例持有。
9. `relay_control_adapter.c` 和 `relay_control.c` 通过静态状态持有 `load_control_state_t`，并用 `infra/memory/arena` 分配。`metering_energy_adapter.c` 用同样的方式持有 `metering_state_t`。多实例产品必须先改成组合根持有。
10. 电能迁移后旧数组仍是持久化镜像：每次累加先导入、再累加、再导出。校验和、双备份和停电保存仍由旧代码处理。后续可以把这些也搬到 target 持久化适配器里。
11. `applications.h` 已删除，`nidavellir.h` 也不再 include 它。剩余耦合：adapters 仍是 staging，直接 include infra/targets 头；infra 的 `ndv_RN8209.c`、`esam_pro.c`、`nidavellir.c` 仍读 `port.h` 和 `kernel/legacy_control_block.h`。层检查对前两条用 staging 跳过，最后一条靠字面路径没有层前缀而不报。

## 9. 死文件清单

2026-11-10 核查：10 个文件都不在 IAR 工程里，也没有任何编译源 include 或引用。已删除 7 个：

- `domain/metering/legacy/electrical measurement.c`（重复，正式版是 `electrical_measurement.c`）
- `domain/metering/legacy/iv/iv_data.c`（重复 `baremetal_instance_entry`，本身含语法错误）
- `domain/display/legacy/display.c`（被 `application/apps/display.c` 取代）
- `application/meterbase/unbuilt/factory.c`
- `application/meterbase/unbuilt/parainit.c`
- `application/meterbase/unbuilt/tasklet.c`（重复 `infra/kernel/tasklet.c`）
- `application/meterbase/unbuilt/meterbase.h`
- `infra/board/esam698_win.c`（Windows 专用，未进入任何工程）

仍保留，2026-11-10 用户确认保留：

- `domain/metering/legacy/demand.c`：需量检测的旧实现。当前固件没有启用需量，`get_1_Demand.c` 直接返回 `OBJECT_NOT_EXIST`，也没有任何调用方创建需量对象。保留作未来范围的参考。
- `domain/tariff/legacy/gs_month_bill.c`、`gs_step_bill.c`：月结算和阶梯结算的旧实现。当前固件没有接这两个文件，保留作未来范围的参考。

恢复这些功能时，先把行为迁进 `domain/metering/demand` 和 `domain/tariff`，再删除 staging 文件；不要直接把它们重新编进 IAR 工程。

## 10. 新会话启动清单

1. 克隆或进入仓库。
2. 读 `docs/architecture.md`、`docs/ROADMAP.md` 和 `.audit/ddd-refactor.todo.md`。
3. 跑一遍全部验证命令，确认基线。
4. 从第 7 节第一项开始。
5. 每完成一个单元，更新 `.audit/ddd-refactor.todo.md`，在本地提交。

用户要求只 commit，不 push 远端。不要执行 `git push`，也不要建议推送。

## 11. 关键提交

```
3e91536b Delete the applications umbrella
64a1ea3a Move the record group into gs_record.h
6b815622 Cut the kernel umbrella's dependency on applications
405ddc56 Tolerate non-UTF-8 compiler output in the baseline
31c956a0 Move the first applications.h declarations and the control block into layers
3f108114 Add the demand registry to the application layer
ead99a6a Move energy accumulation onto the metering aggregate
75dac290 Move the freeze sequence rules into the domain
5fdb1b37 Update the handoff for the freeze and link units
66204a0c Link the legacy object set and check for undefined symbols
f4ac485a Route the power-down path through the event state machine
71738849 Rewrite the freeze power-on catch-up as a domain planner
25ad0903 Rewrite the freeze triggers as a domain module
946a24b2 Keep the legacy compile checks working on GCC 14
9d4ed9b3 Finish the event migration and remove event_duration_pro
f1b49ccd Route the legacy event call sites through the new domain modules
895ccf13 Rewrite the over-current, cover-open and power-off events
9b6eabd6 Extract the event debounce state machine
a4ac0b40 Split meterbase into bounded-context legacy directories
9973867f Move test packages and kernel build files out of packages
91980208 Move applications into application/apps
96e23734 Stage meterbase under domain
d30b7296 Move instances to targets
8ca68ab6 Move protocol packages into application/adapters
1af091c3 Make the strict ARM check state the legacy skip
99356b4b Move OS, security, upgrade and boot into their layers
```
