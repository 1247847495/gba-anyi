# -*- coding: utf-8 -*-
"""find_remaining.py - Find remaining untranslated user-visible messages."""
import io
import re
import glob
import sys

sys.stdout.reconfigure(encoding="utf-8")

files = glob.glob("src/platform/ps2/**/*.cpp", recursive=True)
files += glob.glob("src/common/**/*.cpp", recursive=True)

pat = re.compile(
    r'(MainLoopStatusPrintf|MainLoopModalPrintf|ScrPrintf)\s*\(\s*[^;]*?"((?:[^"\\]|\\.)*[A-Za-z]{3}[^"]*)"',
    re.S)

for f in sorted(files):
    if "third_party" in f:
        continue
    src = io.open(f, encoding="utf-8", errors="replace").read()
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
    src = re.sub(r'//[^\n]*', '', src)
    for m in pat.finditer(src):
        s = m.group(2)
        # only short single-line literals (skip giant matched blobs)
        if "\n" in s or len(s) > 90:
            continue
        print("%s: %s(...%s...)" % (f, m.group(1), s))
