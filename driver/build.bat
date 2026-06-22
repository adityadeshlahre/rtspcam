@echo off
setlocal

:: Find the highest WDK version installed
set WDKROOT=%WindowsSdkDir%..\..\Include
for /f "usebackq delims=" %%d in (`dir "%WDKROOT%" /b /o-n 2^>nul`) do (
    set WDKVER=%%d
    goto found
)
echo WDK not found at %WDKROOT%
exit /b 1

:found
set KM=%WDKROOT%\%WDKVER%\km
set SHARED=%WDKROOT%\%WDKVER%\shared
echo Using WDK %WDKVER%

cl.exe /nologo /c /EHsc /kernel /GS- /Zp8 /Gd /Oi /Oy- /W4 /I"%KM%" /I"%SHARED%" driver.cpp device.cpp queue.cpp
if errorlevel 1 exit /b 1

link.exe /nologo /driver /kernel /subsystem:native /out:rtspcam.sys /entry:GsDriverEntry /nodefaultlib /libpath:"%WindowsSdkDir%..\..\Lib\%WDKVER%\km\arm64" ks.lib ntoskrnl.lib driver.obj device.obj queue.obj
if errorlevel 1 exit /b 1

echo Build succeeded: rtspcam.sys
dir rtspcam.sys
