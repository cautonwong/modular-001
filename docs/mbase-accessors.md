# mbase 访问器与 Linux 外设文件模拟方案

日期：2026-11-12
基线：`94bc0da3`
状态：方案待确认，未开工

## 1. 目标

1. `struct mbase_control_block_t` 的布局只被持久化层看到。其余 target 代码通过不透明句柄和领域方法访问。
2. Linux 主机用文件模拟 EEPROM 和 NOR flash，持久化路径可以真实运行、可以回归。

## 2. 硬约束

- 结构体不拆。`sizeof` 1368，`persistent_section_start` 849，`active_p` 852，`active_n` 904 不变。
- 只做本地提交，不 push。
- IAR 仍是权威固件构建。主机验证行为，不替代真机。
- 迁移业务规则时先补 domain 测试，再删旧实现。

## 3. 现状数字

- 79 个文件引用结构体，约 1460 处箭头访问，其中约 68 个文件参与构建。
- 结构体 213 个字段，179 个被真实访问；489 次读，616 次写，250 次取地址，48 次数组索引。
- 布局操作：112 处 `offsetof` 分布在 24 个文件，70 处 `nd_set_offset`，29 处 `sizeof(字段)`，62 处 `memcpy/memset`。
- 分域统计：适配器 6 个文件约 117 处；mapper_645 13 个文件 324 处；mapper_698 24 个文件 404 处；legacy 功能 21 个文件 352 处；组装入口 4 个文件约 92 处。
- 已有三种访问器模式：
  - `application/adapters/dl698/meter_base_view.h` 加 `targets/stategrid_meter/app/meter_base_adapter.c`，30 个指针 getter，231 次调用。
  - `fr_645_env` 加 `targets/stategrid_meter/app/fr_645_env_adapter.c`，12 个指针 getter，104 次调用。
  - `fr_698_env` 加 `targets/stategrid_meter/app/fr_698_env_adapter.c`，管 instance 和 pipeline，不碰 mbase 字段。
- `domain/metering/metering_energy_port.h` 已把电能账本变成不透明 `nd_energy_ledger_t`。
- 3 个只出现在 `#if 0` 死代码里的字段引用不存在，位于 `targets/stategrid_meter/mapper_698/map_698_action/action_f_Other.c:640-691`：`alarm_threshold1`、`alarm_threshold2`、`prepay_para_update`。生成访问器之前先删除。
- 文件模拟已有先例：`targets/stategrid_meter/board/eeprom_win.c` 用固定大小、`0xFF` 填充的磁盘文件实现 EEPROM 和 flash 驱动，但整文件包在 `#ifdef WIN32` 内。
- `tests/legacy/app_resource_tool_test_resource.c` 已经装配过文件设备，但没有接入 CMake/CTest。

## 4. 领域设计

### 4.1 mbase 的定位

`mbase_control_block_t` 不是领域聚合，而是一条遗留持久化记录。它把九个限界上下文的字段混在一个 1368 字节的布局里。DDD 的处理不是拆结构体，而是把它降级为 `target.stategrid_meter.persistence` 的私有存储格式，对外按上下文提供聚合句柄。

### 4.2 上下文地图

| 上下文 | 代表字段 | 消费者 | 接口位置 | target 实现 |
|---|---|---|---|---|
| 计量与电能 | `active_p/n[_dec]`、`MonthEnegery`、`UseEnergy`、`Energy_chksum`、`meter_const`、`power_dir` | domain.metering | `domain/metering/metering_energy_port.h`（已有） | `metering_energy_adapter.c` |
| 需量、费率、阶梯 | `TOUNo`、`STEPNo`、`StepSeason`、`Price`、`TOUPrice`、`STEPPrice`、`StepNum`、`Fee` | domain.tariff、645/698 mapper | `domain/tariff/tariff_state_port.h`（新增） | `tariff_state_adapter.c`（新增） |
| 事件与记录 | `*_status`、`*_duration`、`*_acc_time`、`active_flag`、`record_flag`、事件参数 | domain.events、698 mapper | `domain/events/event_state_port.h`（新增） | `event_state_adapter.c`（新增） |
| 冻结 | `minute_freeze_seq`、`frz_*`、`trigger_insfreeze_delay_sec` | domain.freeze | `domain/freeze/freeze_state_port.h`（新增） | `freeze_state_adapter.c`（新增） |
| 安全与 ESAM | 连接、认证超时、`rand8_645`、`AuthOn`、`key_state`、MAC 与密码计数 | domain.security、dl645/dl698 | `domain/security/security_state_port.h`（新增） | `security_state_adapter.c`（新增） |
| 通信与上报 | `Baut_*`、`clientaddr`、上报模式字、`report_2015state`、`report_piid`、645 上报状态 | domain.report、dl645/dl698 | `domain/report/report_state_port.h`（新增） | `report_state_adapter.c`（新增） |
| 显示 | LCD 时长、`AutoPtr`、`BackLightTmr`、`Page_ID`、`commdisp_*` | target display | `application/ports/display_state.h`（新增） | `display_state_adapter.c`（新增） |
| 负荷控制与继电器 | `relay_ctl_mode`、`cmd/mode/sta`、`alarmmin`、`relay_do`、`switch_*_succees` | domain.load_control、relay 适配器 | `domain/load_control/relay_state_port.h`（新增） | `relay_state_adapter.c`（新增） |
| 运行与测量 | 电压电流功率、温度、电池、`meter_start_cnt`、`metertype`、`run_state` | 计量输入、显示 | `application/ports/metering_input.h`（已有，补全） | `metering_input_adapter.c` |
| 持久化布局 | `static_*`、`dyn_*`、chksum、空白区、`id`、`os_fields` | 无外部消费者 | 仅 `targets/stategrid_meter/persistence/` | `legacy_energy_store.c` 等 |

