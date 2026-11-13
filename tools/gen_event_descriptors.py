#!/usr/bin/env python3
"""Generate event code -> domain record fields from the IDIS event table.

The source is docs/idis/event_codes.csv (code/name extracted from the IDIS
Pack3 IP profile event table). The classification below follows the table's
event-log columns and descriptions:
  - Standard Event Log: meter/supply/clock/memory events
  - Fraud Detection Log: fraud/tamper
  - M-Bus Event/Control logs: M-Bus
  - Communication logs: network/modem
  - Power Quality Event Log: voltage/current quality
  - Disconnector Control Log: disconnect/reconnect
"""
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/idis/event_codes.csv"
OUT_C = ROOT / "targets/idis_ip_meter/cosem/nd_idis_event_desc.c"
OUT_H = ROOT / "targets/idis_ip_meter/cosem/nd_idis_event_desc.h"
OUT_CSV = ROOT / "docs/idis/event_descriptor_map.csv"

SOURCE = {
    "OTHER": 0,
    "SUPPLY": 1,
    "COVER": 2,
    "CLOCK": 3,
    "BATTERY": 4,
    "MEMORY": 5,
    "LOAD": 6,
    "NETWORK": 7,
    "MBUS": 8,
    "SECURITY": 9,
    "VOLTAGE": 10,
    "TARIFF": 11,
    "DISCONNECT": 12,
    "FIRMWARE": 13,
}

SEVERITY = {"INFO": 0, "WARN": 1, "ERROR": 2, "CRITICAL": 3}


def classify(code: int, name: str):
    text = name.lower()

    if code == 1:
        return "ND_EVENT_POWER_OFF", "SUPPLY", "CRITICAL"
    if code == 2:
        return "ND_EVENT_POWER_UP", "SUPPLY", "INFO"
    if 3 <= code <= 6:
        return "ND_EVENT_CLOCK", "CLOCK", "ERROR" if code == 6 else "INFO"
    if 7 <= code <= 8:
        return "ND_EVENT_BATTERY", "BATTERY", "WARN"
    if code in (9, 19):
        return "ND_EVENT_GENERIC", "TARIFF", "INFO"
    if code in (10, 11):
        return "ND_EVENT_GENERIC", "MEMORY", "INFO"
    if 12 <= code <= 16:
        return "ND_EVENT_ERROR", "MEMORY", "ERROR"
    if 17 <= code <= 18:
        return "ND_EVENT_GENERIC", "FIRMWARE", "INFO"
    if code == 20:
        return "ND_EVENT_FRAUD", "SECURITY", "WARN"
    if code == 21:
        return "ND_EVENT_GENERIC", "OTHER", "INFO"
    if 22 <= code <= 29:
        return "ND_EVENT_COMMUNICATION", "NETWORK", "INFO"
    if 30 <= code <= 39:
        return "ND_EVENT_MBUS", "MBUS", "INFO"
    if code in (40, 44):
        return "ND_EVENT_COVER_OPEN", "COVER", "WARN"
    if code in (41, 45):
        return "ND_EVENT_COVER_OPEN", "COVER", "INFO"
    if code in (42, 43):
        return "ND_EVENT_FRAUD", "SECURITY", "WARN"
    if code == 46:
        return "ND_EVENT_GENERIC", "SECURITY", "INFO"
    if code in (47, 48):
        return "ND_EVENT_GENERIC", "SECURITY", "WARN"
    if code in (49, 50):
        return "ND_EVENT_FRAUD", "SECURITY", "CRITICAL"
    if code == 51:
        return "ND_EVENT_ERROR", "FIRMWARE", "ERROR"
    if code == 52:
        return "ND_EVENT_FRAUD", "SECURITY", "WARN"
    if 53 <= code <= 59:
        return "ND_EVENT_LOAD_CONTROL", "LOAD", "INFO"
    if 60 <= code <= 69:
        return "ND_EVENT_DISCONNECT", "DISCONNECT", \
            "INFO" if code in (66, 69) else "WARN"
    if 70 <= code <= 75:
        return "ND_EVENT_DISCONNECT", "DISCONNECT", "WARN"
    if 76 <= code <= 87:
        return "ND_EVENT_VOLTAGE", "VOLTAGE", \
            "INFO" if code >= 85 else "WARN"
    if code == 88:
        return "ND_EVENT_VOLTAGE", "VOLTAGE", "ERROR"
    if code == 89:
        return "ND_EVENT_NEUTRAL_CURRENT", "VOLTAGE", "CRITICAL"
    if code == 90:
        return "ND_EVENT_VOLTAGE", "VOLTAGE", "WARN"
    if code == 91:
        return "ND_EVENT_OVER_CURRENT", "VOLTAGE", "WARN"
    if 92 <= code <= 94:
        return "ND_EVENT_VOLTAGE", "VOLTAGE", "WARN"
    if code in (95, 96, 97, 98):
        return "ND_EVENT_LOAD_CONTROL", "LOAD", "INFO"
    if code == 99:
        return "ND_EVENT_MBUS", "MBUS", "WARN"
    if code == 100:
        return "ND_EVENT_MBUS", "MBUS", "WARN"
    if code == 101:
        return "ND_EVENT_MBUS", "MBUS", "INFO"
    if code in (102, 112, 122, 132):
        return "ND_EVENT_BATTERY", "MBUS", "WARN"
    if code in (103, 113, 123, 133):
        return "ND_EVENT_FRAUD", "MBUS", "WARN"
    if code in (104, 114, 124, 134):
        return "ND_EVENT_CLOCK", "MBUS", "INFO"
    if code in (105, 115, 125, 135):
        return "ND_EVENT_MBUS", "MBUS", "INFO"
    if code in (106, 116, 126, 136):
        return "ND_EVENT_MBUS", "MBUS", "ERROR"
    if code in (107, 108, 109, 117, 118, 119):
        return "ND_EVENT_LOAD_CONTROL", "LOAD", "INFO"
    if 110 <= code <= 136:
        return "ND_EVENT_MBUS", "MBUS", "WARN"
    if 140 <= code <= 159:
        if code in (143, 146, 147, 149, 150, 151, 152, 153, 157, 158):
            return "ND_EVENT_COMMUNICATION", "NETWORK", "INFO"
        if code in (140, 141, 154, 156):
            return "ND_EVENT_COMMUNICATION", "NETWORK", "WARN"
        return "ND_EVENT_COMMUNICATION", "NETWORK", "ERROR"
    if 160 <= code <= 196:
        return "ND_EVENT_DISCONNECT", "MBUS", "WARN"
    if code == 254:
        return "ND_EVENT_GENERIC", "LOAD", "INFO"
    if code == 255:
        return "ND_EVENT_GENERIC", "OTHER", "INFO"
    if "cover" in text:
        return "ND_EVENT_COVER_OPEN", "COVER", "WARN"
    if "battery" in text:
        return "ND_EVENT_BATTERY", "BATTERY", "WARN"
    if "clock" in text:
        return "ND_EVENT_CLOCK", "CLOCK", "INFO"
    if "error" in text or "failure" in text:
        return "ND_EVENT_ERROR", "MEMORY", "ERROR"
    return "ND_EVENT_GENERIC", "OTHER", "INFO"


