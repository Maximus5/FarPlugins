
/*
Copyright (c) 2009-2012 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#define _ASSERT(x)
	#define _ASSERTE(x)
#endif
#include "version.h"
#include "Resolve_Lang.h"

// Heap checking
#if defined(_DEBUG)
	#ifdef MVALIDATE_HEAP
		#define MCHKHEAP _ASSERT(HeapValidate(GetProcessHeap(),0,NULL));
	#else
		#define MCHKHEAP
	#endif
#elif defined(MVALIDATE_HEAP)
	#define MCHKHEAP HeapValidate(GetProcessHeap(),0,NULL);
#else
	#define MCHKHEAP
#endif

#include "ResolveDlg.h"

#include <tchar.h>

#include "../../common/plugin.h"
#include "../../common/FarHelper.h"

#undef _tcslen
#ifdef _UNICODE
#define _tcslen lstrlenW
#else
#define _tcslen lstrlenA
#endif

#if FAR_UNICODE>=1906
// Plugin GUID
GUID guid_PluginGuid = { /* fc5e35f4-02f8-4dc0-9acc-f6b6c962a51c */
    0xfc5e35f4,
    0x02f8,
    0x4dc0,
    {0x9a, 0xcc, 0xf6, 0xb6, 0xc9, 0x62, 0xa5, 0x1c}
  };
GUID guid_ResolveDlg = { /* 7429e6a7-f190-4f56-a974-14560676d851 */
    0x7429e6a7,
    0xf190,
    0x4f56,
    {0xa9, 0x74, 0x14, 0x56, 0x06, 0x76, 0xd8, 0x51}
  };
GUID guid_Msg1 = { /* 9398c13b-fc89-49c0-8dbb-e06429022eb6 */
    0x9398c13b,
    0xfc89,
    0x49c0,
    {0x8d, 0xbb, 0xe0, 0x64, 0x29, 0x02, 0x2e, 0xb6}
  };
GUID guid_Msg2 = { /* 504f5430-caf6-49c0-8019-b3803af2a63d */
    0x504f5430,
    0xcaf6,
    0x49c0,
    {0x80, 0x19, 0xb3, 0x80, 0x3a, 0xf2, 0xa6, 0x3d}
  };
GUID guid_Msg3 = { /* bbabad5b-d2e9-409c-9ba8-fed021f1e41a */
    0xbbabad5b,
    0xd2e9,
    0x409c,
    {0x9b, 0xa8, 0xfe, 0xd0, 0x21, 0xf1, 0xe4, 0x1a}
  };
GUID guid_Msg4 = { /* a03e507c-7422-45a1-b41b-0100f1544be8 */
    0xa03e507c,
    0x7422,
    0x45a1,
    {0xb4, 0x1b, 0x01, 0x00, 0xf1, 0x54, 0x4b, 0xe8}
  };
GUID guid_Msg5 = { /* 41b0d3de-2a99-4585-a05b-07781304ca17 */
    0x41b0d3de,
    0x2a99,
    0x4585,
    {0xa0, 0x5b, 0x07, 0x78, 0x13, 0x04, 0xca, 0x17}
  };
#endif

struct PluginStartupInfo psi;
struct FarStandardFunctions FSF;

/*
TODO: нужен полноценный вывод результата в поля диалога
TODO: настройка путей include/lib/bin
*/

#if defined(__GNUC__)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			break;
	}
	return TRUE;
}
#endif

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo);
  void WINAPI ExitFARW(void);
  int  WINAPI ConfigureW(int ItemNumber);
  HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item);
  int WINAPI GetMinFarVersionW(void);
  void WINAPI GetPluginInfoW(struct PluginInfo *pi);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return DllMain(hDll, dwReason,lpReserved);
}
#endif

#ifndef FAR_UNICODE
wchar_t* gsRootPath = NULL;
#endif
char* gsPath = NULL;		//("MSVCPATH"));
char* gsIncPath = NULL;	//("IncPath"));
char* gsLibPath = NULL;	//("LibPath"));
char* gsIncs = NULL;		//("Includes"));
char* gsLibs = NULL;		//("Libraries"));

struct SettingString
{
	char** Value;
	LPCSTR TitleA;
	LPCWSTR TitleW;
	LPCSTR DefValue;
	LPCSTR DefValue_WOW;
};
SettingString gs[] =
{
	{&gsPath, "MSVCPATH", L"MSVCPATH", DEFAULT_PATH, DEFAULT_PATH_WOW},
	{&gsIncPath, "IncPath", L"IncPath", DEFAULT_INCPATH, DEFAULT_INCPATH_WOW},
	{&gsLibPath, "LibPath", L"LibPath", DEFAULT_LIBPATH, DEFAULT_LIBPATH_WOW},
	{&gsIncs, "Includes", L"Includes", DEFAULT_INCS},
	{&gsLibs, "Libraries", L"Libraries", DEFAULT_LIBS},
};
#define GS_COUNT ARRAYSIZE(gs)


BOOL IsWindows64(BOOL *pbIsWow64Process/* = NULL */)
{
	typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
	BOOL is64bitOs = FALSE, isWow64process = FALSE;
#ifdef _WIN64
	is64bitOs = TRUE; isWow64process = FALSE;
#else
	// Проверяем, где мы запущены
	isWow64process = FALSE;
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	if (hKernel)
	{
		IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

		if (IsWow64Process_f)
		{
			BOOL bWow64 = FALSE;

			if (IsWow64Process_f(GetCurrentProcess(), &bWow64) && bWow64)
			{
				isWow64process = TRUE;
			}
		}
	}

	is64bitOs = isWow64process;
#endif

	if (pbIsWow64Process)
		*pbIsWow64Process = isWow64process;

	return is64bitOs;
}

