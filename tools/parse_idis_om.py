#!/usr/bin/env python3
"""Parse the IDIS P3 object-model PDF text (pdftotext -layout) into CSV.

The worksheet repeats a header on every page. Column x-positions differ per
page, so each page is sliced using its own header line.
"""
import csv
import re
import sys

HEADER = "Object / Attribute Name"
COLS = ["Attribute Type", "Class", "Ver.", "OBIS code / Default Value",
        "Public (16)", "Preestablished (102)", "Management (1)"]
OBIS_RE = re.compile(r"[0-9A-Fa-f*]+-[0-9A-Fa-f*]+:[0-9A-Fa-f*.\-]+")
OBIS_FULL = re.compile(r"^[0-9A-Fa-f*]+-[0-9A-Fa-f*]+:[0-9A-Fa-f*.\-]+$")
FLAG_RE = re.compile(r"^(M|O|NA)(-[A-Za-z0-9]+)?$")


def header_positions(page_lines):
    for line in page_lines:
        if HEADER in line and "Class" in line and "Ver." in line:
            pos = {}
            for c in COLS:
                i = line.find(c)
                if i >= 0:
                    pos[c] = i
            # wrapped header: look one line below for the missing right columns
            if "Management (1)" not in pos:
                for nxt in page_lines:
                    i = nxt.find("Management (1)")
                    if i >= 0:
                        pos["Management (1)"] = i
                        break
            if "Public (16)" not in pos or "Preestablished (102)" not in pos:
                return None
            return pos
    return None


def cell(line, pos, name, end=None):
    start = pos[name]
    stop = end if end is not None else len(line)
    return line[start:stop].strip()


def parse_page(lines, page_no, rows, state):
    pos = header_positions(lines)
    if not pos:
        return
    order = ["Attribute Type", "Class", "Ver.", "OBIS code / Default Value",
             "Public (16)", "Preestablished (102)", "Management (1)"]
    pos["name"] = 0
    for a, b in zip(order, order[1:] + [None]):
        if b:
            pos["end_" + a] = pos[b]
    current = state.get('current')
    for line in lines:
        if not line.strip():
            continue
        name_cell = line[0:pos["Attribute Type"]].strip()
        cls = cell(line, pos, "Class", pos["Ver."])
        ver = cell(line, pos, "Ver.", pos["OBIS code / Default Value"])
        ver_num = ver.split()[0] if ver.split() else ""
        obis = cell(line, pos, "OBIS code / Default Value", pos["Public (16)"])
        pub = cell(line, pos, "Public (16)", pos["Preestablished (102)"])
        pre = cell(line, pos, "Preestablished (102)", pos["Management (1)"])
        mgmt = cell(line, pos, "Management (1)")
        if not name_cell and not cls and not obis:
            continue
        if cls.isdigit() and ver_num.isdigit() and OBIS_RE.search(obis):
            parts = [p for p in re.split(r"\s{2,}", name_cell) if p]
            flags = []
            while parts and FLAG_RE.match(parts[-1]):
                flags.insert(0, parts.pop())
            oname = " ".join(parts)
            current = {"page": page_no, "object": oname, "class": int(cls),
                       "version": int(ver_num), "obis": OBIS_RE.search(obis).group(0),
                       "m1": flags[0] if len(flags) > 0 else "",
                       "m2": flags[1] if len(flags) > 1 else "",
                       "attributes": []}
            state['current'] = current
            rows.append(current)
        elif name_cell and re.match(r"^\d+\s", name_cell) and current:
            parts = name_cell.split(None, 1)
            idx = int(parts[0])
            aname = parts[1].strip() if len(parts) > 1 else ""
            if idx < 1 or idx > 99 or not aname or not re.match(r"[A-Za-z_]", aname):
                continue
            current["attributes"].append({
                "index": idx, "name": aname,
                "type": cell(line, pos, "Attribute Type", pos["Class"]),
                "tag": obis, "public": pub, "preest": pre, "mgmt": mgmt,
            })


def main(path, out):
    pages = open(path, encoding="utf-8", errors="replace").read().split("\f")
    rows = []
    state = {'current': None}
    for i, page in enumerate(pages, 1):
        parse_page(page.splitlines(), i, rows, state)
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["page", "object", "class", "version", "obis", "m1", "m2",
                    "attr_index", "attr_name", "attr_type", "attr_tag",
                    "public", "preest", "mgmt"])
        for o in rows:
            if o["attributes"]:
                for a in o["attributes"]:
                    w.writerow([o["page"], o["object"], o["class"], o["version"],
                                o["obis"], o["m1"], o["m2"], a["index"],
                                a["name"], a["type"], a["tag"], a["public"],
                                a["preest"], a["mgmt"]])
            else:
                w.writerow([o["page"], o["object"], o["class"], o["version"],
                            o["obis"], o["m1"], o["m2"], "", "", "", "", "", "", ""])
    print(f"{len(rows)} objects -> {out}")
    classes = {}
    for o in rows:
        classes[o["class"]] = classes.get(o["class"], 0) + 1
    print("class ids:", sorted(classes.items()))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
