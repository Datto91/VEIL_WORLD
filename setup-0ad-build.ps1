#!/usr/bin/env pwsh
# ==============================================================================
# 0 A.D. Build Environment Setup Script
# ==============================================================================
# This script sets up the complete build environment for the 0 A.D. MCP Controller
# project on Windows. It handles:
#   1. Cloning the 0 A.D. source repository (Git mirror)
#   2. Downloading/verifying pre-built dependencies
#   3. Generating the Visual Studio solution via premake
#
# Prerequisites:
#   - Windows 10/11
#   - Git for Windows (already installed)
#   - Visual Studio 2022 (Community/Professional/Enterprise) with:
#       - "Desktop development with C++" workload
#       - Windows 10/11 SDK
#       - MSVC v143 build tools
#
# Usage:
#   .\setup-0ad-build.ps1 [-SkipClone] [-SkipDeps] [-SkipWorkspaces]
# ==============================================================================

param(
    [switch]$SkipClone,
    [switch]$SkipDeps,
    [switch]$SkipWorkspaces
)

$ErrorActionPreference = "Stop"
$WorkspaceRoot = $PSScriptRoot
$ZeroADDir = Join-Path $WorkspaceRoot "0ad"
$GitRepo = "https://github.com/0ad/0ad.git"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Cyan
    Write-Host "  $Message" -ForegroundColor Cyan
    Write-Host "================================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Info {
    param([string]$Message)
    Write-Host "  [INFO] $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "  [WARN] $Message" -ForegroundColor Yellow
}

function Write-Err {
    param([string]$Message)
    Write-Host "  [ERROR] $Message" -ForegroundColor Red
}

# ==============================================================================
# Step 0: Check Prerequisites
# ==============================================================================
Write-Step "Checking Prerequisites"

# Check Git
$gitVersion = git --version 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Err "Git is not installed. Please install Git for Windows."
    exit 1
}
Write-Info "Git: $gitVersion"

# Check Visual Studio
$vsWherePaths = @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
)
$vsInstalled = $false
foreach ($vswhere in $vsWherePaths) {
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -property installationPath 2>$null
        $vsVersion = & $vswhere -latest -property installationVersion 2>$null
        if ($vsPath) {
            Write-Info "Visual Studio: $vsVersion at $vsPath"
            $vsInstalled = $true
            break
        }
    }
}

if (-not $vsInstalled) {
    Write-Warn "Visual Studio not detected!"
    Write-Warn "Please install Visual Studio 2022 with these components:"
    Write-Warn "  - Desktop development with C++ workload"
    Write-Warn "  - Windows 10/11 SDK"
    Write-Warn "  - MSVC v143 build tools"
    Write-Warn ""
    Write-Warn "Download from: https://visualstudio.microsoft.com/downloads/"
    Write-Warn ""
    Write-Warn "You can install VS while the source is cloning (Step 1)."
    Write-Warn "The script will continue with source checkout."
}

# Check disk space (need ~20 GB for source + deps + build)
$drive = Get-PSDrive C
$freeGB = [math]::Round($drive.Free / 1GB, 2)
Write-Info "Free disk space: ${freeGB} GB"
if ($freeGB -lt 25) {
    Write-Warn "Less than 25 GB free. The build may require more space."
}

# ==============================================================================
# Step 1: Clone 0 A.D. Source Repository
# ==============================================================================
if (-not $SkipClone) {
    Write-Step "Cloning 0 A.D. Source Repository"

    if (Test-Path (Join-Path $ZeroADDir ".git")) {
        Write-Info "Repository already cloned at: $ZeroADDir"
        Write-Info "Pulling latest changes..."
        Push-Location $ZeroADDir
        git -P pull --ff-only 2>&1
        Pop-Location
    } else {
        Write-Info "Cloning from: $GitRepo"
        Write-Info "Destination: $ZeroADDir"
        Write-Info ""
        Write-Info "This will take a while (~5-15 GB download)..."
        Write-Info "Using --depth 1 for shallow clone to save time and space."
        Write-Info ""

        # Shallow clone to reduce download size significantly
        git clone --depth 1 --single-branch --branch master $GitRepo $ZeroADDir
        if ($LASTEXITCODE -ne 0) {
            Write-Err "Git clone failed. Check your internet connection and try again."
            exit 1
        }
        Write-Info "Clone complete!"
    }
} else {
    Write-Info "Skipping clone (--SkipClone specified)"
}

