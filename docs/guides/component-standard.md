# 组件标准写法

本文规定一个组件的目录、清单、接口和测试形状。目标有两个。新组件照抄模板就能接入。已有组件不会因为个人习惯各写各的。

## 1. 目录和命名

目录决定名字和层，不要手起名。

| 目录 | 组件名 | layer |
|---|---|---|
| `domain/<x>` | `domain.<x>` | `domain` |
| `application/<x>` | `application.<x>` | `application` |
| `application/adapters/<x>` | `application.adapters.<x>` | `protocol` |
| `components/<类别>/<x>` | `<类别>.<x>` | 该类别 |
| `infra/<x>` | `infra.<x>` | `infra` |
| `infra/boards/<board>` | `board.<board>` | `platform` |
| `infra/soc/<vendor>/<soc>` | `soc.<vendor>_<soc>` | `platform` |
| `infra/third_party/<vendor>/<lib>` | `third_party.<vendor>_<lib>` | `platform` |
| `targets/<t>` | `target.<t>` | `product` |
| `targets/<t>/<子目录>` | `target.<t>.<子目录>`（跳过 `app/`） | `product` |
| `nidavellir/include*` | `legacy.*` | `infra` 或 `product` |

旧的 `nd_*` 组件名已在 2026-11-15 统一迁移到上表规则，现在没有例外。

## 2. 清单模板

```yaml
name: domain.example
version: 0.1.0
layer: domain
paths: [domain/example]
dependencies: [domain.common]
```

- `version` 必须是 `x.y.z`。
- `dependencies` 只写你 include 的组件。方向见 `docs/platform-layout.md` 第 4 节。
- 属于待迁移的旧代码才加 `legacy: true`，加了就要在 `tools/legacy_report.py` 里能解释。

## 3. 接口文件

每个组件有一个公开头文件，其他组件只 include 它。

- 用仓库根相对路径 include，例如 `"domain/example/example.h"`。
- 头文件自己能编译。需要的前置类型用前向声明，不靠调用方先 include。
- 函数全部在头文件里声明。`check-arm.sh` 对 domain 和 application 启用了 `-Wmissing-prototypes`。
- 可变全局变量不进头文件。需要共享状态时给访问器，参考 `nd_platform_device_error()`。
- 消费者定义的端口放 `application/ports/`，实现放在 target 的组合根里绑定。模板见 `docs/guides/port-binding.md`。
- 头文件不 include `targets/`、`infra/soc`、`infra/boards`、`nidavellir/`。这些由 `check:platform` 和 `check:layers` 拦截。

## 4. 测试

- host 单元测试放 `tests/unit/test_<模块>.c`，用 Unity，必须有 `setUp` 和 `tearDown`。
- 测试断言用户可观察的值，不断言内部结构。
- 组件行为跨文件时，优先在 target 的 Python 验收用例里加一条，模板见 `docs/guides/acceptance-case.md`。

## 5. 新建组件

```sh
python3 tools/new_component.py --path domain/example --dep domain.common
```

工具会生成 `component.yaml`、`example.h`、`example.c`，并跳过不符合标准的路径或名字。生成后把组件名加进 `products/<product>/product.yaml` 的 `components` 列表，再跑：

```sh
FREERTOS_KERNEL_DIR=/home/edge/FreeRTOS-KernelV11.3.3 ./ci.sh
```

`check:component-standard` 会校验名字、版本、层、路径和依赖。`check:components` 会校验归属和 include 方向。端口和验收用例分别用 `tools/new_port.py`、`tools/new_acceptance_case.py` 生成。

## 6. 检查清单

- [ ] 目录在正确的层，名字由目录推导。
- [ ] 清单字段齐全，版本是 `x.y.z`。
- [ ] 公开头文件独立可编译，函数有原型。
- [ ] 没有可变全局，没有跨层 include。
- [ ] 依赖只列实际 include 的组件。
- [ ] 有单元测试或验收用例覆盖新行为。
