//DEBUG:
//F:\VCProject\FarPlugin\#FAR180\far.exe
//C:\Temp\1000-very-long-path\F1..567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\F2..567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890

#include <windows.h>
#include <tchar.h>
#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#define _ASSERT(x)
	#define _ASSERTE(x)
#endif
#ifdef _UNICODE
#include "pluginW.hpp"
#else
#include "pluginA.hpp"
#endif
#include "FileTypes_Lang.h"

#undef _tcslen
#ifdef _UNICODE
#define _tcslen lstrlenW
#else
#define _tcslen lstrlenA
#endif

#ifdef _DEBUG
#define MCHKHEAP _ASSERT(_CrtCheckMemory());
#else
#define MCHKHEAP
#endif

struct PluginStartupInfo psi;
struct FarStandardFunctions FSFW;

LONG GetString(LPCWSTR asKey, LPCWSTR asName, wchar_t* pszValue, DWORD cCount)
{
	HKEY hk; LONG nRegRc; DWORD nSize;
	
	pszValue[0] = 0;

	nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, asKey, 0, KEY_READ, &hk);
	if (!nRegRc) {
		nSize = (cCount-1)*sizeof(*pszValue);
		nRegRc = RegQueryValueExW(hk, NULL, NULL, NULL, (LPBYTE)pszValue, &nSize);
		if (nRegRc) {
			*pszValue = 0;
		} else {
			pszValue[nSize/sizeof(pszValue[0])] = 0;
		}
		RegCloseKey(hk);
	}
	
	return nRegRc;
}

wchar_t* lstrdup(LPCWSTR asText)
{
	int nLen = asText ? lstrlenW(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1)*2);
	if (nLen)
		lstrcpyW(psz, asText);
	else
		psz[0] = 0;
	return psz;
}

void lstrcpy_t(TCHAR* pszDst, int cMaxSize, const wchar_t* pszSrc)
{
	if (cMaxSize<1) return;
#ifdef _UNICODE
	lstrcpyn(pszDst, pszSrc, cMaxSize);
#else
	WideCharToMultiByte(CP_OEMCP, 0, pszSrc, -1, pszDst, cMaxSize, 0,0);
#endif
}

#ifndef _UNICODE
void lstrcpy_t(wchar_t* pszDst, int cMaxSize, const char* pszSrc)
{
	MultiByteToWideChar(CP_OEMCP, 0, pszSrc, -1, pszDst, cMaxSize);
}
#endif

wchar_t* GetString(LPCWSTR asKey, LPCWSTR asName)
{
	HKEY hk; LONG nRegRc; DWORD nSize;
	wchar_t* pszValue = NULL;
	wchar_t szTemp[MAX_PATH];
	
	nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, asKey, 0, KEY_READ, &hk);
	if (!nRegRc) {
		nSize = sizeof(szTemp)-2;
		nRegRc = RegQueryValueExW(hk, NULL, NULL, NULL, (LPBYTE)szTemp, &nSize);
		if (nRegRc == ERROR_MORE_DATA) {
			pszValue = (wchar_t*)malloc(nSize+2);
			if (!pszValue)
				return lstrdup(L"");
			nRegRc = RegQueryValueExW(hk, NULL, NULL, NULL, (LPBYTE)pszValue, &nSize);
			if (nRegRc) {
				*pszValue = 0;
			} else {
				pszValue[nSize/sizeof(pszValue[0])] = 0;
			}
		} else if (nRegRc == 0) {
			szTemp[nSize/sizeof(szTemp[0])] = 0;
			pszValue = lstrdup(szTemp);
		}
		RegCloseKey(hk);
	}
	
	return pszValue;
}

typedef struct tag_FileType {
	FILETIME  ftModified;
	wchar_t   szExt[16]; // Если больше - не показываем
	char      szExtA[16];
} FileType;

typedef struct tag_FileMethod {
	FILETIME  ftModified;
	wchar_t   szName[MAX_PATH+1]; // Если больше - не показываем
	wchar_t   *pszExecute;
	wchar_t   szType[MAX_PATH+1];

#ifndef _UNICODE
	char      szNameA[MAX_PATH+1];
	char     *pszExecuteA;
#endif

	//wchar_t   szTempFileName[32];
} FileMethod;

#define MAX_PLUG_PATH (MAX_PATH*3)

