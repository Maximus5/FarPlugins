@echo off

if exist makefile_lib_vc set MAKEFILE=makefile_lib_vc
if exist makefile_vc set MAKEFILE=makefile_vc


call "%VS90COMNTOOLS%..\..\VC\BIN\vcvars32.bat"
set WIDE=
set IA64=
set AMD64=
set DEBUG=
nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%
if errorlevel 1 goto end

set WIDE=1
nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%
if errorlevel 1 goto end



call "%VS90COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat"
set WIDE=
set AMD64=1
nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%

set WIDE=1
nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%

:end
