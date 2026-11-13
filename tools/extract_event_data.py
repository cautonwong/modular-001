#!/usr/bin/env python3
"""Extract the IP profile event-code list and the alarm-register bit map."""
import csv
import re

TEXT = 'docs/idis/IDIS-S03-001Pack3IPprofile-Xed1.0HexingElectrical.txt'
EVENTS = 'docs/idis/event_codes.csv'
ALARMS = 'docs/idis/alarm_bits.csv'

ALARM1 = [
    (0, 'Clock invalid', '6'), (1, 'Battery replace', '7'),
    (8, 'Program memory error', '12'), (9, 'RAM error', '13'),
    (10, 'NV memory error', '14'), (11, 'Measurement system error', '16'),
    (12, 'Watchdog error', '15'), (13, 'Fraud attempt', '40,42,44,46,49,50'),
    (16, 'M-Bus communication error ch1', '100'),
    (17, 'M-Bus communication error ch2', '110'),
    (18, 'M-Bus communication error ch3', '120'),
    (19, 'M-Bus communication error ch4', '130'),
    (20, 'M-Bus fraud attempt ch1', '103'), (21, 'M-Bus fraud attempt ch2', '113'),
    (22, 'M-Bus fraud attempt ch3', '123'), (23, 'M-Bus fraud attempt ch4', '133'),
    (24, 'Permanent error M-Bus ch1', '106'), (25, 'Permanent error M-Bus ch2', '116'),
    (26, 'Permanent error M-Bus ch3', '126'), (27, 'Permanent error M-Bus ch4', '136'),
    (28, 'Battery low on M-Bus ch1', '102'), (29, 'Battery low on M-Bus ch2', '112'),
    (30, 'Battery low on M-Bus ch3', '122'), (31, 'Battery low on M-Bus ch4', '132'),
]
ALARM2 = [
    (0, 'Total Power Failure', '1'), (1, 'Power Resume', '2'),
    (2, 'Voltage Missing Phase L1', '82'), (3, 'Voltage Missing Phase L2', '83'),
    (4, 'Voltage Missing Phase L3', '84'), (5, 'Voltage Normal Phase L1', '85'),
    (6, 'Voltage Normal Phase L2', '86'), (7, 'Voltage Normal Phase L3', '87'),
    (8, 'Missing Neutral', '89'), (9, 'Phase Asymmetry', '90'),
    (10, 'Current Reversal', '91'), (11, 'Wrong Phase Sequence', '88'),
    (12, 'Unexpected Consumption', '52'), (13, 'Key Exchanged', '48'),
    (14, 'Bad Voltage Quality L1', '92'), (15, 'Bad Voltage Quality L2', '93'),
    (16, 'Bad Voltage Quality L3', '94'), (17, 'External Alert', '20'),
    (18, 'Local communication established', '158'),
    (19, 'New M-Bus Device Installed Ch1', '105'),
    (20, 'New M-Bus Device Installed Ch2', '115'),
    (21, 'New M-Bus Device Installed Ch3', '125'),
    (22, 'New M-Bus Device Installed Ch4', '135'),
    (27, 'M-Bus valve alarm Ch1', '164'), (28, 'M-Bus valve alarm Ch2', '174'),
    (29, 'M-Bus valve alarm Ch3', '184'), (30, 'M-Bus valve alarm Ch4', '194'),
    (31, 'Disconnect/Reconnect Failure', '68'),
]


def events():
    lines = open(TEXT, encoding='utf-8', errors='replace').read().splitlines()
    start = next(i for i, l in enumerate(lines) if l.strip() == 'Event Code' and i > 6900)
    end = next(i for i, l in enumerate(lines) if i > start and l.strip().startswith('11. Appendix'))
    pat = re.compile(r'^\s*(\d{1,4})\s+([A-Za-z][^\s].*?)\s{2,}')
    found = {}
    for i in range(start, end):
        m = pat.match(lines[i])
        if m:
            code = int(m.group(1))
            name = m.group(2).strip()
            if 1 <= code <= 999 and not name.startswith(('Event Code', 'Name')):
                found.setdefault(code, (name, i + 1))
            continue
        solo = re.match(r'^\s*(\d{1,4})\s*$', lines[i])
        if solo:
            code = int(solo.group(1))
            if not (1 <= code <= 999):
                continue
            name = ''
            for back in range(i - 1, max(start, i - 4), -1):
                text = lines[back].strip()
                if text and re.match(r'^[A-Za-z]', text) and 'Indicates' not in text:
                    name = text
                    break
            if name:
                found.setdefault(code, (name, i + 1))
    return [(c, n, p) for c, (n, p) in sorted(found.items())]


def main():
    ev = events()
    # The table prints the name of code 106 on the line above the code.
    if 106 not in {c for c, _, _ in ev}:
        ev.append((106, 'Permanent Error M-Bus channel 1', 7744))
        ev.sort()
    with open(EVENTS, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['code', 'name', 'source_line'])
        w.writerows(ev)
    with open(ALARMS, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['register', 'bit', 'alarm', 'event_codes'])
        for bit, name, codes in ALARM1:
            w.writerow([1, bit, name, codes])
        for bit, name, codes in ALARM2:
            w.writerow([2, bit, name, codes])
    print(f'{len(ev)} event codes -> {EVENTS}; {len(ALARM1)+len(ALARM2)} alarm bits -> {ALARMS}')
    missing = [i for i in range(1, max(c for c, _, _ in ev) + 1) if i not in {c for c, _, _ in ev}]
    print('codes not printed:', missing)


if __name__ == '__main__':
    main()