class FTPlugin
{
public:
	wchar_t  szDir[MAX_PLUG_PATH];
	int      nDirLen;
	TCHAR    szTitle[1050];
#ifndef _UNICODE
	char     szDirA[MAX_PLUG_PATH];
#endif
	
	//
	int nFileTypesCount, nMaxTypesCount;
	FileType* pTypes;
	//
	int nMethCount, nMaxMethCount;
	wchar_t szDefaultMethod[MAX_PATH+1];
	FileMethod* pMethods;
	
public:
	FTPlugin() {
		szDir[0] = szDefaultMethod[0] = 0;
		UpdateTitle();
		nFileTypesCount = nMaxTypesCount = nMethCount = nMaxMethCount = 0;
		pTypes = NULL;
		pMethods = NULL;
	};
	
public:
	void UpdateTitle() {
		lstrcpy(szTitle, _T("FileTypes:"));
		
		nDirLen = (int)lstrlenW(szDir);
		if (nDirLen) {
			while (nDirLen && szDir[nDirLen-1] == L'\\')
				szDir[nDirLen--] = 0;
		}
		
		if (szDir[0]) {
			lstrcpy_t(szTitle+10, MAX_PATH*2, szDir);
		}
	};
	void FreeMethods() {
		if (pMethods) {
			for (int i = 0; i < nMethCount; i++) {
				if (pMethods[i].pszExecute)
					free(pMethods[i].pszExecute);
#ifndef _UNICODE
				if (pMethods[i].pszExecuteA)
					free(pMethods[i].pszExecuteA);
#endif
			}
			free(pMethods); pMethods = NULL;
		}
		nMethCount = nMaxMethCount = 0;
		szDefaultMethod[0] = 0;
	};
	void FreeTypeList() {
		FreeMethods();
		if (pTypes) {
			free(pTypes); pTypes = NULL;
		}
		nFileTypesCount = nMaxTypesCount = 0;
	};
	BOOL AddType(const wchar_t* pszExt, FILETIME ftModified) {
		_ASSERTE(nMaxTypesCount > 0 && pTypes != NULL);
		if (nFileTypesCount >= nMaxTypesCount) {
			int nMaxTypes = nMaxTypesCount + 1024;
			FileType* pNewTypes = (FileType*)malloc(nMaxTypes*sizeof(FileType));
			if (!pNewTypes)
				return FALSE;
			memmove(pNewTypes, pTypes, sizeof(FileType)*nFileTypesCount);
			free(pTypes);
			pTypes = pNewTypes; pNewTypes = NULL;
			nMaxTypesCount = nMaxTypes;
		}
		_ASSERTE(lstrlenW(pszExt) < 16);
		lstrcpyW(pTypes[nFileTypesCount].szExt, pszExt);
#ifndef _UNICODE
		lstrcpy_t((char*)(pTypes[nFileTypesCount].szExtA), 16, pszExt);
#endif
		pTypes[nFileTypesCount].ftModified = ftModified;
		nFileTypesCount++;
		return TRUE;
	};
	BOOL AddMethod(const wchar_t* pszType, const wchar_t* pszName, wchar_t** pszExecute, FILETIME ftModified) {
		_ASSERTE(nMaxMethCount > 0 && pMethods != NULL);
		_ASSERTE(pszExecute && *pszExecute);
		
		if (nMethCount >= nMaxMethCount) {
			int nMaxMth = nMaxMethCount + 1024;
			FileMethod* pNewMethods = (FileMethod*)malloc(nMaxMth*sizeof(FileMethod));
			if (!pNewMethods)
				return FALSE;
			memmove(pNewMethods, pMethods, sizeof(FileMethod)*nMethCount);
			free(pMethods);
			pMethods = pNewMethods; pNewMethods = NULL;
			nMaxMethCount = nMaxMth;
		}
		
		_ASSERTE(lstrlenW(pszName) <= (MAX_PATH-1));
		_ASSERTE(lstrlenW(pszType) <= (MAX_PATH));
		if (szDefaultMethod[0] && lstrcmpiW(pszName, szDefaultMethod) == 0) {
			pMethods[nMethCount].szName[0] = L'*';
			lstrcpyW(pMethods[nMethCount].szName+1, pszName);
		} else {
			lstrcpyW(pMethods[nMethCount].szName, pszName);
		}
		lstrcpyW(pMethods[nMethCount].szType, pszType);
		pMethods[nMethCount].pszExecute = *pszExecute;
		*pszExecute = NULL;
		pMethods[nMethCount].ftModified = ftModified;

#ifndef _UNICODE
		lstrcpy_t(pMethods[nMethCount].szNameA, MAX_PATH+1, pMethods[nMethCount].szName);
		if (pMethods[nMethCount].pszExecute && pMethods[nMethCount].pszExecute[0]) {
			int nLen = lstrlenW(pMethods[nMethCount].pszExecute);
			pMethods[nMethCount].pszExecuteA = (char*)malloc(nLen+1);
			lstrcpy_t(pMethods[nMethCount].pszExecuteA, nLen+1, pMethods[nMethCount].pszExecute);
		} else {
			pMethods[nMethCount].pszExecuteA = NULL;
		}
#endif

		//pMethods[nMethCount].szTempFileName[0] = 0;
		nMethCount++;
		
		return TRUE;
	};
	HANDLE LoadTypeList() {
		wchar_t szName[255];
		DWORD nSize, i;
		FILETIME  ftModified;
		LONG nRegRc = 0;
		LONG nMaxTypes = max(2048, nMaxTypesCount);
		
		FreeTypeList();
		
		pTypes = (FileType*)malloc(nMaxTypes*sizeof(FileType));
		if (pTypes == NULL) {
			_ASSERTE(pTypes!=NULL);
			goto wrap;
		}
		nMaxTypesCount = nMaxTypes;
		
		
		for (i = 0;; i++) {
			nSize = 255;
			nRegRc = RegEnumKeyExW(HKEY_CLASSES_ROOT, i, szName, &nSize, NULL, NULL, NULL, &ftModified);
			if (nRegRc == ERROR_SUCCESS) {
				if (szName[0] == L'.' && lstrlenW(szName) < 16) {
					if (!AddType(szName+1, ftModified)) {
						_ASSERT(FALSE);
						break;
					}
				}
			} else if (nRegRc == ERROR_NO_MORE_ITEMS) {
				break; // Ключи кончились
			} else if (nRegRc == ERROR_MORE_DATA) {
				// Слишком длинное имя ключа - не интересует
			} else {
				_ASSERT(nRegRc==0);
			}
		}
		
		
		return (HANDLE)this;
	wrap:
		delete this;
		return INVALID_HANDLE_VALUE;
	};
	
