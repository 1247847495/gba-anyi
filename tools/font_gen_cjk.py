# -*- coding: utf-8 -*-
"""font_gen_cjk.py - Generate the CJK bitmap font atlas source (font_cjk.cpp)
for SNESticle Aurora's Chinese UI.

Renders 12px SimSun bitmap glyphs (the classic crisp 12x12 CJK pixel grid),
packs them into 93 cols x 85 rows of 11x12 cells (atlas 1024x1024, PSMT4,
full GB2312 coverage), and emits:

    src/common/base/font_cjk.cpp
      _FontCjkCount      number of glyphs
      _FontCjkCp[]       Unicode codepoints, ascending (for UTF-8 lookup)
      _FontCjkGbk[]      GBK double-byte codes, ascending (for GBK filenames)
      _FontCjkBits[]     N * 24 bytes: 12 rows x 2 bytes (11 px used per row)

Run:  python tools/font_gen_cjk.py
"""

import os
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("PIL/Pillow required: pip install pillow")

FONT_PATH = r"C:\Windows\Fonts\simsun.ttc"
OUT_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "src", "common", "base", "font_cjk.cpp")

# ---------------------------------------------------------------- charset --
# FULL GB2312: symbol zones 1-9 (punctuation / fullwidth ASCII / kana /
# greek / cyrillic / radicals) + all level-1 hanzi (zones 16-55) + all
# level-2 hanzi (zones 56-87).  7445 valid codes total; any code the
# gb2312 codec rejects is simply skipped.  Lookup is binary search, grid
# position = index order, so the enumeration order only affects readability.
def gb2312_all():
    chars = []
    for hi in range(0xA1, 0xF8):
        if 0xAA <= hi <= 0xAF:      # zones 10-15 are empty in GB2312
            continue
        for lo in range(0xA1, 0xFF):
            try:
                ch = bytes([hi, lo]).decode("gb2312")
                chars.append(ch)
            except UnicodeDecodeError:
                pass
    return chars

# Punctuation / kana / symbols needed early by the menus (most already
# inside GB2312; dedup keeps whichever hits first).
CJK_PUNCT = "，。、：；！？“”‘’（）《》〈〉【】〔〕…—·～￥「」『』・"
ROMAN_NUM = "ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩⅪⅫ"
SYMBOLS = "★☆♪→←↑↓①②③④⑤⑥⑦⑧⑨⑩℃†°□■△▲○●◎◇◆"

# Chars NOT in GB2312 that still show up in Chinese ROM names (voiced
# kana variants, etc.).
EXTRA_CHARS = "ゔヵヶ゛゜"

def build_charset():
    chars = []
    seen = set()
    def add(s):
        for ch in s:
            if ch not in seen and not ch.isspace():
                seen.add(ch)
                chars.append(ch)
    add(CJK_PUNCT)      # punctuation first (menu text needs it)
    add(ROMAN_NUM)
    add(SYMBOLS)
    add(EXTRA_CHARS)
    add(gb2312_all())
    return chars

# ----------------------------------------------------------------- render --
CELL_W, CELL_H = 11, 12
# 93 cols x 85 rows = 7905 cells (>= 7445 GB2312 glyphs + extras).
# 93 * 11 = 1023 px wide, 85 * 12 = 1020 px tall -> 1024x1024 atlas.
# font.cpp uploads it as a PSMT4 texture (512 KiB, same VRAM as the old
# 512x1024 PSMT8 atlas).
COLS, ROWS = 93, 85
MAX_GLYPHS = COLS * ROWS          # 7905

def render_glyph(font, ch):
    """Render one char at 12px SimSun, threshold into the 11x12 cell.

    The glyph is sampled in its NATURAL position (no ink-bbox cropping) so
    CJK punctuation like ，。 keeps its hanging low position and narrow kana
    keep their designed side bearings.  SimSun 12px draws hanzi ink at
    y[1,12], so the cell is sampled from source row +1 (shift up 1px) to
    fit the 12px cell.
    """
    img = Image.new("L", (32, 32), 0)
    d = ImageDraw.Draw(img)
    d.text((0, 0), ch, fill=255, font=font)
    px = img.load()
    # any ink at all?
    has_ink = any(px[x, y] >= 128 for y in range(16) for x in range(16))
    if not has_ink:
        return None
    rows = []
    for r in range(CELL_H):
        sy = r + 1
        bits = 0
        for c in range(CELL_W):
            if px[c, sy] >= 128:
                bits |= 1 << (CELL_W - 1 - c)
        rows.append(bits)
    # width stat (for the histogram)
    xs = [x for y in range(24) for x in range(24) if px[x, y] >= 128]
    w = (max(xs) - min(xs) + 1) if xs else 0
    return rows, w, CELL_H

