
#pragma once

class CAdjustProcessToken;
struct RegPath;

class RegConfig {
public:
	static void Init(
			 #if FAR_UNICODE>=1900
			 LPCTSTR asModuleName
			 #else
			 LPCTSTR asRootKey, LPCTSTR asModuleName
			 #endif
		);
	
	RegConfig(
			 #if FAR_UNICODE>=1900
			 LPCTSTR asModuleName
			 #else
			 LPCTSTR asRootKey, LPCTSTR asModuleName
			 #endif
		);
	~RegConfig();

	void LoadConfiguration();
	void SaveConfiguration();
	BOOL mb_SettingsChanged;
	BOOL Configure();
	BOOL ConfigureVisual();
	
	//DWORD samDesired_();
	BOOL  getWow64on32();
	//const TCHAR* bitSuffix();
	void LoadVersionInfo(LPCTSTR asModuleName);
	#ifdef _UNICODE
	UINT EditorCodePage();
	#endif

	UINT TitleAddLen();
	
	void SetLastRegPath(HKEY ahKey, LPCWSTR asSubKey);
public:
#ifndef FAR_UNICODE
	TCHAR *pszPluginKey;
#endif
	TCHAR sCommandPrefix[16];
	TCHAR sRegTitlePrefix[20];
	TCHAR sRegTitleDirty[20];
	BOOL  is64bitOs; // 64-битная операционка
	BOOL  isWow64process; // 32-битное приложение запущено в x64 среде
private:
	BOOL  bWow64on32_; // 2-Auto. 1-Отображать 64-битный реестр, 0-32-битный
public:
	//BOOL  bVirtualize; // отображать виртуализированные значения реестра
	BOOL  bAddToDisksMenu;
	TCHAR cDiskMenuHotkey[2];
	BOOL  bAddToPluginsMenu;
	BOOL  bBrowseRegFiles; // 0 - не открывать из панелей, 1 - меню [Browse/Import], 2 - только Browse
	BOOL  bBrowseRegHives;
	BOOL  bSkipAccessDeniedMessage; // 2-пытаться повысить привилегии, если не получилось - показать ошибку, 1-не показывать ошибку, 0-показывать ошибку
	BOOL  bUseBackupRestore;
	BOOL  bSkipPrivilegesDeniedMessage;
	BOOL  bShowKeysAsDirs;
	BOOL  bUnsortLargeKeys; DWORD nLargeKeyCount;
	BOOL  bCreateUnicodeFiles;
	DWORD nAnsiCodePage;
	BOOL  bEditBinaryAsText;
	BOOL  bLoadDescriptions; DWORD nLargeKeyTimeout;
	BOOL  bExportDefaultValueFirst;
	BOOL  bConfirmImport, bShowImportResult;
	BOOL  bEscapeRNonExporting;
	BOOL  bCheckMacroSequences; // "sequence"
	BOOL  bCheckMacrosInVars;   // "%%variable"
	BOOL  bRefreshChanges; // 0-только по CtrlR, 1-автоматически
	DWORD nRefreshKeyTimeout, nRefreshSubkeyTimeout;
	BOOL  bRestorePanelMode;
	wchar_t* pszLastRegPath; int nMaxRegPath; // полный путь вроде (HKEY_CURRENT_USER\\Software\\FAR2)
	TCHAR sVersionInfo[128];

	// Не сохраняются в реестр
	BOOL  bUseInternalBookmarks; // TRUE - наши закладки, FALSE - закладки из RegEdit.exe

protected:
	CAdjustProcessToken *mp_Token;
	int mn_TokenRef; BOOL mb_TokenWarnedOnce, mb_RestoreAquired;
public:
	BOOL BackupPrivilegesAcuire(BOOL abRequireRestore);
	void BackupPrivilegesRelease();
	//LPCTSTR GetActivePrivileges(); // Для заголовка панели
	
protected:
	DWORD    cchBookmarksMaxCount;
	DWORD    cchBookmarksLen;
	wchar_t* pszBookmarks;
	wchar_t  szBookmarksValueName[64];
public:
	void BookmarksGet(RegPath* pKey, BOOL bRemote, TCHAR** ppszBookmarks, DWORD* pnBookmarksCount);
	void BookmarkInsert(int nPos, RegPath* pKey);
	void BookmarkDelete(int nPos);
	void BookmarksEdit();
	DWORD BookmarksLen(DWORD* pnBookmarksCount);
	void BookmarksReset(RegPath* pKey, BOOL bRemote);
protected:
	void BookmarksLoadInt(RegPath* pKey, BOOL bRemote);
	void BookmarkInsertInt(int nPos, RegPath* pKey, BOOL abSaveImmediate = TRUE);
	void BookmarkInsertInt(int nPos, LPCWSTR pszNewKey, BOOL abSaveImmediate = TRUE);
	void BookmarkDeleteInt(int nPos);
	void BookmarksSaveInt();
	void BookmarksLoadExt(RegPath* pKey, BOOL bRemote);
	void BookmarkInsertExt(int nPos, RegPath* pKey);
	void BookmarkDeleteExt(int nPos);
	void BookmarkParse(LPCWSTR pszNew, DWORD cchNew);
	
public:
	BOOL  bSomeValuesMissed;
};

extern RegConfig *cfg;
