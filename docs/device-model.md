# SoC、board 与 RTOS 设备模型

状态：2026-11-10 设计约束。本文说明换 SoC、换 board、换到 Zephyr 这类带设备驱动模型的 RTOS 时，现有代码怎么分层。

## 1. 核心判断

当前产品线按一个 board 服务一个产品来构建，board 在构建期选定。不引入运行期 board 抽象，也不做 `nd_board_ops_t` 这类函数表。这个假设不是永久约束。通信模块板、板卡修订和测试固件都可能打破它，所以 board 实现仍要保持边界，不做全局单例。

组合根本来就同时看见 domain、application、infra 和 board，直接调用具体 board 函数即可。运行期多态只会增加一层没有收益的间接。

但应用层和领域层不能因此直接 include 某个 board 的头。业务代码需要的能力仍由消费者定义的端口表达，target 在组合根里把端口绑到具体 board 函数。这不是 board 抽象，是业务代码和产品装配之间的依赖倒置。

应用端口不是运行期 board 工厂。它只表达业务需要的一个动作，例如背光开关、继电器反馈、表盖状态。端口按业务能力增长，不按板子数量增长。target 可以只写几行适配函数，直接调用 board 实现，不需要通用 `board_ops`。

Zephyr 的 `struct device`、devicetree、`gpio_dt_spec` 和 `k_*` 只留在 infra 和 target。domain、application 和协议适配器不出现 Zephyr 头，也不出现 `CONFIG_*` 平台宏。

### 1.1 为什么不做运行期 board 接口

- 当前 board 和产品基本一对一，不存在运行期换板需求。函数表只会增加间接和全局状态。
- Zephyr 本身已经提供设备模型。再包一层 `nd_board_ops` 等于重复实现，而且会把 Zephyr 设备指针泄漏到上层。
- 组合根同时看见所有层，直接调用具体 board 函数最清楚，编译期就能发现缺失实现。
- host 测试用链接期替换假 board 实现即可，不需要运行期注入。

### 1.2 这个假设什么时候失效

- 同一块通信板被多个表型复用。仓库里的 `products/comm_plc` 和 `products/comm_rf` 已经接近这种情况。
- 同一产品有多个板卡修订，需要按版本选择不同实现。
- 测试固件、bootloader 和生产工装复用同一块板，但产品逻辑不同。
- 将来需要运行时热切换板级配置，例如一台设备接不同背板。

出现前三种情况时，仍优先用编译期选择和不同 target 配置。只有最后一种才考虑运行期接口。应用端口仍然保留，因为它解决的是业务代码和产品装配的依赖方向，不是板子切换。

### 1.3 厂商 HAL 和 RTOS 原样放 infra

厂商给的 SoC HAL、SDK 和 RTOS 可以原封不动放在 `infra/soc/<soc>`、`infra/rtos/<rtos>`。这些目录是平台实现，允许 include 自己的头，也允许保留厂商的文件结构和构建方式。

product/target 直接 include 这些厂商头，直接调用厂商 API，用来实现组件需要的端口。例如 target 的 `metering_input_adapter.c` 调用厂商 ADC，`backlight_adapter.c` 调用 board 的 GPIO。这层适配是产品装配，不是通用抽象。

规则只有一条，共享 application、domain 和 components 不 include 厂商头，也不调用具体 RTOS API。需要平台能力时，由 target 实现对应端口。

## 2. 构建期选择

每个产品一个 target 组合根。组合根选择 board 实现文件，直接调用 board 函数。

```c
/* targets/<product>/main.c */
#include "targets/stategrid_meter/board/board_io.h"
#include "application/ports/display.h"

static const nd_display_port_t display = {
    .ctx = NULL,
    .backlight_set = board_backlight_set_adapter,
};

void product_init(void)
{
    nd_display_bind(&display);
}
```

约束。

