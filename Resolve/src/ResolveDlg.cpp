
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
#include <vector>
#include <TCHAR.h>

#undef _tcslen
#ifdef _UNICODE
#define _tcslen lstrlenW
#else
#define _tcslen lstrlenA
#endif

#include "ResolveDlg.h"

CResolveDlg::CResolveDlg()
{
	m_szConstant = NULL;
	ms_TempDir = NULL;

	ms_CurDir = (TCHAR*)calloc(1000, sizeof(TCHAR));
	GetCurrentDirectory(1000, ms_CurDir);
}

CResolveDlg::~CResolveDlg()
{
	if (ms_CurDir)
	{
		SetCurrentDirectory(ms_CurDir);
		free(ms_CurDir);
	}

	if (ms_TempDir)
	{
		int nLen = lstrlen(ms_TempDir);
		TCHAR* pszMask = (TCHAR*)calloc(nLen+MAX_PATH+2,sizeof(TCHAR));
		lstrcpy(pszMask, ms_TempDir);
		TCHAR* pszFile = pszMask + lstrlen(pszMask) + 1;
		lstrcat(pszMask, _T("\\*.*"));
		WIN32_FIND_DATA fnd;
		HANDLE h = FindFirstFile(pszMask, &fnd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do {
				if (!(fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					lstrcpy(pszFile, fnd.cFileName);
					DeleteFile(pszMask);
				}
			} while (FindNextFile(h, &fnd));
			FindClose(h);
		}
		RemoveDirectory(ms_TempDir);
		free(ms_TempDir);
	}
}


#define Write(data) {dw=0; LPCSTR lsTmp=(data); DWORD nLen=strlen(lsTmp); WriteFile(hFile,lsTmp,nLen,&dw,NULL);}

BOOL CResolveDlg::OnResolve(LPCTSTR asTempDir, LPCSTR asConst, LPTSTR* rsError, LPCTSTR* rsType, LPCTSTR* rsValue)
{
	_ASSERTE(rsError && *rsError==NULL);

	m_szConstant = asConst;

	ms_TempDir = _tcsdup(asTempDir);
	CreateDirectory(ms_TempDir, NULL);
	SetCurrentDirectory(ms_TempDir);

	/*
	char szDir[MAX_PATH+1], szCpp[MAX_PATH+20], szExe[MAX_PATH+20];
	int nLen = GetTempPathA(MAX_PATH, szDir);
	SetCurrentDirectoryA(szDir);
	if (!nLen) return FALSE;
	if (szDir[nLen-1] != ('\\')) {szDir[nLen-1] = ('\\'); szDir[nLen] = 0;}
	strcpy_s(szCpp, szDir); lstrcatA(szCpp, ("DefResolve.cpp"));
	strcpy_s(szExe, szDir); lstrcatA(szExe, ("DefResolve.exe"));
	*/
	#define szCpp "DefResolve.cpp"
	#define szExe "DefResolve.exe"

	HANDLE hFile = CreateFileA(szCpp, GENERIC_WRITE,
		FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
		NULL);

	DWORD dw;
	Write(("#define dfMYCONST ("));
	Write(m_szConstant);
	Write((")\r\n"));

	Write(gsIncs);
	Write(("\r\n"));

	Write(CPP_CODE);
	CloseHandle(hFile);

	int nMaxLen = strlen(gsIncPath)+strlen(gsLibPath)+strlen(gsPath)+100;
	LPSTR lpEnv = (char*)calloc(nMaxLen,sizeof(char));
	LPSTR lpCur = lpEnv;
	lstrcpyA(lpCur, ("PATH=")); lstrcatA(lpCur, gsPath); lpCur+=strlen(lpCur)+1;
	lstrcpyA(lpCur, ("include=")); lstrcatA(lpCur, gsIncPath); lpCur+=strlen(lpCur)+1;
	lstrcpyA(lpCur, ("lib=")); lstrcatA(lpCur, gsLibPath); lpCur+=strlen(lpCur)+1;
	*lpCur = '\0';

	BOOL lbRc = SetEnvironmentVariableA(("PATH"),gsPath);
	lbRc = SetEnvironmentVariableA(("include"),gsIncPath);
	lbRc = SetEnvironmentVariableA(("lib"),gsLibPath);

	char *szData = (char*)calloc(strlen(gsLibs)+4096,sizeof(char));
	lstrcpyA(szData, ("cl.exe /c /nologo /W3 /EHsc /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /D \"_CRT_SECURE_NO_WARNINGS\" \""));
	lstrcatA(szData, szCpp);
	lstrcatA(szData, ("\" \"/Fo"));
	//lstrcatA(szData, szDir);
	lstrcatA(szData, ("DefResolve.obj\" "));
	if (!Execute(szData, lpEnv, rsError, TRUE))
	{
		return FALSE;
	}

	LPSTR psz = NULL;
	while ((psz = strchr(gsLibs, ('\r')))!=NULL) *psz = (' ');
	while ((psz = strchr(gsLibs, ('\n')))!=NULL) *psz = (' ');
	lstrcpyA(szData, ("link.exe DefResolve.obj "));
	lstrcatA(szData, gsLibs);
	lstrcatA(szData, (" /nologo /subsystem:console /incremental:no /machine:I386 /out:\"DefResolve.exe\""));
	if (!Execute(szData, lpEnv, rsError, TRUE))
	{
		return FALSE;
	}

	lstrcpyA(szData, ("DefResolve.exe"));
	if (!Execute(szData, lpEnv, rsError, FALSE))
	{
		return FALSE;
	}

	hFile = CreateFileA(("DefResolve.dat"), GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
		NULL);
	dw = GetFileSize(hFile, NULL);
	BOOL bRead;
	#ifdef _UNICODE
		DWORD dwRead=0;
		*rsError = (TCHAR*)calloc((dw+1)*2,sizeof(TCHAR));
		bRead = ReadFile(hFile, (*rsError)+dw+1, dw, &dwRead, NULL);
		(*rsError)[dw]=0;
		MultiByteToWideChar(CP_ACP, 0, (LPSTR)((*rsError)+dw+1), dwRead, *rsError, dw);
	#else
		*rsError = (char*)calloc((dw+1),sizeof(char));
		bRead = ReadFile(hFile, (*rsError)+dw+1, dw, &dw, NULL);
		(*rsError)[dw]=0;
	#endif
	CloseHandle(hFile);

	/*
	DeleteFileA(("DefResolve.cpp"));
	DeleteFileA(("DefResolve.dat"));
	DeleteFileA(("DefResolve.obj"));
	DeleteFileA(("DefResolve.exe"));
	*/

	TCHAR* pszN = _tcschr(*rsError, _T('\n'));
	if (!pszN)
	{
		return FALSE;
	}

	*pszN = 0;
	*rsType = _tcschr(*rsError, _T(':'));
	if (!*rsType) *rsType = *rsError; else (*rsType)++;
	while (**rsType == _T(' ')) (*rsType)++;
	*rsValue = _tcschr(pszN+1, _T(':'));
	if (!*rsValue) *rsValue = pszN+1; else (*rsValue)++;
	while (**rsValue == _T(' ')) (*rsValue)++;

	return TRUE;
}

BOOL CResolveDlg::Execute(LPSTR a_szCmdLine, LPVOID a_lpEnvironment, LPTSTR *r_szReturn, BOOL abAllowOutput)
{
	HANDLE hReadPipe, hWritePipe;
	SECURITY_ATTRIBUTES saAttr; memset(&saAttr,0,sizeof(saAttr));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL;
	::CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0);

	::SetEnvironmentVariableA("PATH",gsPath);

	DWORD dwLastError;
	BOOL bCreate;
	STARTUPINFOA si; memset(&si,0,sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi; memset(&pi,0,sizeof(pi));

	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	si.hStdInput = hReadPipe;
	if (!abAllowOutput)
		si.dwFlags = STARTF_USESTDHANDLES;

	bCreate = ::CreateProcessA(NULL,a_szCmdLine,NULL,NULL,TRUE,
		NORMAL_PRIORITY_CLASS|(abAllowOutput ? 0 : DETACHED_PROCESS), NULL,
		NULL, &si, &pi);
	dwLastError = ::GetLastError();

	if (!bCreate)
	{
		CloseHandle(hWritePipe); CloseHandle(hReadPipe);
		int nMaxLen = lstrlenA(a_szCmdLine)+100;
		*r_szReturn = (TCHAR*)calloc(nMaxLen,sizeof(TCHAR));
		_tcscpy_s(*r_szReturn, nMaxLen, _T("Command executing failed:\n"));
		#ifdef _UNICODE
		MultiByteToWideChar(CP_OEMCP, 0, a_szCmdLine, -1, ((*r_szReturn)+lstrlenW(*r_szReturn)), strlen(a_szCmdLine)+1);
		#else
		_tcscat(*r_szReturn, a_szCmdLine);
		#endif
		return FALSE;
	}

	// Ожидание завершения
	DWORD dw = -1;
	::WaitForSingleObject(pi.hProcess, INFINITE);
	::GetExitCodeProcess(pi.hProcess,&dw);

	CloseHandle(hWritePipe); 
	DWORD dwRead, dwAll=0;
	char szBuffer[4096];
	std::vector<char*> szReads;

	for (;;)
	{
		if (!ReadFile(hReadPipe, szBuffer, 4095, &dwRead, NULL) ||
			dwRead==0) break;
		szBuffer[dwRead] = '\0';
		dwAll += dwRead;
		//r_szReturn += szBuffer;
		char* psz = NULL;
		while ((psz = strstr(szBuffer, "\r\n"))!=NULL) *psz = ' ';
		szReads.push_back(_strdup(szBuffer));
	}
	CloseHandle(hReadPipe);

	if (dw!=0)
	{
		int nCmdLen = lstrlenA(a_szCmdLine);
		*r_szReturn = (TCHAR*)calloc(dwAll+nCmdLen+128, sizeof(TCHAR));
		TCHAR *pszDst = *r_szReturn;
		wsprintf(pszDst, _T("%s\nCommand executing failed: 0x%08X\n"), _T("CPP Resolve"), dw); pszDst += _tcslen(pszDst);
		//_tcscpy_s(pszDst, dwAll+100, _T("Command executing failed:\n")); pszDst += _tcslen(pszDst);
		#ifdef _UNICODE
		MultiByteToWideChar(CP_OEMCP, 0, a_szCmdLine, -1, pszDst, nCmdLen+1);
		#else
		_tcscpy_s(pszDst, nCmdLen+1, a_szCmdLine);
		#endif
		pszDst += lstrlenW(pszDst);
		*(pszDst++) = _T('\n'); *pszDst = 0;

		for (UINT i=0; i<szReads.size(); i++)
		{
			char* psz = szReads[i];
			#ifdef _UNICODE
			MultiByteToWideChar(CP_OEMCP, 0, psz, -1, pszDst, dwAll);
			#else
			strcpy(pszDst, ("Command executing failed:\n"));
			#endif
			pszDst += _tcslen(pszDst);
		}

		return FALSE;
	}

	for (UINT i=0; i<szReads.size(); i++)
	{
		char* psz = szReads[i];
		free(psz);
	}

	return TRUE;
}
