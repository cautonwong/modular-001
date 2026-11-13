#!/usr/bin/env python3
"""Build a chain-template index from the per-member logic-chain CSV."""

import csv
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/idis/obis_logic_chains.csv"
OUT = ROOT / "docs/idis/chain_templates.csv"
SUMMARY = ROOT / "docs/idis/chain_templates.md"

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


def unique(values):
    out = []
    for value in values:
        value = (value or "").strip()
        if value and value not in out:
            out.append(value)
    return " | ".join(out)


def main():
    rows = list(csv.DictReader(SRC.open(encoding="utf-8")))
    groups = {}
    for row in rows:
        g = groups.setdefault(
            row["chain_id"],
            {
                "chain_id": row["chain_id"],
                "members": 0,
                "objects": set(),
                "domains": [],
                "data_structures": [],
                "existing_code": [],
                "embedding": [],
                "gaps": [],
                "representative": [],
            },
        )
        g["members"] += 1
        g["objects"].add(row["obis"])
        if len(g["representative"]) < 3 and row["obis"] not in g["representative"]:
            g["representative"].append(row["obis"])
        for key, field in (
            ("domains", "domain"),
            ("data_structures", "data_structure"),
            ("existing_code", "existing_code"),
            ("embedding", "embedding"),
            ("gaps", "gap"),
        ):
            g[key].append(row[field])

    out = []
    for chain_id, g in groups.items():
        domains = sorted(set(g["domains"]))
        components = "; ".join(
            DOMAIN_COMPONENTS.get(domain, "review ownership") for domain in domains
        )
        out.append(
            {
                "chain_id": chain_id,
                "members": g["members"],
                "objects": len(g["objects"]),
                "domains": "; ".join(domains),
                "domain_components": components,
                "representative_obis": "; ".join(g["representative"]),
                "data_structures": unique(g["data_structures"]),
                "existing_code": unique(g["existing_code"]),
                "embedding": unique(g["embedding"]),
                "gaps": unique(g["gaps"]),
            }
        )
    out.sort(key=lambda r: (-r["members"], r["chain_id"]))

    with OUT.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(out[0]))
        writer.writeheader()
        writer.writerows(out)

    lines = [
        "# IDIS P3 链路模板索引",
        "",
        "由 `tools/build_chain_templates.py` 从 `obis_logic_chains.csv` 生成。",
        "每个 OBIS 属于一个链路模板，模板列出成员数、domain 组件、代表 OBIS、数据结构和缺口。",
        "",
        f"- 模板数：{len(out)}",
        f"- 成员总数：{sum(r['members'] for r in out)}",
        "",
        "| 模板 | 成员 | 对象 | domain | domain 组件 | 代表 OBIS |",
        "|---|---:|---:|---|---|---|",
    ]
    for r in out:
        lines.append(
            f"| `{r['chain_id']}` | {r['members']} | {r['objects']} | "
            f"`{r['domains']}` | {r['domain_components']} | `{r['representative_obis']}` |"
        )
    lines.append("")
    lines.append("逐模板的数据结构和缺口见 `chain_templates.csv`。")
    SUMMARY.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
