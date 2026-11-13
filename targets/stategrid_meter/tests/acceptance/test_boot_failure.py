from __future__ import annotations

from cases import case


@case(
    "ACC-BOOT-002",
    "a resource allocation failure exits nonzero and is reported as init failed",
    ("boot", "failure"),
)
def init_failure_is_visible(ctx):
    run = ctx.run(cycles=1, extra_args=("--arena-size", "384"))
    assert run.exit_code != 0, (run.exit_code, run.stdout, run.stderr)
    assert any("init failed" in line for line in run.trace), run.trace
    assert run.done_line is None, run.trace
