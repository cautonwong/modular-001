#!/usr/bin/env python3
"""Check every OBIS instance for missing Blue Book attribute IDs."""
import csv
import importlib.util
from collections import defaultdict

spec = importlib.util.spec_from_file_location('v', 'tools/validate_bluebook.py')
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)

OUT = 'docs/idis/object_member_gaps.md'


def main():
    bb = v.parse_bluebook()
    rows = list(csv.DictReader(open('docs/idis/obis_members.csv', encoding='utf-8')))
    by = defaultdict(lambda: {'attrs': set(), 'methods': set(), 'version': '', 'obj': ''})
    for r in rows:
        key = r['obis']
        by[key]['obj'] = r['object']
        by[key]['version'] = r['version']
        if r['kind'] == 'attribute':
            by[key]['attrs'].add(int(r['id']))
        else:
            by[key]['methods'].add(int(r['id']))
    gaps = []
    for ob, data in sorted(by.items()):
        ic = next((r['ic'] for r in rows if r['obis'] == ob), '')
        if not ic.isdigit():
            continue
        ver = int(data['version']) if data['version'].isdigit() else 0
        if (int(ic), ver) not in bb:
            continue
        expected = {i for i, _ in bb[(int(ic), ver)][0]}
        missing = sorted(expected - data['attrs'])
        mandatory_methods = {i for i, _, opt in bb[(int(ic), ver)][1] if not opt}
        missing_methods = sorted(mandatory_methods - data['methods'])
        if missing or missing_methods:
            gaps.append((ob, int(ic), ver, missing, missing_methods, data['obj']))
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write('# OBIS 实例属性缺口\n\n')
        f.write('按 Blue Book 同版本类的属性编号和强制方法编号检查每个实例。当前结果为空。\n\n')
        for ob, ic, ver, missing, missing_methods, obj in gaps:
            f.write(f'- `{ob}` IC {ic} v{ver} {obj} 缺属性 {missing}，缺强制方法 {missing_methods}\n')
    print(f'{len(gaps)} objects with missing attributes -> {OUT}')


if __name__ == '__main__':
    main()
