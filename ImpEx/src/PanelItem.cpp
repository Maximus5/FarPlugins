
#include "stdafx.h"
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include "COMMON.H"

extern UnDecorateSymbolName_t UnDecorate_Dbghelp;
extern HMODULE ghDbghelp;

static const TCHAR cszOwnerDescCols[] = _T("N,O,Z");
//static const TCHAR szOwnerDescWidths[] = _T("0,14,8");
static const TCHAR cszOwnerCols[] = _T("N,O");
static const TCHAR cszDescCols[] = _T("N,Z");
//static const TCHAR szOwnerWidths[] = _T("0,14");
static const TCHAR cszName[] = _T("Name");
static const TCHAR cszErrInfo[] = _T("Error info");
const TCHAR* gszColumnTitles[3] = {cszName,cszErrInfo,NULL};
TCHAR gszColumnWidths[16] = _T("0,10");
PanelMode gDefaultPanelMode = {0};

extern PBYTE g_pMappedFileBase;
extern ULARGE_INTEGER g_FileSize;

MPanelItem::MPanelItem(LPCTSTR asItem, DWORD anAttr, DWORD anSize, BOOL abRoot/*=FALSE*/)
{
	nMagic = IMPEX_MAGIC;
	cbSizeOfStruct = sizeof(MPanelItem);

	pszFileName = NULL;
	memset(&item, 0, sizeof(item));
	int nLen = lstrlen(asItem);
	if (nLen > MAX_PATH) {
		pszFileName = _tcsdup(asItem);
		LPCTSTR pszSlash = _tcsrchr(asItem, _T('\\'));
		if (pszSlash) pszSlash++; else pszSlash = asItem;
		lstrcpyn(item.cFileName, pszSlash, MAX_PATH);
	} else {
		lstrcpyn(item.cFileName, asItem, MAX_PATH);
	}
	if (!abRoot) {
		//TCHAR *psz = item.cFileName;
		//while (*psz) {
		//	if (_tcschr(_T("<>|:*?/\\\""), *psz))
		//		*psz = _T('$');
		//	psz++;
		//}
	}
	item.dwFileAttributes = anAttr;
	item.nFileSizeLow = anSize;

	memset(pPanelModes, 0, sizeof(pPanelModes));
	nPanelModes = 0;
	bNeedAutoNameWidth = FALSE;

	nChildren = 0;
	pParent = NULL;
	pFirstChild = NULL;
	pLastChild = NULL;
	pNextSibling = NULL;
	pFlags = NULL;
	isFlags = FALSE;
	
	pszText = pszOwner = pszDescription = NULL;
	pBinaryData = NULL; nBinarySize = 0;
	nMaxTextSize = nCurTextLen = 0;
}

MPanelItem::~MPanelItem()
{
	MPanelItem* pChild;
	while (pFirstChild) {
		pChild = pFirstChild->pNextSibling;
		delete pFirstChild;
		pFirstChild = pChild;
	}
	if (pBinaryData) free(pBinaryData);
	if (pszText) free(pszText);
	if (pszOwner) free(pszOwner);
	if (pszDescription) free(pszDescription);
}
	
MPanelItem* MPanelItem::AddChild(LPCTSTR asItem, DWORD anAttr, DWORD anSize)
{
	bool lbNonEmpty = false;
	if (asItem) {
		LPCTSTR psz = asItem;
		while (*psz == _T(' ')) psz++;
		if (*psz) lbNonEmpty = true;
	}
	if (!lbNonEmpty)
		asItem = _T("<Empty name>");

	MPanelItem* pChild = new MPanelItem(asItem, anAttr, anSize);
	if (!pChild) {
		_ASSERTE(pChild!=NULL);
	} else {
		pChild->pParent = this;
		if (!pFirstChild) {
			_ASSERTE(pLastChild==NULL && nChildren==0);
			pFirstChild = pLastChild = pChild;
		} else {
			_ASSERTE(pLastChild->pNextSibling==NULL);
			pLastChild->pNextSibling = pChild;
			pLastChild = pChild;
		}
		nChildren ++;
	}
	return pChild;
}

