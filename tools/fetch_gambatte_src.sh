#!/bin/bash
# 获取 Gambatte libretro 源码到 third_party/gambatte
set -e
DEST="/mnt/d/SNESticleAurora-main/src/third_party/gambatte"

if [ -d "$DEST/.git" ] || [ -f "$DEST/Makefile" ]; then
    echo "[gambatte] already present"
    exit 0
fi

mkdir -p "$DEST"

cd /tmp
rm -rf gambatte-src gambatte.tar.gz

if git clone --depth 1 https://github.com/libretro/gambatte.git gambatte-src 2>/tmp/gambatte-clone.log; then
    echo "[gambatte] git clone ok"
else
    echo "[gambatte] git clone failed, trying tarball"
    cat /tmp/gambatte-clone.log || true
    curl -L -o gambatte.tar.gz https://codeload.github.com/libretro/gambatte/tar.gz/refs/heads/master
    mkdir -p gambatte-src
    tar -xzf gambatte.tar.gz -C gambatte-src --strip-components=1
fi

cp -a gambatte-src/. "$DEST/"
echo "[gambatte] source ready at $DEST"
ls "$DEST" | head -20
echo '=== ps2 in Makefile? ==='
grep -n "ps2" "$DEST/Makefile" "$DEST/Makefile.common" 2>/dev/null | head -10 || echo "no ps2 target"
