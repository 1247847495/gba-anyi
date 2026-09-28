#!/bin/bash
# 直接用 codeload tarball 获取 Gambatte（git clone TLS 会挂）
set -e
DEST="/mnt/d/SNESticleAurora-main/src/third_party/gambatte"

if [ -f "$DEST/Makefile" ]; then
    echo "[gambatte] already present"
    exit 0
fi

rm -rf /tmp/gambatte-src /tmp/gambatte.tar.gz
cd /tmp

for i in 1 2 3; do
    echo "[gambatte] tarball attempt $i"
    if curl -sL --max-time 180 -o gambatte.tar.gz https://codeload.github.com/libretro/gambatte/tar.gz/refs/heads/master; then
        break
    fi
    sleep 3
done

ls -la gambatte.tar.gz
mkdir -p gambatte-src
tar -xzf gambatte.tar.gz -C gambatte-src --strip-components=1

mkdir -p "$DEST"
cp -a gambatte-src/. "$DEST/"
echo "[gambatte] source ready"
ls "$DEST" | head -15
echo '=== ps2 support in Makefile ==='
grep -n "ps2" "$DEST/Makefile" 2>/dev/null | head -10 || echo "(no ps2 target)"
