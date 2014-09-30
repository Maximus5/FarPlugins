
#include "header.h"


MRegistryBase::MRegistryBase(BOOL abWow64on32, BOOL abVirtualize)
{
	eType = RE_UNDEFINED;
	bRemote = FALSE;
	sRemoteServer[0] = /*sRemoteLogin[0] = sRemotePassword[0] =*/ sRemoteResource[0] = 0;
	bDirty = false;
	mb_Wow64on32 = abWow64on32;
	mb_Virtualize = abVirtualize;
	mp_TempExportBuffer = NULL;
	mn_TempExportBuffer = 0;
	//bRemote = abRemote;
	//if (bRemote && asRemoteServer) {
	//	while (*asRemoteServer == L'\\' || *asRemoteServer == '/') asRemoteServer++;
	//	// The name of the remote computer. The string has the following form: \\computername
	//	if (asRemoteServer[0]) {
	//		sRemoteServer[0] = sRemoteServer[1] = L'\\';
	//		lstrcpynW(sRemoteServer+2, asRemoteServer, countof(sRemoteServer)-2);
	//	}
	//	lstrcpynW(sRemoteLogin, asRemoteLogin ? asRemoteLogin : L"", countof(sRemoteLogin));
	//	lstrcpynW(sRemotePassword, asRemotePassword ? asRemotePassword : L"", countof(sRemotePassword));
	//}
}

MRegistryBase::~MRegistryBase()
{
	// отключиться, если было подключение
	ConnectLocal();
	//
	SafeFree(mp_TempExportBuffer);
}

//DWORD MRegistryBase::SamDesired()
//{
//	switch (mb_Wow64on32)
//	{
//	case 0:
//		return KEY_WOW64_32KEY;
//	case 1:
//		return KEY_WOW64_64KEY;
//	default:
//		return 0;
//	}
//}

//MRegistryBase* MRegistryBase::CreateWorker(RegWorkType aType, bool abRemote, LPCWSTR asRemoteServer, LPCWSTR asRemoteLogin, LPCWSTR asRemotePassword)
//{
//	MRegistryBase* pWorker = NULL;
//	if (aType == RE_WINAPI)
//	{
//		pWorker = new MRegistryWinApi(/*abRemote, asRemoteServer, asRemoteLogin, asRemotePassword*/);
//		//if (abRemote)
//		
//	} else {
//		// Другие пока не поддерживаются
//		_ASSERTE(aType == RE_WINAPI);
//	}
//	return pWorker;
//}

BOOL MRegistryBase::ConnectRemote(LPCWSTR asServer, LPCWSTR asLogin /*= NULL*/, LPCWSTR asPassword /*= NULL*/, LPCWSTR asResource /*= NULL*/)
{
	if (!asServer || !*asServer)
	{
		InvalidOp();
		return FALSE;
	}
	
	// Сначала отключиться, если было другое подключение
	ConnectLocal();

	while (*asServer == L'\\' || *asServer == '/') asServer++;
	// The name of the remote computer. The string has the following form: \\computername
	if (asServer[0])
	{
		sRemoteServer[0] = sRemoteServer[1] = L'\\';
		lstrcpynW(sRemoteServer+2, asServer, countof(sRemoteServer)-2);

		if (asLogin && *asLogin && asPassword)
		{
			NETRESOURCEW res = {0};
			
			_ASSERTE(countof(sRemoteResource) > countof(sRemoteServer));
			if (asResource && asResource[0] == L'\\' && asResource[1] == L'\\')
			{
				lstrcpynW(sRemoteResource, asResource, countof(sRemoteResource));
			}
			else
			{
				lstrcpynW(sRemoteResource, sRemoteServer, countof(sRemoteResource));
				if (asResource && asResource[0] == L'\\')
				{
					lstrcatW(sRemoteResource, asResource);
				}
				else
				{
					lstrcatW(sRemoteResource, L"\\");
					if (asResource && *asResource)
						lstrcatW(sRemoteResource, asResource);
					else
						lstrcatW(sRemoteResource, L"IPC$");
				}
			}
			
			res.dwType = RESOURCETYPE_ANY;
			res.lpRemoteName = sRemoteResource;
			
			// Подключаемся
			CScreenRestore pop(GetMsg(REM_RemoteConnecting), GetMsg(REPluginName));
			DWORD dwErr = WNetAddConnection2W(&res, asPassword, asLogin, CONNECT_TEMPORARY);
			pop.Restore();
			if (dwErr != 0)
			{
				REPlugin::MessageFmt(REM_LogonFailed, asLogin, dwErr, _T("RemoteConnect"));
				return FALSE;
			}
		} else {
			sRemoteResource[0] = 0;
		}
	}
	bRemote = true;
	return TRUE;
}
#ifndef _UNICODE
BOOL MRegistryBase::ConnectRemote(LPCSTR asServer, LPCSTR asLogin /*= NULL*/, LPCSTR asPassword /*= NULL*/, LPCSTR asResource /*= NULL*/)
{
	wchar_t szServer[MAX_PATH+2], szLogin[MAX_PATH], szPassword[MAX_PATH], szResource[MAX_PATH];
	szServer[0] = szLogin[0] = szPassword[0] = szResource[0] = 0;
	if (asServer && *asServer) lstrcpy_t(szServer, countof(szServer), asServer);
	if (asLogin && *asLogin) lstrcpy_t(szLogin, countof(szLogin), asLogin);
	if (asPassword && *asPassword) lstrcpy_t(szPassword, countof(szPassword), asPassword);
	if (asResource && *asResource) lstrcpy_t(szResource, countof(szResource), asResource);
	return ConnectRemote(szServer, szLogin, szPassword, szResource);	
}
#endif
void MRegistryBase::ConnectLocal()
{
	if (bRemote)
	{
		if (sRemoteResource[0])
		{
			WNetCancelConnection2W(sRemoteResource, 0, 0);
		}
	}
	bRemote = false; sRemoteServer[0] = 0; sRemoteResource[0] = 0;
}

