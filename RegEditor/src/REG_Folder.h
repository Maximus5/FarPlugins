
/*
    Класс для загрузки содержимого ключа для
    последнующего отображения в панели FAR
    Используется при кешировании
*/

#pragma once

struct RegFolder; // см. ниже
class RegFolderCache;

// Key or Value, ready for PluginPanelItem
struct RegItem
{
	DWORD          nMagic;     // REGEDIT_MAGIC == 0x52674564
	REGTYPE        nValueType; // REG_DWORD, REG_SZ, и т.п. или REG__KEY для ключей
	const wchar_t  *pszName;   // память под pszName выделяется вместе со структурой
	bool bDefaultValue;        // "@" чтобы различать строковые имена
	DWORD nFlags;              // Удаленный ключ или значение (REGF_xxx)
	TCHAR szDescription[128];  // больше в панелях показывать смысла нет
	TCHAR szOwner[64];
	// Данные
	DWORD   nDataSize; // заполняется всегда
	LPCBYTE ptrData;   // заполняется только при поиске, память выделяется в RegFolder чохом на ключ
	//const PluginPanelItem* piSort;
	// Выделенная память для фоновой загрузки
	TCHAR szTempDescription[128];
	TCHAR szTempOwner[64];
	// Выделенная память для подмены реального имени ключа или значения ".."
	TCHAR szTwoDots[4];
	// Индекс и Folder
	RegFolder* pFolder;
	DWORD nItemIndex;
	LPCWSTR pszComment; // Это ТОЛЬКО ссылка. Память не выделяется, значение не копируется!
};

struct RegFolderThreadArg
{
	RegFolder* pFolder;
	REPlugin* pPlugin;
	MRegistryBase* pWorker;
	DWORD nMaxValueLen;
	LPBYTE pData;
	DWORD nLastProcessedItems;
};

struct RegFolder
{
	// Где
	RegPath key;
	#ifdef _DEBUG
	DWORD  nDummy;
	wchar_t sDbgKeyPath[MAX_PATH+1];
	#endif
	BOOL bForceRelease; // Это для папок, которые создаются при поиске
	// Что
	DWORD mn_ItemCount, mn_MaxItemCount;
	RegItem* mp_Items;
	bool bHaveDefaultValue;
	PluginPanelItem* mp_PluginItems;
	size_t mn_HolderLen, mn_MaxHolderSize;
	wchar_t* mpsz_NamesHolder;
	size_t mn_DataHolderLen, mn_MaxDataHolderSize;
	LPBYTE mptr_DataHolder;
	BOOL bShowKeysAsDirs;
	//HANDLE mh_ChangeNotify;
	DWORD mn_LastKeyStamp, mn_LastSubkeyStamp;
	BOOL  mb_RegistryChanged, mb_MonitorRegistryChange, mb_ForceRegistryCheck;
	REGFILETIME mft_LastSubkey;

	RegFolder()
	{
		memset(this, 0, sizeof(RegFolder));
		nRefCount = 1;
	}
	~RegFolder()
	{
		_ASSERTE(nRefCount == 0);
		//_ASSERTE(mp_Items == NULL && mp_PluginItems == NULL);
		if (key.eType != RE_UNDEFINED)
			FinalFree();
	}

	/* ** Методы ** */
	// инициализация в NULL
	void Init(RegPath* apKey);
	// Загрузить (если были изменения) список подключей и значений
	BOOL LoadKey(REPlugin* pPlugin, MRegistryBase* pWorker, KeyFirstEnum aKeysFirst, BOOL abForceReload, BOOL abSilence, BOOL abLoadDesc, MRegistryBase* file = NULL, HKEY hKeyWrite = NULLHKEY);
	// убедиться, что выделено достаточно памяти
	void AllocateItems(/*RegPath* apKey,*/ int anSubkeys, int anValues, int anMaxKeyLen, int anMaxValueNameLen);
	// добавить в список, при необходимости - увеличит выделенную память
	BOOL AddItem(LPCWSTR aszName, int anNameLen, REGFILETIME aftModified, LPCTSTR asDesc, LPCTSTR asOwner, DWORD adwFileAttributes /*= FILE_ATTRIBUTE_DIRECTORY*/, REGTYPE anValueType /*= REG__KEY*/, DWORD anValueSize /*= 0*/, LPCBYTE apValueData, LPCWSTR apszComment /*= NULL*/);
	// Сервисная функция. Форматирования для визуального отображения в панелях
	//void FormatDataVisual(REGTYPE nDataType, LPBYTE pData, DWORD dwDataSize, wchar_t *szDesc/*[128]*/);
	// сброс количества элементов, память не освобождвается
	void Reset();
	// Увеличить счетчик использований
	int AddRef();
	// Уменьшить счетчик, если nRefCount < 1 - FinalRelease()
	int Release();
	// Проверить, нужно ли обновить панель в фаре
	BOOL CheckLoadingFinished(BOOL abAlwaysCopyFromTemp = FALSE);
	void ResetLoadingFinished();
	// Проверить, не изменился ли реестр
	void MonitorRegistryChange(BOOL abEnabled);
	BOOL CheckRegistryChanged(REPlugin *pPlugin, MRegistryBase* pWorker, HKEY hKey = NULL);
	void ResetRegistryChanged();

	// Wrappers
	LONG OpenKey(MRegistryBase* pWorker, HKEY* phKey, DWORD anRights = KEY_READ);
	LONG CreateKey(MRegistryBase* pWorker, HKEY* phKey, DWORD anRights /*= (KEY_READ|KEY_WRITE)*/);
	void CloseKey(MRegistryBase* pWorker, HKEY* phKey);
	
	// Serialize
	BOOL ExportToFile(REPlugin* pPlugin, MRegistryBase* pWorker, MRegistryBase* file, BOOL abUnicode);
	BOOL Transfer(REPlugin* pPlugin, MRegistryBase* pSrcWorker, RegFolder *pDstFolder, MRegistryBase* pDstWorker);
	//BOOL ExportToFileAnsi(REPlugin* pPlugin, MRegistryBase* pWorker, MRegistryBase* file);
	//BOOL ExportToFileUnicode(REPlugin* pPlugin, MRegistryBase* pWorker, MRegistryBase* file);
	
private:
	//friend class RegFolderCache;
	int nRefCount;
	// освободить всю память
	void FinalFree(); // void FinalRelease();
	//
	void UpdateShowKeysAsDirs();
	BOOL CreateDescReadThread(REPlugin* pPlugin, MRegistryBase* pWorker);
	HANDLE hDescReadThread; DWORD nDescReadThreadId; BOOL bRequestStopReadThread;
#ifdef _DEBUG
	public:
#endif
	RegFolderThreadArg args;
#ifdef _UNICODE
	SynchroArg m_SyncArg;
#endif
private:
	static DWORD WINAPI DescReadThread(LPVOID lpParameter);
	void RequireItemsLock();
	BOOL bDescrWasUpdated;
	DWORD cSubKeys; // = 0;
	DWORD cMaxSubKeyLen; // = 0;
	DWORD cValues; // = 0;
	DWORD cMaxValueNameLen; // = 0;
	DWORD cMaxValueLen; // = 0;
};
