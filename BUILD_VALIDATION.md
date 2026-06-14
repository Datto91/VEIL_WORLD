# 0 A.D. Build Validation Report

## Status: BLOCKED — Visual Studio Not Installed

Build cannot proceed because Visual Studio is not installed on this machine.
Once VS is installed, run `build_0ad.ps1` (see below) for a command-line build.

## Solution Structure

| Item | Value |
|------|-------|
| Solution File | `build\workspaces\vs2017\pyrogenesis.sln` |
| Format | Visual Studio 2017 (Format Version 12.00) |
| Platform | **Win32** (32-bit) |
| Platform Toolset | `v141_xp` (VS2017 + XP support) |
| C++ Standard | C++17 (`/std:c++17`) |
| Output Directory | `binaries\system\` |
| Target Executable | `pyrogenesis.exe` (Release) / `pyrogenesis_dbg.exe` (Debug) |

### Projects in Solution (18 total)

| Project | Type | Purpose |
|---------|------|---------|
| pyrogenesis | Application (.exe) | Main game executable |
| atlas | Library | Map/scenario editor (requires wxwidgets) |
| Collada | Library | 3D model conversion |
| engine | Library | Core engine systems |
| gladwrapper | Library | OpenGL loader |
| graphics | Library | Rendering pipeline |
| gui | Library | In-game UI system |
| lobby | Library | Multiplayer lobby |
| lowlevel | Library | Platform abstraction layer |
| mocks_real / mocks_test | Library | Test infrastructure |
| mongoose | Library | Embedded HTTP server |
| network | Library | Multiplayer networking |
| rlinterface | Library | Reinforcement learning interface |
| scriptinterface | Library | SpiderMonkey JS bridge |
| simulation2 | Library | Game simulation logic |
| test | Application (.exe) | Unit test runner |
| tinygettext | Library | Internationalization |

## Dependency Status

### All Required Dependencies: PRESENT ✓

| Library | Location | lib/ | include/ |
|---------|----------|------|----------|
| SDL2 | `libraries/win32/sdl2` | ✓ | ✓ |
| libpng | `libraries/win32/libpng` | ✓ | ✓ |
| zlib | `libraries/win32/zlib` | ✓ | ✓ |
| SpiderMonkey (mozjs91) | `libraries/source/spidermonkey` | ✓ | ✓ |
| libxml2 | `libraries/win32/libxml2` | ✓ | ✓ |
| Boost | `libraries/win32/boost` | ✓ | ✓ |
| enet | `libraries/win32/enet` | ✓ | ✓ |
| libcurl | `libraries/win32/libcurl` | ✓ | ✓ |
| ICU | `libraries/win32/icu` | ✓ | ✓ |
| iconv | `libraries/win32/iconv` | ✓ | ✓ |
| libsodium | `libraries/win32/libsodium` | ✓ | ✓ |
| fmt | `libraries/win32/fmt` | ✓ | ✓ |
| freetype | `libraries/win32/freetype` | ✓ | ✓ |
| OpenAL | `libraries/win32/openal` | ✓ | ✓ |
| Vorbis | `libraries/win32/vorbis` | ✓ | ✓ |
| NVTT | `libraries/source/nvtt` | ✓ | ✓ |
| gloox | `libraries/win32/gloox` | ✓ | ✓ |
| miniupnpc | `libraries/win32/miniupnpc` | ✓ | ✓ |
| GLAD | `libraries/source/glad` | — | ✓ |

### Optional Dependencies

| Library | Status | Notes |
|---------|--------|-------|
| wxwidgets | NOT INSTALLED | Only needed for Atlas map editor (not pyrogenesis.exe) |

### SpiderMonkey Lib Files

| File | Size |
|------|------|
| `mozjs91-ps-release.lib` | 1,113,632 bytes |
| `mozjs91-ps-rust.lib` | 13,209,318 bytes |
| `mozjs91-ps-debug.lib` | 1,130,308 bytes |
| `mozjs91-ps-rust-debug.lib` | 35,529,850 bytes |

### Runtime DLLs Pre-deployed in `binaries/system/`

52 DLLs are already present including: SDL2.dll, mozjs91-ps-release.dll, OpenAL32.dll,
libpng16.dll, zlib1.dll, libcurl.dll, enet.dll, libvorbisfile.dll, nvtt.dll,
miniupnpc.dll, gloox-1.0.dll, freetype.dll, libsodium.dll, icuuc68.dll, and more.

## Build Requirements

### Visual Studio Installation

Install one of:
- **Visual Studio 2022 Community** (free) — recommended
- Visual Studio 2022 Professional or Enterprise

Required workloads:
- "Desktop development with C++"
- Individual components needed:
  - MSVC v143 build tools (or current)
  - Windows SDK (10.0 or later)
  - C++ ATL support (for some components)

### Important Notes

1. **Platform is Win32**: The solution targets 32-bit (Win32), NOT x64
2. **Toolset upgrade**: VS2022 will prompt to retarget from `v141_xp` to `v143` — accept this
3. **wxwidgets not needed**: The Atlas editor project will fail to build without wxwidgets, but pyrogenesis.exe itself does NOT depend on it. Build just the `pyrogenesis` project.
4. **No additional downloads**: All pre-built libraries and DLLs are already in place

## Build Command

Once Visual Studio is installed, use the provided `build_0ad.ps1` script or run manually:

```powershell
# Find MSBuild
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"

# Build pyrogenesis only (skips Atlas which needs wxwidgets)
& $msbuild "build\workspaces\vs2017\pyrogenesis.vcxproj" /p:Configuration=Release /p:Platform=Win32 /m
```

## Verification Steps

After successful build:
1. Verify `binaries\system\pyrogenesis.exe` exists
2. (Optional) Launch: `cd binaries\system && .\pyrogenesis.exe`
3. Game should reach main menu if all data files are present
