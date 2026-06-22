@echo off
setlocal

:: Ensure ARM64 build environment
set VSCMD_START_DIR=%CD%
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
        call "%%i\VC\Auxiliary\Build\vcvarsall.bat" arm64 2>nul
    )
)
if errorlevel 1 (
    echo Warning: vcvarsall.bat not found, using PATH as-is.
    echo Ensure you are running from an ARM64 developer prompt.
)

set KITS=C:\Program Files (x86)\Windows Kits\10

:: Prefer the last stable WDK over pre-release (10.0.28000.0)
set WDKVER=
for /f "delims=" %%d in ('dir "%KITS%\Include" /b /o-n 2^>nul') do (
    echo %%d|findstr /r "^[0-9]" >nul
    if not errorlevel 1 if not "%%d"=="10.0.28000.0" set WDKVER=%%d
    if not errorlevel 1 if not "%%d"=="10.0.28000.0" goto found
)
echo No WDK version found in %KITS%\Include
exit /b 1

:found
set KM=%KITS%\Include\%WDKVER%\km
set SHARED=%KITS%\Include\%WDKVER%\shared
set LIB=%KITS%\Lib\%WDKVER%\km\arm64

echo WDK: %WDKVER%

cl.exe /nologo /c /EHsc /std:c++17 /kernel /GS- /Zp8 /Gd /Oi /Oy- /W4 /D_ARM64_ /I"%KM%" /I"%SHARED%" driver.cpp device.cpp queue.cpp
if errorlevel 1 exit /b 1

link.exe /nologo /machine:arm64 /driver /kernel /subsystem:native /out:rtspcam.sys /entry:GsDriverEntry /nodefaultlib /libpath:"%LIB%" ks.lib ntoskrnl.lib driver.obj device.obj queue.obj
if errorlevel 1 exit /b 1

echo Build succeeded: rtspcam.sys
dir rtspcam.sys