rows = list(csv.DictReader(SRC.open(encoding="utf-8")))
items = []
for row in rows:
    code = int(row["code"])
    name = row["name"].replace("\n", " ").strip()
    kind, source, severity = classify(code, name)
    items.append((code, name, kind, SOURCE[source], SEVERITY[severity],
                  source, severity))

items.sort(key=lambda item: item[0])

with OUT_CSV.open("w", encoding="utf-8", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["code", "name", "kind", "source", "severity"])
    for code, name, kind, source_id, severity_id, source, severity in items:
        writer.writerow([code, name, kind, source, severity])

with OUT_H.open("w", encoding="utf-8") as f:
    f.write("#ifndef ND_IDIS_EVENT_DESC_H\n#define ND_IDIS_EVENT_DESC_H\n\n")
    f.write('#include "domain/events/event_record.h"\n\n')
    f.write("#define ND_EVENT_SOURCE_OTHER 0\n")
    f.write("#define ND_EVENT_SOURCE_SUPPLY 1\n")
    f.write("#define ND_EVENT_SOURCE_COVER 2\n")
    f.write("#define ND_EVENT_SOURCE_CLOCK 3\n")
    f.write("#define ND_EVENT_SOURCE_BATTERY 4\n")
    f.write("#define ND_EVENT_SOURCE_MEMORY 5\n")
    f.write("#define ND_EVENT_SOURCE_LOAD 6\n")
    f.write("#define ND_EVENT_SOURCE_NETWORK 7\n")
    f.write("#define ND_EVENT_SOURCE_MBUS 8\n")
    f.write("#define ND_EVENT_SOURCE_SECURITY 9\n")
    f.write("#define ND_EVENT_SOURCE_VOLTAGE 10\n")
    f.write("#define ND_EVENT_SOURCE_TARIFF 11\n")
    f.write("#define ND_EVENT_SOURCE_DISCONNECT 12\n")
    f.write("#define ND_EVENT_SOURCE_FIRMWARE 13\n\n")
    f.write("#define ND_EVENT_SEVERITY_INFO 0\n")
    f.write("#define ND_EVENT_SEVERITY_WARN 1\n")
    f.write("#define ND_EVENT_SEVERITY_ERROR 2\n")
    f.write("#define ND_EVENT_SEVERITY_CRITICAL 3\n\n")
    f.write("typedef struct {\n")
    f.write("    uint16_t code;\n")
    f.write("    nd_event_kind_t kind;\n")
    f.write("    uint8_t source;\n")
    f.write("    uint8_t severity;\n")
    f.write("} nd_idis_event_descriptor_t;\n\n")
    f.write("const nd_idis_event_descriptor_t *nd_idis_event_describe(uint16_t code);\n\n")
    f.write("#endif\n")

with OUT_C.open("w", encoding="utf-8") as f:
    f.write('#include "targets/idis_ip_meter/cosem/nd_idis_event_desc.h"\n\n')
    f.write("static const nd_idis_event_descriptor_t descriptors[] = {\n")
    for code, name, kind, source_id, severity_id, source, severity in items:
        f.write(f'    {{{code}, {kind}, {source_id}, {severity_id}}}, /* {name} */\n')
    f.write("};\n\n")
    f.write("const nd_idis_event_descriptor_t *nd_idis_event_describe(uint16_t code)\n{\n")
    f.write("    for (unsigned int index = 0; index < sizeof(descriptors) / sizeof(descriptors[0]); index++) {\n")
    f.write("        if (descriptors[index].code == code) {\n")
    f.write("            return &descriptors[index];\n")
    f.write("        }\n")
    f.write("    }\n")
    f.write("    return 0;\n")
    f.write("}\n")

print(f"{len(items)} event descriptors -> {OUT_C}, {OUT_CSV}")
