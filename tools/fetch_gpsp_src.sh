#!/bin/bash
# 获取 gpSP libretro 源码到 third_party/gpsp
# 优先 git clone，TLS 失败则退回 codeload tarball
set -e
DEST="/mnt/d/SNESticleAurora-main/src/third_party/gpsp"

if [ -d "$DEST/.git" ] || [ -f "$DEST/Makefile" ]; then
    echo "[gpsp] already present"
    exit 0
fi

mkdir -p "$DEST"

cd /tmp
rm -rf gpsp-src gpsp.tar.gz

if git clone --depth 1 https://github.com/libretro/gpsp.git gpsp-src 2>/tmp/gpsp-clone.log; then
    echo "[gpsp] git clone ok"
else
    echo "[gpsp] git clone failed, trying tarball"
    cat /tmp/gpsp-clone.log || true
    curl -L -o gpsp.tar.gz https://codeload.github.com/libretro/gpsp/tar.gz/refs/heads/master
    mkdir -p gpsp-src
    tar -xzf gpsp.tar.gz -C gpsp-src --strip-components=1
fi

# 拷贝源码（含 .git 便于后续更新）
cp -a gpsp-src/. "$DEST/"
echo "[gpsp] source ready at $DEST"
ls "$DEST" | head -30
