#!/bin/bash
set -euo pipefail
packaging_dir=$(cd -- "$(dirname -- "$0")" && pwd)
driver=${1:-/private/tmp/fxsound-driver-artifacts/FxSound.driver}
output=${2:-/private/tmp/fxsound-packaging-spike}
if [[ ! -d "$driver/Contents/MacOS" ]]; then echo 'Build the HAL driver bundle first.' >&2; exit 1; fi
if [[ -n $(find "$driver" -type l -print -quit) ]]; then echo 'Driver payload must not contain symlinks.' >&2; exit 1; fi
codesign --verify --strict "$driver"
mkdir -p "$output"
staging=$(mktemp -d /private/tmp/fxsound-package-stage.XXXXXX)
mkdir -p "$staging/root/Library/Audio/Plug-Ins/HAL" "$staging/scripts"
ditto --norsrc --noextattr --noacl "$driver" "$staging/root/Library/Audio/Plug-Ins/HAL/FxSound.driver"
xattr -cr "$staging/root"
codesign --verify --strict "$staging/root/Library/Audio/Plug-Ins/HAL/FxSound.driver"
cp "$packaging_dir/scripts/preinstall" "$packaging_dir/scripts/postinstall" "$staging/scripts/"
for arch in x86_64 arm64; do
    xcrun clang -O2 -Wall -Wextra -Werror -arch "$arch" -mmacosx-version-min=14.0 \
        -framework CoreAudio -framework CoreFoundation "$packaging_dir/audio-install-guard.c" \
        -o "$staging/guard-$arch"
done
xcrun lipo -create "$staging/guard-x86_64" "$staging/guard-arm64" -output "$staging/scripts/audio-install-guard"
guard_identity=${3:--}
if [[ "$guard_identity" == - ]]; then
    codesign --sign - --options runtime --timestamp=none "$staging/scripts/audio-install-guard"
else
    if [[ "$guard_identity" != 'Developer ID Application:'* ]]; then echo 'Explicit Developer ID Application identity required.' >&2; exit 2; fi
    codesign --sign "$guard_identity" --timestamp --options runtime "$staging/scripts/audio-install-guard"
fi
codesign --verify --strict "$staging/scripts/audio-install-guard"
cp "$staging/scripts/audio-install-guard" "$output/audio-install-guard"
pkgbuild --root "$staging/root" --scripts "$staging/scripts" --ownership recommended \
    --identifier org.fxsound.driver.spike --version 0.1.0 --install-location / \
    "$output/fxsound-driver-component.pkg"
productbuild --synthesize --product "$packaging_dir/requirements.plist" \
    --package "$output/fxsound-driver-component.pkg" "$output/Distribution.xml"
productbuild --distribution "$output/Distribution.xml" --package-path "$output" \
    "$output/FxSound-driver-development-spike.pkg"
pkgutil --payload-files "$output/fxsound-driver-component.pkg"
python3 "$packaging_dir/source-snapshot.py" "$output"
(cd "$output" && shasum -a 256 FxSound-driver-development-spike.pkg \
    fxsound-driver-component.pkg fxsound-driver-spike-source.tar.gz LICENSES.txt SOURCE_SHA256SUMS > SHA256SUMS)
printf 'Unsigned driver-only spike: %s/FxSound-driver-development-spike.pkg\n' "$output"
printf 'Staging retained for inspection: %s\n' "$staging"
