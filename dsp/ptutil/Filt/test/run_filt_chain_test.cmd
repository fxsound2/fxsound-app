@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
	echo Visual Studio not found.
	exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set "OUT=%TEMP%\filt_chain_test"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /EHsc /W3 /O2 /fp:precise /I "%~dp0..\..\include" "%~dp0FiltChainTest.cpp" "%~dp0..\FiltChain.cpp" "%~dp0..\FiltLowCut.cpp" /Fo"%OUT%\\" /Fe"%OUT%\FiltChainTest.exe"
if errorlevel 1 exit /b 1
"%OUT%\FiltChainTest.exe"
