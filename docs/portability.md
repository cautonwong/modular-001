# Blade 可移植性顶层设计

状态：2026-11-10 草案。平台目录和可见性以 `docs/platform-layout.md` 为准，产品线扩展步骤见 `docs/guides/product-platform-extension.md`。

本文只覆盖平台可移植性，包括 SoC、board、RTOS 和 domain profile。整体组件化 monorepo 设计，包括水表、燃气表、通信模块和协议组件，见 `docs/monorepo.md`。两者冲突时以 `docs/monorepo.md` 为准。SoC、board 和 Zephyr 设备模型的具体接缝见 `docs/device-model.md`。

目标是一套核心代码支持多种产品组合：

- 不同 SoC，例如当前 v85xxp，后续其它计量 MCU。
- 不同 board，按原理图变化引脚、外设和器件。
- 不同 RTOS，裸机、FreeRTOS、Zephyr 或厂商调度器。
- 不同 domain，当前国网单相表，后续国网三相表、外销 DLMS 表等。

硬约束仍然是旧持久化字节逐字节兼容。

## 1. 产品组合模型

一个产品由四部分组成。

```
product = domain profile + protocol adapters + platform(soc + board + rtos)
```

- domain profile 决定计量量、费率模型、事件集合、对象模型和结算规则。
- protocol adapters 决定对外协议，例如 DL/T 698.45、DL/T 645、DLMS/COSEM。
- platform 决定代码跑在什么硬件和什么调度器上。
- targets 是组合根，只做选择、绑定和启动，不实现业务规则。

domain 和 application 不出现 `#ifdef SOC_V85XXP`、`#ifdef BOARD_XXX` 或 `#ifdef RTOS_FREERTOS`。产品差异通过编译期选择实现文件和运行期绑定端口来表达。

## 2. 目录布局

保持现有四层，不增加第五层。平台差异放进 infra 的子目录。

```
domain/
  common/                 通用值对象和规则
  metering/
  tariff/
  freeze/
  events/
  load_control/
  profiles/
    single_phase/         国网单相表的量测和对象组合
    three_phase/          国网三相表
    dlms_export/          外销 DLMS 表
application/
  ports/                  消费者定义的端口
  metering/               应用用例
  apps/                   产品应用入口
  adapters/
    dl698/                国网 698 协议适配
    dl645/                国网 645 协议适配
    dlms/                 外销 DLMS 适配
infra/
  kernel/                 队列、FIFO、链表、内存等通用原语
  memory/
  storage/
  comms/
  security/
  soc/<soc>/              寄存器、启动文件、厂商库、SoC 外设
targets/
  <product>/              组合根、产品配置、IAR/CMake 工程
    board/                引脚表、板级驱动、器件连接
    rtos/                 具体调度入口和时间实现
    soc/                  产品专用的 SoC 胶水代码
```

`domain/profiles` 只组合 domain 内的值对象和规则，不直接操作硬件。`infra/soc` 是 SoC 实现；board 和产品专用 RTOS 代码放在 `targets/<product>`。共享业务代码通过消费者端口访问它们。

## 3. 依赖规则

依赖方向只向内，沿用四层规则。

| 源层 | 允许 include |
|---|---|
| domain | domain |
| application | domain, application |
| infra | domain, application, infra |
| targets | domain, application, infra, targets |

补充约束：

- domain 不 include infra、targets、协议包或厂商头。
- application、domain 和共享组件不 include `infra/soc`、`targets/<product>/board`、具体 RTOS 或 Zephyr 头。
- infra 的通用部分不 include 某个 target 的产品配置。
- target 可以 include 全部，但 target 代码里不写业务规则。
- 协议适配器只做编码解码和对象映射，不直接读引脚、不直接写寄存器。

## 4. 端口目录

端口由消费者定义，实现放在 infra 或 target。端口只暴露值对象和不透明句柄，不暴露结构体布局。

平台服务端口：

- `platform`：看门狗、事件输出、系统节拍、复位、低功耗。
- `gpio`：按键、继电器反馈、LED、告警输出。
- `uart`、`spi`、`i2c`：通信外设。
- `flash`、`eeprom`：参数和记录存储。
- `adc`、`meter_frontend`：电压、电流、脉冲、计量芯片。
- `lcd`、`key`：显示和按键。
- `relay`：继电器输出和反馈。

业务服务端口：

- `parameter_store`：参数读写，按参数 ID 或领域键。
- `record_store`：事件和冻结记录。
- `security`：ESAM、认证、随机数和 MAC。
- `comm_channel`：通道发送、接收和通道状态。
- `time_source`：日历、秒脉冲、校时。

