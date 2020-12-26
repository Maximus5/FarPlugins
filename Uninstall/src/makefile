!include project.ini

TOOLSDIR = .\tools

CPPFLAGS = -nologo -Zi -W3 -Gy -GS -GR -EHsc -MP -c
DEFINES = -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_WIN32_WINNT=0x0500 -D_FAR_USE_FARFINDDATA -DSTRSAFE_NO_DEPRECATE
LINKFLAGS = -nologo -debug -incremental:no -map -manifest:no -dynamicbase -nxcompat -largeaddressaware -dll -base:0x68000000
RCFLAGS = -nologo

!if "$(CPU)" == "AMD64" || "$(PLATFORM)" == "x64"
PLATFORM = x64
LIBSUFFIX = 64
!ifdef WIDE
DLLSUFFIX = W64
!else
DLLSUFFIX = A64
!endif
RCFLAGS = $(RCFLAGS) -Dx64
!else
PLATFORM = x86
!ifdef WIDE
DLLSUFFIX = W
!else
DLLSUFFIX = A
!endif
LINKFLAGS = $(LINKFLAGS) -safeseh
LIBSUFFIX = 32
!endif

MODULEFULL=$(MODULE)$(DLLSUFFIX)

!ifdef RELEASE
OUTDIR = ..\Release
DEFINES = $(DEFINES) -DNDEBUG
CPPFLAGS = $(CPPFLAGS) -O1 -GL -MT
LINKFLAGS = $(LINKFLAGS) -opt:ref -opt:icf -LTCG /pdb:".\$(OUTDIR)\$(MODULE)$(DLLSUFFIX).pdb"
!else
OUTDIR = ..\Debug
DEFINES = $(DEFINES) -DDEBUG
CPPFLAGS = $(CPPFLAGS) -Od -RTC1 -MTd
LINKFLAGS = $(LINKFLAGS) -fixed:no /pdb:".\$(OUTDIR)\$(MODULE)$(DLLSUFFIX).pdb"
LIBSUFFIX=$(LIBSUFFIX)d
!endif

!ifdef OLDFAR
FARSDK = farsdk\ansi
DEFFILE = UnInstall.def
DEFINES = $(DEFINES) -DFARAPI17
FARBRANCH = 1
!else
FARSDK = farsdk\unicode
DEFFILE = UnInstallW.def
DEFINES = $(DEFINES) -DFARAPI18 -DUNICODE -D_UNICODE
FARBRANCH = 2
!endif

OUTDIR = $(OUTDIR).$(PLATFORM).$(FARBRANCH)
INCLUDES = -I$(FARSDK) -I$(OUTDIR)
CPPFLAGS = $(CPPFLAGS) -Fo$(OUTDIR)\ -Fd$(OUTDIR)\ $(INCLUDES) $(DEFINES)
RCFLAGS = $(RCFLAGS) $(INCLUDES) $(DEFINES)

!ifdef BUILD
!include $(OUTDIR)\far.ini
!endif

OBJS = $(OUTDIR)\UnInstall.obj

LIBS = user32.lib advapi32.lib ole32.lib shell32.lib

project: depfile $(OUTDIR)\far.ini
  $(MAKE) -nologo -$(MAKEFLAGS) build_project BUILD=1

distrib: depfile $(OUTDIR)\far.ini
  $(MAKE) -nologo -$(MAKEFLAGS) build_distrib BUILD=1

build_project: $(OUTDIR)\$(MODULE)$(DLLSUFFIX).dll $(OUTDIR)\UnInstall_Eng.lng $(OUTDIR)\UnInstall_Rus.lng \
               $(OUTDIR)\UnInstall_Eng.hlf $(OUTDIR)\UnInstall_Rus.hlf $(OUTDIR)\File_ID.diz \
               $(OUTDIR)\ReadMe.Rus.txt $(OUTDIR)\WhatsNew.Rus.txt

$(OUTDIR)\$(MODULE)$(DLLSUFFIX).dll: $(OUTDIR)\plugin.def $(OBJS) $(OUTDIR)\UnInstall.res project.ini
  link $(LINKFLAGS) -def:$(OUTDIR)\plugin.def -out:$@ $(OBJS) $(OUTDIR)\UnInstall.res $(LIBS)

