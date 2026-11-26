# 放置与语义端口设计（D0）

日期：2026-11-25。基线：`f0f0ae08`。本文只写设计；W2 起按本文执行，评审通过前不动固件。

## 1. 问题与前提

剩余审计项（C2.2、C2.4-C2.9、C6.1 剩余、C6.2、C6.3）不是文件放错，而是组件图
没有表达四类关注点的所有权：wire（协议编码/对象号）、UI（页面/按键/LED）、
storage（块/扇区/布局）、customer（客户定制常量）。文件按“当前消费者”被放
在 domain，后续每出现一个消费者就再搬一次；每次搬迁都要改 PATH_FINDINGS、
基线条目、product.yaml、IAR 源表和测试，这正是补丁模式。

证据（已完成的单元）：

| 单元 | 提交 | 搬迁内容 | 每次都要动的东西 | 基线变化 |
| --- | --- | --- | --- | --- |
| P1a | `0d05d65d` | display/backlight 出 domain | PATH_FINDINGS C3.1、purity/public-structs 基线、manifest、.ewp、tests | purity 66→56，structs 117→114 |
| P1b | `9e96b789` | 按键/LED 出 domain | PATH_FINDINGS C3.2、基线条目、context 状态、tests | purity 56→49 |
| P1c（非搬迁参照） | `7a2be6fc` | 两通道校验重复，改成 domain validator + 映射 | 两 mapper、宏、checker | 无 purity 变化（说明“不搬文件也要改多处的校验重复”） |
| P2a1 | `f0f0ae08` | report/retry 出 domain | C2.1/C2.3 路径规则删除、public-structs/definitions、manifest、.ewp | purity 49→26，definitions 203→193 |

共同点：每轮都靠“删一条 per-finding path 规则 + 缩一次基线”收尾，而路径规则
本身只是把当前文件标成“待搬走”。要停止补丁，需要先固定所有权与接口，再让
wave 按所有权执行；守卫只检查所有权不变式，不再为单个 finding 加规则。

## 2. 所有权表

层级规则以 `tools/check_components.py` 的 `FORBIDDEN` 为准（见第 3 节）。
“端口”一律消费者定义、`void *ctx` 不透明、组合根绑定一次。

