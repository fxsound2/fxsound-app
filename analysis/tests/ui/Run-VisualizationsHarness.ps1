param(
    [Parameter(Mandatory=$true)][string]$SourceRoot,
    [Parameter(Mandatory=$true)][string]$JuceModuleRoot,
    [Parameter(Mandatory=$true)][string]$AppConfigHeader,
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9A-Fa-f]{64}$')][string]$ExpectedAppConfigSha256,
    [Parameter(Mandatory=$true)][string]$ModuleObjectDirectory,
    [Parameter(Mandatory=$true)][string]$ModuleCompileCommandLog,
    [Parameter(Mandatory=$true)][string]$ExpectedVCToolsVersion,
    [Parameter(Mandatory=$true)][string]$BuildRoot,
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9-]+$')][string]$RunTag,
    [ValidateSet('JUCE9-v145','JUCE6-v143')][string]$Variant = 'JUCE9-v145',
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [switch]$CompileOnly
)
$ErrorActionPreference = 'Stop'

function Get-AbsoluteInput([string]$Path, [string]$Kind) {
    $root = [IO.Path]::GetPathRoot($Path)
    if (!$root -or $root -eq '\' -or $root.EndsWith(':')) { throw "Absolute path required: $Path" }
    $resolved = [IO.Path]::GetFullPath($Path)
    if (!(Test-Path -LiteralPath $resolved -PathType $Kind)) { throw "Input missing: $resolved" }
    return $resolved
}
function Quote-LinkResponseToken([string]$Value) {
    # Paths are individual response-file tokens, never interpreted by a shell.
    $escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}
function Write-JsonFile([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
}

$source = Get-AbsoluteInput $SourceRoot 'Container'
$modules = Get-AbsoluteInput $JuceModuleRoot 'Container'
$config = Get-AbsoluteInput $AppConfigHeader 'Leaf'
$objectDirectory = Get-AbsoluteInput $ModuleObjectDirectory 'Container'
$commandLog = Get-AbsoluteInput $ModuleCompileCommandLog 'Leaf'
$buildPathRoot = [IO.Path]::GetPathRoot($BuildRoot)
if (!$buildPathRoot -or $buildPathRoot -eq '\' -or $buildPathRoot.EndsWith(':')) { throw 'BuildRoot must be absolute.' }
$outputRoot = [IO.Path]::GetFullPath($BuildRoot)
if ($outputRoot.TrimEnd('\','/').Equals($source.TrimEnd('\','/'), [StringComparison]::OrdinalIgnoreCase) -or
    $outputRoot.StartsWith($source.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildRoot must be outside SourceRoot.'
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VCToolsVersion -ne $ExpectedVCToolsVersion) {
    throw 'Use the x64 Developer PowerShell and VCToolsVersion of the original module build.'
}
if ((Get-FileHash -LiteralPath $config -Algorithm SHA256).Hash -ne $ExpectedAppConfigSha256) {
    throw 'AppConfig differs from the configuration recorded for the reusable module objects.'
}
$compiler = (Get-Command cl.exe -ErrorAction Stop).Source
$linker = (Get-Command link.exe -ErrorAction Stop).Source
$modern = $Variant -eq 'JUCE9-v145'
$versionHeader = Get-AbsoluteInput (Join-Path $modules 'juce_core/system/juce_StandardHeader.h') 'Leaf'
$versionText = Get-Content -LiteralPath $versionHeader -Raw
$requiredJuceMajor = if ($modern) { 9 } else { 6 }
if ($versionText -notmatch ('(?m)^#define\s+JUCE_MAJOR_VERSION\s+' + $requiredJuceMajor + '\s*$')) {
    throw 'JUCE headers do not match the reusable object variant.'
}
if (($modern -and $ExpectedVCToolsVersion -notmatch '^14\.5') -or
    (!$modern -and $ExpectedVCToolsVersion -notmatch '^14\.[34]')) { throw 'Toolset does not match Variant.' }
$standard = if ($modern) { '/std:c++latest' } else { '/std:c++17' }
$crt = if ($Configuration -eq 'Debug') { '/MTd' } else { '/MT' }
$optimizations = if ($Configuration -eq 'Debug') { @('/Od','/Zi') } else { @('/O2') }

# Reuse the actual app module definitions, not an independently guessed set.
$commands = Get-Content -LiteralPath $commandLog
$moduleCommand = $null
for ($i = 0; $i -lt $commands.Count - 1; ++$i) {
    if ($commands[$i] -match '(?i)^\^.*include_juce_core\.cpp(?:\||$)') {
        $moduleCommand = $commands[$i + 1]
        break
    }
}
if (!$moduleCommand) { throw 'No include_juce_core.cpp command in the supplied MSBuild tlog.' }
if ($moduleCommand -notmatch ('(?i)(?:^|\s)' + [regex]::Escape($crt) + '(?:\s|$)')) {
    throw 'The module command does not use the requested CRT.'
}
$definitions = @([regex]::Matches($moduleCommand, '(?i)(?:^|\s)/D\s*("[^"]*"|\S+)') | ForEach-Object {
    '/D' + $_.Groups[1].Value.Trim('"')
} | Where-Object { $_ -notmatch '(?i)^/Diagnostics:' -and $_ -notmatch '(?i)^/DJUCE_LOG_ASSERTIONS(?:=|$)' })
if (!$definitions.Count) { throw 'No module preprocessor definitions found; refusing an ambiguous reuse.' }

$moduleNames = if ($modern) {
    @('core_CompilationTime','core_zlib','data_structures','events',
      'graphics','graphics_Harfbuzz','graphics_Sheenbidi','graphics_libjpg_1','graphics_libjpg_2','graphics_libjpg_3',
      'graphics_libpng','graphics_libwebp_dec','graphics_libwebp_enc','graphics_libwebp_utils','graphics_libwebp_sharpyuv',
      'graphics_libwebp_dsp_dec','graphics_libwebp_dsp_dec_avx2','graphics_libwebp_dsp_dec_sse41',
      'graphics_libwebp_dsp_enc','graphics_libwebp_dsp_enc_avx2','graphics_libwebp_dsp_enc_sse41','graphics_lunasvg',
      'gui_basics','gui_basics_2','gui_basics_3','gui_basics_4','gui_basics_5','gui_extra')
} else { @('data_structures','events','graphics','gui_basics','gui_extra') }
# JUCE's default Debug logAssertion uses DBG/OutputDebugString, bypassing Logger.
# Rebuild this one genuine unit locally with logging enabled. All other cached
# module objects and the application's AppConfig remain untouched.
$referenceCoreObject = Get-AbsoluteInput (Join-Path $objectDirectory 'include_juce_core.obj') 'Leaf'
$diagnosticCoreSource = Get-AbsoluteInput (Join-Path $source 'fxsound/JuceLibraryCode/include_juce_core.cpp') 'Leaf'
$moduleObjects = @($moduleNames | ForEach-Object {
    Get-AbsoluteInput (Join-Path $objectDirectory "include_juce_$_.obj") 'Leaf'
})
$relativeSources = @('analysis/AudioAnalyzer.cpp','analysis/WaterfallRaster.cpp','analysis/ui/WaterfallWindow.cpp',
    'analysis/experimental/pitch/PitchMotion.cpp','analysis/experimental/pitch/SpectralCorrelation.cpp',
    'analysis/experimental/pitch/HarmonicCombProfile.cpp','analysis/experimental/pitch/ui/PitchMotionIndicator.cpp',
    'analysis/experimental/pitch/ui/SpectralCorrelationIndicator.cpp','analysis/experimental/pitch/ui/HarmonicCombIndicator.cpp',
    'analysis/experimental/pitch/ui/PitchExperimentWindow.cpp')
$compileSources = @($relativeSources | ForEach-Object { Get-AbsoluteInput (Join-Path $source $_) 'Leaf' })
$compileSources += Get-AbsoluteInput (Join-Path $PSScriptRoot 'VisualizationsHarness.cpp') 'Leaf'
$compileSources += $diagnosticCoreSource
if (($compileSources | ForEach-Object {[IO.Path]::GetFileNameWithoutExtension($_)} | Sort-Object -Unique).Count -ne $compileSources.Count) {
    throw 'Source basenames collide; give each compiled source its own object destination.'
}
$directory = Join-Path $outputRoot "VisualizationsHarness/$Variant/x64/$Configuration/$RunTag"
if (Test-Path -LiteralPath $directory) { throw 'RunTag output already exists; use a new tag to preserve its evidence.' }
[void][IO.Directory]::CreateDirectory($directory)
$intermediates = Join-Path $directory 'objects'
[void][IO.Directory]::CreateDirectory($intermediates)
$exports = Join-Path $directory 'png'
$executable = Join-Path $directory 'VisualizationsHarness.exe'
$compileArguments = @('/nologo','/c','/MP2','/EHsc','/W3',$standard,$crt) + $optimizations + $definitions +
    @('/DNOMINMAX','/D_CRT_SECURE_NO_WARNINGS','/DJUCE_LOG_ASSERTIONS=1',"/FI$config","/I$source","/I$modules",
      "/Fo$intermediates\","/Fd$(Join-Path $directory 'compiler.pdb')") + $compileSources
$objectInputs = @($moduleObjects | ForEach-Object {
    [ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash}
})
$inputRecord = [ordered]@{
    variant=$Variant;configuration=$Configuration;vcToolsVersion=$env:VCToolsVersion;
    compiler=$compiler;linker=$linker;appConfig=$config;appConfigSha256=$ExpectedAppConfigSha256;
    moduleCommandLog=$commandLog;moduleCommandLogSha256=(Get-FileHash -LiteralPath $commandLog -Algorithm SHA256).Hash;
    juceVersionHeader=$versionHeader;juceVersionHeaderSha256=(Get-FileHash -LiteralPath $versionHeader -Algorithm SHA256).Hash;
    compileArguments=$compileArguments;moduleObjects=$objectInputs;
    diagnosticCore=[ordered]@{ source=$diagnosticCoreSource;loggingAssertions=$true;
        referenceObject=$referenceCoreObject;referenceObjectSha256=(Get-FileHash -LiteralPath $referenceCoreObject -Algorithm SHA256).Hash };
    sources=@($compileSources | ForEach-Object {[ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash}});
    analysisHeaders=@(Get-ChildItem -LiteralPath (Join-Path $source 'analysis') -Recurse -File -Filter '*.h' | ForEach-Object {
        [ordered]@{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
}
Write-JsonFile (Join-Path $directory 'inputs.json') $inputRecord
Push-Location -LiteralPath $directory
try {
    & $compiler @compileArguments *> (Join-Path $directory 'compile.log')
    if ($LASTEXITCODE) { throw "Harness compilation failed; see $directory/compile.log" }
    $newObjects = @($compileSources | ForEach-Object {Join-Path $intermediates ([IO.Path]::GetFileNameWithoutExtension($_)+'.obj')})
    foreach ($object in $newObjects) { if (!(Test-Path -LiteralPath $object -PathType Leaf)) {throw "Missing object: $object"} }
    # The six JUCE module families autolink their platform libraries too. These
    # explicit SDK libraries make the Windows linkage inspectable in one place.
    $windowsLibraries = @('kernel32.lib','user32.lib','gdi32.lib','comdlg32.lib','comctl32.lib','imm32.lib',
        'shell32.lib','shlwapi.lib','advapi32.lib','ole32.lib','oleaut32.lib','uuid.lib','winmm.lib','wininet.lib',
        'ws2_32.lib','version.lib','dbghelp.lib','shcore.lib','dwmapi.lib','vfw32.lib','dwrite.lib','d2d1.lib',
        'dxgi.lib','d3d11.lib','dcomp.lib','dxguid.lib','opengl32.lib','glu32.lib')
    $linkArguments = @('/NOLOGO','/LTCG','/INCREMENTAL:NO','/SUBSYSTEM:CONSOLE',"/OUT:$executable",
        "/PDB:$(Join-Path $directory 'harness.pdb')") + $newObjects + $moduleObjects + $windowsLibraries
    if ($Configuration -eq 'Debug') { $linkArguments += '/DEBUG' }
    $response = Join-Path $directory 'link.rsp'
    [IO.File]::WriteAllLines($response, [string[]]@($linkArguments | ForEach-Object {Quote-LinkResponseToken $_}), [Text.UTF8Encoding]::new($false))
    & $linker "@$response" *> (Join-Path $directory 'link.log')
    if ($LASTEXITCODE) { throw "Harness link failed; see $directory/link.log" }
    if ($CompileOnly) { Write-Output "Compiled only: $executable"; return }
    $process = Start-Process -FilePath $executable -ArgumentList @('"'+$exports+'"') -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $directory 'results.log') -RedirectStandardError (Join-Path $directory 'stderr.log')
    $timedOut = !$process.WaitForExit(60000)
    if ($timedOut) { $process.Kill(); $process.WaitForExit() }
    $nativeExit = [int]$process.ExitCode
    $stderrPath = Join-Path $directory 'stderr.log'
    $unexpectedDiagnostics = @(Select-String -LiteralPath $stderrPath -Pattern 'UNEXPECTED_JUCE_ASSERT|UNEXPECTED_CRT_ASSERT_OR_ERROR')
    Write-JsonFile (Join-Path $directory 'exit.json') ([ordered]@{
        exitCode=$nativeExit;exitCodeHex=('0x{0:X8}' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($nativeExit),0));
        timedOut=$timedOut;unexpectedDiagnosticLines=$unexpectedDiagnostics.Count;finished=(Get-Date -Format o)
    })
    if ($timedOut -or $nativeExit -or $unexpectedDiagnostics.Count) { throw "Harness failed/timeout (exit $nativeExit, unexpected diagnostics $($unexpectedDiagnostics.Count)); see $directory/exit.json and logs" }
    Write-Output "PASS: $directory; PNG scales are offscreen exports, not native peer DPI coverage."
} finally { Pop-Location }
