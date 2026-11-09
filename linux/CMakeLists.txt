# Linux port — CMake build system for FxSound
# Windows .jucer/.vcxproj/.sln are untouched. This file is Linux-only.
cmake_minimum_required(VERSION 3.22)
project(FxSoundLinux VERSION 1.2.17 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

option(FXSOUND_LINUX_WERROR "Warnings-as-errors for new Linux files" ON)

# ---- DfxDsp (static, JUCE-free) ----
# Core DSP + ptutil, excluding Win32-only harness and WinMM file-play path.
# Matches DfxDsp.vcxproj ClCompile list (108 files): Filt, DspUtil/*/*, ptechDsp .c
# 3-level, ptComSftDfx .c. Recursive glob to avoid missing subdirs.
file(GLOB_RECURSE DFXDSP_CORE
  ${REPO_ROOT}/dsp/DfxDsp.cpp ${REPO_ROOT}/dsp/DfxDspPrivate.cpp ${REPO_ROOT}/dsp/DfxDspPreset.cpp ${REPO_ROOT}/dsp/DfxDspEq.cpp ${REPO_ROOT}/dsp/DfxDspRegistry.cpp
  ${REPO_ROOT}/dsp/ptutil/VALS/*.cpp ${REPO_ROOT}/dsp/ptutil/PRELST/*.cpp ${REPO_ROOT}/dsp/ptutil/dfxp/*.cpp
  ${REPO_ROOT}/dsp/ptutil/dfxSharedUtil/*.cpp ${REPO_ROOT}/dsp/ptutil/COM/*.cpp
  ${REPO_ROOT}/dsp/ptutil/pstr/*.cpp ${REPO_ROOT}/dsp/ptutil/ptime/*.cpp
  ${REPO_ROOT}/dsp/ptutil/realSample/*.cpp ${REPO_ROOT}/dsp/ptutil/Qnt/*.cpp
  ${REPO_ROOT}/dsp/ptutil/SOS/*.cpp ${REPO_ROOT}/dsp/ptutil/Filt/*.cpp
  ${REPO_ROOT}/dsp/ptutil/DspUtil/*.cpp ${REPO_ROOT}/dsp/ptutil/DspUtil/*/*.cpp
  ${REPO_ROOT}/dsp/ptComSftDfx/*.cpp ${REPO_ROOT}/dsp/ptComSftDfx/*.c
  ${REPO_ROOT}/dsp/ptutil/PWAV/PwavConvert.cpp ${REPO_ROOT}/dsp/ptutil/PWAV/PwavUtil.cpp
  ${REPO_ROOT}/dsp/ptechDsp/Aural/Aural032/Auralp32.c ${REPO_ROOT}/dsp/ptechDsp/Aural/Aural0/Auralp.c
  ${REPO_ROOT}/dsp/ptechDsp/Dly32/Dly832/dly8p32.c ${REPO_ROOT}/dsp/ptechDsp/Dly8/Dly8p.c
  ${REPO_ROOT}/dsp/ptechDsp/Lex/Lex16/Lex16.c ${REPO_ROOT}/dsp/ptechDsp/Lex/Lex32/Lex32.c
  ${REPO_ROOT}/dsp/ptechDsp/Maximizer/Maxi16/Maxi16.c ${REPO_ROOT}/dsp/ptechDsp/Maximizer/Maxi32/Maxi32.c
  ${REPO_ROOT}/dsp/ptechDsp/Peq/Peq832/peq8p32.c ${REPO_ROOT}/dsp/ptechDsp/Peq/Peq8/peq8p.c
  ${REPO_ROOT}/dsp/ptechDsp/Play/Play16/Play16.c ${REPO_ROOT}/dsp/ptechDsp/Play/Play32/Play32.c
  ${REPO_ROOT}/dsp/ptechDsp/wide/Wide16/Wide16.c ${REPO_ROOT}/dsp/ptechDsp/wide/Wide32/Wide32.c
)

add_library(DfxDsp STATIC ${DFXDSP_CORE})
target_include_directories(DfxDsp PUBLIC
  ${REPO_ROOT}/dsp/ptutil/include ${REPO_ROOT}/dsp/include ${REPO_ROOT}/dsp ${REPO_ROOT}/audiopassthru/include
  ${REPO_ROOT}/dsp/ptutil/VALS ${REPO_ROOT}/dsp/ptutil/PRELST ${REPO_ROOT}/dsp/ptutil/PWAV ${REPO_ROOT}/dsp/ptutil/dfxp
)
# compat/ ONLY for DfxDsp/ptutil (never PipeWire). No force-include.
target_include_directories(DfxDsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/compat)
target_compile_definitions(DfxDsp PRIVATE JUCE_MODAL_LOOPS_PERMITTED DSPSOFT_TARGET PT_NON_MFC PT_DSP_BUILD=2)
target_compile_options(DfxDsp PRIVATE -O2 -ffp-contract=off -fno-strict-aliasing -fwrapv)

# ---- AudioPassthruCore (static, no WASAPI, no PipeWire) ----
file(GLOB AP_CORE
  ${REPO_ROOT}/audiopassthru/src/MRY/*.cpp ${REPO_ROOT}/audiopassthru/src/MTH/*.cpp
  ${REPO_ROOT}/audiopassthru/src/pstr/*.cpp ${REPO_ROOT}/audiopassthru/src/ptime/*.cpp
  ${REPO_ROOT}/audiopassthru/src/SLOUT/*.cpp
)
add_library(AudioPassthruCore STATIC ${AP_CORE}
  ${CMAKE_CURRENT_SOURCE_DIR}/registry_posix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dsp_registry_posix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/os_posix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/file_reg_posix.cpp
)
target_include_directories(AudioPassthruCore PUBLIC
  ${REPO_ROOT}/audiopassthru/include ${REPO_ROOT}/dsp/include ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(AudioPassthruCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/compat)
target_link_libraries(AudioPassthruCore PUBLIC DfxDsp)

# ---- AudioPassthruPW (PipeWire backend) ----
add_library(AudioPassthruPW STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sndDevices_pw.cpp
)
target_include_directories(AudioPassthruPW PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ${REPO_ROOT}/audiopassthru/include ${REPO_ROOT}/dsp/include
)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND OR PKG_CONFIG_FOUND)
  pkg_check_modules(PIPEWIRE QUIET libpipewire-0.3)
  pkg_check_modules(SYSTEMD QUIET libsystemd)
  if(PIPEWIRE_FOUND)
    target_include_directories(AudioPassthruPW PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    target_compile_options(AudioPassthruPW PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
    target_link_libraries(AudioPassthruPW PUBLIC ${PIPEWIRE_LIBRARIES})
  endif()
  find_package(Threads REQUIRED)
  target_link_libraries(AudioPassthruPW PUBLIC Threads::Threads)
endif()

if(FXSOUND_LINUX_WERROR)
  target_compile_options(AudioPassthruPW PRIVATE -Wall -Wextra -Werror)
endif()
target_compile_options(AudioPassthruCore PRIVATE -Wall -Wextra)
target_compile_options(DfxDsp PRIVATE -Wall -Wextra)

# ---- FxSound GUI (JUCE) ----
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE")
if(EXISTS ${JUCE_DIR}/CMakeLists.txt)
  # Patch JUCE 6.1.6 for GCC 16+ (<utility> missing for std::exchange due to header pruning)
  set(JUCE_STD_HEADER "${JUCE_DIR}/modules/juce_core/system/juce_StandardHeader.h")
  if(EXISTS "${JUCE_STD_HEADER}")
    file(READ "${JUCE_STD_HEADER}" _JUCE_HEADER_CONTENT)
    if(NOT _JUCE_HEADER_CONTENT MATCHES "<utility>")
      string(REPLACE "#include <algorithm>" "#include <algorithm>\n#include <utility>  /* FxSound Linux port patch: std::exchange (GCC16 header pruning) */" _JUCE_HEADER_FIXED "${_JUCE_HEADER_CONTENT}")
      file(WRITE "${JUCE_STD_HEADER}" "${_JUCE_HEADER_FIXED}")
      message(STATUS "Patched JUCE with #include <utility> for GCC 16+")
    endif()
  endif()

  add_subdirectory(${JUCE_DIR} EXCLUDE_FROM_ALL)
  file(GLOB FX_GUI
    ${REPO_ROOT}/fxsound/Source/Main.cpp ${REPO_ROOT}/fxsound/Source/GUI/*.cpp
    ${REPO_ROOT}/fxsound/Source/Utils/Settings/*.cpp
    ${REPO_ROOT}/fxsound/Source/Utils/SysInfo/*.cpp
  )
  list(FILTER FX_GUI EXCLUDE REGEX ".*FxSystemTrayView\\.cpp$")

  file(GLOB FX_BIN_RESOURCES LIST_DIRECTORIES false
    ${REPO_ROOT}/fxsound/Images/*.svg ${REPO_ROOT}/fxsound/Images/*.png
    ${REPO_ROOT}/fxsound/Fonts/Gilroy*.ttf
    ${REPO_ROOT}/Resources/Strings/*.txt
  )
  list(FILTER FX_BIN_RESOURCES EXCLUDE REGEX "button-(depressed|hover|normal)\\.svg$")
  list(FILTER FX_BIN_RESOURCES EXCLUDE REGEX "FxSound Logo White\\.png$")
  list(FILTER FX_BIN_RESOURCES EXCLUDE REGEX "FxSound White Bars\\.png$")
  list(FILTER FX_BIN_RESOURCES EXCLUDE REGEX "logo-red\\.png$")
  list(FILTER FX_BIN_RESOURCES EXCLUDE REGEX "logo-white\\.png$")
  juce_add_binary_data(BinaryData SOURCES ${FX_BIN_RESOURCES})

  juce_add_gui_app(FxSound PRODUCT_NAME "FxSound" VERSION 1.2.17 NEEDS_CURL TRUE)
  target_sources(FxSound PRIVATE ${FX_GUI}
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioPassthru_linux.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/single_instance_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/registry_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/os_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crash_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/paths_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/autostart_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/notify_posix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tray_sni.cpp
  )
  target_include_directories(FxSound PRIVATE
    ${REPO_ROOT}/fxsound/Source ${REPO_ROOT}/fxsound/Source/GUI
    ${REPO_ROOT}/audiopassthru/include ${REPO_ROOT}/dsp/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${REPO_ROOT}/Resources/Strings
  )
  target_compile_definitions(FxSound PRIVATE
    JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1 JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_VERSION_STRING="1.2.17"
  )
  target_link_libraries(FxSound PRIVATE
    juce::juce_gui_extra juce::juce_gui_basics juce::juce_graphics
    juce::juce_data_structures juce::juce_events juce::juce_core juce::juce_cryptography
    BinaryData
    DfxDsp AudioPassthruPW AudioPassthruCore
  )
  if(SYSTEMD_FOUND)
    target_include_directories(FxSound PRIVATE ${SYSTEMD_INCLUDE_DIRS})
    target_link_libraries(FxSound PRIVATE ${SYSTEMD_LIBRARIES})
    target_compile_definitions(FxSound PRIVATE HAVE_SYSTEMD=1)
  endif()
  juce_generate_juce_header(FxSound)

  add_custom_command(TARGET FxSound POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${REPO_ROOT}/bin/x64/Factsoft
      $<TARGET_FILE_DIR:FxSound>/Factsoft
    COMMENT "Copying Factsoft factory presets to build output"
  )
else()
  message(WARNING "JUCE missing (expected in ${JUCE_DIR}) — FxSound GUI skipped. DSP target still builds.")
endif()

include(GNUInstallDirs)
if(TARGET FxSound)
  install(TARGETS FxSound RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/fxsound.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/fxsound.service DESTINATION ${CMAKE_INSTALL_DATADIR}/systemd/user)
  install(DIRECTORY ${REPO_ROOT}/bin/x64/Factsoft DESTINATION ${CMAKE_INSTALL_DATADIR}/fxsound)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/packaging/com.fxsound.FxSound.metainfo.xml)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/com.fxsound.FxSound.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
  endif()
  if(EXISTS ${REPO_ROOT}/fxsound/Images/fxsound.png)
    install(FILES ${REPO_ROOT}/fxsound/Images/fxsound.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps
    )
  endif()
  if(EXISTS ${REPO_ROOT}/fxsound/Images/fxsound_large.png)
    install(FILES ${REPO_ROOT}/fxsound/Images/fxsound_large.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
      RENAME fxsound.png
    )
  endif()
endif()

# ==============================================================================
# CPack Packaging Configuration (.deb, .rpm, .tar.gz)
# ==============================================================================
set(CPACK_PACKAGE_NAME "fxsound")
set(CPACK_PACKAGE_VENDOR "FxSound")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-fidelity audio enhancer, equalizer, and effects for Linux via PipeWire")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pourih/fxsound-linux")
set(CPACK_PACKAGE_CONTACT "pouria802@gmail.com")
set(CPACK_PACKAGE_LICENSE "AGPL-3.0-or-later")
set(CPACK_RESOURCE_FILE_LICENSE "${REPO_ROOT}/LICENSE")

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 2)
set(CPACK_PACKAGE_VERSION_PATCH 17)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

# Generators
set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_SOURCE_GENERATOR "TGZ")

# Debian (.deb)
set(CPACK_DEBIAN_PACKAGE_NAME "fxsound")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pouria802@gmail.com")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "pipewire, wireplumber")
if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
  else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()
set(CPACK_DEBIAN_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")

# RPM (.rpm)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tmp")
set(CPACK_RPM_PACKAGE_NAME "fxsound")
set(CPACK_RPM_PACKAGE_LICENSE "AGPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_URL "https://github.com/pourih/fxsound-linux")
set(CPACK_RPM_PACKAGE_REQUIRES "pipewire, wireplumber, pipewire-utils")
set(CPACK_RPM_PACKAGE_AUTOREQPROV "yes")
set(CPACK_RPM_SPEC_MORE_DEFINE "%define _tmppath ${CMAKE_BINARY_DIR}/tmp")
set(CPACK_RPM_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-1.${CMAKE_SYSTEM_PROCESSOR}.rpm")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
  /usr/share/applications
  /usr/share/icons
  /usr/share/icons/hicolor
  /usr/share/icons/hicolor/48x48
  /usr/share/icons/hicolor/48x48/apps
  /usr/share/icons/hicolor/256x256
  /usr/share/icons/hicolor/256x256/apps
  /usr/share/icons/hicolor/scalable
  /usr/share/icons/hicolor/scalable/apps
  /usr/share/metainfo
  /usr/share/systemd
  /usr/share/systemd/user
)
set(CPACK_RPM_DEFAULT_DIR_PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)
set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)

# Archive (.tar.gz)
set(CPACK_ARCHIVE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)
