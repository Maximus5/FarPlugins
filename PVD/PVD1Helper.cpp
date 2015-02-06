#include <windows.h>
#include "PictureViewPlugin.h"
#ifdef _DEBUG
	#ifndef WIN64
		#include <crtdbg.h>
	#else
		#define _ASSERTE(a)
	#endif
#else
	#ifndef _ASSERTE
		#define _ASSERTE(a)
	#endif
#endif

#define sizeofarray(array) (sizeof(array)/sizeof(*array))
#define ZeroStruct(s) memset(&(s),0,sizeof(s)); (s).cbSize = sizeof(s)

typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef DWORD u32;

HMODULE ghModule = NULL;

// Функция требуется только для заполнения переменной ghModule
// Если плагин содержит свою точку входа - для использования PVD1Helper
// ему необходимо заполнять ghModule самостоятельно
BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		ghModule = (HMODULE)hModule;
	return TRUE;
}

UINT LoadFarVersion()
{
	static DWORD dwVer = 0;
	if (dwVer) return dwVer;

	WCHAR FarPath[MAX_PATH+1];
	DWORD dwSize = 0, dwRsrvd = 0;
	void *pVerData = NULL;

	dwSize = GetModuleFileName(0,FarPath,MAX_PATH);
	if (!dwSize || dwSize>=MAX_PATH) goto wrap;

	dwSize = GetFileVersionInfoSize(FarPath, &dwRsrvd);
	if (dwSize>0) {
		void *pVerData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
		if (pVerData) {
			VS_FIXEDFILEINFO *lvs = NULL;
			UINT nLen = sizeof(lvs);
			if (GetFileVersionInfo(FarPath, 0, dwSize, pVerData)) {
				TCHAR szSlash[3]; lstrcpyW(szSlash, L"\\");
				if (VerQueryValue ((void*)pVerData, szSlash, (void**)&lvs, &nLen)) {
					dwVer = HIWORD(lvs->dwFileVersionMS);
					//dwBuild = lvs->dwFileVersionLS;
				}
			}
			HeapFree(GetProcessHeap(), 0, pVerData);
		}
	}

	if (!dwVer) dwVer = 2;
wrap:
	return dwVer;
}

static int gnInitCounter = 0;
static void* gpInitContext = NULL;

struct VerMap
{
	void *pContext, *pImageContext;
	pvdInfoImage2 InfoImage2;
	pvdInfoPage2 InfoPage2;
	pvdInfoDecode2 DecodeInfo2;

	char szFormatName[MAX_PATH];  // название формата файла
	char szCompression[MAX_PATH]; // алгоритм сжатия
	char szComments[MAX_PATH];    // различные комментарии о файле

	pvdDecodeCallback DecodeCallback1;
	void *pDecodeCallbackContext1;

	wchar_t szRegKey[MAX_PATH*3];

	wchar_t* GetRegKey() {
		if (!ghModule)
			return NULL;

		DWORD nSize;
		wchar_t sUserName[MAX_PATH], sFileName[MAX_PATH], *pszName;

		// GetModuleBaseName defined in WinNT 4 of higher
		nSize = GetModuleFileName(ghModule, sFileName, MAX_PATH);
		if (!nSize || nSize>=MAX_PATH)
			return NULL;
		pszName = sFileName+nSize-1;
		while (pszName>sFileName && *(pszName-1)!=L'\\') pszName--;
		CharLowerBuffW(pszName, lstrlenW(pszName));

		if (LoadFarVersion() == 2)
			lstrcpyW(szRegKey, L"Software\\Far2\\");
		else
			lstrcpyW(szRegKey, L"Software\\Far\\");

		nSize = GetEnvironmentVariable(L"FARUSER", sUserName, MAX_PATH);
		if (nSize && nSize < MAX_PATH) {
			lstrcatW(szRegKey, L"Users\\");
			lstrcatW(szRegKey, sUserName);
			lstrcatW(szRegKey, L"\\");
		}

		lstrcatW(szRegKey, L"Plugins\\PictureView\\");
		lstrcatW(szRegKey, pszName);
		lstrcatW(szRegKey, L"\\Settings");

		return szRegKey;
	};
};

BOOL __stdcall DecodeCallback2(void *pDecodeCallbackContext2, UINT32 iStep, UINT32 nSteps,
							   pvdInfoDecodeStep2* pImagePart)
{
	if (pDecodeCallbackContext2) {
		struct VerMap *pVerMap = (struct VerMap*)pDecodeCallbackContext2;
		if (pVerMap->DecodeCallback1)
			return pVerMap->DecodeCallback1(pVerMap->pDecodeCallbackContext1, iStep, nSteps);
	}
	return TRUE;
}