void LoadSettings()
{
#if FAR_UNICODE>=2460
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_PluginGuid, INVALID_HANDLE_VALUE};
	if (psi.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) != 0)
	{
		for (int i = 0; i < GS_COUNT; i++)
		{
			if (*gs[i].Value) { free(*gs[i].Value); *gs[i].Value = NULL; }

			FarSettingsItem fsi = {0};
			fsi.Name = gs[i].TitleW;
			fsi.Type = FST_STRING;
			if (psi.SettingsControl(sc.Handle, SCTL_GET, 0, &fsi))
			{
				int nLen = lstrlen(fsi.String);
				*gs[i].Value = (char*)calloc(nLen+1,sizeof(**gs[i].Value));
				//TODO: Unicode?
				WideCharToMultiByte(CP_ACP, 0, fsi.String, -1, *gs[i].Value, nLen+1, 0,0);
			}
		}
		psi.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
#else
	HKEY hKey = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, gsRootPath, 0, KEY_READ, &hKey)==ERROR_SUCCESS)
	{
		#define QueryValue(sz,valuename) { \
			if (sz) { free(sz); sz = NULL; } \
			if (!RegQueryValueExA(hKey, valuename, NULL, NULL, NULL, &dw)) { \
				sz = (char*)calloc(dw+2,1); \
				if (RegQueryValueExA(hKey, valuename, NULL, NULL, (LPBYTE)sz, &dw)) { \
					free(sz); sz = NULL; \
				} \
			} \
		}

		DWORD dw;
		
		for (int i = 0; i < GS_COUNT; i++)
		{
			QueryValue(*gs[i].Value,gs[i].TitleA);
		}

		RegCloseKey(hKey); hKey = NULL;
	}
#endif

	BOOL lbWin64 = IsWindows64(NULL);
	char szExpanded[1024];

	for (int i = 0; i < GS_COUNT; i++)
	{
		if (!*gs[i].Value || !**gs[i].Value)
		{
			if (*gs[i].Value) free(*gs[i].Value);
			ExpandEnvironmentStringsA((lbWin64&&gs[i].DefValue_WOW)?gs[i].DefValue_WOW:gs[i].DefValue, szExpanded, ARRAYSIZE(szExpanded));
			_ASSERTE(szExpanded[0] != 0);
			*gs[i].Value = _strdup(szExpanded);
		}
	}
}

void SaveSettings()
{
#if FAR_UNICODE>=2460
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_PluginGuid, INVALID_HANDLE_VALUE};
	if (psi.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) != 0)
	{
		for (int i = 0; i < GS_COUNT; i++)
		{
			FarSettingsItem fsi = {0};
			fsi.Name = gs[i].TitleW;
			fsi.Type = FST_STRING;
			int nLen = *gs[i].Value ? lstrlenA(*gs[i].Value) : 0;
			wchar_t* psz = (wchar_t*)calloc(nLen+1,sizeof(*psz));
			//TODO: Unicode?
			MultiByteToWideChar(CP_ACP, 0, *gs[i].Value ? *gs[i].Value : "", -1, psz, nLen+1);
			fsi.String = psz;
			psi.SettingsControl(sc.Handle, SCTL_SET, 0, &fsi);
			free(psz);
		}
		psi.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
#else
	HKEY hKey = NULL;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, gsRootPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL)==ERROR_SUCCESS)
	{
		DWORD dw;
		
		for (int i = 0; i < GS_COUNT; i++)
		{
			if (*gs[i].Value)
			{
				dw = lstrlenA(*gs[i].Value)+1;
				RegSetValueExA(hKey, gs[i].TitleA, 0, REG_SZ, (LPBYTE)*gs[i].Value, dw);
			}
		}

		RegCloseKey(hKey); hKey = NULL;
	}
#endif
}

#if FAR_UNICODE>=2460
void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	_ASSERTE(Info->StructSize == sizeof(GlobalInfo));
	
	Info->MinFarVersion = FARMANAGERVERSION;
	
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,MVV_4,VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"Resolve";
	Info->Description = L"Resolve plugin";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}
#endif

// minimal(?) FAR version
int WINAPI GetMinFarVersionW(void)
{
#if FAR_UNICODE>=2460
	#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
	#define FARMANAGERVERSION2 MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD)
#else
	#define FARMANAGERVERSION2 FARMANAGERVERSION
#endif

	return FARMANAGERVERSION2;
}

LPCWSTR GetMsg(int aiMsg)
{
	if (!psi.GetMsg)
		return L"";

	return psi.GetMsg(PluginNumber,aiMsg);
}

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
    //static TCHAR *szMenu[1];
    //szMenu[0]=_T("Resolve");
	//pi->StructSize = sizeof(struct PluginInfo);
	//pi->Flags = PF_EDITOR | PF_VIEWER;
	//pi->PluginMenuStrings = szMenu;
	//pi->PluginMenuStringsNumber = 1;
	//pi->PluginConfigStrings = szMenu;
	//pi->PluginConfigStringsNumber = 1;
	//pi->CommandPrefix = NULL;
	//pi->Reserved = 0;	

	#if FAR_UNICODE>=1906
	_ASSERTE(pi->StructSize >= sizeof(struct PluginInfo));
	#else
	pi->StructSize = sizeof(struct PluginInfo);
	#endif

	pi->Flags = PF_EDITOR | PF_VIEWER;

	static TCHAR szMenu[MAX_PATH];
	lstrcpy(szMenu, GetMsg(RSPluginName));
    static TCHAR *pszMenu[1];
    pszMenu[0] = szMenu;
	
	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_PluginGuid;
		pi->PluginMenu.Strings = pszMenu;
		pi->PluginMenu.Count = 1;
	#else
		pi->PluginMenuStrings = pszMenu;
		pi->PluginMenuStringsNumber = 1;
	#endif
	
	#if FAR_UNICODE>=1906
		pi->PluginConfig.Guids = &guid_PluginGuid;
		pi->PluginConfig.Strings = pszMenu;
		pi->PluginConfig.Count = 1;
	#else
		pi->PluginConfigStrings = pszMenu;
		pi->PluginConfigStringsNumber = 1;
	#endif
}

