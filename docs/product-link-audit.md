# product-link 源例外清零（2026-11-23）

原审计（同日早些时候）报 14 条 source 例外：两个 IDIS 产品各 7 条，全部
justified。评审决定采用产品级源选择（Option A）清零。

## 决定

- 两个 IDIS 产品在 `product.yaml` 用 `sources:` 声明固件实际链接的源：
  `core.types` 的 `components/core/types/utils.c`、`protocol.dlms` 的
  `components/protocol/dlms/src/nd_axdr.c`。
- 四个 `nd_dlms_*.c` 保持 host-test-only，由全局 accounting 规则保有：
  某个固件产品选中的组件，其每个 `.c` 必须被至少一个固件产品选中，或被
  `tests/CMakeLists.txt` 编译。
- 采用反查规则：被选中组件交付的源若出现在链接里，必须在产品解析清单中；
  仓库内每个链接输入必须落在某个组件 `paths` 下，`infra/third_party/` 和
  仓库外路径是构建输入。
- `non_firmware` 记录保留在 baseline；两个 twin 继续各自复制组件与源清单。

## 变更

- `tools/check_components.py` / `tools/product_manifest.py`：共享的
  `parse_manifest()` 解析 `sources:` 嵌套映射，`product_sources()` 消费它。
  子单元 1 落地时三个产品都还没使用 `sources:`，`--cmake`
  输出与基线逐字节一致（`cmp` 已核对）；两个 IDIS 产品随后各自收窄 7 个
  test-only 源。`--self-check` 每次调用都跑。
- `tools/check_product_component_usage.py`：链接覆盖改读 resolver，加反查
  规则和全局 accounting 规则。旧 premise 机制（`load_baseline` source 分支、
  `check_precondition`、`include_reaches`、`owns_path`、`dependency_closure`）
  删除，保留测试清单扫描 `compiled_in()`。
- `tools/product_link_baseline.txt`：删除 14 条 source 记录，只留
  `non_firmware`。
- 文档：`docs/architecture.md`、`docs/platform-layout.md`、本文件、
  `docs/ROADMAP.md`。

## 结果

```
product link ok (3 firmware products, 0 source exceptions, 4 non-firmware products)
```

- 两个 IDIS 的链接输入集逐字节不变：裸机 149 个源、FreeRTOS 156 个源；
  POLL 与 IRQ 两种模式相同。
- 四个 IDIS 镜像 ROM 不变：95,068 / 97,552 / 99,288 / 101,968 B（裸机 POLL、
  裸机 IRQ、FreeRTOS POLL、FreeRTOS IRQ），ELF sha256 相同。
- stategrid 链接清单（231 个源）与 `--cmake` 输出不变。
- 生成的两个 IDIS 源清单各少 7 个 test-only 源（102 行，原 109 行）。

## 负例

- 新源无落点：给被选中组件加一个既不链接也不在测试清单的 `.c`，失败。
- linked 未声明：把 `core.types/list.c` 加进 IDIS 的 `INFRA_SOURCES`，反查
  规则失败。
- 测试清单删条目：删掉 `tests/CMakeLists.txt` 的 `nd_dlms_apdu.c`，
  accounting 规则失败。

## 复现

```sh
# arm-none-eabi-gcc 和 python3 在 PATH 上
python3 tools/check_product_component_usage.py
python3 tools/product_manifest.py --self-check
python3 tools/product_manifest.py --cmake meter_electricity_single_phase_idis
```
