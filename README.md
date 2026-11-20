# modular-001

`modular-001` 是一个面向智能电表、通信模块及多 SoC、board、RTOS 产品的组件化 C11 单体仓库。项目使用领域驱动的分层架构、消费者定义端口和 target 组合根装配，目标平台以 Cortex-M0 为主，并保留存量设备 EEPROM 布局兼容。

## 当前状态

- 主语言为 C，并包含少量 Python、Assembly、CMake、C++ 和 Go。
- 已落地领域、应用、协议、服务、基础设施和产品 target 分层。
- 支持裸机及 FreeRTOS 构建。
- 当前产品包括国网状态表、两个 IDIS IP 电表 target 和 host 仿真 target。
- 业务组件和平台组件通过 `product.yaml` 选择。
- 持久化兼容、依赖方向、组件归属、Cortex-M0 编译和固件链接受自动检查约束。

## 仓库布局

| 路径 | 职责 |
|---|---|
| `domain/` | 纯领域能力：计量、费率、冻结、事件、负荷控制、显示、安全、上报等 |
| `application/` | 用例编排和协议适配器 |
| `components/` | 可复用 core、domain、application、protocol 和 services 组件 |
| `infra/` | SoC、驱动、RTOS、第三方代码及基础设施适配 |
| `targets/` | 产品组合根、board 包、应用入口、配置和构建脚本 |
| `products/` | 产品描述：组件选择与平台选择 |
| `tests/` | 主机单元测试、验收用例、协议语料和仿真测试 |
| `formal/` | CBMC、Frama-C/Eva、TLA+ 和 Lean 形式化验证 |
| `tools/` | 架构门禁、构建清单、固件检查和验收工具 |
| `docs/` | 架构、平台、协议、验证和迁移文档 |

## 架构约束

- `domain` 不依赖平台、协议或具体 target。
- `application` 依赖领域契约，并通过消费者定义的端口工作。
- `components` 不得直接依赖 SoC、board、RTOS、target 或产品目录。
- `targets/<product>` 是唯一装配点：创建对象、绑定端口、启动调度。
- 平台差异只出现在 `infra/soc`、`infra/drivers`、`infra/rtos` 和产品 board。
- 聚合和资源由调用方持入或注入，不读可变全局状态，不由 domain 调用宿主 `malloc`。
- 存量电表持久化布局必须逐字节兼容；布局偏移由静态断言和回归测试锁定。

## C4 模型

下面的七张图按 C4 模型的四个抽象层及关键横切视图组织。

### 1. 系统上下文

```mermaid
flowchart LR
    Operator["开发 / 运维人员"] --> Repo["modular-001<br/>组件化嵌入式固件仓库"]
    Field["现场工程师"] --> Meter["智能电表"]
    Meter --> Repo
    Meter -->|DL/T 698 / 645 / DLMS| Headend["主站 / 采集系统"]
    Comm["PLC / RF 通信模块"] --> Headend
    Repo --> CI["CI 与验收环境"]
    Repo --> Hardware["SoC / board / HIL"]
```

### 2. 容器视图

```mermaid
flowchart TB
    subgraph Repo["modular-001 仓库"]
        Domain["Domain 容器<br/>计量、费率、冻结、事件、安全"]
        Application["Application 容器<br/>用例与协议适配"]
        Components["Components 容器<br/>core / protocol / services"]
        Infra["Infrastructure 容器<br/>SOC / driver / RTOS / third-party"]
        Target["Target 容器<br/>host 与产品固件组合根"]
        Tools["Tools 与 Tests<br/>门禁、单元、验收、仿真"]
    end

    Target --> Application
    Target --> Domain
    Target --> Components
    Target --> Infra
    Application --> Domain
    Components --> Domain
    Infra -->|实现端口| Application
    Tools --> Domain
    Tools --> Components
    Tools --> Target
```

### 3. 组件视图：领域与应用

```mermaid
flowchart LR
    subgraph Domain["Domain 组件"]
        Metering["Metering<br/>能量、需量、瞬时量"]
        Tariff["Tariff<br/>费率、阶梯、日历"]
        Freeze["Freeze<br/>冻结计划与序号"]
        Events["Events<br/>去抖、记录、告警"]
        Control["Load Control<br/>继电器与负荷控制"]
        Security["Security<br/>权限与认证结果"]
        Report["Report<br/>上报策略与编码规则"]
    end

    subgraph Application["Application 组件"]
        MeterUse["计量秒用例"]
        Adapters["698 / 645 协议适配"]
        Ports["消费者定义端口"]
    end

    MeterUse --> Metering
    MeterUse --> Freeze
    MeterUse --> Report
    Adapters --> Metering
    Adapters --> Tariff
    Adapters --> Events
    Adapters --> Security
    Adapters --> Control
    Adapters --> Ports
```