| 项 | 当前文件 | 目标层/组件 | 接口或端口 | wave |
| --- | --- | --- | --- | --- |
| C2.2 权限策略 | `domain/security/security_mode.{c,h}` | 语义判定留 `domain/security`；OI/OAD/word 表进 `application/adapters/dl698` | domain 新 `security_policy`：按操作类（GET/SET/ACTION）× 安全等级判定；dl698 把 OI/OAD 映射成操作类 | W2 |
| C2.2 请求分类 | `domain/security/security_request.{c,h}` | 语义请求结构与判定留 `domain/security`（去掉 oad 与 wire choice）；OI/OAD/wire choice 映射进 `application/adapters/dl698` | domain `nd_security_request_t`：security/verification choice 的语义值、auth timeout、plain_read_allowed、report/get 判定；dl698 把 OI/OAD 和 wire choice 映射进该结构 | W2 |
| C2.2 ESAM 结果 | `domain/security/esam_result.{c,h}` | `components/services/security`（*） | `nd_esam_result_class_t` 语义分类；adapter 消费 | W2 |
| C2.4 时间编码 | `domain/common/time_format.{c,h}` | `components/services/time_format` | 共享 wire 时间编解码；消费者同时有 645 mapper（ctl_14_check.c、ctl_14_write.c、ctl_other.c）与 698 mapper（fr_axdr.c、set_4_Parameter.c、action_4_Parameter.c、action_f_Other.c），另有 `tariff/legacy/touFee.c` 与 host 测试。application 不能 include protocol，645/698 mapper 也不能互相依赖，services 是唯一被 application 与 product 同时允许的共享层 | W3 |
| C2.5 push 设置 | `domain/push/push.{c,h}` | `components/services/push` | 设置/目标结构；消费者只有 IDIS 两个 target（`app/main.c`、cosem `nd_ic_push.c`）与 host 测试，没有 application/stategrid 消费者 | W3 |
| C2.6 记录捕获/事件 OAD | `domain/events/record_capture.{c,h}`、`power_event.{c,h}`、`program_list.{c,h}`、`event_catalog.{c,h}` | 事件 identity/kind/去抖留 `domain/events`；OAD 映射、可见性规则、`0xAA` 后缀进 `application/adapters/dl698` | domain 保留 kind/identity；dl698 提供 `record_capture_view`、`event_oad_view`（含 power-off 后缀判定） | W3 |
| C2.7 冻结容量 | `domain/freeze/storage.{c,h}` | `infra/storage/freeze_layout` | `nd_freeze_scheme_units` 的扇区/字节数学；唯一调用者是 target `gs_freeze.c`，product 可直接 include infra；domain 不定义容量端口、组合根不绑定（将来 domain 真需要容量时再按消费者定义端口） | W3 |
| C2.8 M-Bus | `domain/mbus/mbus_client.{c,h}` | `components/services/mbus` | 地址分配与身份归 adapter/服务；IDIS cosem 消费 | W3 |
| C2.9 bus 主题 | `components/core/bus/nd_bus.{c,h}` | 保留协议中立事件；删除无订阅者的帧主题 | 全仓唯一订阅者是 `tests/unit/test_nd_bus.c`，生产代码没有订阅者；DL698/DL645 frame 主题和 frame payload 删除，stategrid entry 的 frame publish 同步删除；等出现真实订阅者时再评审 adapter 注册 API | W4 |
| C6.1 客户常量（剩余） | `domain/events/power_event.*` 的 `0xAA` 后缀；`domain/common/status.h` 的 `0xAA`/`0x55` 客户事件码 | 后缀规则进 `application/adapters/dl698`；`status.h` 只留语义值，客户码进 target wire 头 | domain 只留 `ND_STATUS_SET/RESET`、`ND_POWER_ON/OFF` 语义值（1/0）；`0xAA`/`0x55` 移到 target `meter_wire_status.h`；target 端口 read 把 wire 映射成语义、write 反向映射；target 直接比较控制块用 wire 头，application 只比较语义；dl698 `event_oad_view` 判后缀；target `customer_event_map(wire_code)` → `nd_event_kind_t` | W3（后缀）/W5（客户码） |
| C6.2 清单可达性 | IDIS 清单含无 caller 的 domain 组件 | 按 product 闭包删选未用组件；comm_plc 保留 dl698 | 无（清单 + 新守卫） | W2 起逐 wave |
| C6.3 meter_base_view | `application/adapters/dl698/meter_base_view.h` | 头移到 `targets/stategrid_meter/persistence`（该组件 paths 拥有）；`application/ports/meter_base.h` 定义 typed vtable | 34 个 `nd_mb_*` 符号各一个函数指针，真实签名（数组显式 channel/client，字节缓冲返回指针，save 无参，status/power 返回语义）；无 field enum/switch；const 放 flash；target 薄包装，组合根绑定一次 | W5 |

（*）`esam_result` 若放 `infra/security` 会违反第 3 节：application 禁止 include
infra。按“规则以 FORBIDDEN 为准”，W2 把它放 `components/services/security`
（application 允许 include services）。若要回到 `infra/security`，须先单独评审
修改 FORBIDDEN，不能在搬迁里顺手放宽。

## 3. 层级规则（check:components 的 FORBIDDEN）

| 层 | 禁止 include |
| --- | --- |
| core | domain, application, protocol, services, infra, platform, legacy, product |
| domain | application, protocol, services, infra, platform, legacy, product |
| application | protocol, infra, platform, legacy, product |
| protocol | application, infra, platform, legacy, product |
| services | protocol, application, infra, platform, legacy, product |
| infra | product |
| platform | product |
| product | 无 |

