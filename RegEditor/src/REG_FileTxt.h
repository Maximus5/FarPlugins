
#pragma once

/*

	Объект предназначен для работы с текстовыми (Ansi/Unicode) и бинарными файлами.

*/

class MFileTxt : public MRegistryBase
{
protected:
	/* 
	**** унаследованы от MRegistryBase ***
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH];
	**************************************
	*/
	HANDLE hFile;
	BOOL   bOneKeyCreated;
	BOOL   bUnicode;
	wchar_t *psTempDirectory;
	wchar_t *psFilePathName;
	TCHAR *psShowFilePathName;
	//BOOL   bKeyWasCreated;
	// Write buffer (cache)
	LPBYTE ptrWriteBuffer; DWORD nWriteBufferLen, nWriteBufferSize;
	// Value export buffer
	LPBYTE pExportBufferData; DWORD cbExportBufferData;
	LPBYTE pExportFormatted;  DWORD cbExportFormatted;
	LPBYTE pExportCPConvert;  DWORD cbExportCPConvert; DWORD nAnsiCP;
	wchar_t* pszExportHexValues;
	// I/O result
	BOOL bLastRc; DWORD nLastErr;
public:
	MFileTxt(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MFileTxt();
	// Text file operations
	BOOL FileCreateTemp(LPCWSTR asDefaultName, LPCWSTR asExtension, BOOL abUnicode);
	BOOL FileCreate(LPCWSTR asPath/* may contain filename */, LPCWSTR asDefaultName, LPCWSTR asExtension, BOOL abUnicode, BOOL abConfirmOverwrite, BOOL abNoBOM = FALSE);
	virtual void FileClose();
	void FileDelete();
	BOOL FileWrite(LPCWSTR aszText, int anLen=-1);
	virtual BOOL FileWriteHeader(MRegistryBase* pWorker) = 0;
	virtual BOOL FileWriteValue(LPCWSTR pszValueName, REGTYPE nDataType, const BYTE* pData, DWORD nDataSize, LPCWSTR pszComment) = 0;
	BOOL FileWriteMSZ(LPCWSTR aszText, DWORD anLen);
	BOOL FileWriteBuffered(LPCVOID apData, DWORD nDataSize); // Binary data
	BOOL Flush();
	LPBYTE GetExportBuffer(DWORD cbSize); // --> pExportBufferData
	LPCWSTR GetFilePathName();
	LPCTSTR GetShowFilePathName();
	static LONG LoadText(LPCWSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize);
	static LONG LoadTextMSZ(LPCWSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize);
	static LONG LoadData(LPCWSTR asFilePathName, void** pData, DWORD* pcbSize, size_t ncbMaxSize = -1);
	#ifndef _UNICODE
	static LONG LoadText(LPCSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize);
	static LONG LoadTextMSZ(LPCSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize);
	static LONG LoadData(LPCSTR asFilePathName, void** pData, DWORD* pcbSize);
	#endif
	void ReleasePointers();
protected:
	BOOL FileCreateApi(LPCWSTR asFilePathName, BOOL abUnicode, BOOL abAppendExisting, BOOL abNoBOM = FALSE);
	LPBYTE GetFormatBuffer(DWORD cbSize); // --> pExportFormatted
	LPBYTE GetConvertBuffer(DWORD cbSize); // --> pExportCPConvert
public:
	static BOOL bBadMszDoubleZero;
public:
	// ***
	virtual MRegistryBase* Duplicate();
	//virtual LONG NotifyChangeKeyValue(RegFolder *pFolder, HKEY hKey);
	virtual LONG RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged) { return -1; };
	// Wrappers
	virtual LONG CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL, LPCWSTR pszComment = NULL) = 0;
	virtual LONG OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL) { return -1; };
	virtual LONG CloseKey(HKEY* phKey) { *phKey = NULL; return 0; };
	virtual LONG QueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime) { return -1; };
	virtual LONG EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment = NULL) { return -1; };
	virtual LONG EnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime, DWORD* pnKeyFlags = NULL, TCHAR* lpDefValue = NULL, DWORD cchDefValueMax = 0, LPCWSTR* ppszKeyComment = NULL) { return -1; };
	virtual LONG QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment = NULL) { return -1; };
	virtual LONG SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment = NULL);
	virtual LONG DeleteValue(HKEY hKey, LPCWSTR lpValueName) {return -1;}; // = 0;
	virtual LONG DeleteSubkeyTree(HKEY hKey, LPCWSTR lpSubKey) {return -1;}; // = 0;
	virtual LONG ExistValue(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpValueName, REGTYPE* pnDataType = NULL, DWORD* pnDataSize = NULL) { return -1;}; // = 0;
	virtual LONG ExistKey(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpSubKey) { return -1;}; // = 0;
	virtual LONG SaveKey(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL) { return -1;}; // = 0;
	virtual LONG RestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags = 0) { return -1;}; // = 0;
	virtual LONG GetSubkeyInfo(HKEY hKey, LPCWSTR lpszSubkey, LPTSTR pszDesc, DWORD cchMaxDesc, LPTSTR pszOwner, DWORD cchMaxOwner) { return -1;};
};