void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo)
{
	::psi = *aInfo;
	::FSF = *aInfo->FSF;
	::psi.FSF = &::FSF;

#if !defined(FAR_UNICODE)
	int lRootLen = lstrlen(aInfo->RootKey);
	gsRootPath = (wchar_t*)calloc(lRootLen+32,2);
	lstrcpy(gsRootPath, aInfo->RootKey);
	if (gsRootPath[lRootLen-1] != _T('\\'))
		gsRootPath[lRootLen++] = _T('\\');
	lstrcpy(gsRootPath+lRootLen, _T("CPPResolve"));
#endif
}

#if FAR_UNICODE>=2460
#define ExitArg void*
#else
#define ExitArg void
#endif

void WINAPI ExitFARW(ExitArg)
{
}

void WritePath(HANDLE hFile, LPCSTR pszList)
{
	LPCSTR pszColon;
	DWORD dw;
	while (*pszList == ';') pszList++;
	while (*pszList)
	{
		pszColon = strchr(pszList, ';');
		if (!pszColon) pszColon = pszList + lstrlenA(pszList);
		WriteFile(hFile, pszList, (DWORD)(pszColon-pszList), &dw, NULL);
		WriteFile(hFile, "\r\n", 2, &dw, NULL);
		while (*pszColon == ';') pszColon++;
		pszList = pszColon;
	}
}

void ParseStringSettings(char* pszData)
{
	char *pszSection, *pszSectionEnd;
	char *pszLine, *pszDest, *pszNextLine;
	SettingString* ps = NULL;
	bool lbIsPath;
	while (pszData && *pszData)
	{
		//while (*pszData == ' ' || *pszData == '\t' || *pszData == '\r' || *pszData == '\n')
		//	pszData++;
		pszSection = strchr(pszData, '[');
		if (!pszSection)
			break;
		pszSection++;
		pszSectionEnd = strchr(pszSection, ']');
		if (!pszSectionEnd)
			break;
		*pszSectionEnd = 0; ps = NULL;
		for (int i = 0; i < GS_COUNT; i++)
		{
			if (lstrcmpiA(pszSection, gs[i].TitleA) == 0)
			{
				ps = gs+i;
				lbIsPath = (i < 3); // замена '\r\n' на ';'
				break;
			}
		}
		if (!ps)
		{
			_ASSERTE(ps!=NULL);
			break;
		}

		pszLine = pszSectionEnd+1;
		while (*pszLine == ' ' || *pszLine == '\t' || *pszLine == '\r' || *pszLine == '\n')
			pszLine++;
		*ps->Value = (char*)calloc(lstrlenA(pszLine)+1,1);
		pszDest = *ps->Value;
		if (lbIsPath)
		{
			while (*pszLine)
			{
				if (*pszDest) lstrcatA(pszDest, ";");
				pszNextLine = strpbrk(pszLine, "\r\n");
				if (pszNextLine) *pszNextLine = 0;
				lstrcatA(pszDest, pszLine);
				pszLine = pszNextLine+1;
				while (*pszLine == ' ' || *pszLine == '\t' || *pszLine == '\r' || *pszLine == '\n')
					pszLine++;
				if (*pszLine == '[')
					break;
			}
			pszData = pszLine;
		}
		else
		{
			pszSection = strstr(pszLine, "\n[");
			if (!pszSection)
				lstrcpyA(pszDest, pszLine);
			else
				memmove(pszDest, pszLine, pszSection-pszLine+1);
			pszLine = pszDest + lstrlenA(pszDest);
			while (pszLine >= pszDest && (*(pszLine-1) == '\r' || *(pszLine-1) == '\n'))
			{
				pszLine--; *pszLine = 0;
			}

			// Next
			if (!pszSection)
				break;
			pszData = pszSection+1;
		}
	}
}