推论（写进文档避免再猜）：
- wire 协议编码只能进 application/adapters（application 不能 include protocol，
  protocol 也不能被 application 看见）；domain 只能放协议中立语义。
- application 不得 include infra，所以“驱动结果类型”若要被 application 使用，
  必须放 components/services 或 domain，不能放 infra。
- domain 不得 include services，所以 services 模块只能被 application/product 消费。
- product 全开，target 可以直接 include application/adapters 与 infra/storage。

## 4. 语义端口

- 安全权限按“操作类 × 安全等级”判定（GET/SET/ACTION × plain/cipher/…），不按
  OI/OAD；OI/OAD 到操作类的映射表是 dl698 的 wire 细节。
- 报表决策按语义事件类（起报/止报/2015 类）而不是 OAD；OAD 选择留给 adapter。
- 冻结容量数学（扇区/字节、flash/EEPROM 差异）放 `infra/storage/
  freeze_layout`，target `gs_freeze` 直接调用；domain/freeze 只做时间与回补
  状态机。domain 没有容量消费者，因此不定义容量端口，也不在组合根绑定；将来
  domain 真需要容量时再按消费者定义端口。
- 安全请求语义（security/verification choice 的语义值、auth timeout、
  plain_read_allowed、report/get 判定）留 `domain/security`；`domain/security/
  security_request` 去掉 oad 与 wire choice 字段，dl698 负责 OI/OAD 与 wire
  choice 到语义结构的映射。
- 时间编解码是 645 与 698 共享的 wire codec，放 `components/services/time_format`；
  services 可被 application 与 product 同时 include，避免 645 mapper 依赖 698
  adapter（application 也不能 include protocol）。
- push 与 M-Bus 的消费者是 IDIS target 与 host 测试；模块放
  `components/services/push` 与 `components/services/mbus`，IDIS 清单从 domain
  换成 services 组件。
- M-Bus 地址分配与身份编码归 adapter/service；domain 只保留“读值/时间”语义。
- bus 只带协议中立事件（秒/分、电源、按键、告警）。DL645/DL698 frame 主题和
  frame payload 当前没有生产订阅者：全仓唯一的 `nd_bus_subscribe` 调用在
  `tests/unit/test_nd_bus.c`，stategrid entry 的 frame publish 返回值也无人使用。
  无消费者的主题直接删除；等出现真实订阅者时再引入 adapter 注册 API，并重新
  评审，core 不出现协议名。
- 事件 identity（kind/去抖状态）留 domain；可见性与 OAD 后缀规则归 dl698。
- `meter_base_view` 的取值端口由 application 定义在
  `application/ports/meter_base.h`：typed vtable，按 `surface.txt` 的 34 个
  `nd_mb_*` 符号每个一个函数指针，签名就是该访问形状的真实签名（标量/数组返回
  存储指针，显式 channel/client 参数；tsa/report_random_data 返回字节指针；
  save actions 无返回值无参数；status/power 返回语义值），不做 field id 解释器，
  没有枚举和 switch。vtable 为 `const`，放 flash。原 `meter_base_view.h` 移到
  target（persistence 的 paths 拥有，放 persistence），46 个 target include、
  target 测试和 IAR 源表同步；target 为每个指针提供薄包装（能直接指向现有
  accessor 的直接指向，status/power 两个做 wire->语义映射），组合根绑定一次；
  application 只 include `application/ports/meter_base.h`，不 include target，
  也不再知道存储布局。out-of-line 的 `nd_mb_*` wrapper 归 application（每个 entry
  一个真实函数，拿 instance、取端口、调 entry，签名不变）；target 只实现 entry
  和 vtable，不暴露 wrapper。residual：端口隐藏了 target 布局，但 accessor 仍是
  产品字段语义，不是 domain value object；C6.3 的层次边界已修，语义 value object
  /snapshot 留给后续，不阻塞本 wave。

## 5. Wave 表

