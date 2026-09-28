#!/bin/bash
# AURORA_T23_VERIFY: markers present in ELF + silent symbol linked + copy.
f=/mnt/d/GBA/build/anyi-gba.elf
echo "--- marker strings (expect DBG[ present):"
strings -a "$f" | grep -E "^DBG\[" | sort -u | head -8
echo "--- silent symbol (expect T defined):"
/root/.local/ps2dev/ee/bin/mips64r5900el-ps2-elf-nm "$f" | grep -i "ModalSilent\|ModalPrintf"
echo "--- watchdog residue (expect empty):"
strings -a "$f" | grep -c "看门狗" || true
