<#
.SYNOPSIS
    Builds fxmcp.exe for x64, x86, and arm64, copying each into
    fxsound-app\bin\<arch>\ alongside FxSound.exe/fxdiag.exe's own build
    output (matching fxdiag.vcxproj's post-build "copy $(TargetPath)
    ..\bin\$(PlatformTarget)\" convention).

.PARAMETER Configuration
    "Release" (default, stripped via -ldflags="-s -w") or "Debug" (includes
    debug symbols, no stripping).

.PARAMETER Platforms
    Which architectures to build. Defaults to all three: x64, x86, arm64.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Configuration Debug
    .\build.ps1 -Platforms x64,arm64
#>
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "x86", "arm64")]
    [string[]]$Platforms = @("x64", "x86", "arm64")
)

$ErrorActionPreference = "Stop"

# GOARCH for each platform name, matching the same "x64"/"x86"/"arm64"
# folder names fxdiag.vcxproj's post-build step already copies into
# ..\bin\$(PlatformTarget)\.
$goarchByPlatform = @{
    "x64"   = "amd64"
    "x86"   = "386"
    "arm64" = "arm64"
}

function Resolve-GoExe {
    $cmd = Get-Command go -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "C:\Program Files\Go\bin\go.exe",
        "$env:LOCALAPPDATA\Programs\Go\bin\go.exe"
    )
    foreach ($p in $candidates) {
        if (Test-Path $p) { return $p }
    }

    throw "go.exe not found on PATH or in common install locations. Install Go or add it to PATH."
}

$goExe = Resolve-GoExe
$fxmcpRoot = $PSScriptRoot
$binRoot = Join-Path (Split-Path $fxmcpRoot -Parent) "bin"

Write-Host "Using go: $goExe"
Write-Host "Output root: $binRoot"
Write-Host ""

# Refresh the embedded docs/COMMAND_LINE_OPTIONS.md copy before building,
# so a release build never ships stale embedded docs -- see
# internal/mcpserver/docsdata/embed.go.
Write-Host "Running go generate..."
Push-Location $fxmcpRoot
try {
    & $goExe generate ./...
    if ($LASTEXITCODE -ne 0) { throw "go generate failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

$ldflags = if ($Configuration -eq "Release") { "-s -w" } else { "" }

$results = @()
foreach ($platform in $Platforms) {
    $goarch = $goarchByPlatform[$platform]
    $outDir = Join-Path $binRoot $platform
    $outFile = Join-Path $outDir "fxmcp.exe"

    Write-Host "Building $platform (GOARCH=$goarch, $Configuration) -> $outFile"

    New-Item -ItemType Directory -Force -Path $outDir | Out-Null

    Push-Location $fxmcpRoot
    try {
        $env:GOOS = "windows"
        $env:GOARCH = $goarch
        $env:CGO_ENABLED = "0"

        if ($ldflags) {
            & $goExe build -ldflags="$ldflags" -o $outFile ./cmd/fxmcp
        }
        else {
            & $goExe build -o $outFile ./cmd/fxmcp
        }
        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
        Remove-Item Env:\GOOS -ErrorAction SilentlyContinue
        Remove-Item Env:\GOARCH -ErrorAction SilentlyContinue
        Remove-Item Env:\CGO_ENABLED -ErrorAction SilentlyContinue
    }

    if ($exitCode -ne 0) {
        throw "build failed for $platform (GOARCH=$goarch), exit code $exitCode"
    }

    $size = (Get-Item $outFile).Length
    $results += [pscustomobject]@{
        Platform = $platform
        GOARCH   = $goarch
        Output   = $outFile
        SizeKB   = [math]::Round($size / 1KB, 0)
    }
}

Write-Host ""
Write-Host "Build complete:"
$results | Format-Table -AutoSize
