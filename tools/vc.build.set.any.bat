@echo off

rem if "%VS90COMNTOOLS%"=="" if exist "C:\Program Files\Microsoft Visual Studio 9.0\VC\bin" set VS90COMNTOOLS=C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\
rem if "%VS90COMNTOOLS%"=="" if exist "C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat" set VS90COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\


rem VS120COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\
rem VS110COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\
rem VS100COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\
rem VS90COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\

set VS_VERSION=%~1
if "%VS_VERSION%" == "" set VS_VERSION=9

if "%VS_VERSION%" == "9"  goto ver_9
if "%VS_VERSION%" == "10" goto ver_10
if "%VS_VERSION%" == "11" goto ver_11
if "%VS_VERSION%" == "12" goto ver_12

:ver_9
if "%VS90COMNTOOLS%"=="" if "%PROCESSOR_ARCHITEW6432%"=="AMD64" set VS90COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\BIN\
if "%VS90COMNTOOLS%"=="" if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set VS90COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\BIN\
if "%VS90COMNTOOLS%"=="" set VS90COMNTOOLS=C:\Program Files\Microsoft Visual Studio 9.0\VC\BIN\
set VS_COMNTOOLS=%VS90COMNTOOLS%
goto done

:ver_10
if "%VS100COMNTOOLS%"=="" if "%PROCESSOR_ARCHITEW6432%"=="AMD64" set VS100COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN\
if "%VS100COMNTOOLS%"=="" if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set VS100COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN\
if "%VS100COMNTOOLS%"=="" set VS100COMNTOOLS=C:\Program Files\Microsoft Visual Studio 10.0\VC\BIN\
set VS_COMNTOOLS=%VS100COMNTOOLS%
goto done

:ver_11
if "%VS110COMNTOOLS%"=="" if "%PROCESSOR_ARCHITEW6432%"=="AMD64" set VS110COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\BIN\
if "%VS110COMNTOOLS%"=="" if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set VS110COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\BIN\
if "%VS110COMNTOOLS%"=="" set VS110COMNTOOLS=C:\Program Files\Microsoft Visual Studio 11.0\VC\BIN\
set VS_COMNTOOLS=%VS110COMNTOOLS%
goto done

:ver_12
if "%VS120COMNTOOLS%"=="" if "%PROCESSOR_ARCHITEW6432%"=="AMD64" set VS120COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\BIN\
if "%VS120COMNTOOLS%"=="" if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set VS120COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\BIN\
if "%VS120COMNTOOLS%"=="" set VS120COMNTOOLS=C:\Program Files\Microsoft Visual Studio 12.0\VC\BIN\
set VS_COMNTOOLS=%VS120COMNTOOLS%
goto done

:done
set VS