int    WINAPI ConfigureW(int ItemNumber)
{
	LoadSettings();
	/*
	QueryValue(gsPath,("MSVCPATH"));
	QueryValue(gsIncPath,("IncPath"));
	QueryValue(gsLibPath,("LibPath"));
	QueryValue(gsIncs,("Includes"));
	QueryValue(gsLibs,("Libraries"));
	*/

	wchar_t szTempPath[MAX_PATH*2];
	FSF.MkTemp(szTempPath,
		#ifdef _UNICODE
			MAX_PATH*2,
		#endif
		_T("CRSV"));

	HANDLE hFile = CreateFile(szTempPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		LPCWSTR lsLines[3];
		lsLines[0] = GetMsg(RSPluginName);
		lsLines[1] = GetMsg(RSCantCreateTemp);
		lsLines[2] = szTempPath;
		psi.Message(_PluginNumber(guid_Msg1), FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK, NULL, lsLines, 3, 0);
		return 0;
	}
	DWORD dw;
	for (int i = 0; i < GS_COUNT; i++)
	{
		WriteFile(hFile, "[", 1, &dw, NULL);
		WriteFile(hFile, gs[i].TitleA, lstrlenA(gs[i].TitleA), &dw, NULL);
		WriteFile(hFile, "]\r\n", 3, &dw, NULL);
		if (i < 3)
			WritePath(hFile, *gs[i].Value);
		else
			WriteFile(hFile, *gs[i].Value, lstrlenA(*gs[i].Value), &dw, NULL);
		WriteFile(hFile, "\r\n\r\n", 4, &dw, NULL);
	}
	CloseHandle(hFile);

	int nEdtRc = psi.Editor(szTempPath, NULL, 0,0,-1,-1, EF_DISABLEHISTORY,
		#ifdef _UNICODE
		1, 0, 1251
		#else
		0, 0
		#endif
	);
	
	if (nEdtRc == EEC_MODIFIED)
	{
		hFile = CreateFile(szTempPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			LPCWSTR lsLines[3];
			lsLines[0] = GetMsg(RSPluginName);
			lsLines[1] = GetMsg(RSCantOpenTemp);
			lsLines[2] = szTempPath;
			psi.Message(_PluginNumber(guid_Msg2), FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK, NULL, lsLines, 3, 0);
			return 0;
		}
		for (int i = 0; i < GS_COUNT; i++)
		{
			if (*gs[i].Value) { free(*gs[i].Value); *gs[i].Value = NULL; }
		}
		DWORD nLen = GetFileSize(hFile, NULL);
		if (nLen == 0)
		{
			for (int i = 0; i < GS_COUNT; i++)
			{
				if (*gs[i].Value) free(*gs[i].Value);
				*gs[i].Value = _strdup(gs[i].DefValue);
			}
		}
		else
		{
			char* pszData = (char*)calloc(nLen+2,1);
			ReadFile(hFile, pszData, nLen, &nLen, NULL);
			// Парсинг
			ParseStringSettings(pszData);
			free(pszData);
		}
		CloseHandle(hFile);
		for (int i = 0; i < GS_COUNT; i++)
		{
			if (!*gs[i].Value || !**gs[i].Value)
			{
				if (*gs[i].Value) free(*gs[i].Value);
				*gs[i].Value = _strdup(gs[i].DefValue);
			}
		}

		SaveSettings();
	}

	DeleteFile(szTempPath);
	
	return 0;
}

FARDLGRET WINAPI ResolveDlgProc(
  HANDLE   hDlg,
  FARINT   Msg,
  FARINT   Param1,
  FARDLGPARM Param2
);

CResolveDlg* pResolve = NULL;

wchar_t* SaveEnvVar(LPCWSTR asVarName)
{
	int nEnvLen = 32767; DWORD dwErr;
	wchar_t *pszSave = (wchar_t*)malloc(nEnvLen*2); pszSave[0] = 0;
	if (!GetEnvironmentVariableW(asVarName, pszSave, nEnvLen))
	{
		dwErr = GetLastError();
		if (dwErr == ERROR_ENVVAR_NOT_FOUND)
		{
			free(pszSave); pszSave = NULL;
		}
	}
	return pszSave;
}

