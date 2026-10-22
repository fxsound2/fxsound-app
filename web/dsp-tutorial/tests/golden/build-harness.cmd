@echo off
rem Builds the golden-reference harness against the real FxSound engine (x64 Release).
rem Run from any directory. Requires Visual Studio 2022 (MSBuild + MSVC).
setlocal
set HERE=%~dp0
set REPO=%HERE%..\..\..\..
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
if not exist "%VS%" set VS=C:\Program Files\Microsoft Visual Studio\2022\Professional
if not exist "%VS%" set VS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise

call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
msbuild "%REPO%\dsp\DfxDsp.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo || exit /b 1
msbuild "%REPO%\audiopassthru\audiopassthru.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo || exit /b 1

if not exist "%HERE%work" mkdir "%HERE%work"
cl /nologo /EHsc /O2 /MT /I"%REPO%\dsp\include" "%HERE%harness.cpp" ^
   "%REPO%\lib\Release-x64\DfxDsp.lib" "%REPO%\lib\Release-x64\audiopassthru.lib" ^
   advapi32.lib user32.lib shlwapi.lib ole32.lib winmm.lib shell32.lib version.lib ^
   /Fo"%HERE%work\\" /Fe"%HERE%work\harness.exe" || exit /b 1
echo Built %HERE%work\harness.exe
