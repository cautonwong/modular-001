#!/usr/bin/env python3
"""Check product-local RTOS target copies.

Under docs/device-model.md a target that switches RTOS is copied and owns
FreeRTOSConfig.h and freertos.c. The FreeRTOS kernel itself stays under
infra/third_party; shared domain/application/components must not include it.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
SHARED = ("domain", "application", "components")
FREERTOS_HEADERS = {
    "FreeRTOS.h", "task.h", "queue.h", "semphr.h", "timers.h",
    "event_groups.h", "stream_buffer.h", "portmacro.h", "FreeRTOSConfig.h",
}


def target_dirs():
    base = os.path.join(ROOT, "targets")
    if not os.path.isdir(base):
        return []
    return [
        os.path.join(base, name)
        for name in sorted(os.listdir(base))
        if name.endswith("_freertos") and os.path.isdir(os.path.join(base, name))
    ]


def main():
    failures = []
    for target in target_dirs():
        app = os.path.join(target, "app")
        for required in ("freertos.c", "FreeRTOSConfig.h"):
            if not os.path.isfile(os.path.join(app, required)):
                failures.append(f"{os.path.relpath(app, ROOT)} missing {required}")
        for dirpath, _dirs, files in os.walk(target):
            for name in files:
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, ROOT)
                if "FreeRTOS-Kernel" in rel:
                    failures.append(f"{rel}: kernel must stay in infra/third_party")
                if not name.endswith((".c", ".h")):
                    continue
                with open(path, encoding="utf-8", errors="replace") as handle:
                    for line in handle:
                        match = INCLUDE.match(line)
                        if match and "targets/" in match.group(1) \
                                and not match.group(1).startswith(
                                    os.path.relpath(target, ROOT) + "/"):
                            failures.append(
                                f"{rel} includes another target: {match.group(1)}")
    for layer in SHARED:
        for dirpath, _dirs, files in os.walk(os.path.join(ROOT, layer)):
            for name in files:
                if not name.endswith((".c", ".h")):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, ROOT)
                with open(path, encoding="utf-8", errors="replace") as handle:
                    for line in handle:
                        match = INCLUDE.match(line)
                        if match and os.path.basename(match.group(1)) in FREERTOS_HEADERS:
                            failures.append(
                                f"{rel} includes FreeRTOS header {match.group(1)}")
    for failure in failures:
        print(f"FAIL {failure}")
    print(f"rtos targets ok ({len(target_dirs())} copied target(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
