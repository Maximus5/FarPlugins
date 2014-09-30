
#include "header.h"

#define USERS_HIVE_NAME_FORMAT L"FarRegEdit-%08x"

MFileHive::MFileHive(BOOL abWow64on32, BOOL abVirtualize)
	: MRegistryWinApi(abWow64on32, abVirtualize)
{
	eType = RE_HIVE;
	sHiveSubkeyName[0] = 0;
	psTempDirectory = NULL;
	psFilePathName = NULL;
	psShowFilePathName = NULL;
	hHiveKey = NULL;
	fRegLoadAppKey = NULL;
	HMODULE hAdvApi = GetModuleHandle(_T("Advapi32.dll"));
	if (hAdvApi)
		fRegLoadAppKey = (RegLoadAppKey_t)GetProcAddress(hAdvApi, "RegLoadAppKeyW");
}

MFileHive::~MFileHive()
{
	UnmountHive();
	_ASSERTE(hHiveKey == NULL);
	if (psTempDirectory)
	{
		DeleteFileW(psFilePathName);
		RemoveDirectoryW(psTempDirectory);
	}
	SafeFree(psFilePathName);
	SafeFree(psTempDirectory);
	SafeFree(psShowFilePathName);
}

// Static operations (for command line mount/unmount)
LONG MFileHive::GlobalMountHive(LPCWSTR asHiveFilePathName, HKEY ahRoot, LPCWSTR apszKey, HKEY* phKey)
{
	// При неудаче или в XP зовем RegLoadKey
	BOOL bWriteAccess = FALSE;
	BOOL bNeedRelease = TRUE;
	DWORD nBackupError = 0;
	if (!cfg->BackupPrivilegesAcuire(TRUE))
	{
		if (!cfg->BackupPrivilegesAcuire(FALSE))
		{
			nBackupError = GetLastError();
			bNeedRelease = FALSE;
		}
	}
	else
	{
		bWriteAccess = TRUE;
	}
	
	LONG hRc = ::RegLoadKeyW(ahRoot, apszKey, asHiveFilePathName);
	LONG lMountRc = 0;
	if (hRc == 0 && phKey)
	{
		lMountRc = ::RegOpenKeyExW(ahRoot, apszKey, 0, 
			(bWriteAccess ? KEY_ALL_ACCESS : KEY_READ) & ~(KEY_NOTIFY), phKey);
		if (lMountRc != 0)
		{
			hRc = lMountRc;
			lMountRc = ::RegUnLoadKeyW(ahRoot, apszKey);
		}
	}

	if (bNeedRelease)
		cfg->BackupPrivilegesRelease();

	return hRc;
}
LONG MFileHive::GlobalUnmountHive(HKEY ahRoot, LPCWSTR apszKey)
{
	LONG hRc = 0;
	BOOL bNeedRelease = TRUE;
	if (!cfg->BackupPrivilegesAcuire(TRUE))
		if (!cfg->BackupPrivilegesAcuire(FALSE))
			bNeedRelease = FALSE;

	hRc = ::RegUnLoadKeyW(ahRoot, apszKey);

	if (bNeedRelease)
		cfg->BackupPrivilegesRelease();

	return hRc;
}

LONG MFileHive::UnmountHive()
{
	// В XP зовем RegUnLoadKey, Vista и выше - только CloseKey
	LONG hRc = 0;
	if (hHiveKey)
	{
		// Зовем именно API функцию, т.к. наша может не закрыть этот хендл (он живой на время жизни панели)
		::RegCloseKey(hHiveKey);
		hHiveKey = NULL;
	}
	if (sHiveSubkeyName[0])
	{
		GlobalUnmountHive(HKEY_USERS, sHiveSubkeyName);
		
		//BOOL bNeedRelease = TRUE;
		//if (!cfg->BackupPrivilegesAcuire(TRUE))
		//	if (!cfg->BackupPrivilegesAcuire(FALSE))
		//		bNeedRelease = FALSE;
		//
		//hRc = ::RegUnLoadKeyW(HKEY_USERS, sHiveSubkeyName);
		//
		//if (bNeedRelease)
		//	cfg->BackupPrivilegesRelease();

		sHiveSubkeyName[0] = 0;
	}
	
	return 0;
}

