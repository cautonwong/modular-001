# 端口绑定模板

端口表达业务需要的一个动作。它不是设备驱动模型。仓库里没有 device ops、驱动注册表或运行期设备工厂，也不为"一个版本服务多个产品"设计抽象。一个 board 服务一个产品，产品在构建期选定，组合根直接调用具体实现。

## 1. 什么时候需要端口

domain、application 或 components 需要一项平台能力，但不能 include 具体实现时。

例子。背光开关、继电器反馈、参数读写、存储读写、上报发送。

不需要端口的情况。能力只在一个 target 内部使用。那就直接调用，不要为单次使用造接口。

## 2. 生成器

```sh
python3 tools/new_port.py --capability backlight \
  --action "set(bool enable)" --action "read(uint8_t *buffer, uint16_t length)"
```

生成 `application/ports/backlight.h`，只包含能力结构体和动作函数指针。动作按 `name(type arg, ...)` 传入，返回值固定为 `int`。生成后写 target 适配表、在组合根把表传给消费者的构造函数、补假端口测试。生成器不产生 `.c`、绑定函数或全局 `bound`。

## 3. 端口头文件

位置 `application/ports/<能力>.h`。

```c
#ifndef ND_APPLICATION_PORTS_EXAMPLE_H
#define ND_APPLICATION_PORTS_EXAMPLE_H

#include <stdbool.h>
#include <stdint.h>

/* 业务需要的动作，不出现设备类型、寄存器名或厂商头。 */
typedef struct nd_example_port {
    void *ctx;
    int (*read)(void *ctx, uint8_t *buffer, uint16_t length);
    int (*write)(void *ctx, const uint8_t *buffer, uint16_t length);
} nd_example_port_t;

#endif
```

消费者在构造函数里接收 `const nd_example_port_t *`，存进自己的不透明结构体，调用 `port->read(port->ctx, ...)`。没有绑定函数，也没有文件级可变指针。头文件不声明包装函数。

规则。

- `ctx` 不透明。头文件不解释它指向什么。
- 函数按业务动作命名。不要 `dev_ops`、`ioctl`、`board_ops`。
- 返回值用 `int`，0 表示成功，负数表示失败。
- 头文件不 include `targets/` 或 `infra/soc`。
- 端口按业务能力增长，不按板子数量增长。

## 4. target 侧适配

位置 `targets/<t>/app/<能力>_adapter.c`。

```c
#include "application/ports/example.h"
#include "targets/<t>/board/board_io.h"

static int example_read(void *ctx, uint8_t *buffer, uint16_t length)
{
    (void)ctx;
    return board_example_read(buffer, length) == 0 ? 0 : -1;
}

static int example_write(void *ctx, const uint8_t *buffer, uint16_t length)
{
    (void)ctx;
    return board_example_write(buffer, length) == 0 ? 0 : -1;
}

const nd_example_port_t *target_example_port(void)
{
    static const nd_example_port_t port = {
        .ctx = NULL,
        .read = example_read,
        .write = example_write,
    };
    return &port;
}
```

适配函数只做参数转换和错误码转换。业务判断留在 domain。

## 5. 组合根

组合根把适配表传给消费者的构造函数，消费者自己保存指针。

```c
consumer_t *consumer = consumer_create(alloc, target_example_port());
```

端口在构造时注入，先于第一次使用。host 测试构造时传假端口即可，不需要运行期注入框架。

## 6. 测试

- 端口消费者把假端口传给构造函数做单元测试，放在 `tests/unit/test_<模块>.c`。
- 适配层由 target 验收覆盖，见 `docs/guides/acceptance-case.md`。
- 断言可观察的返回值，不断言内部调用顺序。

## 7. 检查清单

- [ ] 端口只表达业务动作，没有设备类型或厂商头。
- [ ] `ctx` 不透明，头文件独立可编译。
- [ ] 端口由组合根构造注入，先于第一次使用，没有绑定全局。
- [ ] 适配函数不含业务判断。
- [ ] 有假端口的单元测试。
