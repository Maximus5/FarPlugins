
#pragma once

//#define KEEP_LOWERS
#define USE_CRC_CACHE
#define USE_CRC_CACHE_VAL

/*

	������ ������������ ��� �������� ������ � �������
	REGEDIT4/REGEDIT5 � "����������" ���������.
	� ����������� ������� - ��� �� ��������, �������� MFileTxt.

*/

#define FILEREGITEM_MAGIC 0xFA26B7C5

struct FileRegItem;

#ifdef USE_CRC_CACHE
#define CRC_CACHE_COUNT  256
#ifndef _DEBUG
	#define CRC_CACHE_FRINGE 128
#else
	#define CRC_CACHE_FRINGE 16
#endif
struct FileRegCrcCache
{
	int              IsUsed; // 0 - �� ����������������, 1 - ������������, 2 - ��������� (����� �������� ���������?)
	// WARNING!!     ������ ���������� ������ - ������ CRC_CACHE_COUNT ���������
	DWORD           *pCRC32;
	FileRegItem    **ppItems;
	int              nItemsCount;
	// � ��� ��������� �� ��������� ����
	FileRegCrcCache *pNextBlock;
	// � ������ �����, ��� ��������� ����������, ������ ������ �� �������� ���� (����� ��������� �� ����)
	FileRegCrcCache *pActiveBlock;
	
	FileRegCrcCache* InitializeFrom(FileRegItem *pFirstItem);
	void AddNewItem(FileRegItem *pItem, DWORD nCRC32);
	FileRegItem* FindItem(const wchar_t *apszName, DWORD anNameCRC32);
	void DeleteItem(FileRegItem *pItem);
};
#endif

struct FileRegItem
{
	DWORD       nMagic;     // FILEREGITEM_MAGIC ��� �������� ����������
	REGTYPE     nValueType; // REG_DWORD, REG_SZ, � �.�. ��� REG__KEY ��� ������
	//bool  bDefaultValue;  // "@" ����� ��������� ��������� �����
	HKEY        hPredefined;// ��� ���������������� ������ (HKEY_xxx)
	LPCWSTR     pszName;    // ������ ���������� ������ �� ����������
	#ifdef KEEP_LOWERS
	LPCWSTR		pszNameLwr; // ������ ���������� ������ �� ����������
	#endif
	int         NameLen;    // ����� �����
	DWORD       NameCRC32;  // CRC32 ��� ����� � ������ �������� (��� �������� ������)
	BOOL        bDeletion;  // �������� ������ [-HKEY_...] ��� �������� "ValueName"=-
	BOOL        bIndirect;  // TRUE ��� ������, ������� ���������� �� �����, FALSE - 
	LPCWSTR     pszComment; // ����������� � ��� �� ������, ��� � ��������
	DWORD       cbCommSize; // ������ ���������� ������ ��� ����������� � ������
	const BYTE *pData;      // ������ ���������� ������ �� ����������, ��� ��� pTempBuffer
	DWORD       cbDataSize; // ������ ������ � ������
	REGFILETIME ftCreationTime;
	// Tree support
	FileRegItem *pParent;
	//FileRegItem *pFirstChild, *pLastChild;
	FileRegItem *pFirstKey, *pLastKey;
	FileRegItem *pFirstVal, *pLastVal, *pDefaultVal;
	FileRegItem *pNextSibling, *pPrevSibling;
	// ���������� � �����
	DWORD nKeyCount, nMaxKeyNameLen;
	// _ASSERTE(nValCommentCount >= nValCount)
	DWORD nValCount, nValCommentCount, nMaxValNameLen, nMaxValDataSize;
	// ����������� �������
	FileRegItem *pCachedFindKey, *pCachedFindVal;
	#ifdef USE_CRC_CACHE
	FileRegCrcCache *pCrcKey;
	#ifdef USE_CRC_CACHE_VAL
	FileRegCrcCache *pCrcVal;
	#endif
	#endif
	DWORD dwKeyIndex; FileRegItem *pKeyIndex;
	DWORD dwValIndex; FileRegItem *pValIndex; BOOL bValIndexComment/*TRUE - ���� ������������ � �����������!*/;
	// Temp buffers, ����� ������ ������ �����/�������� ��������� ���������� �����
	//DWORD nNameBufferSize; wchar_t* pszTempBuffer;
	DWORD nDataBufferSize; LPBYTE   pTempBuffer;
	// ��� ���� ��� - ��������� ������ � �������� ���������
	wchar_t* pszFullPath; DWORD nFullPathSize, nFullPathLen;
	FileRegItem* pLastCreatedKey;
};

