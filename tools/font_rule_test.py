# -*- coding: utf-8 -*-
"""Validate the planned classifier + GBK->UTF-8 conversion rule."""

def classify_and_convert(data):
    """Return (encoding, text) using the planned rule."""
    # pass 1: strict UTF-8 + plausibility
    i = 0
    ok_utf8 = True
    cps = []
    n = len(data)
    while i < n:
        c = data[i]
        if c < 0x80:
            i += 1
            continue
        def cont(j):
            return j < n and (data[j] & 0xC0) == 0x80
        if 0xC2 <= c <= 0xDF and cont(i+1):
            cp = ((c & 0x1F) << 6) | (data[i+1] & 0x3F)
            cps.append(cp); i += 2
        elif 0xE0 <= c <= 0xEF and cont(i+1) and cont(i+2):
            cp = ((c & 0x0F) << 12) | ((data[i+1] & 0x3F) << 6) | (data[i+2] & 0x3F)
            if cp < 0x800:
                ok_utf8 = False; break
            cps.append(cp); i += 3
        elif 0xF0 <= c <= 0xF4 and cont(i+1) and cont(i+2) and cont(i+3):
            cp = (((c & 0x07) << 18) | ((data[i+1] & 0x3F) << 12) |
                  ((data[i+2] & 0x3F) << 6) | (data[i+3] & 0x3F))
            cps.append(cp); i += 4
        else:
            ok_utf8 = False
            break
    if ok_utf8:
        # plausibility: multi-byte cps must be <0x100 or >=0x2000
        if all(cp < 0x100 or cp >= 0x2000 for cp in cps):
            return "UTF-8", data.decode("utf-8")
    # GBK conversion
    out = []
    i = 0
    while i < n:
        c = data[i]
        if c < 0x80:
            out.append(chr(c)); i += 1
            continue
        t = data[i+1] if i+1 < n else 0
        if 0x81 <= c <= 0xFE and 0x40 <= t <= 0xFE and t != 0x7F:
            try:
                ch = bytes([c, t]).decode("gbk")
                out.append(ch)
            except UnicodeDecodeError:
                out.append('?')
            i += 2
        else:
            out.append('?')
            i += 1
    return "GBK", "".join(out)

names = [
    "口袋妖怪 绿宝石.gba", "超级机器人大战J.gba", "火焰之纹章 封印之剑.gba",
    "黄金的太阳 开启的封印.gba", "恶魔城 白夜协奏曲.gba", "洛克人Zero 4.gba",
    "王国之心 记忆之链.gba", "塞尔达传说 众神的三角力量.gba", "马里奥赛车Advance.gba",
    "星之卡比 镜之大迷宫.gba", "最终幻想VAdvance.gba", "逆转裁判3.gba",
    "宝可梦 火红.gba", "光明之魂2.gba", "高级战争2 黑洞崛起.gba",
    "银河战士 融合.gba", "瓦里奥制造.gba", "勇者斗恶龙怪兽篇.gba",
    "我们的太阳.gba", "母3 (Mother 3).gba", "石.gba", "超级玛丽奥世界.sfc",
    "魂斗罗3.sfc", "三国志II 覇王の大陸.nes", "影子传说.nes",
    "街头霸王II·加速版.sfc", "冒险岛Ⅰ代.nes", "恐龙战队①.md",
]
fails = 0
for name in names:
    for enc in ("gbk", "utf-8"):
        data = name.encode(enc)
        cls, text = classify_and_convert(data)
        good = (text == name)
        if not good:
            fails += 1
        mark = "OK " if good else "FAIL"
        print(f"{mark} {enc:5s} -> {cls:5s}  {name!r} => {text!r}")

# UI literal sanity: no multi-byte cps in 0x100-0x1FFF
ui = "设置 手柄按键·更改叉○圆△三角□方块…—①★℃½™→「」无封面复制路径过长。磁盘已弹出；1秒后换另一面..."
data = ui.encode("utf-8")
cls, text = classify_and_convert(data)
print(("OK " if cls == "UTF-8" and text == ui else "FAIL"), "UI ->", cls, text == ui)
if cls != "UTF-8" or text != ui:
    fails += 1
bad = [hex(ord(ch)) for ch in ui if 0x100 <= ord(ch) < 0x2000]
print("UI cps in suspicious band 0x100-0x1FFF:", bad if bad else "none")

print("FAILS:", fails)
