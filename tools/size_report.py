#!/usr/bin/env python3
"""Print artifact sizes and section totals for an ELF produced by the ARM build."""
import os
import subprocess
import sys


def section_totals(nm, elf):
    output = subprocess.check_output(
        [nm, "--print-size", "--radix=d", elf], text=True)
    text = rodata = data = bss = 0
    for line in output.splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        try:
            size = int(parts[1])
        except ValueError:
            continue
        kind = parts[2]
        if kind in ("T", "t"):
            text += size
        elif kind in ("R", "r"):
            rodata += size
        elif kind in ("D", "d"):
            data += size
        elif kind in ("B", "b"):
            bss += size
    return text, rodata, data, bss


def main():
    if len(sys.argv) < 3:
        print("usage: size_report.py <nm> <artifact> [artifact...]")
        return 2
    nm = sys.argv[1]
    artifacts = sys.argv[2:]
    print("== artifacts ==")
    for path in artifacts:
        print(f"  {path}  {os.path.getsize(path)} bytes")
    elf = artifacts[0]
    text, rodata, data, bss = section_totals(nm, elf)
    print(f"  text={text}  rodata={rodata}  data={data}  bss={bss}  "
          f"total={text + rodata + data + bss} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
