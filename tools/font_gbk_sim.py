# -*- coding: utf-8 -*-
"""Simulate font.cpp _FontDecode (greedy UTF-8-first) on GBK filenames
to confirm/deny the misparse theory."""

FONT_CP_GBK = 0x80000000

def font_decode(s, i):
    """Return (cp, nbytes) exactly like font.cpp."""
    c = s[i]
    if c < 0x80:
        return c, 1
    def cont(j):
        return j < len(s) and (s[j] & 0xC0) == 0x80
    if 0xC2 <= c <= 0xDF and cont(i+1):
        return ((c & 0x1F) << 6) | (s[i+1] & 0x3F), 2
    if 0xE0 <= c <= 0xEF and cont(i+1) and cont(i+2):
        cp = ((c & 0x0F) << 12) | ((s[i+1] & 0x3F) << 6) | (s[i+2] & 0x3F)
        if cp >= 0x800:
            return cp, 3
    if 0xF0 <= c <= 0xF4 and cont(i+1) and cont(i+2) and cont(i+3):
        cp = (((c & 0x07) << 18) | ((s[i+1] & 0x3F) << 12) |
              ((s[i+2] & 0x3F) << 6) | (s[i+3] & 0x3F))
        return cp, 4
    # GBK fallback
    t = s[i+1] if i+1 < len(s) else 0
    if 0x81 <= c <= 0xFE and 0x40 <= t <= 0xFE and t != 0x7F:
        return FONT_CP_GBK | (c << 8) | t, 2
    return c, 1

# common Chinese GBA ROM names
names = [
    "口袋妖怪 绿宝石.gba",
    "超级机器人大战J.gba",
    "火焰之纹章 封印之剑.gba",
    "黄金的太阳 开启的封印.gba",
    "恶魔城 白夜协奏曲.gba",
    "洛克人Zero 4.gba",
    "王国之心 记忆之链.gba",
    "塞尔达传说 众神的三角力量.gba",
    "马里奥赛车Advance.gba",
    "星之卡比 镜之大迷宫.gba",
    "最终幻想VAdvance.gba",
    "逆转裁判3.gba",
    "宝可梦 火红.gba",
    "光明之魂2.gba",
    "高级战争2 黑洞崛起.gba",
    "银河战士 融合.gba",
    "瓦里奥制造.gba",
    "勇者斗恶龙怪兽篇.gba",
    "我们的太阳.gba",
    "母3 (Mother 3).gba",
]

for name in names:
    gbk = name.encode("gbk")
    out = []
    i = 0
    mangled = []
    while i < len(gbk):
        cp, n = font_decode(gbk, i)
        if cp & FONT_CP_GBK:
            # GBK path: look up real char
            try:
                ch = bytes([(cp >> 8) & 0xFF, cp & 0xFF]).decode("gbk")
            except Exception:
                ch = "?"
            out.append(ch)
        else:
            # decoded as UTF-8
            try:
                ch = chr(cp)
            except Exception:
                ch = "?"
            out.append(ch)
            mangled.append((i, n, ch))
        i += n
    result = "".join(out)
    flag = " <-- MANGLED" if mangled else ""
    print(f"{name}\n  GBK->render: {result}{flag}")
    if mangled:
        for pos, n, ch in mangled:
            print(f"    byte {pos}: ate {n} bytes as UTF-8 -> {ch!r}")
