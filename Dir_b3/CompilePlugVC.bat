@echo off
@cls

rem =============== Use Microsoft Visual Studio .NET 2003 ======================

rem @call "%VS60COMN%\..\VC98\Bin\VCVARS32.BAT"
rem if exist "%VS71COMNTOOLS%\vsvars32.bat" call "%VS71COMNTOOLS%\vsvars32.bat"
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
  goto :start_build
)
if exist "%VS90COMNTOOLS%..\..\VC\BIN\vcvars32.bat" call "%VS90COMNTOOLS%..\..\VC\BIN\vcvars32.bat"

rem  ======================== Set name and version ... =========================

:start_build
@set PlugName=dir
@set fileversion=3,0,0,8
@set fileversion_str=3.0 build 8
@set comments=Current developer: ConEmu.Maximus5@gmail.com
@set filedescription=DIR XP/2003/Vista/7/10 parse for FAR Manager
@set legalcopyright=© Alexander Arefiev 2001, @ Maximus5 2022

rem  ==================== remove temp files ====================================

if not exist %PlugName%.map goto nomap
attrib -H %PlugName%.map
del %PlugName%.map
:nomap

rem  ==================== Make %PlugName%.def file... ==========================

rem @if not exist %PlugName%.def (
echo Make %PlugName%.def file...

rem @echo LIBRARY           %PlugName%.so                      > %PlugName%.def
@echo EXPORTS                                              >  %PlugName%.def
@echo   IsArchive                                          >> %PlugName%.def
@echo   OpenArchive                                        >> %PlugName%.def
@echo   GetArcItem                                         >> %PlugName%.def
@echo   CloseArchive                                       >> %PlugName%.def
@echo   GetFormatName                                      >> %PlugName%.def
@echo   GetDefaultCommands                                 >> %PlugName%.def
@echo   LoadSubModule                                      >> %PlugName%.def
@echo   UnloadSubModule                                    >> %PlugName%.def
rem @echo   SetFarInfo                                         >> %PlugName%.def

@if exist %PlugName%.def echo ... succesfully
rem )

rem  ================== Make %PlugName%.rc file... =============================

@if not exist %PlugName%.rc (
echo Make %PlugName%.rc file...

@echo #define VERSIONINFO_1   1                                 > %PlugName%.rc
@echo.                                                          >> %PlugName%.rc
@echo VERSIONINFO_1  VERSIONINFO                                >> %PlugName%.rc
@echo FILEVERSION    %fileversion%                              >> %PlugName%.rc
@echo PRODUCTVERSION 1,71,0,0                                   >> %PlugName%.rc
@echo FILEFLAGSMASK  0x0                                        >> %PlugName%.rc
@echo FILEFLAGS      0x0                                        >> %PlugName%.rc
@echo FILEOS         0x4                                        >> %PlugName%.rc
@echo FILETYPE       0x2                                        >> %PlugName%.rc
@echo FILESUBTYPE    0x0                                        >> %PlugName%.rc
@echo {                                                         >> %PlugName%.rc
@echo   BLOCK "StringFileInfo"                                  >> %PlugName%.rc
@echo   {                                                       >> %PlugName%.rc
@echo     BLOCK "000004E4"                                      >> %PlugName%.rc
@echo     {                                                     >> %PlugName%.rc
@echo       VALUE "CompanyName",      "%companyname%\0"         >> %PlugName%.rc
@echo       VALUE "FileDescription",  "%filedescription%\0"     >> %PlugName%.rc
@echo       VALUE "FileVersion",      "%fileversion_str%\0"     >> %PlugName%.rc
@echo       VALUE "InternalName",     "%PlugName%\0"            >> %PlugName%.rc
@echo       VALUE "LegalCopyright",   "%legalcopyright%\0"      >> %PlugName%.rc
@echo       VALUE "OriginalFilename", "%PlugName%.fmt\0"        >> %PlugName%.rc
@echo       VALUE "ProductName",      "FAR Manager\0"           >> %PlugName%.rc
@echo       VALUE "ProductVersion",   "1.71\0"                  >> %PlugName%.rc
@echo       VALUE "Comments",         "%comments%\0"            >> %PlugName%.rc
@echo     }                                                     >> %PlugName%.rc
@echo   }                                                       >> %PlugName%.rc
@echo   BLOCK "VarFileInfo"                                     >> %PlugName%.rc
@echo   {                                                       >> %PlugName%.rc
@echo     VALUE "Translation", 0, 0x4e4                         >> %PlugName%.rc
@echo   }                                                       >> %PlugName%.rc
@echo }                                                         >> %PlugName%.rc

@if exist %PlugName%.rc echo ... succesfully
)

