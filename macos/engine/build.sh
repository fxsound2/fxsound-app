#!/bin/bash
set -euo pipefail
engine_dir=$(cd "$(dirname "$0")" && pwd)
output_dir=${1:?Usage: build.sh OUTPUT_DIRECTORY}
mkdir -p "$output_dir"
architectures=${FXSOUND_ARCHS:-"arm64 x86_64"}
for architecture in $architectures; do
    clang++ -std=c++20 -O2 -Wall -Wextra -Werror -fobjc-arc -mmacosx-version-min=14.0 \
        -arch "$architecture" "$engine_dir/audio-devices.cpp" "$engine_dir/audio-bridge.cpp" \
        "$engine_dir/ipc-server.cpp" "$engine_dir/recovery-channel.cpp" "$engine_dir/dsp-controller.cpp" "$engine_dir/dsp-controller-controls.cpp" \
        "$engine_dir/dsp-controller-storage.cpp" "$engine_dir/command-dispatcher.mm" "$engine_dir/engine-main.mm" \
        -framework CoreAudio -framework CoreFoundation -framework Foundation \
        -o "$output_dir/fxsound-engine.$architecture"
    clang++ -std=c++20 -O2 -Wall -Wextra -Werror -mmacosx-version-min=14.0 \
        -arch "$architecture" "$engine_dir/audio-devices.cpp" "$engine_dir/recovery-channel.cpp" "$engine_dir/supervisor-main.cpp" \
        -framework CoreAudio -framework CoreFoundation -o "$output_dir/fxsound-supervisor.$architecture"
done
for target in fxsound-engine fxsound-supervisor; do
    slices=()
    for architecture in $architectures; do slices+=("$output_dir/$target.$architecture"); done
    lipo -create "${slices[@]}" -output "$output_dir/$target"
    codesign --force --sign - --options runtime --timestamp=none \
        --entitlements "$engine_dir/../packaging/audio-input.entitlements" "$output_dir/$target"
    codesign --verify --strict "$output_dir/$target"
    lipo -archs "$output_dir/$target"
done
