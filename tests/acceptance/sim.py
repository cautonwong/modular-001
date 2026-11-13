from __future__ import annotations

import json
import re
import subprocess
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

CYCLE_RE = re.compile(
    r"^\[sim\] cycle=(\d+) time=(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d) ms=(\d+)$"
)
BOOT_RE = re.compile(r"^\[sim\] boot ")
DONE_RE = re.compile(r"^\[sim\] done cycles=(\d+)$")
STAGE_RE = re.compile(r"^\[sim\] stage=(.+)$")
FILTER_RE = re.compile(
    r"^\[sim\] filter pipeline=(\S+) id=0x([0-9a-fA-F]+) ret=(-?\d+)$"
)
MESSAGE_RE = re.compile(
    r"^\[sim\] msg from=0x([0-9a-fA-F]+) to=0x([0-9a-fA-F]+) "
    r"ctrl=0x([0-9a-fA-F]+) len=(\d+)$"
)

EEPROM_SIZE = 65535
FLASH_SIZE = 8388607

# The 32-bit reset counter lives in the parameter block. A blank block makes
# the firmware read 0xFFFFFFFF, so the first boot wraps to 0 and each later
# boot increments from there. The offset is part of the current EEPROM layout;
# a layout change must show up as a failing acceptance case, not as a silent
# test update.
EEPROM_RESET_COUNT_OFFSET = 6006
EEPROM_RESET_COUNT_WIDTH = 4


class HarnessError(RuntimeError):
    """The simulator could not be run or produced no observable artifact."""


class SimTimeout(HarnessError):
    """The simulator did not finish inside the acceptance timeout."""


class SimFailure(AssertionError):
    """The simulator ran but did not produce the expected external artifact."""


@dataclass(frozen=True)
class Cycle:
    index: int
    time: str
    ms: int


@dataclass(frozen=True)
class FilterHit:
    pipeline: str
    filter_id: int
    ret: int


@dataclass(frozen=True)
class Message:
    source: int
    target: int
    control: int
    length: int


@dataclass
class SimRun:
    command: list[str]
    exit_code: int
    stdout: str
    stderr: str
    trace: list[str]
    cycles: list[Cycle]
    stages: list[str]
    filters: list[FilterHit]
    messages: list[Message]
    eeprom: bytes
    flash: bytes
    uart_tx: bytes
    uart_tx_hex: list[str]
    relay_log: list[str]
    backlight_log: list[str]
    lcd_log: list[str]
    duration_s: float
    run_dir: Path

    @property
    def boot_line(self) -> str | None:
        for line in self.trace:
            if BOOT_RE.match(line):
                return line
        return None

    @property
    def done_line(self) -> str | None:
        for line in self.trace:
            if DONE_RE.match(line):
                return line
        return None

    @property
    def reset_count(self) -> int:
        end = EEPROM_RESET_COUNT_OFFSET + EEPROM_RESET_COUNT_WIDTH
        if len(self.eeprom) < end:
            raise HarnessError("EEPROM image is too short to hold the reset counter")
        return int.from_bytes(self.eeprom[EEPROM_RESET_COUNT_OFFSET:end], "little")

    def non_ff_eeprom(self) -> int:
        return len(self.eeprom) - self.eeprom.count(0xFF)

    def non_ff_flash(self) -> int:
        return len(self.flash) - self.flash.count(0xFF)


@dataclass
class SimContext:
    harness: "SimHarness"
    case_dir: Path
    runs: list[SimRun] = field(default_factory=list)

    def run(
        self,
        cycles: int = 3,
        *,
        verbose: bool = False,
        seed_eeprom: bytes | None = None,
        seed_flash: bytes | None = None,
        scenario: str | None = None,
        start_time: str | None = None,
        extra_args: tuple[str, ...] = (),
    ) -> SimRun:
        result = self.harness.run(
            self.case_dir,
            cycles=cycles,
            verbose=verbose,
            seed_eeprom=seed_eeprom,
            seed_flash=seed_flash,
            scenario=scenario,
            start_time=start_time,
            extra_args=extra_args,
        )
        self.runs.append(result)
        return result


