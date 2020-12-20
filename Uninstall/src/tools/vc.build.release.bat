@echo off

if exist makefile set MAKEFILE=makefile

if exist "%VS90COMNTOOLS%..\..\VC\BIN\vcvars32.bat" (
  call "%VS90COMNTOOLS%..\..\VC\BIN\vcvars32.bat"
) else (
  call "C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat"
)

set WIDE=1
set IA64=
set AMD64=
set DEBUG=

nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%
if errorlevel 1 goto end

rem rem call "%VS90COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat"
rem if exist "%VS90COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat" (
rem   call "%VS90COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat"
rem ) else (
rem   call "C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\vcvarsx86_amd64.bat"
rem )
rem set AMD64=1
rem nmake /A /B /F %MAKEFILE%
rem rem nmake /F %MAKEFILE%

:end
