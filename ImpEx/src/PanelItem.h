
#pragma once

struct MPanelItem;
class MImpEx;

#define IMPEX_MAGIC 0x78456D49 // 'ImEx'

struct MPanelItem
{
	DWORD nMagic;			// IMPEX_MAGIC
	DWORD cbSizeOfStruct;	// 1196 в этой версии
	DWORD nBinarySize;		// размер
	LPBYTE pBinaryData;		// и собственно бинарные данные

//protected:
	// Заменить на FAR_FIND_DATA? В смысле, чтобы длинные имена поддержать, а не только до MAX_PATH
	// тут ведь еще и путь к просматриваемому exe-шнику может быть!
	WIN32_FIND_DATA item;
	TCHAR* pszFileName;
	/*enum {
		eUndefined,
		eBinaryData,
		eTextData
	} DataType;*/
	TCHAR *pszText, *pszOwner, *pszDescription;
	size_t nMaxTextSize, nCurTextLen;
	//
	int nChildren;
	MPanelItem* pFirstChild;
	MPanelItem* pLastChild;
	//
	MPanelItem* pNextSibling;
	//
	MPanelItem* pParent;
	//
	MPanelItem* pFlags;
	//
	PanelMode pPanelModes[10];
	const TCHAR* pszColumnTitles[3];
	TCHAR szColumnWidths[32];
	BOOL bNeedAutoNameWidth;
	int nColumnWidths[3];
	int nPanelModes;
	BOOL isFlags;
	
//protected:
	MPanelItem(LPCTSTR asItem, DWORD anAttr, DWORD anSize, BOOL abRoot=FALSE);
//public:
	~MPanelItem();
	
//public:
	MPanelItem* AddChild(LPCTSTR asItem, DWORD anAttr, DWORD anSize);
	MPanelItem* AddFolder(LPCSTR asItem, BOOL abCreateAlways = TRUE);
	MPanelItem* AddFile(LPCSTR asItem, DWORD anSize = 0);
#ifdef _UNICODE
	MPanelItem* AddFolder(LPCWSTR asItem, BOOL abCreateAlways = TRUE);
	MPanelItem* AddFile(LPCWSTR asItem, DWORD anSize = 0);
#endif
	void AddFlags(LPCTSTR asItem);
	MPanelItem* FindChild(LPCTSTR asItem, USERDATAPTR anUserData);
	MPanelItem* Root();
	MPanelItem* Parent();
	
//public:
	void printf(LPCSTR asFormat, ...);
#ifdef _UNICODE
	void printf(LPCWSTR asFormat, ...);
#endif
	void AddText(LPCSTR asText, int nLen = -1, BOOL abDoNotAppend = FALSE);
	void AddText(LPCWSTR asText, int nLen = -1, BOOL abDoNotAppend = FALSE);
	void SetColumnsTitles(LPCTSTR asOwnerTitle, int anOwnerWidth, LPCTSTR asDescTitle, int anDescWidth, int anNameWidth=0);
	BOOL IsColumnTitles();
	void SetColumns(LPCSTR asOwner, LPCSTR asDescription);
	void SetColumns(LPCWSTR asOwner, LPCWSTR asDescription);
	void SetData(const BYTE* apData, DWORD anSize, BOOL bIsCopy = FALSE, BOOL bAddBom = FALSE);
	BOOL GetData(HANDLE hFile);
	void SetError(LPCTSTR asErrorDesc);
	void SetErrorPtr(LPCVOID ptr, DWORD nSize);
	
	LPCTSTR GetText();
	LPCTSTR GetFileName();
	
//public:
	int getFindData(struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	void getPanelModes (
		#if FAR_UNICODE>1900
			struct OpenPanelInfo *Info
		#else
			struct OpenPluginInfo *Info
		#endif
	);
	
	//friend struct MPanelItem;
	//friend class MImpEx;
};

class MImpEx
{
public:
	static HANDLE Open(LPCTSTR asFileName, bool abForceDetect);
public:
	MPanelItem* pRoot;
	MImpEx(LPCTSTR asFileName);
	~MImpEx();
public:
	TCHAR sCurDir[MAX_PATH*2];
	TCHAR sPanelTitle[MAX_PATH*3+32], *pszPanelAdd;
	MPanelItem* pCurFolder;
public:
	int setDirectory ( LPCTSTR Dir, int OpMode );
	void getOpenPluginInfo (
		#if FAR_UNICODE>1900
			struct OpenPanelInfo *Info
		#else
			struct OpenPluginInfo *Info
		#endif
	);
	int getFindData ( struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode );
	void freeFindData ( struct PluginPanelItem *PanelItem, int ItemsNumber);
	int getFiles( struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, TCHAR *DestPath, int OpMode );
	bool ViewCode ();
};
