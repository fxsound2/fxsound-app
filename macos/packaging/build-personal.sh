#!/bin/bash
set -euo pipefail
packaging_dir=$(cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(cd -- "$packaging_dir/../.." && pwd)
app=${1:?Pass the built FxSound.app path}
driver=${2:?Pass the built FxSound.driver path}
output=${3:-$repo_dir/dist}
juce_source=${4:-/private/tmp/fxsound-juce-8}
version=$(python3 - "$app/Contents/Info.plist" <<'PY'
import plistlib
import sys
with open(sys.argv[1], 'rb') as source:
    print(plistlib.load(source)['CFBundleShortVersionString'])
PY
)
for bundle in "$app" "$driver"; do
    codesign --verify --deep --strict "$bundle"
done
mkdir -p "$output"
output=$(cd -- "$output" && pwd)
staging=$(mktemp -d /private/tmp/fxsound-personal-package.XXXXXX)
trap 'rm -rf "$staging"' EXIT
mkdir -p "$staging/root/Applications" "$staging/root/Library/Audio/Plug-Ins/HAL" "$staging/scripts" "$staging/dmg"
ditto --norsrc --noextattr --noacl "$app" "$staging/root/Applications/FxSound.app"
ditto --norsrc --noextattr --noacl "$driver" "$staging/root/Library/Audio/Plug-Ins/HAL/FxSound.driver"
xattr -cr "$staging/root"
cp "$packaging_dir/scripts/preinstall" "$staging/scripts/preinstall"
cp "$packaging_dir/scripts/personal-postinstall" "$staging/scripts/postinstall"
for arch in x86_64 arm64; do
    xcrun clang -O2 -Wall -Wextra -Werror -arch "$arch" -mmacosx-version-min=14.0 \
        -framework CoreAudio -framework CoreFoundation "$packaging_dir/audio-install-guard.c" \
        -o "$staging/guard-$arch"
done
xcrun lipo -create "$staging/guard-x86_64" "$staging/guard-arm64" -output "$staging/scripts/audio-install-guard"
codesign --sign - --options runtime --timestamp=none "$staging/scripts/audio-install-guard"
chmod 755 "$staging/scripts/"*
cp "$staging/scripts/audio-install-guard" "$staging/root/Applications/FxSound.app/Contents/Helpers/audio-install-guard"
for target in fxsound-engine fxsound-supervisor; do
    codesign --force --sign - --options runtime --timestamp=none \
        --entitlements "$packaging_dir/audio-input.entitlements" \
        "$staging/root/Applications/FxSound.app/Contents/Helpers/$target"
done
codesign --force --sign - --options runtime --timestamp=none \
    --entitlements "$packaging_dir/audio-input.entitlements" "$staging/root/Applications/FxSound.app"
codesign --verify --deep --strict "$staging/root/Applications/FxSound.app"
codesign --verify --strict "$staging/root/Library/Audio/Plug-Ins/HAL/FxSound.driver"
pkgbuild --root "$staging/root" --scripts "$staging/scripts" --ownership recommended \
    --identifier org.fxsound.personal --version "$version" --install-location / \
    "$staging/fxsound-component.pkg"
python3 - "$staging/root/Applications/FxSound.app/Contents/MacOS/FxSound" "$staging/requirements.plist" <<'PY'
import plistlib
import subprocess
import sys
architectures = subprocess.check_output(['xcrun', 'lipo', '-archs', sys.argv[1]], text=True).split()
with open(sys.argv[2], 'wb') as output:
    plistlib.dump({'os': ['14.0'], 'arch': architectures}, output)
PY
productbuild --synthesize --product "$staging/requirements.plist" \
    --package "$staging/fxsound-component.pkg" "$staging/Distribution.xml"
productbuild --distribution "$staging/Distribution.xml" --package-path "$staging" \
    "$staging/dmg/FxSound-Install.pkg"
cp "$packaging_dir/personal-readme.txt" "$staging/dmg/READ-ME.txt"
cp "$packaging_dir/uninstall-personal.command" "$staging/dmg/Uninstall-FxSound.command"
chmod 755 "$staging/dmg/Uninstall-FxSound.command"
python3 "$packaging_dir/personal-source.py" "$staging/dmg" "$juce_source"
hdiutil create -ov -format UDZO -volname FxSound -srcfolder "$staging/dmg" "$output/FxSound-macOS-personal.dmg"
cp "$staging/dmg/FxSound-Install.pkg" "$output/FxSound-Install.pkg"
shasum -a 256 "$output/FxSound-macOS-personal.dmg" "$output/FxSound-Install.pkg" > "$output/SHA256SUMS"
pkgutil --payload-files "$staging/fxsound-component.pkg" > "$output/package-payload.txt"
printf 'Personal installer: %s/FxSound-macOS-personal.dmg\n' "$output"
