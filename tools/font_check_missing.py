# -*- coding: utf-8 -*-
"""Check which CJK chars used in source UI strings are missing from the
generated font_cjk.cpp atlas tables."""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def atlas_cps():
    """Codepoints present in the generated atlas (from _FontCjkCpSorted)."""
    src = os.path.join(ROOT, "src", "common", "base", "font_cjk.cpp")
    if not os.path.exists(src):
        sys.exit("font_cjk.cpp not found")
    data = open(src, encoding="utf-8").read()
    m = re.search(r"_FontCjkCpSorted\[\d+\]\s*=\s*\{(.*?)\};", data, re.S)
    if not m:
        sys.exit("CpSorted table not found")
    return set(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{4})", m.group(1)))

def scan_source_files():
    """Collect every char appearing in string literals of .cpp/.h files."""
    chars = {}   # cp -> [file:line]
    str_re = re.compile(r'"((?:[^"\\]|\\.)*)"')
    for base, dirs, files in os.walk(os.path.join(ROOT, "src")):
        # skip emulator-core trees: their intl headers carry French/Spanish
        # text that never reaches the PS2 UI
        rel = os.path.relpath(base, os.path.join(ROOT, "src")).replace("\\", "/")
        if rel.startswith(("cores/", "common/libretro")) or "/libretro" in rel:
            dirs[:] = []
            continue
        for fn in files:
            if not fn.endswith((".cpp", ".c", ".h", ".hpp")):
                continue
            if "intl" in fn or "translation" in fn:
                continue
            path = os.path.join(base, fn)
            raw = open(path, "rb").read()
            # try utf-8, then gbk
            for enc in ("utf-8", "gbk"):
                try:
                    text = raw.decode(enc)
                    break
                except UnicodeDecodeError:
                    text = None
            if text is None:
                continue
            for i, line in enumerate(text.splitlines(), 1):
                # skip pure comment lines to reduce noise (rough)
                for m in str_re.finditer(line):
                    s = m.group(1)
                    for ch in s:
                        cp = ord(ch)
                        # CJK + fullwidth + symbols the UI actually draws
                        if (0x2E80 <= cp <= 0x9FFF or    # CJK radicals..Yi
                            0xFF00 <= cp <= 0xFFEF or    # fullwidth forms
                            0x2000 <= cp <= 0x206F or    # general punct
                            0x3000 <= cp <= 0x30FF or    # CJK punct + kana
                            cp in (0x00B7, 0x00A9, 0x00AE, 0x2122, 0x00BD)):
                            chars.setdefault(cp, []).append(
                                f"{fn}:{i}")
    return chars

cps = atlas_cps()
usage = scan_source_files()
missing = {cp: locs for cp, locs in usage.items() if cp not in cps}

print(f"atlas glyphs: {len(cps)}")
print(f"distinct non-ascii chars used in strings: {len(usage)}")
print(f"missing from atlas: {len(missing)}")
for cp in sorted(missing):
    ch = chr(cp)
    gbk = ""
    try:
        gbk = " gbk=" + ch.encode("gbk").hex().upper()
    except Exception:
        gbk = " (not in GBK)"
    print(f"  U+{cp:04X} {ch}{gbk}  x{len(missing[cp])}  "
          f"e.g. {missing[cp][:3]}")
