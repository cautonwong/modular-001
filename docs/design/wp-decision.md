# Frama-C WP Phase 0 决策：drop

日期：2026-11-23
基线：`a8b8b364`

## 1. 问题与判据

计划 §8 用两个玩具契约判断 WP 能否进门禁：

- 假后置条件（无内存、无量词）必须由至少一个 pin 住的 solver 在 5 s 内判
  `invalid`。
- 真后置条件必须没有 Timeout/Unknown。

两条都满足才 accept；否则记录证据并 drop，不保留假门禁。玩具契约在
`formal/frama-c/wp/`，判定脚本是 `tools/check_wp.py`。

## 2. 结论

**Drop。** 假目标在全部 pin 住的 solver 上都不是 `invalid`：z3 4.8.12 和
Alt-Ergo 2.6.5 是 Timeout，Alt-Ergo 2.0.0、CVC5 1.2.1、CVC5 1.3.1 是
Unknown。真目标在五个版本上都是 valid，说明 prover 接线本身正常，卡的是
假目标所需的 sat 判定。WP 不进门禁，CI 的 Frama-C 门禁仍只有 Eva。

## 3. 版本与 pin

| 工具 | 版本 | 来源 | 备注 |
| --- | --- | --- | --- |
| Frama-C | 33.0 (Arsenic) | 官方 bundle | why3 运行时 1.8.2，不含 prover |
| z3 | 4.8.12 | 系统 `/usr/bin/z3` | |
| Alt-Ergo | 2.0.0 | 系统 `/usr/bin/alt-ergo` | |
| Alt-Ergo | 2.6.5 | 官方 static release | sha256 `3bc59972fedd570da07c0a3b9a9e778f7c20050edecb1a1ce3290d63f64a20d3` |
| CVC5 | 1.2.1 | 官方 static release | sha256 `6d44abc233980a14d72cc5809287d27c3335b1d6ee863381d0b5ffcbd0d8de56` |
| CVC5 | 1.3.1 | 官方 static release | sha256 `1a1cda20d2df4938fa4944a69f33ddc9172e319ece0eed0aa09c4d7abede3ed1` |

Alt-Ergo 2.6.5 的 static binary 用提交号回答 `--version`（`997f541`），why3
的检测正则只接受数字版本。用一个只回答 `--version` 为 `2.6.5`、其余参数原样
转给同一 binary 的 shim，让 why3 选中 `alt_ergo_26` driver；solver 本身仍是
上表 sha256 pin 的 release。CVC5 1.3.1 不在 why3 1.8.2 的 `version_ok` 列表
（只列 1.0–1.2），但该 driver 仍能调用；1.2.1 是声明支持范围内的版本。

## 4. 命令

```sh
# 判定逻辑自检（合成报告，覆盖 valid/invalid/timeout/unknown/缺目标）
python3 tools/check_wp.py --self-check

# 系统 z3 4.8.12 和 Alt-Ergo 2.0.0
FRAMA_C=<frama-c-bundle>/bin/frama-c python3 tools/check_wp.py --solver z3
FRAMA_C=<frama-c-bundle>/bin/frama-c python3 tools/check_wp.py --solver alt-ergo

# Alt-Ergo 2.6.5 / CVC5：把对应 release 放到 PATH 最前
PATH=<alt-ergo-2.6.5>/bin:$PATH FRAMA_C=<frama-c-bundle>/bin/frama-c \
    python3 tools/check_wp.py --solver alt-ergo
PATH=<cvc5-release>/bin:$PATH FRAMA_C=<frama-c-bundle>/bin/frama-c \
    python3 tools/check_wp.py --solver cvc5
```

默认 `--timeout 5`。脚本对每个 solver 跑两个玩具，读 `-wp-report-json`，按
ACSL property 名和 verdict 判定；只有至少一个 solver 同时满足两个判据才退出
0。

## 5. per-goal JSON 计数