#define ExprId 2
#define SourceId 4
#define ResolveBtnId 5
#define ErrLookBtnId 6
#define TypeId 9
#define ValueId 11
#define Copy1BtnId 12 // Value only
#define Copy2BtnId 13 // <type> <name> = <value>
#define Copy3BtnId 14 // value/*name*/
#if FAR_UNICODE>=2460
HANDLE WINAPI OpenW(const struct OpenInfo *Info)
#else
HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item)
#endif
{
	LoadSettings();

#if FAR_UNICODE>=2460
	FarDialogItem dialog[] = {
	{ DI_DOUBLEBOX,    3,  1, 53, 11, {0}, 0,0,0, GetMsg(RSDlgTitle) },
	{ DI_TEXT,         5,  3,  0,  0, {0}, 0,0,0, GetMsg(RSExprLabel) },
	{ DI_EDIT,        13,  3, 51,  0, {0}, L"resolve_expr", 0, DIF_HISTORY},
	{ DI_TEXT,         5,  4,  0,  0, {0}, 0,0,0, GetMsg(RSSourceLabel) },
	{ DI_EDIT,        13,  4, 51,  0, {0}, L"resolve_source", 0, DIF_HISTORY|DIF_USELASTHISTORY/*, L"ntdll.dll"*/},
	{ DI_BUTTON,       27, 5,  0,  0, {0}, 0,0,0, GetMsg(RSResolveBtn) },
	{ DI_BUTTON,       40, 5,  0,  0, {0}, 0,0,0, GetMsg(RSLookupBtn) },
	//
	{ DI_TEXT,         5,  6,  0,  0, {0}, 0,0, DIF_SEPARATOR},
	//
	{ DI_TEXT,         5,  8,  0,  0, {0}, 0,0,0, GetMsg(RSTypeLabel) },
	{ DI_EDIT,        13,  8, 51,  0, {0}, 0,0, DIF_READONLY, L""},
	{ DI_TEXT,         5,  9,  0,  0, {0}, 0,0,0, GetMsg(RSValueLabel) },
	{ DI_EDIT,        13,  9, 51,  0, {0}, 0,0, DIF_READONLY, L""},
	{ DI_BUTTON,       13,10,  0,  0, {0}, 0,0,0, GetMsg(RSCopy1Btn) },
	{ DI_BUTTON,       25,10,  0,  0, {0}, 0,0,0, GetMsg(RSCopy2Btn) },
	{ DI_BUTTON,       39,10,  0,  0, {0}, 0,0,0, GetMsg(RSCopy3Btn) }
	}; //type,x1,y1,x2,y2,{Sel},History,Mask,Flags,Data,MaxLength,UserData
#else
	FarDialogItem dialog[] = {
	{ DI_DOUBLEBOX,    3,  1, 53, 11, 0, {0}, 0, 0, GetMsg(RSDlgTitle) },
	{ DI_TEXT,         5,  3,  0,  0, 0, {0}, 0, 0, GetMsg(RSExprLabel) },
	{ DI_EDIT,        13,  3, 51,  0, 1, {(INT_PTR)L"resolve_expr"}, DIF_HISTORY, 0, 0 },
	{ DI_TEXT,         5,  4,  0,  0, 0, {0}, 0, 0, GetMsg(RSSourceLabel) },
	{ DI_EDIT,        13,  4, 51,  0, 1, {(INT_PTR)L"resolve_source"}, DIF_HISTORY|DIF_USELASTHISTORY/*, 0, L"ntdll.dll"*/},
	{ DI_BUTTON,       27, 5,  0,  0, 0, {0}, 0, 0, GetMsg(RSResolveBtn) },
	{ DI_BUTTON,       40, 5,  0,  0, 0, {0}, 0, 0, GetMsg(RSLookupBtn) },
	//
	{ DI_TEXT,         5,  6,  0,  0, 0, {0}, DIF_SEPARATOR, 0, L"" },
	//
	{ DI_TEXT,         5,  8,  0,  0, 0, {0}, 0, 0, GetMsg(RSTypeLabel) },
	{ DI_EDIT,        13,  8, 51,  0, 0, {0}, DIF_DISABLE, 0, L"" },
	{ DI_TEXT,         5,  9,  0,  0, 0, {0}, 0, 0, GetMsg(RSValueLabel) },
	{ DI_EDIT,        13,  9, 51,  0, 0, {0}, DIF_DISABLE, 0, L"" },
	{ DI_BUTTON,       13,10,  0,  0, 0, {0}, 0, 0, GetMsg(RSCopy1Btn) },
	{ DI_BUTTON,       25,10,  0,  0, 0, {0}, 0, 0, GetMsg(RSCopy2Btn) },
	{ DI_BUTTON,       39,10,  0,  0, 0, {0}, 0, 0, GetMsg(RSCopy3Btn) }
	}; //type,x1,y1,x2,y2,Focus,{Sel},Flags,Def,Data
#endif

	wchar_t *pszSavePath = SaveEnvVar(L"PATH");
	wchar_t *pszSaveInc  = SaveEnvVar(L"include");
	wchar_t *pszSaveLib  = SaveEnvVar(L"lib");

	pResolve = new CResolveDlg();

#if FAR_UNICODE>=2460
	HANDLE hDlg = psi.DialogInit(&guid_PluginGuid, &guid_ResolveDlg, -1, -1, 57, 13, NULL, dialog,
              sizeof(dialog)/sizeof(dialog[0]), 0, 0, ResolveDlgProc, 0);
#else
	HANDLE hDlg = psi.DialogInit(psi.ModuleNumber, -1, -1, 57, 13, NULL, dialog,
              sizeof(dialog)/sizeof(dialog[0]), 0, 0, ResolveDlgProc, 0);
#endif
    psi.DialogRun(hDlg);
    psi.DialogFree(hDlg);

	delete pResolve; pResolve = NULL;

	if (pszSavePath)
	{
		SetEnvironmentVariableW(L"PATH", pszSavePath); free(pszSavePath);
	}
	if (pszSaveInc)
	{
		SetEnvironmentVariableW(L"include", pszSaveInc); free(pszSaveInc);
	}
	if (pszSaveLib)
	{
		SetEnvironmentVariableW(L"lib", pszSaveLib); free(pszSaveLib);
	}
	
#if FAR_UNICODE>=2573
	return FALSE;
#else
	return INVALID_HANDLE_VALUE;
#endif
}

#include "FacList.h"
#include "InetErrNames.h"
#include "WinErrorNames.h"
#include "NtStatusNames.h"
#include "WiCErrorNames.h"

