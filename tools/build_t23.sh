#!/bin/bash
# AURORA_T23_BUILD: full rebuild, log to file, show errors + summary.
export PS2DEV=/root/.local/ps2dev
export PATH=$PATH:$PS2DEV/ee/bin
cd /mnt/d/GBA
make fast > /tmp/t23_build.log 2>&1
rc=$?
echo "make exit: $rc"
grep -nE "error:|Error [0-9]|\\[ERROR" /tmp/t23_build.log | head -20
tail -8 /tmp/t23_build.log
