# -*- coding: utf-8 -*-
"""dump_glyphs.py - render glyphs from font_cjk.cpp as ASCII art to verify
the bitmap data itself is correct (finds generator bit-order bugs)."""
import io
import re
import sys

sys.stdout.reconfigure(encoding="utf-8")

src = io.open("src/common/base/font_cjk.cpp", encoding="utf-8").read()

count = int(re.search(r"_FontCjkCount\s*=\s*(\d+)", src).group(1))

def grab(name):
    m = re.search(r"_FontCjk%s\[[0-9]*\]\s*=\s*\{(.*?)\};" % name, src, re.S)
    body = m.group(1)
    return [int(x, 16) if x.lower().startswith("0x") else int(x)
            for x in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]

cp_sorted = grab("CpSorted")
cp_idx    = grab("IdxOfCp")
gbk_sorted = grab("GbkSorted")
gbk_idx    = grab("IdxOfGbk")

# glyph bit rows
mbits = re.search(r"_FontCjkBits\[[0-9]*\]\[[0-9]*\]\s*=\s*\{(.*?)\};", src, re.S)
rows = re.findall(r"\{((?:0x[0-9A-Fa-f]{2}\s*,\s*)*0x[0-9A-Fa-f]{2})\}", mbits.group(1))

def lookup(sorted_arr, idx_arr, key):
    lo, hi = 0, len(sorted_arr) - 1
    while lo <= hi:
        mid = (lo + hi) >> 1
        if sorted_arr[mid] == (key & 0xFFFF):
            return idx_arr[mid]
        if sorted_arr[mid] < key:
            lo = mid + 1
        else:
            hi = mid - 1
    return -1

def render(gi):
    row = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", rows[gi])]
    out = []
    for r in range(12):
        lo = row[r * 2]
        hi = row[r * 2 + 1]
        bits = lo | (hi << 8)
        line = ""
        for c in range(11):
            line += "#" if (bits & (1 << (10 - c))) else "."
        out.append(line)
    return out

def show(ch):
    cp = ord(ch)
    g = ch.encode("gbk")
    gbk = (g[0] << 8) | g[1]
    gi1 = lookup(cp_sorted, cp_idx, cp)
    gi2 = lookup(gbk_sorted, gbk_idx, gbk)
    print("=== %s  U+%04X  gbk=0x%04X  gi(cp)=%s  gi(gbk)=%s" % (ch, cp, gbk, gi1, gi2))
    if gi1 >= 0:
        for line in render(gi1):
            print("   " + line)
    if gi1 != gi2:
        print("  NOTE: cp/gbk glyph indices differ! gbk view:")
        if gi2 >= 0:
            for line in render(gi2):
                print("   " + line)

for ch in "马里奥传说超级世界":
    show(ch)
