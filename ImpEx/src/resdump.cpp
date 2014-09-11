//==================================
// PEDUMP - Matt Pietrek 1997-2001
// FILE: RESDUMP.CPP
//==================================

//1. StringTable визуализировать файлами с Description
//2. StringTable добавить колонку языка - чтобы сортировать можно было
//3. Icon, Bitmap - подпихивать заголовок, добавить расширение
//4. Icon, Bitmap - в Owner - пихать язык
//5. VersionInfo
//6. Dialog visualisation

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <wchar.h>
#pragma hdrstop
#include "common.h"
#include "extrnvar.h"
#include "resdump.h"

extern bool gbDecimalIds;

static const TCHAR cszLanguage[] = _T("Lang");
static const TCHAR cszString[] = _T("String value");
static const TCHAR cszType[] = _T("Type");
static const TCHAR cszInfo[] = _T("Information");

typedef const BYTE *LPCBYTE;

// The predefined resource types
char *SzResourceTypes[] = {
"???_0",
"CURSOR",
"BITMAP",
"ICON",
"MENU",
"DIALOG",
"STRING",
"FONTDIR",
"FONT",
"ACCELERATORS",
"RCDATA",
"MESSAGETABLE",
"GROUP_CURSOR",
"???_13",
"GROUP_ICON",
"???_15",
"VERSION",
"DLGINCLUDE",
"???_18",
"PLUGPLAY",
"VXD",
"ANICURSOR",
"ANIICON",
"HTML",
"MANIFEST"
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoFlags[] = {
	{VS_FF_DEBUG, L"VS_FF_DEBUG"},
	{VS_FF_INFOINFERRED, L"VS_FF_INFOINFERRED"},
	{VS_FF_PATCHED, L"VS_FF_PATCHED"},
	{VS_FF_PRERELEASE, L"VS_FF_PRERELEASE"},
	{VS_FF_PRIVATEBUILD, L"VS_FF_PRIVATEBUILD"},
	{VS_FF_SPECIALBUILD, L"VS_FF_SPECIALBUILD"},
	{0}
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoFileOS[] = {
	{VOS_DOS, L"VOS_DOS"},
	{VOS_NT, L"VOS_NT"},
	{VOS__WINDOWS16, L"VOS__WINDOWS16"},
	{VOS__WINDOWS32, L"VOS__WINDOWS32"},
	{VOS_OS216, L"VOS_OS216"},
	{VOS_OS232, L"VOS_OS232"},
	{VOS__PM16, L"VOS__PM16"},
	{VOS__PM32, L"VOS__PM32"},
	{VOS_WINCE, L"VOS_WINCE"},
	{0}
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoDrvSubtype[] = {
	{VFT2_DRV_PRINTER, L"VFT2_DRV_PRINTER"},
	{VFT2_DRV_KEYBOARD, L"VFT2_DRV_KEYBOARD"},
	{VFT2_DRV_LANGUAGE, L"VFT2_DRV_LANGUAGE"},
	{VFT2_DRV_DISPLAY, L"VFT2_DRV_DISPLAY"},
	{VFT2_DRV_NETWORK, L"VFT2_DRV_NETWORK"},
	{VFT2_DRV_SYSTEM, L"VFT2_DRV_SYSTEM"},
	{VFT2_DRV_INSTALLABLE, L"VFT2_DRV_INSTALLABLE"},
	{VFT2_DRV_SOUND, L"VFT2_DRV_SOUND"},
	{VFT2_DRV_COMM, L"VFT2_DRV_COMM"},
	{VFT2_DRV_INPUTMETHOD, L"VFT2_DRV_INPUTMETHOD"},
	{VFT2_DRV_VERSIONED_PRINTER, L"VFT2_DRV_VERSIONED_PRINTER"},
	{0}
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoFontSubtype[] = {
	{VFT2_FONT_RASTER, L"VFT2_FONT_RASTER"},
	{VFT2_FONT_VECTOR, L"VFT2_FONT_VECTOR"},
	{VFT2_FONT_TRUETYPE, L"VFT2_FONT_TRUETYPE"},
	{0}
};

struct String { 
	WORD   wLength; 
	WORD   wValueLength; 
	WORD   wType; 
	WCHAR  szKey[1]; 
	WORD   Padding[ANYSIZE_ARRAY]; 
	WORD   Value[1]; 
};
struct StringA { 
	WORD   wLength; 
	WORD   wValueLength; 
	char   szKey[1]; 
	char   Padding[ANYSIZE_ARRAY]; 
	char   Value[1]; 
};

struct StringTable { 
	WORD   wLength; 
	WORD   wValueLength; 
	WORD   wType; 
	WCHAR  szKey[9];
	WORD   Padding[ANYSIZE_ARRAY];
	String Children[ANYSIZE_ARRAY];
};
struct StringTableA { 
	WORD    wLength; 
	WORD    wType; 
	char    szKey[9];
	char    Padding[ANYSIZE_ARRAY];
	StringA Children[ANYSIZE_ARRAY];
};

struct StringFileInfo { 
	WORD        wLength; 
	WORD        wValueLength; 
	WORD        wType; 
	WCHAR       szKey[ANYSIZE_ARRAY]; 
	WORD        Padding[ANYSIZE_ARRAY]; 
	StringTable Children[ANYSIZE_ARRAY]; 
};
struct StringFileInfoA { 
	WORD         wLength; 
	WORD         wType; 
	char         szKey[ANYSIZE_ARRAY]; 
	char         Padding[ANYSIZE_ARRAY]; 
	StringTableA Children[ANYSIZE_ARRAY]; 
};

struct Var { 
	WORD  wLength; 
	WORD  wValueLength; 
	WORD  wType; 
	WCHAR szKey[ANYSIZE_ARRAY]; 
	WORD  Padding[ANYSIZE_ARRAY]; 
	DWORD Value[ANYSIZE_ARRAY]; 
};

struct VarFileInfo { 
	WORD  wLength; 
	WORD  wValueLength; 
	WORD  wType; 
	WCHAR szKey[ANYSIZE_ARRAY]; 
	WORD  Padding[ANYSIZE_ARRAY]; 
	Var   Children[ANYSIZE_ARRAY]; 
};

//PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pStrResEntries = 0;
PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pDlgResEntries = 0;
//DWORD g_cStrResEntries = 0;
DWORD g_cDlgResEntries = 0;

DWORD GetOffsetToDataFromResEntry( 	PBYTE pResourceBase,
									PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry )
{
	// The IMAGE_RESOURCE_DIRECTORY_ENTRY is gonna point to a single
	// IMAGE_RESOURCE_DIRECTORY, which in turn will point to the
	// IMAGE_RESOURCE_DIRECTORY_ENTRY, which in turn will point
	// to the IMAGE_RESOURCE_DATA_ENTRY that we're really after.  In
	// other words, traverse down a level.

	PIMAGE_RESOURCE_DIRECTORY pStupidResDir;
	pStupidResDir = (PIMAGE_RESOURCE_DIRECTORY)(pResourceBase + pResEntry->OffsetToDirectory);

    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry =
	    	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pStupidResDir + 1);// PTR MATH

	PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry =
			(PIMAGE_RESOURCE_DATA_ENTRY)(pResourceBase + pResDirEntry->OffsetToData);

	return pResDataEntry->OffsetToData;
}


// Get an ASCII string representing a resource type
void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
    if ( type <= (WORD)RT_MANIFEST )
        strncpy(buffer, SzResourceTypes[type], cBytes);
    else
		sprintf(buffer, gbDecimalIds ? "%05u" : "0x%X", type);
}

//
// If a resource entry has a string name (rather than an ID), go find
// the string and convert it from unicode to ascii.
//
void GetResourceNameFromId
(
    DWORD id, PBYTE pResourceBase, PSTR buffer, UINT cBytes
)
{
    PIMAGE_RESOURCE_DIR_STRING_U prdsu;

    // If it's a regular ID, just format it.
    if ( !(id & IMAGE_RESOURCE_NAME_IS_STRING) )
    {
        sprintf(buffer, gbDecimalIds ? "%05u" : "0x%04X", id);
        return;
    }
    
    id &= 0x7FFFFFFF;
    prdsu = MakePtr(PIMAGE_RESOURCE_DIR_STRING_U, pResourceBase, id);

    // prdsu->Length is the number of unicode characters
    WideCharToMultiByte(CP_ACP, 0, prdsu->NameString, prdsu->Length,
                        buffer, cBytes, 0, 0);
    buffer[ min(cBytes-1,prdsu->Length) ] = 0;  // Null terminate it!!!
}

void DumpStringTable(
					 MPanelItem *pRoot, MPanelItem* pBin,
					 DWORD stringIdBase, DWORD langId,
					 LPCWSTR ptrRes, DWORD resSize
					 )
{
	wchar_t * pszBuf = (wchar_t*)malloc(2048*2);
	DWORD nMaxLen = 2048;
	LPCBYTE pFileEnd = g_pMappedFileBase+g_FileSize.QuadPart;
	LPCBYTE pResEnd = ((LPCBYTE)ptrRes)+resSize;
	if (pFileEnd < pResEnd)
		pResEnd = pFileEnd;

	TCHAR szResID[32];
	TCHAR szLang[16]; wsprintf(szLang, _T("0x%04X"), langId);
	TCHAR szDump[1024];

	pRoot->SetColumnsTitles(NULL, 0, cszString, 0, 12);

	LPCWSTR pStrEntry = ptrRes;
	if ( !pStrEntry) {
		pBin->AddText(_T("\n!!! Invalid pStrEntry in DumpStringTable !!!\n"));
		return;
	}

	if (!stringIdBase) {
		pBin->AddText(_T("\n!!! Invalid stringIdBase == 0 in DumpStringTable !!!\n"));
		return;
	}

	unsigned int id = (stringIdBase - 1) << 4;

	wsprintf(szDump, _T("  LangID: 0x%04X,    BaseStringID: %5u.    ResourceSize: %5ubytes.  (Strings trimmed to 512 chars)\n"), langId, id, resSize);
	pBin->AddText(szDump, -1, TRUE);
	pBin->AddText(_T("  ----------------------------------------------------------------------------------------------------\n"), -1, TRUE);

	for ( unsigned j = 0; j < 16; j++ )
	{
		if (!ValidateMemory(pStrEntry,2)) {
			pBin->SetErrorPtr(pStrEntry,2); break;
		}
		DWORD len = (WORD)(*pStrEntry++); // Хоть тут и WORD, но заложимся на замены
		DWORD len0 = len;
		if ( !len )
		{
			len = len;
		}
		else
		{
			if (!ValidateMemory(pStrEntry,len)) {
				pBin->SetErrorPtr(pStrEntry,len); break;
			}
			if (((LPCBYTE)(pStrEntry+len)) >= pResEnd) {
				len = (WORD)min(0xFFFF,(((pResEnd - (LPCBYTE)pStrEntry) >> 1)));
			}


			wsprintf(szResID, _T("%u.%s"), id + j, szLang );

			// Буфер должен быть достаточно большим, чтобы вместить возможные замены \r\n\t\0
			if ((len*2) >= nMaxLen) {
				if (pszBuf) free(pszBuf);
				nMaxLen = len*2+1;
				pszBuf = (wchar_t*)malloc(nMaxLen*2);
				if (!pszBuf) {
					nMaxLen = 0;
					pBin->printf(_T("%-5u: \n!!! Can't allocate %i bytes\n"), id + j, nMaxLen*2);
					continue;
				}
			}

			wchar_t c;
			wchar_t *pszDst = pszBuf;
			const wchar_t *pszSrc = pStrEntry;
			DWORD nModLen = len;
			for ( unsigned k = 0; k < len; k++, pszSrc++, pszDst++ )
			{
				c = *pszSrc;
				if (c>=32) {
					*(pszDst) = c;
				} else {
					switch (c) {
						case L'\t':
							*(pszDst++) = L'\\';
							*(pszDst) = L't';
							nModLen++;
							break;
						case L'\r':
							*(pszDst++) = L'\\';
							*(pszDst) = L'r';
							nModLen++;
							break;
						case L'\n':
							*(pszDst++) = L'\\';
							*(pszDst) = L'n';
							nModLen++;
							break;
						case 0:
							*(pszDst++) = L'\\';
							*(pszDst) = L'0';
							nModLen++;
							break;
						default:
							*(pszDst) = c;
					}
				}
			}
			*pszDst = 0;
			MPanelItem* pRes = pRoot->AddFile(szResID, nModLen);
			pRes->SetData((LPBYTE)pStrEntry, len*2, FALSE, TRUE);
			pRes->SetColumns(NULL, pszBuf);

			wsprintf(szDump, _T("    %-5u: len=%-4u; "), id + j, len0);
			pBin->AddText(szDump, -1, TRUE);
			pBin->AddText(pszBuf, min(nModLen,512), TRUE);
			pBin->AddText(_T("\n"), -1, TRUE);
		}

		if (len0 != len) {
			pBin->printf(_T("\n\n!!! String table was broken !!!\n!!! Required %i WCHARs, but StringTable have only %i WCHARs left !!!\n\n"),len0,len);
			return;
		}

		pStrEntry += len;
	}
}

#pragma pack( push )
#pragma pack( 1 )

typedef struct
{
	BYTE        bWidth;          // Width, in pixels, of the image
	BYTE        bHeight;         // Height, in pixels, of the image
	BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
	BYTE        bReserved;       // Reserved ( must be 0)
	WORD        wPlanes;         // Color Planes
	WORD        wBitCount;       // Bits per pixel
	DWORD       dwBytesInRes;    // How many bytes in this resource?
	DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	WORD           idReserved;   // Reserved (must be 0)
	WORD           idType;       // Resource Type (1 for icons, 2 for cursors)
	WORD           idCount;      // How many images?
	ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

// DLL and EXE Files
// Icons can also be stored in .DLL and .EXE files. The structures used to store icon images in .EXE and .DLL files differ only slightly from those used in .ICO files.
// Analogous to the ICONDIR data in the ICO file is the RT_GROUP_ICON resource. In fact, one RT_GROUP_ICON resource is created for each ICO file bound to the EXE or DLL with the resource compiler/linker. The RT_GROUP_ICON resource is simply a GRPICONDIR structure:
// #pragmas are used here to insure that the structure's
// packing in memory matches the packing of the EXE or DLL.
typedef struct
{
	BYTE   bWidth;               // Width, in pixels, of the image
	BYTE   bHeight;              // Height, in pixels, of the image
	BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE   bReserved;            // Reserved
	WORD   wPlanes;              // Color Planes
	WORD   wBitCount;            // Bits per pixel
	DWORD   dwBytesInRes;         // how many bytes in this resource?
	WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;

typedef struct 
{
	WORD            idReserved;   // Reserved (must be 0)
	WORD            idType;       // Resource type (1 for icons, 2 for cursors)
	WORD            idCount;      // How many images?
	GRPICONDIRENTRY   idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;

#pragma pack( pop )

void ResourceParseFlags(DWORD dwFlags, wchar_t* pszFlags, DWORD_FLAG_DESCRIPTIONSW* pFlags)
{
	BOOL bFirst = TRUE;
	wchar_t* pszStart = pszFlags;
	wsprintfW(pszFlags, L"0x%XL", dwFlags);
	pszFlags += lstrlenW(pszFlags);
	*pszFlags = 0;

	while (pFlags->flag)
	{
		if ( (dwFlags & pFlags->flag) == pFlags->flag ) {
			if (bFirst) {
				bFirst = FALSE;
				while ((pszFlags - pszStart) < 8) {
					*(pszFlags++) = L' '; *pszFlags = 0;
				}
				wcscpy(pszFlags, L" // "); pszFlags += wcslen(pszFlags);
			} else {
				*(pszFlags++) = L'|';
			}
			wcscpy(pszFlags, pFlags->name);
			pszFlags += wcslen(pszFlags);
		}
		pFlags++;
	}
	*pszFlags = 0;
}

void ParseVersionInfoFixed(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, VS_FIXEDFILEINFO* pVer, wchar_t* &psz)
{
	wchar_t szMask[255], szFlags[255], szOS[255], szFileType[64], szFileSubType[64];
	ResourceParseFlags(pVer->dwFileFlagsMask, szMask, VersionInfoFlags);
	ResourceParseFlags(pVer->dwFileFlags, szFlags, VersionInfoFlags);
	ResourceParseFlags(pVer->dwFileOS, szOS, VersionInfoFileOS);
	szFileType[0] = 0;
	wsprintfW(szFileSubType, L"0x%XL", pVer->dwFileSubtype);
	switch (pVer->dwFileType) {
		case VFT_APP: wcscpy(szFileType, L"     // VFT_APP"); break;
		case VFT_DLL: wcscpy(szFileType, L"     // VFT_DLL"); break;
		case VFT_DRV: wcscpy(szFileType, L"     // VFT_DRV"); 
			{
				wchar_t* pszStart = szFileSubType;
				wchar_t* pszFlags = pszStart + lstrlenW(pszStart);
				while ((pszFlags - pszStart) < 8) {
					*(pszFlags++) = L' '; *pszFlags = 0;
				}
				for (int k=0; VersionInfoDrvSubtype[k].flag; k++) {
					if (pVer->dwFileSubtype == VersionInfoDrvSubtype[k].flag) {
						wcscat(szFileSubType, L" // "); wcscat(szFileSubType, VersionInfoDrvSubtype[k].name);
						break;
					}
				}
			}
			break;
		case VFT_FONT: wcscpy(szFileType, L"     // VFT_FONT");
			{
				wchar_t* pszStart = szFileSubType;
				wchar_t* pszFlags = pszStart + lstrlenW(pszStart);
				while ((pszFlags - pszStart) < 8) {
					*(pszFlags++) = L' '; *pszFlags = 0;
				}
				for (int k=0; VersionInfoFontSubtype[k].flag; k++) {
					if (pVer->dwFileSubtype == VersionInfoFontSubtype[k].flag) {
						wcscat(szFileSubType, L" // "); wcscat(szFileSubType, VersionInfoFontSubtype[k].name);
						break;
					}
				}
			}
			break;
		case VFT_VXD: wcscpy(szFileType, L"     // VFT_VXD"); break;
		case VFT_STATIC_LIB: wcscpy(szFileType, L"     // VFT_STATIC_LIB"); break;
	}

	wsprintfW(psz,
		L"#include <windows.h>\n\n"
		L"VS_VERSION_INFO VERSIONINFO\n"
		L" FILEVERSION    %u,%u,%u,%u\n"
		L" PRODUCTVERSION %u,%u,%u,%u\n"
		L" FILEFLAGSMASK  %s\n"
		L" FILEFLAGS      %s\n"
		L" FILEOS         %s\n"
		L" FILETYPE       0x%XL%s\n"
		L" FILESUBTYPE    %s\n"
		L"BEGIN\n"
		,
		HIWORD(pVer->dwFileVersionMS), LOWORD(pVer->dwFileVersionMS),
		HIWORD(pVer->dwFileVersionLS), LOWORD(pVer->dwFileVersionLS),
		HIWORD(pVer->dwProductVersionMS), LOWORD(pVer->dwProductVersionMS),
		HIWORD(pVer->dwProductVersionLS), LOWORD(pVer->dwProductVersionLS),
		szMask, szFlags,
		szOS,
		pVer->dwFileType, szFileType,
		szFileSubType
		);
	psz += wcslen(psz);
}

//#define ALIGN_TOKEN(p) p = (LPWORD)( ((((DWORD_PTR)p) + 3) >> 2) << 2 );
#define ALIGN_TOKEN(p) p = (LPWORD)( ((( ((DWORD_PTR)p) - ((DWORD_PTR)ptrRes) + 3) >> 2) << 2 ) + ((DWORD_PTR)ptrRes))

void ParseVersionInfoVariableString(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, LPWORD pToken, wchar_t* &psz)
{
	StringFileInfo *pSFI = (StringFileInfo*)pToken;
	LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
		LPWORD pEnd1 = (LPWORD)(((LPBYTE)pToken)+*pToken);
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		wcscat(psz, L"    BLOCK \"");
		wcscat(psz, pSFI->szKey);
		wcscat(psz, L"\"\n");
		if (pSFI->wType != 1) {
			wcscat(psz, L"    // Warning! Binary data in StringFileInfo\n");
		}
		{
			wcscat(psz, L"    BEGIN\n");
			psz += wcslen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (LPWORD)(pSFI->szKey+wcslen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			ALIGN_TOKEN(pToken);
			if ((((LPBYTE)pToken)+sizeof(StringTable)) <= (LPBYTE)pEnd1) {
				StringTable *pST = (StringTable*)pToken;
				LPWORD pEnd2 = (LPWORD)(((LPBYTE)pToken)+*pToken);
				if (pEnd1 < pEnd2)
					pEnd2 = pEnd1;
				// Specifies an 8-digit hexadecimal number stored as a Unicode string. 
				// The four most significant digits represent the language identifier. 
				// The four least significant digits represent the code page for which 
				// the data is formatted. Each Microsoft Standard Language identifier contains 
				// two parts: the low-order 10 bits specify the major language, 
				// and the high-order 6 bits specify the sublanguage.
				wcscat(psz, L"        BLOCK \"");
				psz += wcslen(psz);
				wmemmove(psz, pST->szKey, 8);
				psz += 8; *psz = 0; wcscat(psz, L"\"\n");
				wcscat(psz, L"        BEGIN\n");
				pToken = (LPWORD)(pST->szKey+8);
				//while (*pToken == 0 && pToken < pEnd2) pToken++;
				ALIGN_TOKEN(pToken);
				while ((((LPBYTE)pToken)+sizeof(String)) <= (LPBYTE)pEnd2) {
					String *pS = (String*)pToken;
					if (pS->wLength == 0) break; // Invalid?
					LPWORD pNext = (LPWORD)(((LPBYTE)pToken)+pS->wLength);
					wcscat(psz, L"            VALUE \""); psz += wcslen(psz);
					wcscat(psz, pS->szKey);
					wcscat(psz, L"\", "); psz += wcslen(psz);
					// Выровнять текст в результирующем .rc
					for (int k = lstrlenW(pS->szKey); k < 17; k++) *(psz++) = L' ';
					*(psz++) = L'"'; *psz = 0;
					pToken = (LPWORD)(pS->szKey+wcslen(pS->szKey)+1);
					//while (*pToken == 0 && pToken < pEnd2) pToken++;
					ALIGN_TOKEN(pToken);
					int nLenLeft = pS->wValueLength;
					while (pToken < pEnd2 && nLenLeft>0) {
						switch (*pToken) {
							case 0:
								*(psz++) = L'\\'; *(psz++) = L'0'; break;
							case L'\r':
								*(psz++) = L'\\'; *(psz++) = L'r'; break;
							case L'\n':
								*(psz++) = L'\\'; *(psz++) = L'n'; break;
							case L'\t':
								*(psz++) = L'\\'; *(psz++) = L't'; break;
							default:
								*(psz++) = *pToken;
						}
						pToken++; nLenLeft--;
					}
					*psz = 0;
					//if (pToken < pEnd2 && pS->wValueLength) {
					//	// Вообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					wcscat(psz, L"\"\n"); psz += wcslen(psz);

					// Next value
					pToken = pNext;
					if (pToken < pEnd2 && *pToken == 0)
					{
						wcscat(psz, L"            // Zero-length item found\n"); psz += wcslen(psz);
						while (pToken < pEnd2 && *pToken == 0) pToken ++;
					}
				}
				wcscat(psz, L"        END\n");
			}
			//
			wcscat(psz, L"    END\n");
		}
	}
}
void ParseVersionInfoVariableStringA(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, char* pToken, wchar_t* &psz)
{
	wchar_t szTemp[MAX_PATH*2+1], *pwsz = NULL;
	StringFileInfoA *pSFI = (StringFileInfoA*)pToken;
	char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(StringFileInfoA)) {
		char* pEnd1 = (char*)(((LPBYTE)pToken)+*((WORD*)pToken));
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		wcscat(psz, L"    BLOCK \"");
		//wcscat(psz, pSFI->szKey);
		psz += wcslen(psz);
		MultiByteToWideChar(CP_ACP,0,pSFI->szKey,-1,psz,64);
		//for (int x=0; x<8; x++) {
		//	*psz++ = pSFI->szKey[x] ? pSFI->szKey[x] : L' ';
		//}
		wcscat(psz, L"\"\n");
		//if (pSFI->wType != 1) {
		//	wcscat(psz, L"    // Warning! Binary data in StringFileInfo\n");
		//}
		{
			wcscat(psz, L"    BEGIN\n");
			psz += wcslen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (char*)(pSFI->szKey+strlen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			//ALIGN_TOKEN(pToken); ???
			pToken++; // ???
			if ((((LPBYTE)pToken)+sizeof(StringTableA)) <= (LPBYTE)pEnd1) {
				StringTableA *pST = (StringTableA*)pToken;
				char* pEnd2 = (char*)(((LPBYTE)pToken)+*((WORD*)pToken));
				if (pEnd2 > pEnd1)
					pEnd2 = pEnd1;
				// Specifies an 8-digit hexadecimal number stored as a Unicode string. 
				// The four most significant digits represent the language identifier. 
				// The four least significant digits represent the code page for which 
				// the data is formatted. Each Microsoft Standard Language identifier contains 
				// two parts: the low-order 10 bits specify the major language, 
				// and the high-order 6 bits specify the sublanguage.
				wcscat(psz, L"        BLOCK \"");
				psz += wcslen(psz);
				//wmemmove(psz, pST->szKey, 8); ???
				psz[MultiByteToWideChar(CP_ACP,0,pST->szKey,8,psz,64)] = 0;
				psz += 8; *psz = 0; wcscat(psz, L"\"\n");
				wcscat(psz, L"        BEGIN\n");
				pToken = (char*)(pST->szKey+8);
				//while (*pToken == 0 && pToken < pEnd2) pToken++;
				//ALIGN_TOKEN(pToken); ???
				pToken += 4; //???
				while ((((LPBYTE)pToken)+sizeof(StringA)) <= (LPBYTE)pEnd2) {
					StringA *pS = (StringA*)pToken;
					if (pS->wLength == 0) break; // Invalid?
					char* pNext = (char*)(((LPBYTE)pToken)+pS->wLength);
					if (pNext > pEnd2)
						pNext = pEnd2;
					wcscat(psz, L"            VALUE \""); psz += wcslen(psz);
					//wcscat(psz, pS->szKey); ???
					psz[MultiByteToWideChar(CP_ACP,0,pS->szKey,-1,psz,32)] = 0;
					wcscat(psz, L"\", "); psz += wcslen(psz);
					// Выровнять текст в результирующем .rc
					for (int k = lstrlenA(pS->szKey); k < 17; k++) *(psz++) = L' ';
					*(psz++) = L'"'; *psz = 0;
					pToken = (char*)(pS->szKey+strlen(pS->szKey)+1);
					while (*pToken == 0 && pToken < pNext) pToken++;
					//ALIGN_TOKEN(pToken); ???
					int nLenLeft = min(pS->wValueLength,MAX_PATH*2);
					if (nLenLeft > (pNext-pToken))
						nLenLeft = (int)(pNext-pToken);
					szTemp[MultiByteToWideChar(CP_ACP,0,pToken,nLenLeft,szTemp,nLenLeft)] = 0;
					pwsz = szTemp;
					while (nLenLeft>0) {
						switch (*pwsz) {
							case 0:
								*(psz++) = L'\\'; *(psz++) = L'0'; break;
							case L'\r':
								*(psz++) = L'\\'; *(psz++) = L'r'; break;
							case L'\n':
								*(psz++) = L'\\'; *(psz++) = L'n'; break;
							case L'\t':
								*(psz++) = L'\\'; *(psz++) = L't'; break;
							default:
								*(psz++) = *pwsz;
						}
						pwsz++; nLenLeft--;
					}
					*psz = 0;
					//if (pToken < pEnd2 && pS->wValueLength) {
					//	// Вообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					wcscat(psz, L"\"\n"); psz += wcslen(psz);

					// Next value
					pToken = pNext;
					if (pToken < pEnd2 && *pToken == 0)
					{
						wcscat(psz, L"            // Zero-length item found\n"); psz += wcslen(psz);
						while (pToken < pEnd2 && *pToken == 0) pToken ++;
					}
				}
				wcscat(psz, L"        END\n");
			}
			//
			wcscat(psz, L"    END\n");
		}
	}
}

void ParseVersionInfoVariableVar(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, LPWORD pToken, wchar_t* &psz)
{
	VarFileInfo *pSFI = (VarFileInfo*)pToken;
	LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(VarFileInfo)) {
		LPWORD pEnd1 = (LPWORD)(((LPBYTE)pToken)+*pToken);
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		wcscat(psz, L"    BLOCK \"");
		wcscat(psz, pSFI->szKey);
		wcscat(psz, L"\"\n");
		if (pSFI->wType != 1) {
			wcscat(psz, L"    // Warning! Binary data in VarFileInfo\n");
		}
		{
			wcscat(psz, L"    BEGIN\n");
			psz += wcslen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (LPWORD)(pSFI->szKey+wcslen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			ALIGN_TOKEN(pToken);
			if ((((LPBYTE)pToken)+sizeof(Var)) <= (LPBYTE)pEnd1) {
				pToken = (LPWORD)(pSFI->szKey+wcslen(pSFI->szKey)+1);
				//while (*pToken == 0 && pToken < pEnd1) pToken++;
				ALIGN_TOKEN(pToken);
				while ((((LPBYTE)pToken)+sizeof(Var)) <= (LPBYTE)pEnd1) {
					Var *pS = (Var*)pToken;
					if (pS->wLength == 0) break; // Invalid?
					LPWORD pNext = (LPWORD)(((LPBYTE)pToken)+pS->wLength);
					wcscat(psz, L"        VALUE \""); psz += wcslen(psz);
					wcscat(psz, pS->szKey);
					wcscat(psz, L"\""); psz += wcslen(psz);
					pToken = (LPWORD)(pS->szKey+wcslen(pS->szKey)+1);
					// Align to 32bit boundary
					ALIGN_TOKEN(pToken);
					//pToken++;
					//while (*pToken == 0 && pToken < pEnd1) pToken++;

					// The low-order word of each DWORD must contain a Microsoft language identifier, 
					// and the high-order word must contain the IBM code page number. 
					// Either high-order or low-order word can be zero, indicating that the file 
					// is language or code page independent. 
					while ((pToken+2) <= pEnd1) {
						psz += wcslen(psz);
						//DWORD nLangCP = *((LPDWORD)pToken);
						wsprintfW(psz, L", 0x%X, %u", (DWORD)(pToken[0]), (DWORD)(pToken[1]));
						pToken += 2;
					}
					//	// Вообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					wcscat(psz, L"\n"); psz += wcslen(psz);

					// Next value
					pToken = pNext;
				}
			}
			wcscat(psz, L"    END\n");
		}
	}
}

void ParseVersionInfoVariable(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, LPWORD pToken, wchar_t* &psz)
{
	StringFileInfo *pSFI = (StringFileInfo*)pToken;

	if (_wcsicmp(pSFI->szKey, L"StringFileInfo") == 0) {
		LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
			ParseVersionInfoVariableString(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
		}
	} else if (_wcsicmp(pSFI->szKey, L"VarFileInfo") == 0) {
		LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(VarFileInfo)) {
			ParseVersionInfoVariableVar(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
		}
	}
}
void ParseVersionInfoVariableA(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, char* pToken, wchar_t* &psz)
{
	StringFileInfoA *pSFI = (StringFileInfoA*)pToken;

	if (_stricmp(pSFI->szKey, "StringFileInfo") == 0) {
		char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
			ParseVersionInfoVariableStringA(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
		}
	}
}

void ParseVersionInfo(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID &ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy)
{
	WORD nTestSize = ((WORD*)ptrRes)[0];
	WORD nTestShift = ((WORD*)ptrRes)[1];
	if (nTestSize == resSize && nTestShift < resSize) {
		VS_FIXEDFILEINFO* pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+0x28);
		// По идее, должно быть здесь, но если нет - ищем сигнатуру
		if (pVer->dwSignature != 0xfeef04bd) {
			DWORD nMax = resSize - sizeof(VS_FIXEDFILEINFO);
			for (UINT i = 4; i < nMax; i++) {
				if (((VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i))->dwSignature == 0xfeef04bd) {
					pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i);
					break;
				}
			}
		}
		DWORD nNewSize = resSize*2 + 2048;
		if (pVer->dwSignature == 0xfeef04bd && nNewSize > resSize) {
			ptrBuf = (LPBYTE)malloc(nNewSize);
			if (!ptrBuf) {
				pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
			} else {
				wchar_t* psz = (LPWSTR)ptrBuf;
				WORD nBOM = 0xFEFF; // CP-1200
				*psz++ = nBOM;
				
				// VS_FIXEDFILEINFO
				ParseVersionInfoFixed(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pVer, psz);

				StringFileInfo *pSFI = (StringFileInfo*)(pVer+1);
				LPWORD pToken = (LPWORD)pSFI;
				LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
				while (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
					pSFI = (StringFileInfo*)pToken;
					if (pSFI->wLength == 0)
						break; // Invalid
					ParseVersionInfoVariable(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
					//
					pToken = (LPWORD)(((LPBYTE)pSFI)+pSFI->wLength);
					while (pToken < pEnd && *pToken == 0)
						pToken ++;
					psz += wcslen(psz);
				}

				// Done
				wcscat(psz, L"END\n");
				{
					// сначала добавим "бинарные данные" ресурса (NOT PARSED)
					pChild = pRoot->AddFile(fileNameBuffer, resSize);
					pChild->SetData((LPBYTE)ptrRes, resSize);
				}
				strcat(fileNameBuffer, ".rc");
				ptrRes = ptrBuf;
				// И вернем преобразованные в "*.rc" (PARSED)
				resSize = lstrlenW((LPWSTR)ptrBuf)*2; // первый WORD - BOM
				bIsCopy = TRUE;
			}
		}
	}
}

// ANSI for 16bit PE's
void ParseVersionInfoA(MPanelItem *pRoot, char* fileNameBuffer, MPanelItem *&pChild, LPVOID &ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy)
{
	WORD nTestSize = ((WORD*)ptrRes)[0];
	//WORD nTestShift = ((WORD*)ptrRes)[1]; // ???
	if (nTestSize >= sizeof(VS_FIXEDFILEINFO) && nTestSize <= resSize /*&& nTestShift < resSize*/) {
		VS_FIXEDFILEINFO* pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+0x28);
		// По идее, должно быть здесь, но если нет - ищем сигнатуру
		if (pVer->dwSignature != 0xfeef04bd) {
			DWORD nMax = resSize - sizeof(VS_FIXEDFILEINFO);
			for (UINT i = 4; i < nMax; i++) {
				//BUGBUG: Проверить, на x64 жить такое будет, или свалится на Alignment?
				if (((VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i))->dwSignature == 0xfeef04bd) {
					pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i);
					break;
				}
			}
		}
		DWORD nNewSize = resSize*2 + 2048;
		if (pVer->dwSignature == 0xfeef04bd // Сигнатура найдена
			&& nNewSize > resSize)          // Размер ресурса (resSize) не превышает DWORD :)
		{
			ptrBuf = (LPBYTE)malloc(nNewSize);
			if (!ptrBuf) {
				pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
			} else {
				wchar_t* psz = (LPWSTR)ptrBuf;
				WORD nBOM = 0xFEFF; // CP-1200
				*psz++ = nBOM;

				// VS_FIXEDFILEINFO
				ParseVersionInfoFixed(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pVer, psz);

				StringFileInfoA *pSFI = (StringFileInfoA*)(pVer+1);
				char* pToken = (char*)pSFI;
				char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
				while (pToken < pEnd && *pToken > sizeof(StringFileInfoA)) {
					pSFI = (StringFileInfoA*)pToken;
					if (pSFI->wLength == 0)
						break; // Invalid
					ParseVersionInfoVariableA(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
					//
					pToken = (char*)(((LPBYTE)pSFI)+pSFI->wLength);
					while (pToken < pEnd && *pToken == 0)
						pToken ++;
					psz += wcslen(psz);
				}


				// Done
				wcscat(psz, L"END\n");
				{
					// сначала добавим "бинарные данные" ресурса (NOT PARSED)
					pChild = pRoot->AddFile(fileNameBuffer, resSize);
					pChild->SetData((LPBYTE)ptrRes, resSize);
				}
				strcat(fileNameBuffer, ".rc");
				ptrRes = ptrBuf;
				// И вернем преобразованные в "*.rc" (PARSED)
				resSize = lstrlenW((LPWSTR)ptrBuf)*2;
				bIsCopy = TRUE;
			}
		}
	}
}

MPanelItem* CreateResource(MPanelItem *pRoot, 
						   DWORD rootType, LPVOID ptrRes, DWORD resSize,
						   LPCSTR asID, LPCSTR langID, DWORD stringIdBase, DWORD anLangId)
{
	char fileNameBuffer[MAX_PATH];
	MPanelItem *pChild = NULL;
	LPBYTE ptrBuf = NULL;
	BOOL bIsCopy = FALSE;
	TCHAR szType[32] = {0}, szInfo[64] = {0};
	TCHAR szTextInfo[1024];
	BOOL bDontSetData = FALSE;

	if (asID && *asID) {
		strcpy(fileNameBuffer, asID);
	} else {
		strcpy(fileNameBuffer, "????");
	}
	if (langID && *langID) {
		strcat(fileNameBuffer, "."); strcat(fileNameBuffer, langID);
	}

	switch (rootType) {
	case (WORD)RT_GROUP_ICON: case 0x8000+(WORD)RT_GROUP_ICON:
	case (WORD)RT_GROUP_CURSOR: case 0x8000+(WORD)RT_GROUP_CURSOR:
		{
			strcat(fileNameBuffer, ".txt");
			pChild = pRoot->AddFile(fileNameBuffer, 0);
			if (!ValidateMemory(ptrRes, max(resSize,sizeof(GRPICONDIR)))) {
				pChild->SetErrorPtr(ptrRes, resSize);
			} else if (resSize < sizeof(GRPICONDIR)) {
				pChild->SetColumns(
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
					_T("!!! Invalid size of resource"));
				pChild->printf(_T("\n!!! Invalid size of %s resource, expected %i bytes, but only %i bytes exists !!!"),
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
					sizeof(GRPICONDIR), resSize);
			} else {
				GRPICONDIR* pIcon = (GRPICONDIR*)ptrRes;
				int nSizeLeft = resSize - 6;
				int nResCount = pIcon->idCount;

				wsprintf(szInfo, _T("Count: %i"), nResCount);
				pChild->SetColumns(
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
					szInfo);

				wsprintf(szTextInfo, _T("%s, Count: %u\n%s\n"),
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
					nResCount,
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("====================") : _T("======================"));
				pChild->AddText(szTextInfo, -1, TRUE); // Не добавлять в DUMP.TXT		

				LPGRPICONDIRENTRY pEntry = pIcon->idEntries;
				while (nSizeLeft >= sizeof(GRPICONDIRENTRY)) {
					if (!ValidateMemory(pEntry, sizeof(GRPICONDIRENTRY))) {
						pChild->SetErrorPtr(pEntry, sizeof(GRPICONDIRENTRY));
						break;
					}
					wsprintf(szTextInfo, 
						_T("ID: 0x%04X (%u),  BytesInRes: %i\n")
						_T("  Width: %i, Height: %i\n")
						_T("  ColorCount: %i, Planes: %i, BitCount: %i\n\n"),
						(UINT)pEntry->nID, (UINT)pEntry->nID, (UINT)pEntry->dwBytesInRes,
						(UINT)pEntry->bWidth, (UINT)pEntry->bHeight,
						(UINT)pEntry->bColorCount, (UINT)pEntry->wPlanes, (UINT)pEntry->wBitCount);
					//
					pChild->AddText(szTextInfo, -1, TRUE); // Не добавлять в DUMP.TXT		
					nSizeLeft -= sizeof(GRPICONDIRENTRY);
					nResCount --;
					pEntry++;
				}
			}
			return pChild;
		} break;
	case (WORD)RT_ICON: case 0x8000+(WORD)RT_ICON:
		{
			_tcscpy(szType, _T("ICON"));

			if (!ValidateMemory(ptrRes, resSize)) {
				pRoot->SetErrorPtr(ptrRes, resSize);
			} else if (resSize>4 && *((DWORD*)ptrRes) == 0x474e5089/* %PNG */) {
				strcat(fileNameBuffer, ".png");
				_tcscpy(szInfo, _T("PNG format"));
			} else
			if (resSize > sizeof(BITMAPINFOHEADER) 
				&& ((BITMAPINFOHEADER*)ptrRes)->biSize == sizeof(BITMAPINFOHEADER)
				&& (((BITMAPINFOHEADER*)ptrRes)->biWidth && ((BITMAPINFOHEADER*)ptrRes)->biWidth < 256)
				&& (((BITMAPINFOHEADER*)ptrRes)->biHeight == (((BITMAPINFOHEADER*)ptrRes)->biWidth * 2)))
			{
				wsprintf(szInfo, _T("%ix%i (%ibpp)"), 
					((BITMAPINFOHEADER*)ptrRes)->biWidth, ((BITMAPINFOHEADER*)ptrRes)->biWidth,
					((BITMAPINFOHEADER*)ptrRes)->biBitCount*((BITMAPINFOHEADER*)ptrRes)->biPlanes);

				// Делаем копию буфера, но предваряем его заголовком иконки
				DWORD nNewSize = resSize + sizeof(ICONDIR);
				ptrBuf = (LPBYTE)malloc(nNewSize);
				if (!ptrBuf) {
					pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
				} else {
					ICONDIR* pIcon = (ICONDIR*)ptrBuf;
					pIcon->idReserved = 0;
					pIcon->idType = 1;
					pIcon->idCount = 1;
					pIcon->idEntries[0].bWidth = (BYTE)((BITMAPINFOHEADER*)ptrRes)->biWidth;
					pIcon->idEntries[0].bHeight = (BYTE)((BITMAPINFOHEADER*)ptrRes)->biWidth;
					pIcon->idEntries[0].bColorCount = (((BITMAPINFOHEADER*)ptrRes)->biBitCount >= 8)
						? 0 : (1 << ((BITMAPINFOHEADER*)ptrRes)->biBitCount);
					pIcon->idEntries[0].bReserved = 0;
					pIcon->idEntries[0].wPlanes = ((BITMAPINFOHEADER*)ptrRes)->biPlanes;
					pIcon->idEntries[0].wBitCount = ((BITMAPINFOHEADER*)ptrRes)->biBitCount;
					pIcon->idEntries[0].dwBytesInRes = resSize;
					pIcon->idEntries[0].dwImageOffset = sizeof(ICONDIR);
					memmove(&(pIcon->idEntries[1]), ptrRes, resSize);

					ptrRes = ptrBuf;
					resSize = nNewSize;
					bIsCopy = TRUE;
					strcat(fileNameBuffer, ".ico");
				}
			}
		} break;
	case (WORD)RT_CURSOR: case 0x8000+(WORD)RT_CURSOR:
		{
			_tcscpy(szType, _T("CURSOR"));

			if (!ValidateMemory(ptrRes, resSize)) {
				pRoot->SetErrorPtr(ptrRes, resSize);
			} else if (resSize>4 && *((DWORD*)ptrRes) == 'GNP%') {
				strcat(fileNameBuffer, ".png");
			} else if (resSize > (sizeof(BITMAPINFOHEADER)+4)) {
				BITMAPINFOHEADER* pBmpTest = (BITMAPINFOHEADER*)(((LPBYTE)ptrRes)+4);
				if (pBmpTest->biSize == sizeof(BITMAPINFOHEADER)
					&& pBmpTest->biWidth && pBmpTest->biWidth < 256
					&& pBmpTest->biHeight == pBmpTest->biWidth * 2)
				{
					wsprintf(szInfo, _T("%ix%i (%ibpp)"), 
						pBmpTest->biWidth, pBmpTest->biWidth,
						pBmpTest->biBitCount*pBmpTest->biPlanes);

					// Делаем копию буфера, но предваряем его заголовком иконки
					DWORD nNewSize = resSize + sizeof(ICONDIR) - 4;
					ptrBuf = (LPBYTE)malloc(nNewSize);
					if (!ptrBuf) {
						pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
					} else {
						ICONDIR* pIcon = (ICONDIR*)ptrBuf;
						pIcon->idReserved = 0;
						pIcon->idType = 2;
						pIcon->idCount = 1;
						pIcon->idEntries[0].bWidth = (BYTE)pBmpTest->biWidth;
						pIcon->idEntries[0].bHeight = (BYTE)pBmpTest->biWidth;
						pIcon->idEntries[0].bColorCount = 0;
							//(pBmpTest->biBitCount >= 8) ? 0 : (1 << (pBmpTest->biBitCount));
						pIcon->idEntries[0].bReserved = 0;
						pIcon->idEntries[0].wPlanes = ((WORD*)ptrRes)[0];
						pIcon->idEntries[0].wBitCount = ((WORD*)ptrRes)[1];
						pIcon->idEntries[0].dwBytesInRes = resSize-4;
						pIcon->idEntries[0].dwImageOffset = sizeof(ICONDIR);
						memmove(&(pIcon->idEntries[1]), ((LPBYTE)ptrRes)+4, resSize-4);

						ptrRes = ptrBuf;
						resSize = nNewSize;
						bIsCopy = TRUE;
						strcat(fileNameBuffer, ".cur");
					}
				}
			}
		} break;
	case (WORD)RT_BITMAP: case 0x8000+(WORD)RT_BITMAP:
		{
			_tcscpy(szType, _T("BITMAP"));
			DWORD nNewSize = sizeof(BITMAPFILEHEADER)+resSize;
			if (!ValidateMemory(ptrRes, max(sizeof(BITMAPINFOHEADER),resSize))) {
				pRoot->SetErrorPtr(ptrRes, resSize);
			} else if (nNewSize<resSize) {
				pRoot->AddText(_T("\n!!! Too large resource, can't insert header !!!\n"));
			} else {
				LPBITMAPINFOHEADER pInfo = (LPBITMAPINFOHEADER)ptrRes;
				if (pInfo->biSize < sizeof(BITMAPINFOHEADER)
					|| pInfo->biPlanes != 1 || pInfo->biBitCount > 32
					|| (pInfo->biWidth & 0xFF000000) || (pInfo->biHeight & 0xFF000000))
				{
					pRoot->AddText(_T("\n!!! Invalid BITMAPINFOHEADER !!!\n"));
					_tcscpy(szInfo, _T("Invalid BITMAPINFOHEADER"));
				} else {
					ptrBuf = (LPBYTE)malloc(nNewSize);
					if (!ptrBuf) {
						pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
					} else {
						LPBITMAPFILEHEADER pBmp = (LPBITMAPFILEHEADER)ptrBuf;

						wsprintf(szInfo, _T("%ix%i (%ibpp)"), 
							pInfo->biWidth, pInfo->biHeight,
							pInfo->biBitCount*pInfo->biPlanes);

						pBmp->bfType = 0x4d42; //'BM';
						pBmp->bfSize = nNewSize;
						pBmp->bfReserved1 = pBmp->bfReserved2 = 0;
						//TODO: наверное еще и размер таблицы цветов нужно добавить?
						pBmp->bfOffBits = sizeof(BITMAPFILEHEADER)+pInfo->biSize;
						if (pInfo->biBitCount == 0 || pInfo->biCompression == BI_JPEG || pInfo->biCompression == BI_PNG) {
							// Дополнительные сдвиги на палитру не требуются
						} else {
							if (pInfo->biCompression == BI_BITFIELDS) {
								if (pInfo->biBitCount == 16 || pInfo->biBitCount == 32) {
									// color table consists of three DWORD color masks that specify the 
									// red, green, and blue components, respectively, of each pixel
									pBmp->bfOffBits += 3 * sizeof(DWORD);
								}
							} else if (pInfo->biCompression == BI_RGB || pInfo->biCompression == BI_RLE4 || pInfo->biCompression == BI_RLE8) {
								if (pInfo->biBitCount == 1 || pInfo->biBitCount == 4 || pInfo->biBitCount == 8) {
									if (pInfo->biClrUsed == 0) {
										pBmp->bfOffBits += ((DWORD)1 << pInfo->biBitCount) * (DWORD)sizeof(RGBQUAD);
									} else if (pInfo->biClrUsed <= 256) {
										// BUGBUG: What is real palette size in such files?
										pBmp->bfOffBits += pInfo->biClrUsed * (DWORD)sizeof(RGBQUAD);
									}
								}
							}
						}
						
						// Copying contents of bitmap resource
						memmove(pBmp+1, ptrRes, resSize);

						ptrRes = ptrBuf;
						resSize = nNewSize;
						bIsCopy = TRUE;
						strcat(fileNameBuffer, ".bmp");
					}
				}
			}
		} break;
	case (WORD)RT_MANIFEST:
		{
			strcat(fileNameBuffer, ".xml");
		} break;
	case (WORD)RT_STRING:
		{
			MPanelItem* pBin = pRoot->AddFolder(_T("Binary Data"), FALSE);

			if (!pRoot->IsColumnTitles())
				pBin->AddText( _T("<String Table>\n==============\n"), -1, TRUE );

			_ASSERTE(stringIdBase<=0x1000); // base of StringID
			__try {
				DumpStringTable( pRoot, pBin, stringIdBase, anLangId, (LPCWSTR)ptrRes, resSize );
			}__except(EXCEPTION_EXECUTE_HANDLER){
				pRoot->SetError(SZ_DUMP_EXCEPTION);
			}

			pBin->AddText( _T("\n"), -1, TRUE );

			pRoot = pBin;

		} break;
	case (WORD)RT_VERSION:
		{
			if (resSize > sizeof(VS_FIXEDFILEINFO)) {
				ParseVersionInfo(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy);
			}
		} break;
	case 0x8000+(WORD)RT_VERSION:
		{
			if (resSize > sizeof(VS_FIXEDFILEINFO)) {
				ParseVersionInfoA(pRoot, fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy);
			}
		} break;
	default:
		if (!_tcscmp(pRoot->item.cFileName, _T("AVI")))
			strcat(fileNameBuffer, ".avi");
	}

	pChild = pRoot->AddFile(fileNameBuffer, resSize);
	if (szType[0]) {
		if (!pRoot->IsColumnTitles())
			pRoot->SetColumnsTitles(cszType, 12, cszInfo, 0, -18);

		pChild->SetColumns(szType, szInfo[0] ? szInfo : NULL);
	}

	if (!bDontSetData)
		pChild->SetData((LPBYTE)ptrRes, resSize, bIsCopy);

	if (ptrBuf)
		free(ptrBuf);
	return pChild;
}

//
// Dump the information about one resource directory entry.  If the
// entry is for a subdirectory, call the directory dumping routine
// instead of printing information in this routine.
//
void DumpResourceEntry(
    MPanelItem *pRoot, LPCSTR asID,
	PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry,
    PBYTE pResourceBase,
    DWORD level, DWORD rootType, DWORD parentType )
{
    UINT i;
    char nameBuffer[128];
    PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry;
    
    if ( pResDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY )
    {
        DumpResourceDirectory( pRoot, (PIMAGE_RESOURCE_DIRECTORY)
            ((pResDirEntry->OffsetToData & 0x7FFFFFFF) + pResourceBase),
            pResourceBase, level, pResDirEntry->Name, rootType);
        return;
    }

	pResDataEntry = MakePtr(PIMAGE_RESOURCE_DATA_ENTRY, pResourceBase, pResDirEntry->OffsetToData);

	LPVOID ptrRes = NULL;
	if (g_bIs64Bit)
		ptrRes = GetPtrFromRVA(pResDataEntry->OffsetToData, gpNTHeader64, g_pMappedFileBase);
	else
		ptrRes = GetPtrFromRVA(pResDataEntry->OffsetToData, gpNTHeader32, g_pMappedFileBase);

    if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING )
    {
        GetResourceNameFromId(pResDirEntry->Name, pResourceBase, nameBuffer,
                              sizeof(nameBuffer));
    }
    else
    {
		sprintf(nameBuffer, "0x%04X", pResDirEntry->Name);
    }

	CreateResource(pRoot, rootType, ptrRes, pResDataEntry->Size, asID, nameBuffer, parentType, pResDirEntry->Name);

	// Spit out the spacing for the level indentation
	for ( i=0; i < level; i++ ) pRoot->printf("    ");
	pRoot->printf("Name: %s  DataEntryOffs: %08X\n",
		nameBuffer, pResDirEntry->OffsetToData);

    
    // the resDirEntry->OffsetToData is a pointer to an
    // IMAGE_RESOURCE_DATA_ENTRY.  Go dump out that information.  First,
    // spit out the proper indentation
    for ( i=0; i < level; i++ ) pRoot->printf("    ");
    
    pRoot->printf("DataRVA: %05X  DataSize: %05X  CodePage: %X\n",
            pResDataEntry->OffsetToData, pResDataEntry->Size,
            pResDataEntry->CodePage);


	//pRoot->SetData((LPBYTE)ptrRes, pResDataEntry->Size);

	//if (rootType == (WORD)RT_STRING) {
	//	MPanelItem* pStr = pRoot->Root()->FindChild(_T("String Table"), 0);
	//	if (!pStr) {
	//		pStr = pRoot->Root()->AddFolder(_T("String Table"));
	//		pStr->printf( "<String Table>\n==============\n" );
	//	}

	//	_ASSERTE(parentType<=0x1000); // base of StringID
	//	DumpStringTable( pStr, parentType, pResDirEntry->Name, (LPCWSTR)ptrRes, pResDataEntry->Size );

	//	pStr->printf( "\n" );
	//}
}


//
// Dump the information about one resource directory.
//
void DumpResourceDirectory(	MPanelItem *pRoot, PIMAGE_RESOURCE_DIRECTORY pResDir,
							PBYTE pResourceBase,
							DWORD level,
							DWORD resourceType, DWORD rootType /*= 0*/, DWORD parentType /*= 0*/ )
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
    char szType[64];
    UINT i;

	MPanelItem* pChild = pRoot;

    // Level 1 resources are the resource types
    if ( level == 1 )
    {
		rootType = resourceType;

		if ( resourceType & IMAGE_RESOURCE_NAME_IS_STRING )
		{
			GetResourceNameFromId( resourceType, pResourceBase,
									szType, sizeof(szType) );
		}
		else
		{
	        GetResourceTypeName( resourceType, szType, sizeof(szType) );
		}
	}
    else    // All other levels, just print out the regular id or name
    {
        GetResourceNameFromId( resourceType, pResourceBase, szType,
                               sizeof(szType) );
    }
	
	if (level == 1) {
		// заложимся на то, что в 16бит PE в типах ресурсов установлен бит 0x8000
		if ((resourceType & 0x7FFF) == (WORD)RT_GROUP_CURSOR)
			pChild = pRoot->AddFolder(_T("CURSOR"), FALSE);
		else if ((resourceType & 0x7FFF) == (WORD)RT_GROUP_ICON)
			pChild = pRoot->AddFolder(_T("ICON"), FALSE);
		else
			pChild = pRoot->AddFolder(szType, FALSE);
	}

	if ( level == 1 )
	{
		pChild->printf( "    ---------------------------------------------------"
			"-----------\n" );
	}

    // Spit out the spacing for the level indentation
    for ( i=0; i < level; i++ )
        pChild->printf("    ");

    pChild->printf(
        "ResDir (%s) Entries:%02X (Named:%02X, ID:%02X) TimeDate:%08X",
        szType, pResDir->NumberOfNamedEntries+ pResDir->NumberOfIdEntries,
        pResDir->NumberOfNamedEntries, pResDir->NumberOfIdEntries,
        pResDir->TimeDateStamp );
        
	if ( pResDir->MajorVersion || pResDir->MinorVersion )
		pChild->printf( " Vers:%u.%02u", pResDir->MajorVersion, pResDir->MinorVersion );
	if ( pResDir->Characteristics)
		pChild->printf( " Char:%08X", pResDir->Characteristics );
	pChild->printf( "\n" );

	//
	// The "directory entries" immediately follow the directory in memory
	//
    resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir+1);

	// If it's a stringtable, save off info for future use
	//if (resourceType == (WORD)RT_STRING)
	//{
	//	if ( level == 1 )
	//	{
	//		g_pStrResEntries = resDirEntry;
	//		g_cStrResEntries = pResDir->NumberOfIdEntries;
	//	}
	//}

	// If it's a stringtable, save off info for future use
	if ( level == 1 && (resourceType == (WORD)RT_DIALOG))
	{
		g_pDlgResEntries = resDirEntry;
		g_cDlgResEntries = pResDir->NumberOfIdEntries;
	}
	    
    for ( i=0; i < pResDir->NumberOfNamedEntries; i++, resDirEntry++ )
        DumpResourceEntry(pChild, szType, resDirEntry, pResourceBase, level+1, rootType, resourceType);

    for ( i=0; i < pResDir->NumberOfIdEntries; i++, resDirEntry++ )
        DumpResourceEntry(pChild, szType, resDirEntry, pResourceBase, level+1, rootType, resourceType);
}