# ==============================================================================
# Step 2: Set Up Dependencies
# ==============================================================================
if (-not $SkipDeps) {
    Write-Step "Setting Up Dependencies"

    # 0 A.D. bundles pre-built Windows dependencies via SVN or separate download
    # Check if libraries directory exists
    $libsDir = Join-Path $ZeroADDir "libraries"
    $winLibsDir = Join-Path $libsDir "win32"

    if (-not (Test-Path $ZeroADDir)) {
        Write-Err "0 A.D. source not found at $ZeroADDir. Run clone step first."
        exit 1
    }

    # Check for the dependency download script
    $depsScript = Join-Path $ZeroADDir "libraries\download-dependencies.ps1"
    $depsScriptAlt = Join-Path $ZeroADDir "libraries\download-dependencies.sh"
    $depsBat = Join-Path $ZeroADDir "build\workspaces\update-workspaces.bat"

    # 0 A.D. stores Windows libraries in libraries/win32/ subdirectories
    # They can be downloaded via SVN or a bundled script
    if (Test-Path (Join-Path $ZeroADDir "libraries\README.txt")) {
        Write-Info "Libraries directory exists. Checking for pre-built dependencies..."
        $readmeContent = Get-Content (Join-Path $ZeroADDir "libraries\README.txt") -Raw -ErrorAction SilentlyContinue
        Write-Info $readmeContent.Substring(0, [Math]::Min(500, $readmeContent.Length))
    }

    # Check for win32 pre-built libs (0 A.D. stores them in libraries/win32/)
    if (Test-Path $winLibsDir) {
        Write-Info "Win32 libraries directory found at: $winLibsDir"
        $libCount = (Get-ChildItem $winLibsDir -Directory -ErrorAction SilentlyContinue).Count
        Write-Info "  Contains $libCount dependency directories"
    } else {
        Write-Warn "Win32 pre-built libraries not found."
        Write-Info "0 A.D. Windows dependencies need to be downloaded separately."
        Write-Info ""
        Write-Info "Options:"
        Write-Info "  1. Download from: https://releases.wildfiregames.com/dependencies/"
        Write-Info "  2. Use SVN checkout: svn checkout https://svn.wildfiregames.com/public/ps/libraries/win32"
        Write-Info ""

        # Try the SVN approach if svn is available
        $svnAvailable = Get-Command svn -ErrorAction SilentlyContinue
        if ($svnAvailable) {
            Write-Info "SVN found. Downloading Windows dependencies via SVN..."
            Push-Location $libsDir
            svn checkout "https://svn.wildfiregames.com/public/ps/libraries/win32" win32
            Pop-Location
        } else {
            Write-Warn "SVN not available. Dependencies must be downloaded manually."
            Write-Info "Install TortoiseSVN or command-line SVN, then run:"
            Write-Info "  cd $libsDir"
            Write-Info "  svn checkout https://svn.wildfiregames.com/public/ps/libraries/win32 win32"
        }
    }

    # Key dependencies that 0 A.D. requires (bundled in libraries/win32/):
    Write-Info ""
    Write-Info "Required dependencies (should be in libraries/win32/):"
    Write-Info "  - SpiderMonkey (Mozilla's JavaScript engine)"
    Write-Info "  - Boost (C++ utility library)"
    Write-Info "  - SDL2 (Simple DirectMedia Layer)"
    Write-Info "  - OpenAL (audio)"
    Write-Info "  - libpng, zlib, libjpeg (image formats)"
    Write-Info "  - libxml2 (XML parsing)"
    Write-Info "  - ICU (internationalization)"
    Write-Info "  - Enet (networking)"
    Write-Info "  - NVIDIA Texture Tools"
    Write-Info "  - FCollada (COLLADA model loading)"

} else {
    Write-Info "Skipping dependency setup (--SkipDeps specified)"
}

