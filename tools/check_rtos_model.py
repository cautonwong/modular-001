#!/usr/bin/env python3
"""Freeze the FreeRTOS thread model recorded in the design doc.

The IDIS FreeRTOS target relies on a specific configuration: preemption,
no time slicing, no task notifications, no mutexes, no software timers, a
single-slot save queue and a worker one priority above the app task. The
model and its rationale live in docs/design/freertos-thread-model.md.
This checker fails when FreeRTOSConfig.h, freertos.c or the doc table drifts
from the recorded values, or when the record stops naming a frozen knob.

A new FreeRTOS product with different values must extend the expected
tables and the doc in the same change.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOC = ROOT / "docs" / "design" / "freertos-thread-model.md"

CONFIG_EXPECTED = {
    "configTICK_RATE_HZ": "100u",
    "configUSE_PREEMPTION": "1",
    "configUSE_TIME_SLICING": "0",
    "configMAX_PRIORITIES": "5",
    "configUSE_TASK_NOTIFICATIONS": "0",
    "configUSE_MUTEXES": "0",
    "configUSE_TIMERS": "0",
    "configTOTAL_HEAP_SIZE": "(12u * 1024u)",
    "configCHECK_FOR_STACK_OVERFLOW": "2",
    "configSUPPORT_STATIC_ALLOCATION": "0",
    "configUSE_TICKLESS_IDLE": "0",
    "configMAX_SYSCALL_INTERRUPT_PRIORITY": "0xC0u",
}
TASK_EXPECTED = {
    "APP_TASK_PRIORITY": "3u",
    "WORKER_TASK_PRIORITY": "(APP_TASK_PRIORITY + 1u)",
    "APP_TASK_STACK_WORDS": "768u",
    "SAVE_QUEUE_DEPTH": "1u",
}
WORKER_STACK_WORDS = "512"

DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)[ \t]+(.+?)[ \t]*$",
                    re.MULTILINE)
WORKER_TASK = re.compile(r'xTaskCreate\(worker_task,\s*"[^"]*",\s*(\w+)')
APP_TASK = re.compile(
    r'xTaskCreate\(app_task,\s*"[^"]*",\s*([^,]+),\s*[^,]+,\s*([^,]+)')

SELF_CHECK_CONFIG = """
#define configTICK_RATE_HZ 100u
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 0
#define configMAX_PRIORITIES 5
#define configUSE_TASK_NOTIFICATIONS 0
#define configUSE_MUTEXES 0
#define configUSE_TIMERS 0
#define configTOTAL_HEAP_SIZE (12u * 1024u)
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configSUPPORT_STATIC_ALLOCATION 0
#define configUSE_TICKLESS_IDLE 0
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 0xC0u
"""
SELF_CHECK_TASKS = """
#define APP_TASK_PRIORITY 3u
#define WORKER_TASK_PRIORITY (APP_TASK_PRIORITY + 1u)
#define APP_TASK_STACK_WORDS 768u
#define SAVE_QUEUE_DEPTH 1u
xTaskCreate(worker_task, "work", 512, NULL, WORKER_TASK_PRIORITY, NULL);
"""
SELF_CHECK_APP = """
xTaskCreate(app_task, "app", APP_TASK_STACK_WORDS, NULL,
            APP_TASK_PRIORITY, &app_task_handle);
