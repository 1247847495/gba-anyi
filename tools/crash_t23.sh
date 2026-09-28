#!/bin/bash
# AURORA_T23_CRASH_ANALYSIS: locate EPC 0x2a7b68 and BadVAddr context.
cd /mnt/d/GBA
NM=/root/.local/ps2dev/ee/bin/mips64r5900el-ps2-elf-nm
OBJDUMP=/root/.local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump
$NM -n build/anyi-gba.elf | awk -v addr=002a7b68 '
  { if (strtonum("0x" $1) <= strtonum("0x" addr)) { last=$0 } else { print "EPC between: " last; print "            " $0; exit } }'
echo "--- disasm around EPC:"
$OBJDUMP -d --start-address=0x2a7b40 --stop-address=0x2a7b90 build/anyi-gba.elf | tail -20
echo "--- also check t23 stripped copy matches (same build):"
md5sum build/anyi-gba.elf build/anyi-gba.s中文测试版23.elf 2>/dev/null | head -2
