# -*- coding: utf-8 -*-
"""verify_cjk_lookup.py - Simulate the C++ binary-search lookups against
font_cjk.cpp data to find why only the first CJK char renders."""
import io
import re
import sys

sys.stdout.reconfigure(encoding="utf-8")

src = io.open("src/common/base/font_cjk.cpp", encoding="utf-8").read()

def grab_array(name):
    m = re.search(r"_FontCjk%s\[[0-9]*\]\s*=\s*\{(.*?)\};" % name, src, re.S)
    if not m:
        raise SystemExit("array %s not found" % name)
    body = m.group(1)
    vals = [int(x, 16) if x.lower().startswith("0x") else int(x)
            for x in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]
    return vals

cp_sorted  = grab_array("CpSorted")
cp_idx     = grab_array("IdxOfCp")
gbk_sorted = grab_array("GbkSorted")
gbk_idx    = grab_array("IdxOfGbk")
bits       = re.findall(r"\{((?:0x[0-9A-Fa-f]{2}\s*,\s*)*0x[0-9A-Fa-f]{2})\}", src)
count      = int(re.search(r"_FontCjkCount\s*=\s*(\d+)", src).group(1))

print("count=%d  cp_sorted=%d  cp_idx=%d  gbk_sorted=%d  gbk_idx=%d  glyph_rows=%d"
      % (count, len(cp_sorted), len(cp_idx), len(gbk_sorted), len(gbk_idx), len(bits)))

def check_sorted(arr, name):
    bad = [i for i in range(1, len(arr)) if arr[i] < arr[i-1]]
    if bad:
        print("%s: NOT SORTED - %d violations, first at idx %d (%04X after %04X)"
              % (name, len(bad), bad[0], arr[bad[0]], arr[bad[0]-1]))
    else:
        print("%s: sorted OK" % name)

check_sorted(cp_sorted, "CpSorted")
check_sorted(gbk_sorted, "GbkSorted")

# exact copy of the C++ binary search
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

def glyph_blank(gi):
    if gi < 0 or gi >= len(bits):
        return None
    row = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", bits[gi])]
    # 12 rows x 2 bytes
    if len(row) < 24:
        return None
    return all(b == 0 for b in row)

tests = "马里奥传说天地无双合集世界超级魂斗罗勇者斗恶龙塞尔达幻想三国志西游记封神榜恶魔城洛克人魂斗"
missing_cp, missing_gbk, blank = [], [], []
for ch in tests:
    cp = ord(ch)
    gbk = ch.encode("gbk")[0] << 8 | ch.encode("gbk")[1]
    gi1 = lookup(cp_sorted, cp_idx, cp)
    gi2 = lookup(gbk_sorted, gbk_idx, gbk)
    if gi1 < 0:
        missing_cp.append(ch)
    if gi2 < 0:
        missing_gbk.append(ch)
    if gi1 >= 0 and glyph_blank(gi1):
        blank.append(ch + "(cp)")

print("\nUnicode lookup missing : %s" % (" ".join(missing_cp) or "none"))
print("GBK lookup missing     : %s" % (" ".join(missing_gbk) or "none"))
print("Blank glyphs           : %s" % (" ".join(blank) or "none"))

# also: does gbk table actually contain the classic GBK double-byte code form?
probe = "马"
g = probe.encode("gbk")
print("\n'马' unicode U+%04X gbk=0x%02X%02X -> expect sorted arrays contain 0x%04X and 0x%04X"
      % (ord(probe), g[0], g[1], ord(probe), (g[0] << 8) | g[1]))
