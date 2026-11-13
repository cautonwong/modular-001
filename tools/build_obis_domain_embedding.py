#!/usr/bin/env python3
"""Build a per-OBIS domain embedding matrix from the logic-chain CSV."""

import csv
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/idis/obis_logic_chains.csv"
OUT = ROOT / "docs/idis/obis_domain_embedding.csv"
SUMMARY = ROOT / "docs/idis/obis_domain_embedding.md"

DOMAIN_COMPONENTS = {
    "metering": "domain/metering/energy.h + energy_set.h",
    "metering.measurement": "domain/metering/measurement.h + target measurement port",
    "metering.demand": "domain/metering/demand.h + demand_history.h",
    "metering.demand+freeze": "demand.h + demand_history.h + freeze/schedule.h + freeze/capture.h",
    "metering+freeze": "energy_set.h + freeze/schedule.h + freeze/capture.h",
    "metering+load_control": "blocked: C1",
    "events": "domain/events/* + event_record.h",
    "events/freeze": "event_record.h + freeze/capture.h + target profile store",
    "events+freeze": "event_record.h + freeze/capture.h + target profile store",
    "freeze": "domain/freeze/schedule.h + capture.h + target profile store",
    "tariff": "domain/tariff/tariff.h + activity_calendar.h",
    "load_control": "domain/load_control/load_control.h + disconnect.h + target relay adapter",
    "common.clock": "domain/common/calendar.h + clock_value.h + target clock port",
    "common.clock+services.time": "clock_value.h + target NTP",
    "m-bus": "components/protocol/mbus + target M-Bus client + profile store",
    "network": "target net adapters + nd_ip_port_t",
    "protocol.push": "target push engine + nd_cosem_port + nd_ip_port",
    "protocol.link": "target HDLC/local port + nd_dlms_link_t",
    "protocol.function": "target function state",
    "association": "components/protocol/dlms association + target association class",
    "security": "components/protocol/dlms security + target security/ESAM",
    "product": "target product config",
    "display": "target display model",
    "display/protocol": "target display/CIP state",
    "upgrade": "target image state + flash/bootloader",
}

SEVERITY = {"blocked": 3, "new": 2, "partial": 1, "reuse": 0}


def unique(values):
    seen = []
    for value in values:
        value = (value or "").strip()
        if value and value not in seen:
            seen.append(value)
    return " | ".join(seen)


def support_of(group):
    return max(group, key=lambda value: SEVERITY.get(value, 0))


def main():
    rows = list(csv.DictReader(SRC.open(encoding="utf-8")))
    groups = {}
    for row in rows:
        g = groups.setdefault(
            row["obis"],
            {
                "obis": row["obis"],
                "ic": row["ic"],
                "object": row["object"],
                "domain": row["domain"],
                "members": 0,
                "attributes": 0,
                "methods": 0,
                "chain_templates": [],
                "support": [],
                "state_owner": [],
                "data_structures": [],
                "existing_code": [],
                "embedding": [],
                "persistence": [],
                "gaps": [],
            },
        )
        g["members"] += 1
        if row["kind"] == "method":
            g["methods"] += 1
        else:
            g["attributes"] += 1
        for key, field in (
            ("chain_templates", "chain_id"),
            ("support", "support"),
            ("state_owner", "state_owner"),
            ("data_structures", "data_structure"),
            ("existing_code", "existing_code"),
            ("embedding", "embedding"),
            ("persistence", "persistence"),
            ("gaps", "gap"),
        ):
            g[key].append(row[field])

    out = []
    for obis, g in groups.items():
        out.append(
            {
                "obis": obis,
                "ic": g["ic"],
                "object": g["object"],
                "domain": g["domain"],
                "members": g["members"],
                "attributes": g["attributes"],
                "methods": g["methods"],
                "chain_templates": unique(g["chain_templates"]),
                "support": support_of(g["support"]),
                "state_owner": unique(g["state_owner"]),
                "data_structures": unique(g["data_structures"]),
                "domain_components": DOMAIN_COMPONENTS.get(
                    g["domain"], "review ownership"
                ),
                "existing_code": unique(g["existing_code"]),
                "embedding": unique(g["embedding"]),
                "persistence": unique(g["persistence"]),
                "gaps": unique(g["gaps"]),
            }
        )
    out.sort(key=lambda r: (r["domain"], r["obis"]))

    with OUT.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(out[0]))
        writer.writeheader()
        writer.writerows(out)

    by_domain = Counter(r["domain"] for r in out)
    by_support = Counter(r["support"] for r in out)
    lines = [
        "# IDIS P3 逐 OBIS domain 嵌入矩阵",
        "",
        "由 `tools/build_obis_domain_embedding.py` 从 `obis_logic_chains.csv` 生成。",
        "每行是一个 OBIS，列出数据结构和状态、经过的 domain 组件、现有代码锚点、嵌入方式和缺口。",
        "",
        f"- OBIS 对象：{len(out)}",
        f"- 成员行：{sum(r['members'] for r in out)}",
        f"- 支撑分布：{dict(by_support)}",
        "",
        "## 按 domain",
        "",
        "| domain | 对象数 | 成员数 |",
        "|---|---:|---:|",
    ]
    for domain, count in by_domain.most_common():
        members = sum(r["members"] for r in out if r["domain"] == domain)
        lines.append(f"| `{domain}` | {count} | {members} |")
    lines.append("")
    lines.append("明细见 `obis_domain_embedding.csv`。")
    SUMMARY.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
