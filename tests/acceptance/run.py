from __future__ import annotations

import argparse
import importlib
import json
import sys
import time
import traceback
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))

from cases import Case, SelectorError, describe, select  # noqa: E402
from sim import HarnessError, SimContext, SimHarness, SimRun, SimTimeout, evidence_record  # noqa: E402
from sim import write_jsonl  # noqa: E402

DEFAULT_CASE_DIR = ROOT / "targets/stategrid_meter/tests/acceptance"

STATUS_EXIT = {
    "clean": 0,
    "failed": 1,
    "invalid": 2,
    "incomplete": 3,
}


@dataclass
class CaseResult:
    case: Case
    status: str
    duration_s: float
    error: str | None = None
    detail: str | None = None
    runs: list[SimRun] = field(default_factory=list)


def load_cases(case_dir: Path) -> None:
    sys.path.insert(0, str(case_dir))
    for path in sorted(case_dir.glob("test_*.py")):
        importlib.import_module(path.stem)


def default_sim() -> Path:
    for candidate in (ROOT / "build" / "firmware_sim", ROOT / "tests" / "build" / "firmware_sim"):
        if candidate.is_file():
            return candidate
    return ROOT / "build" / "firmware_sim"


def verdict_of(results: list[CaseResult]) -> str:
    statuses = {result.status for result in results}
    if "fail" in statuses:
        return "failed"
    if "invalid" in statuses:
        return "invalid"
    if "incomplete" in statuses:
        return "incomplete"
    if not results:
        return "invalid"
    return "clean"


def run_case(case: Case, harness: SimHarness, workdir: Path) -> CaseResult:
    case_dir = workdir / case.id
    case_dir.mkdir(parents=True, exist_ok=True)
    context = SimContext(harness, case_dir)
    started = time.monotonic()
    try:
        case.fn(context)
    except AssertionError as exc:
        return CaseResult(
            case, "fail", time.monotonic() - started, error=str(exc) or "assertion failed",
            runs=context.runs,
        )
    except SimTimeout as exc:
        return CaseResult(case, "incomplete", time.monotonic() - started, error=str(exc), runs=context.runs)
    except HarnessError as exc:
        return CaseResult(case, "invalid", time.monotonic() - started, error=str(exc), runs=context.runs)
    except Exception as exc:  # a broken harness must not look like a device failure
        return CaseResult(
            case,
            "invalid",
            time.monotonic() - started,
            error=f"{type(exc).__name__}: {exc}",
            detail=traceback.format_exc(),
            runs=context.runs,
        )
    return CaseResult(case, "pass", time.monotonic() - started, runs=context.runs)


def write_case_evidence(path: Path, result: CaseResult) -> None:
    records: list[dict[str, object]] = [
        {
            "kind": "case",
            "id": result.case.id,
            "clause": result.case.clause,
            "tags": list(result.case.tags),
        }
    ]
    for run in result.runs:
        try:
            records.append(evidence_record(run))
        except HarnessError as exc:
            records.append({"kind": "run", "error": str(exc)})
    records.append(
        {
            "kind": "verdict",
            "status": result.status,
            "duration_s": round(result.duration_s, 4),
            "error": result.error,
            "detail": result.detail,
        }
    )
    write_jsonl(path, records)


def write_summary(
    evidence_root: Path,
    started_at: datetime,
    sim: Path,
    selectors: list[str],
    results: list[CaseResult],
    verdict: str,
) -> None:
    summary = {
        "kind": "acceptance",
        "started_at": started_at.isoformat().replace("+00:00", "Z"),
        "sim": str(sim),
        "select": selectors,
        "verdict": verdict,
        "cases": [
            {
                "id": result.case.id,
                "status": result.status,
                "duration_s": round(result.duration_s, 4),
                "error": result.error,
                "runs": len(result.runs),
            }
            for result in results
        ],
    }
    (evidence_root / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )

    lines = [
        "# firmware_sim acceptance",
        "",
        f"- verdict: {verdict}",
        f"- simulator: `{sim}`",
        f"- started: {started_at.isoformat().replace('+00:00', 'Z')}",
        "",
        "| case | status | runs | seconds |",
        "|---|---|---:|---:|",
    ]
    for result in results:
        lines.append(
            f"| {result.case.id} | {result.status} | {len(result.runs)} | "
            f"{result.duration_s:.3f} |"
        )
    failures = [result for result in results if result.error]
    if failures:
        lines.extend(["", "## Failures", ""])
        for result in failures:
            lines.append(f"- `{result.case.id}`: {result.error}")
    (evidence_root / "report.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    scope = {
        "kind": "acceptance",
        "sim": str(sim),
        "select": selectors,
        "cases": [result.case.id for result in results],
    }
    (evidence_root / "run-scope.json").write_text(
        json.dumps(scope, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run firmware_sim acceptance cases")
    parser.add_argument("--sim", type=Path, default=None, help="firmware_sim binary")
    parser.add_argument("--workdir", type=Path, default=ROOT / "build" / "acceptance" / "work")
    parser.add_argument("--evidence-dir", type=Path, default=None)
    parser.add_argument("--select", action="append", default=[], help="case:ID or tag:NAME")
    parser.add_argument("--list", action="store_true", help="list selected cases and exit")
    parser.add_argument("--no-evidence", action="store_true")
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument(
        "--cases", type=Path, default=DEFAULT_CASE_DIR,
        help="directory containing target-specific test_*.py modules")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    load_cases(args.cases)

    try:
        selected = select(args.select)
    except SelectorError as exc:
        print(f"INVALID {exc}", file=sys.stderr)
        return STATUS_EXIT["invalid"]

    if args.list:
        print(describe(selected))
        return 0 if selected else STATUS_EXIT["invalid"]
    if not selected:
        print("INVALID no acceptance cases selected", file=sys.stderr)
        return STATUS_EXIT["invalid"]

    sim = args.sim or default_sim()
    if not sim.is_file():
        print(
            f"INVALID firmware_sim is not built at {sim}; "
            "run cmake --build build --target firmware_sim",
            file=sys.stderr,
        )
        return STATUS_EXIT["invalid"]

    started_at = datetime.now(timezone.utc)
    harness = SimHarness(sim, ROOT, timeout=args.timeout)
    results = [run_case(item, harness, args.workdir) for item in selected]
    verdict = verdict_of(results)

    for result in results:
        label = {"pass": "PASS", "fail": "FAIL", "invalid": "INVALID", "incomplete": "INCOMPLETE"}[
            result.status
        ]
        print(f"{label} {result.case.id} {result.case.clause} ({result.duration_s:.2f}s)")
        if result.error:
            print(f"  {result.error}")

    evidence_dir = args.evidence_dir
    if evidence_dir is None:
        evidence_dir = args.workdir.parent / "evidence"
    if not args.no_evidence:
        evidence_root = evidence_dir / started_at.strftime("%Y%m%dT%H%M%S%fZ")
        evidence_root.mkdir(parents=True, exist_ok=True)
        for result in results:
            write_case_evidence(evidence_root / f"{result.case.id}.jsonl", result)
        write_summary(evidence_root, started_at, sim, args.select, results, verdict)
        print(f"evidence {evidence_root}")

    passed = sum(result.status == "pass" for result in results)
    failed = sum(result.status == "fail" for result in results)
    invalid = sum(result.status == "invalid" for result in results)
    incomplete = sum(result.status == "incomplete" for result in results)
    print(
        f"verdict={verdict} passed={passed} failed={failed} "
        f"invalid={invalid} incomplete={incomplete}"
    )
    return STATUS_EXIT[verdict]


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
