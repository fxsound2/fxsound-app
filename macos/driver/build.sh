#!/bin/sh
set -eu
source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=${1:-/private/tmp/fxsound-driver-artifacts}
mkdir -p "$output_dir/FxSound.driver/Contents/MacOS" "$output_dir/slices" "$output_dir/FxSound.driver/Contents/Resources"
sdk=$(xcrun --sdk macosx --show-sdk-path)
for architecture in x86_64 arm64 arm64e; do
    xcrun clang -std=c11 -fvisibility=hidden -O2 -Wall -Wextra -Werror -arch "$architecture" \
        -isysroot "$sdk" -mmacosx-version-min=14.0 -bundle \
        -framework CoreAudio -framework CoreFoundation \
        "$source_dir"/src/*.c -o "$output_dir/slices/FxSound-$architecture"
done
xcrun lipo -create "$output_dir/slices/FxSound-x86_64" \
    "$output_dir/slices/FxSound-arm64" "$output_dir/slices/FxSound-arm64e" \
    -output "$output_dir/FxSound.driver/Contents/MacOS/FxSound"
cp "$source_dir/Info.plist" "$output_dir/FxSound.driver/Contents/Info.plist"
cp "$source_dir/apple-sample-license.txt" "$output_dir/FxSound.driver/Contents/Resources/Apple-License.txt"
plutil -lint "$output_dir/FxSound.driver/Contents/Info.plist"
codesign --force --sign - --options runtime --timestamp=none "$output_dir/FxSound.driver"
codesign --verify --strict --verbose=2 "$output_dir/FxSound.driver"
xcrun lipo -info "$output_dir/FxSound.driver/Contents/MacOS/FxSound"
printf 'Built offline bundle: %s/FxSound.driver\n' "$output_dir"
