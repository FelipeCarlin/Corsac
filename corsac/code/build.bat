@echo off

if not exist ..\..\build mkdir ..\..\build
pushd ..\..\build

:: Compile 64-bit
cl -nologo -Od -Z7 -FC ..\corsac\code\win32_corsac.cpp -Fe: corsac.exe -DCORSAC_SLOW=1

popd