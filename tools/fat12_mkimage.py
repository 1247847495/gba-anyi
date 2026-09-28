#!/usr/bin/env python3
# Build a minimal FAT12 image containing one file whose LFN is a long
# Chinese ROM name and whose SFN is the GBK 8.3 name that Chinese Windows
# would generate ("马里奥~1.SFC"). This replicates what the user's USB
# stick actually stores, so we can verify how FatFs reports the name.

SECTOR = 512
TOT_SEC = 256         # 253 data clusters -> FAT12; FatFs check_fs requires >=128 sectors
RES_SEC = 1
NFATS = 1
FAT_SEC = 1
ROOT_SEC = 1          # 16 entries

LFN = "马里奥世界超级大冒险合集完美中文版.sfc"
SFN_NAME = "马里奥~1".encode("gbk")   # C2 ED C0 EF B0 C2 7E 31 (8 bytes)
SFN_EXT = b"SFC"
assert len(SFN_NAME) == 8

img = bytearray(SECTOR * TOT_SEC)

# ---- boot sector ----
bs = bytearray(SECTOR)
bs[0:3] = b"\xEB\x3C\x90"
bs[3:11] = b"MSWIN4.1"
import struct
struct.pack_into("<H", bs, 11, 512)      # bytes/sector
bs[13] = 1                               # sectors/cluster
struct.pack_into("<H", bs, 14, RES_SEC)  # reserved sectors
bs[16] = NFATS
struct.pack_into("<H", bs, 17, 16)       # root entries
struct.pack_into("<H", bs, 19, TOT_SEC)  # total sectors 16
bs[21] = 0xF8                            # media
struct.pack_into("<H", bs, 22, FAT_SEC)  # sectors per FAT
struct.pack_into("<H", bs, 24, 18)       # sectors per track
struct.pack_into("<H", bs, 26, 2)        # heads
bs[36] = 0x80
bs[38] = 0x29                            # boot signature
struct.pack_into("<I", bs, 39, 0x12345678)
bs[43:54] = b"TESTVOLUME "
bs[54:62] = b"FAT12   "
bs[510] = 0x55
bs[511] = 0xAA
img[0:512] = bs

# ---- FAT ----
fat = bytearray(SECTOR * FAT_SEC)
fat[0] = 0xF8
fat[1] = 0xFF
fat[2] = 0xFF
# cluster 2 = EOC (FAT12 entry 2 lives in bytes 3..4: fat[3] | (fat[4]&0xF)<<8)
fat[3] = 0xFF
fat[4] = 0xFF         # entry 2 = 0xFFF (EOC)
img[512 * RES_SEC : 512 * (RES_SEC + FAT_SEC)] = fat

# ---- root directory ----
root_off = 512 * (RES_SEC + FAT_SEC * NFATS)

sfn11 = SFN_NAME + SFN_EXT
assert len(sfn11) == 11

# LFN checksum over the 11 SFN bytes
chk = 0
for b in sfn11:
    chk = (((chk & 1) << 7) + (chk >> 1) + b) & 0xFF

name16 = [ord(c) for c in LFN]
nchars = len(name16)
n_entries = (nchars + 12) // 13

def lfn_entry(seq, chunk):
    e = bytearray(32)
    e[0] = seq
    e[11] = 0x0F
    e[12] = 0
    e[13] = chk
    chars = list(chunk)
    if len(chars) < 13:
        chars.append(0x0000)            # terminator
    while len(chars) < 13:
        chars.append(0xFFFF)            # padding
    e[1:11] = struct.pack("<5H", *chars[0:5])
    e[14:26] = struct.pack("<6H", *chars[5:11])
    e[28:32] = struct.pack("<2H", *chars[11:13])
    return e

entries = []
for i in range(n_entries):
    seq = n_entries - i                 # physical order: N..1, then SFN
    chunk = name16[(seq - 1) * 13 : seq * 13]
    if i == 0:
        seq |= 0x40                     # last logical chunk flag
    entries.append(lfn_entry(seq, chunk))

# SFN entry
sfn = bytearray(32)
sfn[0:11] = sfn11
sfn[11] = 0x20                          # ATTR_ARCHIVE
sfn[12] = 0x00                          # NTres: case info only
struct.pack_into("<H", sfn, 22, 0x6000) # write time
struct.pack_into("<H", sfn, 24, 0x5C00) # write date (2026-ish)
struct.pack_into("<H", sfn, 26, 2)      # first cluster
struct.pack_into("<I", sfn, 28, 123456) # size
entries.append(sfn)

off = root_off
for e in entries:
    img[off:off + 32] = e
    off += 32

with open("fat12.img", "wb") as f:
    f.write(img)

print("image written: fat12.img (%d bytes), LFN='%s' (%d chars, %d LFN entries), SFN=%s, chksum=0x%02X"
      % (len(img), LFN, nchars, n_entries, SFN_NAME.hex().upper(), chk))
