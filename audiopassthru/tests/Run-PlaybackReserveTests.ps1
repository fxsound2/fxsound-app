param(
    [Parameter(Mandatory=$true)][string]$BuildRoot,
    [Parameter(Mandatory=$true)][string]$AudioLibrary,
    [Parameter(Mandatory=$true)][string]$DspLibrary,
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
function Test-AbsoluteWindowsPath([string]$Path) {
    $root = [IO.Path]::GetPathRoot($Path)
    return $root -and $root -ne '\' -and !$root.EndsWith(':')
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (!(Test-AbsoluteWindowsPath $BuildRoot)) { throw 'BuildRoot must be absolute.' }
$output = [IO.Path]::GetFullPath($BuildRoot)
if ($output.TrimEnd('\','/').Equals($repo.TrimEnd('\','/'), [StringComparison]::OrdinalIgnoreCase) -or
    $output.StartsWith($repo.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildRoot must be outside the source checkout.'
}
foreach ($library in @($AudioLibrary, $DspLibrary)) {
    if (!(Test-AbsoluteWindowsPath $library) -or !(Test-Path -LiteralPath $library -PathType Leaf)) {
        throw "Pass an absolute path to a matching library: $library"
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'Use an x64 Visual Studio Developer PowerShell.' }
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'MSVC cl.exe was not found.' }
$flags = if ($Configuration -eq 'Debug') { @('/Od','/Zi','/MTd','/D_DEBUG') } else { @('/O2','/MT','/DNDEBUG') }
$dir = Join-Path $output "PlaybackReserveTests/x64/$Configuration"
[void][IO.Directory]::CreateDirectory($dir)
$exe = Join-Path $dir 'PlaybackReserveTests.exe'
Push-Location -LiteralPath $dir
try {
    & cl.exe /nologo /std:c++17 /EHsc /W3 @flags /DUNICODE /D_UNICODE /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS `
        "/I$repo/audiopassthru/include" "/I$repo/dsp/include" "/I$repo/dsp/ptutil/include" (Join-Path $PSScriptRoot 'PlaybackReserveTests.cpp') `
        $AudioLibrary $DspLibrary "/Fe:$exe" /link /LTCG ole32.lib uuid.lib oleaut32.lib advapi32.lib user32.lib shell32.lib winmm.lib shlwapi.lib comsuppw.lib `
        *> (Join-Path $dir 'build.log')
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed; see $dir/build.log" }
    $process = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $dir 'results.log') -RedirectStandardError (Join-Path $dir 'stderr.log')
    $timedOut = !$process.WaitForExit(30000)
    if ($timedOut) {
        $process.Kill()
        $process.WaitForExit()
    }
    $nativeExit = [int]$process.ExitCode
    $unsignedExit = [BitConverter]::ToUInt32([BitConverter]::GetBytes($nativeExit), 0)
    $exitHex = '0x{0:X8}' -f $unsignedExit
    $exitRecord = [pscustomobject]@{
        Configuration = $Configuration
        ExitCode = $nativeExit
        ExitCodeHex = $exitHex
        TimedOut = $timedOut
        Finished = (Get-Date -Format o)
    }
    [IO.File]::WriteAllText((Join-Path $dir 'exit.json'), ($exitRecord | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
    Write-Output "PlaybackReserveTests ($Configuration): native exit $nativeExit ($exitHex)"
    if ($timedOut) { throw "PlaybackReserveTests timed out; see $dir/exit.json" }
    if ($nativeExit -ne 0) { throw "Tests failed with $exitHex; see $dir/results.log and exit.json" }
    Write-Output "PlaybackReserveTests ($Configuration): PASS; $dir/results.log"
} finally { Pop-Location }