# ==============================================================================
# Step 3: Generate Visual Studio Solution
# ==============================================================================
if (-not $SkipWorkspaces) {
    Write-Step "Generating Visual Studio Solution"

    $workspacesDir = Join-Path $ZeroADDir "build\workspaces"
    $updateBat = Join-Path $workspacesDir "update-workspaces.bat"

    if (-not (Test-Path $updateBat)) {
        Write-Err "update-workspaces.bat not found at: $updateBat"
        Write-Err "The source tree may be incomplete."
        exit 1
    }

    Write-Info "Running update-workspaces.bat to generate VS solution..."
    Write-Info "  Location: $updateBat"
    Write-Info ""

    Push-Location $workspacesDir
    cmd /c "update-workspaces.bat" 2>&1
    $exitCode = $LASTEXITCODE
    Pop-Location

    if ($exitCode -ne 0) {
        Write-Warn "update-workspaces.bat returned exit code: $exitCode"
        Write-Warn "This may indicate missing dependencies."
        Write-Warn "Check the output above for specific errors."
    } else {
        Write-Info "Visual Studio solution generated successfully!"
    }

    # Check if .sln was created
    $slnPath = Join-Path $workspacesDir "vs2017\pyrogenesis.sln"
    $slnPathAlt = Join-Path $workspacesDir "vs2022\pyrogenesis.sln"

    if (Test-Path $slnPath) {
        Write-Info "Solution file: $slnPath"
        Write-Info "(vs2017 format - fully compatible with VS2022, will auto-retarget)"
    } elseif (Test-Path $slnPathAlt) {
        Write-Info "Solution file: $slnPathAlt"
    } else {
        # Check for any .sln files
        $slnFiles = Get-ChildItem (Join-Path $workspacesDir "*") -Filter "*.sln" -Recurse -ErrorAction SilentlyContinue
        if ($slnFiles) {
            Write-Info "Solution file(s) found:"
            $slnFiles | ForEach-Object { Write-Info "  $_" }
        } else {
            Write-Warn "No .sln file found. Check update-workspaces.bat output for errors."
        }
    }
} else {
    Write-Info "Skipping workspace generation (--SkipWorkspaces specified)"
}

# ==============================================================================
# Summary
# ==============================================================================
Write-Step "Setup Summary"

Write-Info "Workspace Root: $WorkspaceRoot"
Write-Info "0 A.D. Source:  $ZeroADDir"

if (Test-Path $ZeroADDir) {
    Write-Info "Source Status:  CLONED"
} else {
    Write-Warn "Source Status:  NOT CLONED"
}

if ($vsInstalled) {
    Write-Info "Visual Studio:  INSTALLED"
} else {
    Write-Warn "Visual Studio:  NOT INSTALLED"
    Write-Warn ""
    Write-Warn "NEXT STEP: Install Visual Studio 2022 Community Edition"
    Write-Warn "  Download: https://visualstudio.microsoft.com/downloads/"
    Write-Warn "  Required workload: 'Desktop development with C++'"
}

Write-Host ""
Write-Host "To build 0 A.D. after setup is complete:" -ForegroundColor White
Write-Host "  1. Open the .sln file in Visual Studio"
Write-Host "  2. Set configuration to Release x64"
Write-Host "  3. Build solution (Ctrl+Shift+B)"
Write-Host "  4. Run pyrogenesis.exe from binaries/system/"
Write-Host ""
