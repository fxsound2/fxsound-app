#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${FXSOUND_TEST_BUILD_DIR:-/private/tmp/fxsound-engine-unit-tests}
library=${1:-/private/tmp/fxsound-dsp-independent-tsan/libfxsound-dsp.a}
mkdir -p "$build_dir"
xcrun clang++ -std=c++17 -O2 -g -Wall -Wextra -Werror -fsanitize=thread -I "$root/dsp/include" "$root/macos/tests/dsp-concurrency-test.cpp" "$library" -o "$build_dir/dsp-concurrency-test"
TSAN_OPTIONS=halt_on_error=1 "$build_dir/dsp-concurrency-test" "$root/Installer/Resources/Factsoft/Default.fac"
