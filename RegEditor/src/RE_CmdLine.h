
#pragma once

class OpenPluginArg
{
public:
	eCommandAction eAction; // aInvalid = 0, aBrowseLocal, aBrowseRemote, aBrowseFileReg, aBrowseFileHive, aExportKeysValues, aImportRegFile
	union {
		struct {
			wchar_t* wsBrowseLocalKey;
		};
		struct {
			wchar_t* wsBrowseRemoteKey;
			wchar_t* wsBrowseRemoteServer;
			wchar_t* wsBrowseRemoteLogin;
			wchar_t* wsBrowseRemotePassword;
		};
		struct {
			wchar_t* wsBrowseFileName; // aBrowseFileReg, aBrowseFileHive
		};
		struct {
			eExportFormat nExportFormat; // eExportReg4 = 0, eExportReg5 = 1, eExportHive = 2,
			int nExportKeysOrValuesCount;
			wchar_t** ppszExportKeysOrValues;
			wchar_t*  pszExportDestFile;
		};
		struct {
			wchar_t* pszSourceFile;
		};
		struct {
			wchar_t* pszMountHiveFilePathName;
			wchar_t* pszMountHiveKey;
			HKEY hRootMountKey;
			LPCWSTR pszMountHiveSubKey;
		};
		struct {
			wchar_t* pszUnmountHiveKey;
			HKEY hRootUnmountKey;
			LPCWSTR pszUnmountHiveSubKey;
		};
	};

	OpenPluginArg();
	void ReleaseMem();
	~OpenPluginArg();
	// Регистронезависимое сравнение символа
	//#define CHREQ(s,i,c) ( ((((DWORD)(s)[i])|0x20) == (((DWORD)c)|0x20)) || ((((DWORD)(s)[i])&~0x20) == (((DWORD)c)&~0x20)) )
	//bool CHREQ(const wchar_t* s, int i, wchar_t c);
	bool IsFilePath(LPCWSTR apsz);
	//bool IsRegPath(LPCWSTR apsz, HKEY* phRootKey = NULL, LPCWSTR* ppszSubKey = NULL, BOOL abCheckExist = FALSE);
	void SkipSpaces(wchar_t*& psz);
	// Получить токен
	wchar_t* GetToken(wchar_t*& psz, BOOL abForceLast = FALSE);
	// Разбор
	int ParseCommandLine(LPCTSTR asCmdLine);
};
