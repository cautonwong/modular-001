#!/usr/bin/env python3
"""Run the IDIS design-data validators when the internal docs are present."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MEMBERS = ROOT / "docs/idis/obis_members.csv"
SCRIPTS = ("validate_bluebook.py", "validate_objects.py", "validate_scripts.py")


def main():
    if not MEMBERS.is_file():
        print("skipped: docs/idis is not present (internal reference documents)")
        return 0
    for script in SCRIPTS:
        result = subprocess.run([sys.executable, str(ROOT / "tools" / script)],
                                cwd=ROOT)
        if result.returncode != 0:
            return result.returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
