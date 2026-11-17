# 平台层目录与可见性规范

状态：2026-11-13 提案。适用于 `modular-001` monorepo 的多 SoC、多 board、多 RTOS 产品线。

本文只回答三件事。平台代码放在哪。谁能 include 谁。产品线如何扩展和定制。扩展步骤见 `docs/guides/product-platform-extension.md`。

## 1. 结论

平台不是一层，而是四类资产加一个第三方区。

```
platform = soc + drivers + board + rtos
infra/third_party = 原样引入的上游代码
```

产品只做装配，不实现平台细节。

```
product = domain profile + protocol adapters + platform(soc + board + rtos)
```

平台差异只允许出现在 `infra/soc`、`infra/drivers`、`infra/rtos` 和 `targets/<product>`。domain、application、components 不出现具体 SoC、board、RTOS 的头文件或条件编译。

`nidavellir` 已在 Wave 6 删除。目录、include 路径、组件清单和检查器例外一起移除；调度循环归 product target。

## 2. 目录布局

```
infra/
  soc/
    <vendor>/<soc>/            # 厂商 HAL、启动文件、链接脚本、SoC 外设
  drivers/
    <class>/<chip>/            # 可复用器件驱动，例如 metering/rn8209
  rtos/
    <rtos>/                    # 具体 RTOS 实现/胶水；不做共享抽象
  third_party/
    <vendor>/<library>/        # 上游原样代码，附 LICENSE 和版本
    arm/cmsis/                 # CMSIS core
    netx/crypto/               # 现有 infra/security/crypto 的归属
    freertos/                  # 外部 RTOS checkout，原样引入；内核目录不提交
    zephyr/                    # 外部 RTOS 内核，原样引入
  boot/
    <boot>/                    # 独立 boot 镜像和升级工具
  security/                    # 自研安全适配层
  kernel/                      # 旧 kernel，legacy
  memory/                      # 旧 infra，legacy

targets/
  <product>/                   # 组合根
    app/                       # main、任务创建、端口绑定
    board/                     # 产品默认 board 包：引脚、器件实例、初始化
    config/                    # 产品参数
    build/                     # IAR/CMake/链接脚本
    tests/                     # 产品级测试

products/
  <product>/
    product.yaml               # 选择 components 和 platform 组件

```

多 SoC 用 `infra/soc/<vendor>/<soc>` 区分。多 RTOS 用 `infra/rtos/<rtos>` 区分。board 放 `targets/<product>/board`，因为一块 board 对应一个产品。多个产品共享时复制，不设 `infra/boards`。第三方库统一放 `infra/third_party/<vendor>/<library>`，不改上游目录结构。

## 3. 归属规则

判断归属只看两个问题。这段代码是否被多个产品复用。它是否直接依赖具体硬件或 RTOS。

| 资产 | 目录 | 归属 | 复用边界 |
|---|---|---|---|
| 厂商 SoC HAL、启动、链接 | `infra/soc/<vendor>/<soc>` | 平台组 | 同一 SoC 的所有 board |
| 器件驱动 | `infra/drivers/<class>/<chip>` | 平台组 | 同一器件的所有 board |
| 产品 board 包 | `targets/<product>/board` | 产品线 | 一块 board 对应一个产品，共享时复制 |
| RTOS 具体实现/胶水 | `infra/rtos/<rtos>` | 平台组 | 只被选中的 target 构建；不形成共享抽象 |
| 产品组合根 | `targets/<product>` | 产品线 | 不对外复用 |
| 产品独有板级覆盖 | `targets/<product>/board` | 产品线 | 只服务该产品 |
| 上游库 | `infra/third_party/<vendor>/<library>` | 平台组 | 只允许 infra 适配层 include |
| 业务能力 | `domain/`、`components/` | 领域组 | 所有产品 |

三个容易放错的点。

