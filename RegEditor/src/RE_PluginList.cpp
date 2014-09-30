
#include "header.h"
#include <Tlhelp32.h>
#include <ObjBase.h>

//#include "Hooks/RegEditExe.h"

#define NEED_REGEDIT_STYLE (WS_VISIBLE|WS_THICKFRAME|WS_CAPTION|WS_MAXIMIZEBOX)
#define WAIT_REGEDIT_IDLE 10000

REPluginList *gpPluginList = NULL;

REPluginList::REPluginList()
{
	mn_PlugCount = 0;
	memset(Plugins, 0, sizeof(Plugins));

	mp_SidCache = NULL; nMaxSidCount = nSidCount = 0;
#ifdef _DEBUG
	SearchTokenGroupsForSID();
#endif

	mn_RegeditPID = 0; mb_Hooked = FALSE;
	mh_RegeditProcess = NULL;
	mh_RegeditParent = mh_RegeditTree = mh_RegeditList = NULL;
}

REPluginList::~REPluginList()
{
	_ASSERTE(mn_PlugCount==0);
	int nPluginsLeft = 0;
	for (UINT i = 0; i < countof(Plugins); i++)
	{
		if (Plugins[i] != NULL)
		{
			nPluginsLeft ++;
		}
	}
	_ASSERTE(nPluginsLeft==0);
	
	if (mh_RegeditProcess)
	{
		CloseHandle(mh_RegeditProcess); mh_RegeditProcess = NULL;
	}
}

int REPluginList::Register(REPlugin* pPlugin)
{
	mn_PlugCount++;
	for (UINT i = 0; i < countof(Plugins); i++)
	{
		if (Plugins[i] == NULL)
		{
			Plugins[i] = pPlugin;
			return mn_PlugCount;
		}
	}
	InvalidOp();
	return mn_PlugCount;
}

int REPluginList::Unregister(REPlugin* pPlugin)
{
	if (mn_PlugCount > 0)
		mn_PlugCount--;
	for (UINT i = 0; i < countof(Plugins); i++)
	{
		if (Plugins[i] == pPlugin)
		{
			Plugins[i] = NULL;
			return mn_PlugCount;
		}
	}
	InvalidOp();
	return mn_PlugCount;
}

BOOL REPluginList::IsValid(REPlugin* pPlugin)
{
	if (!pPlugin)
		return FALSE;
	for (UINT i = 0; i < countof(Plugins); i++)
	{
		if (Plugins[i] && Plugins[i] == pPlugin)
		{
			return TRUE;
		}
	}
	//InvalidOp();
	_ASSERT(FALSE);
	return FALSE;
}

REPlugin* REPluginList::GetOpposite(REPlugin* pPlugin)
{
	for (UINT i = 0; i < countof(Plugins); i++)
	{
		if (Plugins[i] == NULL)
			continue;
		if (Plugins[i] == pPlugin)
			continue;
		
		PanelInfo inf = {sizeof(inf)};
		int nFRc;
		#ifdef _UNICODE
			#if FAR_UNICODE>=1988
				nFRc = psiControl((HANDLE)Plugins[i], FCTL_GETPANELINFO, 0, &inf);
			#else
				nFRc = psiControl((HANDLE)Plugins[i], FCTL_GETPANELINFO, 0, (LONG_PTR)&inf);
			#endif
		#else
		nFRc = psiControl((HANDLE)Plugins[i], FCTL_GETPANELSHORTINFO, &inf);
		#endif
		if (nFRc)
		{
			#if FAR_UNICODE>=1906
				if ((inf.Flags & (PFLAGS_PLUGIN|PFLAGS_VISIBLE)) == (PFLAGS_PLUGIN|PFLAGS_VISIBLE))
				{
					return Plugins[i];
				}
			#else
				if (inf.Plugin && inf.Visible)
				{
					return Plugins[i];
				}
			#endif
		}
	}
	return NULL;
}

