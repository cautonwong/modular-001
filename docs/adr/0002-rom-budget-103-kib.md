# ADR-0002: IDIS 固件 ROM 预算 103 KiB

日期：2026-11-27

## Status

Accepted。决定已落地，由链接断言、ELF checker 和文档一致性 checker 守护。

## Context

IDIS 有两个固件 target，裸机 `targets/idis_ip_meter` 和 FreeRTOS
`targets/idis_ip_meter_freertos`。每个 target 有 POLL 和 IRQ 两条 lane，共四个
镜像。ROM 上限是 103 KiB，即 105,472 字节（AGENTS.md 项目约束；
`docs/idis-object-registry.md` 的预算表）。`tools/check_idis_firmware.py` 的
`ROM_BYTES = 103 * 1024` 是这个数字的唯一来源。链接断言和 checker 允许镜像
正好等于上限，超过才失败。

103 KiB 是自限预算，不是硬件限制。`docs/ROADMAP.md` §7 明确记录了这个前提，
并给出 100 KiB 约束出现时的处置方向，先砍 IRQ 传输。这个预算的作用是给每个
固件功能一个可核算的上限。

ROM 的计量口径也必须固定，否则预算不可比较。`tools/check_idis_firmware.py`
取 `.isr_vector`、`.text`、`.ARM.exidx` 和 `.data` 的 LMA 跨度，含 section 间
对齐填充，等于链接产物 `.bin` 的大小，不是 section 大小之和
（`docs/guides/verification.md`）。当前四个镜像的 load span 是裸机 POLL
95,356、裸机 IRQ 97,956、FreeRTOS POLL 99,632、FreeRTOS IRQ 102,444 字节，
最紧的 FreeRTOS IRQ lane 余 3,028 字节（`docs/ROADMAP.md` §2；本次分支
`./ci.sh` 输出）。

## Options

1. 采用 100 KiB 上限。现在不选。FreeRTOS IRQ lane 只剩 3,028 B 余量，要先
   砍 IRQ 传输才能进 100 KiB；`docs/ROADMAP.md` §7 记录了这个分叉，并把它留
   到真实约束出现时。
2. 采用 103 KiB 上限，作为自限预算。选中。它覆盖四个镜像，并保留 3,028 B
   的最小余量（`docs/ROADMAP.md` §2）。
3. 只在发布前量一次，不设固定上限。否决，因为 AGENTS.md 要求任何固件功能先
   做 ROM 账，单次测量无法阻止单个功能悄悄吃掉余量；W5 就是按 1,120 B 上限
   核算，实测 1,104 B（`docs/design/placement-seam.md` W5 行）。
4. 预算数字分散写入各文档，靠评审保持一致。否决，因为数字会漂移。选中方案
   让 `tools/check_idis_firmware.py` 做唯一来源，`tools/check_rom_budget_docs.py`
   检查链接脚本、AGENTS.md、review 规则和文档都引用同一个数，并拒绝残留的
   102 KiB。

## Decision

- IDIS 两个 target 的 ROM 上限是 103 KiB（105,472 B）。四个镜像都不得超过
  这个值。
- 上限在链接期强制。两个 `build/idis.ld` 都有
  `ASSERT(_sidata + SIZEOF(.data) <= ORIGIN(ROM) + 103K, "ROM budget exceeded:
  product limit is 103 KiB")`。超出时链接失败，不生成镜像。
- 上限在构建后复核。`tools/check_idis_firmware.py` 计算 ROM load span，与
  `ROM_BYTES` 比较，并在 CI 打印每个 target 的 ELF section 实测值。
- 上限只有一个来源。`ROM_BYTES` 在 `tools/check_idis_firmware.py`；
  `tools/check_rom_budget_docs.py` 要求两个链接脚本、AGENTS.md、
  `.opencodereview/rule.json`、`docs/guides/product-platform-extension.md` 和
  `docs/idis-object-registry.md` 引用同一个 `<K> KiB`，并在 K 不是 102 时
  拒绝 `102 KiB` 或 `102K` 字样。
- 每个固件功能先做 ROM 账。功能评审给出相对当前基线的增量上限，实测超出即
  不接受（AGENTS.md 项目约束；W5 的 1,120 B 上限是实例）。
- 103 KiB 按自限预算陈述。文档不得把它写成硬件限制；100 KiB 约束出现时，
  先砍 IRQ 传输，再改预算数字和所有执行者（`docs/ROADMAP.md` §7）。

## Consequences

- 新固件功能以最紧的 FreeRTOS IRQ lane 为准。当前余量 3,028 B，先看 IRQ
  传输（`docs/ROADMAP.md` §4）。
- 超预算在链接期失败，不会进入验收。`check:rom-budget-docs` 还会拦住只改
  checker 不改文档，或只改文档不改链接脚本的半套修改。
- ROM 数字有固定口径。CI 的 `rom=` 是 load span，不是 section 之和；跨工具链
  比较时也必须用同一口径（`docs/guides/verification.md`）。
- 预算不覆盖 RAM。RAM 另有 64 KiB 上限，由同一个 checker 的 `RAM_BYTES` 和
  `idis.ld` 的 RAM 断言约束（`tools/check_idis_firmware.py`；
  `docs/idis-object-registry.md`）。
- 预算会限制功能选择。需要大块表或新协议栈时，先证明放得进最紧 lane，否则
  先做裁剪或换 lane 策略（`docs/ROADMAP.md` §4 第 2 条）。

## Evidence

- `tools/check_idis_firmware.py`：`ROM_BYTES = 103 * 1024`，`RAM_BYTES = 64 * 1024`，
  ROM load span 计算和 section 输出。
- `targets/idis_ip_meter/build/idis.ld` 和
  `targets/idis_ip_meter_freertos/build/idis.ld`：`ORIGIN(ROM) + 103K` 断言和
  `product limit is 103 KiB` 消息。
- `tools/check_rom_budget_docs.py`：唯一来源、文件清单、102 KiB 残留检测和
  `--self-check`。
- `AGENTS.md` 规则表、完成定义和项目约束中的 103 KiB 条目。
- `docs/ROADMAP.md` §2、§4、§7：当前镜像数字、最紧余量、自限预算说明和
  100 KiB 分叉。
- `docs/guides/verification.md`：链接断言与 checker 共同守住预算，`rom=` 口径。
- `docs/idis-object-registry.md`：预算以字节表示的同一数字，checker 口径。
- 本次分支 `./ci.sh` 的 `check:rom-budget-docs` 和两个 IDIS target 的
  `check_idis_firmware.py` 输出。

## Reversal conditions

- 真实的硬件或客户约束是 100 KiB。先砍 IRQ 传输，再同时修改
  `tools/check_idis_firmware.py` 的 `ROM_BYTES`、两个链接断言、AGENTS.md、
  review 规则和相关文档；`check:rom-budget-docs` 会在任一处不同步时失败。
- 某个必需功能在 103 KiB 内无法落地，且 IRQ 传输已经砍到极限。那时显式重开
  预算决定，给出新的上限、实测余量和取舍，不静默放宽断言。
- 产品换到 ROM 更大的 MCU，或 103 KiB 约束不再适用于该产品。可以退役这个
  上限，但要用一份新 ADR 取代本记录，并保留当时的实测数字作为历史。