### 4.3 两层接口

第一层是 target 内部的底座。生成器从结构体定义读取字段名和类型，产出 `targets/stategrid_meter/persistence/meter_record.h`：每个字段一个 offset、size、rows 常量和 lvalue 访问宏。

```c
#define nd_mbase_offset_active_p() ((uint32_t)offsetof(...))
#define nd_mbase_size_active_p()   ((uint32_t)sizeof(...))
#define nd_mbase_active_p(m)       ((m)->active_p)
```

选择头文件宏而不是 `.c` 函数句柄的理由：

- 616 次写和 250 次取地址，lvalue 宏同时覆盖读、写、取地址和数组下标，替换是纯机械的。
- 112 处 `offsetof` 和静态初始化需要常量表达式，函数调用做不到，宏可以。
- 不新增 IAR 源表条目，205 源清单和链接基线不动。
- 调用方只拼宏名，不再拼字段名；布局只出现在生成的 persistence 头里。

`struct mbase_control_block_t *` 仍然出现在 target 内部签名里。硬约束是 domain/application 不看到布局，`check_instance_boundary.py` 继续强制；target 内部文件本来就知道 mbase。

第二层不变：跨层消费者用已有端口（`metering_energy_port`、`meter_base_view` 等），target 适配器实现。本轮没有新增领域端口，因为 adapter 的 117 处访问改为宏后，跨层接口已经由现有端口满足。

第二层是消费者端口。每个跨层消费者只声明它需要的领域方法，由 target 适配器实现，内部再调第一层。

```c
/* domain/tariff/tariff_state_port.h，消费者是 domain.tariff */
struct nd_tariff_state;
struct nd_tariff_state *tariff_state_from_instance(struct instance_t *);
uint32_t nd_tariff_current_price(struct nd_tariff_state *);
uint8_t nd_tariff_switch_period(struct nd_tariff_state *, uint8_t slot);
```

原则：

- 有业务行为的地方给领域方法，不给 getter。例如事件状态用 `nd_event_begin/end/accumulate`，不用逐字段读写。
- 只有数组、缓存和纯数据表保留类型化 getter。
- 跨层消费者（domain/application）的接口头放在消费者自己的组件里。target 内部文件的接口头放在 target 对应目录。
- 持久化适配器是唯一允许 `offsetof`、`sizeof`、整块 memcpy 和校验和计算的地方。

### 4.4 组件与复用

- 领域方法模块留在 `domain/<context>/`，继续做纯函数和主机测试。
- 端口头归消费者组件。`component.yaml` 的 dependencies 由 `check_components.py` 按真实 include 强制。
- 文件模拟驱动作为可复用测试支持放 `tests/support/`，实现现有 `nd_store_t`。如果以后 Windows 仿真也要用，再提升到 `components/services/block_store`。

## 5. 迁移阶段

每个阶段独立提交，并以 `./ci.sh` 全绿加专项检查结束。