//void REPluginList::UpdateAllTitles()
//{
//	for (UINT i = 0; i < countof(Plugins); i++)
//	{
//		if (Plugins[i])
//		{
//			Plugins[i]->m_Key.Update();
//		}
//	}
//}

void REPluginList::OnSettingsChanged(DWORD abWow64on32)
{
	for (UINT i = 0; i < countof(Plugins); i++)
	{
		if (Plugins[i])
		{
			Plugins[i]->setWow64on32(abWow64on32);
			Plugins[i]->m_Key.Update();
			Plugins[i]->UpdatePanel(false);
			Plugins[i]->RedrawPanel();
		}
	}
}

BOOL REPluginList::GetSIDName(LPCWSTR asComputer, PSID pSID, TCHAR* pszOwner, DWORD cchOwnerMax)
{
	BOOL lbExist = FALSE;
	_ASSERTE(pszOwner!=0);
	*pszOwner = 0;

	// Кеш
	if (mp_SidCache && nSidCount)
	{
		for (DWORD i = 0; i < nSidCount; i++)
		{
			if (EqualSid(pSID, mp_SidCache[i].pSID))
			{
				lstrcpyn(pszOwner, mp_SidCache[i].pszOwner, cchOwnerMax);
				return TRUE;
			}
		}
	}
	
	// Если в кеше нет - читаем из системы
	DWORD nAccountLength = 0, nDomainLength = 0;
	SID_NAME_USE snu;
	DWORD nLastErr = 0;
	if (!asComputer)
		asComputer = L"";
	BOOL lbRc = LookupAccountSidW(asComputer, pSID, NULL, &nAccountLength, NULL, &nDomainLength, &snu);
	if (!lbRc) nLastErr = GetLastError();
	if (nAccountLength && nDomainLength)
	{
		LPWSTR pszAccount = (wchar_t*)calloc(nAccountLength+1,2);
		LPWSTR pszDomain = (wchar_t*)calloc(nDomainLength+1,2);
		if (pszAccount && pszDomain)
		{
			if (!LookupAccountSidW(asComputer, pSID, pszAccount, &nAccountLength, pszDomain, &nDomainLength, &snu))
			{
				nLastErr = GetLastError();
			}
			else
			{
				//if (*pszDomain && (nDomainLength+nAccountLength+1) <= cchOwnerMax)
				//{
				//	lstrcpy_t(pszOwner, cchOwnerMax, pszDomain);
				//	lstrcat(pszOwner, _T("\\"));
				//	lstrcpy_t(pszOwner+lstrlen(pszOwner), cchOwnerMax-nDomainLength, pszAccount);
				//} else {
				lstrcpy_t(pszOwner, cchOwnerMax, pszAccount);
				//}
				lbExist = TRUE;
			}
		}
		SafeFree(pszAccount);
		SafeFree(pszDomain);
	}

	// Кеш
	if (!mp_SidCache || (nSidCount+1) > nMaxSidCount)
	{
		nMaxSidCount = nSidCount + 128;
		SidCache *pSidCache = (SidCache*)calloc(nMaxSidCount, sizeof(SidCache));
		if (nSidCount) {
			memmove(pSidCache, mp_SidCache, sizeof(SidCache)*nSidCount);
			SafeFree(mp_SidCache);
		}
		mp_SidCache = pSidCache;
	}

	DWORD nSidLen = GetLengthSid(pSID);
	mp_SidCache[nSidCount].pSID = (PSID)calloc(nSidLen,1);
	CopySid(nSidLen, mp_SidCache[nSidCount].pSID, pSID);
	mp_SidCache[nSidCount].pszOwner = lstrdup(lbExist ? pszOwner : _T(""));
	nSidCount++;

	return lbExist;
}

