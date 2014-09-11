// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <TCHAR.h>

#if !defined(__GNUC__)
	#include <crtdbg.h>
#else
	#define _ASSERTE(f)
#endif

// reference additional headers your program requires here

#include "../../common/plugin.h"
#include "../../common/FarHelper.h"

#include "PanelItem.h"


//#ifdef _UNICODE
//	#define TEXP(fn) fn##W
//	#define F757NA 0,
//	//WARNING: Именно присваивание! Память не освобождать!
//	#define FILENAMEDUP(p,sz) (p).lpwszFileName = _tcsdup(sz);
//	#define FILENAMECPY(p,sz) (p).lpwszFileName = sz;
//	#define FILENAMEPTR(p) (p).lpwszFileName
//#else
//	#define TEXP(fn) fn
//	#define F757NA (LPVOID)
//	#define FILENAMECPY(p,sz) strcpy((p).cFileName, sz);
//	#define FILENAMEDUP FILENAMECPY
//	#define FILENAMEPTR(p) (p).cFileName
//#endif

#define SAFEFREE(ptr) {if ((ptr)!=NULL) { free((ptr)); (ptr)=NULL; }}

extern PluginStartupInfo psi;
extern FarStandardFunctions fsf;

void merror(LPCTSTR asFormat, ...);

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s) 
#define WARNING(s) 
#else
#define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))


#define SHOW_FUNC_WITH_DLL TRUE
#define DUMP_FILE_NAME _T("DUMP.TXT")

#define SZ_DUMP_EXCEPTION _T("Exception while dumping")

#define countof(ar) (sizeof(ar)/sizeof((ar)[0]))
