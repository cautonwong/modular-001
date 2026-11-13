#!/usr/bin/env python3
"""Compile-check the IAR stategrid project with arm-none-eabi-gcc.

Parses the .ewp for sources, include paths and defines, then reports pass and
fail counts with failures grouped by their first error. Missing source files
are reported separately, which is how moved files surface as build-file work.
"""
import collections
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EWP = os.path.join(ROOT, "targets/stategrid_meter/iar for arm/VG_DTZY.ewp")
PROJ_DIR = os.path.dirname(EWP)

# Symbols the IAR runtime and the C library provide at the final link. Any
# other undefined name means the IAR source list is missing a file.
EXTERNAL_SYMBOLS = {
    "CSTACK$$Base", "__CALL",
    "abs", "asin", "div", "memcmp", "memcpy", "memset", "mktime",
}


def external_symbol(name):
    return name in EXTERNAL_SYMBOLS or name.startswith("__aeabi_")


def resolve(path):
    return os.path.normpath(path.replace("$PROJ_DIR$", PROJ_DIR).replace("\\", "/"))


def first_configuration(tree):
    return tree.getroot().find("configuration")


def collect_option(configuration, option_name):
    for option in configuration.iter("option"):
        if option.findtext("name") == option_name:
            return [state.text for state in option.findall("state") if state.text]
    return []


def collect_sources(tree):
    sources = []
    for name in tree.getroot().iter("name"):
        if name.text and name.text.endswith(".c"):
            sources.append(resolve(name.text))
    return sources


def classify(stderr):
    missing = re.search(r"fatal error: ([^:]+): No such file", stderr)
    if missing:
        return "missing header: " + missing.group(1).strip()
    fatal = re.search(r"fatal error: (.*)", stderr)
    if fatal:
        return "fatal: " + fatal.group(1).strip()
    error = re.search(r"error: (.*)", stderr)
    if error:
        return "error: " + error.group(1).strip()
    return "error: unknown"


def main():
    tree = ET.parse(EWP)
    configuration = first_configuration(tree)
    includes = [resolve(path) for path in collect_option(configuration, "CCIncludePath2")]
    defines = collect_option(configuration, "CCDefines")
    sources = collect_sources(tree)

    # -fpermissive keeps GCC 14 from promoting the old code's implicit
    # declarations to errors; GCC 10 only warned.
    flags = ["-std=gnu11", "-mcpu=cortex-m0", "-mthumb", "-ffreestanding", "-w",
             "-fpermissive", "-I", ROOT]
    for path in includes:
        flags += ["-I", path]
    for define in defines:
        flags.append("-D" + define)

    ok = 0
    failures = collections.OrderedDict()
    missing_files = []
    build_dir = tempfile.mkdtemp(prefix="blade-baseline-")
    objects = []
    for index, source in enumerate(sources):
        rel = os.path.relpath(source, ROOT)
        if not os.path.isfile(source):
            missing_files.append(rel)
            continue
        obj = os.path.join(build_dir, f"{index:03d}.o")
        try:
            result = subprocess.run(
                ["arm-none-eabi-gcc", *flags, "-c", source, "-o", obj],
                capture_output=True,
                text=True,
                errors="replace",
                timeout=30,
            )
        except subprocess.TimeoutExpired:
            failures.setdefault("timeout", []).append(rel)
            continue
        if result.returncode == 0:
            ok += 1
            objects.append(obj)
        else:
            failures.setdefault(classify(result.stderr), []).append(rel)

    undefined = []
    link_error = None
    if not failures and not missing_files:
        combined = os.path.join(build_dir, "all.o")
        link = subprocess.run(
            ["arm-none-eabi-ld", "-r", "-o", combined, *objects],
            capture_output=True,
            text=True,
            errors="replace",
        )
        if link.returncode != 0:
            link_error = link.stderr.strip()
        else:
            nm = subprocess.run(
                ["arm-none-eabi-nm", "-u", combined],
                capture_output=True,
                text=True,
            )
            if nm.returncode != 0:
                link_error = nm.stderr.strip()
            else:
                undefined = [line.split()[-1] for line in nm.stdout.splitlines()
                             if line.split()]
    unexpected = [name for name in undefined if not external_symbol(name)]
    shutil.rmtree(build_dir, ignore_errors=True)

    print(f"sources={len(sources)} ok={ok} failed={sum(len(v) for v in failures.values())} "
          f"missing_files={len(missing_files)}")
    if failures or missing_files:
        print("link: skipped (compile failures)")
    elif link_error:
        print(f"link: failed\n{link_error}")
    else:
        print(f"link_undefined={len(unexpected)}")
        for name in unexpected[:20]:
            print("  " + name)
    for reason, files in failures.items():
        print(f"\n[{len(files)}] {reason}")
        for rel in files[:6]:
            print("  " + rel)
        if len(files) > 6:
            print(f"  ... and {len(files) - 6} more")
    if missing_files:
        print(f"\n[{len(missing_files)}] missing source files")
        for rel in missing_files[:20]:
            print("  " + rel)
    return 0 if not (failures or missing_files or link_error or unexpected) else 1


if __name__ == "__main__":
    sys.exit(main())
