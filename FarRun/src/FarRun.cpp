
/*
Copyright (c) 2014 Maximus5
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

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
	SetEnvironmentVariable(_T("ConEmuDrive"), _T("T:"));
#endif

	TCHAR szIniFile[MAX_PATH] = _T("");
	GetModuleFileName(NULL, szIniFile, ARRAYSIZE(szIniFile));
	TCHAR* pszDot = _tcsrchr(szIniFile, _T('.'));
	if (!pszDot)
	{
		printf("GetModuleFileName failed, no extension\n");
		return 100;
	}
	_tcscpy(pszDot, _T(".ini"));

	TCHAR szIniCmd[0x800] = _T("");
	GetPrivateProfileString(_T("Run"), _T("Cmd"), _T(""), szIniCmd, ARRAYSIZE(szIniCmd), szIniFile);

	if (!*szIniCmd)
	{
		_tprintf(_T("Invalid initialization file:\n%s\nRequired: [Run] Cmd = ...\n"), szIniFile);
		return 101;
	}

	size_t cchAddLen = 0;
	for (int i = 1; i < argc; i++)
	{
		cchAddLen += _tcslen(argv[i]) + 3; // Space + two quotes
	}

	size_t cchMax = 0;
	TCHAR* pszExpand = NULL;
	if (_tcschr(szIniCmd, _T('%')))
	{
		DWORD nSize = ExpandEnvironmentStrings(szIniCmd, NULL, 0);
		if (nSize)
		{
			cchMax = max(_tcslen(szIniCmd),nSize) + 1 + cchAddLen;
			pszExpand = (TCHAR*)calloc(cchMax, sizeof(*pszExpand));
			DWORD nExp = ExpandEnvironmentStrings(szIniCmd, pszExpand, nSize+1);
			if (!nExp || nExp > nSize)
			{
				*pszExpand = 0;
			}
		}
	}

	if (!pszExpand)
	{
		cchMax = _tcslen(szIniCmd) + 1 + cchAddLen;
		pszExpand = (TCHAR*)calloc(cchMax, sizeof(*pszExpand));
	}

	if (!*pszExpand)
	{
		_tcscpy(pszExpand, szIniCmd);
	}

	for (int i = 1; i < argc; i++)
	{
		bool bQuot = (_tcschr(argv[i], _T(' ')) != NULL);
		_tcscat(pszExpand, bQuot ? _T(" \"") : _T(" "));
		_tcscat(pszExpand, argv[i]);
		if (bQuot) _tcscat(pszExpand, _T("\""));
	}

	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi = {};

	if (!CreateProcess(NULL, pszExpand, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		DWORD nErr = GetLastError();
		_tprintf(_T("Failed to run process, command:\n%s\nError code=%u\n"), pszExpand, nErr);
		return 102;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD nRc = 200;
	GetExitCodeProcess(pi.hProcess, &nRc);

	return nRc;
}
