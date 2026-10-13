#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${FXSOUND_TEST_BUILD_DIR:-/private/tmp/fxsound-engine-unit-tests}
library=${1:-/private/tmp/fxsound-personal-gui-8-build/dsp/libfxsound-dsp.a}
flags=${FXSOUND_TEST_FLAGS:--O2 -g}
mkdir -p "$build_dir"
xcrun clang++ -std=c++17 -Wall -Wextra -Werror $flags -DPT_PORTABLE_DSP -DPT_NON_MFC \
    -DDSPSOFT_TARGET -DPT_DSP_BUILD=PT_DSP_DFX -include "$root/macos/dsp/portable-runtime.h" \
    -I "$root/dsp/include" -I "$root/dsp/ptutil/include" -I "$root/macos/dsp" \
    "$root/macos/tests/dsp-preset-switch-test.cpp" "$library" -o "$build_dir/dsp-preset-switch-test"
UBSAN_OPTIONS=halt_on_error=1 "$build_dir/dsp-preset-switch-test" "$root/Installer/Resources/Factsoft"