MPanelItem* MPanelItem::AddFolder(LPCSTR asItem, BOOL abCreateAlways /*= TRUE*/)
{
	LPCTSTR pszItem = NULL;
	#ifdef _UNICODE
		wchar_t szItem[MAX_PATH+1];
		MultiByteToWideChar(CP_ACP, 0, asItem, -1, szItem, MAX_PATH+1);
		pszItem = szItem;
	#else
		pszItem = asItem;
	#endif
	if (!abCreateAlways) {
		MPanelItem* pFind = FindChild(pszItem, 0);
		if (pFind && (pFind->item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			return pFind;
		}
	}
	return AddChild(pszItem, FILE_ATTRIBUTE_DIRECTORY, 0);
}

MPanelItem* MPanelItem::AddFile(LPCSTR asItem, DWORD anSize)
{
	LPCTSTR pszItem = NULL;
	#ifdef _UNICODE
		wchar_t szItem[MAX_PATH+1];
		MultiByteToWideChar(CP_ACP, 0, asItem, -1, szItem, MAX_PATH+1);
		pszItem = szItem;
	#else
		pszItem = asItem;
	#endif
	return AddChild(pszItem, FILE_ATTRIBUTE_NORMAL, anSize);
}

#ifdef _UNICODE
MPanelItem* MPanelItem::AddFolder(LPCWSTR asItem, BOOL abCreateAlways /*= TRUE*/)
{
	if (!abCreateAlways) {
		MPanelItem* pFind = FindChild(asItem, 0);
		if (pFind && (pFind->item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			return pFind;
		}
	}
	return AddChild(asItem, FILE_ATTRIBUTE_DIRECTORY, 0);
}

MPanelItem* MPanelItem::AddFile(LPCWSTR asItem, DWORD anSize)
{
	return AddChild(asItem, FILE_ATTRIBUTE_NORMAL, anSize);
}
#endif

void MPanelItem::AddFlags(LPCTSTR asItem)
{
	if (!asItem || !*asItem)
		return;
	size_t nLen = _tcslen(asItem);

	// Check first, may be flag already exists?
	if (pFlags) {
		
		TCHAR* psz = pFlags->item.cFileName;
		TCHAR* pszNext = _tcschr(psz, _T(','));
		while (psz && *psz) {
			if (!pszNext) pszNext = psz + _tcslen(psz);

			if ((pszNext - psz) == nLen) {
				if (_tcsncmp(psz, asItem, nLen) == 0)
					return; // Этот флаг уже добавили
			}

			if (*pszNext != _T(',')) break;
			psz = pszNext+1;
			pszNext = _tcschr(psz, _T(','));
		}

		_tcscat(pFlags->item.cFileName, _T(","));
		_tcscat(pFlags->item.cFileName, asItem);
		pFlags->AddText(_T(","));
	} else {
		pFlags = AddFile(asItem, 0);
		pFlags->isFlags = TRUE;
	}
	pFlags->AddText(asItem, -1, TRUE);
}

MPanelItem* MPanelItem::FindChild(LPCTSTR asItem, USERDATAPTR anUserData)
{
	MPanelItem* pChild;
	// По идее, в anUserData должна быть ссылка на MPanelItem
	if (anUserData) {
		pChild = pFirstChild;
		while (pChild) {
			if (pChild == (MPanelItem*)anUserData)
				return pChild;
			pChild = pChild->pNextSibling;
		}
	}

	// Если не нашли - ищем по имени
	pChild = pFirstChild;
	while (pChild) {
		if (lstrcmpi(pChild->item.cFileName, asItem) == 0)
			return pChild;
		pChild = pChild->pNextSibling;
	}
	return NULL;
}

MPanelItem* MPanelItem::Root()
{
	MPanelItem* p = this;
	while (p->pParent)
		p = p->pParent;
	return p;
}

MPanelItem* MPanelItem::Parent()
{
	MPanelItem* p = this;
	if (p->pParent)
		p = p->pParent;
	return p;
}

void MPanelItem::printf(LPCSTR asFormat, ...)
{
	if (!asFormat || !*asFormat)
		return;
		
    va_list args;
    va_start(args,asFormat);
    static char szBuffer[1025]; //2010-03-15 для ускорения - их очень много
	szBuffer[0] = 0;
    
    int nLen = vsprintf(szBuffer, asFormat, args);
    
    if (nLen > 0) {
    	_ASSERTE(nLen<1023);
    	szBuffer[nLen] = 0;

		AddText(szBuffer, nLen);
	}
}

void MPanelItem::AddText(LPCSTR asText, int nLen /*= -1*/, BOOL abDoNotAppend /*= FALSE*/)
{
	if (!asText || !*asText)
		return;

	if (nLen<0)
		nLen = lstrlenA(asText);

	if ((nLen+1+nCurTextLen) >= nMaxTextSize) {
		size_t nAddSize = max(min(4096,nMaxTextSize),(UINT)(nLen+1));
		size_t nNewMaxSize = nMaxTextSize + nAddSize;

		TCHAR *pszNewText = (TCHAR*)malloc(nNewMaxSize*sizeof(TCHAR));
		if (!pszNewText) {
			_ASSERTE(pszNewText!=NULL);
			return;
		}
		if (pszText && nCurTextLen>0) {
			memmove(pszNewText, pszText, (nCurTextLen+1)*sizeof(TCHAR)); // ASCII+Z
			free(pszText);
		} else {
			nCurTextLen = 0;
			pszNewText[0] = 0;
		}
		pszText = pszNewText;
		nMaxTextSize = nNewMaxSize;
	}

	// Добавить этот-же текст во всех родителей

	#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, asText, nLen, pszText+nCurTextLen, nLen);
	#else	
	memmove(pszText+nCurTextLen, asText, nLen);
	#endif
	pszText[nCurTextLen+nLen] = 0;

	if (!abDoNotAppend) {
		if (pParent)
			pParent->AddText(pszText+nCurTextLen, nLen);
	}

	nCurTextLen += nLen;
}

#ifdef _UNICODE
void MPanelItem::printf(LPCWSTR asFormat, ...)
{
	if (!asFormat || !*asFormat)
		return;
		
    va_list args;
    va_start(args,asFormat);
    wchar_t szBuffer[1025]; szBuffer[0] = 0;
    
    int nLen = vswprintf(szBuffer, 1024, asFormat, args);
    
    if (nLen > 0) {
    	_ASSERTE(nLen<1023);
    	szBuffer[nLen] = 0;

		AddText(szBuffer, nLen);
	}
}
#endif

void MPanelItem::AddText(LPCWSTR asText, int nLen /*= -1*/, BOOL abDoNotAppend /*= FALSE*/)
{
	if (!asText || !*asText)
		return;

	if (nLen<0)
		nLen = lstrlenW(asText);

#ifndef _UNICODE
	char* pszData = (char*)malloc(nLen+1);
	if (!pszData)
		this->printf("\nCan't allocate memory: %i bytes\n", nLen+1);
	else {
		WideCharToMultiByte(CP_ACP, 0, asText, nLen, pszData, nLen, 0, 0);
		pszData[nLen] = 0;
		AddText(pszData, nLen, abDoNotAppend);
		free(pszData);
	}
#else
	if ((nLen+1+nCurTextLen) >= nMaxTextSize) {
		size_t nAddSize = max(min(4096,max(255,nMaxTextSize)),(UINT)(nLen+1));
		size_t nNewMaxSize = nMaxTextSize + nAddSize;

		wchar_t *pszNewText = (wchar_t*)malloc(nNewMaxSize*2);
		if (!pszNewText) {
			_ASSERTE(pszNewText!=NULL);
			return;
		}
		if (pszText && nCurTextLen>0) {
			memmove(pszNewText, pszText, (nCurTextLen+1)*2); // ASCII+Z
			free(pszText);
		} else {
			nCurTextLen = 0;
			pszNewText[0] = 0;
		}
		pszText = pszNewText;
		nMaxTextSize = nNewMaxSize;
	}
	
	if (!abDoNotAppend) {
		if (pParent)
			pParent->AddText(asText, nLen);
	}

	wmemcpy(pszText+nCurTextLen, asText, nLen);
	pszText[nCurTextLen+nLen] = 0;
	nCurTextLen += nLen;
#endif
}


void MPanelItem::SetData(const BYTE* apData, DWORD anSize, BOOL bIsCopy /*= FALSE*/, BOOL bAddBom /*= FALSE*/)
{
	if (!apData) {
		this->printf(_T("\n<Can't access memory: %i bytes at 0x00000000>\n"), anSize);
		return;
	}

	if (!bIsCopy) {
		ULONGLONG nPos = (apData - g_pMappedFileBase);
		if ((nPos+anSize) > g_FileSize.QuadPart) {
			TCHAR szErrInfo[128];
			if (nPos > g_FileSize.QuadPart) {
				wsprintf(szErrInfo, _T("Can't access memory at 0x%08X"), (DWORD)(apData-g_pMappedFileBase));
				this->AddText(szErrInfo, -1, TRUE);
				Parent()->SetError(szErrInfo);
				return;
			} else {
				wsprintf(szErrInfo, _T("Can't access %i bytes at 0x%08X"), anSize, (DWORD)(apData-g_pMappedFileBase));
				Parent()->SetError(szErrInfo);
			}
			anSize = (DWORD)(g_FileSize.QuadPart - nPos);
		}
	}


	if (!bIsCopy && !ValidateMemory(apData, anSize))
	{
		this->printf(_T("\n<Can't access memory: %i bytes at 0x%08X>\n"), anSize, (DWORD)(apData-g_pMappedFileBase));
	}
	else if (bIsCopy && IsBadReadPtr(apData, anSize))
	{
		this->printf(_T("\n<Can't access memory: %i bytes at 0x%08X>\n"), anSize, (DWORD)(apData-g_pMappedFileBase));
	}
	else
	{
		if (pBinaryData) {
			_ASSERTE(pBinaryData==NULL);
			free(pBinaryData); nBinarySize = 0;
		}
		int nShift = bAddBom ? 2 : 0;
		pBinaryData = (LPBYTE)malloc(anSize+nShift);
		nBinarySize = anSize+nShift;
		if (!pBinaryData) {
			this->printf(_T("\n<Can't allocate memory: %i bytes at 0x%08X>\n"), anSize, (DWORD)(apData-g_pMappedFileBase));
		} else {
			if (bAddBom) {
				WORD nBOM = 0xFEFF; // CP-1200
				memmove(pBinaryData, &nBOM, 2);
			}
			memmove(pBinaryData+nShift, apData, anSize);
		}

		item.nFileSizeLow = anSize; // реально может быть меньше
	}
}

BOOL MPanelItem::GetData(HANDLE hFile)
{
	BOOL lbRc = false;
	DWORD nWrite = 0;
	if (pBinaryData) {
		lbRc = WriteFile(hFile, pBinaryData, nBinarySize, &nWrite, 0);
	} else if (pszText) {
		#ifdef _UNICODE
		WORD nBOM = 0xFEFF; // CP-1200
		WriteFile(hFile, &nBOM, 2, &nWrite, 0);
		#endif

		nWrite = lstrlen(pszText)*sizeof(TCHAR);
		lbRc = WriteFile(hFile, pszText, nWrite, &nWrite, 0);
	}
	return lbRc;
}

BOOL MPanelItem::IsColumnTitles()
{
	return (nPanelModes > 0);
}

void MPanelItem::SetColumnsTitles(LPCTSTR asOwnerTitle, int anOwnerWidth, LPCTSTR asDescTitle, int anDescWidth, int anNameWidth)
{
	if (asOwnerTitle)
	{
		pszColumnTitles[0] = cszName;
		pszColumnTitles[1] = asOwnerTitle;
		pszColumnTitles[2] = asDescTitle;

		if (anNameWidth < 0) {
			bNeedAutoNameWidth = TRUE;
			anNameWidth = -anNameWidth;
		}
		nColumnWidths[0] = anNameWidth;
		nColumnWidths[1] = anOwnerWidth;
		nColumnWidths[2] = (asDescTitle) ? anDescWidth : 0;

		wsprintf(szColumnWidths, (asDescTitle) ? _T("%i,%i,%i") : _T("%i,%i"), anNameWidth, anOwnerWidth, anDescWidth);

		nPanelModes = 1; // по просьбам трудящихся не будем перехватывать все режимы
		for (int i=0; i<10; i++) {
			pPanelModes[i].ColumnTypes = (TCHAR*)((asDescTitle) ? cszOwnerDescCols : cszOwnerCols);
			pPanelModes[i].ColumnWidths = szColumnWidths;
			pPanelModes[i].ColumnTitles = (TCHAR**)pszColumnTitles;
			#if defined(FAR_UNICODE)
			pPanelModes[i].Flags = PMFLAGS_DETAILEDSTATUS;
			#else
			pPanelModes[i].DetailedStatus = TRUE;
			#endif
		}
	} else
	if (asDescTitle)
	{
		pszColumnTitles[0] = cszName;
		pszColumnTitles[1] = asDescTitle;

		if (anNameWidth < 0) {
			bNeedAutoNameWidth = TRUE;
			anNameWidth = -anNameWidth;
		}
		nColumnWidths[0] = anNameWidth;
		nColumnWidths[1] = anDescWidth;

		wsprintf(szColumnWidths, _T("%i,%i"), anNameWidth, anDescWidth);

		nPanelModes = 1; // по просьбам трудящихся не будем перехватывать все режимы
		for (int i=0; i<10; i++) {
			pPanelModes[i].ColumnTypes = (TCHAR*)cszDescCols;
			pPanelModes[i].ColumnWidths = szColumnWidths;
			pPanelModes[i].ColumnTitles = (TCHAR**)pszColumnTitles;
			#if defined(FAR_UNICODE)
			pPanelModes[i].Flags = PMFLAGS_DETAILEDSTATUS;
			#else
			pPanelModes[i].DetailedStatus = TRUE;
			#endif
		}
	} else {
		nPanelModes = 0;
	}
}

void MPanelItem::SetColumns(LPCSTR asOwner, LPCSTR asDescription)
{
	LPCSTR Src[2] = {asDescription,asOwner};
	TCHAR** Dst[2] = {&pszDescription,&pszOwner};
	for (int i=0; i<2; i++)
	{
		if (*(Dst[i])) {
			free(*(Dst[i])); *(Dst[i]) = NULL;
		}
		if (Src[i] && *(Src[i])) {
			int nLen = (int)strlen(Src[i]);
			if ((*(Dst[i]) = (TCHAR*)malloc((nLen+1)*sizeof(TCHAR)))!=NULL) {
				#ifdef _UNICODE
				MultiByteToWideChar(CP_ACP, 0, Src[i], nLen+1, *(Dst[i]), nLen+1);
				#else
				strcpy(*(Dst[i]), Src[i]);
				#endif
			}
		}
	}
}

void MPanelItem::SetColumns(LPCWSTR asOwner, LPCWSTR asDescription)
{
#ifdef _UNICODE
	LPCWSTR Src[2] = {asOwner,asDescription};
	wchar_t** Dst[2] = {&pszOwner,&pszDescription};
	for (int i=0; i<2; i++)
	{
		if (*(Dst[i])) {
			free(*(Dst[i])); *(Dst[i]) = NULL;
		}
		if (Src[i] && *(Src[i])) {
			int nLen = lstrlen(Src[i]);
			if ((*(Dst[i]) = (wchar_t*)malloc((nLen+1)*2))!=NULL) {
				wcscpy(*(Dst[i]), Src[i]);
			}
		}
	}
#else
	char* pszOwner = NULL;
	char* pszDescr = NULL;
	if (asOwner) {
		int nLen = lstrlenW(asOwner)+1;
		pszOwner = (char*)malloc(nLen);
		WideCharToMultiByte(CP_OEMCP, 0, asOwner, nLen, pszOwner, nLen, 0, 0);
	}
	if (asDescription) {
		int nLen = lstrlenW(asDescription)+1;
		pszDescr = (char*)malloc(nLen);
		WideCharToMultiByte(CP_OEMCP, 0, asDescription, nLen, pszDescr, nLen, 0, 0);
	}
	SetColumns(pszOwner, pszDescr);
#endif
}

void MPanelItem::SetError(LPCTSTR asErrorDesc)
{
	if (!asErrorDesc || !*asErrorDesc) return;
	this->printf(_T("\n!!! %s !!!\n\n"), asErrorDesc);

	SetColumns(asErrorDesc, NULL);
	if (!nPanelModes)
		SetColumnsTitles(cszErrInfo, 0, NULL, 0, -20);

	MPanelItem *p = pParent;
	while (p) {
		p->SetColumns(asErrorDesc, NULL);
		if (!p->nPanelModes)
			p->SetColumnsTitles(cszErrInfo, 0, NULL, 0, -20);
		p = p->pParent;
	}
}

void MPanelItem::SetErrorPtr(LPCVOID ptr, DWORD nSize)
{
	TCHAR szErrInfo[128];
	wsprintf(szErrInfo, _T("Can't access %u bytes at 0x%08X"), nSize, (DWORD)(((LPBYTE)ptr)-g_pMappedFileBase));
	this->AddText(szErrInfo, -1, TRUE);
	this->SetColumns(szErrInfo, NULL);
	Parent()->SetError(szErrInfo);
}

LPCTSTR MPanelItem::GetText()
{
	return pszText ? pszText : _T("");
}

LPCTSTR MPanelItem::GetFileName()
{
	return item.cFileName;
}

int MPanelItem::getFindData(struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	int nCount = 0;
	*pPanelItem = (PluginPanelItem*)calloc(nChildren+1,sizeof(PluginPanelItem));
	
	MPanelItem* pChild = pFirstChild;
	while (pChild && nCount <= nChildren) {
		#ifdef _UNICODE
		PanelItemFileNamePtr((*pPanelItem)[nCount]) = pChild->item.cFileName;
		PanelItemFileSize((*pPanelItem)[nCount]) = pChild->item.nFileSizeLow;
		#else
		lstrcpy((*pPanelItem)[nCount].FindData.cFileName, pChild->item.cFileName);
		(*pPanelItem)[nCount].FindData.nFileSizeLow = pChild->item.nFileSizeLow;
		#endif
		PanelItemAttributes((*pPanelItem)[nCount]) = pChild->item.dwFileAttributes;
		(*pPanelItem)[nCount].Description = pChild->pszDescription;
		(*pPanelItem)[nCount].Owner = pChild->pszOwner;

		PanelItemUserData((*pPanelItem)[nCount]) = (USERDATAPTR)pChild;
	
		nCount++;
		pChild = pChild->pNextSibling;
	}
	_ASSERTE(pChild == NULL);
	
	// + DUMP.TXT
	#ifdef _UNICODE
	PanelItemFileNamePtr((*pPanelItem)[nCount]) = DUMP_FILE_NAME;
	PanelItemFileSize((*pPanelItem)[nCount]) = pszText ? (wcslen(pszText)+1)*2 : 0; // + BOM
	#else
	lstrcpy((*pPanelItem)[nCount].FindData.cFileName, DUMP_FILE_NAME);
	(*pPanelItem)[nCount].FindData.nFileSizeLow = pszText ? (UINT)strlen(pszText) : 0;
	#endif
	PanelItemAttributes((*pPanelItem)[nCount]) = FILE_ATTRIBUTE_NORMAL;
	// В Description/Owner может быть информация об Exception и т.п.
	(*pPanelItem)[nCount].Description = pszDescription;
	(*pPanelItem)[nCount].Owner = pszOwner;

	PanelItemUserData((*pPanelItem)[nCount]) = (USERDATAPTR)pChild;
	nCount++;
	
	*pItemsNumber = nCount;
	return TRUE;
}

void MPanelItem::getPanelModes (
		#if FAR_UNICODE>1900
			struct OpenPanelInfo *Info
		#else
			struct OpenPluginInfo *Info
		#endif
		)
{
	if (nPanelModes) {
		if (bNeedAutoNameWidth) {
			int nMaxLen = -1;
			MPanelItem* p = pFirstChild;
			while (p) {
				int nLen = lstrlen(p->item.cFileName);
				if (nLen > nMaxLen)
					nMaxLen = min(32,nLen);
				p = p->pNextSibling;
			}

			if (nMaxLen > nColumnWidths[0]) {
				nColumnWidths[0] = nMaxLen;
				wsprintf(szColumnWidths, (nColumnWidths[2]) ? _T("%i,%i,%i") : _T("%i,%i"), 
					nColumnWidths[0], nColumnWidths[1], nColumnWidths[2]);
			}
			bNeedAutoNameWidth = FALSE;
		}
		Info->PanelModesNumber = nPanelModes;
		Info->PanelModesArray = pPanelModes;
	} else {
		Info->PanelModesNumber = 1;
		Info->PanelModesArray = &gDefaultPanelMode;

		if (!gDefaultPanelMode.ColumnTypes) {
			gszColumnTitles[0] = cszName;
			gszColumnTitles[1] = cszErrInfo;
			_tcscpy(gszColumnWidths, _T("0,10"));
			gDefaultPanelMode.ColumnTypes = (TCHAR*)cszOwnerCols;
			gDefaultPanelMode.ColumnWidths = gszColumnWidths;
			gDefaultPanelMode.ColumnTitles = (TCHAR**)gszColumnTitles;
			#if FAR_UNICODE>=1900
			gDefaultPanelMode.Flags = PMFLAGS_DETAILEDSTATUS;
			#else
			gDefaultPanelMode.DetailedStatus = TRUE;
			#endif
		}
	}
}


extern bool DumpFile(MPanelItem* pRoot, LPCTSTR filename, bool abForceDetect);

//DumpFile(MPanelItem* pRoot, LPTSTR filename, bool abForceDetect)
HANDLE MImpEx::Open(LPCTSTR asFileName, bool abForceDetect)
{
	if (!ghDbghelp)
	{
		ghDbghelp = LoadLibrary(_T("Dbghelp.dll"));
		if (ghDbghelp)
		{
			UnDecorate_Dbghelp = (UnDecorateSymbolName_t)GetProcAddress(ghDbghelp, "UnDecorateSymbolName");
		}
	}

	MImpEx* pImpEx = new MImpEx(asFileName);

	LPCTSTR pszName = asFileName;
	TCHAR* pszBuffer = NULL;
	#ifdef _UNICODE
	int nLen = lstrlen(asFileName);
	if (asFileName[0] == L'\\' && asFileName[1] == L'\\')
	{
		if (asFileName[2] != L'?' && asFileName[2] != L'.')
		{
			// UNC
			pszBuffer = (wchar_t*)malloc((nLen+10)*2);
			wcscpy(pszBuffer, L"\\\\?\\UNC");
			wcscat(pszBuffer, asFileName+1);
			pszName = pszBuffer;
		}
	}
	else if (asFileName[1] == L':' && asFileName[2] == L'\\')
	{
		// UNC
		pszBuffer = (wchar_t*)malloc((nLen+10)*2);
		wcscpy(pszBuffer, L"\\\\?\\");
		wcscat(pszBuffer, asFileName);
		pszName = pszBuffer;
	}
	#endif
	
	bool bRc = DumpFile(pImpEx->pRoot, pszName, abForceDetect);

	if (pszBuffer) free(pszBuffer);

	if (!bRc) {
		delete pImpEx;
		return FAR_PANEL_NONE;
	}
	
	return (HANDLE)pImpEx;
}

MImpEx::MImpEx(LPCTSTR asFileName)
{
	pRoot = new MPanelItem(asFileName, FILE_ATTRIBUTE_DIRECTORY, 0, TRUE);
	_ASSERTE(pRoot!=NULL);
	
	sCurDir[0] = 0;
	
	LPCTSTR asName = fsf.PointToName(asFileName);
	_tcscpy(sPanelTitle, _T("ImpEx:")); pszPanelAdd = sPanelTitle + _tcslen(sPanelTitle);
	if (asName) { lstrcpyn(pszPanelAdd, asName, MAX_PATH); pszPanelAdd += _tcslen(pszPanelAdd); }
	//*pszPanelAdd++ = _T('\\'); *pszPanelAdd = 0;
	
	pCurFolder = pRoot;
}

MImpEx::~MImpEx()
{
	if (pRoot) {
		delete pRoot;
		pRoot = NULL;
		pCurFolder = NULL;
		sCurDir[0] = 0;
		sPanelTitle[0] = 0;
	}
}

int MImpEx::setDirectory ( LPCTSTR Dir, int OpMode )
{
	// В случае удачного завершения возвращаемое значение должно равняться TRUE.
	// Иначе функция должна возвращать FALSE.
	if (Dir==NULL) return FALSE;

	TCHAR szToken[MAX_PATH];
	LPTSTR pszSlash = NULL;
	BOOL lbChanged = FALSE;
	
	while (Dir && *Dir) {
		if (Dir[0]==_T('\\')) {
			sCurDir[0] = 0;
			pCurFolder = pRoot;
			Dir ++;
			lbChanged = TRUE;

		} else if (Dir[0]==_T('.') && Dir[1]==_T('.') && (Dir[2]==0 || Dir[2]==_T('\\'))) {
			if (pCurFolder->pParent)
				pCurFolder = pCurFolder->pParent;
			pszSlash = _tcsrchr(sCurDir, _T('\\'));
			if (pszSlash)
				*pszSlash = 0;
			else
				sCurDir[0] = 0;
			Dir += (Dir[2]==_T('\\')) ? 3 : 2;
			lbChanged = TRUE;

		} else {
			LPCTSTR pszToken = NULL;
			MPanelItem* pDir = NULL;
			pszSlash = (TCHAR*)_tcschr(Dir, _T('\\'));
			if (pszSlash) {
				lstrcpyn(szToken, Dir, (int)(pszSlash-Dir+1));
				szToken[pszSlash-Dir] = 0;
				pszToken = szToken;
				Dir = pszSlash+1;
			} else {
				pszToken = Dir;
				Dir = NULL; // закончили
			}

			pDir = pCurFolder->FindChild(pszToken, 0);
			if (!pDir)
				break;

			if (sCurDir[0]) lstrcat(sCurDir, _T("\\"));
			lstrcat(sCurDir, pszToken);
			pCurFolder = pDir;
			lbChanged = TRUE;
		}
	}
	
	if (sCurDir[0]) {
		*pszPanelAdd = _T('\\');
		lstrcpyn(pszPanelAdd+1, sCurDir, MAX_PATH);
	} else {
		*pszPanelAdd = 0;
	}

	return lbChanged;
}

void MImpEx::getOpenPluginInfo (
	#if FAR_UNICODE>1900
		struct OpenPanelInfo *Info
	#else
		struct OpenPluginInfo *Info
	#endif
	)
{
    Info->StructSize = sizeof(*Info);

    // Флаги
    Info->Flags = OPIF_ADDDOTS
		#if FAR_UNICODE>1900
		| OPIF_NONE
		#else
		| OPIF_USEHIGHLIGHTING | OPIF_USEFILTER
		#endif
		| OPIF_RAWSELECTION ;

    //Info->InfoLines = (mi_InfoLines>0) ? m_InfoLines : NULL;
    //Info->InfoLinesNumber = mi_InfoLines;

    Info->StartPanelMode = _T('0');
    Info->StartSortMode = SM_NAME;
    Info->StartSortOrder = 0;

    Info->KeyBar = NULL; // TODO

    Info->ShortcutData = NULL;
    
	Info->PanelTitle = sPanelTitle; //_T("ImpEx:");
    Info->CurDir = sCurDir;
    Info->Format = _T("ImpEx");
    Info->HostFile = pRoot->GetFileName(); // Чтобы при выходе из плагина выделение вернулось на файл

	pCurFolder->getPanelModes ( Info );
}

int MImpEx::getFindData ( struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode )
{
	return pCurFolder->getFindData(pPanelItem, pItemsNumber, OpMode );
}

void MImpEx::freeFindData ( struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	// Freeing only PanelItem, because of all fields are pointers to our three
	if (PanelItem) free(PanelItem);
}

int MImpEx::getFiles( struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, TCHAR *DestPath, int OpMode )
{
	int liRc = -1;

	//if ((OpMode & OPM_SILENT)==0) {
	//	if (!ConfirmCopy(PanelItem, ItemsNumber, DestPath, Move))
	//		return -1;
	//}

	TCHAR lsPath[MAX_PATH*2];
	lstrcpy(lsPath, DestPath);
	int nLen = lstrlen(lsPath);
	if (nLen<1) return -1;
	if (lsPath[nLen-1] != _T('\\'))
	{
		lsPath[nLen++] = _T('\\');
		lsPath[nLen] = 0;
	}

	for (int i=0; i<ItemsNumber; i++)
	{
		if (PanelItemAttributes(PanelItem[i]) & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Это будет при ShiftF2, тогда все обламывается, что нехорошо
			//liRc = -1; // ошибка
			//break; // пока не разрешаем!
			continue; // пока не разрешаем!
		}

		lsPath[nLen-1] = 0; // Оставить только путь
		CreateDirectory(lsPath, NULL); // Директория должна быть создана
		lsPath[nLen-1] = _T('\\');
		lstrcpy(lsPath+nLen, FILENAMEPTR(PanelItem[i]));

		LPCTSTR pszName = FILENAMEPTR(PanelItem[i]);

		MPanelItem* pItem = lstrcmpi(pszName, DUMP_FILE_NAME) ?
			pCurFolder->FindChild(pszName, PanelItemUserData(PanelItem[i])) : pCurFolder;
		if (pItem)
		{
			TCHAR *psz = lsPath+nLen;
			while (*psz)
			{
				if (_tcschr(_T("<>|:*?/\\\""), *psz))
					*psz = _T('$');
				psz++;
			}

			SetFileAttributes(lsPath, FILE_ATTRIBUTE_NORMAL); // иначе на R/O & System файлах обламывается
			HANDLE hFile = CreateFile ( lsPath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
				OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if (hFile != INVALID_HANDLE_VALUE)
			{
				pItem->GetData(hFile);
				//DWORD nWrite = 0;
				//LPCTSTR pszText = pItem->GetText();

				//#ifdef _UNICODE
				//WORD nBOM = 0xFEFF;
				//WriteFile(hFile, &nBOM, 2, &nWrite, 0);
				//#endif
				//nWrite = lstrlen(pszText)*sizeof(TCHAR);
				//WriteFile(hFile, pszText, nWrite, &nWrite, 0);

				CloseHandle ( hFile );
				//SetFileAttributes(lsPath, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_READONLY);
				liRc = 1;

				if (PanelItem[i].Flags & PPIF_SELECTED)
					PanelItem[i].Flags -= PPIF_SELECTED;
			}
			else
			{
				liRc = -1;
				break;
			}
		}
		else
		{
			liRc = -1;
			break;
		}
	}

	//В случае успеха возвращаемое значение должно быть равно 1. 
	//В случае провала возвращается 0. 
	//Если функция была прервана пользователем, то должно возвращаться -1. 
	return liRc;
}

bool MImpEx::ViewCode ()
{
	if (!sCurDir || (lstrcmpi(sCurDir, ExportsTableName) != 0))
		return false;

	return true;
}
