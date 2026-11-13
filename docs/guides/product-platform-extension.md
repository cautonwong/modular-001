# 产品线平台扩展指南

本文面向产品线和平台组。目录归属和可见性规则见 `docs/platform-layout.md`。这里只给步骤和检查清单。

## 0. 先选组合

一个产品在 `products/<product>/product.yaml` 里选择四类东西。

```yaml
name: meter_electricity_single_phase_idis
components:
  - domain.metering
  - application.metering
  - protocol.dlms
  - soc.v85xxp_v8530
  - driver.rn8209
  - rtos.baremetal
```

产品目录只放组合根。

```
targets/<product>/
  app/          main、对象创建、端口绑定
  board/        产品默认 board 包，引脚、器件实例、初始化
  config/       产品参数
  build/        IAR/CMake/链接脚本
```

## 1. 新增 SoC

适用场景。引入一颗新的计量 MCU。

1. 建目录 `infra/soc/<vendor>/<soc>/`。现有例子是 `infra/soc/v85xxp/v8500`，厂商头放在其 `vendor/` 子目录。
2. 原样放入厂商 HAL、启动文件、链接脚本和 CMSIS 依赖。不要改厂商文件。
3. 加 `component.yaml`。

```yaml
name: soc.<vendor>_<soc>
version: 0.1.0
layer: platform
paths: [infra/soc/<vendor>/<soc>]
dependencies: []
```

4. 提供最小启动路径。时钟、看门狗、系统节拍、UART 控制台。
5. 在 `targets/<product>/app` 里绑定 `nd_clock_port_t`、`nd_store_t`、`nd_measurement_port_t` 等端口。
6. 加 host 编译冒烟测试。没有硬件的部分用 fake 端口。

不要做。不要把 SoC 头文件暴露给 domain 或 components。不要在 `infra/soc` 里写产品业务。

## 2. 新增 board

适用场景。同一 SoC 换一块板，引脚、器件、电源时序不同。

默认规则是一块 board 对应一个产品，board 放 `targets/<product>/board/`。

1. 建目录 `targets/<product>/board/`。
2. 放板级文件。 `board.c`、`pins.c`、`devices.c`、`board.h`。
3. 板级文件只做三件事。配置引脚，实例化 `infra/drivers` 里的器件，按顺序初始化。
4. 产品选择自己的 board。`product.yaml` 里不再单列 board 组件，board 随产品 target 构建。
5. 只有当第二个产品明确复用同一块 board 时，才把它提升到 `infra/boards/<board>/`，并给原产品保留差异覆盖。
6. `infra/boards/` 作为跨产品 board 的位置。现有 `infra/boards/stategrid_single_phase` 同时服务 645/698 两个产品，已经按这个规则提升。

检查。board 不 include 其他 `targets/`。board 不写费率、对象模型、事件规则。

## 3. 新增 RTOS

适用场景。从裸机换到 FreeRTOS 或 Zephyr。

终态没有 `nidavellir`。它是待删除的 legacy 内核，不再作为新产品的选项。

FreeRTOS 按 `docs/device-model.md` 的 target 边界处理，不做通用 RTOS 适配层。

1. 外部内核 checkout 放 `infra/third_party/freertos/`（目录不提交），或在构建时设置 `FREERTOS_KERNEL_DIR` 指向 FreeRTOS 11.3.3；保留 LICENSE、`portable/GCC/ARM_CM0`、`portable/MemMang`。
2. 复制 product target 为 `targets/<product>_freertos/`。
3. 在复制 target 的 `app/` 放 `FreeRTOSConfig.h` 和 `freertos.c`；`freertos.c` 只做任务、队列、工作队列、ISR 投递和端口接线。
4. `infra/rtos/freertos/` 只作为具体实现/胶水目录；它不是共享抽象层，不定义 `nd_os_port_t`，不提供公共 adapter。
5. target 的 CMake 直接加入内核、CM0 port、heap 和 `FreeRTOSConfig.h`；内核源码不复制到 target。
6. target 直接 include `FreeRTOS.h` 并调用 `xTaskCreate` 等 API；共享层仍不得出现这些头或调用。
7. 复制 target 不保留对原 target 的 include；CMake 源列表和链接脚本一起复制并改名。