void LookupErrName(DWORD nErr, wchar_t (&szMnemo)[96], LPCWSTR asModule)
{
	szMnemo[0] = 0;
	
	if (lstrcmpiW(asModule, L"wininet.dll")==0 || lstrcmpiW(asModule, L"wininet")==0)
	{
		for (size_t i = 0; i < ARRAYSIZE(InetErr); i++)
		{
			if (InetErr[i].nErr == nErr)
			{
				//lstrcpynW(szMnemo, InetErr[i].sErr, ARRAYSIZE(szMnemo));
				MultiByteToWideChar(CP_ACP, 0, InetErr[i].sErr, -1, szMnemo, ARRAYSIZE(szMnemo));
				return;
			}
		}
	}
	
	// Попробовать поискать в ntstatus.h и WinError.h
	if (lstrcmpiW(asModule, L"ntdll.dll")==0 || lstrcmpiW(asModule, L"ntdll")==0)
	{
		for (size_t i = 0; i < ARRAYSIZE(NtErr); i++)
		{
			if (NtErr[i].nErr == nErr)
			{
				//lstrcpynW(szMnemo, NtErr[i].sErr, ARRAYSIZE(szMnemo));
				MultiByteToWideChar(CP_ACP, 0, NtErr[i].sErr, -1, szMnemo, ARRAYSIZE(szMnemo));
				return;
			}
		}
	}
	
	for (size_t i = 0; i < ARRAYSIZE(WinErr); i++)
	{
		if (WinErr[i].nErr == nErr)
		{
			//lstrcpynW(szMnemo, WinErr[i].sErr, ARRAYSIZE(szMnemo));
			MultiByteToWideChar(CP_ACP, 0, WinErr[i].sErr, -1, szMnemo, ARRAYSIZE(szMnemo));
			return;
		}
	}

	for (size_t i = 0; i < ARRAYSIZE(WiCErr); i++)
	{
		if (WiCErr[i].nErr == nErr)
		{
			//lstrcpynW(szMnemo, WinErr[i].sErr, ARRAYSIZE(szMnemo));
			MultiByteToWideChar(CP_ACP, 0, WiCErr[i].sErr, -1, szMnemo, ARRAYSIZE(szMnemo));
			return;
		}
	}

	// Last chance. Win32 error in STATUS or HRESULT?
	DWORD nSev = /*HRESULT_SEVERITY(nVal)*/ ((nErr >> 30) & 0x3);
	DWORD nFac = HRESULT_FACILITY(nErr);
	if (/*(nSev == STATUS_SEVERITY_WARNING) &&*/ (nFac == FACILITY_WIN32))
	{
		nErr = LOWORD(nErr);

		for (size_t i = 0; i < ARRAYSIZE(WinErr); i++)
		{
			if (WinErr[i].nErr == nErr)
			{
				//lstrcpynW(szMnemo, WinErr[i].sErr, ARRAYSIZE(szMnemo));
				MultiByteToWideChar(CP_ACP, 0, WinErr[i].sErr, -1, szMnemo, ARRAYSIZE(szMnemo));
				return;
			}
		}
	}
}

