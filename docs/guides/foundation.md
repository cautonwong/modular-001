# 基础层 `components/core`

`components/core` 就是本仓库的 foundation。没有单独的 `foundation` 层，也不建。
core 组件不依赖任何其他组件，供 domain、application、services、infra 和 targets
共用。

## 现有组件

| 组件 | 内容 | 依赖 |
|---|---|---|
| `core.types` | `type_def.h`、`utils.h` 及其实现（BCD/CRC/缓冲区工具、RTC 时间换算、bitarithm、list） | 无 |
| `core.buf` | 缓冲区 | 无 |
| `core.bus` | 订阅式总线 | 无 |
| `core.fifo` | 唯一的 FIFO 存储类型 `struct fifo_t` 和 `fifoBuf_*` API | 无 |

## 判断标准

放进 core 的代码必须同时满足：

1. 不依赖平台。没有寄存器、SoC 头、RTOS 调用、`targets/` 或 `infra/soc`。
2. 与产品无关。换一个产品、换一块板子仍然成立。
3. 被两个以上不相关的组件使用，或者属于所有层共用的类型。

不满足就留在消费方：只被一个 target 使用的头放回该 target（例如
`resource_blocks.h`、`key_config.h`），只被 SoC 使用的代码进 `infra/soc`
（例如 tick 定时器 `time_link`），协议状态机进 `application/adapters`。

## 边界

- `check:components` 的层规则：core 不能 include domain、application、
  protocol、services、infra、platform、targets。依赖清单必须为空。
- `tools/check-arm.sh` 只严格编译 domain 和 application；core 的公开头仍需独立可编译，
  不引入 host libc 之外的新依赖。
- 一个组件一个公开头。FIFO 只有 `components/core/fifo/fifo.h` 一个存储类型。
  在 domain 或 application 里定义第二套 FIFO 结构会被 `check:layers` 拒绝。
- `infra/kernel` 已于 2026-11-18 删除：FIFO 进 `core.fifo`，通用工具和时间换算
  进 `core.types`，tick 定时器进 `soc.v85xxp_v8500`，target 专属头回到 target。
  `check:component-standard` 在 `infra/kernel` 目录重新出现时失败。

## 新增 core 组件

按 `docs/guides/component-standard.md` 的模板创建 `components/core/<name>/`，
`layer: core`、`dependencies: []`，然后在 `product.yaml` 的 `components` 里显式
选中。不要从 core include 任何上层头；需要上层能力时，能力本身放错了位置。

## 验证

`check:components` 的自检覆盖 include 方向和依赖方向；`host:strict` 和
`host:sim-san` 编译并运行 core 的 host 测试（`test_fifo`、`test_list`、
`test_bitarithm`）。
