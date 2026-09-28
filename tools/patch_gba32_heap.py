#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AURORA_GBA32_BOOSTER_V4: post-link heap-start patch.

Rewrites _main's "la $a0, _end" (the SetupHeap base argument) into
"la $a0, __gba32_heap_start" so the malloc heap starts AFTER the
booster PT_LOAD. _end keeps its baseline address so crt0's bss clear
loop stays byte-identical with the baseline image.

MIPS "la $a0, sym" expands to:
    lui   a0, hi(sym)     # 0x3c040000 | ((sym + 0x8000) >> 16)
    addiu a0, a0, lo(sym)# 0x24840000 | (sym & 0xffff)
(with GCC free to interleave other instructions between the two).
This script finds that exact pair for the OLD symbol, verifies it is
unique in the whole ELF, and rewrites both immediates for the NEW
symbol. Little-endian.

Usage: patch_gba32_heap.py <elf>
Exits non-zero (and leaves the file untouched) unless exactly one
candidate pair is found and the write verifies.
"""

import os
import struct
import subprocess
import sys


def nm_value(elf, sym):
    nm = os.environ.get("GBA32_NM", "mips64r5900el-ps2-elf-nm")
    out = subprocess.check_output([nm, elf], text=True)
    values = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == sym:
            values.add(int(parts[0], 16))
    if len(values) != 1:
        raise SystemExit(
            "patch_gba32_heap: expected exactly one definition of %s in %s, "
            "got %d (%s)" % (sym, elf, len(values), sorted(hex(v) for v in values))
        )
    return values.pop()


def hi(v):
    return (v + 0x8000) >> 16


def lo(v):
    return v & 0xFFFF


def w32(v):
    return struct.pack("<I", v)


# Allowed instruction distance between the lui and its addiu (words).
PAIR_WINDOW_WORDS = (1, 2, 3, 4, 5)


def find_pairs(data, lui_word, addiu_word):
    """Return file offsets of (lui, addiu) pairs: lui at i, addiu at
    i + 4*k for k in PAIR_WINDOW_WORDS."""
    lui_b = w32(lui_word)
    addiu_b = w32(addiu_word)
    pairs = []
    pos = 0
    while True:
        pos = data.find(lui_b, pos)
        if pos < 0:
            break
        for k in PAIR_WINDOW_WORDS:
            j = pos + 4 * k
            if data[j : j + 4] == addiu_b:
                pairs.append((pos, j))
                break
        pos += 1
    return pairs


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_gba32_heap.py <elf>")
    elf = sys.argv[1]

    old = nm_value(elf, "_end")
    new = nm_value(elf, "__gba32_heap_start")
    if old >= new:
        raise SystemExit(
            "patch_gba32_heap: __gba32_heap_start (0x%x) must be > _end (0x%x)"
            % (new, old)
        )

    old_lui = 0x3C040000 | (hi(old) & 0xFFFF)
    old_addiu = 0x24840000 | lo(old)
    new_lui = 0x3C040000 | (hi(new) & 0xFFFF)
    new_addiu = 0x24840000 | lo(new)

    with open(elf, "rb") as f:
        data = f.read()

    pairs = find_pairs(data, old_lui, old_addiu)
    if len(pairs) != 1:
        raise SystemExit(
            "patch_gba32_heap: expected exactly one 'la $a0,_end' pair "
            "(lui 0x%08x / addiu 0x%08x) in %s, found %d at %s"
            % (
                old_lui,
                old_addiu,
                elf,
                len(pairs),
                [hex(p[0]) for p in pairs],
            )
        )

    lui_off, addiu_off = pairs[0]
    patched = (
        data[:lui_off]
        + w32(new_lui)
        + data[lui_off + 4 : addiu_off]
        + w32(new_addiu)
        + data[addiu_off + 4 :]
    )

    with open(elf, "r+b") as f:
        f.write(patched)
        f.flush()
        f.seek(0)
        check = f.read()

    ok = (
        check[lui_off : lui_off + 4] == w32(new_lui)
        and check[addiu_off : addiu_off + 4] == w32(new_addiu)
    )
    if not ok:
        raise SystemExit("patch_gba32_heap: verify-after-write failed")
    print(
        "[ GBA32 ] heap start patched in %s: la $a0,0x%x -> la $a0,0x%x "
        "(file offsets 0x%x / 0x%x)"
        % (elf, old, new, lui_off, addiu_off)
    )


if __name__ == "__main__":
    main()
