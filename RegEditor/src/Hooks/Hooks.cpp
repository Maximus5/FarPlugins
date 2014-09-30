
//#define SHOW_BOX_ON_LOAD

// Minimum version - Win2000
//#undef WINVER
//#define WINVER 0x0500
//#undef _WIN32_WINNT
//#define _WIN32_WINNT 0x0500


#include <Windows.h>
#include <Tlhelp32.h>
#include <commctrl.h>
#include <tchar.h>
#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#define _ASSERT(x)
	#define _ASSERTE(x)
#endif

#define REGEDIT_INPROCESS
#include "RegEditExe.h"




HINSTANCE g_hInstance = NULL;
BOOL gbInitialized = FALSE;
BOOL gbServerStarted = FALSE;

int SelectServerInit();
int SelectServerDone();

void ShowError(LPCSTR asMessage, LPCSTR asArg = NULL)
{
	char szErrInfo[1024]; DWORD nErrorCode = GetLastError();
	if (asArg)
		wsprintfA(szErrInfo, asMessage, asArg, nErrorCode);
	else
		wsprintfA(szErrInfo, "%s\nErrorCode = 0x%08X", asMessage, nErrorCode);
	MessageBoxA(NULL, szErrInfo, "RegEditorH", MB_OK|MB_SETFOREGROUND|MB_ICONSTOP);
}

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:

			g_hInstance = (HINSTANCE)hModule;

			if (!gbInitialized)
			{
				#ifdef SHOW_BOX_ON_LOAD
					MessageBoxW(NULL, L"RegEditorH.DllMain", L"Loaded", MB_OK|MB_SETFOREGROUND);
				#endif
				gbInitialized = TRUE;
				
				// Смотрим, в каком процессе мы запустились: RunDll32 / RegEdit
				char szExeModule[MAX_PATH];
				if (GetModuleFileNameA(NULL, szExeModule, MAX_PATH))
				{
					char* pszName = strrchr(szExeModule, '\\');
					if (pszName) pszName++; else pszName = szExeModule;
					if (lstrcmpiA(pszName, "RegEdit.exe") == 0)
					{
						SelectServerInit();
						gbServerStarted = TRUE;
					} else if (lstrcmpiA(pszName, "RunDll32.exe") == 0) {
						// Вся обработка в StartRegEdit, которую зовет сам RunDll32
					} else {
						ShowError("Unknown exe module name (%s)!\nErrorCode = 0x%08X", pszName);
					}
				} else {
					ShowError("GetModuleFileNameW failed");
				}
			}
			break;
		case DLL_PROCESS_DETACH:
			if (gbServerStarted)
				SelectServerDone();
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}


#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return DllMain(hDll, dwReason,lpReserved);
}
#endif

#ifdef CRTSTARTUP
extern "C"{
BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return DllMain(hDll, dwReason,lpReserved);
};
};
#endif


EXTERN_C VOID WINAPI StartRegEdit(HWND,HINSTANCE,LPCTSTR /*lpCmd*/,DWORD)
{
	TCHAR szModule[MAX_PATH*3];
	if (!GetModuleFileName(g_hInstance, szModule, MAX_PATH*3))
	{
		ShowError("GetModuleFileName(g_hInstance) failed");
		return;
	}

	STARTUPINFOA si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {0};

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;

	BOOL lbRc = CreateProcessA(NULL, "RegEdit.exe", NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	if (!lbRc)
	{
		ShowError("Failed to start 'RegEdit.exe'");
		return;
	}

	Sleep(250);
	
	// Найти окно
	HWND hRegEdit = NULL;
	while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0 && !hRegEdit)
	{
		hRegEdit = NULL;
		while ((hRegEdit = FindWindowExA(NULL, hRegEdit, "RegEdit_RegEdit", NULL)) != NULL)
		{
			DWORD dwPID = 0;
			GetWindowThreadProcessId(hRegEdit, &dwPID);
			if (dwPID == pi.dwProcessId)
				break;
		}
	}
	
	if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0)
	{
		ShowError("RegEdit.exe was terminated", "");
		return;
	}
	
	WaitForInputIdle(pi.hProcess, 10000);

	int nInjRc = 
		InjectHookRegedit(pi.hProcess, szModule,
			#ifdef _WIN64
				TRUE
			#else
				FALSE
			#endif
			);
	if (nInjRc != 0)
	{
		char szErrCode[32]; wsprintfA(szErrCode, "%i", nInjRc);
		HWND hRegEdit = FindWindowExA(NULL, NULL, "RegEdit_RegEdit", NULL);
		if (hRegEdit) SendMessage(hRegEdit, WM_CLOSE, 0, 0);
		ShowError("InjectHook failed, code=%s", szErrCode);
	}
}
