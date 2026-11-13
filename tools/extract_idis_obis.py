#!/usr/bin/env python3
"""Extract every OBIS-like token from the IDIS texts with page and context."""
import csv
import re
import sys

OBIS = re.compile(r'(?<![\w.])((?:[0-9A-Fa-f*xX]+)-(?:[0-9A-Fa-f*xX]+):(?:[0-9A-Fa-f*xX]+)\.(?:[0-9A-Fa-f*xX]+)\.(?:[0-9A-Fa-f*xX]+)\.(?:[0-9A-Fa-f*xX]+))(?![\w.])')
DOTTED = re.compile(r'(?<![\w.])(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})(?![\w.])')


def main(paths, out):
    rows = []
    for path in paths:
        text = open(path, encoding='utf-8', errors='replace').read()
        for pno, page in enumerate(text.split('\f'), 1):
            for line in page.splitlines():
                for m in OBIS.finditer(line):
                    rows.append((path, pno, m.group(1).upper(), 'dash', line.strip()))
                for m in DOTTED.finditer(line):
                    rows.append((path, pno, m.group(1), 'dot', line.strip()))
    with open(out, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['source', 'page', 'obis', 'form', 'context'])
        w.writerows(rows)
    print(f'{len(rows)} tokens -> {out}')
    print('unique dash:', len({r[2] for r in rows if r[3] == 'dash'}))
    print('unique dotted:', len({r[2] for r in rows if r[3] == 'dot'}))


if __name__ == '__main__':
    main(sys.argv[1:-1], sys.argv[-1])
