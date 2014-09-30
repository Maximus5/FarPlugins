
#pragma once

typedef LONG (WINAPI* RegDeleteKeyExW_t)(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired, DWORD Reserved);
typedef LONG (WINAPI* RegRenameKey_t)(HKEY hKey, LPCWSTR lpSubKeyName, LPCWSTR lpNewKeyName);

class MRegistryWinApi : public MRegistryBase
{
protected:
	struct {
		HKEY    hKey; // Predefined HKEY_xxx
		HREGKEY hKeyRemote; // Connected HKEY
		wchar_t szKey[32];
		bool    bSlow;
	} hkPredefined[10];
	DWORD nPredefined;
	BOOL  bTokenAquired;
	HREGKEY  hAcquiredHkey;
	/* 
	**** унаследованы от MRegistryBase ***
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH+2], sRemoteLogin[MAX_PATH], sRemoteLogin[MAX_PATH];
	**************************************
	*/
	//HANDLE hRemoteLogon; // Token для пароля
private:
	LONG ConnectRegistry(HKEY hKey, HKEY* phkResult);
	void FillPredefined();
	RegDeleteKeyExW_t _RegDeleteKeyEx;
	RegRenameKey_t _RegRenameKey;
	HMODULE hAdvApi;
	LONG OpenCreateVirtualKey(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, BOOL abCreate, HKEY* phkResult);
public:
	MRegistryWinApi(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MRegistryWinApi();
	virtual void ConnectLocal();
	virtual BOOL ConnectRemote(LPCWSTR asServer, LPCWSTR asLogin = NULL, LPCWSTR asPassword = NULL, LPCWSTR asResource = NULL);
public:
	// ***
	virtual MRegistryBase* Duplicate();
	virtual BOOL IsPredefined(HKEY hKey);
	//virtual LONG NotifyChangeKeyValue(RegFolder *pFolder, HKEY hKey);
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
	// Service - TRUE, если права были изменены
	virtual BOOL EditKeyPermissions(RegPath *pKey, RegItem* pItem, BOOL abVisual);
	// ppSD - LocalAlloc'ed
	virtual LONG GetKeySecurity(HKEY hkRoot, LPCWSTR lpszKey, HKEY* phKey, HKEY* phParentKey, SECURITY_INFORMATION *pSI, PSECURITY_DESCRIPTOR *ppSD, SECURITY_INFORMATION *pWriteSI);
	virtual LONG GetKeySecurity(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR *ppSD);
	virtual LONG SetKeySecurity(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD);
};
