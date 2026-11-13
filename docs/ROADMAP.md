# 下一阶段路线图

日期：2026-11-11  
基线：`fe0f58b3` 之后加入事件目录迁移

## 1. 目标

把 `modular-001` 从“按目录分层”推进到“按限界上下文和端口组织”的可复用组件仓库：

- 领域层只表达业务规则，不暴露 `mbase_control_block_t`、`instance_t` 的布局。
- application 只依赖消费者定义的端口，不依赖具体存储、协议或板级实现。
- target 只做组合根、硬件适配和持久化兼容。
- product 清单驱动构建和组合，而不是只做说明。
- 所有迁移保持旧持久化字节兼容。

硬约束：只做本地提交，不 push 远端。

## 2. 当前基线

- 33 个组件清单，源文件全部有归属，跨组件未声明依赖为 0。
- `check_layers` 0，platform boundary 0。
- `legacy_baseline` 205/205 编译链接，`link_undefined=0`。
- `ctest` 72/72。
- domain/application 在 Cortex-M0 下严格编译。
- 事件、冻结、记录捕获共 10 组规则已迁入 domain。

## 3. P0 收尾迁移（已完成）

验收标准：

- 每个 `.c/.h` 有组件归属。
- 跨组件 include 必须声明 dependency。
- domain/application 通过严格 ARM 编译。
- 每项业务规则迁移都有主机测试。
- 旧实现从 target 删除，不只做转发壳。

已完成：

- adapter 警告抑制全部删除，并修正 698 DIR 判断和消息类型偏移。
- 645/698 matcher、组帧、AXDR、地址、密码、连接/GET/SET/ACTION、分帧、客户端转发测试。
- 组件归属和依赖闭环。
- 开关误动作、MAC 挂起、编程列表、RTC、冻结空间、回滚、上电恢复、捕获映射、方案可见性、事件目录。

## 4. P1 核心模型解耦（已完成）

### P1.1 收窄 `instance_t`（已完成）

现状：`nidavellir/include/kernel/instance.h:58` 定义宽运行时句柄，domain 只前置声明，但 application 仍可直接取字段。

工作：

1. 盘点 domain/application 对 `instance_t` 的实际访问，按用途分组。
2. 为每组定义消费者端口，例如计量账本、事件存储、时钟、上报队列。
3. target 在组合根实现端口，application 只接收端口句柄。
4. 删除 application 对 `kernel/instance.h` 的直接 include。

验收：

- domain/application 不 include `kernel/instance.h`，也不定义 `instance_t` 布局。
- 现有主机测试和 ARM 检查通过。
- `tools/check_instance_boundary.py` 固化该边界。

### P1.2 让计量端口脱离 `mbase`（已完成）

现状：`domain/metering/metering_energy_port.h:17` 仍暴露 `struct mbase_control_block_t *`。

工作：

1. 定义不透明账本句柄和读写函数，覆盖当前计量端口实际使用的字段。
2. 把 `mbase` 字段访问收进 target/infra 适配器。
3. 保持 `sizeof(struct mbase_control_block_t) = 1368`、`persistent_section_start = 849`、`active_p = 852` 不变。
4. 用 `tools/legacy_probe.c` 和静态断言守住偏移。

验收：

- `domain/metering` 不 include `kernel/legacy_control_block.h`。
- 账本端口不出现 `mbase` 类型。
- 持久化探针数值不变，205/205 链接通过。

状态：端口已改为 `nd_energy_ledger_t`，target 适配器负责句柄转换，边界检查通过。

### P1.3 `mbase` 边界隔离（边界完成，target 内部未收口）

用户已明确暂不拆 `mbase_control_block_t`。本阶段只做隔离：

- domain/application 不 include 布局，账本端口不出现 `mbase` 类型。
- target legacy 仍直接访问字段，尚未全部收进 target 访问器。
- 持久化仍按原结构偏移读写。
- 在 `docs/architecture.md` 保留违规记录。

决策门：只有用户明确同意后才启动结构体拆分；target 内部访问器是后续可选项，不影响分层边界。

## 5. P2 组件与产品化

### P2.1 产品清单驱动构建（固件链接已接入）