	BOOL LoadMethods() {
		// Показать настроенные методы запуска
		LONG nRegRc;
		wchar_t szPath[MAX_PATH*3], szName[MAX_PATH+1], szType[MAX_PATH+1];
		szPath[0] = L'.'; lstrcpyW(szPath+1, szDir);
		
		FreeMethods();
		
		nRegRc = GetString(szPath, L"", szType, MAX_PATH);
		if (nRegRc != 0)
			return FALSE;
		
		
		lstrcpyW(szPath, szType); lstrcatW(szPath, L"\\shell");
		nRegRc = GetString(szPath, L"", szDefaultMethod, MAX_PATH);
		if (nRegRc) szDefaultMethod[0] = 0;
		
		
		HKEY hk = NULL;
		DWORD nSize, i;
		FILETIME  ftModified;
		LONG nMaxMth = 32;
		wchar_t* pszExecute = NULL;
		
		nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, szPath, 0, KEY_READ, &hk);
		if (nRegRc)
			return FALSE;
		
		pMethods = (FileMethod*)malloc(nMaxMth*sizeof(FileMethod));
		if (pMethods == NULL) {
			_ASSERTE(pMethods!=NULL);
			return FALSE;
		}
		nMaxMethCount = nMaxMth;
		

		wchar_t* pszSlash = szPath + lstrlenW(szPath);
		*(pszSlash++) = L'\\';
		
