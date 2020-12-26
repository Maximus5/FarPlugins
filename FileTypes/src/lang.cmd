@echo off
cd /d "%~dp0"
if exist ..\FileTypes_*.lng del ..\FileTypes_*.lng
"%~dp0lng.generator.exe" -nc -i lang.ini -ol Lang.templ
for %%g in (..\FileTypes_*.lng) do \VCProject\Files\Convert\Convert.exe UTF2OEM %%g
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\FileTypes_*.lng "%~1"
)
