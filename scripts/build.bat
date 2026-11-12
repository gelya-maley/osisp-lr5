@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
if not exist build mkdir build
cl /nologo /W3 /O2 /EHsc /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS /I src\common src\common\common.cpp src\server\server.cpp /Fe:build\server.exe /link ws2_32.lib
if errorlevel 1 exit /b 1
cl /nologo /W3 /O2 /EHsc /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS /I src\common src\common\common.cpp src\client\client.cpp /Fe:build\client.exe /link ws2_32.lib
if errorlevel 1 exit /b 1
echo BUILD OK
endlocal