每次报告只有一个 goal；`passed=false` 表示 WP 没判 valid。`timeout=1` /
`unknown=1` 是 JSON 的布尔计数字段。下表是一次 pin 住的运行；provers 列是
`success/time`（秒），时间是当次测量，外部 prover 失败时 JSON 报告为 0。

| solver | toy | goal | verdict | timeout | unknown | provers |
| --- | --- | --- | --- | --- | --- | --- |
| z3 4.8.12 | toy_invalid | typed_toy_invalid_ensures | timeout | 1 | 0 | qed 0/0.0005; Z3:4.8.12 0/0 |
| z3 4.8.12 | toy_valid | typed_toy_valid_ensures | valid | 0 | 0 | qed 0/0.0024; Z3:4.8.12 1/0.0234 |
| Alt-Ergo 2.0.0 | toy_invalid | typed_toy_invalid_ensures | unknown | 0 | 1 | qed 0/0.0006; Alt-Ergo:2.0.0 0/0 |
| Alt-Ergo 2.0.0 | toy_valid | typed_toy_valid_ensures | valid | 0 | 0 | qed 0/0.0021; Alt-Ergo:2.0.0 1/0.0131 |
| Alt-Ergo 2.6.5 | toy_invalid | typed_toy_invalid_ensures | timeout | 1 | 0 | qed 0/0.0007; Alt-Ergo:2.6.5 0/0 |
| Alt-Ergo 2.6.5 | toy_valid | typed_toy_valid_ensures | valid | 0 | 0 | qed 0/0.0021; Alt-Ergo:2.6.5 1/0.026 |
| CVC5 1.2.1 | toy_invalid | typed_toy_invalid_ensures | unknown | 0 | 1 | qed 0/0.0010; CVC5:1.2.1 0/0 |
| CVC5 1.2.1 | toy_valid | typed_toy_valid_ensures | valid | 0 | 0 | qed 0/0.0039; CVC5:1.2.1 1/0.0238 |
| CVC5 1.3.1 | toy_invalid | typed_toy_invalid_ensures | unknown | 0 | 1 | qed 0/0.0020; CVC5:1.3.1 0/0 |
| CVC5 1.3.1 | toy_valid | typed_toy_valid_ensures | valid | 0 | 0 | qed 0/0.0011; CVC5:1.3.1 1/0.0236 |

## 6. 卡点

`toy_invalid` 的 WP 任务把假目标写成 `(assert (not false))`。生成的 SMT 共
150 行，前 145 行是量化算术 prelude（`abs`、`div1`、`mod1` 及带 pattern 的
非线性公理）。假目标
要求 prover 返回 sat，solver 必须为这些全称公理找模型：

- z3 4.8.12 在生成的 `.smt2` 上 30 s 内不返回 sat/unsat（内建 `-T:30` 输出
  timeout）；去掉 prelude 后同一条 `(assert (not false))` 立即返回 sat。
- Alt-Ergo 2.6.5 直接在 `.psmt2` 上 `--timelimit 5` 报 Timeout（4.88 s，
  28169 steps）。
- CVC5 1.2.1/1.3.1 在生成的 `.smt2` 上返回 unknown。

玩具本身确实是假的，卡点是 prelude 的量化算术。真目标由外部 prover 证明
（Qed 失败、prover 成功），接线是通的。

## 7. 影响与重开条件

- 不接 `tools/check_formal.sh`，不进 CI；CI 只跑 Eva。
- `tools/check_wp.py` 保留为可复跑证据。它当前以退出码 1 结束，因为没有任何
  solver 满足判据。
- 重开条件：solver 能在 5 s 内对上述 prelude 返回 sat，或 WP/why3 能只输出
  用到的算术理论；满足后重跑 `tools/check_wp.py`，按同一判据 accept。
- 不恢复未入库的 46-goal store 契约副本；store 契约仍不闭合（历史证据
  26/46、12 Timeout、8 Unknown），本文不把它当基线，也不据此做 store 结论。
- Eva/CBMC 门禁和两个 store 的既有证明不受影响。