//============================================================================

template <class T>
void DumpDialogs( 	MPanelItem *pRoot, PBYTE pImageBase,
					T* pNTHeader,	// 'T' = PIMAGE_NT_HEADERS/32/64
					PBYTE pResourceBase,
					PIMAGE_RESOURCE_DIRECTORY_ENTRY pDlgResEntry,
					DWORD cDlgResEntries )
{
	for ( unsigned i = 0; i < cDlgResEntries; i++, pDlgResEntry++ )
	{
		DWORD offsetToData
			= GetOffsetToDataFromResEntry( pResourceBase, pDlgResEntry );
			
 		PDWORD pDlgStyle = (PDWORD)GetPtrFromRVA(	offsetToData,
													pNTHeader, pImageBase );
		if ( !pDlgStyle )
			break;
		if (!ValidateMemory(pDlgStyle, sizeof(*pDlgStyle)))
			break;

		pRoot->printf( "  ====================\n" );
		if ( HIWORD(*pDlgStyle) != 0xFFFF )
		{
			//	A regular DLGTEMPLATE
			DLGTEMPLATE * pDlgTemplate = ( DLGTEMPLATE * )pDlgStyle;
			if (!ValidateMemory(pDlgTemplate, sizeof(*pDlgTemplate)))
				break;

			pRoot->printf( "  style: %08X\n", pDlgTemplate->style );			
			pRoot->printf( "  extended style: %08X\n", pDlgTemplate->dwExtendedStyle );			

			pRoot->printf( "  controls: %u\n", pDlgTemplate->cdit );
			pRoot->printf( "  (%u,%u) - (%u,%u)\n",
						pDlgTemplate->x, pDlgTemplate->y,
						pDlgTemplate->x + pDlgTemplate->cx,
						pDlgTemplate->y + pDlgTemplate->cy );
			PWORD pMenu = (PWORD)(pDlgTemplate + 1);	// ptr math!

			//
			// First comes the menu
			//
			if ( *pMenu )
			{
				if ( 0xFFFF == *pMenu )
				{
					pMenu++;
					pRoot->printf( "  ordinal menu: %u\n", *pMenu );
				}
				else
				{
					pRoot->printf( "  menu: " );
					while ( *pMenu )
						pRoot->printf( "%c", LOBYTE(*pMenu++) );				

					pMenu++;
					pRoot->printf( "\n" );
				}
			}
			else
				pMenu++;	// Advance past the menu name

			//
			// Next comes the class
			//			
			PWORD pClass = pMenu;
						
			if ( *pClass )
			{
				if ( 0xFFFF == *pClass )
				{
					pClass++;
					pRoot->printf( "  ordinal class: %u\n", *pClass );
				}
				else
				{
					pRoot->printf( "  class: " );
					while ( *pClass )
					{
						pRoot->printf( "%c", LOBYTE(*pClass++) );				
					}		
					pClass++;
					pRoot->printf( "\n" );
				}
			}
			else
				pClass++;	// Advance past the class name
			
			//
			// Finally comes the title
			//

			PWORD pTitle = pClass;
			if ( *pTitle )
			{
				pRoot->printf( "  title: " );

				while ( *pTitle )
					pRoot->printf( "%c", LOBYTE(*pTitle++) );
					
				pTitle++;
			}
			else
				pTitle++;	// Advance past the Title name

			pRoot->printf( "\n" );

			PWORD pFont = pTitle;
						
			if ( pDlgTemplate->style & DS_SETFONT )
			{
				pRoot->printf( "  Font: %u point ",  *pFont++ );
				while ( *pFont )
					pRoot->printf( "%c", LOBYTE(*pFont++) );

				pFont++;
				pRoot->printf( "\n" );
			}
	        else
    	        pFont = pTitle; 

			// DLGITEMPLATE starts on a 4 byte boundary
			LPDLGITEMTEMPLATE pDlgItemTemplate = (LPDLGITEMTEMPLATE)pFont;
			
			for ( unsigned i=0; i < pDlgTemplate->cdit; i++ )
			{
				if (!ValidateMemory(pDlgItemTemplate, sizeof(*pDlgItemTemplate)))
					break;

				// Control item header....
				pDlgItemTemplate = (DLGITEMTEMPLATE *)
									(((DWORD_PTR)pDlgItemTemplate+3) & ~3);
				
				pRoot->printf( "    style: %08X\n", pDlgItemTemplate->style );			
				pRoot->printf( "    extended style: %08X\n",
						pDlgItemTemplate->dwExtendedStyle );			

				pRoot->printf( "    (%u,%u) - (%u,%u)\n",
							pDlgItemTemplate->x, pDlgItemTemplate->y,
							pDlgItemTemplate->x + pDlgItemTemplate->cx,
							pDlgItemTemplate->y + pDlgItemTemplate->cy );
				pRoot->printf( "    id: %u\n", pDlgItemTemplate->id );
				
				//
				// Next comes the control's class name or ID
				//			
				PWORD pClass = (PWORD)(pDlgItemTemplate + 1);
				if ( *pClass )
				{							
					if ( 0xFFFF == *pClass )
					{
						pClass++;
						pRoot->printf( "    ordinal class: %u", *pClass++ );
					}
					else
					{
						pRoot->printf( "    class: " );
						while ( *pClass )
							pRoot->printf( "%c", LOBYTE(*pClass++) );

						pClass++;
						pRoot->printf( "\n" );
					}
				}
				else
					pClass++;
					
				pRoot->printf( "\n" );			

				//
				// next comes the title
				//

				PWORD pTitle = pClass;
				
				if ( *pTitle )
				{
					pRoot->printf( "    title: " );
					if ( 0xFFFF == *pTitle )
					{
						pTitle++;
						pRoot->printf( "%u\n", *pTitle++ );
					}
					else
					{
						while ( *pTitle )
							pRoot->printf( "%c", LOBYTE(*pTitle++) );
						pTitle++;
						pRoot->printf( "\n" );
					}
				}
				else	
					pTitle++;	// Advance past the Title name

				pRoot->printf( "\n" );
				
				PBYTE pCreationData = (PBYTE)(((DWORD_PTR)pTitle + 1) & (0-2) );	// Round up or down to 2 byte boundary (?)
				
				if ( *pCreationData )
					pCreationData += *pCreationData;
				else
					pCreationData++;

				pDlgItemTemplate = (DLGITEMTEMPLATE *)pCreationData;	
				
				pRoot->printf( "\n" );
			}
			
			pRoot->printf( "\n" );
		}
		else
		{
			// A DLGTEMPLATEEX		
		}
		
		pRoot->printf( "\n" );
	}
}


