#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${FXSOUND_TEST_BUILD_DIR:-/private/tmp/fxsound-engine-unit-tests}
library=${1:-/private/tmp/fxsound-dsp-independent-optimized/libfxsound-dsp.a}
mkdir -p "$build_dir"
xcrun clang++ -std=c++20 -O1 -g -Wall -Wextra -Werror -fobjc-arc -DFXSOUND_HAVE_DSP -I "$root/dsp/include" -I "$root/macos/dsp" "$root/macos/tests/command-dsp-schema-test.mm" "$root/macos/engine/command-dispatcher.mm" "$root/macos/engine/dsp-controller.cpp" "$root/macos/engine/dsp-controller-controls.cpp" "$root/macos/engine/dsp-controller-storage.cpp" "$root/macos/engine/audio-bridge.cpp" "$root/macos/engine/audio-devices.cpp" "$library" -framework CoreAudio -framework CoreFoundation -framework Foundation -o "$build_dir/command-dsp-schema-test"
if [ "${FXSOUND_TEST_DEVICE_READ:-0}" = 1 ]; then "$build_dir/command-dsp-schema-test"; fi
