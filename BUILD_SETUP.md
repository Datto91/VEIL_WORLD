# 0 A.D. MCP Controller - Build Environment Setup

## Overview

This document describes how to set up the build environment for the 0 A.D. MCP Controller project on Windows.

## Prerequisites

### Required Software

1. **Git for Windows** - Already installed (v2.53.0)
2. **Visual Studio 2022** (Community Edition is free)
   - Download: https://visualstudio.microsoft.com/downloads/
   - Required workload: **Desktop development with C++**
   - Required components:
     - MSVC v143 build tools (x64/x86)
     - Windows 10/11 SDK (latest)
     - C++ CMake tools (optional, for some deps)
3. **SVN client** (for downloading pre-built dependencies)
   - TortoiseSVN: https://tortoisesvn.net/downloads.html
   - Or command-line SVN via chocolatey: `choco install svn`

### Disk Space

- Source code (shallow clone): ~2-3 GB
- Pre-built dependencies: ~3-5 GB  
- Build output: ~5-10 GB
- **Total recommended: 25+ GB free**

## Setup Steps

### Step 1: Clone Source Repository

The 0 A.D. source is cloned from the GitHub mirror using a shallow clone:

```powershell
cd c:\Users\byrcharl\Documents\KIRO_GAMES\VEIL_WORLD
git clone --depth 1 --single-branch --branch master https://github.com/0ad/0ad.git 0ad
```

This creates the `0ad/` directory with the full source tree.

### Step 2: Download Pre-Built Dependencies

0 A.D. uses pre-built Windows libraries stored in SVN. These include:
- SpiderMonkey (JavaScript engine)
- Boost
- SDL2
- OpenAL
- libpng, zlib, libjpeg
- libxml2, ICU
- Enet
- FCollada

Download them into `0ad/libraries/win32/`:

```powershell
cd 0ad\libraries
svn checkout https://svn.wildfiregames.com/public/ps/libraries/win32 win32
```

Alternatively, some builds bundle dependencies. Check if `libraries/win32/` is populated after clone.

### Step 3: Generate Visual Studio Solution

Run the workspace generator:

```powershell
cd 0ad\build\workspaces
.\update-workspaces.bat
```

Or use the custom VS2022-friendly wrapper:
```powershell
.\update-workspaces-vs2022.bat
```

This runs premake5 to generate a Visual Studio solution file at:
- `0ad/build/workspaces/vs2017/pyrogenesis.sln`

**Note:** The bundled premake5 only supports the `vs2017` action, but the generated solution is fully compatible with Visual Studio 2022. VS2022 will auto-retarget the platform toolset on first open.

### Step 4: Build the Project

1. Open `pyrogenesis.sln` in Visual Studio 2022
2. Set solution configuration to **Release** and platform to **x64**
3. Build the solution (Ctrl+Shift+B)
4. The executable is produced at `0ad/binaries/system/pyrogenesis.exe`

### Step 5: Verify

Launch the game:
```powershell
cd 0ad\binaries\system
.\pyrogenesis.exe
```

The game should start and reach the main menu.

## Automated Setup

Run the automated setup script:

```powershell
.\setup-0ad-build.ps1
```

Or the batch wrapper:
```cmd
setup-0ad-build.bat
```

Flags:
- `-SkipClone` - Skip the git clone step
- `-SkipDeps` - Skip dependency download
- `-SkipWorkspaces` - Skip VS solution generation

## Troubleshooting

### Clone is too slow
The full repo with history is ~8 GB. We use `--depth 1` for a shallow clone (~2-3 GB).

### Missing dependencies error during update-workspaces.bat
Ensure `libraries/win32/` contains all required pre-built libraries. Use SVN checkout as described in Step 2.

### Visual Studio can't find SDK
Ensure you installed the Windows 10/11 SDK as part of VS setup. Run the Visual Studio Installer and add it.

### premake errors
Check that premake5 is present at `0ad/build/premake/premake5.exe`. It should be included in the source tree.

## Project Structure (relevant paths)

```
VEIL_WORLD/
  0ad/                          # 0 A.D. source tree (shallow clone from GitHub)
    binaries/system/            # Built executable output
    build/premake/              # premake5 build system
      premake5/bin/release/premake5.exe
    build/workspaces/           # premake + VS solution generation
      update-workspaces.bat     # Original (generates vs2017 format)
      update-workspaces-vs2022.bat  # Wrapper with VS2022 notes
      vs2017/                   # Generated VS solution (compatible with VS2022)
        pyrogenesis.sln
    libraries/                  # Third-party dependencies
      win32/                    # Pre-built Windows libraries (17 packages)
        boost/, sdl2/, openal/, enet/, zlib/, libpng/, libxml2/,
        icu/, freetype/, fmt/, gloox/, iconv/, libcurl/,
        libsodium/, miniupnpc/, vorbis/, wxwidgets/
      source/                   # Source-built libraries
        spidermonkey/           # Mozilla SpiderMonkey JS engine (pre-built .lib included)
    source/                     # C++ source code
      mcp/                      # <-- Our MCP module goes here (Task 2.1+)
  setup-0ad-build.ps1          # Automated setup script
  setup-0ad-build.bat          # Batch wrapper
  BUILD_SETUP.md               # This document
```
