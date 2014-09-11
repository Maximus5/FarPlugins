// TestLib.cpp : Defines the entry point for the console application.
//

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0500     // Change this to the appropriate value to target other versions of Windows.
#endif

#include <windows.h>
#include <stdio.h>
#include <tchar.h>


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		#ifdef _WIN64
		printf("Usage: TestLib64.exe \"<LibraryFileName>\"\n");
		#else
		printf("Usage: TestLib32.exe \"<LibraryFileName>\"\n");
		#endif
		return 1;
	}

	LPCTSTR pszLib = argv[1];

	int nLen = lstrlen(pszLib);
#ifdef _UNICODE
	char* pszLibO = (char*)malloc(nLen+1); pszLibO[nLen] = 0;
	WideCharToMultiByte(CP_OEMCP, 0, pszLib, -1, pszLibO, nLen+1, 0,0);
	printf("Loading library: '%s'\n", pszLibO);
	free(pszLibO);
#else
	printf("Loading library: '%s'\n", pszLib);
#endif

	HMODULE hLib = LoadLibrary(pszLib);
	if (hLib)
	{
		printf("Library was successfully loaded\n");
		FreeLibrary(hLib);
		return 0;
	}

	DWORD dwErr = GetLastError();
	printf("Library loading FAILED! ErrorCode=0x%08X (%i)\n", dwErr, dwErr);
	char* pszErrMsg = NULL;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|60,
		NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pszErrMsg, 0, NULL);
	if (pszErrMsg)
	{
		CharToOemBuffA(pszErrMsg, pszErrMsg, lstrlenA(pszErrMsg));
		printf("%s\n", pszErrMsg);
		LocalFree(pszErrMsg);
	}

	printf("\n");

	return dwErr;
}

