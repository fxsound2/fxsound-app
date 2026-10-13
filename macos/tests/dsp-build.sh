#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${FXSOUND_TEST_BUILD_DIR:-/private/tmp/fxsound-engine-unit-tests}
mkdir -p "$build_dir"
optimized=${1:-/private/tmp/fxsound-dsp-independent-optimized/libfxsound-dsp.a}
sanitized=${2:-/private/tmp/fxsound-dsp-independent-sanitize/libfxsound-dsp.a}
xcrun clang++ -std=c++17 -O2 -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror -I "$root/dsp/include" "$root/macos/tests/dsp-engine-test.cpp" "$optimized" -o "$build_dir/dsp-engine-test"
"$build_dir/dsp-engine-test" "$root"
xcrun clang++ -std=c++17 -O2 -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror -I "$root/dsp/include" "$root/macos/tests/dsp-allocation-test.cpp" "$optimized" -o "$build_dir/dsp-allocation-test"
"$build_dir/dsp-allocation-test" "$root/Installer/Resources/Factsoft/Default.fac"
xcrun clang++ -std=c++17 -O2 -g -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I "$root/dsp/include" "$root/macos/tests/dsp-engine-test.cpp" "$sanitized" -o "$build_dir/dsp-engine-test-sanitize"
UBSAN_OPTIONS=halt_on_error=1 "$build_dir/dsp-engine-test-sanitize" "$root"
xcrun clang++ -std=c++17 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I "$root/dsp/include" "$root/macos/tests/dsp-eq-resize-test.cpp" "$sanitized" -o "$build_dir/dsp-eq-resize-test"
UBSAN_OPTIONS=halt_on_error=1 "$build_dir/dsp-eq-resize-test" "$root/Installer/Resources/Factsoft/Default.fac"
