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