		for (i = 0;; i++) {
			nSize = MAX_PATH-1;
			nRegRc = RegEnumKeyExW(hk, i, szName, &nSize, NULL, NULL, NULL, &ftModified);
			if (nRegRc == ERROR_SUCCESS) {
				if (lstrlenW(szName) >= (MAX_PATH))
					continue;
				
				lstrcpyW(pszSlash, szName); lstrcatW(pszSlash, L"\\command");
				pszExecute = GetString(szPath, L"");
				if (!pszExecute) pszExecute = lstrdup(L"");
					
				if (!AddMethod(szType, szName, &pszExecute, ftModified)) {
					_ASSERT(FALSE);
					if (pszExecute) { free(pszExecute); pszExecute = NULL; }
					break;
				}

			} else if (nRegRc == ERROR_NO_MORE_ITEMS) {
				break; // Ключи кончились
			} else if (nRegRc == ERROR_MORE_DATA) {
				// Слишком длинное имя ключа - не интересует
			} else {
				_ASSERT(nRegRc==0);
			}
		}
		
		if (pszExecute) { free(pszExecute); pszExecute = NULL; }
		
		if (hk) {
			RegCloseKey(hk); hk = NULL;
		}
		return TRUE;
	};
	
	void EditCurrentItem() {
		if (szDir[0] == 0)
			return; // только для методов
		if (nMethCount == 0)
			return; // пусто
			
		PanelInfo inf; memset(&inf, 0, sizeof(inf));
		INT_PTR nCurLen = 0, i, iMeth = -1;
		PluginPanelItem* item=NULL;
		TCHAR szTemp[MAX_PATH], *pszSlash = NULL;
		TCHAR szTitle[MAX_PATH*2];
		HANDLE hFile = INVALID_HANDLE_VALUE;
		DWORD nWrite = 0, nLen = 0;

#ifdef _UNICODE
		if (!psi.Control((HANDLE)this, FCTL_GETPANELINFO, 0, (LONG_PTR)&inf)) goto wrap;
#else
		if (!psi.Control((HANDLE)this, FCTL_GETPANELINFO, (void*)&inf)) goto wrap;
#endif
		if (inf.ItemsNumber<=0 || inf.CurrentItem==0) goto wrap;
		
#ifdef _UNICODE
		nCurLen = psi.Control((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, NULL);
		item = (PluginPanelItem*)calloc(nCurLen,1);
		if (!psi.Control((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (LONG_PTR)item)) goto wrap;
#else
		item = &(inf.PanelItems[inf.CurrentItem]);
#endif
		
		if ((item->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) goto wrap;
		//nCurLen = lstrlenW(item->FindData.lpwszFileName);
		for (i = 0; i < nMethCount; i++) {
#ifdef _UNICODE
			if (lstrcmpi(item->FindData.lpwszFileName, pMethods[i].szName) == 0) {
				iMeth = i; break;
			}
#else
			if (lstrcmpi(item->FindData.cFileName, pMethods[i].szNameA) == 0) {
				iMeth = i; break;
			}
#endif
		}
		if (iMeth == -1) goto wrap;
		
		//if (pMethods[iMeth].szTempFileName[0]) {
		//	// этот метод уже редактируется - хорошо бы редактор активировать...
		//	goto wrap;
		//}
		
		szTemp[0] = 0;
#ifdef _UNICODE
		FSFW.MkTemp(szTemp, MAX_PATH, L"FLTP");
#else
		FSFW.MkTemp(szTemp, "FLTP");
#endif
		if (!szTemp[0]) goto wrap;
		
		hFile = CreateFile(szTemp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) goto wrap;
#ifdef _UNICODE
		nLen = lstrlen(pMethods[iMeth].pszExecute)*2;
		WriteFile(hFile, pMethods[iMeth].pszExecute, nLen, &nWrite, NULL);
#else
		nLen = lstrlen(pMethods[iMeth].pszExecuteA);
		WriteFile(hFile, pMethods[iMeth].pszExecuteA, nLen, &nWrite, NULL);
#endif
		CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
		
		pszSlash = _tcsrchr(szTemp, _T('\\'));
		if (pszSlash) pszSlash++; else pszSlash = szTemp;
		//lstrcpyn(pMethods[iMeth].szTempFileName, pszSlash, 32);

		lstrcpy_t(szTitle, MAX_PATH, pMethods[iMeth].szType);
		lstrcat(szTitle, _T("\\"));
#ifdef _UNICODE
		lstrcat(szTitle, pMethods[iMeth].szName);
#else
		lstrcat(szTitle, pMethods[iMeth].szNameA);
#endif
		
		i = psi.Editor(szTemp, szTitle, 0,0,-1,-1,
				/*EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONLYFILEONCLOSE|*/EF_DISABLEHISTORY,
				1, 1
#ifdef _UNICODE
				, 1200
#endif
				);
		//if (i == EEC_OPEN_ERROR || i == EEC_LOADING_INTERRUPTED) {
		//	// Неудача
		//pMethods[iMeth].szTempFileName[0] = 0;
		//}
		
		if (i == EEC_MODIFIED) {
			hFile = CreateFile(szTemp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				DWORD nCurSize = GetFileSize(hFile, NULL);
				wchar_t* pszNewValue = NULL;
				char* pszNewValueA = NULL;
				if (nCurSize != INVALID_FILE_SIZE) {
					nLen = lstrlenW(pMethods[iMeth].pszExecute);
					if (nLen <= nCurSize) {
						pszNewValue = (wchar_t*)malloc(nCurSize*2+2);
						if (pszNewValue) {
							free(pMethods[iMeth].pszExecute); pMethods[iMeth].pszExecute = pszNewValue;
						}
#ifndef _UNICODE
						pszNewValueA = (char*)malloc(nCurSize+1);
						if (pszNewValueA) {
							free(pMethods[iMeth].pszExecuteA); pMethods[iMeth].pszExecuteA = pszNewValueA;
						} else {
							free(pMethods[iMeth].pszExecute); pMethods[iMeth].pszExecute = NULL;
						}
#endif
					} else {
						pszNewValue = pMethods[iMeth].pszExecute;
						#ifndef _UNICODE
						pszNewValueA = pMethods[iMeth].pszExecuteA;
						#endif
					}
					if (pszNewValue) {
#ifdef _UNICODE
						if (!ReadFile(hFile, pMethods[iMeth].pszExecute, nCurSize, &nWrite, NULL)) nWrite = 0;
						pszNewValue[nWrite/2] = 0;
						for (wchar_t* pch = pszNewValue; *pch; pch++) {
							if (*pch == L'\t' || *pch == L'\r' || *pch == L'\n')
								*pch = L' ';
						}
#else
						if (!ReadFile(hFile, pMethods[iMeth].pszExecuteA, nCurSize, &nWrite, NULL)) nWrite = 0;
						pszNewValueA[nWrite] = 0;
						for (char* pch = pszNewValueA; *pch; pch++) {
							if (*pch == '\t' || *pch == '\r' || *pch == '\n')
								*pch = ' ';
						}
						lstrcpy_t(pszNewValue, nCurSize+1, pszNewValueA);
#endif

						// Установить в реестре!
						/*
						[HKEY_CLASSES_ROOT\ACDC_JPG\shell\edit\command]
						@="C:\\WINDOWS\\system32\\mspaint.exe \"%1\""
						*/
						wchar_t szFullPath[MAX_PATH*3];
						wchar_t *pszName = pMethods[iMeth].szName;
						if (*pszName == L'*') {
							if (lstrcmpiW(pszName+1, szDefaultMethod) == 0)
								pszName++;
						}
						wsprintfW(szFullPath, L"%s\\shell\\%s\\command", pMethods[iMeth].szType, pszName);
						HKEY hk = NULL;
						if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szFullPath, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL)) {
							wsprintfW(szFullPath, L"Software\\Classes\\%s\\shell\\%s\\command", pMethods[iMeth].szType, pszName);
							if (RegCreateKeyExW(HKEY_CURRENT_USER, szFullPath, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL))
								hk = NULL;							
						}
						if (hk) {
							RegSetValueExW(hk, L"", 0, REG_SZ, (LPBYTE)pszNewValue, 2*(lstrlenW(pszNewValue)+1));
							RegCloseKey(hk);
						}
					}
				}
				CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
			}
		}
		DeleteFile(szTemp);
		
	wrap:
#ifdef _UNICODE
		if (item) {
			free(item); item = NULL;
		}
#endif

		if (hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
		}
	};
	
public:	
	~FTPlugin() {
		FreeTypeList();
	};
};