| wave | 文件 | 测试 | 验收 lane | ELF 预算 | 删除的 path 规则 / 基线 |
| --- | --- | --- | --- | --- | --- |
| W1（完成） | report_policy/retry | test_report_* | firmware_acceptance | +8 B | C2.1/C2.3 path 已删 |
| W2 security | security_mode 拆分（语义策略留 domain、OI/OAD 表进 dl698）、security_request 语义化（去 oad/wire choice）、esam_result | test_security_*、ACC-SEC-*（含 blocked 探针不翻） | firmware_acceptance；IDIS 16/16 | stategrid ≤ +256 B；IDIS ROM 0（仅清单变化） | 删 C2.2 path（security_mode/esam_result）与对应 oad token 基线；IDIS 清单去 domain.security |
| W3 语义模块 | time_format、push、record_capture/power_event/program_list/event_catalog 的 OAD 与可见性拆分、freeze 容量数学 + infra/storage/freeze_layout、M-Bus、`0xAA` 后缀 | 各模块单测（含 freeze 布局数学（flash/EEPROM）、四个事件文件的 OAD 映射）+ ACC-TAR/ACC-EVT/ACC-MB 相关；IDIS cosem 用例 | firmware_acceptance；IDIS 16/16 | stategrid ≤ +256 B；IDIS ROM/BSS 0（纯移动） | 删 C2.4/C2.5/C2.6/C2.7/C2.8 path 与 C6.1 后缀条（time_format 进 services/time_format，push/mbus 进 services，IDIS 清单换组件；freeze 容量进 infra/storage/freeze_layout，无 domain 端口）；基线删除覆盖 record_capture/power_event/program_list/event_catalog 的 oad token；IDIS 清单把 domain.push/mbus 换成 services 组件；W3 结束启用 check:product-callers |
| W4 bus | 删除无订阅者的 DL698/DL645 frame 主题与 frame payload、stategrid entry 的 frame publish；保留 TICK_SECOND/TICK_MINUTE/POWER_CHANGE/KEY/ALARM | 更新 test_nd_bus；新增断言 `nd_bus.h` 不含 DL645/DL698/FRAME 名字 | firmware_acceptance | stategrid 预期不变或变小；IDIS 不变（不选 core.bus） | 无 domain-purity 项；C2.9 随无订阅者主题删除而关闭 |
| W5 meter_base_view + 客户码 | meter_base_view 移 target、`application/ports/meter_base.h` typed vtable、target 薄包装、`status.h` 语义化 + target wire 头 | test_meter_base_view（34 个 vtable 项的读写/索引/字节/save 映射）、status wire<->semantic 正反映射、ACC-MET/ACC-EVT/ACC-LC 回归 | firmware_acceptance | stategrid ≤ +1,120 B；最终实测 +1,104（vtable 144、target 实现 60、application wrappers 982、调用点 -78；修复 unused-local 前为 +1,120）；IDIS 四个镜像 ROM/RAM 逐项不变 | 删 C6.1 剩余客户常量条目；C6.3 由端口替代；residual：端口隐藏 target 布局，但 accessor 仍是产品字段语义，不是 domain value object；C6.3 层次边界已修，value-object/snapshot 留给后续，不阻塞本 wave |

每个 wave 结束时的 product 清单只保留“product 闭包里有 caller”的组件；W2 删
IDIS 的 domain.security，W3 再把 domain.push/domain.mbus 换成 services 组件，
IDIS 因此不再携带 698-only 代码（C6.2 的 IDIS 半边）。comm_plc 保留 dl698，
闭包里 fr_report/fr_698 系列就是这些组件的 caller，不能靠全局 credit 证明。

## 6. 守卫模型

- 结构规则：`check:layers`、`check:components`（FORBIDDEN + manifest
  dependencies）保持唯一结构门禁；不加 per-finding 例外。
