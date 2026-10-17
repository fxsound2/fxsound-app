# Real AudioPassthru backend (Linux/PipeWire). Enabled with -DFXSOUND_STUB_AUDIO=OFF.

set(AP_DIR ${CMAKE_CURRENT_LIST_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

add_library(AudioPassthru STATIC
    ${AP_DIR}/src/linux/AudioPassthruPipeWire.cpp)

target_include_directories(AudioPassthru PUBLIC ${AP_DIR}/include)
target_include_directories(AudioPassthru PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_compile_options(AudioPassthru PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
target_link_libraries(AudioPassthru PUBLIC DfxDsp PRIVATE ${PIPEWIRE_LIBRARIES})

# PipeWire passthrough test: brings the node up in the live graph.
add_executable(passthru_selftest ${AP_DIR}/src/linux/test_passthru.cpp)
target_link_libraries(passthru_selftest PRIVATE AudioPassthru ${PIPEWIRE_LIBRARIES})

# Realtime process block against the real DSP; no PipeWire needed at runtime.
add_executable(process_selftest ${AP_DIR}/src/linux/test_process.cpp)
target_link_libraries(process_selftest PRIVATE DfxDsp)
add_test(NAME process_selftest COMMAND process_selftest)
set_tests_properties(process_selftest PROPERTIES LABELS unit
    ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config/process")

# Live-graph tests need a running PipeWire session; they skip (77) without one.
add_test(NAME routing_follows_default_sink
    COMMAND bash ${AP_DIR}/src/linux/test_routing_live.sh $<TARGET_FILE:passthru_selftest>)
set_tests_properties(routing_follows_default_sink PROPERTIES
    LABELS pw-live SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
