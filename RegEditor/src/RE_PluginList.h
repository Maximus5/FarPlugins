
#pragma once

class REPluginList
{
public:
	REPluginList();
	~REPluginList();
	
	int Register(REPlugin* pPlugin);
	int Unregister(REPlugin* pPlugin);
	BOOL IsValid(REPlugin* pPlugin);
	REPlugin* GetOpposite(REPlugin* pPlugin);
	
	BOOL GetSIDName(LPCWSTR asComputer, PSID pSID, TCHAR* pszOwner, DWORD cchOwnerMax);
	
	BOOL ConnectRegedit(DWORD& nRegeditPID, HANDLE& hProcess, HWND& hParent, HWND& hTree, HWND& hList, BOOL& bHooked);
	
	REPlugin* Plugins[10];
	
	//void UpdateAllTitles();
	void OnSettingsChanged(DWORD abWow64on32);
protected:
	int mn_PlugCount;

	typedef struct tag_SidCache {
		PSID   pSID;
		TCHAR* pszOwner;
	} SidCache;
	SidCache *mp_SidCache;
	DWORD nMaxSidCount, nSidCount;
	
	// RegEdit
	DWORD mn_RegeditPID;
	HANDLE mh_RegeditProcess;
	HWND mh_RegeditParent, mh_RegeditTree, mh_RegeditList;
	BOOL mb_Hooked;
	DWORD FindRegeditPID(HWND& hParent, HWND& hTree, HWND& hList, BOOL& bHooked, HANDLE* phProcess);
	//int InjectHook(HANDLE hProcess);
};

extern REPluginList *gpPluginList;