DWORD REPluginList::FindRegeditPID(HWND& hParent, HWND& hTree, HWND& hList, BOOL& bHooked, HANDLE* phProcess)
{
	// Найти процесс RegEdit.exe
	DWORD nRegeditPID = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if (hSnap == INVALID_HANDLE_VALUE) {
		REPlugin::Message(REM_CreateToolhelp32SnapshotFailed, FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK);
		return 0;
	}
	PROCESSENTRY32 ent = {sizeof(PROCESSENTRY32)};
	if (Process32First(hSnap, &ent))
	{
		do {
			if (lstrcmpi(ent.szExeFile, _T("regedit.exe")) == 0) {
				nRegeditPID = ent.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnap, &ent));
	}
	CloseHandle(hSnap);
	
	// Если по PID найти не удалось - все равно пробуем найти окно
	hParent = hTree = hList = NULL;
	while ((hParent = FindWindowEx(NULL, hParent, _T("RegEdit_RegEdit"), NULL)) != NULL)
	{
		DWORD dwPID = 0;
		if (nRegeditPID && GetWindowThreadProcessId(hParent, &dwPID) && dwPID != nRegeditPID)
			continue;
		
		DWORD dwStyle = (DWORD)GetWindowLongPtr(hParent, GWL_STYLE);
		if ((dwStyle & NEED_REGEDIT_STYLE) == NEED_REGEDIT_STYLE)
		{
			hTree = FindWindowEx(hParent, NULL, _T("SysTreeView32"), NULL);
			hList = FindWindowEx(hParent, NULL, _T("SysListView32"), NULL);
			if (hTree && hList)
				break;
		}
	}
	
	if (!hParent)
		return 0;

	// Проверяем, есть ли в этом RegEdit наш хук
	wchar_t sPipeName[MAX_PATH];
	wsprintfW(sPipeName, L"\\\\.\\pipe\\FarRegEditor.%u", nRegeditPID);
	HANDLE hPipe = CreateFileW(sPipeName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hPipe && hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
		bHooked = TRUE;
	} else {
		// Инжектить отсюда нельзя - сложно получить реальный адрес процедуры LoadLibraryW!
		bHooked = FALSE;
		#ifndef _WIN64
		if (cfg->is64bitOs)
		{
			REPlugin::Message(REM_RegeditFailed_NeedHandle);
			hParent = NULL;
			return nRegeditPID; // Возвращаем реальный PID, т.к. RegEdit.exe уже запущен, и пользователю его нужно сначала закрыть!
		}
		#endif
	}

	// Попробуем для синхронизации открыть хэндл процесса
	if (phProcess)
	{
		*phProcess = OpenProcess(
			SYNCHRONIZE|(bHooked ? 0 :
				(PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ|
				 PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION)),
			FALSE, nRegeditPID);
		if (!*phProcess && !bHooked)
		{
			REPlugin::Message(REM_RegeditFailed_NeedHandle);
			hParent = NULL;
			return nRegeditPID; // Возвращаем реальный PID, т.к. RegEdit.exe уже запущен, и пользователю его нужно сначала закрыть!
		}
	}

	return nRegeditPID;
}

