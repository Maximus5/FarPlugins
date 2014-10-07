@echo off

call "%~dp0vc.build.set.any.bat" %*

if exist "%VS_COMNTOOLS%..\..\VC\BIN\vcvars32.bat" (
  call "%VS_COMNTOOLS%..\..\VC\BIN\vcvars32.bat"
) else (
  echo VS_COMNTOOLS NOT DEFINED!
  exit /B 100
  goto :EOF
)

if errorlevel 1 (
echo !!! x86 build failed !!!
exit /B 1
goto :EOF
)

set WIDE=1
set IA64=
set AMD64=
set DEBUG=
set NO_RELEASE_PDB=
