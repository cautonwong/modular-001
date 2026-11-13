#!/usr/bin/env python3
"""Build the complete OBIS inventory for IDIS P3.

Every concrete OBIS token in the object-model and IP-profile texts must appear
here once. Object-model rows, the extension sheets, the IP-profile tables and
plain-text references are merged into one row per OBIS. Duplicate OBIS with
different interface classes are flagged as conflicts instead of being dropped.
"""
import csv
import re

OM = 'docs/idis/object_model.csv'
BASIC = 'docs/idis/ip_profile_basic_objects.csv'
COMM = 'docs/idis/ip_profile_comm_objects.csv'
RAW = 'docs/idis/all_obis_raw.csv'
TEXT = 'docs/idis/IDIS-S03-001-objectmodelPack3Ed1.0-V3.txt'
OUT = 'docs/idis/all_obis.csv'
PATTERNS = 'docs/idis/obis_patterns.csv'
REPORT = 'docs/idis/obis_coverage.md'

OBIS_RE = re.compile(r'^[0-9A-F]+-[0-9A-F]+:[0-9A-F]+(\.[0-9A-F]+){3}$')


def read_csv(path):
    return list(csv.DictReader(open(path, encoding='utf-8')))


def extension_objects():
    pages = open(TEXT, encoding='utf-8', errors='replace').read().split('\f')
    out = []

    def pl(p):
        return pages[p - 1].splitlines()

    names = []
    for line in pl(54):
        m = re.match(r'^\s{2,}([A-Za-z].*?)\s{2,}(M-D|O|NA)\s+(M-D|O|NA)\s+(\d+)\s*$', line)
        if m:
            names.append((m.group(1).strip(), m.group(4)))
    obis = []
    for line in pl(55):
        m = re.match(r'^\s+(\d+)\s+([0-9A-Fa-f*]+-[0-9A-Fa-f*]+:[0-9A-Fa-f*.\-]+)\s*$', line)
        if m:
            obis.append((int(m.group(1)), m.group(2)))
    for (name, cls), (ver, ob) in zip(names, obis):
        out.append({'obis': ob, 'class_id': cls, 'version': str(ver),
                    'name': name, 'source': 'extension-D', 'flags': 'M-D'})

    names = []
    for p in (56, 57):
        for line in pl(p):
            s = line.strip()
            if not s or s.startswith(('Extension', '#')) or s.startswith('M-M='):
                continue
            names.append(s)
    obis = []
    for p in (58, 59):
        for line in pl(p):
            m = re.match(r'^\s*(M-M|O|NA)\s+(M-M|O|NA)\s+(\d+)\s+(\d+)\s+([0-9A-Fa-f*]+-[0-9A-Fa-f*]+:[0-9A-Fa-f*.\-]+)\s*$', line)
            if m:
                obis.append((int(m.group(3)), int(m.group(4)), m.group(5)))
    for name, (cls, ver, ob) in zip(names[1:], obis):
        out.append({'obis': ob, 'class_id': str(cls), 'version': str(ver),
                    'name': name, 'source': 'extension-M', 'flags': 'M-M'})
    return out


