
#pragma once

#include <windows.h>
#include "PictureViewPlugin.h"
#ifdef _DEBUG
	#ifndef WIN64
		#include <crtdbg.h>
	#else
		#define _ASSERTE(a)
	#endif
#else
	#ifndef _ASSERTE
		#define _ASSERTE(a)
	#endif
#endif

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


extern HMODULE ghModule;

class PVDSettings
{
protected:
	const wchar_t* ms_RegKey;
	HKEY mh_Key, mh_Desc;
public:
	PVDSettings(const wchar_t* asRegKey);
	~PVDSettings();
	void Close();
public:
	static bool FindFile(const wchar_t* asFileOrPath, wchar_t* rsPath, int nMaxLen); //nMaxLen включая '\0'
	static bool GetPluginFolder(wchar_t* rsPath, int nMaxLen); //nMaxLen включая '\\\0'
public:
	void GetStrParam(const wchar_t* asName, const wchar_t* asFormatDescription, const wchar_t* asDefault, wchar_t* rsValue, int nMaxLen); //nMaxLen включая '\0'
	void SetStrParam(const wchar_t* asName, const wchar_t* asFormatDescription, const wchar_t* asValue);
	void GetParam(const wchar_t* asName, const wchar_t* asFormatDescription, DWORD anType, LPCVOID apDefault, LPVOID rpValue, int anSize);
	void SetParam(const wchar_t* asName, const wchar_t* asFormatDescription, DWORD anType, LPCVOID apValue, int anSize);
};

extern const wchar_t* GetVersion(HMODULE hMod);