检查。`tools/check_platform_boundary.py` 只检查 domain、application、components 和 `infra` 非平台实现；target 可以直接调 RTOS API。新代码不要让 domain 出现 `k_thread`、`ndv_tasklet`、`xTaskCreate`。`nidavellir` 相关 include 只减不增。

## 4. 新增驱动

适用场景。新增计量芯片、EEPROM、LCD、继电器、M-Bus 收发器。

1. 建目录 `infra/drivers/<class>/<chip>/`。`<class>` 用 `metering`、`storage`、`display`、`io`、`comm`。
2. 驱动只依赖总线和 GPIO 端口，不直接读 board 全局变量。
3. 加 `component.yaml`，`layer: platform`。
4. board 负责实例化驱动并传入引脚和总线配置。
5. 驱动加 host 测试。用 fake 总线验证寄存器序列。

检查。驱动不 include `targets/`。驱动不包含产品校准参数。校准属于 target 的配置。

## 5. 引入第三方库

适用场景。NetX crypto、lwIP、mbedTLS、FreeRTOS、CMSIS。

1. 放 `infra/third_party/<vendor>/<library>/`，保留上游目录结构和 LICENSE。
2. 不改上游源码。需要配置时用编译宏或适配层包装。
3. 建 `infra/<domain>/<vendor>_adapter/`，把上游 API 翻译成消费者端口。
4. 只有 `infra` 适配层和 `targets` 可以 include `infra/third_party`。
5. 在 `component.yaml` 的依赖里记录适配层，不把上游类型写进公开头。

检查。domain、application、components 不得出现 `infra/third_party/` 或厂商头。适配层公开头只暴露值对象和不透明句柄。

## 6. 新增产品

1. 建 `products/<product>/product.yaml`，选择业务组件、SoC、driver 和 RTOS。board 不在这里选，默认随产品 target。
2. 建 `targets/<product>/`，只放 `app/`、`config/`、`build/` 和必要的 `board/` 覆盖。
3. 在 `app/main.c` 里创建对象、绑定端口、启动节拍。
4. 加产品测试。至少覆盖启动、端口绑定、一次完整业务链。
5. 跑 `tools/check-idis-prep.sh` 或对应产品的检查脚本。
6. 确认 `product.yaml` 里每个名字都能在 `component.yaml` 中找到。

## 7. 定制已有产品

按改动类型选位置。

| 改动 | 放哪里 |
|---|---|
| 引脚、器件、电源时序 | `infra/boards/<board>` 或 `targets/<product>/board` |
| 计量芯片型号 | `infra/drivers/metering/<chip>` |
| 费率、对象模型、事件规则 | `domain` 或 `components` |
| 协议版本 | `components/protocol/<protocol>` |
| 产品参数默认值 | `targets/<product>/config` |
| 一次性产品差异 | `targets/<product>` |

不要为了一个产品改 `infra/soc` 的厂商文件。不要为了一个产品在 domain 加 `#ifdef`。

## 8. 提交前检查

- `tools/check_layers.py` 通过。
- `tools/check_platform_boundary.py` 通过，baseline 没有新增。
- `tools/check_components.py` 通过，`product.yaml` 的选择都能解析。
- `tools/check_rtos_targets.py` 通过，`*_freertos` target 的配置/胶水齐全且未复制内核。
- 新目录有 `component.yaml`，`layer` 正确。
- domain、application、components 没有平台头。
- `targets` 之外没有 include `targets/`。
- `infra/third_party` 只被 `infra` 适配层和 `targets` include。
- 新驱动有 host 测试，新 board 有冒烟启动测试，新产品有完整业务链测试。
