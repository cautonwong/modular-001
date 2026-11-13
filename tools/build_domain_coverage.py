#!/usr/bin/env python3
"""Aggregate IDIS member rows by domain and report contract ownership."""

import csv
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/idis/obis_logic_chains.csv"
OUT_CSV = ROOT / "docs/idis/domain_coverage.csv"
OUT_MD = ROOT / "docs/idis/domain_coverage.md"

DOMAIN_ACTION = {
    "metering": "energy quadrant/phase snapshot",
    "metering.measurement": "measurement value contract",
    "metering.demand": "demand status/history",
    "metering.demand+freeze": "demand history + capture",
    "metering+freeze": "energy capture",
    "events": "event record/log",
    "events/freeze": "event log/profile",
    "events+freeze": "PQ event capture",
    "freeze": "capture value contract",
    "tariff": "activity calendar actions",
    "load_control": "disconnect state",
    "metering+load_control": "unblock C1",
    "common.clock": "clock value/deviation",
    "common.clock+services.time": "NTP boundary",
    "m-bus": "new M-Bus bounded context",
    "display": "display model",
    "upgrade": "image port/target",
}

NON_DOMAIN = {
    "network",
    "protocol.push",
    "protocol.link",
    "protocol.function",
    "association",
    "security",
    "product",
    "display/protocol",
}


def owner(domain):
    if domain in NON_DOMAIN:
        return "target/protocol"
    return "domain"


def main():
    rows = list(csv.DictReader(SRC.open(encoding="utf-8")))
    groups = {}
    for row in rows:
        d = row["domain"]
        g = groups.setdefault(
            d,
            {
                "domain": d,
                "members": 0,
                "objects": set(),
                "support": Counter(),
                "existing_code": set(),
                "gaps": set(),
            },
        )
        g["members"] += 1
        g["objects"].add(row["obis"])
        g["support"][row["support"]] += 1
        if row["existing_code"].strip():
            g["existing_code"].add(row["existing_code"])
        if row["gap"].strip() and not row["gap"].startswith("none"):
            g["gaps"].add(row["gap"])

    out = []
    for d, g in groups.items():
        out.append(
            {
                "domain": d,
                "owner": owner(d),
                "members": g["members"],
                "objects": len(g["objects"]),
                "reuse": g["support"].get("reuse", 0),
                "partial": g["support"].get("partial", 0),
                "new": g["support"].get("new", 0),
                "blocked": g["support"].get("blocked", 0),
                "existing_code_refs": len(g["existing_code"]),
                "unique_gaps": len(g["gaps"]),
                "next_action": DOMAIN_ACTION.get(d, "review ownership"),
            }
        )
    out.sort(key=lambda r: (-r["members"], r["domain"]))

    with OUT_CSV.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(out[0]))
        writer.writeheader()
        writer.writerows(out)

    lines = [
        "# IDIS P3 domain 覆盖表",
        "",
        "由 `tools/build_domain_coverage.py` 从 `obis_logic_chains.csv` 生成。",
        "",
        "| domain | owner | members | objects | reuse | partial | new | blocked | unique gaps | next |",
        "|---|---|---:|---:|---:|---:|---:|---:|---:|---|",
    ]
    for r in out:
        lines.append(
            f"| `{r['domain']}` | {r['owner']} | {r['members']} | {r['objects']} | "
            f"{r['reuse']} | {r['partial']} | {r['new']} | {r['blocked']} | "
            f"{r['unique_gaps']} | {r['next_action']} |"
        )
    lines.append("")
    lines.append(
        f"合计 {sum(r['members'] for r in out)} 行，"
        f"{sum(r['objects'] for r in out)} 个对象计数，"
        f"{len(out)} 个 domain 标签。"
    )
    lines.append("")
    lines.append("逐成员 owner 见 `domain_member_coverage.csv`。")
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