// 0-информация, 1-предупреждение, 2-ошибка
void __stdcall wrapMessageLog(void *pCallbackContext, const wchar_t* asMessage, UINT32 anSeverity)
{
	OutputDebugStringW(asMessage ? asMessage : L"<NULL>");
	OutputDebugStringW(L"\n");
}

// asExtList может содержать '*' (тогда всегда TRUE) или '.' (TRUE если asExt пусто). Сравнение регистронезависимое
BOOL __stdcall wrapExtensionMatch(wchar_t* asExtList, const wchar_t* asExt)
{
	if (!asExtList || !asExt)
		return false;
	// asExt может быть "", тогда в asExtList нужно проверять "." (т.к. без расширения)
	if (!*asExtList /*|| !*asExt*/)
		return false;

	while (*asExtList)
	{
		wchar_t* pszNext = wcschr(asExtList, L',');
		
		if (pszNext) *pszNext = 0; // сделать ASCIIZ
		bool bEqual = false;
		switch (*asExtList)
		{
		case L':':
			//-- во враппере не поддерживается
			////В обработку расширений (активных/неактивных/запрещенных) добавить наряду
			////с самими расширениями и сигнатуры файлов. Проверять первые несколько (16) байт.
			////Пример: djv,djvu,:AT&TFORM
			////Hex: \FF\FE
			////Если нужен просто слэш - \\
			//if (apFileData && anFileDataSize)
			//	bEqual = DataMatch(asExtList, apFileData, anFileDataSize);
			break;
		case 0:
			break;
		default:
			// Переделать наверное на настоящие маски? Хотя слишком длинные строки в настройке получатся...
			if ((*asExtList == L'*') && (asExtList[1] == 0))
				bEqual = true;
			else if ((*asExtList == L'.') && (asExtList[1] == 0) && (*asExt == 0))
				bEqual = true;
			else
				bEqual = (lstrcmpi(asExtList, asExt) == 0);
		}
		if (pszNext) *pszNext = L','; // вернуть

		if (bEqual)
		{
			return true;
		}
		else if (!pszNext)
		{
			break;
		}
		else
		{
			asExtList = pszNext + 1;
		}
	}
	
	return false;

	//if (!asExtList || !asExt) return FALSE;
	//if (!*asExtList) return FALSE;

	//if (!asExt || !*asExt) asExt = L".";
	//if (wcschr(asExtList,L'*'))
	//	return TRUE;

	//while (*asExtList)
	//{
	//	wchar_t* pszNext = wcschr(asExtList, L',');
	//	if (pszNext) *pszNext = 0;
	//	bool bEqual = lstrcmpi(asExtList, asExt) == 0;
	//	if (pszNext) *pszNext = L',';

	//	if (bEqual)
	//	{
	//		return TRUE;
	//	}
	//	else if (!pszNext)
	//	{
	//		break;
	//	}
	//	else
	//	{
	//		asExtList = pszNext + 1;
	//	}
	//}
	//return FALSE;
}

BOOL __stdcall wrapCallSehed(pvdCallSehedProc2 CalledProc, LONG_PTR Param1, LONG_PTR Param2, LONG_PTR* Result)
{
	LONG_PTR rc = CalledProc(Param1, Param2);
	if (Result) *Result = rc;
	return TRUE;
}

int __stdcall wrapSortExtensions(wchar_t* pszExtensions)
{
	// Not needed in PVD1
	return 0;
}

int __stdcall wrapMulDivI32(int a, int b, int c)  // a * (__int64)b / c;
{
	return a * b / c;
}

UINT __stdcall wrapMulDivU32(UINT a, UINT b, UINT c)
{
	return ((a)*(b)/(c));
}
	
UINT __stdcall wrapMulDivU32R(UINT a, UINT b, UINT c)
{
	return (((a)*(b) + (c)/2)/(c));
}

int __stdcall wrapMulDivIU32R(int a, UINT b, UINT c)
{
	return (((a)*(b) + (c)/2)/(c));
}

UINT32 __stdcall pvdInit(void)
{
	return PVD_CURRENT_INTERFACE_VERSION;
}

void __stdcall pvdExit(void)
{
	if (gpInitContext != NULL) 
	{
		pvdExit2(gpInitContext);
		gpInitContext = NULL;
	}
	gnInitCounter = 0;
}

