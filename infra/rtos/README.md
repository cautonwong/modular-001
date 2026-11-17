# infra/rtos

RTOS 适配层目录。每个 RTOS 一个子目录，绑定消费者定义的 OS 端口。

外部 RTOS 内核放 `infra/third_party/<vendor>/<rtos>`。`nidavellir` 已删除，不要在这里重新引入自研内核依赖。见 `docs/platform-layout.md`。