void DoErrLookup(HANDLE hDlg)
{
	MCHKHEAP;

	INT_PTR nLen = 0;
	//_asm int 3;
	nLen = DlgGetTextLength(hDlg, ExprId);
	if (nLen>0)
	{
		FarDialogItemData fdid = {};
		#if FAR_UNICODE>=2573
		fdid.StructSize = sizeof(fdid);
		#endif
		fdid.PtrLength = nLen+1;
		fdid.PtrData = (wchar_t*)calloc(nLen+1,2);
		
		psi.SendDlgMessage(hDlg, DM_GETTEXT, ExprId, (FARDLGPARM)&fdid);
		MCHKHEAP;

		wchar_t* pszSrc = DlgGetText(hDlg, SourceId);
		HMODULE hSource = NULL;
		if (pszSrc)
		{
			hSource = LoadLibrary(pszSrc);
		}

		DWORD nVal = 0;
		WCHAR *psz = fdid.PtrData;
		WCHAR *endptr=NULL;
		while (*psz==L' ' || *psz==L'\t') psz++;
		if (psz[0] == L'0' && (psz[1] == L'x' || psz[1] == L'X'))
			nVal = wcstoul(psz+2, &endptr, 16);
		else
			nVal = wcstoul(psz, &endptr, 10);



		WCHAR* pszFull = NULL;
		WCHAR* lpMsgBuf=NULL;
		DWORD nFormatRc = 0;
		HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
		bool lbNtDll = (hSource && (hSource == hNtDll));
		LPCWSTR pszModule = lbNtDll ? L"ntdll.dll" : pszSrc ? FSF.PointToName(pszSrc) : NULL;
		DWORD nLookupErr = nVal;

		if (!nFormatRc)
		{
			nFormatRc = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				(hSource ? FORMAT_MESSAGE_FROM_HMODULE : 0) |
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS, 
				hSource, nVal, 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
		}
		//A.I. Для WinInet.dll код ошибки содержится в младших 16-и битах
		if (!nFormatRc && (HIWORD(nVal)==0x8007))
		{
			nFormatRc = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				(hSource ? FORMAT_MESSAGE_FROM_HMODULE : 0) |
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS, 
				hSource, LOWORD(nVal), 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
			if (nFormatRc)
				nLookupErr = LOWORD(nVal);
		}
		//A.I. 0xC000xxxx - это скорее всего ошибка из NTSTATUS (ntdll.dll)
		if (!nFormatRc && (HIWORD(nVal)==0xC000 || HIWORD(nVal)==0x8000 || HIWORD(nVal)==0x4000))
		{
			nFormatRc = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS, 
				hNtDll, nVal, 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
			if (nFormatRc)
			{
				lbNtDll = true;
				pszModule = L"ntdll.dll";
			}
		}


		wchar_t szTitle[512], szDec[64], szMnemo[96];
		LookupErrName(nLookupErr, szMnemo, pszModule);
		DWORD nSev = /*HRESULT_SEVERITY(nVal)*/ ((nVal >> 30) & 0x3);
		DWORD nFac = HRESULT_FACILITY(nVal);
		
		wsprintf(szDec, nSev ? GetMsg(RSFmtDecWord)/*"Dec=%i, Word=%u"*/ : GetMsg(RSFmtDec)/*L"Dec=%i"*/, (int)nVal, LOWORD(nVal));
		
		if (*szMnemo)
			wsprintf(szTitle, GetMsg(RSFmtNameErr)/*L"Resolve\nName=%s\nErr=0x%08X, %s\n"*/, szMnemo, nVal, szDec);
		else
			wsprintf(szTitle, GetMsg(RSFmtErr)/*L"Resolve\nErr=0x%08X, %s\n"*/, nVal, szDec);
			
		if (nSev || nFac)
		{
			// 2 бита на Severity не только у NTSTATUS но и у WINERROR
			// 1 бит заявлен только для HRESULT, но и то... Microsoft путается в показаниях...
			//if (lbNtDll)
			//{
			switch(nSev)
			{
			case 0: lstrcatW(szTitle, L"STATUS_SEVERITY_SUCCESS, "); break;
			case 1: lstrcatW(szTitle, L"STATUS_SEVERITY_INFORMATIONAL, "); break;
			case 2: lstrcatW(szTitle, L"STATUS_SEVERITY_WARNING, "); break;
			default: lstrcatW(szTitle, L"SEVERITY_ERROR, ");
			}
			//}
			//else
			//{
			//	lstrcatW(szTitle, nSev ? L"SEVERITY_ERROR, " : L"SEVERITY_SUCCESS, ");
			//}
			
			bool bFound = false;
			for (size_t i = 0; i < ARRAYSIZE(Facs); i++)
			{
				if (Facs[i].nFac == nFac)
				{
					bFound = true;
					lstrcatW(szTitle, Facs[i].sFac);
					lstrcatW(szTitle, L"\n");
					break;
				}
			}
			
			if (!bFound)
			{
				wsprintfW(szTitle+lstrlenW(szTitle), GetMsg(RSFmtFacility)/*L"Facility=%i\n"*/, nFac);
			}
		}


		if (nFormatRc)
		{
			while ((pszFull = wcsstr(lpMsgBuf, L"\r\n")) != NULL)
			{
				pszFull[0] = L' '; pszFull[1] = L'\n';
			}

			pszFull = (WCHAR*)calloc(64+lstrlen(lpMsgBuf)+lstrlen(szTitle),2);
			wsprintf(pszFull, L"%s\n%s\nOK", szTitle, (WCHAR*)lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
		else
		{
			pszFull = (WCHAR*)calloc(lstrlen(szTitle)+128,2);
			wsprintf(pszFull, L"%s\n%s\nOK", szTitle, *szMnemo ? szMnemo : GetMsg(RSCantGetDescr)/*L"Can't get description"*/);
		}
		MCHKHEAP;

		psi.Message(_PluginNumber(guid_Msg3), FMSG_ALLINONE|FMSG_WARNING|FMSG_LEFTALIGN, NULL, 
		(const wchar_t * const *)pszFull, 0, 1);

		free(fdid.PtrData);
		free(pszFull);
		if (pszSrc)
			free(pszSrc);
		if (hSource)
			FreeLibrary(hSource);
		MCHKHEAP;
	}
	else
	{
		// Не введен текст
	}

	psi.SendDlgMessage(hDlg, DM_SETFOCUS, ExprId, 0);
	if (nLen>0)
	{
		EditorSelect ei = {BTYPE_STREAM,0,0,nLen,1};
		psi.SendDlgMessage(hDlg, DM_SETSELECTION, ExprId, (FARDLGPARM)&ei);
	}
}

void DoResolve(HANDLE hDlg)
{
	INT_PTR nLen = 0;
	nLen = DlgGetTextLength(hDlg, ExprId);
	if (nLen>0 && nLen<1000)
	{
		MCHKHEAP;
		WCHAR szText[1024];
		FarDialogItemData fdid;
		#if FAR_UNICODE>=2573
		fdid.StructSize = sizeof(fdid);
		#endif
		fdid.PtrLength = nLen+1;
		fdid.PtrData = szText;

		psi.SendDlgMessage(hDlg, DM_GETTEXT, ExprId, (FARDLGPARM)&fdid);
		psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, TypeId, (FARDLGPARM)_T(""));
		psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, ValueId, (FARDLGPARM)_T(""));
		
		
		//MessageBoxW(GetForegroundWindow(), fdid.PtrData, L"Test", 0);

		LPTSTR lsResult=NULL;
		LPCTSTR lsType=NULL, lsValue=NULL;
		#ifdef _UNICODE
		char szAnsi[1001];
		WideCharToMultiByte(CP_ACP, 0, szText, -1, szAnsi, 1001, 0,0);
		#endif

		psi.SendDlgMessage(hDlg, DM_SHOWDIALOG, FALSE, 0);

		HANDLE hScr = psi.SaveScreen(0,0,-1,-1);
		psiControl(INVALID_HANDLE_VALUE, FCTL_GETUSERSCREEN, 0, NULL);

		TCHAR TempDir[MAX_PATH];
		FSF.MkTemp(TempDir,ARRAYSIZE(TempDir),L"Rsl");
		MCHKHEAP;

		BOOL lbRc = pResolve->OnResolve(
			TempDir,
			#ifdef _UNICODE
			szAnsi
			#else
			szText
			#endif
		, &lsResult, &lsType, &lsValue);

		psiControl(INVALID_HANDLE_VALUE, FCTL_SETUSERSCREEN, 0, NULL);
		psi.RestoreScreen(hScr);

		psi.SendDlgMessage(hDlg, DM_SHOWDIALOG, TRUE, 0);
		MCHKHEAP;

		if (lbRc)
		{
			psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, TypeId, (FARDLGPARM)lsType);
			psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, ValueId, (FARDLGPARM)lsValue);
		}
		else if (lsResult)
		{
			psi.Message(_PluginNumber(guid_Msg3), FMSG_ALLINONE|(lbRc ? 0 : FMSG_WARNING)|FMSG_LEFTALIGN|FMSG_MB_OK, NULL, 
				(const TCHAR * const *)lsResult, 0, 0);
		}
		MCHKHEAP;			
	}
}