#if defined(__GNUC__)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			break;
	}
	return TRUE;
}
#endif

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo);
  void WINAPI ExitFARW(void);
  HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item);
  int WINAPI GetMinFarVersionW(void);
  void WINAPI GetPluginInfoW(struct PluginInfo *pi);
  int WINAPI _export GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
  void WINAPI _export FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
  void WINAPI _export GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info);
  void WINAPI _export ClosePluginW(HANDLE hPlugin);
  int WINAPI _export SetDirectoryW ( HANDLE hPlugin, LPCTSTR Dir, int OpMode );
  int WINAPI _export ProcessEventW(HANDLE hPlugin,int Event,void *Param);
  int WINAPI _export ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return DllMain(hDll, dwReason,lpReserved);
}
#endif

#ifdef CRTSTARTUP
extern "C"{
BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return TRUE;
};
};
#endif

int WINAPI _export GetMinFarVersionW(void)
{
	return FARMANAGERVERSION;
}

void WINAPI _export GetPluginInfoW(struct PluginInfo *pi)
{
	static TCHAR szMenu[MAX_PATH];
	lstrcpy(szMenu, psi.GetMsg(psi.ModuleNumber, FTPluginName)); // "FileTypes"
    static TCHAR *pszMenu[1];
    pszMenu[0] = szMenu;

	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = 0;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = pszMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 'FlTp';
	MCHKHEAP
}

