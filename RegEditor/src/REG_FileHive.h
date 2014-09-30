
#pragma once

class MFileHive : public MRegistryWinApi
{
protected:
	/*
	**** унаследованы от MRegistryWinApi ***
	struct {
		HREGKEY    hKey, hKeyRemote;
		wchar_t szKey[32];
		bool    bSlow;
	} hkPredefined[10];
	DWORD nPredefined;
	BOOL  bTokenAquired;
	HREGKEY  hAcquiredHkey;
	*/
	/* 
	**** унаследованы от MRegistryBase ***
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH];
	**************************************
	*/
	wchar_t sHiveSubkeyName[MAX_REGKEY_NAME+1];
	wchar_t *psTempDirectory;
	wchar_t *psFilePathName;
	TCHAR   *psShowFilePathName;
	HREGKEY hHiveKey;
	typedef LONG (WINAPI* RegLoadAppKey_t)(LPCWSTR lpFile,HKEY* phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
	RegLoadAppKey_t fRegLoadAppKey;
private:
	LONG UnmountHive();
	void FixHiveKey(HKEY& ahKey);
public:
	MFileHive(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MFileHive();
	// ¬озвращает им€ загруженного файла (LoadHiveFile)
	LPCWSTR GetFilePathName();
	LPCTSTR GetShowFilePathName();
	// Hive file operations (Mount)
	LONG LoadHiveFile(LPCWSTR asHiveFilePathName, BOOL abSilent, BOOL abDelayLoading = FALSE);
	// Static operations (for command line mount/unmount)
	static LONG GlobalMountHive(LPCWSTR asHiveFilePathName, HKEY ahRoot, LPCWSTR apszKey, HKEY* phKey = NULL);
	static LONG GlobalUnmountHive(HKEY ahRoot, LPCWSTR apszKey);
	
public:
	// ***
	virtual MRegistryBase* Duplicate();
	virtual BOOL IsPredefined(HKEY hKey);
	virtual LONG RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged);
	// Wrappers
	virtual LONG CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL, LPCWSTR pszComment = NULL);
	virtual LONG OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL);
	virtual LONG CloseKey(HKEY* phKey);
	virtual LONG QueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime);
	virtual LONG EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment = NULL);
	virtual LONG EnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime, DWORD* pnKeyFlags = NULL, TCHAR* lpDefValue = NULL, DWORD cchDefValueMax = 0, LPCWSTR* ppszKeyComment = NULL);
	virtual LONG QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment = NULL);
	virtual LONG SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment = NULL);
	virtual LONG DeleteValue(HKEY hKey, LPCWSTR lpValueName);
	virtual LONG DeleteSubkeyTree(HKEY hKey, LPCWSTR lpSubKey);
	virtual LONG ExistValue(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpValueName, REGTYPE* pnDataType = NULL, DWORD* pnDataSize = NULL);
	virtual LONG ExistKey(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpSubKey);
	virtual LONG SaveKey(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL);
	virtual LONG RestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags = 0);
	virtual LONG GetSubkeyInfo(HKEY hKey, LPCWSTR lpszSubkey, LPTSTR pszDesc, DWORD cchMaxDesc, LPTSTR pszOwner, DWORD cchMaxOwner);

public:
	BOOL EditKeyPermissions(RegPath *pKey, RegItem* pItem, BOOL abVisual);
};
