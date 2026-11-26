# WP Phase 0 wiring probe

Phase 0 的玩具契约：只验证 `frama-c -wp` 的 prover 接线和 verdict 读取，
不建模生产 store，也不进门禁。决策与完整计数见
`docs/design/wp-decision.md`。

## 运行

Frama-C 33.0 bundle 不带 prover；先把 pin 住的 solver 放进 `PATH`。
`FRAMA_C` 指向 bundle 的 `bin/frama-c`。

```sh
export FRAMA_C=<frama-c-bundle>/bin/frama-c
python3 tools/check_wp.py --self-check
python3 tools/check_wp.py --solver z3 --timeout 5
PATH=<alt-ergo-2.6.5>/bin:$PATH python3 tools/check_wp.py --solver alt-ergo
PATH=<cvc5-1.2.1>/bin:$PATH python3 tools/check_wp.py --solver cvc5
```

没有 `PATH` 覆盖时用系统 z3 4.8.12 和 Alt-Ergo 2.0.0。probe 只在至少一个
solver 同时满足“假目标 invalid、真目标 valid”时退出 0；当前 pin 住的版本都
不满足，退出 1。

Alt-Ergo 2.6.5 的 static binary 用提交号回答 `--version`，why3 只认数字
版本；上表的 2.6.5 结果用了一个只把 `--version` 改成 2.6.5 的本地 shim，
底层仍是 `wp-decision.md` 表里 sha256 pin 的同一 release。没有 shim 时 why3
不会选 `alt_ergo_26` driver；直接在生成的 `.psmt2` 上跑该 binary 仍是
Timeout。

## 当前结果

- 真目标：z3 4.8.12、Alt-Ergo 2.0.0/2.6.5、CVC5 1.2.1/1.3.1 都判 valid，
  0.013–0.026 s，且都是外部 prover 而不是 Qed。
- 假目标：z3 4.8.12 和 Alt-Ergo 2.6.5 判 timeout；Alt-Ergo 2.0.0 和
  CVC5 1.2.1/1.3.1 判 unknown。没有 invalid。
- 原因：WP 的量化算术 prelude（abs/div/mod）让假目标的 sat 查询无法在 5 s
  内决出；去掉 prelude 后 z3 立即返回 sat。

## 不做什么

- 不进 `tools/check_formal.sh`，不进 CI。当前 CI 的 Frama-C 门禁仍只有 Eva。
- 不恢复未入库的 46-goal store 契约副本。
