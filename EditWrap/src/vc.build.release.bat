@echo off

call "%~dp0..\..\common\vc.build.sdk.check.bat"
if errorlevel 1 goto :EOF

if exist makefile_vc set MAKEFILE=makefile_vc

call "%~dp0..\..\common\vc.build.set.x32.bat" %*
if errorlevel 1 goto :EOF

rem Thanks to lua's double
set NEEDDEFLIB=1

set WIDE=
set IA64=
set AMD64=
rem ¬ключим отладочную информацию
rem set DEBUG=1
set DEBUG=

rem ANSI
rem set WIDE=1
rem nmake /A /B /F %MAKEFILE%
rem rem nmake /F %MAKEFILE%
rem if errorlevel 1 goto end

rem Old Far3
rem set WIDE=3
rem nmake /A /B /F %MAKEFILE%
rem rem nmake /F %MAKEFILE%
rem if errorlevel 1 goto end

rem Far3 Lua
set WIDE=3L
nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%
if errorlevel 1 goto end


call "%~dp0..\..\common\vc.build.set.x64.bat" %*
if errorlevel 1 goto :EOF

set WIDE=
set AMD64=1
rem nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%

rem FAR2
rem set WIDE=1
rem nmake /A /B /F %MAKEFILE%
rem rem nmake /F %MAKEFILE%

rem Old Far3
rem set WIDE=3
rem nmake /A /B /F %MAKEFILE%
rem rem nmake /F %MAKEFILE%

rem Far3 Lua
set WIDE=3L
nmake /A /B /F %MAKEFILE%
rem nmake /F %MAKEFILE%

:end
