@echo off
cd /d "%~dp0"
if exist ..\Release\RegEditor_*.lng del ..\Release\RegEditor_*.lng
rem Generate UTF-8 lng files
"%~dp0..\..\tools\lng.generator.exe" -nc -i lang.ini -ol Lang.templ
rem Old ANSI verions requires OEM codepage
"%~dp0..\..\tools\Convert.exe" UTF2OEM ..\Release\RegEditor_en.lng
copy ..\Release\RegEditor_ru.lng ..\Release\RegEditor_ru._lng
"%~dp0..\..\tools\Convert.exe" UTF2OEM ..\Release\RegEditor_ru._lng
rem Alternative destination?
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\Release\RegEditor_*.lng "%~1"
)
rem Copy to Debug folder
if exist ..\Debug copy ..\Release\RegEditor_*.lng ..\Debug
rem Done
echo *.lng compilation done
