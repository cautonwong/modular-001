# 验收用例模板

验收用例驱动真实构建产物，断言用户可观察的值。它不替代单元测试，也不检查内部结构。

## 1. 位置和命名

- IDIS target：`targets/idis_ip_meter/tests/acceptance/test_<area>.py`。
- stategrid target：`targets/stategrid_meter/tests/acceptance/test_<area>.py`。
- 用例 id 用 `<产品>.<行为>`，例如 `idis.power-failure`。
- 一个文件放同一行为域的用例。不要按实现文件拆分。

## 2. 生成器

```sh
python3 tools/new_acceptance_case.py --id idis.example-behavior \
  --description "one sentence on the observable behavior" \
  --tags "idis,persistence" --seconds 10 \
  --start-time 2026-01-01T00:00:00 \
  --require profiles=22 --restart-require registry=5
```

生成 `targets/idis_ip_meter/tests/acceptance/test_<behavior>.py`，断言来自 `--require` 和 `--restart-require` 的 key=value。生成后按需补场景和预置数据。

## 3. 用例骨架

```python
from cases import case


def require(run, key: str, expected: str | None = None) -> str:
    actual = run.values.get(key)
    assert actual is not None, (
        f"missing {key} in runner output: {run.stdout[-300:]!r}")
    if expected is not None:
        assert actual == expected, f"{key}={actual}, expected {expected}"
    return actual


@case(
    "idis.example-behavior",
    "one sentence on the observable behavior",
    ("idis", "persistence"),
)
def example_behavior(ctx):
    first = ctx.run(seconds=10, start_time="2026-01-01T00:00:00")
    assert first.exit_code == 0, f"runner exited {first.exit_code}: {first.stderr[-200:]}"
    require(first, "profiles", "22")
    require(first, "alarm", "0x00000002")

    second = ctx.run(seconds=2)
    assert second.exit_code == 0, f"runner exited {second.exit_code}: {second.stderr[-200:]}"
    require(second, "alarm", "0x00000002")
```

规则。

- 只断言 runner 打印的 `key=value`，不读目录内部状态。
- 时间从固定 `start_time` 开始，结果才可重复。
- 持久化行为用两次运行验证。第二次不带 `start_time`，由固件从存储恢复。
- 需要外部数据时在 `ctx.case_dir` 预置文件，例如注册表 EEPROM 镜像。
- 断言失败时打印实际值和 stderr 尾部。

## 4. stategrid 的差异

stategrid 的验收走 `tests/acceptance/run.py --sim <firmware_sim>`，用例写法相同，`ACC-*` 编号需要在 `docs/firmware-acceptance-matrix.md` 登记一行，`check:acceptance-matrix` 会校验。

## 5. 运行

```sh
# IDIS
python3 tests/acceptance/run.py \
  --runner build/ci-host/idis_host_runner \
  --cases targets/idis_ip_meter/tests/acceptance \
  --workdir /tmp/acc --evidence-dir /tmp/acc-ev

# 只跑一条
--select case:idis.example-behavior
```

CI 里由 `ci_matrix` 自动执行，verdict 必须是 `clean`。

## 6. 检查清单

- [ ] 用例 id 和文件名符合命名规则。
- [ ] 断言的是 runner 输出，不是内部实现。
- [ ] 起始时间固定，两次运行的持久化路径被覆盖。
- [ ] 失败信息带实际值。
- [ ] 新用例让 verdict 保持 `clean`。
