#!/usr/bin/env python3
"""Compare IDIS object members with the Blue Book class definitions."""
import csv
import os
import re
import sys
from collections import defaultdict

BB = 'docs/idis/Blue-Book-Ed14.txt'
MEMBERS = 'docs/idis/obis_members.csv'
OUT = 'docs/idis/bluebook_class_check.md'
BASELINE = 'tools/bluebook_baseline.txt'

HEADER = re.compile(r'class_id\s*=\s*(\d+),\s*version\s*=\s*(\d+)')
NUMBERED = re.compile(r'^\s*(\d+)\.\s+([A-Za-z_][A-Za-z0-9_ ]*?)(?:\s*\(data\))?\s{2,}(.*)$')


def parse_bluebook():
    lines = open(BB, encoding='utf-8', errors='replace').read().splitlines()
    classes = {}
    i = 0
    while i < len(lines):
        m = HEADER.search(lines[i])
        if not m or 'class_id' not in lines[i] or not re.search(r'0[.…]+n', lines[i]):
            i += 1
            continue
        ic, ver = int(m.group(1)), int(m.group(2))
        attrs, methods = [], []
        mode = None
        i += 1
        while i < len(lines):
            line = lines[i]
            if HEADER.search(line) and re.search(r'0[.…]+n', line):
                break
            if re.match(r'^\s*(Attribute description|Method description)', line):
                break
            if re.match(r'^\s*Attributes\b', line):
                mode = 'attr'
            elif re.match(r'^\s*Specific methods\b', line):
                mode = 'method'
            else:
                m2 = NUMBERED.match(line)
                if m2 and mode:
                    mid, name, rest = int(m2.group(1)), m2.group(2).strip(), m2.group(3)
                    name = re.sub(r'\s*\(data\)\s*$', '', name).strip()
                    if mode == 'attr':
                        attrs.append((mid, name))
                    else:
                        methods.append((mid, name, bool(re.search(r'(^|\s)o(\s|$)', rest))))
            i += 1
        classes.setdefault((ic, ver), (attrs, methods))
    return classes


def member_sets():
    rows = list(csv.DictReader(open(MEMBERS, encoding='utf-8')))
    by = defaultdict(lambda: {'attr': set(), 'method': set()})
    for r in rows:
        ic = r['ic'].split(',')[0]
        if not ic.isdigit():
            continue
        ver = int(r['version']) if r['version'].isdigit() else 0
        kind = 'method' if r['kind'] == 'method' else 'attr'
        by[(int(ic), ver)][kind].add((int(r['id']), r['member']))
    return by


def load_baseline(path):
    if not os.path.exists(path):
        return set()
    entries = set()
    for line in open(path, encoding='utf-8'):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2 or not all(part.isdigit() for part in parts):
            raise SystemExit(f'{path}: bad baseline line {line!r}')
        entries.add((int(parts[0]), int(parts[1])))
    return entries


def main():
    bb = parse_bluebook()
    ids = member_sets()
    baseline = load_baseline(BASELINE)
    differing = set()
    out = ['# IDIS 成员矩阵与 Blue Book 类定义对照', '',
           '按接口类版本逐项对照。Blue Book 标为可选的 method 可以被 IDIS 省略。', '']
    problems = 0
    for key in sorted(ids):
        if key not in bb:
            out.append(f'## IC {key[0]} v{key[1]}\n\nBlue Book 没有这个版本的类定义，跳过。\n')
            continue
        battrs, bmethods = bb[key]
        ia, im = ids[key]['attr'], ids[key]['method']
        ba = {i: n for i, n in battrs}
        bm = {i: (n, opt) for i, n, opt in bmethods}
        ia_names = defaultdict(set)
        for i, n in ia:
            ia_names[i].add(n)
        im_names = defaultdict(set)
        for i, n in im:
            im_names[i].add(n)
        missing_a = sorted(set(ba) - set(ia_names))
        missing_m = sorted(set(bm) - set(im_names))
        extra_a = sorted(set(ia_names) - set(ba))
        extra_m = sorted(set(im_names) - set(bm))
        name_diff = []
        for i in sorted(set(ba) & set(ia_names)):
            if ba[i] not in ia_names[i]:
                name_diff.append((i, ba[i], sorted(ia_names[i])))
        for i in sorted(set(bm) & set(im_names)):
            if bm[i][0] not in im_names[i]:
                name_diff.append((i, bm[i][0], sorted(im_names[i])))
        if not (missing_a or missing_m or extra_a or extra_m or name_diff):
            continue
        problems += 1
        differing.add(key)
        out.append(f'## IC {key[0]} v{key[1]}')
        if missing_a:
            out.append(f'- Blue Book 有、矩阵没有的属性：{[(i, ba[i]) for i in missing_a]}')
        if missing_m:
            out.append(f'- Blue Book 有、矩阵没有的方法：{[(i, bm[i][0], "optional" if bm[i][1] else "mandatory") for i in missing_m]}')
        if extra_a:
            out.append(f'- 矩阵有、Blue Book 没有的属性：{[(i, sorted(ia_names[i])) for i in extra_a]}')
        if extra_m:
            out.append(f'- 矩阵有、Blue Book 没有的方法：{[(i, sorted(im_names[i])) for i in extra_m]}')
        if name_diff:
            out.append(f'- 同编号不同名字：{name_diff}')
        out.append('')
    open(OUT, 'w', encoding='utf-8').write('\n'.join(out))
    unexpected = sorted(differing - baseline)
    stale = sorted(baseline - differing)
    print(f'compared {len(ids)} IDIS class versions; {problems} with differences '
          f'({len(baseline & differing)} baselined, {len(unexpected)} unexpected, '
          f'{len(stale)} stale) -> {OUT}')
    for key in unexpected:
        print(f'unexpected Blue Book difference: IC {key[0]} v{key[1]}')
    for key in stale:
        print(f'stale baseline entry: IC {key[0]} v{key[1]}')
    return 1 if unexpected or stale else 0


if __name__ == '__main__':
    sys.exit(main())
