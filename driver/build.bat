@echo off
setlocal

set KITS=C:\Program Files (x86)\Windows Kits\10

:: Find highest WDK version (numeric only)
set WDKVER=
for /f "delims=" %%d in ('dir "%KITS%\Include" /b /o-n 2^>nul') do (
    echo %%d|findstr /r "^[0-9]" >nul && set WDKVER=%%d
    if defined WDKVER goto found
)
echo No WDK version found in %KITS%\Include
exit /b 1

:found
set KM=%KITS%\Include\%WDKVER%\km
set SHARED=%KITS%\Include\%WDKVER%\shared
set LIB=%KITS%\Lib\%WDKVER%\km\arm64

echo WDK: %WDKVER%

:: Force ARM64 target architecture
set CL=/D_ARM64_ /D_ARM64

cl.exe /nologo /c /EHsc /kernel /GS- /Zp8 /Gd /Oi /Oy- /W4 /I"%KM%" /I"%SHARED%" driver.cpp device.cpp queue.cpp
if errorlevel 1 exit /b 1

link.exe /nologo /machine:arm64 /driver /kernel /subsystem:native /out:rtspcam.sys /entry:GsDriverEntry /nodefaultlib /libpath:"%LIB%" ks.lib ntoskrnl.lib driver.obj device.obj queue.obj
if errorlevel 1 exit /b 1

echo Build succeeded: rtspcam.sys
dir rtspcam.sys
