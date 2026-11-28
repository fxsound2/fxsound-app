param(
    [Parameter(Mandatory=$true)][string]$BuildRoot,
    [Parameter(Mandatory=$true)][string]$DspLibrary,
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [switch]$ExpectClipped
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
if (!(Test-AbsoluteWindowsPath $DspLibrary) -or !(Test-Path -LiteralPath $DspLibrary -PathType Leaf)) {
    throw 'DspLibrary must identify an existing absolute library path.'
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'Use an x64 Visual Studio Developer PowerShell.' }
$compiler = Get-Command cl.exe -ErrorAction Stop
$flags = if ($Configuration -eq 'Debug') { @('/Od','/Zi','/MTd','/D_DEBUG') } else { @('/O2','/MT','/DNDEBUG') }
$mode = if ($ExpectClipped) { 'BaselineClipped' } else { 'Headroom' }
$dir = Join-Path $output "FloatHeadroomTests/x64/$Configuration/$mode"
[void][IO.Directory]::CreateDirectory($dir)
$exe = Join-Path $dir 'FloatHeadroomTests.exe'
$source = Join-Path $PSScriptRoot 'FloatHeadroomTests.cpp'
[ordered]@{
    configuration=$Configuration; expectedOutput=$mode; compiler=$compiler.Source;
    vcToolsVersion=$env:VCToolsVersion; library=$DspLibrary;
    librarySha256=(Get-FileHash -LiteralPath $DspLibrary -Algorithm SHA256).Hash;
    testSourceSha256=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dir 'inputs.json') -Encoding UTF8
Push-Location -LiteralPath $dir
try {
    & $compiler.Source /nologo /std:c++17 /EHsc /W3 @flags /DWIN32 /DUNICODE /D_UNICODE /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS `
        "/I$repo/dsp/ptutil/include" $source $DspLibrary "/Fe:$exe" /link /LTCG `
        advapi32.lib user32.lib ole32.lib oleaut32.lib shell32.lib shlwapi.lib winmm.lib uuid.lib `
        *> (Join-Path $dir 'build.log')
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed; see $dir/build.log" }
    $arguments = @('--capture', ('"' + (Join-Path $dir 'unchanged-paths.f32') + '"'))
    if ($ExpectClipped) { $arguments += '--expect-clipped' }
    # All calls are numerical with valid API arguments and null diagnostic sinks.
    # No DfxDsp/GUI/profile host or audio endpoint is constructed.
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $dir 'results.log') -RedirectStandardError (Join-Path $dir 'stderr.log')
    if (!$process.WaitForExit(30000)) {
        $process.Kill()
        $process.WaitForExit()
        throw 'FloatHeadroomTests timed out.'
    }
    if ($process.ExitCode -ne 0) { throw "Tests failed; see $dir/results.log" }
    Write-Output "FloatHeadroomTests ($Configuration, $mode): PASS; $dir/results.log"
} finally { Pop-Location }
