# 产品线平台扩展指南

本文面向产品线和平台组。目录归属和可见性规则见 `docs/platform-layout.md`。组件的标准形状和模板见 `docs/guides/component-standard.md`，端口绑定见 `docs/guides/port-binding.md`，验收用例见 `docs/guides/acceptance-case.md`。这里只给步骤和检查清单。

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

## 9. 裸机还是 RTOS

先看业务需要的并发度，不要先选内核。

| 条件 | 选择 |
|---|---|
| 所有业务能在一个秒级 tick 里顺序跑完，驱动是轮询或短事务 | 裸机 |
| 有阻塞式驱动、协议栈、持久化后台任务或明确的优先级 | RTOS |
| RAM 余量小于 8 KiB，或 ROM 余量小于 2 KiB | 裸机，先做预算 |

裸机 target 的参考是 `targets/idis_ip_meter`，RTOS target 的参考是
`targets/idis_ip_meter_freertos`。两者共用业务对象，不共用任务模型。

规则。共享层不出现 OS API。domain、application、components 不 include
`FreeRTOS.h`，也不定义 `nd_os_port_t`。target 直接调用内核 API。不做通用
RTOS 抽象层。

## 10. RTOS 下怎么编程

推荐模型。一个业务 task 加一个工作 task，按职责分，不按外设分。

1. 业务 task 周期延迟 10 ms，秒变化时执行一次业务 tick。参考
   `targets/idis_ip_meter_freertos/app/freertos.c` 的 `app_task`。
2. 慢操作放工作 task。持久化、慢总线、文件写入走队列，请求合并。
3. 跨 task 只传消息或完成信号。队列传请求，二值信号量传完成，不用全局忙等。
4. 对象模型只允许一个 task 修改。写者 task 的优先级要低于修改者，或者写者在
   保存期间暂停修改者。当前 IDIS FreeRTOS target 让持久化 worker 优先级高于
   app task，app task 是唯一对象修改者。
5. ISR 只用 `FromISR` API，投递队列或任务通知。ISR 不调用业务逻辑、不阻塞、
   不写 EEPROM。
6. 掉电保存走队列加有限超时。超时值按 PMU 保持时间定，写在代码注释里。
7. 看门狗在长操作里也要喂。EEPROM 写端口已经在每次写前后清看门狗。
8. 栈按最坏路径测量。`vApplicationStackOverflowHook` 必须保留。
9. 优先级不要全相同。相同优先级的 task 用时间片，行为难复现。

检查。没有在 ISR 里调用阻塞 API。没有无界队列。没有两个 task 同时写同一个
对象或同一路 EEPROM。没有在 domain 出现 `xTaskCreate`、`k_thread`、
`ndv_tasklet`。

## 11. target 的组织和接线

target 是组合根，只做选择、创建和绑定。

```
targets/<product>/
  app/          main、对象创建、端口绑定、任务入口
  board/        引脚、器件实例、上电顺序
  config/       产品参数默认值
  build/        CMake/IAR/链接脚本
  tests/        产品单元和验收测试
```

接线顺序固定，方便对照和排错。

1. 启动 SoC。时钟、看门狗、系统节拍。
2. 初始化 board。GPIO、I2C/SPI/UART、继电器和显示引脚。
3. 打开存储和 flash 端口。
4. 创建对象模型。静态实例或组合根分配的实例。
5. 绑定端口。时钟、存储、计量、继电器、显示、通信，逐个检查非 NULL。
6. 加载持久化状态。
7. 应用产品默认值。
8. 启动节拍或调度器。

端口用编译期结构体或函数指针绑定，不做运行时注册表。target 可以 include
平台头和 RTOS API，共享层不可以。board 默认放 target 下，第二个产品明确复用
时才提升到 `infra/boards/`。

验收。target 自己的外部契约由 `targets/<target>/tests/acceptance/` 覆盖，
共享测试只测共享逻辑。

## 12. 改已有组件时怎么保持兼容

产品清单按名字引用组件，改名就是破坏性改动。

1. 组件改名。同时更新所有 `products/*/product.yaml`，跑 `./ci.sh` 的
   `check:products`。不能只改一边。