// Возвращает имя загруженного файла (LoadHiveFile)
LPCWSTR MFileHive::GetFilePathName()
{
	_ASSERTE(psFilePathName != NULL);
	return psFilePathName;
}
LPCTSTR MFileHive::GetShowFilePathName()
{
	_ASSERTE(psFilePathName != NULL || psShowFilePathName != NULL);
	if (!psShowFilePathName && psFilePathName) {
		psShowFilePathName = UnmakeUNCPath_t(psFilePathName);
	}
	return psShowFilePathName;
}

// *.reg file operations (import)
LONG MFileHive::LoadHiveFile(LPCWSTR asHiveFilePathName, BOOL abSilent, BOOL abDelayLoading /*= FALSE*/)
{
	// На всякий случай - сначала зовем Unmount
	_ASSERTE(hHiveKey == NULL);
	UnmountHive();
	
	LONG hRc = 0;
	if (IsNetworkPath(asHiveFilePathName) || IsNetworkUNCPath(asHiveFilePathName)) {
		//TODO: Если файл сетевой - его нужно скопировать на локальный диск :( По крайней мере в XP так.
		InvalidOp();
		//return ERROR_FILE_NOT_FOUND;
	} else {
		psFilePathName = MakeUNCPath(asHiveFilePathName);
	}

	// Сначала - пробуем подмонтировать в HKEY_USERS
	CopyFileName(sHiveSubkeyName, MAX_REGKEY_NAME, asHiveFilePathName);
	hRc = GlobalMountHive(psFilePathName, HKEY_USERS, sHiveSubkeyName, &hHiveKey);
	if (hRc == 0)
		return hRc;
	// Раз не удалось подмонтировать - сразу сбросим имя ключа
	sHiveSubkeyName[0] = 0;
	
	// Если не удалось - в Vista+ можно попробоват подмонтировать для приложения
	if (fRegLoadAppKey)
	{
		// Vista: RegLoadAppKey - Each process can load only one hive at a time.
		LONG hVRc = fRegLoadAppKey(psFilePathName, &hHiveKey, KEY_ALL_ACCESS, 0, 0);
		// Если не удалось подцепиться с полным доступом - попробуем "только чтение"?
		if (hVRc == ERROR_ACCESS_DENIED)
			hVRc = fRegLoadAppKey(psFilePathName, &hHiveKey, KEY_READ, 0, 0);
			
		if (hVRc == 0)
			return hVRc;
	}
	
	// При неудаче или в XP зовем RegLoadKey
	//BOOL bWriteAccess = FALSE;
	//BOOL bNeedRelease = TRUE;
	//if (!cfg->BackupPrivilegesAcuire(TRUE)) {
	//	if (!cfg->BackupPrivilegesAcuire(FALSE))
	//		bNeedRelease = FALSE;
	//} else {
	//	bWriteAccess = TRUE;
	//}
	
	
	//wsprintfW(sHiveSubkeyName, USERS_HIVE_NAME_FORMAT, GetTickCount());
	//CopyFileName(sHiveSubkeyName, MAX_REGKEY_NAME, asHiveFilePathName);
	//hRc = GlobalMountHive(psFilePathName, HKEY_USERS, sHiveSubkeyName, &hHiveKey);
	//hRc = ::RegLoadKeyW(HKEY_USERS, sHiveSubkeyName, psFilePathName);
	//LONG lMountRc = 0;
	//if (hRc == 0)
	//{
	//	lMountRc = ::RegOpenKeyExW(HKEY_USERS, sHiveSubkeyName, 0, 
	//		(bWriteAccess ? KEY_ALL_ACCESS : KEY_READ) & ~(KEY_NOTIFY), &hHiveKey);
	//	if (lMountRc != 0)
	//	{
	//		hRc = lMountRc;
	//		lMountRc = ::RegUnLoadKeyW(HKEY_USERS, sHiveSubkeyName);
	//	}
	//}
	//
	//if (bNeedRelease)
	//	cfg->BackupPrivilegesRelease();

	return hRc;
}

// ***
MRegistryBase* MFileHive::Duplicate()
{
	InvalidOp();
	return NULL;
}

BOOL MFileHive::IsPredefined(HKEY hKey)
{
	return FALSE;
}

