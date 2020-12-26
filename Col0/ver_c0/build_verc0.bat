@echo off

set MAKEFILE=makefile_verc0

setlocal
call "%~dp0..\..\tools\vc.build.set.x32.bat"
set WIDE=1
set IA64=
set AMD64=
nmake /A /B /F %MAKEFILE% FAR_UNICODE=4245
endlocal

setlocal
call "%~dp0..\..\tools\vc.build.set.x64.bat"
set WIDE=1
set IA64=
set AMD64=1
nmake /A /B /F %MAKEFILE% FAR_UNICODE=4245
endlocal
