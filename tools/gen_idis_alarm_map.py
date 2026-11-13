#!/usr/bin/env python3
"""Generate the IDIS event-code to alarm-bit table from alarm_bits.csv."""
import csv
from pathlib import Path

SRC = Path('docs/idis/alarm_bits.csv')
OUT_C = Path('targets/idis_ip_meter/cosem/nd_idis_event_map.c')
OUT_H = Path('targets/idis_ip_meter/cosem/nd_idis_event_map.h')

rows = list(csv.DictReader(SRC.open(encoding='utf-8')))
rules = []
seen = {}
for row in rows:
    register = int(row['register'])
    bit = int(row['bit'])
    if register not in (1, 2) or not 0 <= bit <= 31:
        raise SystemExit(f"invalid register/bit: {row}")
    for text in row['event_codes'].split(','):
        text = text.strip()
        if not text:
            continue
        code = int(text)
        if code in seen and seen[code] != (register, bit):
            raise SystemExit(f"event {code} mapped twice: {seen[code]} and {(register, bit)}")
        seen[code] = (register, bit)
        rules.append((code, register - 1, bit, row['alarm']))

rules.sort(key=lambda item: (item[1], item[2], item[0]))

with OUT_H.open('w', encoding='utf-8') as f:
    f.write('#ifndef ND_IDIS_EVENT_MAP_H\n#define ND_IDIS_EVENT_MAP_H\n\n')
    f.write('#include "targets/idis_ip_meter/cosem/nd_alarm_state.h"\n\n')
    f.write('extern const nd_idis_event_rule_t nd_idis_event_rules[];\n')
    f.write('extern const unsigned int nd_idis_event_rule_count;\n\n#endif\n')

with OUT_C.open('w', encoding='utf-8') as f:
    f.write('#include "targets/idis_ip_meter/cosem/nd_idis_event_map.h"\n\n')
    f.write('const nd_idis_event_rule_t nd_idis_event_rules[] = {\n')
    for code, register, bit, alarm in rules:
        f.write(f'    {{{code}, {register}, {bit}}}, /* {alarm} */\n')
    f.write('};\n\n')
    f.write('const unsigned int nd_idis_event_rule_count =\n')
    f.write('    sizeof(nd_idis_event_rules) / sizeof(nd_idis_event_rules[0]);\n')

print(f'{len(rules)} event rules -> {OUT_C}')
