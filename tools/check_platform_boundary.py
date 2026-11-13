#!/usr/bin/env python3
"""Enforce the platform boundary.

Vendor SoC HAL and RTOS code may live unchanged under infra/soc, infra/drivers,
infra/boards or infra/third_party and include their own headers. RTOS-specific
configuration and glue are target-local under the device-model policy. The product target may include and call them to
adapt component ports. Shared application, domain and component code must stay
platform-free, and only target/infra implementation code may call RTOS
scheduling APIs directly. Known legacy kernel-header debt is tracked in
tools/platform_boundary_baseline.txt; new violations fail."""
import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYERS = ("application", "domain", "components", "infra")
PLATFORM_PREFIXES = (
    "infra/soc/",
    "infra/drivers/",
    "infra/boards/",
    "infra/rtos/",
    "infra/third_party/",
    "targets/",
    "zephyr/",
    "kernel/",
    "nidavellir/",
)
PLATFORM_HEADERS = {
    "zephyr.h",
    "cpu_surport.h",
    "target.h",
    "lib_gpio.h",
    "lib_uart.h",
    "lib_clk.h",
    "rn8318.h",
    "rn8209.h",
    "nidavellir.h",
    "FreeRTOS.h",
    "task.h",
    "queue.h",
    "semphr.h",
    "timers.h",
    "event_groups.h",
    "stream_buffer.h",
    "portmacro.h",
    "FreeRTOSConfig.h",
}
# Platform implementations are allowed to include their own SDK headers.
PLATFORM_IMPLEMENTATION = (
    os.path.join("infra", "soc"),
    os.path.join("infra", "drivers"),
    os.path.join("infra", "boards"),
    os.path.join("infra", "rtos"),
    os.path.join("infra", "third_party"),
    os.path.join("infra", "kernel"),
    os.path.join("infra", "security"),
    os.path.join("infra", "boot"),
)
BASELINE = os.path.join(ROOT, "tools", "platform_boundary_baseline.txt")
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
RTOS_CALL = re.compile(
    r"\b(ndv_timer_\w+|ndv_tasklet\w*|send_mesg|recv_mesg|"
    r"tasklet_create|tasklet_start|k_timer\w*|k_thread\w*|k_mutex\w*|"
    r"k_msgq\w*|xTaskCreate\w*|vTaskDelay\w*|vTaskStartScheduler|"
    r"xQueue\w*|xSemaphore\w*|xTimer\w*|portYIELD\w*)\s*\("
)


def is_platform(include):
    return include.startswith(PLATFORM_PREFIXES) or include in PLATFORM_HEADERS


def in_paths(path, roots):
    return any(path == root or path.startswith(root + os.sep) for root in roots)


def load_baseline():
    counts = collections.Counter()
    if not os.path.exists(BASELINE):
        return counts
    with open(BASELINE, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if line and not line.startswith("#"):
                counts[line] += 1
    return counts


def main():
    baseline = load_baseline()
    failed = []
    baselined = 0
    for layer in LAYERS:
        for dirpath, _dirs, files in os.walk(os.path.join(ROOT, layer)):
            rel_dir = os.path.relpath(dirpath, ROOT)
            if in_paths(rel_dir, PLATFORM_IMPLEMENTATION):
                continue
            for name in files:
                if not name.endswith((".c", ".h")):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, ROOT)
                with open(path, encoding="utf-8", errors="replace") as handle:
                    for line in handle:
                        match = INCLUDE.match(line)
                        rtos = RTOS_CALL.search(line)
                        if (match and is_platform(match.group(1))) or rtos:
                            what = match.group(1) if match and is_platform(match.group(1)) else rtos.group(1)
                            key = f"{rel} {what}"
                            if baseline[key] > 0:
                                baseline[key] -= 1
                                baselined += 1
                            else:
                                failed.append(key)
    for entry in failed:
        print(f"FAIL {entry}")
    stale = sum(baseline.values())
    print(f"platform boundary ok ({baselined} baselined, {stale} stale)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
