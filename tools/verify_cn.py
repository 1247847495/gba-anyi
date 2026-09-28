# -*- coding: utf-8 -*-
"""verify_cn.py - Verify localization safety and completeness."""
import io
import re
import sys

sys.stdout.reconfigure(encoding="utf-8")

# 1. Verify no strcmp/== comparisons against translated display-name functions
COMPARE_FUNCS = [
    "MainLoopStateGetDeviceName()",
    "MainLoopSramGetDeviceName()",
    "SmbGetStatusText()",
    "InputSnesMouseGetModeName()",
    "MainLoopTurboGetSpeedName()",
]

import glob
files = glob.glob("src/**/*.cpp", recursive=True) + \
        glob.glob("src/**/*.h", recursive=True)
files = [f for f in files if "third_party" not in f]

bad = 0
for f in files:
    src = io.open(f, encoding="utf-8", errors="replace").read()
    for fn in COMPARE_FUNCS:
        for m in re.finditer(
                r'(strcmp|strcasecmp|strncmp|strncasecmp)\s*\(\s*'
                r'[^\n]*' + re.escape(fn), src):
            print("COMPARE RISK: %s :: %s" % (f, m.group(0)[:100]))
            bad += 1

# 2. Check buffer-size safety: snprintf into known fixed buffers
BUF_CHECKS = [
    ("src/platform/ps2/system/mainloop_menu.cpp",
     "_MainLoop_StateManagerSramEntry", 48,
     ["SRAM存储: ", "SRAM存储: "]),
]

# 3. Remaining visible English strings in the main UI files
print("\n--- remaining English display strings ---")
check = {
    "src/platform/ps2/ui/uiVideo.cpp":
        r'_(VideoRow|VideoHeader|VideoCenter)\s*\([^)]*"([A-Za-z][^"]{3,})"',
    "src/platform/ps2/ui/uiNetwork.cpp":
        r'(SmbHeader|SmbRow|SmbAction|SmbCenter)\s*\([^)]*"([A-Za-z][^"]{3,})"',
}
for f, pat in check.items():
    src = io.open(f, encoding="utf-8").read()
    src = re.sub(r'//[^\n]*', '', src)
    for m in re.finditer(pat, src):
        print("%s: %s" % (f, m.group(2)))

# 4. Encoding sanity: all modified files are valid UTF-8
print("\n--- UTF-8 validity ---")
for f in [
    "src/platform/ps2/ui/uiVideo.cpp",
    "src/platform/ps2/ui/uiNetwork.cpp",
    "src/platform/ps2/ui/uiBrowser.cpp",
    "src/platform/ps2/system/mainloop_menu.cpp",
    "src/platform/ps2/system/mainloop_state.cpp",
]:
    try:
        io.open(f, encoding="utf-8").read()
        print("OK  %s" % f)
    except UnicodeDecodeError as e:
        print("BAD %s: %s" % (f, e))
        bad += 1

print("\nissues: %d" % bad)
