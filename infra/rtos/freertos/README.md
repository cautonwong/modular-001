# infra/rtos/freertos

FreeRTOS 的具体实现/胶水目录，不是共享抽象层。

按 `docs/device-model.md`：

- 上游内核在 `infra/third_party/freertos/FreeRTOS-KernelV11.3.3/`。
- `FreeRTOSConfig.h`、任务/队列/工作队列和端口接线在选中的 product target。
- 这里只放可复用的具体实现；不定义 `nd_os_port_t`，不提供公共 adapter，不要求
  domain/application 使用统一 OS API。
