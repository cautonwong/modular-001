# IDIS 对象注册表设计

## 1. 背景

Association LN 的属性 2 是 `object_list`。当前实现只放 5 个条目：clock、
有功电能、association、security setup、SAP assignment。条目编码已经符合
IDIS 对象模型，但列表不是完整的对象注册表。

审核要求让 `nd_idis_objects_t` 的实际注册信息成为 object_list 的唯一来源。
本文记录为什么现在不直接做全量注册表，以及后续可执行的三条路径。

## 2. 实测数据

对象数来自 `idis_host_runner --audit` 的 2026-11-19 运行，与 2026-11-14 相同。
ROM 和 RAM 来自 2026-11-19 的构建：pinned ARM GNU 14.3.rel1（GitHub CI 固定
工具链）和本机 Zephyr 14.3.0。

| 指标 | 数值 |
|---|---:|
| audit 表中的唯一对象实例 | 358 |
| 至少一个属性返回 OK 的对象 | 354 |
| 全量条目最小 ROM（class 1 B + version 1 B + OBIS 6 B） | 2478 B |
| 全量条目含访问权限（约 8 B/条） | 2832 B |
| object_list 响应大小（约 28 B/条） | 9912 B |

ROM 预算 103 KiB（105472 B），数值为 checker 口径：

| lane | pinned ARM 14.3.rel1 | local Zephyr 14.3.0 |
|---|---:|---:|
| FreeRTOS IRQ | 104836 B，余 636 B | 102104 B，余 3368 B |
| FreeRTOS POLL | 102148 B，余 3324 B | 99416 B，余 6056 B |
| 裸机 IRQ | 100492 B，余 4980 B | 97759 B，余 7713 B |
| 裸机 POLL | 98008 B，余 7464 B | 95275 B，余 10197 B |

RAM 预算 64 KiB（65536 B）。两套工具链的数字相同：

| lane | RAM | 余量 |
|---|---:|---:|
| FreeRTOS IRQ | 58100 B | 7436 B |
| FreeRTOS POLL | 56588 B | 8948 B |
| 裸机 IRQ | 45560 B | 19976 B |
| 裸机 POLL | 44048 B | 21488 B |

把全量表放进最窄的 FreeRTOS IRQ lane：

| 口径 | ROM 余量 | 2478 B 最小表 | 2832 B 权限表 |
|---|---:|---:|---:|
| pinned ARM 14.3.rel1 | 636 B | 放不进 | 放不进 |
| local Zephyr 14.3.0 | 3368 B | 余 890 B | 余 536 B |

结论。354 条全量注册表的最小 ROM 是 2478 B。按 pinned CI 口径，FreeRTOS IRQ
只剩 636 B，放不进。按本机 Zephyr 口径，2478 B 能放进 3368 B，但只剩 890 B；
带访问权限的 2832 B 只剩 536 B，任何后续增长都会顶破预算。所以按 CI 口径不可行，
按本机口径只剩极小余量，产品上仍不建议全量表。裸机 IRQ 在两个口径下都放得下
权限表，但同一功能也要在 FreeRTOS 上成立。object_list 响应约 9.9 KiB，超过
常见单帧 APDU，需要 block transfer 或子集策略。

口径说明。ROM 以链接脚本的 `_sidata + SIZEOF(.data)` 断言为准。
`check_idis_firmware.py` 的 `rom=` 现在是 load span：`.isr_vector`、`.text`、
`.ARM.exidx` 和 `.data` 的 LMA 跨度，含 section 间对齐填充，等于 `.bin` 大小。
上表的数字按旧口径（section 大小之和，且不计 `.ARM.exidx`）记录，比 load span
小 9–10 B；按 load span 口径，两处能放进的余量各少 9–10 B，结论不变。

## 3. 可选方案

### A. 全量 ROM 表

按 CI 口径不可行。pinned ARM 14.3.rel1 的 FreeRTOS IRQ 放不下 2478 B 的最小表。
按本机 Zephyr 14.3.0 口径，最小表能放进，但只剩约 0.9 KiB；带访问权限的表只剩
约 0.5 KiB（第 2 节表）。两套口径的差别不改变产品结论，全量表仍不建议。

### B. ROM 子集表

按优先级选最多约 160 条，放进 ROM。

- 优点。实现简单，启动即可用，不依赖外部存储。
- 缺点。列表不完整，选择哪些对象是产品决定。
- 适用。产品明确只暴露一个子集，且客户端接受该子集。

### C. 外部注册表

注册表放 EEPROM 或 SPI flash，启动时加载到 RAM。

- 优点。不占 ROM，可放全量 354 条。
- 缺点。需要产线工具写入，增加一个持久化失败模式；RAM 约 2.8 KiB。
- 适用。需要完整 object_list，且产品已经有参数写入流程。

### D. 保留当前子集

保留 5 条，加 dispatcher 校验测试，防止条目和实现漂移。