def main():
    chars = build_charset()
    if len(chars) > MAX_GLYPHS:
        sys.exit(f"ERROR: charset too big: {len(chars)} > {MAX_GLYPHS}; "
                 f"widen the atlas grid in font.cpp too")

    font = ImageFont.truetype(FONT_PATH, 12, index=0)

    glyphs = []   # (ch, rows[12])
    stats_w = {}
    skipped = []
    for ch in chars:
        g = render_glyph(font, ch)
        if g is None:
            skipped.append(ch)
            continue
        rows, w, h = g
        if h > CELL_H:
            # clip bottom (rare: glyphs should fit 12px line)
            pass
        stats_w[w] = stats_w.get(w, 0) + 1
        glyphs.append((ch, rows))

    n = len(glyphs)
    print(f"glyphs rendered: {n} (skipped blank: {len(skipped)})")
    print(f"ink width histogram: {sorted(stats_w.items())}")

    # GBK encodings
    cp_list = []
    gbk_list = []
    bits = []
    for ch, rows in glyphs:
        cp = ord(ch)
        try:
            gb = ch.encode("gbk")
            code = (gb[0] << 8) | gb[1] if len(gb) == 2 else 0
        except (UnicodeEncodeError, UnicodeDecodeError):
            code = 0
        cp_list.append(cp)
        gbk_list.append(code)
        flat = []
        for r in rows:
            flat.append(r & 0xFF)
            flat.append((r >> 8) & 0xFF)
        bits.append(flat)

    # sanity: cp ascending? need sorted-by-cp for binary search; keep grid = index
    order = sorted(range(n), key=lambda i: cp_list[i])
    cp_sorted = [cp_list[i] for i in order]
    gbk_of_sorted = [gbk_list[i] for i in order]
    # glyph index mapping: sorted position -> original glyph index
    idx_of_sorted = order

    # also need GBK-sorted order for GBK lookup
    gorder = sorted(range(n), key=lambda i: gbk_list[i])
    gbk_sorted = [gbk_list[i] for i in gorder]
    cp_of_gsorted = [cp_list[i] for i in gorder]

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as f:
        f.write("/* font_cjk.cpp - CJK 12px bitmap font (generated).\n")
        f.write(" * Source font: SimSun 12px embedded bitmap (classic 12x12 grid).\n")
        f.write(" * Coverage: full GB2312 (zones 1-9 symbols + level-1/2 hanzi).\n")
        f.write(" * Atlas: 93 cols x 85 rows of 11x12 cells -> 1024x1024 PSMT4 texture.\n")
        f.write(" * Glyph index i lives at atlas cell (i %% 93, i / 93).\n")
        f.write(" * Tables:\n")
        f.write(" *   _FontCjkCp[]     glyph indices sorted by Unicode cp (binary search)\n")
        f.write(" *   _FontCjkGbk[]    glyph indices sorted by GBK code  (binary search)\n")
        f.write(" *   _FontCjkBits[]   N glyphs x 24 bytes (12 rows x 2 bytes, 11 px row)\n")
        f.write(" * Regen: python tools/font_gen_cjk.py\n")
        f.write(" */\n")
        f.write('#include "types.h"\n\n')
        f.write('extern "C" const int _FontCjkCount = %d;\n\n' % n)

        # cp -> glyph index table
        f.write('extern "C" const Uint16 _FontCjkCpSorted[%d] = {\n' % n)
        for i in range(0, n, 12):
            f.write("    " + ",".join("0x%04X" % c for c in cp_sorted[i:i+12]) + ",\n")
        f.write("};\n")
        f.write('extern "C" const Uint16 _FontCjkIdxOfCp[%d] = {\n' % n)
        for i in range(0, n, 12):
            f.write("    " + ",".join("%d" % v for v in idx_of_sorted[i:i+12]) + ",\n")
        f.write("};\n\n")

        # gbk -> glyph index table
        f.write('extern "C" const Uint16 _FontCjkGbkSorted[%d] = {\n' % n)
        for i in range(0, n, 12):
            f.write("    " + ",".join("0x%04X" % c for c in gbk_sorted[i:i+12]) + ",\n")
        f.write("};\n")
        f.write('extern "C" const Uint16 _FontCjkIdxOfGbk[%d] = {\n' % n)
        for i in range(0, n, 12):
            f.write("    " + ",".join("%d" % v for v in gorder[i:i+12]) + ",\n")
        f.write("};\n\n")

        # bitmaps
        f.write('extern "C" const Uint8 _FontCjkBits[%d][24] = {\n' % n)
        for ch, rows in glyphs:
            flat = []
            for r in rows:
                flat.append(r & 0xFF)
                flat.append((r >> 8) & 0xFF)
            f.write("    {" + ",".join("0x%02X" % b for b in flat) + "}, /* %s */\n" % ch)
        f.write("};\n")

    print(f"wrote {OUT_PATH} ({n} glyphs)")

if __name__ == "__main__":
    main()
