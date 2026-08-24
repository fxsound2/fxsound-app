cmake_minimum_required(VERSION 3.16)
project(DfxDspPortability CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This target currently covers only the settings-storage portability layer
# introduced to decouple dsp/ from the Windows Registry
# (ptutil/dfxKeyValueStore/DfxKeyValueStore.cpp). It is NOT yet a build of
# the full DfxDsp static library -- see dsp/DfxDsp.vcxproj for that, and
# the macOS port plan for the remaining work to get the rest of the DSP
# engine building here too. Most sources under ptechDsp/ and ptutil/ have
# not yet been audited for other Windows-only dependencies.
add_library(DfxKeyValueStore STATIC
    ptutil/dfxKeyValueStore/DfxKeyValueStore.cpp
)
target_include_directories(DfxKeyValueStore PUBLIC
    ptutil/include
)

option(DFXDSP_BUILD_TESTS "Build DfxDsp portability unit tests" ON)
if(DFXDSP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
