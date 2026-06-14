@echo off
REM ============================================================================
REM 0 A.D. Build Environment Setup - Batch Wrapper
REM ============================================================================
REM This simply invokes the PowerShell setup script with appropriate flags.
REM Run this from an elevated (Administrator) command prompt for best results.
REM ============================================================================

echo.
echo  0 A.D. MCP Controller - Build Environment Setup
echo  ================================================
echo.

powershell -ExecutionPolicy Bypass -File "%~dp0setup-0ad-build.ps1" %*

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Setup encountered errors. Check output above.
    pause
)