"""
SELF_CHECK_DOC = "\n".join(
    [f"| `{name}` | {value} | reason |"
     for name, value in CONFIG_EXPECTED.items()]
    + [f"`{name} {value}`" for name, value in TASK_EXPECTED.items()]
    + ["worker 栈 512 words"])


def macros(text: str) -> dict[str, str]:
    found = {}
    for match in DEFINE.finditer(text):
        found[match.group(1)] = match.group(2).split("//", 1)[0].strip()
    return found


def app_task_failures(where: str, text: str) -> list[str]:
    """The app task must pass the frozen stack and priority macros."""
    match = APP_TASK.search(text)
    if match is None:
        return [f"{where}: app xTaskCreate not found"]
    failures = []
    stack, priority = (group.strip() for group in match.groups())
    if stack != "APP_TASK_STACK_WORDS":
        failures.append(f"{where}: app stack is {stack}, "
                        "expected APP_TASK_STACK_WORDS")
    if priority != "APP_TASK_PRIORITY":
        failures.append(f"{where}: app priority is {priority}, "
                        "expected APP_TASK_PRIORITY")
    return failures


def doc_failures(text: str) -> list[str]:
    """Every frozen knob must be recorded with its frozen value."""
    failures = []
    for name, value in CONFIG_EXPECTED.items():
        row = re.compile(r"^\|\s*`" + re.escape(name) + r"`\s*\|\s*"
                         + re.escape(value) + r"\s*\|", re.MULTILINE)
        if not row.search(text):
            failures.append(f"table row for {name} is not {value!r}")
    for name, value in TASK_EXPECTED.items():
        record = re.compile(re.escape(name) + r"[\s`]*" + re.escape(value)
                            + r"(?![0-9A-Za-z_])")
        if not record.search(text):
            failures.append(f"{name} is not recorded as {value!r}")
    if "512 words" not in text:
        failures.append("the worker stack is not recorded as 512 words")
    return failures


def compare(where: str, found: dict[str, str],
            expected: dict[str, str]) -> list[str]:
    failures = []
    for name, value in expected.items():
        if name not in found:
            failures.append(f"{where}: missing {name}")
        elif found[name] != value:
            failures.append(
                f"{where}: {name} is {found[name]!r}, expected {value!r}")
    return failures


def target_dirs() -> list[Path]:
    base = ROOT / "targets"
    if not base.is_dir():
        return []
    return sorted(path for path in base.iterdir()
                  if path.is_dir() and path.name.endswith("_freertos"))


def check_target(target: Path) -> list[str]:
    failures = []
    app = target / "app"
    config = app / "FreeRTOSConfig.h"
    tasks = app / "freertos.c"
    rel = target.relative_to(ROOT).as_posix()
    if not config.is_file():
        return [f"{rel}: missing app/FreeRTOSConfig.h"]
    if not tasks.is_file():
        return [f"{rel}: missing app/freertos.c"]

    failures += compare(f"{rel}/FreeRTOSConfig.h",
                        macros(config.read_text(encoding="utf-8")),
                        CONFIG_EXPECTED)
    task_text = tasks.read_text(encoding="utf-8")
    failures += compare(f"{rel}/freertos.c", macros(task_text), TASK_EXPECTED)
    failures += app_task_failures(f"{rel}/freertos.c", task_text)
    for name in ("xTaskCreate(app_task", "xTaskCreate(worker_task"):
        if name not in task_text:
            failures.append(f"{rel}/freertos.c: missing {name}")
    match = WORKER_TASK.search(task_text)
    if match is None:
        failures.append(f"{rel}/freertos.c: worker xTaskCreate not found")
    elif match.group(1) != WORKER_STACK_WORDS:
        failures.append(
            f"{rel}/freertos.c: worker stack {match.group(1)}, "
            f"expected {WORKER_STACK_WORDS}")
    return failures


def check_doc() -> list[str]:
    if not DOC.is_file():
        return [f"{DOC.relative_to(ROOT)}: missing thread-model record"]
    where = DOC.relative_to(ROOT).as_posix()
    return [f"{where}: {failure}"
            for failure in doc_failures(DOC.read_text(encoding="utf-8"))]


def self_check() -> bool:
    config = macros(SELF_CHECK_CONFIG)
    tasks = macros(SELF_CHECK_TASKS)
    if compare("probe", config, CONFIG_EXPECTED):
        print("rtos model self-check: valid config rejected")
        return False
    if compare("probe", tasks, TASK_EXPECTED):
        print("rtos model self-check: valid task constants rejected")
        return False
    if not compare("probe", {}, {"configUSE_PREEMPTION": "1"}):
        print("rtos model self-check: missing macro accepted")
        return False
    broken = dict(config)
    broken["configUSE_TIME_SLICING"] = "1"
    if not compare("probe", broken, {"configUSE_TIME_SLICING": "0"}):
        print("rtos model self-check: wrong value accepted")
        return False
    if doc_failures(SELF_CHECK_DOC):
        print("rtos model self-check: the valid doc record was rejected")
        return False
    stale = SELF_CHECK_DOC.replace("| `configUSE_TIME_SLICING` | 0 |",
                                   "| `configUSE_TIME_SLICING` | 1 |")
    if stale == SELF_CHECK_DOC or not doc_failures(stale):
        print("rtos model self-check: a doc table value change was accepted")
        return False
    drift = SELF_CHECK_DOC.replace("`APP_TASK_PRIORITY 3u`",
                                   "`APP_TASK_PRIORITY 4u`")
    if drift == SELF_CHECK_DOC or not doc_failures(drift):
        print("rtos model self-check: a doc task constant change was accepted")
        return False
    if app_task_failures("probe", SELF_CHECK_APP):
        print("rtos model self-check: the valid app xTaskCreate was rejected")
        return False
    wrong_stack = SELF_CHECK_APP.replace("APP_TASK_STACK_WORDS",
                                         "APP_STACK_BYTES")
    if not app_task_failures("probe", wrong_stack):
        print("rtos model self-check: an app stack change was accepted")
        return False
    wrong_priority = SELF_CHECK_APP.replace("APP_TASK_PRIORITY",
                                            "WORKER_TASK_PRIORITY")
    if not app_task_failures("probe", wrong_priority):
        print("rtos model self-check: an app priority change was accepted")
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()

    if not self_check():
        return 1
    if args.self_check:
        print("rtos model self-check ok")
        return 0

    failures = check_doc()
    targets = target_dirs()
    for target in targets:
        failures += check_target(target)
    for failure in failures:
        print(f"FAIL {failure}")
    if failures:
        return 1
    print(f"rtos model ok ({len(targets)} FreeRTOS target(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