- board 头由 target、board 驱动和 board 实现使用。应用层和领域层不 include。
- 每个 target 只链接一份 board 实现。不同产品可以复用同一份 board 源文件，但每个产品仍通过自己的组合根显式选择，不共享运行期全局状态。
- `products/comm_plc` 和 `products/comm_rf` 已经说明同一类通信板可能被多个产品复用。即使今天是一对一，也不把 board 实现做成全局单例。
- host 测试链接假的 board 实现文件，不引入函数表。
- 如果将来真的出现同一 board 服务多个产品，优先编译期复用加不同 target 配置，而不是运行期切换。运行期接口只在确实需要热切换时再引入。

SoC 同理。product/target 显式选择 `infra/soc/<soc>`。SoC 实现文件可以直接 include 寄存器头、厂商库和启动代码。不抽通用 HAL，也不做运行期 SoC 探测。domain、application 和共享组件不 include SoC 头。

## 3. 应用端口与 board

应用层需要背光、继电器、表盖、按键这类能力时，声明自己的端口。

```c
typedef struct nd_display_port {
    void *ctx;
    void (*backlight_set)(void *ctx, int on);
} nd_display_port_t;

void nd_display_bind(const nd_display_port_t *port);
```

target 实现端口时可以只写一层很薄的适配，直接调用 `board_io` 的 board 函数。没有通用的 `board_ops`，也没有全局 board 绑定。

协议适配器只做编解码和对象映射，不直接读引脚。计量、显示、负荷控制等具体能力通过各自端口进入。

IIC、SPI、UART 的线级操作不属于应用端口。它们属于 board 驱动内部，或者由 Zephyr 的外设设备直接提供。应用层不应该看到 `nd_board_iic_sda_set` 这类函数。

## 4. Zephyr 映射

Zephyr 板上，board 实现文件用 devicetree 和 Zephyr 驱动实现同样的 board 函数。构建时用 Zephyr 的 board 实现替换 stategrid 的寄存器实现，不经过运行期分发。

| 能力 | Zephyr 实现 |
|---|---|
| 背光、继电器 LED、PLC 事件 | `gpio_dt_spec` + `gpio_pin_configure_dt` / `gpio_pin_set_dt` |
| 继电器反馈、表盖、按键 | `gpio_dt_spec` + `gpio_pin_get_dt` |
| RS485 方向 | `gpio_dt_spec` + `gpio_pin_set_dt` |
| EEPROM | `eeprom` 设备，或 `i2c_dt_spec` 加板级器件驱动 |
| SPI Flash | `flash` 设备，或 `spi_dt_spec` 加板级器件驱动 |
| UART 通道 | `uart` 设备 + `uart_rx_enable` / `uart_tx` |
| ADC | `adc_dt_spec` + `adc_read_dt` |
| 看门狗 | `wdt` 设备 |
| 秒节拍 | `k_timer` 或 `k_work_delayable` |
| 通道和消息 | `k_msgq`、`k_fifo`、`k_sem` |
| 互斥 | `k_mutex` |
| 非易失参数 | `flash` / `eeprom` 设备，布局仍由领域层定义 |

devicetree 只描述板级连接。

```dts
/ {
    board {
        backlight-gpios = <&gpioa 4 GPIO_ACTIVE_HIGH>;
        plc-event-gpios = <&gpioc 10 GPIO_ACTIVE_HIGH>;
        relay-led-gpios = <&gpioe 12 GPIO_ACTIVE_HIGH>;
        cover-gpios = <&gpioa 14 GPIO_ACTIVE_HIGH>;
        key-gpios = <&gpioa 2 GPIO_ACTIVE_LOW>;
    };
};
```

Zephyr board 实现只 include `<zephyr/drivers/gpio.h>` 和 devicetree 生成头。domain、application 和协议适配器不 include Zephyr 头。