BOOL REPluginList::ConnectRegedit(DWORD& nRegeditPID, HANDLE& hProcess, HWND& hParent, HWND& hTree, HWND& hList, BOOL& bHooked)
{
	if (mh_RegeditParent && IsWindow(mh_RegeditParent))
	{
		nRegeditPID = mn_RegeditPID;
		hProcess = mh_RegeditProcess;
		hParent = mh_RegeditParent;
		hTree = mh_RegeditTree;
		hList = mh_RegeditList;
		bHooked = mb_Hooked;
		return TRUE;
	}

	BOOL lbCoInitialized = FALSE;
	HRESULT hr = S_OK;
	SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
	BOOL bProcessSelfStarted = FALSE;

	hProcess = NULL; bHooked = FALSE;
	nRegeditPID = FindRegeditPID(hParent, hTree, hList, bHooked, &hProcess);
	if (nRegeditPID)
	{
		//// Попробуем для синхронизации открыть хэндл процесса
		//hProcess = OpenProcess( // Сначала с прицелом на запуск в нем нашей нити
		//	SYNCHRONIZE|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ|
		//	PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION, FALSE, nRegeditPID);

		//TODO: Проверить, есть ли там наш inject?
		
		//// Если отлуп - то просто для синхронизации
		//if (!hProcess)
		//{
		//	DWORD dwErr = GetLastError();
		//	hProcess = OpenProcess(SYNCHRONIZE, FALSE, nRegeditPID);
		//}
		goto wrap; // OK
	}

	if (mh_RegeditProcess)
	{
		CloseHandle(mh_RegeditProcess);
		mh_RegeditProcess = NULL;
	}
	mh_RegeditParent = NULL;


	//int nLen = 0;
	//wchar_t* pszData = NULL;
	// Если RegEdit.exe еще не запущен - нужно запустить
	//if (nRegeditPID == 0)
	//{
	//	BOOL lbSucceeded = FALSE;
	//	//MRegistryWinApi reg;
	//	//HREGKEY hk = NULL;
	//	// Для ускорения запуска - попытаемся сбросить стартовый ключ?
	//	RegPath key = {RE_WINAPI}; key.Init(RE_WINAPI, HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit");
	//	if (0 != reg.CreateKeyEx(HKEY_CURRENT_USER, key.mpsz_Key, 0, 0, 0, KEY_WRITE, 0, &hk, 0))
	//	{			
	//		CantOpenKey(&key, TRUE);
	//	} else {
	//		nLen = m_Key.mpsz_Key ? lstrlenW(m_Key.mpsz_Key) : 1;
	//		if (pItem && pItem->pszName)
	//			nLen += 1 + lstrlenW(pItem->pszName);
	//		pszData = (wchar_t*)calloc(nLen+128,2);
	//		lstrcpyW(pszData, L"My Computer\\");
	//		HKeyToStringKey(m_Key.mh_Root, pszData+lstrlenW(pszData), 40);
	//		if (m_Key.mpsz_Key && *m_Key.mpsz_Key)
	//		{
	//			lstrcatW(pszData, L"\\");
	//			lstrcatW(pszData, m_Key.mpsz_Key);
	//		}
	//		if (pItem && pItem->nValueType == REG__KEY)
	//		{
	//			lstrcatW(pszData, L"\\");
	//			lstrcatW(pszData, pItem->pszName);
	//		}
	//		nLen = lstrlenW(pszData)+1;
	//		if (0 != reg.SetValueEx(hk, L"LastKey", 0, REG_SZ, (LPBYTE)pszData, nLen*2))
	//		{
	//			ValueOperationFailed(&key, L"LastKey", TRUE, FALSE);
	//		} else {
	//			lbSucceeded = TRUE;
	//		}
	//		reg.CloseKey(hk);
	//	}
	//	if (lbSucceeded)
	//	{
	//		DWORD nRc = (DWORD)ShellExecute(NULL, _T("open"), _T("regedit.exe"), NULL, NULL, SW_SHOWNORMAL);
	//		if (nRc < 32)
	//		{
	//			Message(REM_RegeditExeFailed, FMSG_WARNING|FMSG_MB_OK|FMSG_ERRORTYPE);
	//		}
	//	}
	//	return;
	//}

	
	gpProgress = new REProgress(GetMsg(REStartingRegedit), GetMsg(REPluginName));

	// Если RegEdit.exe еще не запущен - нужно запустить
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (hr == S_OK || hr == S_FALSE)
		lbCoInitialized = TRUE;
		
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	TCHAR szVerb[32]; lstrcpy(szVerb, _T("open"));
	//TODO: Если Far32 бит, а OS 64 бит - запустить отсюда "C:\\Windows\\SysWOW64\\regedit.exe"?
	TCHAR szFile[MAX_PATH*2]; lstrcpy(szFile, _T("regedit.exe"));
	TCHAR szParms[MAX_PATH*3]; szParms[0] = 0;
	
	// Всегда, чтобы не было проблем с детектом адреса Kernel32.dll:LoadLibraryW
	// избежать проблем x64/x86 а также перехвата процедуры в ConEmu.
	BOOL lbNeedRunDll = TRUE;
	
	if (lbNeedRunDll)
	{
		OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
		GetVersionEx(&osv);
		if (osv.dwMajorVersion >=6)
			lstrcpy(szVerb, _T("runas"));
		
		TCHAR szWinDir[MAX_PATH];
		GetWindowsDirectory(szWinDir,countof(szWinDir));
		TCHAR szPluginPath[MAX_PATH*2+20];
		lstrcpyn(szPluginPath, psi.ModuleName, MAX_PATH*2);
		TCHAR* pszSlash = _tcsrchr(szPluginPath, _T('\\'));
		if (pszSlash) pszSlash++; else pszSlash = szPluginPath;
		if (cfg->is64bitOs)
			lstrcpy(pszSlash, _T("RegEditorH64.dll"));
		else
			lstrcpy(pszSlash, _T("RegEditorH32.dll"));

		HANDLE hFile = CreateFile(szPluginPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			#ifndef _WIN64
			if (cfg->is64bitOs)
			{
				hParent = NULL; // В 64битной ОС ничего не получится
				REPlugin::Message(REM_RegeditFailed_RequireH64);
				goto wrap;
			}
			#endif

			// Работаем без хука, напрямую
			lbNeedRunDll = FALSE;

		} else {
			CloseHandle(hFile);

			// Запускаем строго c:\windows, т.к. нам нужен (в Win64) 64битный RegEdit!
			wsprintf(szFile,_T("%s\\system32\\rundll32.exe"), szWinDir);
			wsprintf(szParms,_T(" \"%s\", StartRegEdit "), szPluginPath);
		}
	}
	
	
	sei.lpVerb = szVerb; sei.lpFile = szFile; sei.lpParameters = szParms;
	sei.nShow = SW_SHOWNORMAL;
	bProcessSelfStarted = TRUE;
	if (!ShellExecuteEx(&sei))
	{
		REPlugin::Message(REM_RegeditExeFailed, FMSG_WARNING|FMSG_MB_OK|FMSG_ERRORTYPE);
		goto wrap;
	}
	
	gpProgress->Message(GetMsg(REWaitForIdleRegedit));
	DWORD dwStart, dwDelta;
	dwStart = GetTickCount();
	do
	{
		Sleep(100);
		if (sei.hProcess)
		{
			if (lbNeedRunDll)
			{
				if (WaitForSingleObject(sei.hProcess, 100) == WAIT_OBJECT_0)
					break; // RunDll32 завершился, ищем RegEdit
			}
			else
			{
				if (WaitForSingleObject(sei.hProcess, 0) == WAIT_OBJECT_0)
					goto wrap; // процесс завершился
				if (WaitForInputIdle(sei.hProcess, 100) == WAIT_OBJECT_0)
					break; // OK, готов
			}
		}
		dwDelta = GetTickCount() - dwStart;
		if (gpProgress->CheckForEsc(TRUE))
			goto wrap;
	} while (dwDelta < WAIT_REGEDIT_IDLE);
	_ASSERTE(sei.hProcess!=NULL);
	
	
	nRegeditPID = FindRegeditPID(hParent, hTree, hList, bHooked, NULL);
	if (!nRegeditPID)
	{
		REPlugin::Message(REM_RegeditWindowFailed);
		goto wrap;
	}
	hProcess = sei.hProcess; sei.hProcess = NULL;

wrap:
	if (lbCoInitialized)
		CoUninitialize();
	SafeDelete(gpProgress);
	
	if (sei.hProcess)
	{
		CloseHandle(sei.hProcess); sei.hProcess = NULL;
	}

	if (hParent && nRegeditPID)
	{
		//if (mn_RegeditPID != nRegeditPID) mb_Hooked = FALSE;
		mn_RegeditPID = nRegeditPID;
		mh_RegeditProcess = hProcess;
		mh_RegeditParent = hParent;
		mh_RegeditTree = hTree;
		mh_RegeditList = hList;
		mb_Hooked = bHooked;
		
		////TODO: Здесь можно внедрить свою нить в процесс RegEdit.exe
		//if (mb_Hooked)
		//{
		//	bHooked = mb_Hooked;
		//} else {
		//	wchar_t sPipeName[MAX_PATH];
		//	wsprintfW(sPipeName, L"\\\\.\\pipe\\FarRegEditor.%u", nRegeditPID);
		//	HANDLE hPipe = CreateFileW(sPipeName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		//	// Инжектить отсюда нельзя - сложно получить реальный адрес процедуры LoadLibraryW!
		//	//if (hPipe == INVALID_HANDLE_VALUE && !bProcessSelfStarted && hProcess)
		//	//{
		//	//	// Может хук не инжектился?
		//	//	if (0 == InjectHookRegedit(hProcess, psi.ModuleName, cfg->is64bitOs))
		//	//	{
		//	//		// Ура, должно получиться
		//	//		hPipe = CreateFileW(sPipeName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		//	//	}
		//	//}
		//	if (hPipe && hPipe != INVALID_HANDLE_VALUE)
		//	{
		//		CloseHandle(hPipe);
		//		bHooked = mb_Hooked = TRUE;
		//	} else {
		//		if (hProcess == NULL)
		//			mh_RegeditParent = hParent = NULL; // Сделать мы ничего не сможем
		//	}
		//	//else
		//	//{
		//	//	int nHookRc = InjectHook(hProcess);
		//	//	if (nHookRc != 0)
		//	//	{
		//	//		switch (nHookRc)
		//	//		{
		//	//		case -100:
		//	//			// Нет файла хуков, не считаем ошибкой
		//	//			bHooked = mb_Hooked = FALSE;
		//	//			break;
		//	//		default:
		//	//			// Сообщение об ошибке
		//	//			if (nHookRc > 0)
		//	//			{
		//	//				// MessageFmt(int nFormatMsgID, LPCWSTR asArgument, DWORD nErrCode = 0, LPCTSTR asHelpTopic = NULL, DWORD nFlags = FMSG_WARNING|FMSG_MB_OK, int nBtnCount = 0);
		//	//				REPlugin::Message(nHookRc, FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK, 0, _T("JumpRegedit"));
		//	//				if (nHookRc == REM_RegeditFailed_VirtualAllocEx)
		//	//					hParent = NULL; // Значит - полный облом, ничего сделать мы не сможем
		//	//			}
		//	//			else
		//	//			{
		//	//				InvalidOp();
		//	//			}
		//	//		}
		//	//	} else {
		//	//		bHooked = mb_Hooked = TRUE;
		//	//	}
		//	//}
		//}
	}
	
	return (hParent != NULL);
}

