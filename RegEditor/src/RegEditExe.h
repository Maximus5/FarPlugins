
#pragma once

// #ifdef REGEDIT_INPROCESS

__inline int SelectTreeItem(HANDLE hProcess, HWND hParent, HWND hTree, HWND hList, LPWSTR asKeyPath, LPCWSTR asValueName = NULL)
{
	if (!hParent || !hTree || !hList || !asKeyPath || !*asKeyPath || !hProcess)
		return -1;
	if (!IsWindow(hParent) || !IsWindow(hTree) || !IsWindow(hList))
		return -10;

	// К сожалению, фокус можно поставить только так, но после этого почему-то у TreeView/ListView появляется "левая" рамка
	#ifndef REGEDIT_INPROCESS
	SetForegroundWindow(hTree);
	#else
	SetForegroundWindow(hParent);
	#endif
	SetFocus(hTree);
	

	if (hProcess) WaitForInputIdle(hProcess, 100); Sleep(250);
	
	HTREEITEM htviRoot = NULL, htvi = NULL;

	if (0 == SendMessageTimeout(hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&htviRoot) || !htviRoot)
		return -2;

	int iRc = 0;
	wchar_t szBuf[16384] = {0};
	TVITEMW tvi = {0};
	LVITEMW lvi = {0};
	LVFINDINFOW fnd = {LVFI_STRING};
	int nLen = lstrlenW(asKeyPath);
	CharUpperBuffW(asKeyPath, nLen);

#ifndef REGEDIT_INPROCESS
	SIZE_T size = sizeof(TVITEMW)+sizeof(LVFINDINFOW) + 16384*2, write, read;
	LPBYTE ptrRemote = (LPBYTE)VirtualAllocEx(hProcess, 0, size, MEM_COMMIT, PAGE_READWRITE);
	if (!ptrRemote)
		return -3;
	wchar_t* ptrRemoteText = (wchar_t*)(((TVITEMW*)ptrRemote)+1);
	wchar_t* ptrRemoteFind = (wchar_t*)(((LVFINDINFOW*)ptrRemote)+1);
#else
	wchar_t* ptrRemoteText = szBuf;
	wchar_t* ptrRemoteFind = szBuf;
	LPVOID ptrRemote = NULL;
#endif

	
	while (asKeyPath && *asKeyPath)
	{
		wchar_t* pszSlash = wcschr(asKeyPath, L'\\');
		if (pszSlash) {
			*pszSlash = 0;
			nLen = (DWORD)(pszSlash - asKeyPath);
		} else {
			nLen = lstrlenW(asKeyPath);
		}

		TreeView_Expand(hTree, htviRoot, TVE_EXPAND);
		WaitForInputIdle(hProcess, 1000);
		
		htvi = TreeView_GetChild(hTree, htviRoot); // TVGN_CHILD = 4
		while (htvi)
		{
			tvi.mask = TVIF_TEXT|TVIF_HANDLE;
			tvi.hItem = htvi;
			tvi.pszText = ptrRemoteText;
			tvi.cchTextMax = 512;
			#ifndef REGEDIT_INPROCESS
			if (!WriteProcessMemory(hProcess, ptrRemote, &tvi, sizeof(TVITEMW), &write ) || write != sizeof(TVITEMW)) {
				iRc = -4;
				goto wrap;
			}
			#else
			ptrRemote = &tvi;
			#endif

			// Чтобы точно не наколоться с юникодом
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
					iRc = -5;
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
			}
		
			// Next sibling
			htvi = TreeView_GetNextSibling(hTree, htvi);
		}
		if (!htvi)
		{
			iRc = 1; // НЕ НАЙДЕНО
			break;
		}
		
		htviRoot = htvi;
		if (!pszSlash)
			break; // нашли
		asKeyPath = pszSlash+1;
		//// Если ключ НЕ развернут - нужно развернуть
		//if ((tvi.state & TVIS_EXPANDED) == 0)
		//	TreeView_Expand(hTree, htvi, TVE_EXPAND);
	}
	
	TreeView_SelectItem(hTree, htviRoot);

	// ListView	
	if (iRc == 0 && asValueName)
	{
		WaitForInputIdle(hProcess, 1000);
		Sleep(250);

		// К сожалению, фокус можно поставить только так, но после этого почему-то у TreeView/ListView появляется "левая" рамка
		#ifndef REGEDIT_INPROCESS
		SetForegroundWindow(hList);
		#endif
		SetFocus(hList);

		int nItem = 0;
		int nCount = (int)ListView_GetItemCount(hList);
		if (*asValueName)
		{
			fnd.psz = ptrRemoteFind; fnd.vkDirection = VK_DOWN;
			
			#ifndef REGEDIT_INPROCESS
			if (!WriteProcessMemory(hProcess, ptrRemote, &fnd, sizeof(LVFINDINFOW), &write ) || write != sizeof(LVFINDINFOW)) {
				iRc = -6;
				goto wrap;
			}
			#endif

			lstrcpynW(szBuf, asValueName, 16383);

			#ifndef REGEDIT_INPROCESS
			nLen = (lstrlenW(szBuf)+1)*2;
			if (!WriteProcessMemory(hProcess, ptrRemote+sizeof(LVFINDINFOW), szBuf, nLen, &write ) || write != (DWORD)nLen) {
				iRc = -6;
				goto wrap;
			}
			#endif

			// Чтобы точно не наколоться с юникодом
			nItem = SendMessageW(hList, LVM_FINDITEMW, -1, (LPARAM)ptrRemote);
		}
		SendMessageW(hList, WM_KEYDOWN, VK_HOME, 0);
		WaitForInputIdle(hProcess, 1000);
		Sleep(100);
		for (int i = 0; i < nItem; i++)
			SendMessageW(hList, WM_KEYDOWN, VK_DOWN, 0);
		//lvi.state = 0; lvi.stateMask = LVIS_STATEIMAGEMASK; lvi.mask = LVIF_STATE;
		//if (!WriteProcessMemory(hProcess, ptrRemote, &lvi, sizeof(lvi), &write ) || write != sizeof(lvi)) {
		//	iRc = -7;
		//	goto wrap;
		//}
		//SendMessageW(hList, LVM_SETITEMSTATE, -1, (LPARAM)ptrRemote);
		//WaitForInputIdle(hProcess, 100);
		//if (nItem != -1)
		//{
		//	lvi.state = LVIS_FOCUSED|LVIS_SELECTED; lvi.iItem = nItem; //lvi.stateMask = LVIS_STATEIMAGEMASK; lvi.iItem = nItem;
		//	if (!WriteProcessMemory(hProcess, ptrRemote, &lvi, sizeof(lvi), &write ) || write != sizeof(lvi)) {
		//		iRc = -7;
		//		goto wrap;
		//	}
		//	nLen = SendMessageW(hList, LVM_SETITEMSTATE, nItem, (LPARAM)ptrRemote);
		//}
	}
		
	
	// Done
	//iRc = 0;
	SetForegroundWindow(hParent);
#ifndef REGEDIT_INPROCESS
wrap:
	VirtualFreeEx(hProcess, ptrRemote, size, MEM_DECOMMIT);
#endif
	return iRc;
}