rem  ==================== Compile %PlugName%.fmt file...========================

@if exist %PlugName%.fmt del %PlugName%.fmt>nul
@if exist %PlugName%.so del %PlugName%.so>nul
@rc /l 0x4E4 %PlugName%.rc
if errorlevel 1 goto err1

rem @echo !!!!!!!  Compile %PlugName%.fmt with MSVCRT.dll ...  !!!!!!!
@echo ***************
set libcrt=
if exist libCRT.lib set libcrt=/nodefaultlib libCRT.lib
@cl /Zp8 /O2 /GF /Gr /GS- /GR- /EHs-c- /MT %PlugName%.cpp /link /DLL /RELEASE /subsystem:console /machine:I386 /noentry /def:%PlugName%.def %libcrt% kernel32.lib User32.lib %PlugName%.res /map:"%PlugName%.map" /out:"%PlugName%.so" /merge:.rdata=.text
if errorlevel 1 goto err1

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
  goto :start_build_x64
)
if not exist "%VS90COMNTOOLS%..\..\VC\BIN\x86_amd64\vcvarsx86_amd64.bat" goto no_x64
call "%VS90COMNTOOLS%..\..\VC\BIN\x86_amd64\vcvarsx86_amd64.bat"

:start_build_x64
if exist %PlugName%.exp del %PlugName%.exp>nul
if exist %PlugName%.obj del %PlugName%.obj>nul
if exist %PlugName%.lib del %PlugName%.lib>nul
if exist %PlugName%.res del %PlugName%.res>nul

@if exist %PlugName%64.fmt del %PlugName%64.fmt>nul
@if exist %PlugName%64.so del %PlugName%64.so>nul

@rc /D_WIN64 /l 0x4E4 %PlugName%.rc
if errorlevel 1 goto err1

set libcrt=
if exist libCRT64.lib set libcrt=/nodefaultlib libCRT64.lib
@cl /Zp8 /O2 /GF /Gr /GS- /GR- /EHs-c- /MT %PlugName%.cpp /link /DLL /RELEASE /subsystem:console /machine:X64 /noentry /def:%PlugName%.def %libcrt% kernel32.lib User32.lib %PlugName%.res /map:"%PlugName%64.map" /out:"%PlugName%64.so" /merge:.rdata=.text
if errorlevel 1 goto err1

:no_x64
if exist %PlugName%.exp del %PlugName%.exp>nul
if exist %PlugName%64.exp del %PlugName%64.exp>nul
if exist %PlugName%.obj del %PlugName%.obj>nul
if exist %PlugName%.lib del %PlugName%.lib>nul
if exist %PlugName%64.lib del %PlugName%64.lib>nul
if exist %PlugName%.res del %PlugName%.res>nul
if exist %PlugName%.def del %PlugName%.def>nul
if exist %PlugName%.rc  del %PlugName%.rc>nul
rem if exist %PlugName%.map del %PlugName%.map>nul

echo ***************



rem if not exist dir.fmt goto err1
rem if exist E:\Far\Plugins\MultiArc\Formats\Dir.fm_ del E:\Far\Plugins\MultiArc\Formats\Dir.fm_
rem if exist E:\Far\Plugins\MultiArc\Formats\Dir.fmt ren E:\Far\Plugins\MultiArc\Formats\Dir.fmt Dir.fm_
rem copy dir.fmt E:\Far\Plugins\MultiArc\Formats\Dir.fmt
rem
rem if exist E:\Far\Unicode\Plugins\MultiArc\Formats\Dir.fm_ del E:\Far\Unicode\Plugins\MultiArc\Formats\Dir.fm_
rem if exist E:\Far\Unicode\Plugins\MultiArc\Formats\Dir.fmt ren E:\Far\Unicode\Plugins\MultiArc\Formats\Dir.fmt Dir.fm_
rem copy dir.fmt E:\Far\Unicode\Plugins\MultiArc\Formats\Dir.fmt

:err1
