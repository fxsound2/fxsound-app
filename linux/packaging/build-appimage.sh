#!/usr/bin/env bash
# ==============================================================================
# FxSound AppImage Packaging Script
# Builds a standalone portable AppImage for x86_64 Linux.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build}"
APPDIR="${BUILD_DIR}/AppDir"
OUT_DIR="${BUILD_DIR}"

echo "==> Ensuring build directory exists and is built..."
if [ ! -f "${BUILD_DIR}/FxSound_artefacts/Release/FxSound" ] && [ ! -f "${BUILD_DIR}/bin/FxSound" ]; then
    cmake -S "${ROOT_DIR}/linux" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --target FxSound
fi

echo "==> Preparing AppDir in ${APPDIR}..."
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr"

# Install binaries, desktop entry, presets, icons into AppDir
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}" --prefix "/usr"

# AppImage root integration
cp "${APPDIR}/usr/share/applications/fxsound.desktop" "${APPDIR}/"
cp "${APPDIR}/usr/share/icons/hicolor/256x256/apps/fxsound.png" "${APPDIR}/"
if [ -f "${APPDIR}/usr/share/icons/hicolor/scalable/apps/fxsound.svg" ]; then
    cp "${APPDIR}/usr/share/icons/hicolor/scalable/apps/fxsound.svg" "${APPDIR}/"
fi

# Create custom AppRun script
cat << 'EOF' > "${APPDIR}/AppRun"
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "${0}")")"
export PATH="${HERE}/usr/bin:${PATH}"
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
exec "${HERE}/usr/bin/FxSound" "$@"
EOF
chmod +x "${APPDIR}/AppRun"

# Find or download appimagetool
APPIMAGETOOL=""
if command -v appimagetool >/dev/null 2>&1; then
    APPIMAGETOOL="$(command -v appimagetool)"
elif [ -f "/tmp/appimagetool-x86_64.AppImage" ]; then
    APPIMAGETOOL="/tmp/appimagetool-x86_64.AppImage"
else
    echo "==> appimagetool not found. To build the final .AppImage file, install appimagetool or run in GitHub Actions CI."
    echo "==> AppDir is ready at: ${APPDIR}"
    exit 0
fi

EXTRA_ARGS=""
if [ -f "/tmp/runtime-x86_64" ]; then
    EXTRA_ARGS="--runtime-file /tmp/runtime-x86_64"
fi

echo "==> Generating AppImage..."
VERSION="${VERSION:-1.2.17}"
ARCH=x86_64 "${APPIMAGETOOL}" --appimage-extract-and-run ${EXTRA_ARGS} "${APPDIR}" "${OUT_DIR}/fxsound-${VERSION}-x86_64.AppImage"
echo "==> Successfully created: ${OUT_DIR}/fxsound-${VERSION}-x86_64.AppImage"