class MFileTxtReg : public MFileTxt
{
protected:
	/* 
	**** унаследованы от MRegistryBase ***
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH];
	**************************************
	*/
	
	/* 
	**** унаследованы от MFileTxt ***
	HANDLE hFile;
	BOOL   bOneKeyCreated;
	BOOL   bUnicode;
	wchar_t *psTempDirectory;
	wchar_t *psFilePathName;
	TCHAR *psShowFilePathName;
	//BOOL   bKeyWasCreated;
	// Write buffer (cache)
	LPBYTE ptrWriteBuffer; DWORD nWriteBufferLen, nWriteBufferSize;
	// Value export buffer
	LPBYTE pExportBufferData; DWORD cbExportBufferData;
	LPBYTE pExportFormatted;  DWORD cbExportFormatted;
	LPBYTE pExportCPConvert;  DWORD cbExportCPConvert; DWORD nAnsiCP;
	wchar_t* pszExportHexValues;
	// I/O result
	BOOL bLastRc; DWORD nLastErr;
	**************************************
	*/

public:
	MFileTxtReg(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MFileTxtReg();
	// Text file operations
	virtual BOOL FileWriteHeader(MRegistryBase* pWorker);
	virtual BOOL FileWriteValue(LPCWSTR pszValueName, REGTYPE nDataType, const BYTE* pData, DWORD nDataSize, LPCWSTR pszComment);
public:
	// Wrappers
	virtual LONG CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL, LPCWSTR pszComment = NULL);
};

class MFileTxtCmd : public MFileTxt
{
protected:
	/* 
	**** унаследованы от MRegistryBase ***
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH];
	**************************************
	*/
	
	/* 
	**** унаследованы от MFileTxt ***
	HANDLE hFile;
	BOOL   bOneKeyCreated;
	BOOL   bUnicode;
	wchar_t *psTempDirectory;
	wchar_t *psFilePathName;
	TCHAR *psShowFilePathName;
	//BOOL   bKeyWasCreated;
	// Write buffer (cache)
	LPBYTE ptrWriteBuffer; DWORD nWriteBufferLen, nWriteBufferSize;
	// Value export buffer
	LPBYTE pExportBufferData; DWORD cbExportBufferData;
	LPBYTE pExportFormatted;  DWORD cbExportFormatted;
	LPBYTE pExportCPConvert;  DWORD cbExportCPConvert; DWORD nAnsiCP;
	wchar_t* pszExportHexValues;
	// I/O result
	BOOL bLastRc; DWORD nLastErr;
	**************************************
	*/
	
	char *psKeyBuffer; DWORD nKeyBufferSize;
	HKEY mh_LastRoot; wchar_t* ms_LastSubKey; DWORD mn_LastSubKeyCch, mn_LastSubKeyLen;
	//char *psUtfBuffer; DWORD nUtfBufferSize;
	
	char* ms_RECmdHeader;   // "@echo off\r\nrem \r\nrem Reg.exe has several restrictions\r\nrem * REG_QWORD (and other 'specials') is not supported\r\nrem * Strings, value names and keys can't contains \\r\\n\r\nrem * REG_MULTI_SZ may fails, if value contains \\\\0\r\nrem * Empty keys can't be created w/o default value\r\nrem \r\n"
	int   mn_RECmdHeader;
	char* ms_RECmdVarReset; // "set FarRegEditRc=""""\r\n"
	int   mn_RECmdVarReset;
	char* ms_RECmdUTF8Warn; // "\r\nrem Required for Unicode string support"
	int   mn_RECmdUTF8Warn;
	char* ms_RECmdErrCheck; // "if errorlevel 1 if %FarRegEditRc%=="""" set FarRegEditRc="
	int   mn_RECmdErrCheck;
	char* ms_RECmdErrFin;   // "if NOT %FarRegEditRc%=="""" echo At least on value/key failed: %FarRegEditRc%\r\n"
	int   mn_RECmdErrFin;
	
	BOOL bRequestUnicode;

protected:
	LPCSTR PrepareKey(HKEY hKey, LPCWSTR lpSubKey);
	BOOL PrepareCmd(char*& psz, LPCSTR psVerb, LPCSTR psKeyUTF8);
	BOOL PrepareString(char*& psz, LPCWSTR psText, int nLen, int nCchLeft);
	BOOL PrepareMSZ(char*& psz, LPCWSTR psText, int nLen, int nCchLeft, LPCWSTR pszValueName);
	BOOL mb_KeyWriteWaiting, mb_LastKeyHasValues;
	BOOL WriteCurrentKey(BOOL bKeyDeletion, BOOL bAddRN);
	BOOL WriteRegCheck(BOOL bDeletion, LPCSTR psValueName);
public:
	MFileTxtCmd(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MFileTxtCmd();
	// Text file operations
	virtual BOOL FileWriteHeader(MRegistryBase* pWorker);
	virtual BOOL FileWriteValue(LPCWSTR pszValueName, REGTYPE nDataType, const BYTE* pData, DWORD nDataSize, LPCWSTR pszComment);
	virtual void FileClose();
public:
	// Wrappers
	virtual LONG CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights = NULL, LPCWSTR pszComment = NULL);
};
