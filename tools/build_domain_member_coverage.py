#!/usr/bin/env python3
"""Attach a domain owner and contract to every IDIS member row."""

import csv
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/idis/obis_logic_chains.csv"
OUT = ROOT / "docs/idis/domain_member_coverage.csv"
SUMMARY = ROOT / "docs/idis/domain_member_coverage.md"

DOMAIN_CONTRACT = {
    "metering": "domain/metering/energy_set.h",
    "metering.measurement": "domain/metering/measurement.h",
    "metering.demand": "domain/metering/demand.h + demand_history.h",
    "metering.demand+freeze": "demand_history.h + freeze/capture.h",
    "metering+freeze": "energy_set.h + freeze/capture.h",
    "metering+load_control": "blocked: C1",
    "events": "domain/events/event_record.h",
    "events/freeze": "event_record.h + freeze/capture.h",
    "events+freeze": "event_record.h + freeze/capture.h",
    "freeze": "domain/freeze/capture.h",
    "tariff": "domain/tariff/activity_calendar.h + tariff.h",
    "load_control": "domain/load_control/disconnect.h + load_control.h",
    "common.clock": "domain/common/clock_value.h + calendar.h",
    "common.clock+services.time": "clock_value.h + target NTP",
    "m-bus": "new M-Bus bounded context",
    "display": "target display model",
}

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

NON_DOMAIN = {
    "network",
    "protocol.push",
    "protocol.link",
    "protocol.function",
    "association",
    "security",
    "product",
    "display/protocol",
    "upgrade",
}


def owner_and_contract(domain):
    if domain == "m-bus":
        return "m-bus", "new M-Bus bounded context"
    if domain in NON_DOMAIN:
        return "target/protocol", "-"
    return "domain", DOMAIN_CONTRACT.get(domain, "review ownership")


def main():
    rows = list(csv.DictReader(SRC.open(encoding="utf-8")))
    out = []
    for row in rows:
        owner, contract = owner_and_contract(row["domain"])
        out.append(
            {
                "obis": row["obis"],
                "ic": row["ic"],
                "object": row["object"],
                "domain": row["domain"],
                "owner": owner,
                "contract": contract,
                "kind": row["kind"],
                "id": row["id"],
                "member": row["member"],
                "support": row["support"],
                "data_structure": row["data_structure"],
                "domain_components": DOMAIN_COMPONENTS.get(
                    row["domain"], "review ownership"
                ),
                "embedding": row["embedding"],
                "persistence": row["persistence"],
                "gap": row["gap"],
            }
        )

    with OUT.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(out[0]))
        writer.writeheader()
        writer.writerows(out)

    owners = Counter(r["owner"] for r in out)
    contracts = Counter(r["contract"] for r in out)
    support = Counter(r["support"] for r in out)
    lines = [
        "# IDIS P3 成员级 domain 覆盖",
        "",
        "由 `tools/build_domain_member_coverage.py` 生成。每行是 `obis_logic_chains.csv` 的一条成员，带 owner、契约、data_structure 和 domain_components。",
        "",
        f"- 成员总数：{len(out)}",
        f"- domain owner：{owners.get('domain', 0)}",
        f"- M-Bus owner：{owners.get('m-bus', 0)}",
        f"- target/protocol owner：{owners.get('target/protocol', 0)}",
        f"- 支撑分布：{dict(support)}",
        "",
        "## 契约分布",
        "",
        "| 契约 | 成员数 |",
        "|---|---:|",
    ]
    for contract, count in contracts.most_common():
        lines.append(f"| `{contract}` | {count} |")
    lines.append("")
    lines.append("明细见 `domain_member_coverage.csv`。")
    SUMMARY.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