端口命名以消费者为中心。例如 dl698 用 `parameter_port` 和 `meter_base_view`，计量用 `metering_energy_store`，显示用 `display_port`。同一个硬件能力可以有多个消费者端口，target 负责把它们绑到同一个实现上。

厂商 SoC HAL 和 RTOS 可以原样放在 `infra/soc`、`infra/rtos`，保留厂商头文件结构和构建方式。product/target 直接 include 和调用它们来实现这些端口。共享 application、domain 和 components 不 include 厂商头，也不调用具体 RTOS API。

## 5. 组合根与构建

每个产品一个组合根，放在 `targets/<product>/`。

组合根负责：

1. 选择 domain profile。
2. 选择协议适配器集合。
3. 绑定 platform、storage、security、comms、display 等端口。
4. 初始化板级和 SoC。
5. 创建实例并进入主循环或 RTOS 任务。

产品差异只在组合根和构建文件里出现。建议每个产品提供一个 `product_config.h`，只包含产品选择宏，不包含实现细节。

构建方式：

- IAR 工程按产品一份，例如 `targets/<product>/iar/<product>.ewp`。
- CMake 用 preset 或 cache 变量选择 product、soc、board、rtos。
- host 测试只编译 domain、application 和纯 infra，不编译 SoC 和 board。

## 6. Domain 变体

domain 变体不是复制整套代码，而是组合和参数化。

- `domain/metering` 提供单相和三相共用的能量、需量、功率方向规则。
- `domain/profiles/single_phase` 选择单相量测、单相事件集合和国网对象映射。
- `domain/profiles/three_phase` 增加分相量测、相序、缺相、三相不平衡等规则。
- `domain/profiles/dlms_export` 选择 DLMS 对象模型、费率结构和外销事件集合。
- 协议适配器只依赖 domain 的值对象和用例接口，不知道具体 profile。

新增一个 domain 时，优先新增 profile 和协议适配器，不改已有 profile 的行为。

## 7. 新增 SoC、board、RTOS 的步骤

新增 SoC：

1. 在 `infra/soc/<soc>/` 放启动文件、寄存器定义、厂商库和 SoC 外设实现。
2. 具体实现可以直接 include 寄存器头和厂商库，不抽通用 HAL。需要被共享业务代码使用的能力通过消费者端口进入。
3. product/target 显式选择这些实现，组合根直接调用具体函数。
4. 不改 domain 和 application。

新增 board：

1. 在 `targets/<product>/board/` 放引脚表、器件描述和板级初始化。
2. 复用 SoC 实现，只替换引脚和外设连接。
3. 在 target 组合根里选择 board。
4. 不改 domain、application 和 SoC。

新增 RTOS：

1. product/target 显式选择具体 RTOS 并直接 include 它的头。不新增通用 RTOS adapter。
2. 需要内核对象时直接使用该 RTOS 的 API，不把 `k_*` 或 RTOS 句柄带进 domain、application 和共享组件。
3. `infra/rtos/<rtos>` 可以存放具体实现，但不是共享抽象层，只被对应 product 选中。
4. 换 RTOS 时复制 product target 和构建配置，改具体依赖。不做 adapter。前提是共享 application/domain 不 include 具体 RTOS 头。

新增 domain 或协议：

1. 在 `domain/profiles/<profile>/` 组合领域规则和对象模型。
2. 在 `application/adapters/<protocol>/` 实现新的协议适配器。
3. 在 target 组合根里选择和绑定。
4. 已有 domain 规则尽量复用，不复制。

## 8. 当前落地

这一轮已经落下第一批可移植性接缝。

