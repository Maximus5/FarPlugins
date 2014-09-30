#ifndef __FARVERSION_HPP__
#define __FARVERSION_HPP__
#include "plugin.hpp"

#define FAR_MAJOR_VER FARMANAGERVERSION_MAJOR
#define FAR_MINOR_VER FARMANAGERVERSION_MINOR
#define FAR_BUILD FARMANAGERVERSION_BUILD
#define FARCOMPANYNAME "Eugene Roshal & FAR Group"
#define FARGROUPCOPYRIGHT(start_year) "Copyright � " start_year "-2009 FAR Group"
#define FARCOPYRIGHT "Copyright � Eugene Roshal 1996-2000, " FARGROUPCOPYRIGHT("2000")
#define FARPRODUCTNAME "FAR Manager"

#define FULLMAKEPRODUCTVERSION(major, minor, build) #major "." #minor " build " #build
#define MAKEPRODUCTVERSION(major, minor, build) FULLMAKEPRODUCTVERSION(major, minor, build)

#define FARPRODUCTVERSION MAKEPRODUCTVERSION(FAR_MAJOR_VER, FAR_MINOR_VER, FAR_BUILD)

#define fullgenericpluginrc(major, minor, build, desc, name, filename, copyright, pmajor, pminor, pbuild, pname) \
1 VERSIONINFO \
FILEVERSION major, minor, 0, build \
PRODUCTVERSION pmajor, pminor, 0, pbuild \
FILEOS 4 \
FILETYPE 2 \
{ \
 BLOCK "StringFileInfo" \
 { \
  BLOCK "000004E4" \
  { \
   VALUE "CompanyName", FARCOMPANYNAME "\000\000" \
   VALUE "FileDescription", desc "\000" \
   VALUE "FileVersion", MAKEPRODUCTVERSION(major, minor, build) "\000" \
   VALUE "InternalName", name "\000" \
   VALUE "LegalCopyright", copyright "\000\000" \
   VALUE "OriginalFilename", filename "\000" \
   VALUE "ProductName", pname "\000"\
   VALUE "ProductVersion", MAKEPRODUCTVERSION(pmajor, pminor, pbuild) "\000" \
  } \
\
 } \
\
 BLOCK "VarFileInfo" \
 { \
   VALUE "Translation", 0, 0x4e4 \
 } \
\
}

#define genericpluginrc(build, desc, name, filename) fullgenericpluginrc(FAR_MAJOR_VER, FAR_MINOR_VER, build, desc, name, filename, FARCOPYRIGHT, FAR_MAJOR_VER, FAR_MINOR_VER, FAR_BUILD, FARPRODUCTNAME)

#define fullgenericpluginrc_nobuild(major, minor, desc, name, filename, copyright, pmajor, pminor, pbuild, pname) \
1 VERSIONINFO \
FILEVERSION major, minor, 0, 0 \
PRODUCTVERSION pmajor, pminor, 0, pbuild \
FILEOS 4 \
FILETYPE 2 \
{ \
 BLOCK "StringFileInfo" \
 { \
  BLOCK "000004E4" \
  { \
   VALUE "CompanyName", FARCOMPANYNAME "\000\000" \
   VALUE "FileDescription", desc "\000" \
   VALUE "FileVersion", #major "." #minor "\000" \
   VALUE "InternalName", name "\000" \
   VALUE "LegalCopyright", copyright "\000\000" \
   VALUE "OriginalFilename", filename "\000" \
   VALUE "ProductName", pname "\000"\
   VALUE "ProductVersion", MAKEPRODUCTVERSION(pmajor, pminor, pbuild) "\000" \
  } \
\
 } \
\
 BLOCK "VarFileInfo" \
 { \
   VALUE "Translation", 0, 0x4e4 \
 } \
\
}

#endif
