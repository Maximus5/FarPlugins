
/*
    ����� ��� �������� ����������� ����� ���
    ������������� ����������� � ������ FAR
    ������������ ��� �����������
*/

#pragma once

struct RegFolder; // ��. ����
class RegFolderCache;

// Key or Value, ready for PluginPanelItem
struct RegItem
{
	DWORD          nMagic;     // REGEDIT_MAGIC == 0x52674564
	REGTYPE        nValueType; // REG_DWORD, REG_SZ, � �.�. ��� REG__KEY ��� ������
	const wchar_t  *pszName;   // ������ ��� pszName ���������� ������ �� ����������
	bool bDefaultValue;        // "@" ����� ��������� ��������� �����
	DWORD nFlags;              // ��������� ���� ��� �������� (REGF_xxx)
	TCHAR szDescription[128];  // ������ � ������� ���������� ������ ���
	TCHAR szOwner[64];
	// ������
	DWORD   nDataSize; // ����������� ������
	LPCBYTE ptrData;   // ����������� ������ ��� ������, ������ ���������� � RegFolder ����� �� ����
	//const PluginPanelItem* piSort;
	// ���������� ������ ��� ������� ��������
	TCHAR szTempDescription[128];
	TCHAR szTempOwner[64];
	// ���������� ������ ��� ������� ��������� ����� ����� ��� �������� ".."
	TCHAR szTwoDots[4];
	// ������ � Folder
	RegFolder* pFolder;
	DWORD nItemIndex;
	LPCWSTR pszComment; // ��� ������ ������. ������ �� ����������, �������� �� ����������!
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
	// ���
	RegPath key;
	#ifdef _DEBUG
	DWORD  nDummy;
	wchar_t sDbgKeyPath[MAX_PATH+1];
	#endif
	BOOL bForceRelease; // ��� ��� �����, ������� ��������� ��� ������
	// ���
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

	/* ** ������ ** */
	// ������������� � NULL
	void Init(RegPath* apKey);
	// ��������� (���� ���� ���������) ������ ��������� � ��������
	BOOL LoadKey(REPlugin* pPlugin, MRegistryBase* pWorker, KeyFirstEnum aKeysFirst, BOOL abForceReload, BOOL abSilence, BOOL abLoadDesc, MRegistryBase* file = NULL, HKEY hKeyWrite = NULLHKEY);
	// ���������, ��� �������� ���������� ������
	void AllocateItems(/*RegPath* apKey,*/ int anSubkeys, int anValues, int anMaxKeyLen, int anMaxValueNameLen);
	// �������� � ������, ��� ������������� - �������� ���������� ������
	BOOL AddItem(LPCWSTR aszName, int anNameLen, REGFILETIME aftModified, LPCTSTR asDesc, LPCTSTR asOwner, DWORD adwFileAttributes /*= FILE_ATTRIBUTE_DIRECTORY*/, REGTYPE anValueType /*= REG__KEY*/, DWORD anValueSize /*= 0*/, LPCBYTE apValueData, LPCWSTR apszComment /*= NULL*/);
	// ��������� �������. �������������� ��� ����������� ����������� � �������
	//void FormatDataVisual(REGTYPE nDataType, LPBYTE pData, DWORD dwDataSize, wchar_t *szDesc/*[128]*/);
	// ����� ���������� ���������, ������ �� ��������������
	void Reset();
	// ��������� ������� �������������
	int AddRef();
	// ��������� �������, ���� nRefCount < 1 - FinalRelease()
	int Release();
	// ���������, ����� �� �������� ������ � ����
	BOOL CheckLoadingFinished(BOOL abAlwaysCopyFromTemp = FALSE);
	void ResetLoadingFinished();
	// ���������, �� ��������� �� ������
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
	// ���������� ��� ������
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
