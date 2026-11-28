param(
    [Parameter(Mandatory=$true)][string]$BuildRoot,
    [Parameter(Mandatory=$true)][string]$AudioLibrary,
    [Parameter(Mandatory=$true)][string]$DspLibrary,
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
function Test-AbsoluteWindowsPath([string]$Path) {
    $root = [IO.Path]::GetPathRoot($Path)
    # C:relative and \root-relative depend on the caller's current drive/directory.
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
        throw "Missing absolute library path: $library"
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'Use an x64 Visual Studio Developer PowerShell.' }
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'MSVC cl.exe was not found.' }
$flags = if ($Configuration -eq 'Debug') { @('/Od','/Zi','/MTd') } else { @('/O2','/MT') }
foreach ($name in @('AudioShutdownTests','DeviceChangeTests')) {
    $dir = Join-Path $output "AudioLifecycleTests/x64/$Configuration/$name"
    [void][IO.Directory]::CreateDirectory($dir)
    $exe = Join-Path $dir "$name.exe"
    $libraries = @('ole32.lib','uuid.lib')
    $inputs = @((Join-Path $PSScriptRoot "$name.cpp"))
    if ($name -eq 'DeviceChangeTests') {
        $inputs += @($AudioLibrary,$DspLibrary)
        $libraries += @('oleaut32.lib','advapi32.lib','user32.lib','shell32.lib','winmm.lib','shlwapi.lib','comsuppw.lib')
    }
    Push-Location -LiteralPath $dir
    try {
        & cl.exe /nologo /std:c++17 /EHsc /W3 @flags /DUNICODE /D_UNICODE /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS `
            "/I$repo/audiopassthru/include" "/I$repo/dsp/ptutil/include" "/I$repo/dsp/include" `
            @inputs "/Fe:$exe" /link /LTCG @libraries *> (Join-Path $dir 'build.log')
        if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $name; see $dir/build.log" }
        $process = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru `
            -RedirectStandardOutput (Join-Path $dir 'results.log') -RedirectStandardError (Join-Path $dir 'stderr.log')
        if (!$process.WaitForExit(30000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "Test fixture timed out: $name"
        }
        if ($process.ExitCode -ne 0) { throw "Tests failed: $name; see $dir/results.log" }
        Write-Output "$name ($Configuration): PASS; $dir/results.log"
    } finally { Pop-Location }
}
