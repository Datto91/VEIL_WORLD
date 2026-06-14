@echo off
rem ** Create Visual Studio 2022 Workspaces on Windows **
rem ** The bundled premake5 supports vs2017 format which VS2022 can open **
rem ** VS2022 will automatically retarget to the installed platform toolset **

cd ..\premake
if not exist ..\workspaces\vs2017\SKIP_PREMAKE_HERE premake5\bin\release\premake5 --outpath="../workspaces/vs2017" --use-shared-glooxwrapper %* vs2017
cd ..\workspaces

echo.
echo Solution generated at: vs2017\pyrogenesis.sln
echo.
echo NOTE: This solution uses vs2017 format but is fully compatible with VS2022.
echo       Visual Studio 2022 will automatically retarget the platform toolset.
echo.
