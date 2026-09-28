#!/bin/bash
# AURORA_T20_VERIFY: confirm first-frame diag markers survived link.
for f in \
  /mnt/d/GBA/build/gambatte/obj/libgambatte/libretro/libretro.o \
  /mnt/d/GBA/build/gambatte/gambatte_bridge.o \
  /mnt/d/GBA/build/gambatte/gambatte_libretro_ps2.a \
  /mnt/d/GBA/build/anyi-gba.elf \
  /mnt/d/GBA/build/anyi-gba.stripped.elf; do
  echo "=== $f"
  strings -a "$f" | grep -E "LOOP STUCK|run enter f=|RF enter|video ok|loop done|dupe e=" | sort -u | head -10
done
