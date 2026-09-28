#!/bin/bash
# Fetch BDM stack sources from ps2sdk commit f08e889f for analysis
set -e
mkdir -p /tmp/bdmsrc
base="https://raw.githubusercontent.com/ps2dev/ps2sdk/f08e889f"
files="
iop/fs/bdm/src/main.c
iop/fs/bdm/src/part_driver.c
iop/fs/bdmfs_fatfs/src/fs_driver.c
iop/fs/bdmfs_fatfs/src/main.c
iop/fs/libbdm/src/bd_cache.c
iop/fs/libbdm/src/bd_defrag.c
iop/fs/filexio/src/fileXio_iop.c
iop/dev9/atad/src/ps2atad.c
"
for f in $files; do
  out="/tmp/bdmsrc/$(echo "$f" | tr '/' '_')"
  curl -sL "$base/$f" -o "$out"
  echo "=== $f ($(wc -l < "$out") lines)"
done
