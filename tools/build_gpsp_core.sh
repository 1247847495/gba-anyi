#!/bin/bash
# 编译 gpSP PS2 静态库（raw，命名空间化由上层完成）
set -e
export PS2DEV=/root/.local/ps2dev
export PS2SDK=$PS2DEV/ps2sdk
export GSKIT=$PS2DEV/gskit
export PATH=$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PATH

cd /mnt/d/SNESticleAurora-main/src/third_party/gpsp
make -j8 platform=ps2 clean 2>/dev/null || true
make -j8 platform=ps2 2>&1 | tail -30
echo '---'
ls -la gpsp_libretro_ps2.a 2>/dev/null || ls -la *.a