自定义器件也走同一规则。RN8209 计量前端可以用 Zephyr 驱动模型注册成设备，ESAM 也可以作为 SPI 或 I2C 客户端设备。上层只依赖 `meter_frontend` 和 `security` 端口，不直接拿 `struct device *`。如果器件只是板级粘连逻辑，也可以留在 board 实现里，不强行做成 Zephyr 驱动。

## 5. RTOS 选择

这一层不做通用 RTOS adapter，也不做 `nd_rtos_*` 抽象。

具体 RTOS 的选择和绑定只在 product/target。一个 product 选定一个 RTOS 后就不换。product 的 target 文件可以直接 include 具体 RTOS 头，直接调用具体 API。这是有意的显式依赖。

当前只有一套实际运行的调度器，`nidavellir` 的 tasklet、定时器和主循环。Zephyr 有自己的内核对象和设备模型。现在抽一层通用接口，等于先把未来 API 猜一遍，成本高，收益为零。

规则。

- 外部 RTOS 内核原样放 `infra/third_party/<rtos>/`；`FreeRTOSConfig.h`、`freertos.c` 和具体调用留在 product target。
- product/target 是唯一选择和绑定具体 RTOS 的地方。target 可以显式 include `nidavellir` 或 Zephyr 头。
- `infra/rtos/<rtos>` 可以作为具体实现的存放目录，但它不是共享抽象层，也不暴露通用句柄。只有被该 product 的 target 选中时才参与构建。
- domain、application 和共享组件不 include 具体 RTOS 头。共享组件需要定时、互斥、队列时，由 target 传回调或绑定消费者端口。
- 当前 baremetal target 继续直接使用 `nidavellir`。
- Zephyr target 如果真的落地，优先把现有主循环放进一个 Zephyr 线程，只替换节拍、休眠、中断投递这些入口。需要内核对象时，在 Zephyr target 或对应 infra 模块里直接调用 `k_*`。
- 某个 product 要换 RTOS 时，复制该 product 的 target 和构建配置，改具体依赖即可。不做 adapter。这个做法便宜的前提是共享 application/domain 里没有具体 RTOS 头；所以规则是先把 RTOS 依赖关在 target 里。
- 中断里只做投递，不在中断里跑领域逻辑。这条约束与 RTOS 无关，现在就要遵守。

当前遗留代码里，`application/apps` 还有不少 `ndv_timer`、`send_mesg` 这类调用。这是 staging 债。按新规则，它们离开 staging 前要改成 target 注入的回调或消费者端口。新代码不再引入这类依赖。

## 6. 构建与产品选择

`products/<product>/product.yaml` 增加平台选择，只做描述，不写实现。

```yaml
name: meter_electricity_single_phase_cn
components: [...]
soc: v85xxp
board: stategrid_meter
rtos: baremetal
```

产品要换 SoC、board 或 RTOS 时，复制 `targets/<product>` 和构建配置，改具体的 include、源文件列表和初始化代码。复制 product 是预期的分叉方式，不需要在共享代码里加适配层。

换平台时只需要改 target 里实现端口的那些适配函数。端口接口本身不变，共享 application、domain 和 components 也不变。厂商 SoC HAL 和 RTOS 可以原样放在 `infra/soc`、`infra/rtos`，target 直接 include 和调用它们来完成这些适配。

这个分叉便宜的前提是 domain、application 和共享组件不 include 具体平台头。只要它们保持干净，换平台只需要改 target 和对应 infra 实现。

Zephyr 产品另建组合根。

```
targets/<product>/zephyr/
  CMakeLists.txt
  prj.conf
  boards/<board>.overlay
  src/main.c
```

规则。

- `product.yaml` 和 target 是唯一命名 `soc + board + rtos` 组合的地方。具体实现直接编译进该 product，不做运行期选择，也不做通用平台适配层。
- Zephyr 工程用 `find_package(Zephyr)` 和 `zephyr_library()` 组织组件。
- 一份产品可以同时保留 IAR 和 Zephyr 构建，业务源码共用。
- host 测试只编译 domain、application 和纯 infra，链接假 board 实现。不需要假 RTOS。
- Zephyr 侧关闭堆，或把堆留给外部模块。核心路径只用调用方提供的静态内存。

