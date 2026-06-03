# Real DfxDsp build (Linux). Enabled with -DFXSOUND_STUB_DSP=OFF.
#
# Compiles the platform-neutral DSP engine. Windows-only bits (COM licensing
# helpers, registry persistence) are routed through the non-Windows path in
# dsp/ptutil/include/codedefs.h and the Linux registry shim.

set(DSP_DIR ${CMAKE_CURRENT_LIST_DIR})

# Explicit source set, mirroring dsp/DfxDsp.vcxproj. (GLOB is unsafe here: the
# tree contains alternate/prototype variants of the DSP cores that are not part
# of the product and reference missing headers.)
set(DSP_REL_SOURCES
    DfxDsp.cpp DfxDspEq.cpp DfxDspPreset.cpp DfxDspPrivate.cpp DfxDspRegistry.cpp
    ptComSftDfx/Comsftwr.c ptComSftDfx/ComsftwrCPP.cpp
    ptechDsp/Aural/Aural032/Auralp32.c ptechDsp/Aural/Aural0/Auralp.c
    ptechDsp/Dly32/Dly832/dly8p32.c ptechDsp/Dly8/Dly8p.c
    ptechDsp/Lex/Lex16/Lex16.c ptechDsp/Lex/Lex32/Lex32.c
    ptechDsp/Maximizer/Maxi16/Maxi16.c ptechDsp/Maximizer/Maxi32/Maxi32.c
    ptechDsp/Peq/Peq832/peq8p32.c ptechDsp/Peq/Peq8/peq8p.c
    ptechDsp/Play/Play16/Play16.c ptechDsp/Play/Play32/Play32.c
    ptechDsp/wide/Wide16/Wide16.c ptechDsp/wide/Wide32/Wide32.c
    ptutil/COM/Com.cpp ptutil/COM/Comeprom.cpp ptutil/COM/Comget.cpp
    ptutil/COM/ComMem.cpp ptutil/COM/Compass.cpp ptutil/COM/Comread.cpp
    ptutil/COM/Comwave.cpp ptutil/COM/Comwrite.cpp
    ptutil/dfxp/dfxpComm.cpp ptutil/dfxp/dfxpEq.cpp ptutil/dfxp/dfxpGet.cpp
    ptutil/dfxp/dfxpInit.cpp ptutil/dfxp/dfxpProcessClear.cpp ptutil/dfxp/dfxpProcess.cpp
    ptutil/dfxp/dfxpProcessInt.cpp ptutil/dfxp/dfxpProcessReal.cpp ptutil/dfxp/dfxpQnt.cpp
    ptutil/dfxp/dfxpQuit.cpp ptutil/dfxp/dfxpRegistryStandard.cpp ptutil/dfxp/dfxpSession.cpp
    ptutil/dfxp/dfxpSet.cpp ptutil/dfxp/dfxpSpectrum.cpp ptutil/dfxp/dfxpUniversal.cpp
    ptutil/dfxSharedUtil/dfxSharedUtil.cpp
    ptutil/DspUtil/BinauralSync/BinauralSynGet.cpp ptutil/DspUtil/BinauralSync/BinauralSynInit.cpp
    ptutil/DspUtil/BinauralSync/BinauralSynProcess.cpp ptutil/DspUtil/BinauralSync/BinauralSynSet.cpp
    ptutil/DspUtil/GraphicEq/GraphicEqGet.cpp ptutil/DspUtil/GraphicEq/GraphicEqInitBands.cpp
    ptutil/DspUtil/GraphicEq/GraphicEqInit.cpp ptutil/DspUtil/GraphicEq/GraphicEqInitSections.cpp
    ptutil/DspUtil/GraphicEq/GraphicEqProcess.cpp ptutil/DspUtil/GraphicEq/GraphicEqSet.cpp
    ptutil/DspUtil/spectrum/spectrumGet.cpp ptutil/DspUtil/spectrum/spectrumInit.cpp
    ptutil/DspUtil/spectrum/spectrumMessageValues.cpp ptutil/DspUtil/spectrum/spectrumProcess.cpp
    ptutil/DspUtil/spectrum/spectrumReset.cpp ptutil/DspUtil/spectrum/spectrumSet.cpp
    ptutil/DspUtil/SurroundSyn/SurroundSynInit.cpp ptutil/DspUtil/SurroundSyn/SurroundSynProcess.cpp
    ptutil/Filt/Fil12But.cpp ptutil/Filt/FiltbiqdSos.cpp ptutil/Filt/FiltCalcBiqd.cpp
    ptutil/Filt/FiltCalcFilterResponse.cpp ptutil/Filt/Filtpoly.cpp ptutil/Filt/FiltRun.cpp
    ptutil/PRELST/Prelst.cpp
    # ptutil/PWAV/* is Windows multimedia (mmio/waveOut) WAV file I/O and is not
    # used by the audio-processing path; excluded on Linux.
    ptutil/Qnt/Qnt2But.cpp ptutil/Qnt/Qnt.cpp ptutil/Qnt/QntitoBoostCut.cpp ptutil/Qnt/Qntitol.cpp
    ptutil/Qnt/Qntitor2.cpp ptutil/Qnt/Qntitor.cpp ptutil/Qnt/Qntrtoi.cpp ptutil/Qnt/Qntrtol.cpp
    ptutil/Qnt/Qntrtor.cpp
    ptutil/realSample/realSampleForceLegalValues.cpp
    ptutil/SOS/Sos.cpp ptutil/SOS/SosGet.cpp ptutil/SOS/SosProcess.cpp ptutil/SOS/SosSet.cpp
    ptutil/VALS/Valscfg.cpp ptutil/VALS/Vals.cpp ptutil/VALS/Valsfile.cpp ptutil/VALS/Valsget.cpp
    ptutil/VALS/Valsset.cpp ptutil/VALS/Valswarp.cpp)

