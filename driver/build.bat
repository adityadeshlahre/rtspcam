@echo off
setlocal enabledelayedexpansion

set KITS=C:\Program Files (x86)\Windows Kits\10

:: Find the highest WDK version in Include dir
for /f "delims=" %%d in ('dir "%KITS%\Include" /b /o-n 2^>nul') do (
    set WDKVER=%%d
    goto found
)
echo WDK not found in %KITS%\Include
echo Install Windows Driver Kit (WDK) for ARM64
exit /b 1

:found
set KM=%KITS%\Include\%WDKVER%\km
set SHARED=%KITS%\Include\%WDKVER%\shared
set LIB=%KITS%\Lib\%WDKVER%\km\arm64

echo WDK version: %WDKVER%

cl.exe /nologo /c /EHsc /kernel /GS- /Zp8 /Gd /Oi /Oy- /W4 /I"%KM%" /I"%SHARED%" driver.cpp device.cpp queue.cpp
if errorlevel 1 exit /b 1

link.exe /nologo /driver /kernel /subsystem:native /out:rtspcam.sys /entry:GsDriverEntry /nodefaultlib /libpath:"%LIB%" ks.lib ntoskrnl.lib driver.obj device.obj queue.obj
if errorlevel 1 exit /b 1

echo Build succeeded: rtspcam.sys
dir rtspcam.sys