- `application/ports/platform.h` 和 `platform.c` 定义平台服务端口，包含看门狗和事件输出。
- `targets/stategrid_meter/app/platform_adapter.c` 把 v85xxp 看门狗和 PLC 事件引脚接到端口，启动时由组合根绑定。
- `domain/common/status.h` 提供与平台无关的电源和事件状态值。
- dl698 目录不再直接 include `port.h`、`v85xxp_system.h`、`cpu_surport.h`、`pin_function_def.h`、`lib_clk.h`、`lib_uart.h` 或 `infra/kernel/resource_blocks.h`。
- `meter_base_view` 把旧单体控制块收口到 target 适配器，dl698 不再直接访问其字段。
- `targets/stategrid_meter/board/meter_status.h` 声明 board 和 security 需要的表类型、计量芯片故障和 ESAM 电源状态，target 的 `meter_status_adapter.c` 在旧控制块上实现。`ndv_RN8209.c`、`esam_pro.c`、`nidavellir.c` 不再直接读写这些字段。
- `infra/security/esam/esam_status.h` 声明 ESAM 芯片异常、密钥、认证开关、表号和认证超时状态，target 的 `esam_status_adapter.c` 负责映射和保存。`fr_698_esam.c` 不再直接访问旧控制块。
- `targets/stategrid_meter/board/meter_config.h` 固定表类型、计量校准和自热补偿参数 ID。`infra/` 目录已经不再 include `targets/stategrid_meter/app/port.h`，`fr_698_esam.c` 也不再 include `cpu_surport.h`。
- `targets/stategrid_meter/board/board_io.{h,c}` 只提供背光、PLC 事件、继电器 LED 和反馈、表盖、按键这些语义接口。IIC、SPI、UART/RS485、RN8209、ESAM 和继电器线圈的线级操作在 `iic_pins`、`spi_pins`、`uart_pins`、`rn8209_pins`、`esam_pins`、`relay_pins` 私有头里，对应驱动只 include 自己需要的那个。原 `pin_function_def.h` 已删除，寄存器访问只在 board 实现文件里。`board_io` 是 stategrid 专用实现，不计划抽通用 board 接口。按 `docs/device-model.md`，board 在构建期选定，组合根直接调用具体 board 函数。
- `domain/display/backlight.h` 和 `domain/load_control/relay_status.h` 是消费者定义的端口。显示和运行状态代码不再 include `board_io.h`，stategrid target 在组合根里把端口直接绑到 `board_io`。host 测试用假实现验证绑定和调用。
- `domain/metering/metering_input.h` 提供温度和电池输入，返回旧 ADC 原始值。stategrid target 的 `metering_input_adapter.c` 负责映射到板级 ADC 通道，计量 legacy 代码不再直接调用 MCU ADC。
- `infra/board` 已合并到 `targets/stategrid_meter/board`。依赖 board 的 `nidavellir.c`、`time_link.c`、`cpu_v85xxp_isr.c`、`esam_pro.c`、`fr_698_esam.c` 移到 target 的 `rtos`、`soc`、`security`。`tools/check_platform_boundary.py` 已接入 `check-legacy.sh`，共享代码里的平台 include 和 RTOS 调度调用会失败；application/apps 的平台依赖已清零，SoC/RTOS 调用 staging 依赖也已清零；0 条 legacy kernel 头依赖进入 ratchet baseline。

## 9. 迁移顺序

1. 把 `application/adapters/dl698` 从 `nidavellir.h` 伞头解耦，改成显式包含领域和端口头。
2. ~~把 `targets/stategrid_meter/pin_function_def.h` 的引脚宏拆到 board 层，由 board 端口暴露语义。~~ 引脚头已删除，线级操作在私有 pin 模块里，语义接口走 `board_io`。
3. ~~保持 `board_io` 的 stategrid 专用定位，把 IIC、SPI、UART 线级操作移到 board 驱动私有头。~~ 已完成。应用和领域需要的共享能力通过消费者端口进入。
4. 把 `infra/kernel` 里依赖 target 的部分收口到 `targets/<product>/rtos` 或对应平台实现，不定义通用 RTOS adapter。
5. 建立最小 Zephyr 组合根和一块 Zephyr 支持板的 overlay，只跑背光和秒节拍，先证明 board 实现和调度入口能替换。
6. 把 `infra/soc/v85xxp` 的启动、寄存器、厂商库和 SoC 外设整理成可替换目录。
7. 建立 `domain/profiles/single_phase`，把当前国网单相表的量测、事件和对象组合显式化。
8. 为三相和 DLMS 各建 profile 和协议适配器骨架，先用 host 测试锁定规则。
9. 每个产品建立独立组合根和 IAR/Zephyr 工程，host 测试保持一套。

验收标准：

- domain、application 和共享组件的 include 图里没有 `infra/soc`、`targets/<product>/board`、具体 RTOS、Zephyr、devicetree 或 `CONFIG_*` 平台宏。
- 新增一个 SoC 只新增 `infra/soc/<soc>` 和对应 board 实现，不改 domain 和 application。
- 新增一个 board 只新增 `targets/<product>/board`、devicetree overlay 和 target 配置，不改 SoC 和上层。
- 新增一个 RTOS 复制 product target 和构建配置，改具体依赖。共享 domain、application 和协议适配器不改。不做通用 adapter。
- Zephyr 产品能和 IAR 产品共用 domain、application 和协议源码，只替换 board 实现、调度入口和构建文件。
- 新增一个 domain profile 不改变已有 profile 的持久化和上报行为。
- 旧持久化切片、事件记录和冻结记录的字节回归测试持续通过。
