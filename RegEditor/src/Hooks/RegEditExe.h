
#pragma once

#include <commctrl.h>

#include "../RegEditor_Lang.h"

#ifdef REGEDIT_INPROCESS
extern UINT nSetFocusMsg;
extern HINSTANCE g_hInstance;
#endif


__inline int InjectHookRegedit(HANDLE hProcess, LPCTSTR asModulePathName, BOOL ab64bitOs)
{
#ifndef REGEDIT_INPROCESS
	return -1;
#else
	int iRc = 0;

	DWORD dwErr = 0, dwWait = 0;
	wchar_t szPluginPath[MAX_PATH*2+20]; //, *pszSlash;
	HANDLE hFile = NULL;
	wchar_t* pszPathInProcess = NULL;
	SIZE_T size, write = 0;
	HANDLE hThread = NULL; DWORD nThreadID = 0;
	LPTHREAD_START_ROUTINE ptrLoadLibrary = NULL;
	
	//TODO: Хорошо бы проверить, что в Win64 запущен 64битный RegEdit.exe


	#ifdef REGEDIT_INPROCESS
		GetModuleFileNameW(g_hInstance, szPluginPath, sizeof(szPluginPath)/sizeof(szPluginPath[0]));
	#else
		lstrcpy_t(szPluginPath, countof(szPluginPath), psi.ModuleName);
		//#ifdef _UNICODE
		//lstrcpyn(szPluginPath, asModulePathName, MAX_PATH*2);
		//#else
		//MultiByteToWideChar(CP_ACP,0, asModulePathName, -1, szPluginPath, MAX_PATH*2);
		//#endif
		wchar_t* pszSlash = wcsrchr(szPluginPath, L'\\');
		if (pszSlash) pszSlash++; else pszSlash = szPluginPath;
		if (ab64bitOs)
			lstrcpy(pszSlash, L"RegEditorH64.dll");
		else
			lstrcpy(pszSlash, L"RegEditorH32.dll");

		// Проверить наличие
		hFile = CreateFile(szPluginPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			dwErr = GetLastError();
			iRc = -100; // Нет файла хуков, ошибкой не считается
			goto wrap;
		}
		CloseHandle(hFile); hFile = NULL;
	#endif


	//WARNING!! The process handle must have the PROCESS_VM_OPERATION access right!
	
	size = (lstrlen(szPluginPath)+1)*2;
	pszPathInProcess = (wchar_t*)VirtualAllocEx(hProcess, 0, size, MEM_COMMIT, PAGE_READWRITE);
	if (!pszPathInProcess)
	{
		dwErr = GetLastError();
		//_printf("VirtualAllocEx failed! ErrCode=0x%08X\n", dwErr);
		iRc = REM_RegeditFailed_VirtualAllocEx;
		goto wrap;
	}
	if (!WriteProcessMemory(hProcess, pszPathInProcess, szPluginPath, size, &write ) || size != write)
	{
		dwErr = GetLastError();
		//_printf("WriteProcessMemory failed! ErrCode=0x%08X\n", dwErr);
		iRc = REM_RegeditFailed_WriteProcessMemory;
		goto wrap;
	}
	
	//// GetProcAddress не канает - он уже может быть перехвачен ConEmu!
	// Т.к. теперь это запускается только из RunDll32.exe - то канает
	ptrLoadLibrary = (LPTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");

	//#ifdef _DEBUG
	//	//TODO: !
	//	// Для проверки битности процесса (из FAR.exe) можно попробовать смотреть
	//	// MODULEENTRY32.modBaseAddr, а звать TH32CS_SNAPMODULE с учетом, что он скорее всего обломится
	//	// если битность фара не совпадает с битностью RegEdit'а
	//	
	//	// WinXP x86, base load address - 0x7C800000, ProcExport - 0x0000AEEB
	//	if (!ab64bitOs)
	//	{
	//		ptrLoadLibrary = (LPTHREAD_START_ROUTINE)(0x7C800000 + 0x0000AEEB); // 0x0000AEEB;
	//	}
	//	else
	//	{
	//		//TODO: Нужно извлечь настроящий адрес для LoadLibraryW
	//		//InvalidOp();
	//		ptrLoadLibrary = (LPTHREAD_START_ROUTINE)(0x78D20000 + 0x20420);
	//	}
	//#endif
	
	if (ptrLoadLibrary == NULL)
	{
		dwErr = GetLastError();
		VirtualFreeEx(hProcess, pszPathInProcess, size, MEM_DECOMMIT);
		SetLastError(dwErr);
		//_printf("GetProcAddress(kernel32, LoadLibraryW) failed! ErrCode=0x%08X\n", dwErr);
		iRc = REM_RegeditFailed_LoadLibraryPtr;
		goto wrap;
	}
	else
	{
		hThread = CreateRemoteThread(hProcess, NULL, 0, ptrLoadLibrary, pszPathInProcess, 0, &nThreadID);
		if (!hThread) {
			dwErr = GetLastError();
			//_printf("CreateRemoteThread failed! ErrCode=0x%08X\n", dwErr);
			iRc = REM_RegeditFailed_CreateRemoteThread;
			goto wrap;
		}
		// Дождаться, пока отработает
		dwWait = WaitForSingleObject(hThread, 
			#ifdef _DEBUG
						INFINITE
			#else
						10000
			#endif
			);
		if (dwWait != WAIT_OBJECT_0)
		{
			dwErr = GetLastError();
			//_printf("Inject tread timeout!");
			iRc = REM_RegeditFailed_WaitThreadReady;
			goto wrap;
		}
		
		VirtualFreeEx(hProcess, pszPathInProcess, size, MEM_DECOMMIT);
	}

wrap:
	return iRc;
#endif
}


__inline int SelectTreeItem(HANDLE hProcess, HWND hParent, HWND hTree, HWND hList, LPWSTR asKeyPath, LPCWSTR asValueName = NULL)
{
	if (!hProcess)
	{
		SetLastError(ERROR_BAD_ARGUMENTS);
		return REM_RegeditFailed_NeedHandle;
	}
	if (!hParent || !hTree || !hList || !asKeyPath || !*asKeyPath)
	{
		SetLastError(ERROR_BAD_ARGUMENTS);
		return REM_RegeditFailed_NullArgs;
	}
	if (!IsWindow(hParent) || !IsWindow(hTree) || !IsWindow(hList))
	{	
		SetLastError(ERROR_BAD_ARGUMENTS);
		return REM_RegeditFailed_NoWindow;
	}

	// К сожалению, фокус можно поставить только так, но после этого почему-то у TreeView/ListView появляется "левая" рамка
	#ifndef REGEDIT_INPROCESS
	SetForegroundWindow(hTree);
	SetFocus(hTree);
	#else
	SetForegroundWindow(hParent);
	SendMessage(hParent, nSetFocusMsg, 0, (LPARAM)hTree);
	#endif
	

	if (hProcess) WaitForInputIdle(hProcess, 250); (250);
	
	HTREEITEM htviRoot = NULL, htviParent = NULL, htvi = NULL;

	if (0 == SendMessageTimeout(hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&htviRoot) || !htviRoot)
		return REM_RegeditWindowHung;

	int iRc = 0;
	wchar_t szBuf[16384] = {0};
	TVITEMW tvi = {0};
	LVITEMW lvi = {0};
	LVFINDINFOW fnd = {LVFI_STRING};
	LONG_PTR nLen = lstrlenW(asKeyPath);
	CharUpperBuffW(asKeyPath, (DWORD)nLen);

#ifndef REGEDIT_INPROCESS
	SIZE_T size = sizeof(TVITEMW)+sizeof(LVFINDINFOW) + 16384*2, write, read;
	LPBYTE ptrRemote = (LPBYTE)VirtualAllocEx(hProcess, 0, size, MEM_COMMIT, PAGE_READWRITE);
	if (!ptrRemote)
		return REM_RegeditFailed_VirtualAllocEx;
	wchar_t* ptrRemoteText = (wchar_t*)(((TVITEMW*)ptrRemote)+1);
	wchar_t* ptrRemoteFind = (wchar_t*)(((LVFINDINFOW*)ptrRemote)+1);
#else
	wchar_t* ptrRemoteText = szBuf;
	wchar_t* ptrRemoteFind = szBuf;
	LPVOID ptrRemote = NULL;
#endif

	htviParent = htviRoot;
	
	while (asKeyPath && *asKeyPath)
	{
		wchar_t* pszSlash = wcschr(asKeyPath, L'\\');
		if (pszSlash) {
			*pszSlash = 0;
			nLen = (DWORD)(pszSlash - asKeyPath);
		} else {
			nLen = lstrlenW(asKeyPath);
		}

		TreeView_Expand(hTree, htviParent, TVE_EXPAND);
		if (hProcess) WaitForInputIdle(hProcess, 1000);
		
		htvi = TreeView_GetChild(hTree, htviParent); // TVGN_CHILD = 4
		while (htvi)
		{
			tvi.mask = TVIF_TEXT|TVIF_HANDLE;
			tvi.hItem = htvi;
			tvi.pszText = ptrRemoteText;
			tvi.cchTextMax = 512;
			#ifndef REGEDIT_INPROCESS
			if (!WriteProcessMemory(hProcess, ptrRemote, &tvi, sizeof(TVITEMW), &write ) || write != sizeof(TVITEMW)) {
				iRc = REM_RegeditFailed_WriteProcessMemory;
				goto wrap;
			}
			#else
			ptrRemote = &tvi;
			#endif

			// Чтобы точно не наколоться с юникодом - явно TVM_GETITEMW
			if (SendMessageW(hTree, TVM_GETITEMW, 0, (LPARAM)ptrRemote))
			{
				//if (ReadProcessMemory(hProcess, ptrRemote, &tvi, sizeof(TVITEMW), &read) || read != sizeof(TVITEMW))
				//{
				//	iRc = -5;
				//	goto wrap;
				//}
				#ifndef REGEDIT_INPROCESS
				if (!ReadProcessMemory(hProcess, ptrRemoteText, szBuf, 1024, &read) || read != 1024)
				{
					iRc = REM_RegeditFailed_ReadProcessMemory;
					goto wrap;
				}
				#endif
				int nItemLen = lstrlenW(szBuf);
				if (nItemLen == nLen)
				{
					CharUpperBuffW(szBuf, nItemLen);
					if (memcmp(szBuf, asKeyPath, nItemLen*2) == 0)
						break; // Нашли
				}
			} else {
				iRc = REM_RegeditFailed_TVM_GETITEMW; // НЕ НАЙДЕНО
				break;
			}
		
			// Next sibling
			htvi = TreeView_GetNextSibling(hTree, htvi);
		}
		if (!htvi)
		{
			iRc = REM_RegeditFailed_KeyNotFound; // НЕ НАЙДЕНО
			break;
		}
		
		htviParent = htvi;
		if (!pszSlash)
			break; // нашли
		asKeyPath = pszSlash+1;
	}
	
	SendMessage(hTree, WM_SETREDRAW, FALSE, 0);
	TreeView_SelectItem(hTree, htviRoot);
	TreeView_EnsureVisible(hTree, htviRoot);
	if (htviParent != htviRoot)
	{
		TreeView_SelectItem(hTree, htviParent);
		TreeView_EnsureVisible(hTree, htviParent);
	}
	SendMessage(hTree, WM_SETREDRAW, TRUE, 0);

	// ListView	
	if (iRc == 0 && asValueName)
	{
		Sleep(250);
		if (hProcess) WaitForInputIdle(hProcess, 1000);

		// К сожалению, фокус можно поставить только так, но после этого почему-то у TreeView/ListView появляется "левая" рамка
		#ifndef REGEDIT_INPROCESS
		SetForegroundWindow(hList);
		SetFocus(hList);
		#else
		SendMessage(hParent, nSetFocusMsg, 0, (LPARAM)hList);
		#endif
		
		if (hProcess) WaitForInputIdle(hProcess, 1000);

		LRESULT nItem = 0;
		int nCount = (int)ListView_GetItemCount(hList);
		if (*asValueName)
		{
			fnd.psz = ptrRemoteFind; fnd.vkDirection = VK_DOWN;
			
			#ifndef REGEDIT_INPROCESS
			if (!WriteProcessMemory(hProcess, ptrRemote, &fnd, sizeof(LVFINDINFOW), &write ) || write != sizeof(LVFINDINFOW)) {
				iRc = REM_RegeditFailed_WriteProcessMemory;
				goto wrap;
			}
			#else
			ptrRemote = &fnd;
			#endif

			lstrcpynW(szBuf, asValueName, 16383);

			#ifndef REGEDIT_INPROCESS
			nLen = (lstrlenW(szBuf)+1)*2;
			if (!WriteProcessMemory(hProcess, ptrRemoteFind, szBuf, nLen, &write ) || write != (DWORD)nLen) {
				iRc = REM_RegeditFailed_WriteProcessMemory;
				goto wrap;
			}
			#endif

			// Чтобы точно не наколоться с юникодом - явно LVM_FINDITEMW
			nItem = SendMessageW(hList, LVM_FINDITEMW, -1, (LPARAM)ptrRemote);
		}

		lvi.state = 0; lvi.stateMask = 0x000F;
		#ifndef REGEDIT_INPROCESS
		if (!WriteProcessMemory(hProcess, ptrRemote, &lvi, sizeof(lvi), &write ) || write != sizeof(lvi)) {
			iRc = REM_RegeditFailed_WriteProcessMemory;
			goto wrap;
		}
		#else
		ptrRemote = &lvi;
		#endif
		SendMessageW(hList, LVM_SETITEMSTATE, -1, (LPARAM)ptrRemote);
		
		if (hProcess) WaitForInputIdle(hProcess, 250);
		
		if (nItem != -1)
		{
			lvi.state = LVIS_FOCUSED|LVIS_SELECTED; lvi.stateMask = 0x000F;
			#ifndef REGEDIT_INPROCESS
			if (!WriteProcessMemory(hProcess, ptrRemote, &lvi, sizeof(lvi), &write ) || write != sizeof(lvi)) {
				iRc = REM_RegeditFailed_WriteProcessMemory;
				goto wrap;
			}
			#else
			ptrRemote = &lvi;
			#endif
			
			nLen = SendMessageW(hList, LVM_SETITEMSTATE, nItem, (LPARAM)ptrRemote);
		} else {
			iRc = REM_RegeditFailed_ValueNotFound;
		}
	}
		
	
	// Done
	SetForegroundWindow(hParent);
	

#ifndef REGEDIT_INPROCESS
wrap:
	VirtualFreeEx(hProcess, ptrRemote, size, MEM_DECOMMIT);
#endif
	return iRc;
}
