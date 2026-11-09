#!/usr/bin/env python3
"""Fail the build when a layer imports the wrong way.

Rules live in docs/architecture.md. New code uses repo-root-relative includes
such as "domain/metering/metering.h", so the first path segment is the layer.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYERS = ("domain", "application", "infra", "targets")
ALLOWED = {
    "domain": {"domain"},
    "application": {"domain", "application"},
    "infra": {"domain", "application", "infra"},
    "targets": {"domain", "application", "infra", "targets"},
}
SYSTEM_HEADERS = {"stdint.h", "stddef.h", "stdbool.h", "string.h"}
FORBIDDEN_ALLOC = re.compile(r"\b(malloc|calloc|realloc|free|strdup)\s*\(")
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')

# Not yet split into bounded contexts. Remove entries as each module migrates.
STAGING = (os.path.join("domain", "meterbase"),)


def layer_of(rel_path):
    head = rel_path.split(os.sep, 1)[0]
    return head if head in LAYERS else None


def source_files(layer):
    for dirpath, _dirs, files in os.walk(os.path.join(ROOT, layer)):
        rel_dir = os.path.relpath(dirpath, ROOT)
        if any(rel_dir == stage or rel_dir.startswith(stage + os.sep) for stage in STAGING):
            continue
        for name in files:
            if name.endswith((".c", ".h")):
                yield os.path.join(dirpath, name)


def main():
    errors = []
    for layer in LAYERS:
        for path in source_files(layer):
            rel = os.path.relpath(path, ROOT)
            with open(path, encoding="utf-8", errors="replace") as handle:
                for number, line in enumerate(handle, 1):
                    match = INCLUDE.match(line)
                    if match:
                        target = match.group(1)
                        first = target.split("/", 1)[0]
                        if first in LAYERS:
                            if first not in ALLOWED[layer]:
                                errors.append(
                                    f"{rel}:{number}: {layer} may not include {target}"
                                )
                        elif layer == "domain":
                            if target not in SYSTEM_HEADERS:
                                errors.append(
                                    f"{rel}:{number}: domain may not include {target}"
                                )
                    if layer == "domain" and FORBIDDEN_ALLOC.search(line):
                        errors.append(
                            f"{rel}:{number}: domain may not allocate directly"
                        )
    for error in errors:
        print(error)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
