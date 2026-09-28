# -*- coding: utf-8 -*-
"""check_glyphs.py - Verify every non-ASCII char used in localized strings
exists in the CJK font atlas."""
import io
import re
import sys

sys.stdout.reconfigure(encoding="utf-8")

# load font codepoints
src = io.open("src/common/base/font_cjk.cpp", encoding="utf-8").read()
cps = set()
for m in re.finditer(r"0x([0-9A-Fa-f]{4})", src):
    cps.add(int(m.group(1), 16))

FILES = [
    "src/platform/ps2/ui/uiVideo.cpp",
    "src/platform/ps2/ui/uiNetwork.cpp",
    "src/platform/ps2/ui/uiBrowser.cpp",
    "src/platform/ps2/ui/uiLog.cpp",
    "src/platform/ps2/system/mainloop_menu.cpp",
    "src/platform/ps2/system/mainloop_state.cpp",
    "src/platform/ps2/system/mainloop_smb.cpp",
    "src/platform/ps2/system/mainloop_load.cpp",
    "src/platform/ps2/system/mainloop_browser.cpp",
    "src/platform/ps2/system/mainloop_install.cpp",
    "src/platform/ps2/system/mainloop_process.cpp",
    "src/platform/ps2/system/mainloop_menu_runtime.cpp",
    "src/platform/ps2/input/input.cpp",
    "src/platform/ps2/system/mainloop_input.cpp",
    "src/platform/ps2/system/mainloop_ui.cpp",
]

missing = {}
for f in FILES:
    text = io.open(f, encoding="utf-8").read()
    # only inspect inside string literals
    for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', text):
        s = m.group(1)
        for ch in s:
            cp = ord(ch)
            if cp < 0x80:
                continue
            if ch in ("\\",):
                continue
            if cp not in cps:
                missing.setdefault(ch, []).append(f)

if missing:
    for ch, fl in sorted(missing.items()):
        print("MISSING GLYPH: U+%04X %s  in %s" %
              (ord(ch), ch, ",".join(sorted(set(fl)))))
    sys.exit(1)
print("all non-ASCII chars in string literals are covered by the font (%d glyphs)" % len(cps))
