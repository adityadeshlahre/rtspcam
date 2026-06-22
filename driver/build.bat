@echo off
setlocal enabledelayedexpansion

set DRIVER_PATH=%~dp0
set FILES=driver.cpp device.cpp queue.cpp

:: Try to find VS + WDK paths
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" arm64
) else (
    echo Visual Studio 2022 not found. Run from an ARM64 developer prompt.
    exit /b 1
)

:: Find WDK path
set WDK=%WindowsSdkDir%..\..\Include\
if not exist "%WDK%" (
    echo WDK not found.
    exit /b 1
)

for /f %%d in ('dir /b /o-n "%WDK%"') do (
    set WDK_VER=%%d
    goto found
)
:found

echo Building with WDK version: !WDK_VER!
set INC=/I"%WDK%!WDK_VER!\km" /I"%WDK%!WDK_VER!\shared" /I"%WDK%!WDK_VER!\ks"
set LIB_PATH="%WindowsSdkDir%..\..\Lib\!WDK_VER!\km\arm64"

cd /d "%DRIVER_PATH%"

cl.exe /nologo /c /EHsc /kernel /GS- /Zp8 /Gd /Oi /Oy- /W4 !INC! %FILES%
if errorlevel 1 exit /b 1

link.exe /nologo /driver /kernel /subsystem:native /out:rtspcam.sys ^
    /entry:GsDriverEntry /nodefaultlib /libpath:%LIB_PATH% ^
    ks.lib ntoskrnl.lib ^
    driver.obj device.obj queue.obj
if errorlevel 1 exit /b 1

echo.
echo Build succeeded: rtspcam.sys
dir rtspcam.sys
