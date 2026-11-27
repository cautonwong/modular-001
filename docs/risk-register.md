# P0/P1 验收风险登记

日期：2026-11-27
矩阵：`docs/firmware-acceptance-matrix.md`
门禁：`tools/check_risk_register.py`，CI 标签 `check:risk-register`

## 1. 用途

矩阵里每条 P0/P1 的 `X` 行在这里有一条记录。记录回答四个问题：谁负责、
为什么还是 `X`、现在决定怎么处理、什么时候必须重新决定。`A` 行不登记。
矩阵新增 P0/P1 `X` 行而这里没有记录时 CI 失败；矩阵行转为 `A` 后登记残留
也失败。

## 2. 字段

- `ID` 与矩阵行一一对应。
- `优先级` 复制矩阵该行的优先级，只收 P0/P1。
- `Owner` 是负责关闭该行的角色。
  - `firmware`：产品固件路径，包括 domain、application 和 target 代码。
  - `firmware_sim`：整机孪生的观测面或激励模型，见矩阵第 7 节。
  - `hil`：真机或 HIL，见 `docs/ROADMAP.md` 第 6 节。
  - `product`：产品范围或配置，见矩阵阻塞列的产品配置或排除说明。
- `原因` 复制矩阵阻塞列的原文，不在这里重新解释。
- `决定` 取 `fix`、`accept` 或 `defer`。`accept` 必须写批准人。仓库没有
  这些行的产品批准记录，所以本表没有 `accept` 行；产品要接受风险时先补
  批准人再改决定。
- `到期日` 是下次审查日。登记日 2026-11-27 起，P0 取 30 天，P1 取 60 天，
  对应 `docs/ROADMAP.md` 第 4 节的 P0 近期和第 5 节的 P1 中期。ROADMAP
  只有分档，没有日历日期；这两个日期是登记时定的审查点。`fix` 或 `defer`
  到期未处理时 `check:risk-register` 失败。
- `批准人` 只对 `accept` 有意义，其余写 `-`。
- `证据` 是仓库相对路径，指向该行排除理由的出处。

## 3. 登记表

