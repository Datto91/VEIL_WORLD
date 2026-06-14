#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Builds 0 A.D. (pyrogenesis.exe) from source using MSBuild command line.

.DESCRIPTION
    This script locates Visual Studio via vswhere.exe, then builds the
    pyrogenesis project in Release|Win32 configuration.
    
    Prerequisites:
    - Visual Studio 2022 (or 2019) with "Desktop development with C++" workload
    - All dependencies already present in libraries/ (verified by task 1.2)

.PARAMETER Configuration
    Build configuration: Release (default) or Debug

.PARAMETER BuildAll
    If specified, builds the entire solution instead of just pyrogenesis

.PARAMETER Upgrade
    If specified, upgrades the platform toolset to the latest available

.EXAMPLE
    .\build_0ad.ps1
    .\build_0ad.ps1 -Configuration Debug
    .\build_0ad.ps1 -BuildAll
#>

param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    
    [switch]$BuildAll,
    
    [switch]$Upgrade
)

$ErrorActionPreference = "Stop"

# Project root (relative to this script's location)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$SlnDir = Join-Path $ProjectRoot "build\workspaces\vs2017"
$SlnPath = Join-Path $SlnDir "pyrogenesis.sln"
$ProjPath = Join-Path $SlnDir "pyrogenesis.vcxproj"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " 0 A.D. Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Project Root: $ProjectRoot"
Write-Host "Configuration: $Configuration"
Write-Host "Platform: Win32"
Write-Host ""

# --- Step 1: Find Visual Studio ---
Write-Host "[1/4] Locating Visual Studio..." -ForegroundColor Yellow

$vsWherePaths = @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
)

$vsWhere = $null
foreach ($p in $vsWherePaths) {
    if (Test-Path $p) {
        $vsWhere = $p
        break
    }
}

if (-not $vsWhere) {
    Write-Host "ERROR: vswhere.exe not found. Visual Studio is not installed." -ForegroundColor Red
    Write-Host ""
    Write-Host "Install Visual Studio 2022 Community (free) from:" -ForegroundColor Yellow
    Write-Host "  https://visualstudio.microsoft.com/vs/community/" -ForegroundColor White
    Write-Host ""
    Write-Host "Required workload: 'Desktop development with C++'" -ForegroundColor Yellow
    exit 1
}

$vsPath = & $vsWhere -latest -property installationPath
if (-not $vsPath -or -not (Test-Path $vsPath)) {
    Write-Host "ERROR: Visual Studio installation not found." -ForegroundColor Red
    exit 1
}

Write-Host "  Found: $vsPath" -ForegroundColor Green

# --- Step 2: Find MSBuild ---
Write-Host "[2/4] Locating MSBuild..." -ForegroundColor Yellow

$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    # Try older path
    $msbuild = Join-Path $vsPath "MSBuild\15.0\Bin\MSBuild.exe"
}

if (-not (Test-Path $msbuild)) {
    Write-Host "ERROR: MSBuild.exe not found at expected location." -ForegroundColor Red
    Write-Host "  Checked: $(Join-Path $vsPath 'MSBuild\Current\Bin\MSBuild.exe')" -ForegroundColor Gray
    exit 1
}

Write-Host "  Found: $msbuild" -ForegroundColor Green

# --- Step 3: Verify solution file ---
Write-Host "[3/4] Verifying solution structure..." -ForegroundColor Yellow

if (-not (Test-Path $SlnPath)) {
    Write-Host "ERROR: Solution file not found at: $SlnPath" -ForegroundColor Red
    Write-Host "  Run update-workspaces.bat to regenerate the solution." -ForegroundColor Yellow
    exit 1
}

Write-Host "  Solution: $SlnPath" -ForegroundColor Green

# Check for pyrogenesis project
if (-not (Test-Path $ProjPath)) {
    Write-Host "ERROR: pyrogenesis.vcxproj not found." -ForegroundColor Red
    exit 1
}

Write-Host "  Project: $ProjPath" -ForegroundColor Green

# --- Step 4: Build ---
Write-Host "[4/4] Building..." -ForegroundColor Yellow
Write-Host ""

$buildTarget = if ($BuildAll) { $SlnPath } else { $ProjPath }
$targetDesc = if ($BuildAll) { "entire solution" } else { "pyrogenesis.vcxproj" }

Write-Host "Building: $targetDesc"
Write-Host "Command: MSBuild $targetDesc /p:Configuration=$Configuration /p:Platform=Win32 /m"
Write-Host ""

$buildArgs = @(
    $buildTarget,
    "/p:Configuration=$Configuration",
    "/p:Platform=Win32",
    "/m",  # Multi-processor build
    "/nologo",
    "/v:minimal"
)

if ($Upgrade) {
    # Allow VS to upgrade the platform toolset
    $buildArgs += "/p:PlatformToolset=v143"
}

$startTime = Get-Date
& $msbuild @buildArgs
$exitCode = $LASTEXITCODE
$elapsed = (Get-Date) - $startTime

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan

if ($exitCode -eq 0) {
    Write-Host " BUILD SUCCEEDED" -ForegroundColor Green
    Write-Host " Time: $($elapsed.ToString('mm\:ss'))" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    
    # Verify output
    $exeName = if ($Configuration -eq "Release") { "pyrogenesis.exe" } else { "pyrogenesis_dbg.exe" }
    $exePath = Join-Path $ProjectRoot "binaries\system\$exeName"
    
    if (Test-Path $exePath) {
        $exeInfo = Get-Item $exePath
        Write-Host ""
        Write-Host "Output: $exePath" -ForegroundColor Green
        Write-Host "Size: $([math]::Round($exeInfo.Length / 1MB, 2)) MB" -ForegroundColor Green
        Write-Host "Built: $($exeInfo.LastWriteTime)" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "WARNING: Expected output not found at: $exePath" -ForegroundColor Yellow
        Write-Host "Check the build output above for the actual output location." -ForegroundColor Yellow
    }
} else {
    Write-Host " BUILD FAILED (exit code: $exitCode)" -ForegroundColor Red
    Write-Host " Time: $($elapsed.ToString('mm\:ss'))" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Common issues:" -ForegroundColor Yellow
    Write-Host "  1. Platform toolset v141_xp not found:" -ForegroundColor White
    Write-Host "     Re-run with -Upgrade flag to use current toolset" -ForegroundColor Gray
    Write-Host "  2. Windows SDK not found:" -ForegroundColor White
    Write-Host "     Install Windows 10 SDK via VS Installer" -ForegroundColor Gray
    Write-Host "  3. Atlas build failure (wxwidgets missing):" -ForegroundColor White
    Write-Host "     Use default mode (builds only pyrogenesis, skips Atlas)" -ForegroundColor Gray
    Write-Host ""
    exit $exitCode
}