list(TRANSFORM DSP_REL_SOURCES PREPEND ${DSP_DIR}/ OUTPUT_VARIABLE DSP_SOURCES)

# Shared "common" utility sources (string/math/file/memory) live under
# audiopassthru/src and are linked by both the DSP and the audio backend on
# Windows. Compile the portable ones here. (reg/* is Windows registry ->
# replaced by reg_linux.cpp; fileWin32Handle.cpp is Win32-only.)
set(UTIL_DIR ${DSP_DIR}/../audiopassthru/src)
file(GLOB UTIL_SOURCES CONFIGURE_DEPENDS
    ${UTIL_DIR}/pstr/*.cpp
    ${UTIL_DIR}/MTH/*.cpp
    ${UTIL_DIR}/MRY/*.cpp)
# FileGeneral.cpp / FileDate.cpp / ptime are Win32 file-and-time-API based;
# the DSP only needs fileOpen_Wide/fileExist_Wide, provided by file_linux.cpp.

# Linux-only support sources (CSlout logger, registry shim).
list(APPEND DSP_SOURCES
    ${UTIL_SOURCES}
    ${DSP_DIR}/linux/slout_linux.cpp
    ${DSP_DIR}/linux/reg_linux.cpp
    ${DSP_DIR}/linux/pwav_convert_linux.cpp
    ${DSP_DIR}/linux/file_linux.cpp)

add_library(DfxDsp STATIC ${DSP_SOURCES})

# Every DSP subdirectory contributes headers; the original sources use bare
# includes resolved against these. Add them all.
file(GLOB DSP_INCLUDE_DIRS LIST_DIRECTORIES true ${DSP_DIR}/*)
# dsp/linux holds Linux stand-ins for <windows.h>/<conio.h>; it must come first.
target_include_directories(DfxDsp PRIVATE BEFORE ${DSP_DIR}/linux)
target_include_directories(DfxDsp PUBLIC
    ${DSP_DIR}/include
    ${DSP_DIR}/ptutil/include
    ${DSP_DIR}/../audiopassthru/include)
# Local headers (u_mth.h, U_FILE.H, ...) sit next to the util sources.
target_include_directories(DfxDsp PRIVATE
    ${DSP_DIR}/../audiopassthru/src/pstr
    ${DSP_DIR}/../audiopassthru/src/MTH
    ${DSP_DIR}/../audiopassthru/src/MRY
    ${DSP_DIR}/../audiopassthru/src/FILE
    ${DSP_DIR}/../audiopassthru/src/ptime)
foreach(d
    ${DSP_DIR}
    ${DSP_DIR}/ptComSftDfx
    ${DSP_DIR}/ptechDsp/Aural/Aural0   ${DSP_DIR}/ptechDsp/Aural/Aural032
    ${DSP_DIR}/ptechDsp/Dly32/Dly832   ${DSP_DIR}/ptechDsp/Dly8
    ${DSP_DIR}/ptechDsp/Lex/Lex16      ${DSP_DIR}/ptechDsp/Lex/Lex32
    ${DSP_DIR}/ptechDsp/Maximizer/Maxi16 ${DSP_DIR}/ptechDsp/Maximizer/Maxi32
    ${DSP_DIR}/ptechDsp/Peq/Peq8       ${DSP_DIR}/ptechDsp/Peq/Peq832
    ${DSP_DIR}/ptechDsp/Play/Play16    ${DSP_DIR}/ptechDsp/Play/Play32
    ${DSP_DIR}/ptechDsp/wide/Wide16    ${DSP_DIR}/ptechDsp/wide/Wide32
    ${DSP_DIR}/ptutil/COM              ${DSP_DIR}/ptutil/dfxp
    ${DSP_DIR}/ptutil/dfxSharedUtil    ${DSP_DIR}/ptutil/DspUtil/BinauralSync
    ${DSP_DIR}/ptutil/DspUtil/GraphicEq ${DSP_DIR}/ptutil/DspUtil/spectrum
    ${DSP_DIR}/ptutil/DspUtil/SurroundSyn ${DSP_DIR}/ptutil/PRELST
    ${DSP_DIR}/ptutil/PWAV             ${DSP_DIR}/ptutil/Qnt
    ${DSP_DIR}/ptutil/realSample       ${DSP_DIR}/ptutil/SOS
    ${DSP_DIR}/ptutil/VALS             ${DSP_DIR}/ptutil/Filt)
    target_include_directories(DfxDsp PRIVATE ${d})
endforeach()

# Case-insensitivity bridge: the Windows sources include headers with
# inconsistent casing. lc_headers holds symlinks (both original-cased and
# lowercase) so every variant resolves on a case-sensitive filesystem.
# lc_headers/ is gitignored; regenerate after a fresh clone with:
#   mkdir -p dsp/linux/lc_headers
#   find . -name '*.h' -not -path '*/lc_headers/*' -not -path '*/.git/*' | while read h; do
#     ln -sf "$(realpath "$h")" dsp/linux/lc_headers/$(basename "$h")
#     lc=$(basename "$h" | tr A-Z a-z)
#     ln -sf "$(realpath "$h")" dsp/linux/lc_headers/$lc
#   done
target_include_directories(DfxDsp PRIVATE ${DSP_DIR}/linux/lc_headers)

# Project defines from dsp/DfxDsp.vcxproj (minus WIN32). DSPSOFT_TARGET selects
# the software DSP path (no hardware dma32.h); PT_DSP_BUILD picks the DFX build.
# DSP self-test: exercises processAudio with no I/O.
add_executable(dsp_selftest ${DSP_DIR}/linux/test_dsp.cpp)
target_link_libraries(dsp_selftest PRIVATE DfxDsp)

target_compile_definitions(DfxDsp PRIVATE
    PT_LINUX=1
    _LIB
    PT_NON_MFC
    DSPSOFT_TARGET
    PT_DSP_BUILD=PT_DSP_DFX
    $<$<CONFIG:Debug>:_DEBUG>)
# DSP sources predate modern C/C++ conformance; relax to keep the port moving.
target_compile_options(DfxDsp PRIVATE
    -fpermissive
    -Wno-write-strings
    -Wno-narrowing
    -Wno-multichar)
set_source_files_properties(${DSP_SOURCES} PROPERTIES
    COMPILE_FLAGS "-Wno-implicit-function-declaration")