class SimHarness:
    def __init__(self, binary: Path, root: Path, timeout: float = 60.0):
        self.binary = binary
        self.root = root
        self.timeout = timeout
        self._run_index = 0

    def run(
        self,
        case_dir: Path,
        *,
        cycles: int,
        verbose: bool,
        seed_eeprom: bytes | None,
        seed_flash: bytes | None,
        scenario: str | None = None,
        start_time: str | None = None,
        extra_args: tuple[str, ...] = (),
    ) -> SimRun:
        if not self.binary.is_file():
            raise HarnessError(
                f"firmware_sim is not built at {self.binary}; run "
                "cmake --build build --target firmware_sim"
            )

        self._run_index += 1
        run_dir = case_dir / f"run-{self._run_index:02d}"
        run_dir.mkdir(parents=True, exist_ok=True)
        eeprom_path = run_dir / "eeprom.bin"
        flash_path = run_dir / "flash.bin"
        trace_path = run_dir / "trace.txt"
        scenario_path = run_dir / "scenario.txt"
        uart_tx_path = run_dir / "uart-tx.bin"
        uart_tx_hex_path = run_dir / "uart-tx.hex"
        relay_log_path = run_dir / "relay.log"
        backlight_log_path = run_dir / "backlight.log"
        lcd_log_path = run_dir / "lcd.log"
        uart_tx_path.unlink(missing_ok=True)
        uart_tx_hex_path.unlink(missing_ok=True)
        if seed_eeprom is None:
            eeprom_path.unlink(missing_ok=True)
        else:
            eeprom_path.write_bytes(seed_eeprom)
        if seed_flash is None:
            flash_path.unlink(missing_ok=True)
        else:
            flash_path.write_bytes(seed_flash)

        command = [
            str(self.binary),
            "--cycles",
            str(cycles),
            "--eeprom",
            str(eeprom_path),
            "--flash",
            str(flash_path),
            "--trace",
            str(trace_path),
            "--uart-tx",
            str(uart_tx_path),
            "--uart-tx-hex",
            str(uart_tx_hex_path),
            "--relay-log",
            str(relay_log_path),
            "--backlight-log",
            str(backlight_log_path),
            "--lcd-log",
            str(lcd_log_path),
        ]
        if scenario is not None:
            scenario_path.write_text(scenario, encoding="utf-8")
            command.extend(["--scenario", str(scenario_path)])
        if start_time is not None:
            command.extend(["--start-time", start_time])
        command.extend(extra_args)
        if verbose:
            command.append("--verbose")

        started = time.monotonic()
        try:
            completed = subprocess.run(
                command,
                cwd=self.root,
                capture_output=True,
                text=True,
                timeout=self.timeout,
                check=False,
            )
        except subprocess.TimeoutExpired as exc:
            raise SimTimeout(
                f"firmware_sim did not finish within {self.timeout:.0f}s"
            ) from exc
        duration = time.monotonic() - started

        (run_dir / "stdout.txt").write_text(completed.stdout, encoding="utf-8")
        (run_dir / "stderr.txt").write_text(completed.stderr, encoding="utf-8")

        stdout_lines = [
            line for line in completed.stdout.splitlines() if line.startswith("[sim]")
        ]
        trace_lines = list(stdout_lines)
        if trace_path.is_file():
            file_lines = [
                line
                for line in trace_path.read_text(encoding="utf-8").splitlines()
                if line.startswith("[sim]")
            ]
            trace_lines.extend(line for line in file_lines if line not in stdout_lines)

        cycles_seen: list[Cycle] = []
        stages: list[str] = []
        filters: list[FilterHit] = []
        messages: list[Message] = []
        for line in trace_lines:
            match = CYCLE_RE.match(line)
            if match:
                cycles_seen.append(
                    Cycle(int(match.group(1)), match.group(2), int(match.group(3)))
                )
                continue
            match = STAGE_RE.match(line)
            if match:
                stages.append(match.group(1))
                continue
            match = FILTER_RE.match(line)
            if match:
                filters.append(
                    FilterHit(
                        match.group(1), int(match.group(2), 16), int(match.group(3))
                    )
                )
                continue
            match = MESSAGE_RE.match(line)
            if match:
                messages.append(
                    Message(
                        int(match.group(1), 16),
                        int(match.group(2), 16),
                        int(match.group(3), 16),
                        int(match.group(4)),
                    )
                )

        if not eeprom_path.is_file():
            raise SimFailure(
                f"firmware_sim exited {completed.returncode} without creating {eeprom_path}"
            )
        if not flash_path.is_file():
            raise SimFailure(
                f"firmware_sim exited {completed.returncode} without creating {flash_path}"
            )

        return SimRun(
            command=command,
            exit_code=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
            trace=trace_lines,
            cycles=cycles_seen,
            stages=stages,
            filters=filters,
            messages=messages,
            eeprom=eeprom_path.read_bytes(),
            flash=flash_path.read_bytes(),
            uart_tx=uart_tx_path.read_bytes() if uart_tx_path.is_file() else b"",
            uart_tx_hex=(
                uart_tx_hex_path.read_text(encoding="utf-8").split()
                if uart_tx_hex_path.is_file()
                else []
            ),
            relay_log=(
                relay_log_path.read_text(encoding="utf-8").splitlines()
                if relay_log_path.is_file()
                else []
            ),
            backlight_log=(
                backlight_log_path.read_text(encoding="utf-8").splitlines()
                if backlight_log_path.is_file()
                else []
            ),
            lcd_log=(
                lcd_log_path.read_text(encoding="utf-8").splitlines()
                if lcd_log_path.is_file()
                else []
            ),
            duration_s=duration,
            run_dir=run_dir,
        )


def evidence_record(run: SimRun) -> dict[str, object]:
    return {
        "kind": "run",
        "command": run.command,
        "exit_code": run.exit_code,
        "duration_s": round(run.duration_s, 4),
        "cycles": len(run.cycles),
        "stages": len(run.stages),
        "filters": len(run.filters),
        "messages": len(run.messages),
        "first_time": run.cycles[0].time if run.cycles else None,
        "last_time": run.cycles[-1].time if run.cycles else None,
        "eeprom_size": len(run.eeprom),
        "flash_size": len(run.flash),
        "eeprom_non_ff": run.non_ff_eeprom(),
        "flash_non_ff": run.non_ff_flash(),
        "uart_tx_len": len(run.uart_tx),
        "reset_count": run.reset_count,
        "stderr_tail": run.stderr.splitlines()[-5:],
    }


def write_jsonl(path: Path, records: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        for record in records:
            handle.write(json.dumps(record, ensure_ascii=False) + "\n")