void __stdcall pvdPluginInfo(pvdInfoPlugin *pPluginInfo)
{
	pvdInfoPlugin2 info = {sizeof(pvdInfoPlugin2)};
	pvdPluginInfo2(&info);
	pPluginInfo->Priority = 1; //-- info.Priority из второй версии намного больше 10
	static char szName[32]; int nLen=WideCharToMultiByte(CP_UTF8,0,info.pName,-1,szName,sizeof(szName)-1,0,0); szName[nLen]=0;
	pPluginInfo->pName = szName;
	static char szVersion[16]; nLen=WideCharToMultiByte(CP_UTF8,0,info.pVersion,-1,szVersion,sizeof(szVersion)-1,0,0); szVersion[nLen]=0;
	pPluginInfo->pVersion = szVersion;
	static char szComments[64]; nLen=WideCharToMultiByte(CP_UTF8,0,info.pVersion,-1,szComments,sizeof(szComments)-1,0,0); szComments[nLen]=0;
	pPluginInfo->pComments = szComments;
}

BOOL __stdcall pvdFileOpen(const char *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage *pImageInfo, void **ppContext)
{
	VerMap *pVerMap = (VerMap*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VerMap));
	if (!pVerMap) return FALSE;

	if (gnInitCounter == 0) {
		pvdInitPlugin2 init = {sizeof(pvdInitPlugin2)};
		init.nMaxVersion = PVD_UNICODE_INTERFACE_VERSION;
		init.pRegKey = pVerMap->GetRegKey();
		init.ExtensionMatch = wrapExtensionMatch;
		init.MessageLog = wrapMessageLog;
		init.hModule = ghModule;
		init.CallSehed = wrapCallSehed;
		init.SortExtensions = wrapSortExtensions;
		init.MulDivI32 = wrapMulDivI32;
		init.MulDivU32 = wrapMulDivU32;
		init.MulDivU32R = wrapMulDivU32R;
		init.MulDivIU32R = wrapMulDivIU32R;
		init.Flags = PVD_IPF_COMPAT_MODE;

		UINT32 nRc = pvdInit2(&init);
		if (nRc != PVD_UNICODE_INTERFACE_VERSION) {
			pvdExit2(init.pContext);
			HeapFree(GetProcessHeap(), 0, pVerMap);
			return FALSE;
		}
		_ASSERTE(init.pContext); // PicView не волнует, что содержит это поле, но проверим, что плагин про него знает
		pVerMap->pContext = init.pContext;
		gpInitContext = init.pContext;
		gnInitCounter ++;
		_ASSERTE(gnInitCounter == 1);
	} else {
		pVerMap->pContext = gpInitContext;
	}
	gnInitCounter ++;

	// Для отладки версии 2
	static bool bFormatRetrieved = false;
	if (!bFormatRetrieved) {
		bFormatRetrieved = true;
		pvdFormats2 fmt = {sizeof(pvdFormats2)};
		pvdGetFormats2(gpInitContext, &fmt);
	}

	wchar_t szFileName[MAX_PATH*4];
	int nLen=MultiByteToWideChar(CP_UTF8,0,pFileName,-1,szFileName,sizeofarray(szFileName)-1); szFileName[nLen] = 0;


	ZeroStruct(pVerMap->InfoImage2);

	BOOL lbRc = pvdFileOpen2(pVerMap->pContext, szFileName, lFileSize, pBuf, lBuf, &(pVerMap->InfoImage2));

	if (!lbRc) {
		if (gnInitCounter > 0) {
			gnInitCounter--;
			_ASSERTE(gnInitCounter > 0);
		}
		HeapFree(GetProcessHeap(), 0, pVerMap);
	} else {
		_ASSERTE(pVerMap->InfoImage2.pImageContext); // PicView не волнует, что содержит это поле, но проверим, что плагин про него знает
		*ppContext = pVerMap;

		pImageInfo->nPages = pVerMap->InfoImage2.nPages;
		pImageInfo->Flags = pVerMap->InfoImage2.Flags & PVD_IIF_ANIMATED; // Поддерживаемые флаги в версии 1
		nLen=pVerMap->InfoImage2.pFormatName ? WideCharToMultiByte(CP_UTF8,0,pVerMap->InfoImage2.pFormatName,-1,
			pVerMap->szFormatName,sizeof(pVerMap->szFormatName)-1,0,0) : 0;
		pVerMap->szFormatName[nLen]=0;
		pImageInfo->pFormatName = pVerMap->szFormatName;
		nLen=pVerMap->szCompression ? WideCharToMultiByte(CP_UTF8,0,pVerMap->InfoImage2.pCompression,-1,
			pVerMap->szCompression,sizeof(pVerMap->szCompression)-1,0,0) : 0;
		pVerMap->szCompression[nLen]=0;
		pImageInfo->pCompression = pVerMap->szCompression;
		nLen=pVerMap->szComments ? WideCharToMultiByte(CP_UTF8,0,pVerMap->InfoImage2.pComments,-1,
			pVerMap->szComments,sizeof(pVerMap->szComments)-1,0,0) : 0;
		pVerMap->szComments[nLen]=0;
		pImageInfo->pComments = pVerMap->szComments;
	}

	return lbRc;
}

