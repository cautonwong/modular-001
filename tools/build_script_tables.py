#!/usr/bin/env python3
"""List every IDIS P3 script table entry extracted from the two IDIS documents."""
import csv

ROWS = []
def add(table, name, script, service, cls, ln, kind, member, param, optional, source, note=''):
    ROWS.append(dict(table_obis=table, table_name=name, script_id=str(script),
                     service_id=str(service), target_class=str(cls),
                     target_obis=ln, target_kind=kind, target_member=str(member),
                     parameter=param, optional=optional, source=source, note=note))

TARIFF = '0-0:10.0.100.255'
for s in (1, 2, 3, 4):
    add(TARIFF, 'Tariffication script table', s, 1, 6, '0-0:14.0.1.255', 'attribute', 4, '*', 'no', 'OM 0-0:10.0.100.255', 'active_mask selector')
    add(TARIFF, 'Tariffication script table', s, 1, 6, '0-0:14.0.2.255', 'attribute', 4, '*', 'no', 'OM 0-0:10.0.100.255', 'active_mask selector')
    add(TARIFF, 'Tariffication script table', s, 1, 70, '0-1:96.3.10.255', 'attribute', 3, '[1..2]', 'no', 'OM 0-0:10.0.100.255', 'load control state')

add('0-0:10.0.1.255', 'MDI reset / end of billing period', 1, 2, 7, '0-0:98.1.0.255', 'method', 2, '?', 'no', 'IP 7.6 / Blue Book 4.3.6', 'working assumption: capture method; exact action list not printed')

for s, attr, val in ((1, 3, 2), (2, 3, 1), (3, 1, 0), (4, 2, 0)):
    kind = 'attribute' if s <= 2 else 'method'
    add('0-0:10.0.106.255', 'Disconnector script table', s, 1 if s <= 2 else 2, 70,
        '0-0:96.3.10.255', kind, attr, str(val), 'no', 'IP 6.5.1 / OM', '')

for k in (1, 2):
    for off, attr, val in ((1, 3, 2), (2, 3, 1), (3, 1, 0), (4, 2, 0)):
        sid = (k - 1) * 4 + off
        kind = 'attribute' if off <= 2 else 'method'
        add('0-0:10.0.103.255', 'Load Management script table', sid,
            1 if off <= 2 else 2, 70, f'0-{k}:96.3.10.255', kind, attr, str(val),
            'relay 2 optional' if k == 2 else 'no', 'IP 6.8.1 / OM', '')

add('0-0:10.0.107.255', 'Predefined Scripts - Image activation', 1, 2, 18,
    '0-0:44.0.0.255', 'method', 4, '?', 'no', 'IP 6.9 / OM', 'parameter and exact action not printed')

PUSH = {'1': '0-1:25.9.0.255', '2': '0-2:25.9.0.255', '3': '0-3:25.9.0.255',
        '4': '0-4:25.9.0.255', '5': '0-0:25.9.0.255', '6': '0-7:25.9.0.255',
        '7': '0-5:25.9.0.255', '8': '0-6:25.9.0.255'}
for sid, ln in PUSH.items():
    add('0-0:10.0.108.255', 'Push script table', sid, 2, 40, ln, 'method', 1, '0',
        '7/8 optional' if sid in ('7', '8') else 'no', 'IP 7.8 / OM',
        'IP table prints 715/815; 15 is a footnote, script ids are 7 and 8')

FUNC = {
    '1': '{{"LPCAP_1",true},{"LPCAP_2",true}}',
    '2': '{{"LPCAP_1",false},{"LPCAP_2",false}}',
    '3': '{{"LPCAP_1",true}}', '4': '{{"LPCAP_1",false}}',
    '5': '{{"LPCAP_2",true}}', '6': '{{"LPCAP_2",false}}',
    '7': '{{"IF_LO_2W",true}}', '8': '{{"IF_LO_2W",false}}',
}
for sid, param in FUNC.items():
    add('0-0:10.0.111.255', 'IDIS function control script table', sid, 2, 122,
        '0-0:44.1.0.255', 'method', 1, param, 'no', 'IP 6.13.4 / OM', '')

for k in range(1, 5):
    for off, kind, member, val in ((1, 'attribute', 3, 2), (2, 'attribute', 3, 1),
                                   (3, 'method', 1, 0), (4, 'method', 2, 0)):
        sid = (k - 1) * 4 + off
        add('0-1:10.0.106.255', 'M-Bus Disconnector script table', sid,
            1 if off <= 2 else 2, 70, f'0-{k}:24.4.0.255', kind, member, str(val),
            'no', 'IP 6.5.2', 'M-Bus extension')

MBUS_FUNC = {}
for i, name in enumerate(('LPCAP_M1', 'LPCAP_M2', 'LPCAP_M3', 'LPCAP_M4')):
    MBUS_FUNC[str(2 * i + 1)] = f'{{{{"{name}",true}}}}'
    MBUS_FUNC[str(2 * i + 2)] = f'{{{{"{name}",false}}}}'
for sid, param in MBUS_FUNC.items():
    add('0-1:10.0.111.255', 'M-Bus function control script table', sid, 2, 122,
        '0-1:44.1.0.255', 'method', 1, param, 'no', 'IP 6.13.5',
        'M-Bus extension')

add('0-b:10.0.110.255', 'M-Bus profile control script table', '?', '', 9, '',
    '', '', '?', 'yes', 'IP Table 7 / extension M', 'contents not printed; confirm with vendor')

with open('docs/idis/script_tables.csv', 'w', newline='', encoding='utf-8') as f:
    w = csv.DictWriter(f, fieldnames=list(ROWS[0].keys()))
    w.writeheader(); w.writerows(ROWS)

with open('docs/idis/script_tables.md', 'w', encoding='utf-8') as f:
    f.write('# IDIS P3 Script Table 全清单\n\n')
    f.write(f'共 {len(ROWS)} 条脚本动作。service_id 1 = SET，2 = ACTION。\n\n')
    last = None
    for r in ROWS:
        if r['table_obis'] != last:
            last = r['table_obis']
            f.write(f"\n## {r['table_obis']} {r['table_name']}\n\n")
            f.write('| script | service | class | target | member | param | optional | 来源 | 备注 |\n')
            f.write('|---|---|---|---|---|---|---|---|---|\n')
        f.write(f"| {r['script_id']} | {r['service_id']} | {r['target_class']} | `{r['target_obis']}` | {r['target_kind']} {r['target_member']} | {r['parameter']} | {r['optional']} | {r['source']} | {r['note']} |\n")

    f.write('\n## 待确认\n\n')
    f.write('1. MDI reset / end of billing period 脚本 1 的动作列表没有在文档里打印，只有触发关系。\n')
    f.write('2. Image activation 脚本 1 的方法参数没有在文档里打印。\n')
    f.write('3. 费率脚本表的 `*` 和 `[1..2]` 是范围占位，真实掩码要按 CTI 和费率表确认。\n')
    f.write('4. M-Bus profile control script table `0-b:10.0.110.255` 的脚本内容没有打印。\n')
    f.write('5. PUSH 脚本号 7 和 8 是可选脚本。IP profile 表里的 715/815 是 7/8 加脚注 15。\n')

print(f'{len(ROWS)} script actions -> docs/idis/script_tables.csv/.md')
