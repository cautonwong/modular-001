#!/usr/bin/env python3
"""Check every script action target against the OBIS inventory and member matrix."""
import csv
from collections import defaultdict

OUT = 'docs/idis/script_target_check.md'


def main():
    obis = {r['obis']: r for r in csv.DictReader(open('docs/idis/all_obis.csv', encoding='utf-8'))}
    members = defaultdict(set)
    for r in csv.DictReader(open('docs/idis/obis_members.csv', encoding='utf-8')):
        members[r['obis']].add((r['kind'], r['id']))
    problems = []
    for r in csv.DictReader(open('docs/idis/script_tables.csv', encoding='utf-8')):
        target = r['target_obis'].strip('`')
        if not target or '?' in target or target.startswith('0-b'):
            continue
        if target not in obis:
            problems.append((r['table_obis'], r['script_id'], target, 'OBIS not in inventory'))
            continue
        cls = r['target_class']
        if cls and obis[target]['class_id'] != cls:
            problems.append((r['table_obis'], r['script_id'], target,
                             f'class {cls} != inventory {obis[target]["class_id"]}'))
        kind = r['target_kind']
        member = r['target_member']
        if kind in ('attribute', 'method') and member:
            if (kind, member) not in members[target]:
                problems.append((r['table_obis'], r['script_id'], target,
                                 f'{kind} {member} not in member matrix'))
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write('# Script 目标检查\n\n')
        if not problems:
            f.write('所有具体脚本目标都在 OBIS 清单和成员矩阵中。\n')
        else:
            for p in problems:
                f.write(f'- {p}\n')
    print(f'{len(problems)} problems -> {OUT}')
    for p in problems[:20]:
        print(p)


if __name__ == '__main__':
    main()
