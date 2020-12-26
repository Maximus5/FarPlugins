@echo off
cd /d "%~dp0"
if exist ..\FileTypes_*.lng del ..\FileTypes_*.lng
"%~dp0lng.generator.exe" -nc -i lang.ini -ol Lang.templ
C:\GCC\msys\bin\touch.exe "%~dp0FileTypes_Lang.h"
for %%g in (..\FileTypes_*.lng) do \VCProject\Files\Convert\Convert.exe UTF2OEM %%g
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\FileTypes_*.lng "%~1"
)
