#!/usr/bin/env python3
"""Phase 0 WP wiring probe: judge toy goals from Frama-C's WP report JSON.

The false contract must be judged `invalid` and the true contract `valid`.
`timeout`, `unknown`, a missing goal, or a mismatched file fails. The probe is
green only when at least one pinned solver passes both cases; that is the
accept rule from docs/design/wp-decision.md. Run the probe with each pinned
solver and read the per-goal verdicts.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WP_DIR = ROOT / "formal" / "frama-c" / "wp"

# (toy file, ACSL property name, required verdict)
CASES = (
    ("toy_invalid.c", "toy_invalid_ensures", "invalid"),
    ("toy_valid.c", "toy_valid_ensures", "valid"),
)


def select_goals(report: list[dict], property_name: str) -> list[dict]:
    goals = []
    for goal in report:
        if goal.get("property") != property_name:
            continue
        name = str(goal.get("goal", ""))
        if name == property_name or name.endswith("_" + property_name):
            goals.append(goal)
    return goals


def check_report(report: list[dict], toy: str, property_name: str,
                 expected: str) -> list[str]:
    goals = select_goals(report, property_name)
    if not goals:
        return [f"report has no goal named {property_name}"]
    errors: list[str] = []
    for goal in goals:
        name = goal.get("goal")
        if Path(str(goal.get("file", ""))).name != toy:
            errors.append(
                f"{name}: file is {goal.get('file')!r}, expected {toy}")
        if goal.get("verdict") != expected:
            errors.append(
                f"{name}: verdict is {goal.get('verdict')!r}, "
                f"expected {expected!r}")
    return errors


def prover_summary(goal: dict) -> str:
    provers = goal.get("provers") or []
    if not provers:
        return "no prover recorded"
    return ", ".join(
        f"{prover.get('prover')} success={prover.get('success')} "
        f"time={prover.get('time')}"
        for prover in provers)


def run_solver(frama_c: str, solver: str, toy: str,
               timeout: int) -> tuple[list[dict] | None, str]:
    with tempfile.TemporaryDirectory(prefix="check-wp-") as workdir:
        report_path = Path(workdir) / "report.json"
        command = [
            frama_c, "-wp",
            f"-wp-prover={solver}",
            f"-wp-timeout={timeout}",
            "-wp-cache=none",
            f"-wp-report-json={report_path}",
            str(WP_DIR / toy),
        ]
        try:
            result = subprocess.run(command, cwd=workdir, capture_output=True,
                                    text=True)
        except FileNotFoundError:
            return None, f"{frama_c} not found"
        if result.returncode != 0:
            tail = "\n".join(
                (result.stdout + result.stderr).splitlines()[-15:])
            return None, f"frama-c exited {result.returncode}\n{tail}"
        if not report_path.is_file():
            return None, "frama-c wrote no report JSON"
        try:
            report = json.loads(report_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as error:
            return None, f"report JSON is not valid: {error}"
        if not isinstance(report, list):
            return None, "report JSON is not a list of goals"
        return report, ""


def self_check() -> int:
    def goal(name: str, verdict: str) -> dict:
        return {
            "goal": f"typed_{name}",
            "property": name,
            "file": f"formal/frama-c/wp/{name.rsplit('_', 1)[0]}.c",
            "verdict": verdict,
            "provers": [{"prover": "Z3:4.8.12", "time": 0.01, "success": 1}],
        }

    valid = [goal("toy_valid_ensures", "valid")]
    invalid = [goal("toy_invalid_ensures", "invalid")]
    timeout = [goal("toy_invalid_ensures", "timeout")]
    unknown = [goal("toy_invalid_ensures", "unknown")]
    wrong_name = [goal("other_ensures", "invalid")]

    checks = (
        ("valid report passes", check_report(
            valid, "toy_valid.c", "toy_valid_ensures", "valid") == []),
        ("invalid report passes", check_report(
            invalid, "toy_invalid.c", "toy_invalid_ensures", "invalid") == []),
        ("timeout is rejected", check_report(
            timeout, "toy_invalid.c", "toy_invalid_ensures", "invalid") != []),
        ("unknown is rejected", check_report(
            unknown, "toy_invalid.c", "toy_invalid_ensures", "invalid") != []),
        ("wrong goal name is rejected", check_report(
            wrong_name, "toy_invalid.c", "toy_invalid_ensures",
            "invalid") != []),
        ("swapped verdict is rejected", check_report(
            valid, "toy_valid.c", "toy_valid_ensures", "invalid") != []),
    )
    errors = [label for label, ok in checks if not ok]
    if errors:
        print("\n".join(errors))
        print("wp report self-check: FAILED")
        return 1
    print("wp report self-check ok")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frama-c",
                        default=os.environ.get("FRAMA_C", "frama-c"),
                        help="Frama-C binary (default: $FRAMA_C or frama-c)")
    parser.add_argument("--solver", action="append", default=[],
                        help="pinned prover name; repeatable (default: z3)")
    parser.add_argument("--timeout", type=int, default=5,
                        help="per-goal WP timeout in seconds (default: 5)")
    parser.add_argument("--self-check", action="store_true",
                        help="check the report judgement on synthetic goals")
    args = parser.parse_args()

    if args.self_check:
        return self_check()

    solvers = args.solver or ["z3"]
    accepted = False
    for solver in solvers:
        solver_ok = True
        for toy, property_name, expected in CASES:
            report, error = run_solver(args.frama_c, solver, toy,
                                       args.timeout)
            if report is None:
                print(f"{solver} {toy}: run failed: {error}")
                solver_ok = False
                continue
            for goal in select_goals(report, property_name):
                print(f"{solver} {toy}: goal={goal.get('goal')} "
                      f"verdict={goal.get('verdict')} | {prover_summary(goal)}")
            errors = check_report(report, toy, property_name, expected)
            for error in errors:
                print(f"{solver} {toy}: {error}")
            if errors:
                solver_ok = False
        if solver_ok:
            accepted = True
            print(f"{solver}: passed")
        else:
            print(f"{solver}: failed")

    if accepted:
        print("wp probe: ok")
        return 0
    print("wp probe: no pinned solver judged the false goal invalid and the "
          "true goal valid")
    return 1


if __name__ == "__main__":
    sys.exit(main())
