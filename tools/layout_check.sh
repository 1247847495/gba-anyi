#!/bin/bash
# AURORA_LAYOUT_CHECK_V24: every header whose layout changed in V14 must be
# older than EVERY gambatte .o; a stale .o reads SaveState/Cartridge/Memory
# members at pre-V14 offsets -> garbage pointers -> heap trample.
cd /mnt/d/GBA
OBJS=$(find build/gambatte/obj -name "*.o")
echo "total objs: $(echo "$OBJS" | wc -l)"
for h in \
  src/third_party/gambatte/libgambatte/include/savestate.h \
  src/third_party/gambatte/libgambatte/src/mem/cartridge.h \
  src/third_party/gambatte/libgambatte/src/gambatte-memory.h \
  src/third_party/gambatte/libgambatte/src/mem/memptrs.h \
  src/third_party/gambatte/libgambatte/src/video.h ; do
  if [ ! -f "$h" ]; then
    # find actual savestate.h location
    hh=$(find src/third_party/gambatte -name "$(basename $h)" | head -1)
    [ -n "$hh" ] && h="$hh" || { echo "MISSING $(basename $h)"; continue; }
  fi
  hm=$(stat -c %Y "$h")
  stale=0
  for o in $OBJS; do
    om=$(stat -c %Y "$o")
    if [ "$om" -lt "$hm" ]; then
      echo "  STALE: $o ($(date -d @$om '+%m-%d %H:%M')) < $(basename $h) ($(date -d @$hm '+%m-%d %H:%M'))"
      stale=$((stale+1))
    fi
  done
  echo "$(basename $h) mtime $(date -d @$hm '+%m-%d %H:%M') -> stale objs: $stale"
done
echo "--- bridge/main build too:"
for h in src/gb/gambatte/gambatte_bridge.cpp src/platform/ps2/system/mainloop_ui.cpp; do
  hm=$(stat -c %Y "$h"); o=build/gb/gambatte/gambatte_bridge.o
  [ -f "$o" ] && om=$(stat -c %Y "$o") || om=0
  echo "$(basename $h) $(date -d @$hm '+%H:%M') vs bridge.o $(date -d @$om '+%H:%M')"
done
