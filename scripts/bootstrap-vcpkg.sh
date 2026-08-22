#!/usr/bin/env bash
# bootstrap-vcpkg.sh
# 将 vcpkg 与全部依赖安装到项目内部（不污染系统）：
#   - vcpkg 本体          -> <repo>/third_party/vcpkg
#   - 依赖安装产物        -> <repo>/NPGS/vcpkg_installed
#   - 下载缓存            -> <repo>/third_party/vcpkg/downloads
#
# 用法:
#   ./scripts/bootstrap-vcpkg.sh [triplet]      # 默认 x64-linux
#   SKIP_INSTALL=1 ./scripts/bootstrap-vcpkg.sh # 仅克隆+bootstrap

set -euo pipefail

TRIPLET="${1:-x64-linux}"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VCPKG_DIR="$REPO_ROOT/third_party/vcpkg"
CACHE_DIR="$VCPKG_DIR/downloads"
MANIFEST="$REPO_ROOT/NPGS"
INSTALL_DIR="$REPO_ROOT/NPGS/vcpkg_installed"

echo "[1/4] 克隆 vcpkg -> $VCPKG_DIR"
if [[ ! -x "$VCPKG_DIR/vcpkg" && ! -f "$VCPKG_DIR/bootstrap-vcpkg.sh" ]]; then
    mkdir -p "$VCPKG_DIR"
    git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
else
    echo "已存在，跳过克隆"
fi

echo "[2/4] bootstrap vcpkg"
pushd "$VCPKG_DIR" >/dev/null
./bootstrap-vcpkg.sh -disableMetrics
popd >/dev/null

echo "[3/4] 下载缓存 -> $CACHE_DIR"
mkdir -p "$CACHE_DIR"
export VCPKG_DOWNLOADS="$CACHE_DIR"

if [[ "${SKIP_INSTALL:-0}" != "1" ]]; then
    echo "[4/4] 预装 manifest 依赖 ($TRIPLET)..."
    "$VCPKG_DIR/vcpkg" install \
        --x-manifest-root="$MANIFEST" \
        --x-install-root="$INSTALL_DIR" \
        --triplet="$TRIPLET"
else
    echo "[4/4] 跳过依赖预装 (SKIP_INSTALL=1)"
fi

echo ""
echo "完成！后续构建（示例）："
echo "  cmake --preset linux-gcc-release"
echo "  cmake --build --preset linux-gcc-release"
echo "提示：若在其他终端直接构建，请先执行："
echo "  export VCPKG_DOWNLOADS=\"$CACHE_DIR\""
