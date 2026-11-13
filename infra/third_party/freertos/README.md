# FreeRTOS third-party

Source: `/home/edge/FreeRTOS-KernelV11.3.3`, version 11.3.3.

The kernel is an external checkout, not committed. Place FreeRTOS 11.3.3
here, or point `FREERTOS_KERNEL_DIR` at another checkout. The target build
needs the root sources, `include/`, `portable/GCC/ARM_CM0/` and
`portable/MemMang/heap_1.c`; keep `LICENSE.md` beside them.

Do not edit these files. Product configuration and task wiring live in the
copied target, for example `targets/idis_ip_meter_freertos/app/`.
