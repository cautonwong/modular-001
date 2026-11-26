# IDIS 需量记录持久化契约

日期：2026-11-23。实现：`targets/idis_ip_meter/cosem/classes/nd_ic_demand.c` 的
`nd_ic_demand_save` / `nd_ic_demand_load`；两个 target 各一份相同源码，由
`tools/check_idis_twins.py` 校验。测试：`targets/idis_ip_meter/tests/unit/test_idis_demand.c`。

## 1. 记录布局

固定 274 B 单条记录：

| 偏移 | 长度 | 内容 |
|---|---|---|
| 0 | 4 | magic `0x44454D31` |
| 4 | 264 | 6 个 `nd_idis_demand_state_t`，每个 44 B |
| 268 | 4 | version，当前为 1 |
| 272 | 2 | CRC-16/CCITT，poly `0x1021`，初值 `0xFFFF` |

前 268 B 与旧固件读写的 legacy 记录逐字节一致；尾部 6 B 是新固件追加的。
CRC 覆盖 magic、items 和 version，即 CRC 之前的全部 272 B。

## 2. 加载规则与 legacy 窗口

1. magic 不匹配返回 -1。
2. version 为空白 `0xFFFFFFFF` 时按 legacy 记录接受，不校验 CRC。旧固件只写
   268 B，尾部在擦除态 EEPROM 中保持 0xFF；下一次保存补写 version 和 CRC。
3. 其他 version 必须是 1，且 CRC 必须匹配；否则返回 -1。
4. 只有全部校验通过后才拷贝 items；失败时调用者的需量状态保持不变。

legacy 与脏尾的判定只看 version 字，不要求尾部其余字节全为 0xFF。因此空白
version 加被弄脏的 CRC 字节仍按 legacy 接受；这是旧固件的兼容窗口，不是撕裂
写路径：撕裂写要么留下空白 version（按 legacy 接受），要么留下非空 version
和旧 CRC（被 CRC 门禁拒绝）。

## 3. 脏尾与降级再升级

- **脏尾。** 尾部非空白但不构成有效记录时返回 -1。来源包括：新固件保存中断
  只写了部分尾部、旧固件在新记录上覆写了 268 B 前缀、外部写入或介质损坏。
  调用方 `targets/idis_ip_meter/app/main.c` 忽略返回值，需量保持初始化值，
  因此**需量会清零一次**；下一次 300 s 周期保存或断电保存写出完整记录，之后
  不再重置。
- **降级再升级。** 旧固件只覆写 268 B 前缀，不更新 version 和 CRC。升级后
  version 仍是 1，但 CRC 覆盖的 items 已变，加载返回 -1，同样**需量清零一次**。
  这是 fail-closed 的代价；把这种尾部当 legacy 接受，会把旧固件写入的 items
  和新固件留下的 version/CRC 拼成一条看似有效的记录。

## 4. CRC-16 边界与 A/B + CRC-32 的触发条件

- CRC-16/CCITT 对随机损坏的漏检率是 2^-16 = 1/65536。它只门禁非空白
  version 的记录：撕写若留下空白 version（0xFFFFFFFF），整条记录按 legacy
  接受，CRC 不参与。它挡随机损坏和位翻转，不是安全校验，也不防故意构造。
- 需量记录用**单条固定偏移 + 追加 CRC-16**：旧固件按固定偏移读写 268 B
  前缀，追加尾部不改变旧固件的视图；需量丢失一个窗口可以接受，加载失败
  重置一次即可。
- A/B 双 slot + CRC-32 用于能量脉冲存储：IDIS base 记录是两个 1 KiB slot
  （`docs/design/energy-pulse-store.md` §4.2）；stategrid 绑定的是两个 188 B
  snapshot slot（172 B blob + 12 B header + 4 B CRC-32，§4.3、§8.1）。恢复时
  校验 CRC 和 generation，取有效且 generation 最大的 slot，用于撕裂写回退。
- 需量记录升级到 A/B + CRC-32 的触发条件是：记录必须跨撕裂写保留上一份有效
  值、需要 generation 回退，或旧固件不再约束布局与偏移。当前单条需量快照不
  满足这些条件。

## 5. 测试钉住的边界

`targets/idis_ip_meter/tests/unit/test_idis_demand.c` 固定以下边界：legacy
空白尾接受并迁移、新记录偏移与 CRC、CRC 损坏拒绝、未知 version 拒绝、脏尾
拒绝（version 门禁与 CRC 门禁各一例）、降级再升级拒绝。

## 6. HIL 待验

- 真 EEPROM 上的撕写与掉电：目前只有 host 单元测试，没有硬件证据。
- 真旧固件的降级再升级：用只写 268 B 前缀的旧固件复现，确认加载 fail-closed。
- 需量清零一次后，下一次 300 s 周期保存或断电保存能写回完整记录。
