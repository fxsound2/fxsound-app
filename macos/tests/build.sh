#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_dir=${FXSOUND_TEST_BUILD_DIR:-/private/tmp/fxsound-engine-unit-tests}
mkdir -p "$build_dir"
flags=${FXSOUND_TEST_FLAGS:--O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer}
for name in stereo-ring adaptive-resampler; do
    # Flags are intentionally word-split to accept compiler options from the test runner.
    xcrun clang++ -std=c++17 -Wall -Wextra -Werror $flags "$root/$name-test.cpp" -o "$build_dir/$name-test"
    "$build_dir/$name-test"
done
xcrun clang++ -std=c++17 -Wall -Wextra -Werror $flags "$root/audio-json-test.cpp" "$root/../engine/audio-devices.cpp" -framework CoreAudio -framework CoreFoundation -o "$build_dir/audio-json-test"
"$build_dir/audio-json-test"
xcrun clang++ -std=c++17 -Wall -Wextra -Werror $flags "$root/audio-bridge-test.cpp" "$root/../engine/audio-bridge.cpp" "$root/../engine/audio-devices.cpp" -framework CoreAudio -framework CoreFoundation -o "$build_dir/audio-bridge-test"
"$build_dir/audio-bridge-test"
xcrun clang++ -std=c++17 -Wall -Wextra -Werror $flags "$root/ipc-server-test.cpp" "$root/../engine/ipc-server.cpp" -o "$build_dir/ipc-server-test"
if [ "${FXSOUND_TEST_IPC:-0}" = 1 ]; then "$build_dir/ipc-server-test"; fi
xcrun clang++ -std=c++20 -Wall -Wextra -Werror -fobjc-arc $flags "$root/command-schema-test.mm" "$root/../engine/command-dispatcher.mm" "$root/../engine/dsp-controller.cpp" "$root/../engine/dsp-controller-controls.cpp" "$root/../engine/dsp-controller-storage.cpp" "$root/../engine/audio-bridge.cpp" "$root/../engine/audio-devices.cpp" -framework CoreAudio -framework CoreFoundation -framework Foundation -o "$build_dir/command-schema-test"
if [ "${FXSOUND_TEST_DEVICE_READ:-0}" = 1 ]; then "$build_dir/command-schema-test"; fi
xcrun clang++ -std=c++17 -Wall -Wextra -Werror $flags "$root/recovery-channel-test.cpp" "$root/../engine/recovery-channel.cpp" -o "$build_dir/recovery-channel-test"
"$build_dir/recovery-channel-test"