void WINAPI _export SetStartupInfoW(const PluginStartupInfo *aInfo)
{
	::psi = *aInfo;
	::FSFW = *aInfo->FSF;
	::psi.FSF = &::FSFW;
}

void   WINAPI _export ExitFARW(void)
{
}

HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	FTPlugin* pPlugin = NULL;

	pPlugin = new FTPlugin();
	if (!pPlugin) return INVALID_HANDLE_VALUE;

	return pPlugin->LoadTypeList();
}

int WINAPI _export GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
	int nCount = 0, i;
	//LARGE_INTEGER ll;
	PluginPanelItem* ppi = NULL;

	if (pPlugin->szDir[0]) {
		// Показать настроенные методы запуска
		if (!pPlugin->LoadMethods()) {
			*pItemsNumber = 0;
			_ASSERTE(*pPanelItem == NULL);
		} else {
			nCount = pPlugin->nMethCount;
			*pItemsNumber = nCount;
			ppi = (PluginPanelItem*)calloc(nCount, sizeof(PluginPanelItem));
			*pPanelItem = ppi;

			MCHKHEAP

			for (i=0; i<nCount; i++) {
				ppi[i].FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
				ppi[i].FindData.ftCreationTime = pPlugin->pMethods[i].ftModified;
				ppi[i].FindData.ftLastAccessTime = pPlugin->pMethods[i].ftModified;
				ppi[i].FindData.ftLastWriteTime = pPlugin->pMethods[i].ftModified;
#ifdef _UNICODE
				ppi[i].FindData.lpwszFileName = pPlugin->pMethods[i].szName;
				ppi[i].Description = pPlugin->pMethods[i].pszExecute;
				ppi[i].FindData.nFileSize = lstrlen(pPlugin->pMethods[i].pszExecute)*2;
#else
				lstrcpy(ppi[i].FindData.cFileName, pPlugin->pMethods[i].szNameA);
				ppi[i].Description = pPlugin->pMethods[i].pszExecuteA;
				ppi[i].FindData.nFileSizeLow = lstrlenW(pPlugin->pMethods[i].pszExecute);
#endif
			}
		}
		
		
	} else {
		nCount = pPlugin->nFileTypesCount;
		*pItemsNumber = nCount;
		ppi = (PluginPanelItem*)calloc(nCount, sizeof(PluginPanelItem));
		*pPanelItem = ppi;

		MCHKHEAP

		for (i=0; i<nCount; i++) {
			ppi[i].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			ppi[i].FindData.ftCreationTime = pPlugin->pTypes[i].ftModified;
			ppi[i].FindData.ftLastAccessTime = pPlugin->pTypes[i].ftModified;
			ppi[i].FindData.ftLastWriteTime = pPlugin->pTypes[i].ftModified;
			//ll.LowPart = iter->nFileSizeLow;
			//ll.HighPart = iter->nFileSizeHigh;
			//ppi[i].FindData.nFileSize = ll.QuadPart;
#ifdef _UNICODE
			ppi[i].FindData.lpwszFileName = pPlugin->pTypes[i].szExt;
#else
			lstrcpy(ppi[i].FindData.cFileName, pPlugin->pTypes[i].szExtA);
#endif
		}
	}
	MCHKHEAP

	return TRUE;
}