//struct MFileRegMemoryBlock
//{
//	__int64 nBlockSize, nBlockFree;
//	LPBYTE ptrBlock, ptrFirstFree;
//};

class MFileReg : 
		public MRegistryBase,
		public MInternalMalloc
{
protected:
	/* 
	**** ������������ �� MRegistryBase ***
	RegWorkType eType; // � ��� �� �������� (WinApi, *.reg, hive)
	bool bRemote;
	wchar_t sRemoteServer[MAX_PATH];
	**************************************
	*/
	BOOL   bUnicode; // TRUE ��� ������� REGEDIT5 (����� ������ ������������ 2-�� �������), FALSE (������ - 1 ����)
	BOOL   bFirstFileFormatUnicode; // ��� ������������ ���������� (ShiftF2) ��������� ������
	wchar_t *psRegFilePathName;
	TCHAR *psShowFilePathName;
	// Value import (hex convert) buffer
	LPBYTE pImportBufferData; size_t cbImportBufferData;
	short nValueFromHex[0x100]; // '0' -> 0, 'A' -> 10, 'y' (invalid char) -> -1
	//BOOL   bInternalRegParsing; // ��� �������� � ������ - ������ �������� ����� ��������� - ����� �� ������
	// I/O result
	//LONG nLastErr;
	// *.reg Trees
	FileRegItem TreeRoot;
	//REGFILETIME ftCreationTime;
	wchar_t ms_LowerBuff[32768];
	wchar_t* mps_KeyPathLowerBuff; DWORD mn_KeyPathLowerBuff;
	LONG TreeCreateKey(FileRegItem* pParent, const wchar_t *apszKeyPath, DWORD anPathLen,
		BOOL abAllowCreate, BOOL abDeletion, BOOL abInternal, FileRegItem** ppKey, LPDWORD lpdwDisposition = NULL, LPCWSTR apszComment = NULL, int anCommentLen = -1);
	FileRegItem* TreeAddChild(FileRegItem* apParent, REGTYPE anValueType, HKEY ahPredefinedKey,
		const wchar_t *apszName, const wchar_t *apszNameLwr, int anNameLen, DWORD anNameCRC32,
		const BYTE *apData, DWORD acbDataSize, LPCWSTR lpComment = NULL, int anCommentLen = -1, BOOL abDeletion = FALSE);
	FileRegItem* TreeSetValue(FileRegItem* pParent,
		LPCWSTR apszName, LPCWSTR apszNameLwr, int anNameLen, DWORD anNameCRC32,
		REGTYPE nDataType, const BYTE *lpData, DWORD cbData,
		LPCWSTR lpComment = NULL, int anCommentLen = -1, BOOL abDeletion = FALSE);
	void TreeSetComment(FileRegItem* pItem, LPCWSTR lpComment = NULL, int anCommentLen = -1);
	FileRegItem* TreeFindPredefinedKey(FileRegItem* pParent, HKEY ahKey, BOOL bKeyDeleted);
	FileRegItem* TreeFindKey(FileRegItem* pParent, const wchar_t *pszName,
		LPCWSTR apszNameLwr /*= NULL*/, int anNameLen /*= -1*/, DWORD anNameCRC32 /*= 0*/, BOOL bKeyDeleted);
	FileRegItem* TreeFindValue(FileRegItem* pParent, const wchar_t *pszName,
		LPCWSTR apszNameLwr /*= NULL*/, int anNameLen /*= -1*/, DWORD anNameCRC32 /*= 0*/);
	void TreeDeleteChild(FileRegItem* pParent, FileRegItem* pChild);
	bool CompareFileRegItem(FileRegItem* pItem, DWORD nValueType, const wchar_t *pszName);
	FileRegItem* FindExistKey(FileRegItem* pParent, LPCWSTR apszSubKey, DWORD anSubkeyLen, LPCWSTR* ppszSubKeyPart);
	FileRegItem* ItemFromHkey(HKEY hKey);
	BOOL GetSubkeyToken(LPCWSTR& lpSubKey,
			wchar_t* pszSubkeyToken/*[MAX_PATH+1]*/,
			wchar_t* pszSubkeyTokenLwr/*[MAX_PATH+1]*/,
			int& rnTokenLen, DWORD& rnCRC32);
	__inline DWORD CalculateNameCRC32(const wchar_t *apszName, int anNameLen);
	//void SkipLine(LPWSTR& pszLine, LPCWSTR pszEnd, DWORD& nLine, LPCWSTR* ppszLineEnd = NULL);
	//void SkipSpaces(LPWSTR& pszLine, LPCWSTR pszEnd);
	BOOL GetKeyFromLine(
		LPWSTR& pszLine, LPWSTR& pszCol, LPWSTR pszEnd, HKEY& hKeyRoot, LPWSTR& pszKeyName, LPWSTR& pszKeyEnd,
		BOOL& bDeletion, DWORD& nLine, LPWSTR& pszComment, int& nCommentLen);
	BOOL GetValueFromLine(
		LPWSTR& pszLine, LPWSTR& pszCol, LPWSTR pszEnd,
		LPWSTR& pszValName, LPWSTR pszValNameLwr/*[32768]*/, int& nNameLen, DWORD& nNameCRC32,
		BOOL& bDeletion, DWORD& nLine,
		REGTYPE& nValueType, LPBYTE& ptrValueData, DWORD& nValueDataLen,
		LPWSTR& pszComment, int& nCommentLen);
	//BYTE CharToByte(wchar_t ch);
	int nContinueRc; // 1 - continue, 0 - break to editor, -1 - stop
	//// Internal optimized memory allocation
	//size_t nBlockDefaultSize;
	//DWORD  nBlockCount;   // ���������� ������ � pMemBlocks;
	//int    nBlockActive;  // �������� ����
	//MFileRegMemoryBlock* pMemBlocks, *pMemActiveBlock;
	//MFileRegMemoryBlock* InitializeMalloc(size_t anDefaultBlockSize);
	//__inline void* InternalMalloc(size_t anSize);
	//void ReleaseMalloc();
public:
	MFileReg(BOOL abWow64on32, BOOL abVirtualize);
	virtual ~MFileReg();
	//virtual BOOL AllowCaching() { return FALSE; };
	// ���������� ��� ������������ ����� (LoadRegFile)
	LPCWSTR GetFilePathName();
	LPCTSTR GetShowFilePathName();
	// *.reg file operations (import)
	static LONG LoadRegFile(LPCWSTR asRegFilePathName, BOOL abSilent, MRegistryBase* apTargetWorker = NULL, BOOL abDelayLoading = FALSE, MFileReg* apFileReg = NULL);
	wchar_t* GetFirstKeyFilled();
	BOOL IsInitiallyUnicode();
private:
	LPBYTE GetImportBuffer(size_t cbSize); // --> pImportBufferData
	int RegFormatFailed(LPCTSTR asFile, DWORD nLine, DWORD nCol); // 1 - continue, 0 - break to editor, -1 - stop
	void RegLoadingFailed(int nMsgId, LPCTSTR asFile, DWORD anErrCode = 0);
	BOOL bSilent;
	void MarkDirty(FileRegItem* pItem);
public:
	// ***
	virtual MRegistryBase* Duplicate();
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
	// Service - TRUE, ���� ����� ���� ��������
	virtual BOOL EditKeyPermissions(RegPath *pKey, RegItem* pItem, BOOL abVisual) { return FALSE; };
};
