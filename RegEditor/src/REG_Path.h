
#pragma once

struct RegItem; // REG_Folder.h

struct RegPath
{
	RegWorkType eType; // с чем мы работаем (WinApi, *.reg, hive)
	//enum RegWorkType {
	//	RE_UNDEFINED = 0,  // не инициализировано
	//	RE_WINAPI,         // Работа с реестром локальной машины через WinApi
	//	RE_REGFILE,        // *.reg
	//	RE_HIVE,           // «hive» files
	//};
	
	HKEY        mh_Root; // корневой HKEY для WinApi (HKEY_CURRENT_USER, и т.п.)
	wchar_t    *mpsz_Key; // Путь относительно hRoot (i.e. "Software\\Far2"), always Unicode!
	TCHAR      *mpsz_Title;
	TCHAR      *mpsz_Dir; // путь для отображения в комстроке (включая HKEY_CURRENT_USER\…)
	int         mn_MaxKeySize; // in TCHAR's
	//TCHAR      *mpsz_HostFile; // для *.reg и hive файлов
	TCHAR      *mpsz_TitlePrefix; // Доп.префикс для панели (имя хост файла, или имя сервера)
	int         mn_PrefixLen;
	TCHAR      *mpsz_Server;
	int         mn_ServerLen;
	REGFILETIME ftModified;
	//BOOL        bDeletion;
	DWORD       nKeyFlags;  // KEYF_xxx
	LPCWSTR     pszComment; // Это ТОЛЬКО ссылка. Память не выделяется, значение не копируется!
	RegKeyOpenRights eRights;
	DWORD       mb_Wow64on32;
	BOOL        mb_Virtualize;
	
	// инициализация переменных
	void Init(RegPath* apKey);
	void Init(RegWorkType aType, DWORD abWow64on32, BOOL abVirtualize, HKEY ahRoot = NULL, LPCWSTR asKey = NULL, REGFILETIME* aftModified = NULL, LPCTSTR asPrefix = NULL, RegKeyOpenRights aRights = eRightsSimple, DWORD anKeyFlags = 0, LPCTSTR asServer = NULL);
	// Functions
	void SetTitlePrefix(LPCSTR asPrefix, BOOL abNoUpdate = FALSE);
	void SetTitlePrefix(LPCWSTR asPrefix, BOOL abNoUpdate = FALSE);
	void SetServer(LPCSTR asServer, BOOL abNoUpdate = FALSE);
	void SetServer(LPCWSTR asServer, BOOL abNoUpdate = FALSE);
	void Release(); // освободить память
	bool IsEmpty();
	bool IsEqual(struct RegPath *p); // сравнить ключи
	void AllocateAddLen(int anAddLen); // Убедиться, что выделено достаточно памяти под все поля
	void ReallocTitleDir();
	void Update(); // Обновить все поля в соответствии с новым ключом (mpsz_Key)
	BOOL TestKey(LPCWSTR asSubKey);
	BOOL ChDir(LPCWSTR asKey, BOOL abSilence, REPlugin* pPlugin, BOOL abRawKeyName = FALSE);
	BOOL ChDir(RegItem* apKey, BOOL abSilence, REPlugin* pPlugin);
	BOOL IsKeyPredefined(HKEY ahKey);
};