## 7. 迁移顺序

1. ~~保持 `board_io` 的 board 专用身份，不拆通用 board 接口。把 IIC、SPI、UART 的线级函数从公开头移到 board 驱动私有头。~~ 已完成，线级操作在 `*_pins` 私有模块。
2. 给应用和领域需要的共享能力补消费者端口，target 直接调用 board 函数实现这些端口。背光、继电器反馈和计量输入已经落地，继续按真实消费者补。
3. 不做通用 RTOS adapter。Zephyr 版本复制 product target，改构建和依赖，先把现有主循环放进一个 Zephyr 线程，只替换节拍、休眠和中断投递入口。
4. 为一块 Zephyr 支持的板做最小 board 实现和产品骨架，只跑背光和秒节拍，证明构建期替换可行。
5. 把 UART、I2C、SPI、ADC 驱动逐个换成 Zephyr 设备实现。
6. 增加主机假 board 实现，给应用端口补测试。RTOS 侧只测纯 domain/application，不需要假 RTOS。
7. 最后迁移协议组件和存储，保持持久化字节布局不变。

## 8. 验收标准

- 不存在运行期 board 函数表，也不存在全局 board 绑定。
- 当前每个 target 链接一份 board 实现。新增 board 只新增 `targets/<product>/board`、board 实现和 target 绑定，不改 domain 和 application。board 源文件可以在多个产品间复用，但每个产品显式选择自己的实现。
- 新增 SoC 只新增 `infra/soc/<soc>` 和该 SoC 的 board 实现，不改上层。
- 新增 RTOS 复制 product target 和构建配置，改具体依赖。共享 domain、application 和协议适配器不改。不做通用 RTOS adapter。
- domain 和 application 的 include 图里没有 Zephyr、devicetree、`infra/soc`、`targets/stategrid_meter/board`、`infra/rtos` 或 `targets`，也没有 `CONFIG_*` 平台宏。
- Zephyr 产品能和 IAR 产品共用 domain、application 和协议源码，只替换 board 实现、调度入口和构建文件。
- host 测试可以在没有板子和 RTOS 的情况下运行全部业务规则。
- 旧持久化切片、事件记录和冻结记录的字节回归测试持续通过。

## 9. 当前差距

- `board_io` 是 stategrid 专用实现，这个定位保留，不再计划拆通用 board 接口。
- 线级操作已经移出 `board_io.h`，只在 `iic_pins`、`spi_pins`、`uart_pins`、`rn8209_pins`、`esam_pins` 和 `relay_pins` 私有模块里。
- 显示、运行状态和计量输入已经改走端口。application 和 domain 目录不再 include `board_io.h`，也不再直接调用 `nd_board_*`。
- `infra/board` 已合并到 `targets/stategrid_meter/board`，依赖 board 的 nidavellir、time_link、SoC ISR 和 ESAM 源文件也移到 target。非 staging 的平台 include 和 RTOS 调用为 0。
- application/apps、domain legacy 和 dl645 adapter 的平台 include/RTOS 调度调用已经清零。`tools/check_platform_boundary.py` 把它们记为 staging 债，新增的非 staging 违例会直接失败。
- `nidavellir` 仍是实际调度器。这是有意保留的，不计划先抽通用 RTOS adapter。
- `application/apps` 里还有约 50 个文件直接调用 `ndv_timer`、`send_mesg` 或 tasklet。它们还没有遵守“RTOS 选择只在 target”的规则，离开 staging 时要逐个收口。
- 还没有 Zephyr 组合根、devicetree overlay 或 Zephyr board 实现。
- 还没有主机假 board 实现。
