from __future__ import annotations

import re
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

from sim import HarnessError, SimTimeout

_PAIR = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=([^ \t]+)")


def parse_values(stdout: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in stdout.splitlines():
        for key, value in _PAIR.findall(line):
            values[key] = value
    return values


@dataclass
class RunnerRun:
    command: list[str]
    exit_code: int
    stdout: str
    stderr: str
    duration_s: float
    run_dir: Path
    values: dict[str, str]

    def evidence(self) -> dict[str, object]:
        return {
            "kind": "run",
            "command": self.command,
            "exit_code": self.exit_code,
            "duration_s": round(self.duration_s, 4),
            "stdout_tail": self.stdout.splitlines()[-10:],
            "stderr_tail": self.stderr.splitlines()[-5:],
        }


class RunnerContext:
    def __init__(self, harness: "RunnerHarness", case_dir: Path):
        self.harness = harness
        self.case_dir = case_dir
        self.runs: list[RunnerRun] = []

    def run(
        self,
        seconds: int,
        *,
        start_time: str | None = None,
        no_final_save: bool = False,
        extra_args: tuple[str, ...] = (),
    ) -> RunnerRun:
        result = self.harness.run(
            self.case_dir,
            seconds=seconds,
            start_time=start_time,
            no_final_save=no_final_save,
            extra_args=extra_args,
        )
        self.runs.append(result)
        return result


class RunnerHarness:
    def __init__(self, binary: Path, root: Path, timeout: float = 60.0):
        self.binary = binary
        self.root = root
        self.timeout = timeout

    def context(self, case_dir: Path) -> RunnerContext:
        return RunnerContext(self, case_dir)

    def run(
        self,
        case_dir: Path,
        *,
        seconds: int,
        start_time: str | None = None,
        no_final_save: bool = False,
        extra_args: tuple[str, ...] = (),
    ) -> RunnerRun:
        if not self.binary.is_file():
            raise HarnessError(
                f"idis_host_runner is not built at {self.binary}; run "
                "cmake --build build --target idis_host_runner"
            )

        run_dir = case_dir
        run_dir.mkdir(parents=True, exist_ok=True)
        command = [str(self.binary), "--backup-files", str(run_dir),
                   "--seconds", str(seconds)]
        if start_time is not None:
            command.extend(["--start-time", start_time])
        if no_final_save:
            command.append("--no-final-save")
        command.extend(extra_args)

        started = time.monotonic()
        try:
            completed = subprocess.run(command, capture_output=True, text=True,
                                       timeout=self.timeout)
        except subprocess.TimeoutExpired as exc:
            raise SimTimeout(
                f"idis_host_runner did not finish within {self.timeout:.0f}s"
            ) from exc
        return RunnerRun(
            command=command,
            exit_code=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
            duration_s=time.monotonic() - started,
            run_dir=run_dir,
            values=parse_values(completed.stdout),
        )