- 新守卫 `check:product-callers`（`tools/check_product_callers.py`）：只对固件产品
  （由 target 的 CMake `PRODUCT` 或 IAR 项目派生）建立产品闭包（`product.yaml`
  解析出的组件 `.c` + 该产品 target 目录下的 `.c`，扣除 tests 和构建目录）。非
  product 层组件的公共函数必须有同一 product 闭包内的 caller（include 可见的
  声明）；组件内部自调用、全局其它 product 的 caller、测试/host caller 都不
  credit。基线为空；任何无 caller 的选中组件直接失败。非固件产品（comm_plc、
  comm_rf、meter_gas、meter_water）是 source assembly，没有 target 源也没有
  链接，caller 本来就不在闭包里，这是职责边界而不是例外；它们由
  `check:product-component-usage` 证明源可解析，不由本检查判定。自检：同 product
  caller 通过、跨 product caller 不 credit、测试/host caller 不 credit、只有组件
  内部自调用必须失败。旧 `check_component_reachability.py`、其
  `reachability_baseline.txt` 和 ci_matrix lane 由本检查替代并删除，不保留两套
  重叠守卫。
- 启用时序：W2 结束时 IDIS 仍选 domain.push/domain.mbus（W3 才换成 services），
  空基线 checker 会误报；因此 W2 只用当时的 reachability 检查并删 IDIS 的
  domain.security。W3 完成 push/mbus 清单切换后，W3c 启用
  `check:product-callers` 并删除旧的全局 reachability 检查。
- `check:domain` 的 token 规则保留（防止新代码写回 OAD/report/backlight 等词）；
  path 规则随 wave 删除，且不新增 per-finding path 规则；每个 wave 用
  `--update` 缩基线，最终基线为空，或只留带 reason、到期日、批准人的语义例外
  （沿用 mutable_globals_exceptions 的格式，由新例外文件承载）。
- `domain_definitions_allowlist.txt` 是消费者实现端口的清单，不是债务；wave
  新增端口时更新理由，不设到期日。
- 其它既有守卫：`check:public-structs`、`check:dead-code`、`check:mutable-globals`
  随搬迁 `--update` 缩基线；`check:product-link` 继续证明清单源与链接输入一致；
  `check:tariff-limits` 是“限制只能定义在 domain”的示例，后续客户常量沿用该
  模式（定义点唯一）。

## 7. 明确禁止

- wire 类型（OAD/OI/帧结构/BCD 字节布局）回到 domain。
- application include protocol 或 infra。
- 为单个 finding 再加一条 domain-purity path 规则或 allowlist 条目。
- product 选中的组件在该 product 闭包里没有 caller（靠基线或全局 credit 保留）。
- 用放宽 FORBIDDEN 的方式解决放置冲突；先改规则表必须单独评审。

## 8. 每个 wave 的完成定义

1. 行为/验收证据：host 全量、相关 mapper 单测、对应验收 lane 全绿；行为变化
   必须在设计记录里列明规则、通道、DAR/错误映射与测试名。
2. ELF 账：stategrid `.text`/`.data`/`.bss` 实测 delta；IDIS 若未选受影响代码，
   ROM/BSS 与上一基线逐项相同；超预算即停。
3. 基线缩减：该 wave 对应的 domain-purity path 规则与基线条目删除；public-
   structs/dead-code/mutable 按实际 `--update`；无新增例外。
4. 新增守卫（如 `check:product-callers`）先有 self-check 负例和正例，再进
   `ci_matrix.py`。
5. 清单：product.yaml、IAR .ewp、tests/CMakeLists、component.yaml 与源码同
   commit 更新；`firmware_sim_manifest.py` 输出无死路径。
6. 组件收尾：wave 结束时删除原组件的 component.yaml 与空目录，不留空组件；
   tools 里的分析元数据同步改到新路径，grep 全仓（排除历史设计记录）无旧路径。

最终状态：`tools/domain_purity_baseline.txt` 为空（或只剩带 reason/到期日/
批准人的语义例外），`check:product-callers` 基线为空，所有固件 product 闭包自洽，
domain 只保留协议中立的业务语义。
