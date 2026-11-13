# infra/rtos

RTOS 适配层目录。每个 RTOS 一个子目录，绑定消费者定义的 OS 端口。

外部 RTOS 内核放 `infra/third_party/<vendor>/<rtos>`。`nidavellir` 是待删除的 legacy 内核，不要在这里新增依赖。见 `docs/platform-layout.md`。