void WINAPI _export FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	if (PanelItem)
		free(PanelItem);
	return;
}

void WINAPI _export GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
#ifdef _UNICODE
	Info->CurDir = pPlugin->szDir;
#else
	Info->CurDir = pPlugin->szDirA;
#endif
	Info->HostFile = NULL;
	Info->PanelTitle = pPlugin->szTitle;
	Info->Flags = OPIF_USEFILTER|OPIF_USESORTGROUPS|OPIF_USEHIGHLIGHTING|OPIF_ADDDOTS;
	MCHKHEAP
}

void WINAPI _export ClosePluginW(HANDLE hPlugin)
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
	if (pPlugin) {
		delete pPlugin;
	}
}


int WINAPI _export SetDirectoryW ( HANDLE hPlugin, LPCTSTR Dir, int OpMode )
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
	if (!pPlugin)
		return FALSE;
		
	if ((Dir[0] == _T('\\') && Dir[1] == 0) 
		|| (Dir[0] == _T('.') && Dir[1] == _T('.') && Dir[2] == 0 && pPlugin->szDir[0] == 0))
	{

		pPlugin->szDir[0] = 0;
		pPlugin->UpdateTitle();

		return TRUE;
	}
	
	if (lstrcmp(Dir, _T(".."))==0) {
		_ASSERTE(pPlugin->nDirLen>0);
		MCHKHEAP
		pPlugin->szDir[--(pPlugin->nDirLen)] = 0;
		wchar_t* pszSlash = wcsrchr(pPlugin->szDir, L'\\');
		MCHKHEAP
		if (!pszSlash) {
			pPlugin->szDir[0] = 0;
			pPlugin->UpdateTitle();
			return TRUE;
		}
		pszSlash[1] = 0;
		pPlugin->UpdateTitle();
		return TRUE;
	}
	if (Dir[0] != _T('\\')) {
		MCHKHEAP
		int nAddLen = lstrlen(Dir)+1;
		if ((pPlugin->nDirLen + nAddLen + 1) >= MAX_PLUG_PATH)
			return FALSE;

		if (pPlugin->nDirLen) {	
			if (pPlugin->szDir[pPlugin->nDirLen-1]!=L'\\') {
				pPlugin->szDir[pPlugin->nDirLen] = L'\\';
				pPlugin->szDir[pPlugin->nDirLen+1] = 0;
				pPlugin->nDirLen ++;
			}
		}
		
		lstrcpy_t(pPlugin->szDir+pPlugin->nDirLen, MAX_PLUG_PATH-pPlugin->nDirLen, Dir);
		MCHKHEAP
		pPlugin->nDirLen = lstrlenW(pPlugin->szDir);
		
		MCHKHEAP
		pPlugin->UpdateTitle();
		return TRUE;
	}
	return FALSE;
}

int WINAPI _export ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState)
{
	//if ((Key & 0xFF) >= VK_F1 && (Key & 0xFF) <= VK_F12)
	//	CheckCurrentDirectory((FTPlugin*)hPlugin, FALSE);
	if (((Key & 0xFF) == VK_F4) && ControlState == 0) {
		FTPlugin* pPlugin = (FTPlugin*)hPlugin;
		if (pPlugin->szDir[0]) {
			pPlugin->EditCurrentItem();
			return TRUE;
		}
	} else
	if (((Key & 0xFF) == 'R') && ControlState == PKF_CONTROL) {
		FTPlugin* pPlugin = (FTPlugin*)hPlugin;
		if (pPlugin->szDir[0]) {
			pPlugin->LoadMethods();
			return FALSE; // продолжить в фаре
		}
	}
	return FALSE;
}

//int WINAPI _export ProcessEditorEventW(int Event,void *Param)
//{
//	if (Event == EE_SAVE) {
//		WindowInfo wi = {-1};
//		if (psi.AdvControl(psi.ModuleNumber, ACTL_GETWINDOWINFO, &wi)
//			&& wi.Type == WTYPE_EDITOR)
//		{
//			wi.Name=(wchar_t *) malloc(wi.NameSize);
//			if (psi.AdvControl(psi.ModuleNumber, ACTL_GETWINDOWINFO, &wi))
//			{
//			}
//			free(wi.Name);
//		}
//	}
//	return 0;
//}