2. 公共头。只加新函数、新字段。不改已有签名，不重排已有结构体字段。
3. 持久化布局。字段顺序和偏移是冻结的。`tools/check-persistence.sh` 和
   `tools/legacy_probe.c` 会检查。加字段要版本化记录并写迁移。
4. 端口。端口由消费者定义。给端口加方法会打到所有 target 实现。优先新增
   可选函数指针并判 NULL，或者新增端口。
5. 枚举和 OBIS 常量。只追加，不重编号。脚本表和 profile 会引用编号。
6. 旧 API。先迁移所有调用方，再删除旧实现。不要留长期转发壳。
7. 前向兼容。新字段放尾部，读取方忽略未知尾部字节，默认值取零。
8. 回归门禁。`check-persistence.sh`、`check_idis_audit.py`、target 验收和
   `check:products` 必须一起过。

## 13. 门禁清单

统一入口只有一个。

```sh
cd /home/edge/modular-001
FREERTOS_KERNEL_DIR=/home/edge/FreeRTOS-KernelV11.3.3 ./ci.sh
```

`ci.sh` 调用 `tools/ci_matrix.py`，按 target 发现并执行。

host 验证分四层。`host:test` 用默认优化跑单元测试；`host:strict` 用
`-Wall -Wextra -Werror -Wshadow -Wundef -Wstrict-prototypes
-Wmissing-prototypes -Wvla` 加 AddressSanitizer/UndefinedBehaviorSanitizer
重跑同一批测试；`host:analyzer` 用 GCC `-fanalyzer` 对生产库做路径敏感分析；
`host:sim-san` 用 ASan 和 UBSan 跑整机仿真、stategrid 验收矩阵和两个 IDIS 验收套件。
任何一层失败都算 CI 失败。约束写法、穷举边界和已知盲区见
`docs/guides/verification.md`。

干净检出验证。它把 HEAD 导出到临时目录并在那里跑同一份 `ci.sh`，只依赖 git
跟踪的文件。失败时保留目录，成功后可加 `--keep` 保留。

```sh
FREERTOS_KERNEL_DIR=/home/edge/FreeRTOS-KernelV11.3.3 \
  python3 tools/clean_clone_check.py
```

共享检查。

- `check:components`。组件清单、依赖方向、include 归属。
- `check:component-standard`。组件命名、版本、层和路径符合 `docs/guides/component-standard.md`。
- `check:domain`。domain 契约。
- `check:rtos`。`*_freertos` target 的配置和胶水齐全，内核未被复制。
- `check:ids`。IDIS OBIS 常量表。
- `check:id-docs`。docs/idis 存在时跑 Blue Book、对象缺口和脚本目标校验；
  缺失时明确跳过。
- `check:audit`。IDIS 属性、方法和脚本的失败预算。
- `check:acceptance-matrix`。stategrid 122 行验收矩阵和用例一致性。
- `check:layers`。分层方向。
- `check:target-kernel`。target 里残留的 `instance`、`pipeline`、`nidavellir` 引用棘轮。
- `check:idis-twins`。两个 IDIS target 的 COSEM 和 mapper 副本必须一致。
- `check:platform`。平台边界 baseline。
- `check:meter-record`。计量记录生成结果。
- `check:param-tables`。mapper_698 参数表长度不超过编译后的数组大小。
- `check:conversion`。生产库的转换告警不新增类别、不增长计数。
- `check:dead-code`。`#if 0` 死代码块不新增、不增长。
- `check:products`。产品组件闭包和依赖环。

target 检查。

- `stategrid_meter`。legacy 编译、ARM 严格编译、持久化偏移、实例边界、
  mbase 访问、固件链接。
- `idis_ip_meter` 和 `idis_ip_meter_freertos`。CMake ARM 构建、
  `check_idis_firmware.py`（ELF section RAM/ROM、wired symbols、102 KiB
  预算断言）、11 个 Python 验收用例。
- `host`。共享单元测试，当前 127 个，另跑 strict 和 analyzer 两层。
- `msta`。没有 build descriptor，矩阵跳过。

新 target 接入。放 `app/CMakeLists.txt`、`build/CMakeLists.txt` 或 IAR `.ewp`，矩阵会自动发现。
没有构建描述符的目录会被跳过，不会静默当成功。
