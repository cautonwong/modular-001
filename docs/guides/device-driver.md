# 器件驱动规范

适用 `infra/drivers/<class>/<chip>`。`<class>` 用 `metering`、`storage`、`display`、`io`、`comm`、`security`。总线控制器（UART/IIC/SPI）属于 SoC 外设，放 `infra/soc/<vendor>/<soc>`，不放这里。

## 1. 边界

驱动只做三件事。把总线上的寄存器或存储操作翻译成函数。检查参数和范围。返回 0 或负的错误码。

驱动不做这些事。不 include `targets/`、`type_def.h`、`target.h`、`cpu_surport.h`。不出现 `struct dev_info_t`、`driver_ops_t`、`struct bus_ops_t`。不保存校准参数。不读 board 全局变量。

`tools/check_drivers.py` 强制上述约束。

## 2. 目录和清单

```
infra/drivers/storage/eeprom/
  component.yaml
  eeprom.h
  eeprom.c
tests/unit/test_eeprom.c
```

```yaml
name: driver.storage_eeprom
version: 0.1.0
layer: platform
paths: [infra/drivers/storage/eeprom]
dependencies: []
```

驱动不依赖 SoC 组件。总线是函数指针，由 board 注入。

## 3. 公开头文件

```c
#ifndef ND_INFRA_DRIVERS_STORAGE_EEPROM_EEPROM_H
#define ND_INFRA_DRIVERS_STORAGE_EEPROM_EEPROM_H

#include <stdint.h>

typedef struct nd_eeprom_bus {
    void *ctx;
    int (*write)(void *ctx, uint8_t device, uint32_t addr,
                 const uint8_t *buf, uint16_t len);
    int (*read)(void *ctx, uint8_t device, uint32_t addr,
                uint8_t *buf, uint16_t len);
} nd_eeprom_bus_t;

typedef struct {
    nd_eeprom_bus_t bus;
    uint8_t device_address;
    uint16_t page_size;
    uint32_t size;
} nd_eeprom_config_t;

#define ND_EEPROM_STORAGE_SIZE 48

typedef union {
    uint64_t align;
    uint8_t bytes[ND_EEPROM_STORAGE_SIZE];
} nd_eeprom_storage_t;

typedef struct nd_eeprom nd_eeprom_t;

nd_eeprom_t *nd_eeprom_init(nd_eeprom_storage_t *storage,
                            const nd_eeprom_config_t *config);
int nd_eeprom_read(nd_eeprom_t *dev, uint32_t addr, uint8_t *buf, uint16_t len);
int nd_eeprom_write(nd_eeprom_t *dev, uint32_t addr, const uint8_t *buf,
                    uint16_t len);

#endif
```

规则。头文件只 include `stdint.h`、`stddef.h`、`stdbool.h`。句柄不透明，布局在 `.c`。实例存储由调用方持有，驱动里没有 `static` 实例，也没有堆。

## 4. 驱动实现

```c
#include "infra/drivers/storage/eeprom/eeprom.h"

#include <stddef.h>

struct nd_eeprom {
    nd_eeprom_config_t config;
};

_Static_assert(sizeof(struct nd_eeprom) <= sizeof(nd_eeprom_storage_t),
               "eeprom storage too small");

nd_eeprom_t *nd_eeprom_init(nd_eeprom_storage_t *storage,
                            const nd_eeprom_config_t *config)
{
    struct nd_eeprom *dev = (struct nd_eeprom *)storage;

    if (dev == NULL || config == NULL || config->bus.read == NULL
        || config->bus.write == NULL) {
        return NULL;
    }
    dev->config = *config;
    return dev;
}
```

返回 `int`，0 成功，负失败。页写切分、ACK 轮询、重试次数属于驱动。校准值、费率、业务判断属于调用方。

## 5. board 侧

board 提供总线回调和存储，负责实例化。

```c
static int bus_write(void *ctx, uint8_t device, uint32_t addr,
                     const uint8_t *buf, uint16_t len)
{
    /* SoC I2C 起始、设备地址、三字节存储地址、数据、停止。 */
}

static nd_eeprom_storage_t eeprom_storage;

static const nd_eeprom_config_t eeprom_config = {
    .bus = { .ctx = NULL, .write = bus_write, .read = bus_read },
    .device_address = 0xA0,
    .page_size = 0x80,
    .size = 0x10000,
};

nd_eeprom_t *board_eeprom(void)
{
    return nd_eeprom_init(&eeprom_storage, &eeprom_config);
}
```

引脚和初始化顺序属于 board。驱动不认识引脚。

## 6. target 侧

board 把驱动句柄交给消费者端口。适配函数只转参数和错误码。

```c
static int store_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    return nd_eeprom_read(ctx, offset, buffer, (uint16_t)length) == 0 ? 0 : -1;
}
```

端口按消费者定义，绑定在组合根做一次。

## 7. 测试

`tests/unit/test_<chip>.c` 用 Unity 和假总线。

- 断言总线收到的寄存器地址、命令和数据序列。
- 断言越界、空指针、长度为 0 的拒绝路径。
- 不断言驱动内部结构。

host 测试的源文件加入 `tests/CMakeLists.txt` 的 `blade_infra` 或新的 driver 库。

## 8. 迁移对照

下表记录迁移来源。`infra/boards` 已按复制拆分到各 target 的 board 目录，源路径不再存在。

| 源文件 | 目标 |
|---|---|
| `infra/boards/stategrid_single_phase/eeprom.c` | `infra/drivers/storage/eeprom` |
| `infra/boards/stategrid_single_phase/ndv_Norflash.c` | `infra/drivers/storage/mx25l3206e` |
| `infra/boards/stategrid_single_phase/ndv_RN8209.c` | `infra/drivers/metering/rn8209` |
| `infra/boards/stategrid_single_phase/esam698_v2.c` | `infra/drivers/security/esam` |
| `infra/boards/stategrid_single_phase/relay.c` | `targets/<product>/board` |
| `infra/boards/stategrid_single_phase/uart_driver.c` | `infra/soc/v85xxp/v8500` |
| `infra/boards/stategrid_single_phase/iic_driver.c` | `infra/soc/v85xxp/v8500` |
| `infra/boards/stategrid_single_phase/spi_driver.c` | `infra/soc/v85xxp/v8500` |
| `*_pins.c/h`、`board_io.c/h` | `targets/<product>/board` |
