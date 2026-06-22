@echo off
cl.exe /nologo /c /EHsc /kernel /GS- /Zp8 /Gd /Oi /Oy- /W4 driver.cpp device.cpp queue.cpp
if errorlevel 1 exit /b 1
link.exe /nologo /driver /kernel /subsystem:native /out:rtspcam.sys /entry:GsDriverEntry /nodefaultlib ks.lib ntoskrnl.lib driver.obj device.obj queue.obj
if errorlevel 1 exit /b 1
echo Build succeeded: rtspcam.sys
dir rtspcam.sys
