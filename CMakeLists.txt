# FxSound — Linux/CMake build
#
# This build targets Linux (PipeWire). It replicates the Projucer-exported
# build for the JUCE GUI app and links the DSP + audio-passthru components.
# On Windows continue to use the Visual Studio solutions under fxsound/Project.

cmake_minimum_required(VERSION 3.20)
project(FxSound VERSION 1.2.8.0 LANGUAGES C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This CMake build is Linux-only. Use the VS solutions on Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- Options --------------------------------------------------------------
# M0 bootstrap: stub the DSP and audio backends so the GUI compiles & launches.
# Later milestones flip these on to build the real ported DSP (M1) and the
# PipeWire backend (M2/M3).
# DSP is ported (M1) and the PipeWire backend works (M2-M4): default to both.
option(FXSOUND_STUB_DSP        "Use stub DfxDsp instead of the ported engine"    OFF)
option(FXSOUND_STUB_AUDIO      "Use stub AudioPassthru instead of PipeWire"       OFF)

set(FX_ROOT       ${CMAKE_CURRENT_SOURCE_DIR})
set(JUCE_DIR      ${FX_ROOT}/third_party/JUCE)
set(JUCE_MODULES  ${JUCE_DIR}/modules)
set(FX_GUI        ${FX_ROOT}/fxsound/Source)
set(FX_JLC        ${FX_ROOT}/fxsound/JuceLibraryCode)

if(NOT EXISTS ${JUCE_MODULES}/juce_core/juce_core.h)
    message(FATAL_ERROR "JUCE 6.1.6 modules not found at ${JUCE_MODULES}. "
                        "Run: git clone --depth 1 --branch 6.1.6 "
                        "https://github.com/juce-framework/JUCE third_party/JUCE")
endif()

# ===========================================================================
#  DfxDsp library
# ===========================================================================
if(FXSOUND_STUB_DSP)
    add_library(DfxDsp STATIC ${FX_ROOT}/linux/stubs/DfxDsp_stub.cpp)
    target_include_directories(DfxDsp PUBLIC ${FX_ROOT}/dsp/include)
else()
    # M1 wires the real ported DSP sources here.
    include(${FX_ROOT}/dsp/DfxDsp.cmake)
endif()

# ===========================================================================
#  AudioPassthru library
# ===========================================================================
if(FXSOUND_STUB_AUDIO)
    add_library(AudioPassthru STATIC ${FX_ROOT}/linux/stubs/AudioPassthru_stub.cpp)
    target_include_directories(AudioPassthru PUBLIC ${FX_ROOT}/audiopassthru/include)
    target_link_libraries(AudioPassthru PUBLIC DfxDsp)
else()
    # M2/M3 wire the real PipeWire backend here.
    include(${FX_ROOT}/audiopassthru/AudioPassthruLinux.cmake)
endif()

# ===========================================================================
#  FxSound GUI app (JUCE)  — replicates the Projucer Linux export
# ===========================================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI_DEPS REQUIRED
    x11 xext xinerama xrandr xcursor
    freetype2 fontconfig gl alsa
    xcb xcb-keysyms)
# GIO/GDBus for the StatusNotifierItem tray.
pkg_check_modules(GIO REQUIRED gio-2.0)
find_package(Threads REQUIRED)

file(GLOB_RECURSE FX_GUI_SOURCES CONFIGURE_DEPENDS ${FX_GUI}/*.cpp)
# MainComponent.* is a leftover Projucer default (references a missing
# AnnouncementComponent.h) and is not part of the app — exclude it.
list(FILTER FX_GUI_SOURCES EXCLUDE REGEX "MainComponent\\.cpp$")
# SysInfo.cpp is COM/WASAPI based; the Linux reimplementation replaces it.
list(FILTER FX_GUI_SOURCES EXCLUDE REGEX "SysInfo/SysInfo\\.cpp$")
list(APPEND FX_GUI_SOURCES ${FX_ROOT}/linux/SysInfo_linux.cpp)

set(JUCE_WRAPPERS
    ${FX_JLC}/include_juce_core.cpp
    ${FX_JLC}/include_juce_cryptography.cpp
    ${FX_JLC}/include_juce_data_structures.cpp
    ${FX_JLC}/include_juce_events.cpp
    ${FX_JLC}/include_juce_graphics.cpp
    ${FX_JLC}/include_juce_gui_basics.cpp
    ${FX_JLC}/include_juce_gui_extra.cpp
    ${FX_JLC}/include_juce_opengl.cpp)

add_executable(FxSound
    ${FX_GUI_SOURCES}
    ${JUCE_WRAPPERS}
    ${FX_JLC}/BinaryData.cpp
    ${FX_ROOT}/linux/tray/fx_tray_sni.cpp
    ${FX_ROOT}/linux/LinuxHotkeys.cpp)

target_include_directories(FxSound PRIVATE
    ${FX_JLC}
    ${FX_GUI}
    ${FX_GUI}/GUI
    ${FX_GUI}/Utils/SysInfo
    ${FX_ROOT}/linux
    ${FX_ROOT}/linux/tray
    ${JUCE_MODULES}
    ${GIO_INCLUDE_DIRS})

# JUCE module configuration. AppConfig.h carries the module-availability flags;
# here we set the platform/runtime flags the Projucer would inject.
target_compile_definitions(FxSound PRIVATE
    LINUX=1
    JUCE_APP_VERSION=1.2.8.0
    JUCE_APP_VERSION_HEX=0x1020800
    JUCER_LINUX_MAKE=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    $<$<CONFIG:Debug>:DEBUG=1 _DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>)

target_link_libraries(FxSound PRIVATE
    DfxDsp
    AudioPassthru
    ${GUI_DEPS_LIBRARIES}
    ${GIO_LIBRARIES}
    Threads::Threads
    ${CMAKE_DL_LIBS})

target_link_directories(FxSound PRIVATE ${GUI_DEPS_LIBRARY_DIRS})
target_include_directories(FxSound PRIVATE ${GUI_DEPS_INCLUDE_DIRS})
target_compile_options(FxSound PRIVATE ${GUI_DEPS_CFLAGS_OTHER})

# ===========================================================================
#  Install (M6) — standard freedesktop layout
# ===========================================================================
include(GNUInstallDirs)

# Install the binary as lowercase "fxsound" (matches the .desktop Exec and the
# Linux convention); the in-tree build artifact keeps its Projucer name.
install(PROGRAMS $<TARGET_FILE:FxSound>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME fxsound)

install(FILES ${FX_ROOT}/packaging/fxsound.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES ${FX_ROOT}/fxsound/Images/fxsound_large.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME fxsound.png)
install(FILES ${FX_ROOT}/fxsound/Images/fxsound.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps
        RENAME fxsound.png)
install(CODE "execute_process(COMMAND gtk-update-icon-cache -qtf \
    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor\" \
    ERROR_QUIET)")

# Factory presets: the app loads these from <prefix>/share/fxsound/Factsoft
# (see FxController::getFactoryPresetDir).
if(EXISTS ${FX_ROOT}/Installer/Resources/Factsoft)
    install(DIRECTORY ${FX_ROOT}/Installer/Resources/Factsoft
            DESTINATION ${CMAKE_INSTALL_DATADIR}/fxsound)
endif()
if(EXISTS ${FX_ROOT}/bin/BonusPresets)
    install(DIRECTORY ${FX_ROOT}/bin/BonusPresets
            DESTINATION ${CMAKE_INSTALL_DATADIR}/fxsound)
endif()
