<#
.SYNOPSIS
    Packages fxmcp's Claude Desktop Extension (.mcpb).

.DESCRIPTION
    This is a "thin connector" bundle: fxmcp.exe itself is installed by the
    main FxSound installer at a fixed location
    (%ProgramFiles%\FxSound LLC\FxSound\fxmcp.exe), so this .mcpb does NOT
    carry its own copy of the binary -- it just tells Claude Desktop how to
    launch the one already installed there. This also sidesteps needing a
    separate .mcpb per architecture (x64/x86/arm64): the FxSound installer
    already resolved that at install time and always places the binary at
    this same path, so one manifest covers all three.

    Caveat: because the path is a fixed literal (MCPB has no built-in
    variable for Program Files -- see mcpb/manifest.json), this only works
    for a default-location FxSound install. A custom install directory
    chosen during FxSound setup isn't accounted for here.

    Requires the mcpb CLI: npm install -g @anthropic-ai/mcpb

.PARAMETER OutFile
    Where to write the packed .mcpb file. Defaults to dist\fxmcp.mcpb.

.PARAMETER SkipBinCopy
    Skip copying the built .mcpb to fxsound-app\bin\ (see below).

.EXAMPLE
    .\build-mcpb.ps1
#>
param(
    [string]$OutFile = "$PSScriptRoot\dist\fxmcp.mcpb",
    [switch]$SkipBinCopy
)

$ErrorActionPreference = "Stop"

$mcpbDir = Join-Path $PSScriptRoot "mcpb"
$repoRoot = Split-Path $PSScriptRoot -Parent  # fxsound-app\

function Resolve-MCPBExe {
    $cmd = Get-Command mcpb -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "mcpb CLI not found on PATH. Install it with: npm install -g @anthropic-ai/mcpb"
}

$mcpbExe = Resolve-MCPBExe
Write-Host "Using mcpb: $mcpbExe ($(& $mcpbExe --version))"

# Refresh the bundled icon/license from their canonical source locations
# each build, rather than letting a checked-in copy drift out of sync.
$iconSrc = Join-Path $repoRoot "fxsound\Images\fxsound_large.png"
$licenseSrc = Join-Path $repoRoot "LICENSE"

if (-not (Test-Path $iconSrc)) { throw "icon source not found: $iconSrc" }
if (-not (Test-Path $licenseSrc)) { throw "LICENSE not found: $licenseSrc" }

Copy-Item $iconSrc (Join-Path $mcpbDir "icon.png") -Force
Copy-Item $licenseSrc (Join-Path $mcpbDir "LICENSE") -Force

Write-Host "Validating manifest..."
& $mcpbExe validate (Join-Path $mcpbDir "manifest.json")
if ($LASTEXITCODE -ne 0) { throw "manifest validation failed" }

$outDir = Split-Path $OutFile -Parent
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
if (Test-Path $OutFile) { Remove-Item $OutFile -Force }

Write-Host "Packing..."
& $mcpbExe pack $mcpbDir $OutFile
if ($LASTEXITCODE -ne 0) { throw "mcpb pack failed with exit code $LASTEXITCODE" }

Write-Host ""
Write-Host "Built: $OutFile"
Get-Item $OutFile | Format-Table Name, Length, LastWriteTime -AutoSize

if (-not $SkipBinCopy) {
    # The .mcpb is architecture-agnostic (MCPB has no concept of CPU
    # architecture, only OS -- see mcpb/README.md), so this is a single
    # copy at the top level of bin\, not duplicated into bin\x64\,
    # bin\x86\, bin\arm64\ the way fxmcp.exe is by build.ps1.
    $binRoot = Join-Path $repoRoot "bin"
    New-Item -ItemType Directory -Force -Path $binRoot | Out-Null
    $binCopy = Join-Path $binRoot "fxmcp.mcpb"
    Copy-Item $OutFile $binCopy -Force
    Write-Host "Copied to: $binCopy"
}

Write-Host "Note: this bundle is unsigned (mcpb sign) -- pending the DigiCert code signing certificate."