BOOL __stdcall pvdPageInfo(void *pContext, UINT32 iPage, pvdInfoPage *pPageInfo)
{
	_ASSERTE(pContext);
	if (!pContext) {
		OutputDebugStringW(L"pContext == NULL in pvdPageInfo\n");
		return FALSE;
	}

	VerMap* pVerMap = (VerMap*)pContext;
	ZeroStruct(pVerMap->InfoPage2);

	pVerMap->InfoPage2.iPage = iPage;
	BOOL lbRc = pvdPageInfo2(pVerMap->pContext, pVerMap->InfoImage2.pImageContext, &(pVerMap->InfoPage2));

	if (lbRc) {
		pPageInfo->lWidth = pVerMap->InfoPage2.lWidth;
		pPageInfo->lHeight = pVerMap->InfoPage2.lHeight;
		pPageInfo->nBPP = pVerMap->InfoPage2.nBPP;
		pPageInfo->lFrameTime = pVerMap->InfoPage2.lFrameTime;
	}

	return lbRc;
}

BOOL __stdcall pvdPageDecode(void *pContext, UINT32 iPage, pvdInfoDecode *pDecodeInfo, pvdDecodeCallback DecodeCallback, void *pDecodeCallbackContext)
{
	_ASSERTE(pContext);
	if (!pContext) {
		OutputDebugStringW(L"pContext == NULL in pvdPageDecode\n");
		return FALSE;
	}

	VerMap* pVerMap = (VerMap*)pContext;
	pVerMap->DecodeCallback1 = DecodeCallback;
	pVerMap->pDecodeCallbackContext1 = pDecodeCallbackContext;

	ZeroStruct(pVerMap->DecodeInfo2);
	pVerMap->DecodeInfo2.iPage = iPage;
	pVerMap->DecodeInfo2.Flags = PVD_IDF_COMPAT_MODE;

	BOOL lbRc = pvdPageDecode2(pVerMap->pContext, pVerMap->InfoImage2.pImageContext,
		&(pVerMap->DecodeInfo2), DecodeCallback2, pVerMap);

	if (lbRc) {
		if (pVerMap->DecodeInfo2.lWidth != pVerMap->InfoPage2.lWidth 
			|| pVerMap->DecodeInfo2.lHeight != pVerMap->InfoPage2.lHeight)
		{
			// "Уточнение" декодированного размера изображения в версии 1 недопустимо
			OutputDebugStringW(L"Image size mismatch!\n");
			pvdPageFree2(pVerMap->pContext, pVerMap->InfoImage2.pImageContext, &(pVerMap->DecodeInfo2));
			lbRc = FALSE;
		} else {
			pDecodeInfo->Flags = pVerMap->DecodeInfo2.Flags & PVD_IDF_READONLY; // Допустимые флаги в версии 
			pDecodeInfo->pImage = pVerMap->DecodeInfo2.pImage;
			pDecodeInfo->pPalette = pVerMap->DecodeInfo2.pPalette;
			pDecodeInfo->nBPP = pVerMap->DecodeInfo2.nBPP;
			pDecodeInfo->nColorsUsed = pVerMap->DecodeInfo2.nColorsUsed;
			pDecodeInfo->lImagePitch = pVerMap->DecodeInfo2.lImagePitch;
		}
	}

	return lbRc;
}

void __stdcall pvdPageFree(void *pContext, pvdInfoDecode *pDecodeInfo)
{
	_ASSERTE(pContext);
	if (!pContext) {
		OutputDebugStringW(L"pContext == NULL in pvdPageFree\n");
	} else {
		VerMap* pVerMap = (VerMap*)pContext;
		pvdPageFree2(pVerMap->pContext, pVerMap->InfoImage2.pImageContext, &(pVerMap->DecodeInfo2));
	}
}

void __stdcall pvdFileClose(void *pContext)
{
	_ASSERTE(pContext);
	if (!pContext) {
		OutputDebugStringW(L"pContext == NULL in pvdFileClose\n");
	} else {
		VerMap* pVerMap = (VerMap*)pContext;
		pvdFileClose2(pVerMap->pContext, pVerMap->InfoImage2.pImageContext);
		if (gnInitCounter > 0) {
			gnInitCounter--;
			if (gnInitCounter == 0) {
				_ASSERTE(((VerMap*)pContext)->pContext == gpInitContext);
				pvdExit2(((VerMap*)pContext)->pContext);
				gpInitContext = NULL;
			}
		}
		HeapFree(GetProcessHeap(), 0, pContext);
	}
}