.cpp{$(OUTDIR)}.obj::
  $(CPP) $(CPPFLAGS) $<

depfile: $(OUTDIR)
  $(TOOLSDIR)\gendep.exe $(INCLUDES) > $(OUTDIR)\dep.mak

$(OUTDIR)\UnInstall.res: $(OUTDIR)\UnInstall.rc
  $(RC) $(RCFLAGS) -fo$@ $**

PREPROC = $(TOOLSDIR)\preproc $** $@

$(OUTDIR)\UnInstall.rc: project.ini UnInstall.rc
  $(PREPROC)

$(OUTDIR)\plugin.def: project.ini $(DEFFILE)
  $(PREPROC)

$(OUTDIR)\UnInstall_Eng.hlf.1: project.ini UnInstall_Eng.hlf
  $(PREPROC)

$(OUTDIR)\UnInstall_Rus.hlf.1: project.ini UnInstall_Rus.hlf
  $(PREPROC)

$(OUTDIR)\File_ID.diz.1: project.ini File_ID.diz
  $(PREPROC)

$(OUTDIR)\File_ID.diz.2: $(OUTDIR)\far.ini $(OUTDIR)\File_ID.diz.1
  $(PREPROC)

!ifdef OLDFAR
CONVCP = $(TOOLSDIR)\convcp $** $@ 866
!else
CONVCP = copy $** $@
!endif

$(OUTDIR)\UnInstall_Eng.lng: UnInstall_Eng.lng
  $(CONVCP)

$(OUTDIR)\UnInstall_Rus.lng: UnInstall_Rus.lng
  $(CONVCP)

$(OUTDIR)\UnInstall_Eng.hlf: $(OUTDIR)\UnInstall_Eng.hlf.1
  $(CONVCP)

$(OUTDIR)\UnInstall_Rus.hlf: $(OUTDIR)\UnInstall_Rus.hlf.1
  $(CONVCP)

$(OUTDIR)\File_ID.diz: $(OUTDIR)\File_ID.diz.2
  $(CONVCP)

$(OUTDIR)\ReadMe.Rus.txt: ReadMe.Rus.txt
  $(CONVCP)

$(OUTDIR)\WhatsNew.Rus.txt: WhatsNew.Rus.txt
  $(CONVCP)

$(OUTDIR)\far.ini: $(FARSDK)\plugin.hpp
  $(TOOLSDIR)\farver $** $@

$(OUTDIR):
  if not exist $(OUTDIR) mkdir $(OUTDIR)

!ifdef BUILD
!include $(OUTDIR)\dep.mak
!endif


DISTRIB = $(OUTDIR)\$(MODULE)_$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)
!ifndef OLDFAR
DISTRIB = $(DISTRIB)_uni
!endif
DISTRIB_FILES = .\$(OUTDIR)\$(MODULE)$(DLLSUFFIX).dll .\$(OUTDIR)\$(MODULE)$(DLLSUFFIX).map .\$(OUTDIR)\UnInstall_Eng.lng .\$(OUTDIR)\UnInstall_Rus.lng .\$(OUTDIR)\UnInstall_Eng.hlf .\$(OUTDIR)\UnInstall_Rus.hlf .\$(OUTDIR)\File_ID.diz .\$(OUTDIR)\ReadMe.Rus.txt .\$(OUTDIR)\WhatsNew.Rus.txt .\TechInfo.Rus.reg
!if "$(PLATFORM)" != "x86"
DISTRIB = $(DISTRIB)_$(PLATFORM)
!endif
!ifndef RELEASE
DISTRIB = $(DISTRIB)_dbg
DISTRIB_FILES = $(DISTRIB_FILES) .\$(OUTDIR)\$(MODULE)$(DLLSUFFIX).pdb
!endif

build_distrib: $(DISTRIB).7z

$(DISTRIB).7z: $(DISTRIB_FILES) project.ini
  7z a -mx=9 $@ $(DISTRIB_FILES)


clean:
  if exist $(OUTDIR) rd /s /q $(OUTDIR)


.PHONY: project distrib build_project build_distrib depfile clean
.SUFFIXES: .wxs