| ID | 优先级 | Owner | 原因 | 决定 | 到期日 | 批准人 | 证据 |
|---|---|---|---|---|---|---|---|
| ACC-WDG-001 | P1 | hil | 看门狗属于板级/SoC 驱动（`driver_entry.c`、`infra/boot`），孪生以桩替代，由 HIL 验证 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-TMR-001 | P1 | firmware_sim | 定时器服务属于基础设施，业务动作由负荷控制/显示用例观察 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RTC-005 | P1 | firmware_sim | 时间设置路径已可用，但 `gs_freeze.c:340` 的回退序列尚未有外部 flash 断言；需要构造跨日冻结后回拨的组合 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-MET-006 | P0 | firmware_sim | 电压/电流/功率已由 ACC-MET-006 覆盖，但功率因数与频率恒为 0，孪生没有对应模型；整行判据未满足 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-MET-008 | P1 | firmware | 保存阈值半边已覆盖；月结只写 10 字节头，结算电能/需量载荷没有持久化或解码路径 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-MET-010 | P1 | firmware | 在 0xFFFFFFF0 预置后注入 32 脉冲，EEPROM 能量仍为 0xFFFFFFF0，脉冲被 32 位上限保护丢弃；总加/分费率同时进位不成立 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-TAR-004 | P1 | product | 产品路径未进固件：`targets/stategrid_meter/app/tariff/component.yaml:6` 排除 legacy `gs_step_bill.c`，也没有 domain 阶梯模型 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-TAR-008 | P1 | product | 阶梯结算/切换冻结依赖被排除的 `gs_step_bill.c`，固件没有产品路径 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-TAR-005 | P0 | product | 结算电能未实现：同一 component.yaml:6 排除 `gs_month_bill.c`；结算冻结记录只有 10 字节头，不持久化电能；需量无产品路径。冻结半边已由 ACC-FRZ-005 覆盖 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-FRZ-009 | P0 | firmware | `set_5_Frozen.c` 只处理属性 0x300 的组合写入（capture/period/depth），没有 period-only 写入路径或可验证的周期效果 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-FRZ-010 | P1 | firmware_sim | 存储选择由 `gs_freeze.c:51` 的 `cal_record_size_sectors` 按 MinFrez_Sectors 计算；两方案分钟探针只留下一个可读记录，管理器到区域的映射没有外部观测 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-EVT-010 | P1 | firmware_sim | 主动上报从 PLC 通道发出（`fr_report.c:900` / `fr_report_frames.c:181`），孪生只捕获 RS485-1；没有 PLC 连接模型或 TX 捕获 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-EVT-013 | P1 | firmware | 698 编程列表插入要求已连接状态（`mapper_698.c:2937`），CONNECT 依赖 ESAM；645 列表只在认证超时路径插入，普通写 04000402/04000406 返回 D4 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-EVT-017 | P1 | firmware | 供电异常只由 AC 掉电 GPIO 路径的 PowerAbnormalS_status 置位（`driver_entry.c:1079-1125`）；`gs_power_abnormal_event.c` 没有电压/缺相逻辑，孪生没有电参模型 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-LC-003 | P0 | product | 产品配置未启用预付费：没有预付费跳闸/允许的业务路径，只有 698 映射字段（与 ACC-LC-009 同一产品缺口） | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-LC-009 | P1 | product | 产品未启用预付费，仅保留映射字段，无业务路径 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-698-004 | P0 | firmware | 主动上报走 PLC 通道，孪生只捕获 RS485-1；3320 列表项 Report3320_OADList1..4 在 `mapper_698.c:560-568` 为 NULL 初始化，空白镜像 GET 0x33200200 返回 DAR 255 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-698-005 | P0 | firmware_sim | ESAM 安全会话不可用：`tests/sim/sim_stubs.c:169` 的 `openM_ioctl` 返回 -1，安全模式/密钥路径无法完成；孪生没有 ESAM 响应模型 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-698-007 | P0 | firmware | 连接状态未实现：`application/adapters/dl698/fr_698_connect.c:143-181` 的 NullSecurity CONNECT 写入结果 255 并返回 `DL698_SEC_OTHERS_ERR`，没有已连接态可超时；PasswordSecurity 比较未填充的 APDU 缓存，对称/签名路径依赖 ESAM；也没有释放通知或链路空闲超时缝隙 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-SEC-002 | P1 | firmware_sim | ESAM 不建模：`tests/sim/sim_stubs.c:169` 的 `openM_ioctl` 返回 -1，MAC/密文合法路径在 `fr_698_gatekeeper.c:196-279` 以 `90 02 ff 00` 结束，只能验证拒绝分支 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-SEC-005 | P1 | firmware_sim | 红外通道不在孪生观测范围：RS485-1 之外的 TX 未捕获，IR RX 注入触发 SIGSEGV | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-SEC-006 | P0 | firmware_sim | 安全模式转换的密文+MAC 支路依赖 ESAM；`openM_ioctl` 桩返回 -1，只有明文/拒绝支路可观测 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-001 | P1 | firmware_sim | 上报策略 SET 可写，但“选中的通道/对象”需要实际上报帧；PLC TX 未捕获 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-002 | P1 | firmware | 3320 列表在 `mapper_698.c:560-568` 为 NULL 初始化，校验失败，GET 0x33200200 返回 DAR 255；没有 3320 SET 处理器 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-003 | P1 | firmware_sim | 重试需要定时器推进（孪生无 `ndv_timer_tick`）和 PLC/IR TX 捕获，两者都没有 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-004 | P1 | firmware_sim | 上报编码需要实际上报帧；孪生只捕获 RS485-1 TX，PLC/IR 未捕获 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-005 | P1 | firmware | 3320 支路因列表 NULL 初始化不可用；2015 跟随块需要 `nd_mb_report_2015_state[channel]` 和上报模式，开盖/关盖探针未产生跟随块 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-006 | P1 | firmware_sim | 没有 PLC 连接/超时模型，也没有 PLC TX 捕获，通信中断/恢复不可观测 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-RPT-007 | P1 | firmware_sim | 去重/顺序需要重试时钟和可见上报帧；定时器与 PLC/IR TX 捕获都缺 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
| ACC-STO-002 | P0 | firmware_sim | 没有在 `mapper_698.c` 的 `auto_parameter_inital` 附近注入参数损坏的缝隙，工厂模式重初始化不可外部观测 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-STO-003 | P0 | firmware_sim | 事件区轮转需要填满 `record_item[]` 管理区；孪生没有 2 秒粒度的批量事件填充激励 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-STO-005 | P0 | firmware_sim | 没有在记录写入与管理器更新之间切断电源的缝隙，掉电一致性不可注入 | defer | 2026-12-27 | - | docs/firmware-acceptance-matrix.md |
| ACC-STO-006 | P1 | firmware_sim | 没有向存储 pipeline 投递记录消息的缝隙，消费者存在但无法触发 | defer | 2027-01-26 | - | docs/firmware-acceptance-matrix.md |