RegFolder* MRegistryBase::GetFolder(RegPath* apKey, u64 OpMode)
{
	return m_Cache.GetFolder(apKey, OpMode);
}

void MRegistryBase::FreeFindData(struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	RegFolder* pFolder = m_Cache.FindByPanelItems(PanelItem);
	if (!pFolder) {
		_ASSERTE(pFolder != NULL);
		return;
	}
	_ASSERTE(pFolder->mn_ItemCount == ItemsNumber);
	
	// Если нужно освобождать
	// TODO: Если отожрано много памяти - то и при разрешенном кешировании освобождать
#ifdef _DEBUG
	BOOL bNeedRelease = (!AllowCaching() || pFolder->bForceRelease);
#endif
	
	//if (bNeedRelease)
	//{
	_ASSERTE(pFolder->args.pWorker != this);
	if (!pFolder->Release())
	{
		// После Release() элемент должен был освободиться, за исключением
		// тех случаев, когда он остается в кеше (это те ключи, по которым 
		// юзер ходил явно, в панелях)
		_ASSERTE(pFolder->key.eType == RE_UNDEFINED || (!pFolder->bForceRelease));
	}
	//}
}

BOOL MRegistryBase::IsPredefined(HKEY hKey)
{
	if (hKey && (((ULONG_PTR)hKey) & HKEY__PREDEFINED_MASK) == HKEY__PREDEFINED_TEST)
		return TRUE;
	return FALSE;
}

LPBYTE MRegistryBase::GetExportBuffer(DWORD cbSize)
{
	if (mp_TempExportBuffer && mn_TempExportBuffer >= cbSize)
		return mp_TempExportBuffer;
	SafeFree(mp_TempExportBuffer);
	mn_TempExportBuffer = cbSize;
	mp_TempExportBuffer = (LPBYTE)malloc(cbSize);
	return mp_TempExportBuffer;
}

// возвращает битмаск из 'enum RegImportStyle'
DWORD MRegistryBase::GetAllowedImportStyles()
{
	DWORD nAllowed = 0;

	// ExistKey возвращает 0 - если ключ есть
	if (!ExistKey(HKEY__HIVE,0,0))
	{
		// Допустим только импорт "по месту", импорт в корень - невозможен
	}
	else if (cfg->is64bitOs)
	{
		// ExistKey возвращает 0 - если ключ есть

		// Проверить, есть ли в reg-файле HKLM или HKCR
		if (ExistKey(HKEY_LOCAL_MACHINE,0,0) && ExistKey(HKEY_CLASSES_ROOT,0,0))
		{
			// В этом случае (HKCU) импорт ведется без разницы 32/64
			nAllowed = ris_Native;
		}
		else
		{		
			if (ExistKey(HKEY_LOCAL_MACHINE, L"SOFTWARE", L"Wow6432Node") == 0)
			{
				// Допустим импорт только в 64битном режиме (это выгрузка 64битного ключа, раз он содержит Wow6432Node)
				nAllowed = ris_Import64;
			}
			else
			{
				nAllowed = ris_Native|ris_Import32|ris_Import64;
			}
		}
	}
	else
	{
		// x86 OS, различий веток реестра (32bit/64bit) нет вообще
		nAllowed = ris_Native;
	}
	return nAllowed;
}
