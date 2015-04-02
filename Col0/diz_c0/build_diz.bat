@echo off

set MAKEFILE=makefile_vc

call "%~dp0..\..\tools\vc.build.set.x32.bat"

set WIDE=1
set IA64=
set AMD64=

nmake /A /B /F %MAKEFILE%

call "%~dp0..\..\tools\vc.build.set.x64.bat"
set AMD64=1
nmake /A /B /F %MAKEFILE%