- `check_components.py` 校验产品选择的组件依赖闭包，五个产品清单已补齐。
- `tools/product_manifest.py` 按 product.yaml 解析闭包并生成确定性的 CMake 源清单（只收 `.c`）。
- 宿主构建的 `products_check` 目标每次校验产品清单。
- 固件链接使用 `--product meter_electricity_single_phase_cn`：源清单来自 product.yaml 闭包，`check-firmware-link.py` 要求它与 `.ewp` 源表双向完全一致，只在一侧出现的源文件直接失败。
- 未参与构建的 owned 源文件通过组件的 `exclude` 显式记录；39 个组件共 205 个 `.c` 与 EWP 完全一致。
- 尚未完成：IAR 工程本身仍手写，清单不生成 `.ewp`；组合根仍手写。

### P2.2 拆掉兜底组件（检查完成，容器缩小待续）

- `target.stategrid_meter` 根清单只列根目录 33 个文件，不再用宽路径加排除项。
- 18 个 target 子目录各自建立清单，组件总数 59。
- 59 个组件全部检查 include 依赖和层方向，legacy 不再豁免；协议组件通过 `application/ports/fifo_channel.h` 脱离 infra 头。
- `check_components.py` 解析全部 `.ewp` 的 include 路径，按所属工程的搜索顺序解析裸头名；未构建（`exclude`）的源文件不参与依赖检查。
- legacy 容器已按目录拆开：`infra.boot` 拆成 app/drivers/hd/updata，`legacy.kernel_headers` 拆成 kernel/base/dlms/test 头，`target.msta` 拆出 apps；`infra.os.baremetal` 只有单个源文件，保持原样。

### P2.3 协议适配器归位（依赖完成，forward 未绑定）

- `application/adapters/dl645`、`dl698` 已按组件归位，组件依赖检查确认不 include target。
- DLMS 组件保持独立，不进入当前产品构建。
- 698 客户端 `forward` 仍未绑定：当前 target 没有客户端消息消费者，绑一个空回调会改变协议行为；留待真正接入客户端业务时注册。

验收：适配器只依赖 application 端口和协议组件，不依赖 target。已满足。

## 6. P3 验证与交付

### P3.1 真机回归（待硬件）

需要设备或现场环境，按 `docs/esam-compatibility.md` 验证：

- 正确 MAC 通过，篡改 MAC 被拒，纯 RN 通过。
- 上报确认、断线重连、掉电重启后的重发状态。
- 广播校时和红外权限分支。
- 698 服务端 DIR 过滤、分帧末帧、目标钩子消息类型。

### P3.2 协议样本库（骨架完成，现场样本待补）

- `tests/corpus/` 定义现场抓包的落盘格式，`tests/unit/test_protocol_corpus.c` 用原始字节回放 645/698 matcher。
- 已有四条回归种子，覆盖有效帧和坏校验帧。
- 尚未完成：现场抓包需要设备或现场数据；当前样本不是现场报文。

### P3.3 持续集成（已完成）

- `ci.sh` 串起 `ctest`、`legacy_baseline`、`check-arm`、`check-layers`、`check-components`、`check_instance_boundary`、`check_platform_boundary` 和产品闭包检查。
- `.github/workflows/ci.yml` 在 Ubuntu 上安装工具链后执行同一脚本。
- 失败时保留构建日志和命令输出。

### P3.4 固件链接（链接完成，硬件未验证）

- 增加 GNU 链接脚本、复位向量启动文件和 `__CALL`/CSTACK 链接桩。
- `tools/check-firmware-link.py` 用产品源清单（并与 EWP 源表双向锁定）链接出 V85XXP 256K flash / 32K RAM 的最终 ELF，`nm -u` 为 0。
- 尚未完成：启动文件只有复位向量和默认异常处理，没有 IAR 运行时、完整向量表和板级初始化；不能当作已验证的烧录镜像。
- 真机运行验证仍待硬件。

## 7. 当前状态与剩余项

已完成：P0、P1.1、P1.2、P3.3。

已完成主体、仍有外部依赖：

1. P2.1：产品清单已驱动固件源清单并与 EWP 双向锁定；IAR 工程生成和组合根仍手写。
2. P2.2：59 个组件全部检查 include 依赖和层方向；legacy 容器已按目录拆开（`infra.os.baremetal` 为单文件，保持原样）。
3. P2.3：适配器不依赖 target；698 客户端 `forward` 仍无消费者。
4. P3.2：回放骨架已就位；现场抓包待补。
5. P3.4：已链接最终 ELF；启动文件和运行时未在硬件验证。

阻塞项：

6. P3.1：需要真机或现场环境，无法在仓库内完成。

P1.3 的 `mbase` 拆分是决策门，不默认启动。需量、月结、阶梯结算仍按用户要求保留在 legacy，恢复时先迁语义再删旧文件。
