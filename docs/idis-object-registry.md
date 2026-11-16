# IDIS 对象注册表设计

## 1. 背景

Association LN 的属性 2 是 `object_list`。当前实现只放 5 个条目：clock、
有功电能、association、security setup、SAP assignment。条目编码已经符合
IDIS 对象模型，但列表不是完整的对象注册表。

审核要求让 `nd_idis_objects_t` 的实际注册信息成为 object_list 的唯一来源。
本文记录为什么现在不直接做全量注册表，以及后续可执行的三条路径。

## 2. 实测数据

数据来自 `idis_host_runner --audit` 的 2026-11-14 运行结果。

| 指标 | 数值 |
|---|---:|
| audit 表中的唯一对象实例 | 358 |
| 至少一个属性返回 OK 的对象 | 354 |
| 全量条目最小 ROM（class 1 B + version 1 B + OBIS 6 B） | 2478 B |
| 全量条目含访问权限（约 8 B/条） | 2832 B |
| object_list 响应大小（约 28 B/条） | 9912 B |
| FreeRTOS ROM 余量（ELF section，102 KiB 预算） | 1388 B |
| IDIS 裸机 ROM 余量 | 5477 B |
| FreeRTOS RAM 余量（64 KiB） | 11872 B |

结论。354 条全量注册表放不进 FreeRTOS 的 1388 B ROM 余量。裸机余量也不够
带访问权限的全量表。object_list 响应约 9.9 KiB，超过常见单帧 APDU，需要
block transfer 或子集策略。

## 3. 可选方案

### A. 全量 ROM 表

不可行。至少 2478 B，FreeRTOS 只有 1324 B。

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
2. FreeRTOS ROM 余量只有 1324 B，任何全量方案都要先解决预算。
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

### 预算（实测）

| 项目 | 数值 |
|---|---:|
| 注册表 RAM（360 × 8 B） | 2880 B |
| blob 上限（354 条） | 2842 B |
| 裸机镜像 ROM | 96103 B / 102400 B |
| FreeRTOS 镜像 ROM | 100248 B / 102400 B |

FreeRTOS 余量 2152 B，裸机余量 6297 B。本次回收来自把 22 个 profile 描述里的
`captures[12]` 固定数组改成共享的扁平 capture_pool，ROM 减少约 1.9 KiB。
object_list 全量响应约 9.9 KiB，仍需要 block transfer；当前 APDU 实现无法一次返回 354 条。
