@echo off

setlocal
call "%~dp0vc.build.set.any.bat" %*

if defined VSInstallDir (
  echo calling "%VSInstallDir%\VC\Auxiliary\Build\vcvars64.bat"
  endlocal && call "%VSInstallDir%\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "%VS_COMNTOOLS%..\..\VC\BIN\x86_amd64\vcvarsx86_amd64.bat" (
  call "%VS_COMNTOOLS%..\..\VC\BIN\x86_amd64\vcvarsx86_amd64.bat"
) else if exist "%VS_COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat" (
  call "%VS_COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat"
) else (
  echo VS_COMNTOOLS NOT DEFINED!
  exit /B 100
  goto :EOF
)

where cl.exe
if errorlevel 1 (
echo !!! x64 build failed !!!
exit /B 1
goto :EOF
)

if exist C:\WinDDK\7600.16385.1\lib\Crt\amd64 set LIB=C:\WinDDK\7600.16385.1\lib\Crt\amd64;%LIB%

set WIDE=1
set IA64=
set AMD64=1
set DEBUG=
set NO_RELEASE_PDB=