//WARNING!! Библиотека загрузится только под WinXP и выше!
//int REPluginList::InjectHook(HANDLE hProcess)
//{
//	int iRc = InjectHookRegedit(hProcess, psi.ModuleName, cfg->is64bitOs);
//
//	//DWORD dwErr = 0, dwWait = 0;
//	//wchar_t szPluginPath[MAX_PATH*2+20], *pszSlash;
//	//HANDLE hFile = NULL;
//	//wchar_t* pszPathInProcess = NULL;
//	//SIZE_T size, write = 0;
//	//HANDLE hThread = NULL; DWORD nThreadID = 0;
//	//LPTHREAD_START_ROUTINE ptrLoadLibrary = NULL;
//	//
//	////TODO: Хорошо бы проверить, что в Win64 запущен 64битный RegEdit.exe
//	//
//	//
//	//lstrcpy_t(szPluginPath, countof(szPluginPath), psi.ModuleName);
//	//pszSlash = wcsrchr(szPluginPath, L'\\');
//	//if (pszSlash) pszSlash++; else pszSlash = szPluginPath;
//	//if (cfg->is64bitOs)
//	//	lstrcpy(pszSlash, L"RegEditorH64.dll");
//	//else
//	//	lstrcpy(pszSlash, L"RegEditorH32.dll");
//	//
//	//
//	//hFile = CreateFile(szPluginPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
//	//if (hFile == INVALID_HANDLE_VALUE) {
//	//	dwErr = GetLastError();
//	//	iRc = -100; // Нет файла хуков, ошибкой не считается
//	//	goto wrap;
//	//}
//	//CloseHandle(hFile); hFile = NULL;
//	//
//	//
//	////WARNING!! The process handle must have the PROCESS_VM_OPERATION access right!
//	//
//	//size = (lstrlen(szPluginPath)+1)*2;
//	//pszPathInProcess = (wchar_t*)VirtualAllocEx(hProcess, 0, size, MEM_COMMIT, PAGE_READWRITE);
//	//if (!pszPathInProcess)
//	//{
//	//	dwErr = GetLastError();
//	//	//_printf("VirtualAllocEx failed! ErrCode=0x%08X\n", dwErr);
//	//	iRc = REM_RegeditFailed_VirtualAllocEx;
//	//	goto wrap;
//	//}
//	//if (!WriteProcessMemory(hProcess, pszPathInProcess, szPluginPath, size, &write ) || size != write)
//	//{
//	//	dwErr = GetLastError();
//	//	//_printf("WriteProcessMemory failed! ErrCode=0x%08X\n", dwErr);
//	//	iRc = REM_RegeditFailed_WriteProcessMemory;
//	//	goto wrap;
//	//}
//	//
//	//// GetProcAddress не канает - он уже может быть перехвачен ConEmu!
//	////ptrLoadLibrary = (LPTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
//	////TODO: !Это только WinXP x86
////#ifdef _DEBUG
//	//// WinXP x86, base load address - 0x7C800000, ProcExport - 0x0000AEEB
//	//if (!cfg->is64bitOs)
//	//{
//	//	ptrLoadLibrary = (LPTHREAD_START_ROUTINE)(0x7C800000 + 0x0000AEEB); // 0x0000AEEB;
//	//}
//	//else
//	//{
//	//	//TODO: Нужно извлечь настроящий адрес для LoadLibraryW
//	//	//InvalidOp();
//	//	ptrLoadLibrary = (LPTHREAD_START_ROUTINE)(0x78D20000 + 0x20420);
//	//}
////#endif
//	//if (ptrLoadLibrary == NULL)
//	//{
//	//	dwErr = GetLastError();
//	//	VirtualFreeEx(hProcess, pszPathInProcess, size, MEM_DECOMMIT);
//	//	SetLastError(dwErr);
//	//	//_printf("GetProcAddress(kernel32, LoadLibraryW) failed! ErrCode=0x%08X\n", dwErr);
//	//	iRc = REM_RegeditFailed_LoadLibraryPtr;
//	//	goto wrap;
//	//}
//	//else
//	//{
//	//	hThread = CreateRemoteThread(hProcess, NULL, 0, ptrLoadLibrary, pszPathInProcess, 0, &nThreadID);
//	//	if (!hThread) {
//	//		dwErr = GetLastError();
//	//		//_printf("CreateRemoteThread failed! ErrCode=0x%08X\n", dwErr);
//	//		iRc = REM_RegeditFailed_CreateRemoteThread;
//	//		goto wrap;
//	//	}
//	//	// Дождаться, пока отработает
//	//	dwWait = WaitForSingleObject(hThread, 
//	//		#ifdef _DEBUG
//	//					INFINITE
//	//		#else
//	//					10000
//	//		#endif
//	//		);
//	//	if (dwWait != WAIT_OBJECT_0)
//	//	{
//	//		dwErr = GetLastError();
//	//		//_printf("Inject tread timeout!");
//	//		iRc = REM_RegeditFailed_WaitThreadReady;
//	//		goto wrap;
//	//	}
//	//	
//	//	VirtualFreeEx(hProcess, pszPathInProcess, size, MEM_DECOMMIT);
//	//}
//	//
//	//wrap:
//
//	return iRc;
//}
