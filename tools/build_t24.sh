#!/bin/bash
# AURORA_T24_CLEAN_REBUILD: wipe ALL gambatte objects (stale-layout
# hypothesis) and rebuild from scratch, keeping the t23 silent-marker config.
export PS2DEV=/root/.local/ps2dev
export PATH=$PATH:$PS2DEV/ee/bin
cd /mnt/d/GBA
rm -rf build/gambatte
make fast > /tmp/t24_build.log 2>&1
rc=$?
echo "make exit: $rc"
grep -nE "error:|Error [0-9]|\\[ERROR" /tmp/t24_build.log | head -10
tail -6 /tmp/t24_build.log
