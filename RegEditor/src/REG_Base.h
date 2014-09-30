
#pragma once

#ifdef _DEBUG
	#define VIRTUAL_FUNCTION = 0
#else
	#define VIRTUAL_FUNCTION { InvalidOp(); return -1; }
#endif

class MRegistryBase
{
protected:
	RegFolderCache m_Cache;
public:
	//enum RegWorkType {
	//	RE_UNDEFINED = 0,  // не инициализировано
	//	RE_WINAPI,         // Работа с реестром локальной машины через WinApi
	//	RE_REGFILE,        // *.reg
	//	RE_HIVE,           // «hive» files
	//};
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH+2]; // L"\\Server"
	wchar_t sRemoteResource[MAX_PATH*2]; // ресурс, к которому логинимся, например L"\\Server\IPC$"
	bool bDirty;
	BOOL mb_Wow64on32;
	BOOL mb_Virtualize;
	//DWORD samDesired();
public:
	MRegistryBase(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MRegistryBase();
	virtual void ConnectLocal();
	virtual BOOL ConnectRemote(LPCWSTR asServer, LPCWSTR asLogin = NULL, LPCWSTR asPassword = NULL, LPCWSTR asResource = NULL);
	#ifndef _UNICODE
	BOOL ConnectRemote(LPCSTR asServer, LPCSTR asLogin = NULL, LPCSTR asPassword = NULL, LPCSTR asResource = NULL);
	#endif
	//static MRegistryBase* CreateWorker(RegWorkType aType, bool abRemote, LPCWSTR asRemoteServer, LPCWSTR asRemoteLogin, LPCWSTR asRemotePassword);
public:
	RegFolder* GetFolder(RegPath* apKey, u64 OpMode);
	void FreeFindData(struct PluginPanelItem *PanelItem,int ItemsNumber);
	DWORD GetAllowedImportStyles();
private:
	LPBYTE mp_TempExportBuffer; DWORD mn_TempExportBuffer;
public:
	// ***
	virtual MRegistryBase* Duplicate() {return NULL;};// = 0;
	virtual BOOL IsPredefined(HKEY hKey);
	virtual LPBYTE GetExportBuffer(DWORD cbSize);
	virtual BOOL AllowCaching() { return TRUE; };
	//virtual LPCWSTR RootKeyName(HKEY hk);
	virtual LONG RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged) VIRTUAL_FUNCTION;
	
	//virtual LONG NotifyChangeKeyValue(RegFolder *pFolder, HKEY hKey) { return NULL; };
	// Wrappers
	virtual LONG CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL, LPCWSTR pszComment = NULL) VIRTUAL_FUNCTION;
	virtual LONG OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL) VIRTUAL_FUNCTION;
	virtual LONG CloseKey(HKEY* phKey) VIRTUAL_FUNCTION;
	virtual LONG QueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime) VIRTUAL_FUNCTION;
	virtual LONG EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment = NULL) VIRTUAL_FUNCTION;
	virtual LONG EnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime, DWORD* pnKeyFlags = NULL, TCHAR* lpDefValue = NULL, DWORD cchDefValueMax = 0, LPCWSTR* ppszKeyComment = NULL) VIRTUAL_FUNCTION;
	virtual LONG QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment = NULL) VIRTUAL_FUNCTION;
	virtual LONG SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment = NULL) VIRTUAL_FUNCTION;
	virtual LONG DeleteValue(HKEY hKey, LPCWSTR lpValueName) VIRTUAL_FUNCTION;
	virtual LONG DeleteSubkeyTree(HKEY hKey, LPCWSTR lpSubKey) VIRTUAL_FUNCTION;
	virtual LONG ExistValue(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpValueName, REGTYPE* pnDataType = NULL, DWORD* pnDataSize = NULL) VIRTUAL_FUNCTION;
	virtual LONG ExistKey(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpSubKey) VIRTUAL_FUNCTION;
	virtual LONG SaveKey(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL) VIRTUAL_FUNCTION;
	virtual LONG RestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags = 0) VIRTUAL_FUNCTION;
	virtual LONG GetSubkeyInfo(HKEY hKey, LPCWSTR lpszSubkey, LPTSTR pszDesc, DWORD cchMaxDesc, LPTSTR pszOwner, DWORD cchMaxOwner) VIRTUAL_FUNCTION;


public:
	// Service - TRUE, если права были изменены
	virtual BOOL EditKeyPermissions(RegPath *pKey, RegItem* pItem, BOOL abVisual) {return FALSE;}; // = 0;
	// ppSD - LocalAlloc'ed
	virtual LONG GetKeySecurity(HKEY hkRoot, LPCWSTR lpszKey, HKEY* phKey, HKEY* phParentKey, SECURITY_INFORMATION *pSI, PSECURITY_DESCRIPTOR *ppSD, SECURITY_INFORMATION *pWriteSI) {return -1;}; // = 0;
	virtual LONG GetKeySecurity(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR *ppSD) {return -1;}; // = 0;
	virtual LONG SetKeySecurity(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD) {return -1;}; // = 0;
};
