param(
    [Parameter(Mandatory=$true)][string]$BuildRoot,
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [switch]$AddressSanitizer
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$root = [IO.Path]::GetPathRoot($BuildRoot)
if (![IO.Path]::IsPathRooted($BuildRoot) -or !$root -or $root -in @('\','/') -or $root.EndsWith(':')) {
    throw 'BuildRoot must be an absolute Windows path.'
}
$output = [IO.Path]::GetFullPath($BuildRoot)
$prefix = $repo.TrimEnd('\','/')
if ($output.TrimEnd('\','/').Equals($prefix, [StringComparison]::OrdinalIgnoreCase) -or
    $output.StartsWith($prefix + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildRoot must be outside the source checkout.'
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or !(Get-Command cl.exe -ErrorAction SilentlyContinue) -or
    !(Get-Command lib.exe -ErrorAction SilentlyContinue)) { throw 'Use an x64 Visual Studio Developer PowerShell.' }
$variant = if ($AddressSanitizer) { 'asan' } else { 'native' }
$dir = Join-Path $output "VisualizationTests/$variant/x64/$Configuration"
[void][IO.Directory]::CreateDirectory($dir)
$flags = @('/nologo','/std:c++17','/EHsc','/W3','/DNOMINMAX')
$flags += if ($Configuration -eq 'Debug') { @('/Od','/Zi','/MTd','/D_DEBUG') } else { @('/O2','/MT','/DNDEBUG') }
[string[]]$fixtureLinkFlags = @()
if ($AddressSanitizer) {
    $flags += '/fsanitize=address'
    if ($Configuration -eq 'Release') { $flags += '/Zi' }
    $fixtureLinkFlags = @('/link','/DEBUG','/INCREMENTAL:NO')
}
$core = @('analysis/AudioAnalyzer.cpp','analysis/runtime/AnalysisStream.cpp','analysis/WaterfallRaster.cpp',
    'analysis/experimental/pitch/PitchMotion.cpp','analysis/experimental/pitch/SpectralCorrelation.cpp',
    'analysis/experimental/pitch/HarmonicCombProfile.cpp')
$extra = @('analysis/tests/VisualizationTests.cpp','analysis/tests/AnalysisRequestTests.cpp','analysis/tests/WaterfallTests.cpp',
    'analysis/experimental/pitch/PitchMotionTests.cpp','analysis/experimental/pitch/SpectralCorrelationTests.cpp',
    'analysis/experimental/pitch/HarmonicCombTests.cpp')
Push-Location -LiteralPath $dir
try {
    & cl.exe @flags /c @($core | ForEach-Object {Join-Path $repo $_}) *> 'core-build.log'
    if ($LASTEXITCODE -ne 0) { throw "Core compilation failed; see $dir/core-build.log" }
    $objects = @($core | ForEach-Object {[IO.Path]::GetFileNameWithoutExtension($_) + '.obj'})
    & lib.exe /nologo /out:AnalysisCore.lib @objects *> 'core-library.log'
    if ($LASTEXITCODE -ne 0) { throw "Core archive failed; see $dir/core-library.log" }
    $summaries = @()
    foreach ($name in @('AudioAnalysisTests','VisualizationTests')) {
        [string[]]$sources = if ($name -eq 'AudioAnalysisTests') { @(Join-Path $PSScriptRoot 'AudioAnalysisTests.cpp') }
            else { @($extra | ForEach-Object {Join-Path $repo $_}) }
        & cl.exe @flags @sources AnalysisCore.lib "/Fe:$name.exe" @fixtureLinkFlags *> "$name-build.log"
        if ($LASTEXITCODE -ne 0) { throw "Fixture compilation failed; see $dir/$name-build.log" }
        $exe = Join-Path $dir "$name.exe"
        $process = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru `
            -RedirectStandardOutput (Join-Path $dir "$name-results.json") `
            -RedirectStandardError (Join-Path $dir "$name-stderr.log")
        $timedOut = !$process.WaitForExit(180000)
        if ($timedOut) { $process.Kill(); $process.WaitForExit() }
        $result = [ordered]@{ Name=$name; ExitCode=$process.ExitCode; TimedOut=$timedOut; Executable=$exe }
        $result | ConvertTo-Json | Set-Content -LiteralPath "$name-exit.json" -Encoding utf8
        if ($timedOut -or $process.ExitCode -ne 0) { throw "Fixture failed; see $dir/$name-results.json and stderr.log" }
        $payload = Get-Content -LiteralPath "$name-results.json" -Raw | ConvertFrom-Json
        if (!$payload.valid -or @($payload.tests | Where-Object {!$_.passed}).Count -ne 0) { throw "Invalid fixture JSON: $name" }
        $result.Checks = @($payload.tests).Count
        $summaries += $result
    }
    $summaries | ConvertTo-Json | Set-Content -LiteralPath 'summary.json' -Encoding utf8
    Write-Output "Visualization tests $Configuration/$variant PASS; $dir/summary.json"
} finally { Pop-Location }