- **一块 board 对应一个产品**。board 放 `targets/<product>/board`。多个产品需要同一块板时复制一份到各自的 target，不建 `infra/boards`。
- **driver 不放在 board 里**。RN8209、EEPROM、LCD 这类器件驱动放 `infra/drivers`，board 只提供引脚和设备实例。
- **RTOS 内核、配置和产品胶水分开**。外部内核放 `infra/third_party`。`FreeRTOSConfig.h`、任务创建和端口接线放选中的 product target；`infra/rtos/<rtos>` 只放具体实现/胶水，不定义共享 `nd_os_port_t`，也不要求 domain/application 走统一 OS 抽象。

## 3.1 RTOS 选择规则（2026-11-13 修订）

RTOS 不做通用适配层。具体 RTOS 的选择、配置和绑定只发生在 product target。

- 外部内核原样放 `infra/third_party/freertos/`（或对应 RTOS 目录），保留 LICENSE 和版本。
- `FreeRTOSConfig.h`、`freertos.c`、任务/队列/工作队列和端口接线放 `targets/<product>_freertos/app/`。
- `infra/rtos/freertos/` 只允许放具体实现或可复用的 glue；它不是共享抽象层，不暴露 `nd_os_port_t`，也不强制 domain/application 使用统一 OS API。
- 需要换 RTOS 时，复制 product target 和构建配置，改具体依赖；共享 layer 仍不得出现 `FreeRTOS.h`、`xTaskCreate`、RTOS 句柄。
- target 可以直接 include 具体 RTOS 头并调用其 API；`tools/check_platform_boundary.py` 只禁止 domain、application、components 这样做。

## 4. 可见性矩阵

行是消费者，列是提供者。`Y` 表示允许 include，`-` 表示禁止。

| 消费者 \ 提供者 | domain | application | components | infra/soc | infra/drivers | infra/rtos | infra/third_party | targets |
|---|---|---|---|---|---|---|---|---|
| domain | Y | - | - | - | - | - | - | - |
| application | Y | Y | Y | - | - | - | - | - |
| components | Y | Y | Y | - | - | - | - | - |
| infra/soc | - | - | - | Y | - | - | Y | - |
| infra/drivers | - | - | - | Y | Y | - | Y | - |
| infra/rtos | - | - | - | Y | - | Y | Y | - |
| infra/security | - | - | - | Y | - | Y | Y | - |
| targets/<product> | Y | Y | Y | Y | Y | Y | Y | Y |

补充规则。

- `infra/third_party` 只能被 `infra` 适配层和 `targets` include。domain、application、components 一律禁止。
- `targets` 是唯一同时看见全部层的目录。其他 target 不允许互相 include。
- `products/<product>/product.yaml` 只做选择，不写代码，不 include 任何头。
- `infra/drivers` 只依赖总线回调和自己的头，不 include board 引脚或产品配置。
- RTOS API 只允许在 `infra/rtos` 和 `targets` 里直接调用。共享业务代码只使用消费者定义的端口。

## 5. 组合与构建

产品描述是唯一的平台选择点。

```yaml
# products/meter_electricity_single_phase_idis/product.yaml
name: meter_electricity_single_phase_idis
components:
  - domain.metering
  - application.metering
  - protocol.dlms
  - soc.v85xxp_v8530
  - driver.rn8209
  # RTOS 由 target 具体选择和绑定，不在 product.yaml 里做通用抽象
```

规则如下。

- 每个 SoC、driver 目录都带 `component.yaml`，`layer: platform`，`name` 使用 `<类别>.<名字>`。RTOS 不做通用组件选择，由 target 直接绑定。
- 产品 board 在 `targets/<product>/board`，不进入 `component.yaml` 选择。多个产品共享时复制到各自 target，不设 `board.<名字>` 组件。
- `product.yaml` 的 `components` 列表选择业务组件和平台组件；RTOS 由 target 直接选择和绑定。`tools/check_components.py` 校验名字存在。
- `targets/<product>/app` 只写组合根。创建对象、绑定端口、启动节拍，不写业务规则。
- `product.yaml` 是选择描述；当前 CMake 仍显式列出源文件，迁移到读取描述前必须保持两边一致。IAR 工程和 CMake 目标都不应出现两套互相矛盾的平台清单。
- 同一产品可以选多个 board 变体，但一次构建只选一个。变体差异放在 `targets/<product>/board`。

