
#pragma once

struct RegItem; // REG_Folder.h

struct RegPath
{
	RegWorkType eType; // � ��� �� �������� (WinApi, *.reg, hive)
	//enum RegWorkType {
	//	RE_UNDEFINED = 0,  // �� ����������������
	//	RE_WINAPI,         // ������ � �������� ��������� ������ ����� WinApi
	//	RE_REGFILE,        // *.reg
	//	RE_HIVE,           // �hive� files
	//};
	
	HKEY        mh_Root; // �������� HKEY ��� WinApi (HKEY_CURRENT_USER, � �.�.)
	wchar_t    *mpsz_Key; // ���� ������������ hRoot (i.e. "Software\\Far2"), always Unicode!
	TCHAR      *mpsz_Title;
	TCHAR      *mpsz_Dir; // ���� ��� ����������� � ��������� (������� HKEY_CURRENT_USER\�)
	int         mn_MaxKeySize; // in TCHAR's
	//TCHAR      *mpsz_HostFile; // ��� *.reg � hive ������
	TCHAR      *mpsz_TitlePrefix; // ���.������� ��� ������ (��� ���� �����, ��� ��� �������)
	int         mn_PrefixLen;
	TCHAR      *mpsz_Server;
	int         mn_ServerLen;
	REGFILETIME ftModified;
	//BOOL        bDeletion;
	DWORD       nKeyFlags;  // KEYF_xxx
	LPCWSTR     pszComment; // ��� ������ ������. ������ �� ����������, �������� �� ����������!
	RegKeyOpenRights eRights;
	DWORD       mb_Wow64on32;
	BOOL        mb_Virtualize;
	
	// ������������� ����������
	void Init(RegPath* apKey);
	void Init(RegWorkType aType, DWORD abWow64on32, BOOL abVirtualize, HKEY ahRoot = NULL, LPCWSTR asKey = NULL, REGFILETIME* aftModified = NULL, LPCTSTR asPrefix = NULL, RegKeyOpenRights aRights = eRightsSimple, DWORD anKeyFlags = 0, LPCTSTR asServer = NULL);
	// Functions
	void SetTitlePrefix(LPCSTR asPrefix, BOOL abNoUpdate = FALSE);
	void SetTitlePrefix(LPCWSTR asPrefix, BOOL abNoUpdate = FALSE);
	void SetServer(LPCSTR asServer, BOOL abNoUpdate = FALSE);
	void SetServer(LPCWSTR asServer, BOOL abNoUpdate = FALSE);
	void Release(); // ���������� ������
	bool IsEmpty();
	bool IsEqual(struct RegPath *p); // �������� �����
	void AllocateAddLen(int anAddLen); // ���������, ��� �������� ���������� ������ ��� ��� ����
	void ReallocTitleDir();
	void Update(); // �������� ��� ���� � ������������ � ����� ������ (mpsz_Key)
	BOOL TestKey(LPCWSTR asSubKey);
	BOOL ChDir(LPCWSTR asKey, BOOL abSilence, REPlugin* pPlugin, BOOL abRawKeyName = FALSE);
	BOOL ChDir(RegItem* apKey, BOOL abSilence, REPlugin* pPlugin);
	BOOL IsKeyPredefined(HKEY ahKey);
};
