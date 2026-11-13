#!/usr/bin/env python3
"""Run the IDIS host audit and enforce its known failure budget.

The audit table still contains four invalid OBIS entries from the source PDF.
Every other GET must succeed.  The only method failures must be those four
invalid entries plus the three crypto methods that need a real crypto port.
The failure identities are checked, not just the totals.
"""
import os
import re
import shutil
import subprocess
import sys

INVALID_OBIS = {
    "0-1:24.2.5.256",
    "0-1:24.2.6.256",
    "0-1:24.2.7.256",
    "0-1:24.2.8.256",
}
EXPECTED_AUDIT_FAILURES = {
    (obis, str(attribute))
    for obis in INVALID_OBIS
    for attribute in range(1, 6)
}
EXPECTED_METHOD_FAILURES = {
    *((obis, "1") for obis in INVALID_OBIS),
    ("0-0:40.0.0.255", "1"),
    ("0-0:43.0.0.255", "4"),
    ("0-0:43.0.0.255", "5"),
}


def main():
    if len(sys.argv) != 2:
        print("usage: check_idis_audit.py <idis_host_runner>")
        return 2
    runner = sys.argv[1]
    work = os.path.join(os.path.dirname(runner), "audit")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    output = subprocess.check_output(
        [runner, "--backup-files", work, "--audit"], text=True)

    audit_failures = set()
    method_failures = set()
    summary = {}
    for line in output.splitlines():
        match = re.match(r"AUDIT (\d+) (\S+) (\d+) (-?\d+)$", line)
        if match and match.group(4) != "0":
            audit_failures.add((match.group(2), match.group(3)))
        match = re.match(r"METHOD (\d+) (\S+) (\d+) (-?\d+)$", line)
        if match and match.group(4) != "0":
            method_failures.add((match.group(2), match.group(3)))
        match = re.match(r"([A-Z]+) SUMMARY (.*)", line)
        if match:
            for item in match.group(2).split():
                key, _, value = item.partition("=")
                summary[f"{match.group(1)}.{key}"] = int(value)

    failed = False
    if audit_failures != EXPECTED_AUDIT_FAILURES:
        print("unexpected audit failures: "
              + ", ".join(f"{obis}/{attr}"
                          for obis, attr in sorted(audit_failures)))
        failed = True
    if method_failures != EXPECTED_METHOD_FAILURES:
        print("unexpected method failures: "
              + ", ".join(f"{obis}/{method}"
                          for obis, method in sorted(method_failures)))
        failed = True
    if summary.get("SCRIPT.fail") != 0:
        print(f"script failures: {summary.get('SCRIPT.fail')}")
        failed = True
    if failed:
        return 1
    print("audit budget ok: "
          f"attributes {summary.get('AUDIT.ok')}/{summary.get('AUDIT.total')}, "
          f"methods {summary.get('METHOD.ok')}/{summary.get('METHOD.total')}, "
          f"scripts {summary.get('SCRIPT.ok')}/{summary.get('SCRIPT.total')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