//============================================================================

//
// Top level routine called to dump out the entire resource hierarchy
//
template <class T> void DumpResourceSection(MPanelItem *pRoot, PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS 32/64
{
	DWORD resourcesRVA;
    PBYTE pResDir;

	bool bIs64Bit = ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );

	resourcesRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if ( !resourcesRVA )
		return;

    pResDir = (PBYTE)GetPtrFromRVA( resourcesRVA, pNTHeader, pImageBase );

	if ( !pResDir )
		return;
		
	MPanelItem* pChild = pRoot->AddFolder(_T("Resources"));
    pChild->printf("<Resources, RVA: 0x%X>\n", resourcesRVA );

    DumpResourceDirectory(pChild, (PIMAGE_RESOURCE_DIRECTORY)pResDir, pResDir, 0, 0);

	pChild->printf( "\n" );

	if ( !fShowResources )
		return;
		
	//if ( g_cStrResEntries )
	//{
	//	MPanelItem* pStr = pRoot->AddFolder(_T("String Table"));
	//	pStr->printf( "<String Table>\n" );

	//	if ( bIs64Bit )
	//	{
	//		DumpStringTable( pStr, pImageBase, (PIMAGE_NT_HEADERS64)pNTHeader, pResDir, g_pStrResEntries, g_cStrResEntries );
	//	}
	//	else
	//	{
	//		DumpStringTable( pStr, pImageBase, (PIMAGE_NT_HEADERS32)pNTHeader, pResDir, g_pStrResEntries, g_cStrResEntries );
	//	}
	//	pStr->printf( "\n" );
	//}

	if ( g_cDlgResEntries && !g_bUPXed )
	{
		MPanelItem* pDlg = pRoot->AddFolder(_T("Dialogs"));
		pDlg->printf( "<Dialogs>\n" );

		__try {
			if ( bIs64Bit )
			{
				DumpDialogs( pDlg, pImageBase, (PIMAGE_NT_HEADERS64)pNTHeader, pResDir, g_pDlgResEntries, g_cDlgResEntries );
			}
			else
			{
				DumpDialogs( pDlg, pImageBase, (PIMAGE_NT_HEADERS32)pNTHeader, pResDir, g_pDlgResEntries, g_cDlgResEntries );
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			pDlg->SetError(SZ_DUMP_EXCEPTION);
		}
		pDlg->printf( "\n" );
	}

	pChild->printf( "\n" );
}

void DumpResources( MPanelItem *pRoot, PBYTE pImageBase, PIMAGE_NT_HEADERS32 pNTHeader )
{
	if ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
		DumpResourceSection( pRoot, pImageBase, (PIMAGE_NT_HEADERS64)pNTHeader );
	else
		DumpResourceSection( pRoot, pImageBase, (PIMAGE_NT_HEADERS32)pNTHeader );
}
