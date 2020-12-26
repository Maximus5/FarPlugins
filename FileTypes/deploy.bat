@echo off

cd /d "%~dp0"

if "%~1"=="" goto noparm

rem if exist *.pdb del *.pdb
rem if exist *.lib del *.lib
rem if exist *.exp del *.exp

set ex7zlist=-x!*.map -x!*.exp -x!*.lib -x!*.pdb -x!*.suo -x!*.user -x!*.ncb -x!*.log -x!gcc -x!enc_temp_folder -x!*684.* -x!enc_temp_folder -x!*.bak -x!*.vsp -x!*.ilk -x!rebar.bmp -x!kl_parts_gcc.* -x!*.opt -x!*.aps -x!*.7z -x!0PictureView.*.dll -x!0PictureView.dll -x!src\PictureView\headers\plugin977.hpp -x!VTune -x!*.pvd -x!libgfl311.dll -x!090517.Show.Elevation.status.txt -x!run_debug_conman.bat -x!*.hab -x!*.cache -x!*.dsw -x!drag -x!toolbar -x!ResTest -x!Temp

if %COMPUTERNAME%==MAX goto skip_svn

xcopy /S /U /M /Y *.lng F:\VCProject\FarPlugin\ConEmu\WWW\root\trunk\FileTypes\
xcopy /S /U /M /Y src\*.* F:\VCProject\FarPlugin\ConEmu\WWW\root\trunk\FileTypes\src\



if exist FileTypes.1.%~1.7z del FileTypes.1.%~1.7z
7z a -r FileTypes.1.%~1.7z *.dll *.lng
if errorlevel 1 goto err7z

:skip_svn

if exist FileTypes.1.%~1.src.7z del FileTypes.1.%~1.src.7z
7z a -r FileTypes.1.%~1.src.7z * %ex7zlist%
if errorlevel 1 goto err7z

goto fin

:noparm
echo Usage:    deploy.cmd ^<Version^> [+]
echo Example:  deploy.cmd 5
pause
goto fin

:err7z
echo Can't create archive
pause
goto fin

:fin