- 优点。零 ROM 成本，当前 CI 已覆盖。
- 缺点。object_list 不完整，客户端无法发现大部分对象。
- 适用。DLMS 传输层还没接，对象发现不是当前交付目标。

## 4. 建议

现在保留 D。理由如下。

1. DLMS 传输层还没有接，object_list 没有真实客户端消费。
2. FreeRTOS IRQ 的 ROM 余量放不进最小全量表（第 2 节），任何全量方案都要先解决预算。
3. 全量列表的响应需要 block transfer，当前 APDU 路径没有该能力。

等传输层和客户端需求明确后，优先做 C。如果产品只暴露声明子集，做 B。

## 5. 后续实现顺序

1. 写 host 探针工具。遍历 `tests/gen/all_obis.csv`，用
   `nd_cosem_port_get` 属性 1 判断对象是否存在，输出支持的对象表。
   不要手写第二份清单。
2. 生成器输出紧凑表。class_id 1 B，version 打包，OBIS 6 B，访问权限按需。
3. Association LN 从注册表编码 object_list。
4. 加两条测试。每个条目都能被 dispatcher 解析；注册表尺寸不超过预算。
5. 如果走 C，产线工具负责写入注册表，启动时校验 magic 和长度。

## 6. 未决问题

- 客户端是否要求完整 object_list，还是接受声明子集。
- APDU 缓冲区和 block transfer 的支持范围。
- 注册表由固件镜像、EEPROM 参数还是产线工具提供。
- 访问权限的最小集合，是否只暴露 attribute 1。


## 7. C 路径实现（2026-11-15）

外部注册表已经落地，默认仍保留 5 条内置对象。

### 数据格式

EEPROM 偏移 55296（`EEPROM_REGISTRY`，宿主机和两个固件 target 一致），大端：

| 偏移 | 内容 |
|---:|---|
| 0..3 | `NDRG` |
| 4 | 格式版本 1 |
| 5 | 条目大小 8 |
| 6..7 | 条目数 |
| 8.. | 每条 8 B：class_id、version、6 字节 OBIS |
| 末尾 2 B | 前面所有字节的 16 位累加和 |

上电时 `nd_idis_registry_load` 校验 magic、版本、条目大小、条目数、每个 class_id 非零和校验和。
任何一项失败都清空注册表，`load_persisted` 随后回落到 5 条内置对象。

注册表最大 blob（360 条）覆盖 `0xD800` 到 `0xE34A`。其后到能量快照 `0xE800`
之间的空闲区存放 16 B 出厂校准记录，偏移 `0xE400`
（`targets/idis_ip_meter/cosem/nd_idis_calibration.h`）。`0xE400` 是唯一权威
槽位。校准记录只在 magic 匹配且三个 int32 因子都大于零时可用，没有校验和。
`0xE000` 与第 255 条注册表条目及 255 条 blob 的尾部校验和重叠，所以只在
`0xE400` 没有可用记录时读取；读到有效旧记录后写入 `0xE400`（写入失败下次
上电重试），此后不再读 `0xE000`，因为该偏移属于注册表 blob。产线工具必须
改写到 `0xE400`；继续写 `0xE000` 的工具会破坏注册表第 255-256 条，固件无法
阻止。

上电时，magic 存在但因子非正的记录，以及注册表 blob 已覆盖 `0xE000` 的情况，
都以 COSEM 事件 14（NV memory error）上报并保留编译期默认因子。空介质，或
`0xE000` 被注册表以外的内容覆盖，与从未校准无法区分，直接使用默认因子且不
报事件；这是已记录的部署风险，不是可检测的故障。头文件里的 `_Static_assert`
把注册表尾、校准槽和 `0xE800`/`0xEC00` 两个能量快照串成一条链，单独改动任一
常量都无法通过编译。

### 运行时模型

`nd_assoc_state_t` 存放紧凑注册表（最多 360 条，RAM 2880 B；blob 最大 2890 B）加最多
8 条运行时 `add_object` 条目。object_list 按 class 生成访问权限：
class 8 为属性 2 读写，class 3/4 为属性 2 只读，其余为属性 1 只读。

### 生成与写入

```sh
python3 tools/build_idis_registry.py \
  --runner build/ci-host/idis_host_runner \
  --out registry.bin --eeprom eeprom.bin
```

工具跑 `--audit`，取属性 1 返回 OK 的对象（当前 354 条，2842 B），生成 blob，
并按需写入 EEPROM 镜像。产线写入流程复用现有参数上传通道。

### 预算

| 项目 | 数值 |
|---|---:|
| 注册表 RAM（360 × 8 B） | 2880 B |
| blob 上限（354 条） | 2842 B |

镜像 ROM 余量见第 2 节。本次回收来自把 22 个 profile 描述里的 `captures[12]`
固定数组改成共享的扁平 capture_pool，ROM 减少约 1.9 KiB。object_list 全量响应
约 9.9 KiB，仍需要 block transfer；当前 APDU 实现无法一次返回 354 条。
