#!/usr/bin/env python3
"""Check that nd_idis_ids.h is internally consistent with the OBIS inventory.

The header is hand-curated, so this gate catches three failure modes:
duplicate constant names, OBIS tuples that are not in the IDIS inventory,
and ND_OBIS_* identifiers used by the target but not defined here.
"""
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = os.path.join(
    ROOT, "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h")
INVENTORY = os.path.join(ROOT, "tests/gen/all_obis.csv")
TARGET = os.path.join(ROOT, "targets/idis_ip_meter")

# Computed values and document wildcards that are not concrete inventory rows.
ALLOWED = {
    "1-0:16.7.0.255": "computed net active power, not an object-model row",
    "0-11:10.0.110.255": "0-b M-Bus profile control wildcard",
}


def render(tuple_values):
    a, b, c, d, e, f = tuple_values
    return f"{a}-{b}:{c}.{d}.{e}.{f}"


def main():
    text = open(HEADER, encoding="utf-8").read()
    names = re.findall(r"static const nd_obis_t\s+(\w+)", text)
    names += [n for n in re.findall(r"ND_OBIS_CONST\(\s*(\w+)", text)
              if n != "name"]
    duplicates = sorted({name for name in names if names.count(name) > 1})
    tuples = re.findall(
        r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}",
        text)
    tuples += re.findall(
        r"ND_OBIS_CONST\([^,]+,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,"
        r"\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)", text)
    known = {row["obis"] for row in
             csv.DictReader(open(INVENTORY, encoding="utf-8"))}
    unknown = []
    for values in tuples:
        obis = render(values)
        if obis not in known and obis not in ALLOWED:
            unknown.append(obis)

    used = set()
    for dirpath, _, filenames in os.walk(TARGET):
        for filename in filenames:
            if not filename.endswith((".c", ".h")):
                continue
            path = os.path.join(dirpath, filename)
            used.update(re.findall(r"\bND_OBIS_[A-Z0-9_]+",
                                   open(path, encoding="utf-8").read()))
    used.discard("ND_OBIS_CONST")
    undefined = sorted(used - set(names))

    failed = False
    if duplicates:
        print("duplicate constants: " + ", ".join(duplicates))
        failed = True
    if unknown:
        print("OBIS not in inventory: " + ", ".join(sorted(set(unknown))))
        failed = True
    if undefined:
        print("used but undefined: " + ", ".join(undefined))
        failed = True
    if failed:
        return 1
    print(f"nd_idis_ids ok: {len(names)} constants, {len(tuples)} OBIS tuples")
    return 0


if __name__ == "__main__":
    sys.exit(main())