FARDLGRET WINAPI ResolveDlgProc(
  HANDLE   hDlg,
  FARINT   Msg,
  FARINT   Param1,
  FARDLGPARM Param2
)
{
	if (Msg == DN_CLOSE && Param1!=-1)
	{
		INT_PTR nLen = 0;
		nLen = DlgGetTextLength(hDlg, ExprId);
		if (nLen>0)
		{
			WCHAR szText[1024];
			FarDialogItemData fdid;
			#if FAR_UNICODE>=2573
			fdid.StructSize = sizeof(fdid);
			#endif
			fdid.PtrLength = nLen+1;
			fdid.PtrData = szText;
			
			psi.SendDlgMessage(hDlg, DM_GETTEXT, ExprId, (FARDLGPARM)&fdid);
			
			DWORD nVal = 0;
			WCHAR *psz = fdid.PtrData;
			WCHAR *endptr=NULL;
			while (*psz==L' ' || *psz==L'\t') psz++;
			if (*psz)
			{
				if (psz[0] == L'0' && (psz[1] == L'x' || psz[1] == L'X'))
				{
					psz += 2;
					nVal = wcstoul(psz, &endptr, 16);
				}
				else
				{
					nVal = wcstoul(psz, &endptr, 10);
				}
				if (endptr && endptr!=psz)
					DoErrLookup(hDlg);
				else
					DoResolve(hDlg);
				MCHKHEAP;
			}
		}
		return FALSE;
	}
	else if (Msg == DN_BTNCLICK)
	{
		switch (Param1)
		{
		case ResolveBtnId:
			{
				DoResolve(hDlg);
				MCHKHEAP;
			}
			break;
		case ErrLookBtnId:
			{
				DoErrLookup(hDlg);
				MCHKHEAP;
			}
			break;
		case Copy1BtnId:
		case Copy2BtnId:
		case Copy3BtnId:
			{
				MCHKHEAP;
				TCHAR *pszName = DlgGetText(hDlg, ExprId);
				TCHAR *pszType = DlgGetText(hDlg, TypeId);
				TCHAR *pszVal  = DlgGetText(hDlg, ValueId);
				INT_PTR nName =  pszName ? _tcslen(pszName) : 0;
				INT_PTR nType = pszType ? _tcslen(pszType) : 0;
				INT_PTR nVal  = pszVal ? _tcslen(pszVal) : 0;

				psi.SendDlgMessage(hDlg, DM_SETFOCUS, ExprId, 0);

				size_t nMaxCch = nName+nType+nVal+32;
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, nMaxCch*sizeof(TCHAR));
				TCHAR *pszAll = (TCHAR*)GlobalLock(hMem);
				LPCTSTR pszFmt = NULL;

				switch (Param1)
				{
				case Copy1BtnId: // Value only
					//lstrcpy(pszAll, pszVal);
					pszFmt = GetMsg(RSCopy1Fmt);
					break;
				case Copy2BtnId: // <type> <name> = <value>
					//wsprintf(pszAll, _T("%s %s = %s"), pszType, pszName, pszVal);
					pszFmt = GetMsg(RSCopy2Fmt);
					break;
				case Copy3BtnId: // value/*name*/
					//wsprintf(pszAll, _T("%s/*%s*/"), pszVal, pszName);
					pszFmt = GetMsg(RSCopy3Fmt);
					break;
				}

				if (pszFmt)
				{
					TCHAR* p = pszAll;
					TCHAR* pEnd = (pszAll + nMaxCch - 1);
					while (*pszFmt && (p < pEnd))
					{
                		if (*pszFmt == L'%')
                		{
                			pszFmt++;
                			switch (*pszFmt)
                			{
                			case _T('t'):
                				lstrcpyn(p, pszType ? pszType : _T(""), pEnd-p);
                				break;
                			case _T('n'):
                				lstrcpyn(p, pszName ? pszName : _T(""), pEnd-p);
                				break;
                			case _T('v'):
                				lstrcpyn(p, pszVal ? pszVal : _T(""), pEnd-p);
                				break;
							default:
								*p = *pszFmt;
                			}
                			p += lstrlen(p);
                			pszFmt++;
                		}
                		else
                		{
                			*(p++) = *(pszFmt++);
                		}
					}
				}
				else
				{
					psi.Message(_PluginNumber(guid_Msg5), FMSG_ALLINONE|FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK, NULL, 
						(const TCHAR * const *)GetMsg(RSClipFormatFail), 0, 0);
				}

				GlobalUnlock(hMem);
				if (OpenClipboard(NULL))
				{
					EmptyClipboard();
					SetClipboardData(
						#ifdef _UNICODE
						CF_UNICODETEXT
						#else
						CF_TEXT
						#endif
						, hMem);
					CloseClipboard();
				}
				else
				{
					psi.Message(_PluginNumber(guid_Msg5), FMSG_ALLINONE|FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK, NULL, 
						(const TCHAR * const *)GetMsg(RSClipboardFailed), 0, 0);
					GlobalFree(hMem);
				}

				if (pszName) free(pszName);
				if (pszType) free(pszType);
				if (pszVal) free(pszVal);
				MCHKHEAP;
			}
			break;
		}
		return TRUE;
	}

    return psi.DefDlgProc(hDlg,Msg,Param1,Param2);
	//return FALSE;
}