## 6. 强制检查

新增目录后同步扩展三个检查器。否则规范只是一段文字。

| 检查器 | 需要的改动 |
|---|---|
| `tools/check_layers.py` | `ALLOWED` 允许 `infra` 和 `targets` include `infra/third_party`，其余层禁止 |
| `tools/check_platform_boundary.py` | `PLATFORM_PREFIXES` 增加 `infra/drivers/`、`infra/third_party/`；`PLATFORM_IMPLEMENTATION` 增加 `infra/drivers`、`infra/third_party` |
| `tools/check_components.py` | 第三方库默认不进入产品选择；需要选择时把 `infra/third_party` 纳入组件扫描并增加 `thirdparty` 层 |
| `tools/check_rtos_targets.py` | `targets/*_freertos` 必须有 `app/freertos.c` 和 `app/FreeRTOSConfig.h`，内核不得复制到 target，不能 include 其他 target |

当前已经生效的规则保持不变。

- domain 不得 include `infra`、`targets`、`infra/third_party`。
- 共享层不得出现 `cpu_surport.h`、`target.h`、`lib_uart.h`、`zephyr.h` 等平台头。
- `tools/platform_boundary_baseline.txt` 只减不增。

## 7. 当前代码的迁移映射

| 现在 | 目标 | 说明 |
|---|---|---|
| `infra/soc/v85xxp` | `infra/soc/v85xxp/v8500` | 已完成。厂商头文件在 `vendor/`，源码在 SoC 目录，补 `component.yaml` |
| `infra/soc/cm0` | `infra/third_party/arm/cmsis` | 已完成。CMSIS 是上游代码 |
| `infra/soc/cpu_surport.h` | `infra/soc/v85xxp/v8500/cpu_surport.h` | 已完成。SoC 头随 SoC 走 |
| `infra/security/crypto` | `infra/third_party/netx/crypto` + `infra/security/netx_adapter` | 上游库与适配层分开 |
| `infra/os/baremetal` | 已删除 | 第二个 main 不在任何 product；裸机循环在 target |
| `infra/boards/stategrid_single_phase` | `targets/<product>/board` | 已按复制拆分到 stategrid 和两个 IDIS target，`infra/boards` 删除 |
| `targets/stategrid_meter/app/rtos` | 已删除 | 内容迁到 `stategrid_context` 和 target 主循环 |
| `targets/stategrid_meter/board/cpu_v85xxp_isr.c` | 已从 target `soc/` 移到 `board/` | 板级中断入口归 target board；SoC 入口在 infra/soc |
| `infra/boot` | `infra/boot/vg8530` | boot 是独立镜像 |
| `nidavellir/` | 已删除 | 需要的调度能力由 target 主循环或外部 RTOS 提供 |

迁移按此顺序完成：先建目录和 `component.yaml`，再搬 driver/RTOS 胶水，最后改 `product.yaml`、构建并删除旧位置。board 默认留在产品 target，只有第二个产品复用时才提升。

## 8. 边界

- 平台层不实现业务规则。计量、费率、事件、冻结的语义只在 domain 和 application。
- board 文件只描述引脚、器件实例和初始化顺序。校准参数、费率表、对象模型不属于 board。
- 第三方库暴露的原生类型不穿透到 domain。适配层把它翻译成消费者端口的值对象。
- 产品线不修改 `infra/soc` 的厂商文件。需要改行为时在 `targets/<product>` 做覆盖。