def main():
    records = []  # (obis, class_id, version, name, source, flags, status, note)

    def add(obis, cls, ver, name, source, flags, status, note=''):
        records.append({'obis': obis.upper(), 'class_id': str(cls),
                        'version': str(ver), 'name': name, 'source': source,
                        'flags': flags, 'status': status, 'note': note})

    om = {}
    for r in read_csv(OM):
        om.setdefault(r['obis'].upper(), []).append(r)
    for ob, rows in sorted(om.items()):
        classes = sorted({r['class'] for r in rows})
        names = []
        for r in rows:
            if r['object'] not in names:
                names.append(r['object'])
        add(ob, '/'.join(classes), rows[0]['version'], ' / '.join(names),
            'object-model', f"{rows[0]['m1']}/{rows[0]['m2']}",
            'conflict' if len(classes) > 1 else 'defined')

    for r in extension_objects():
        add(r['obis'], r['class_id'], r['version'], r['name'], r['source'],
            r['flags'], 'defined', 'extension sheet')

    for r in read_csv(BASIC):
        add(r['obis'], r['class_id'], '', r['name'], 'ip-profile-table-7', '', 'defined')
    for r in read_csv(COMM):
        add(r['obis'], r['class_id'], '', r['name'], 'ip-profile-table-8-11', '', 'defined')

    for ob, cls, ver, name, note in [
        ('0-0:21.0.1.255', '7', '0', 'General display', 'IP profile 6.11.2'),
        ('0-0:21.0.2.255', '7', '0', 'Alternate display', 'IP profile 6.11.2'),
        ('0-1:20.0.0.255', '19', '0', 'IEC local port setup - consumer information interface', 'IP profile 6.11.1.2.1'),
        ('0-0:96.10.4.255', '1', '0', 'Power Quality Profile status', 'IP profile Table 7; object model prints 1-0:96.10.1.255'),
        ('0-0:2.2.1.255', '29', '2', 'Auto connect (IP profile 8.3 typo)', 'IP profile 8.3; object model has 0-0:2.1.0.255'),
    ]:
        status = 'typo' if 'typo' in note else 'ip-only'
        add(ob, cls, ver, name, 'ip-profile-reference', '', status, note)

    # plain-text references that are not yet defined
    for r in read_csv(RAW):
        ob = r['obis'].upper()
        if r['form'] == 'dot':
            if ob == '0.0.40.0.0.255':
                ob = '0-0:40.0.0.255'
            else:
                continue
        if re.search(r'[X*]', ob) or re.search(r'(^|[-:])B([-:.]|$)', ob):
            continue
        if ob.endswith('.256'):
            add(ob, '', '0', 'OBIS with invalid field 256', 'text-reference',
                '', 'invalid', f'field 256 is invalid; likely {ob[:-1]}5')
        elif not any(x['obis'] == ob for x in records):
            add(ob, '', '0', r['context'][:90], 'text-reference', '', 'reference',
                f"{r['source'].split('/')[-1]} page {r['page']}")

    # one row per OBIS, merging duplicates
    merged = {}
    for r in records:
        cur = merged.get(r['obis'])
        if not cur:
            merged[r['obis']] = dict(r)
        else:
            if r['class_id'] and r['class_id'] not in cur['class_id']:
                cur['class_id'] = ','.join(x for x in [cur['class_id'], r['class_id']] if x)
                cur['status'] = 'conflict'
            if r['name'] and r['name'] not in cur['name']:
                cur['name'] = ' / '.join(x for x in [cur['name'], r['name']] if x)
            if r['source'] not in cur['source']:
                cur['source'] = cur['source'] + '+' + r['source']

    # classify document defects and add corrected rows
    extra = {}
    for r in merged.values():
        if r['obis'].endswith('.256'):
            r['status'] = 'invalid'
            r['note'] = f'field 256 is invalid; corrected row uses {r["obis"][:-1]}5'
            corrected = r['obis'][:-1] + '5'
            if corrected not in merged:
                extra[corrected] = dict(r, obis=corrected, status='defined',
                                        note='corrected from ' + r['obis'])
        if r['obis'] == '0-0:2.2.1.255':
            r['status'] = 'typo'
            r['note'] = 'IP profile 8.3 typo; object model has 0-0:2.1.0.255'
        if r['obis'] == '0-0:96.10.4.255':
            r['status'] = 'ip-only'
            r['note'] = 'object model prints 1-0:96.10.1.255; use this OBIS'
        if r['obis'] == '0-0:10.0.111.255':
            r['note'] = 'IP profile Table 7 prints IC 19; object model and the name require IC 9'
    merged.update(extra)
    rows = sorted(merged.values(), key=lambda r: r['obis'])
    with open(OUT, 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=['obis', 'class_id', 'version', 'name',
                                          'source', 'flags', 'status', 'note'])
        w.writeheader()
        w.writerows(rows)

    with open('docs/idis/obis_inventory.md', 'w', encoding='utf-8') as f:
        f.write('# IDIS P3 全部 OBIS 清单\n\n')
        f.write(f'共 {len(rows)} 个唯一具体 OBIS。每行都来自两份 IDIS 文本，合并了对象表、扩展表和 IP profile。\n\n')
        f.write('| OBIS | IC | 名称 | 来源 | 104/105 | 状态 | 备注 |\n|---|---|---|---|---|---|---|\n')
        for r in rows:
            f.write(f"| `{r['obis']}` | {r['class_id']} | {r['name']} | {r['source']} | {r['flags']} | {r['status']} | {r['note']} |\n")

    patterns = []
    for r in read_csv(RAW):
        ob = r['obis'].upper()
        if re.search(r'[X*]', ob) or re.search(r'(^|[-:])B([-:.]|$)', ob):
            key = (ob, r['source'].split('/')[-1], r['page'])
            if key not in patterns:
                patterns.append(key)
    with open(PATTERNS, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['pattern', 'source', 'page'])
        w.writerows(patterns)

    def count(pred):
        return sum(1 for r in rows if pred(r))

    with open(REPORT, 'w', encoding='utf-8') as f:
        f.write('# IDIS P3 OBIS 覆盖报告\n\n')
        f.write('冲突、非法字段和拼写错误都是备注，不是最终裁决。开发时按计划 1.4 节的工作假设推进，等供应商确认后改表项。\n\n')
        f.write(f'- 全局唯一 OBIS：{len(rows)}\n')
        f.write(f'- 对象模型主表：{count(lambda r: "object-model" in r["source"])}\n')
        f.write(f'- 扩展 D/M：{count(lambda r: "extension" in r["source"])}\n')
        f.write(f'- IP profile 表 7：{count(lambda r: "table-7" in r["source"])}\n')
        f.write(f'- IP profile 表 8-11：{count(lambda r: "table-8-11" in r["source"])}\n')
        f.write(f'- 仅 IP profile：{count(lambda r: r["status"] == "ip-only")}\n')
        f.write(f'- 冲突：{count(lambda r: r["status"] == "conflict")}\n')
        f.write(f'- 非法字段：{count(lambda r: r["status"] == "invalid")}\n')
        f.write(f'- 拼写错误：{count(lambda r: r["status"] == "typo")}\n')
        f.write(f'- 未归类引用：{count(lambda r: r["status"] == "reference")}\n')
        f.write(f'- 通配模式：{len(patterns)}\n\n')

        for title, status in [('冲突 OBIS', 'conflict'), ('非法字段', 'invalid'),
                              ('拼写错误', 'typo'), ('仅 IP profile', 'ip-only'),
                              ('未归类引用', 'reference')]:
            f.write(f'## {title}\n\n')
            for r in rows:
                if r['status'] == status:
                    f.write(f"- `{r['obis']}` IC {r['class_id']} {r['name']} ({r['note']})\n")
            f.write('\n')

        f.write('## 通配模式\n\n')
        for p, s, pg in patterns:
            f.write(f"- `{p}` {s} page {pg}\n")

    raw_concrete = set()
    for r in read_csv(RAW):
        ob = r['obis'].upper()
        if r['form'] == 'dot':
            if ob == '0.0.40.0.0.255':
                ob = '0-0:40.0.0.255'
            else:
                continue
        if re.search(r'[X*]', ob) or re.search(r'(^|[-:])B([-:.]|$)', ob):
            continue
        raw_concrete.add(ob)
    missing = raw_concrete - set(merged)
    extra = set(merged) - raw_concrete
    if missing or extra:
        print('coverage mismatch')
        print('  missing:', sorted(missing))
        print('  extra:', sorted(extra))
        raise SystemExit(1)
    print(f'{len(rows)} OBIS -> {OUT}; {len(patterns)} patterns -> {PATTERNS}; report -> {REPORT}')
    print('coverage check: every concrete OBIS token in both IDIS texts is present')
    for s in sorted({r['status'] for r in rows}):
        print(s, count(lambda r: r['status'] == s))


if __name__ == '__main__':
    main()
