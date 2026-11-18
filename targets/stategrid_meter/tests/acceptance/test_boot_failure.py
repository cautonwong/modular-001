from __future__ import annotations

import re

from cases import case

MALLOC_COUNT_RE = re.compile(r"^\[sim\] mallocs=(\d+)$")


def malloc_count(run):
    counts = [int(match.group(1)) for line in run.trace
              if (match := MALLOC_COUNT_RE.match(line))]
    assert len(counts) == 1, (counts, run.trace)
    return counts[0]


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

    # The simulator reports how many shared allocator calls a clean boot
    # makes, so a new allocation joins this loop without editing it.
    reference = ctx.run(cycles=1)
    assert reference.exit_code == 0, (reference.exit_code, reference.stdout,
                                      reference.stderr)
    total = malloc_count(reference)
    # 49 is the current boot path: context create, block and device
    # registration and the 698 buffers. A drop means an allocation stopped
    # going through the shared hook.
    assert total >= 49, reference.trace

    # Every allocation result, including the context's first one, has to
    # fail closed instead of being dereferenced.
    for index in range(1, total + 1):
        run = ctx.run(cycles=1,
                      extra_args=("--fail-malloc", str(index)))
        assert run.exit_code != 0, (index, run.exit_code, run.stdout,
                                    run.stderr)
        assert any("malloc fail" in line for line in run.trace), (index,
                                                                 run.trace)
        assert any("init failed" in line for line in run.trace), (index,
                                                                 run.trace)
        assert run.done_line is None, (index, run.trace)