void MFileHive::FixHiveKey(HKEY& ahKey)
{
	if (ahKey == NULL || ahKey == HKEY__HIVE)
		ahKey = hHiveKey;
}

// Wrappers
LONG MFileHive::RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged)
{
	//FixHiveKey(hKey);
	return MRegistryWinApi::RenameKey(apParent, abCopyOnly, lpOldSubKey, lpNewSubKey, pbRegChanged);
}

LONG MFileHive::CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/, LPCWSTR pszComment /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::CreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition, pnKeyFlags, apRights, pszComment);
}

LONG MFileHive::OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::OpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult, pnKeyFlags, apRights);
}

LONG MFileHive::CloseKey(HKEY* phKey)
{
	if (*phKey == hHiveKey || *phKey == HKEY__HIVE)
	{
		*phKey = NULL;
		return 0;
	}
	return MRegistryWinApi::CloseKey(phKey);
}

LONG MFileHive::QueryInfoKey(
		HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved,
		LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen,
		LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen,
		LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::QueryInfoKey(
		hKey, lpClass, lpcClass, lpReserved,
		lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen,
		lpcValues, lpcMaxValueNameLen, lpcMaxValueLen,
		lpcbSecurityDescriptor, lpftLastWriteTime);
}

LONG MFileHive::EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::EnumValue(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpDataType, lpData, lpcbData, abEnumComments, ppszValueComment);
}

LONG MFileHive::EnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime, DWORD* pnKeyFlags /*= NULL*/, TCHAR* lpDefValue /*= NULL*/, DWORD cchDefValueMax /*= 0*/, LPCWSTR* ppszKeyComment /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::EnumKeyEx(hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime, pnKeyFlags, lpDefValue, cchDefValueMax, ppszKeyComment);
}

LONG MFileHive::QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::QueryValueEx(hKey, lpValueName, lpReserved, lpDataType, lpData, lpcbData, ppszValueComment);
}

LONG MFileHive::SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::SetValueEx(hKey, lpValueName, Reserved, nDataType, lpData, cbData, pszComment);
}


// Service - TRUE, если права были изменены
BOOL MFileHive::EditKeyPermissions(RegPath *pKey, RegItem* pItem, BOOL abVisual)
{
	return FALSE; // пока - FALSE
}

LONG MFileHive::DeleteValue(HKEY hKey, LPCWSTR lpValueName)
{
	FixHiveKey(hKey);
	LONG hRc = RegDeleteValueW(hKey, lpValueName);
	return hRc;
}

LONG MFileHive::DeleteSubkeyTree(HKEY hKey, LPCWSTR lpSubKey)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::DeleteSubkeyTree(hKey, lpSubKey);
}

LONG MFileHive::ExistValue(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpValueName, REGTYPE* pnDataType /*= NULL*/, DWORD* pnDataSize /*= NULL*/)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::ExistValue(hKey, lpszKey, lpValueName, pnDataType, pnDataSize);
}

LONG MFileHive::ExistKey(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpSubKey)
{
	FixHiveKey(hKey);
	return MRegistryWinApi::ExistKey(hKey, lpszKey, lpSubKey);
}

LONG MFileHive::SaveKey(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes /*= NULL*/)
{
	if (hKey == NULL || hKey == hHiveKey)
		return ERROR_ACCESS_DENIED;
	return MRegistryWinApi::SaveKey(hKey, lpFile, lpSecurityAttributes);
}

LONG MFileHive::RestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags /*= 0*/)
{
	if (hKey == NULL || hKey == hHiveKey)
		return ERROR_ACCESS_DENIED;
	return MRegistryWinApi::RestoreKey(hKey, lpFile, dwFlags);
}
LONG MFileHive::GetSubkeyInfo(HKEY hKey, LPCWSTR lpszSubkey, LPTSTR pszDesc, DWORD cchMaxDesc, LPTSTR pszOwner, DWORD cchMaxOwner)
{
	if (hKey == NULL || hKey == hHiveKey)
		return ERROR_ACCESS_DENIED;
	return MRegistryWinApi::GetSubkeyInfo(hKey, lpszSubkey, pszDesc, cchMaxDesc, pszOwner, cchMaxOwner);
}
