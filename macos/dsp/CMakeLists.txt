cmake_minimum_required(VERSION 3.22)
project(FxSoundDSP LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(FXSOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
include(dsp-sources.cmake)
add_library(fxsound-dsp STATIC ${FXSOUND_DSP_SOURCES} preset-validation.cpp eq-resize.cpp dsp-layout.cpp float32-engine.cpp portable-runtime.cpp portable-files.cpp portable-settings.cpp portable-shared.cpp portable-logging.cpp
 "${FXSOUND_ROOT}/audiopassthru/src/MTH/MthUtil.cpp"
 "${FXSOUND_ROOT}/audiopassthru/src/MTH/Mthfreq.cpp"
 "${FXSOUND_ROOT}/audiopassthru/src/MTH/MthBuffer.cpp")
target_compile_features(fxsound-dsp PUBLIC cxx_std_17)
target_include_directories(fxsound-dsp PUBLIC "${FXSOUND_ROOT}/dsp/include" "${CMAKE_CURRENT_SOURCE_DIR}"
 PRIVATE "${FXSOUND_ROOT}/dsp/ptutil/include")
target_compile_definitions(fxsound-dsp PRIVATE PT_PORTABLE_DSP PT_NON_MFC DSPSOFT_TARGET PT_DSP_BUILD=PT_DSP_DFX)
target_compile_options(fxsound-dsp PRIVATE -O2 -fno-fast-math -ffp-contract=off
 -Wno-writable-strings -Wno-deprecated-declarations
 -Werror=pointer-to-int-cast -Werror=int-to-pointer-cast
 "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/portable-runtime.h")
option(FXSOUND_SANITIZE "Enable address and undefined behavior sanitizers" OFF)
if(FXSOUND_SANITIZE)
 target_compile_options(fxsound-dsp PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
 target_link_options(fxsound-dsp PUBLIC -fsanitize=address,undefined)
endif()
add_executable(fxsound-render ../dsp-test/offline-render.cpp ../dsp-test/wav-file.cpp)
target_link_libraries(fxsound-render PRIVATE fxsound-dsp)
add_executable(fxsound-compare ../dsp-test/compare-renders.cpp ../dsp-test/wav-file.cpp)
target_compile_features(fxsound-compare PRIVATE cxx_std_17)