- P0 脚手架。已完成。`nd_store_t` 的文件实现、`backupfiles/` 持久化回归、`tools/check_mbase_access.py` 加 allowlist。初始基线 1536 次访问、162 次布局操作、56 个文件。
- P1 持久化收口。已完成。生成 `meter_record.h` 的 offset/size 宏，24 个文件的 `offsetof` 机械替换；布局操作先降到 21，收尾后只剩生成头。
- P2 适配器收口。已完成。6 个 adapter 文件 117 处访问改为生成宏，adapter 目录直接访问为 0。
- P3 上下文迁移。已完成。事件、冻结、费率、安全、上报、显示、继电器、运行状态、计量和入口文件全部改为生成宏。没有新增领域端口，现有端口加宏访问器已满足跨层边界。
- P4 mapper 迁移。已完成。645 和 698 mapper 全部改为生成宏，行为由既有 74 个主机测试和 205 源 ARM 编译保护。
- P5 归零。已完成。allowlist 为空，`check_mbase_access.py` 报 0 次访问、0 次布局操作；布局只存在于生成的 `persistence/meter_record.h`。

## 6. Linux 文件模拟

探查结论：主机侧已有 `nd_store_t` 存储缝，`targets/host/composition.c` 已经用它做电能 load/save。直接走固件资源管理器和 `dev_info_t` 不划算，`resource_tool` 把地址存进 `uint32_t`，在 64 位 Linux 上会截断，`pid_t` 也会和 glibc 冲突。

- 新增 `tests/support/file_store.c`，实现 `nd_store_t`：open/pread/pwrite/ftruncate，固定大小，`0xFF` 初始化，越界检查照 `infra/memory/byte_store.c`。不新增产品组件，避免清单改动。
- `backupfiles/` 放两个文件。`eeprom.bin` 按 EEPROM 设备大小初始化，持久化回归使用 1368 字节的 mbase 镜像。`flash.bin` 按 256K 和 4K 扇区初始化，提供擦除、写、读。
- 新增 `tests/unit/test_file_persistence.c`，被 CTest 的 glob 自动收录：
  1. 打开 `backupfiles/eeprom.bin`，把 mbase persistent 段映射到 offset 849。
  2. 执行计量累加、`legacy_energy_save`，关闭并重开文件，`legacy_energy_load` 到新的计量状态，断言电能值。
  3. 断言 `sizeof(struct mbase_control_block_t) == 1368`、`persistent_section_start == 849`、`active_p == 852`，并检查 852 起 172 字节记录。
  4. 对 `flash.bin` 做擦除、写、读、重开回读。当前产品没有 flash 消费者，先做存储层回归。
- 路径默认相对当前目录的 `backupfiles/`，可用参数或环境变量覆盖；CTest 在 `build/` 下运行，所以文件落在 `build/backupfiles/`。
- 不在本期：完整固件 main、资源管理器、调度器、UART/PLC/IRQ/看门狗、真机驱动。避免把主机回归当成真机验证。

## 7. 验证

- `tools/check_mbase_access.py`：统计每个文件直接字段访问，allowlist 只减不增；新增访问直接失败。2026-11-12 归零。
- `tools/gen_meter_record.py --check`：生成头与结构体定义不一致时失败，CI 已接入。
- `tests/unit/test_file_persistence.c`：1368 字节 mbase 镜像的保存、重开、回读，以及 flash 擦写回归。
- `./ci.sh` 全绿：74 个主机测试、205/205 固件源、207 个对象、链接未定义 0、偏移探针不变。

## 8. 风险与取舍

- mapper 是最大风险面。生成宏让替换机械化，但 645/698 表驱动代码的读写语义仍要靠主机测试和样本回放兜底。
- 写入和取地址占多数（616 加 250），所以访问器做成 lvalue 宏，而不是只读函数。
- 112 处 `offsetof` 和整块拷贝是隐藏布局的真正难点。offset 宏解决常量部分；整块 memcpy 仍留在持久化适配器和 legacy 代码里，后续可按命名 load/save 继续收。
- 未参与构建的 legacy 文件仍 include 结构体头。生成器覆盖展开后的 239 个字段，但本轮机械迁移只处理构建内文件；未构建文件由 allowlist 基线排除。
- target 内部仍能看到 `struct mbase_control_block_t`。这是有意的：硬约束只禁止 domain/application 看到布局，`check_instance_boundary.py` 负责那条边界。
- Linux 上不要走固件设备路径。`resource_tool` 用 `uint32_t` 存指针，64 位会截断；定义 `WIN32` 会拉进 `windows.h`；`pid_t` 与 `time.h` 冲突。文件模拟统一走 `nd_store_t`。
- 跳过 architect 多模型竞技：用户已给定 DDD 原则，仓库已有 view 端口先例，形态已具体；代价是没有第二套独立设计对比。
