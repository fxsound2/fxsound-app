#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${FXSOUND_TEST_BUILD_DIR:-/private/tmp/fxsound-engine-unit-tests}
library=${1:-/private/tmp/fxsound-dsp-independent-tsan/libfxsound-dsp.a}
mkdir -p "$build_dir"
flags="-fsanitize=thread"
if [ "${FXSOUND_TEST_ALLOCATIONS:-0}" = 1 ]; then flags="-DFXSOUND_TEST_ALLOCATIONS"; fi
xcrun clang++ -std=c++20 -O1 -g -Wall -Wextra -Werror $flags -DFXSOUND_HAVE_DSP -I "$root/dsp/include" -I "$root/macos/dsp" "$root/macos/tests/dsp-edit-gate-test.cpp" "$root/macos/engine/dsp-controller.cpp" "$root/macos/engine/dsp-controller-controls.cpp" "$root/macos/engine/dsp-controller-storage.cpp" "$root/macos/engine/audio-bridge.cpp" "$root/macos/engine/audio-devices.cpp" "$library" -framework CoreAudio -framework CoreFoundation -o "$build_dir/dsp-edit-gate-test"
TSAN_OPTIONS=halt_on_error=1 "$build_dir/dsp-edit-gate-test" "$root/Installer/Resources/Factsoft/Default.fac"
