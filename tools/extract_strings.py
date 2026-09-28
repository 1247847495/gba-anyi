# -*- coding: utf-8 -*-
"""Extract user-visible English strings from UI sources."""
import re, io, sys

sys.stdout.reconfigure(encoding='utf-8')

files = sys.argv[1:]
for f in files:
    src = io.open(f, encoding='utf-8', errors='replace').read()
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
    src = re.sub(r'//[^\n]*', '', src)
    print(f'===== {f} =====')
    for i, line in enumerate(src.split('\n'), 1):
        for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', line):
            s = m.group(1)
            if len(s) >= 4 and re.search(r'[A-Za-z]{3}', s):
                print(f'{i}: {s}')
