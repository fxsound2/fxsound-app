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
# inconsistent casing ("C_aural.h" for c_aural.h). At configure time, link every
# include spelling used by the DSP sources and headers that names a project
# header, plus each header's own and lowercase name, to the real file. The
# links live in the build tree, so a fresh clone needs no manual step. Several names exist in
# more than one copy (e.g. pt_defs.h in dsp/ptutil/include and
# audiopassthru/include); later lists win, so the DSP's own copies are preferred.
set(DSP_LC_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/lc_headers)
file(REMOVE_RECURSE ${DSP_LC_HEADERS})
file(MAKE_DIRECTORY ${DSP_LC_HEADERS})
file(GLOB_RECURSE _lc_ap_headers ${DSP_DIR}/../audiopassthru/*.h ${DSP_DIR}/../audiopassthru/*.H)
file(GLOB_RECURSE _lc_dsp_headers ${DSP_DIR}/*.h ${DSP_DIR}/*.H)
file(GLOB _lc_core_headers ${DSP_DIR}/ptutil/include/*.h)
list(FILTER _lc_dsp_headers EXCLUDE REGEX "/lc_headers/")
set(_lc_names "")
foreach(h IN LISTS _lc_ap_headers _lc_dsp_headers _lc_core_headers)
    get_filename_component(n ${h} NAME)
    string(TOLOWER ${n} lc)
    set(_lc_target_${lc} ${h})
    list(APPEND _lc_names ${n} ${lc})
endforeach()
foreach(f IN LISTS DSP_SOURCES _lc_dsp_headers _lc_ap_headers)
    file(STRINGS ${f} _incs REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"/]+[>\"]")
    foreach(line IN LISTS _incs)
        string(REGEX REPLACE ".*[<\"]([^>\"]+)[>\"].*" "\\1" n "${line}")
        list(APPEND _lc_names ${n})
    endforeach()
endforeach()
list(REMOVE_DUPLICATES _lc_names)
foreach(n IN LISTS _lc_names)
    string(TOLOWER ${n} lc)
    if(DEFINED _lc_target_${lc})
        file(CREATE_LINK ${_lc_target_${lc}} ${DSP_LC_HEADERS}/${n} SYMBOLIC)
    endif()
endforeach()
target_include_directories(DfxDsp PRIVATE ${DSP_LC_HEADERS})

# Project defines from dsp/DfxDsp.vcxproj (minus WIN32). DSPSOFT_TARGET selects
# the software DSP path (no hardware dma32.h); PT_DSP_BUILD picks the DFX build.
# DSP self-test: exercises processAudio with no I/O.
add_executable(dsp_selftest ${DSP_DIR}/linux/test_dsp.cpp)
target_link_libraries(dsp_selftest PRIVATE DfxDsp)
add_test(NAME dsp_selftest COMMAND dsp_selftest)

# reg_linux self-test: built standalone with libstdc++ bounds assertions so an
# out-of-range read aborts instead of passing silently.
add_executable(reg_selftest ${DSP_DIR}/linux/test_reg.cpp ${DSP_DIR}/linux/reg_linux.cpp)
target_include_directories(reg_selftest PRIVATE
    ${DSP_DIR}/ptutil/include ${DSP_DIR}/linux ${DSP_LC_HEADERS})
target_compile_definitions(reg_selftest PRIVATE PT_LINUX=1 PT_NON_MFC _GLIBCXX_ASSERTIONS)
target_compile_options(reg_selftest PRIVATE -fpermissive -Wno-write-strings)
foreach(scenario roundtrip persisted malformed rmdir)
    add_test(NAME reg_${scenario} COMMAND reg_selftest ${scenario})
    set_tests_properties(reg_${scenario} PROPERTIES LABELS unit
        ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config/reg_${scenario}")
endforeach()
set_tests_properties(dsp_selftest PROPERTIES LABELS unit
    ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config/dsp")
# Effect/power settings keep separate keys and do not interfere.
add_executable(dsp_settings_selftest ${DSP_DIR}/linux/test_dsp_settings.cpp)
target_include_directories(dsp_settings_selftest PRIVATE ${DSP_DIR}/../audiopassthru/src/linux)
target_link_libraries(dsp_settings_selftest PRIVATE DfxDsp)
add_test(NAME dsp_settings_selftest COMMAND dsp_settings_selftest)
set_tests_properties(dsp_settings_selftest PROPERTIES LABELS unit
    ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config/dsp_settings")
# Same run with glibc filling fresh allocations with a non-zero byte, so members
# that are read before being initialized fail deterministically.
add_test(NAME dsp_selftest_perturb COMMAND dsp_selftest)
set_tests_properties(dsp_selftest_perturb PROPERTIES LABELS unit
    ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config/dsp_perturb;MALLOC_PERTURB_=165")

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