### 4. 组件视图：可复用组件与基础设施

```mermaid
flowchart TB
    subgraph Core["components/core"]
        Types["types / bytes"]
        Buf["buf / fifo"]
        Bus["bus"]
    end

    subgraph Protocol["components/protocol"]
        DL698["DL/T 698"]
        DL645["DL/T 645"]
        DLMS["DLMS"]
    end

    subgraph Services["components/services"]
        Channel["channel"]
        Storage["storage"]
        Security["security"]
    end

    subgraph Infra["infra"]
        SOC["SOC HAL"]
        Board["Board adapter"]
        Drivers["器件驱动"]
        RTOS["RTOS / 裸机"]
    end

    Target["Target 组合根"]

    Target --> Core
    Target --> Protocol
    Target --> Services
    Target --> Infra
    Protocol --> Core
    Protocol --> Channel
    Services --> Core
    Infra -->|实现端口| Services
```

### 5. 产品装配视图

```mermaid
flowchart LR
    Manifest["products/<product>/product.yaml"] --> Resolver["component manifest resolver"]
    Resolver --> DomainProfile["domain profile"]
    Resolver --> ProtocolSet["protocol components"]
    Resolver --> PlatformSet["SOC / board / RTOS"]

    subgraph Target["targets/<product>"]
        Main["main / app 装配"]
        Board["board 引脚与器件实例"]
        Config["产品参数"]
        Build["CMake / IAR / linker"]
    end

    DomainProfile --> Main
    ProtocolSet --> Main
    PlatformSet --> Main
    Board --> Main
    Config --> Main
    Main --> Firmware["固件产物"]
```

### 6.运行时交互：协议帧到领域用例

```mermaid
sequenceDiagram
    participant RX as 通道 / UART / PLC
    participant P as Protocol Adapter
    participant D as Domain Use Case
    participant S as Storage / Metering
    participant TX as Channel / Radio / UART

    RX->>P: 接收字节帧
    P->>P: 校验、解析、分帧
    P->>D: 参数化领域请求
    D->>S: 读取或更新领域状态
    S-->>D: 值对象 / 结果
    D-->>P: 响应语义
    P->>P: 编码响应
    P->>TX: 输出帧
```

### 7. 验证与交付流水线

```mermaid
flowchart LR
    Source["Source changes"] --> Static["层依赖 / 组件 / 可变全局检查"]
    Source --> Host["host strict / sanitizer 测试"]
    Source --> ARM["Cortex-M0 编译检查"]
    Source --> Link["固件链接与 section 预算"]
    Source --> Accept["产品验收矩阵"]
    Source --> Formal["CBMC / Frama-C / TLA+ / Lean"]

    Static --> Verdict["Verdict"]
    Host --> Verdict
    ARM --> Verdict
    Link --> Verdict
    Accept --> Verdict
    Formal --> Verdict

    Verdict -->|clean| Artifact["固件与验收证据"]
    Verdict -->|失败| Fix["修复并重跑门禁"]
```

## 构建与验证

仓库提供统一入口：

```bash
./ci.sh
```

常用验证内容包括：

- `host:strict` 与 sanitizer 测试。
- `host:sim-san` 整机仿真。
- Cortex-M0 严格编译。
- 层、组件、平台边界和可变全局检查。
- 产品清单、固件链接和 ROM/RAM 预算检查。
- stategrid 与 IDIS 验收矩阵。
- CBMC、Frama-C Eva、TLA+ 和 Lean 形式化检查。

补充架构文档：

- [架构层与依赖规则](docs/architecture.md)
- [组件化 monorepo 设计](docs/monorepo.md)
- [平台目录与可见性规范](docs/platform-layout.md)
- [ROADMAP](docs/ROADMAP.md)
- [组件标准](docs/guides/component-standard.md)
- [端口绑定](docs/guides/port-binding.md)
- [验证分层](docs/guides/verification.md)
