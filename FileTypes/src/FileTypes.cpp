
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

#ifdef _UNICODE
    #define _tcsscanf swscanf
    #define SETMENUTEXT(itm,txt) itm.Text = txt;
    #define F757NA 0, (LONG_PTR)
    #define _GetCheck(i) (int)psi.SendDlgMessage(hDlg,DM_GETCHECK,i,0)
    #define GetDataPtr(i) ((const TCHAR *)psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
    #define SETTEXT(itm,txt) itm.PtrData = txt
    #define SETTEXTPRINT(itm,fmt,arg) wsprintf(pszBuf, fmt, arg); SETTEXT(itm,pszBuf); pszBuf+=lstrlen(pszBuf)+2;
    #define _tcstoi _wtoi
	#define FILENAMEPTR(p) (p).lpwszFileName
#else
    #define _tcsscanf sscanf
    #define SETMENUTEXT(itm,txt) lstrcpy(itm.Text, txt);
    #define F757NA (void*)
    #define _GetCheck(i) items[i].Selected
    #define GetDataPtr(i) items[i].Ptr.PtrData
    #define SETTEXT(itm,txt) lstrcpy(itm.Data, txt)
    #define SETTEXTPRINT(itm,fmt,arg) wsprintf(itm.Data, fmt, arg)
    #define _tcstoi atoi
	#define FILENAMEPTR(p) (p).cFileName
#endif
#define NUM_ITEMS(X) (sizeof(X)/sizeof(X[0]))

struct PluginStartupInfo psi;
struct FarStandardFunctions FSFW;

int WINAPI _export SetDirectoryW ( HANDLE hPlugin, LPCTSTR Dir, int OpMode );


LONG GetString(LPCWSTR asKey, LPCWSTR asName, wchar_t* pszValue, DWORD cCount, HKEY hRoot=HKEY_CLASSES_ROOT)
{
	HKEY hk; LONG nRegRc; DWORD nSize;
	
	pszValue[0] = 0;

	nRegRc = RegOpenKeyExW(hRoot, asKey, 0, KEY_READ, &hk);
	if (!nRegRc) {
		nSize = (cCount-1)*sizeof(*pszValue);
		nRegRc = RegQueryValueExW(hk, asName, NULL, NULL, (LPBYTE)pszValue, &nSize);
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

wchar_t* GetString(LPCWSTR asKey, LPCWSTR asName, HKEY hRoot=HKEY_CLASSES_ROOT)
{
	HKEY hk; LONG nRegRc; DWORD nSize;
	wchar_t* pszValue = NULL;
	wchar_t szTemp[MAX_PATH];
	
	nRegRc = RegOpenKeyExW(hRoot, asKey, 0, KEY_READ, &hk);
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

TCHAR *GetMsg(int MsgId) {
    return((TCHAR *)psi.GetMsg(psi.ModuleNumber, MsgId));
}

#define FTMAGIC_TYPE 0x71235693
#define FTMAGIC_METH 0x61245952

#define FTO_HKCR 1       // Информация найдена в HKEY_CLASSES_ROOT (например HKCR\.jpg)
#define FTO_PROGID 2 // [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.jpg\OpenWithProgids]
#define FTO_USERCHOICE 4 // [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.jpg\UserChoice]

struct FileType
{
	DWORD nMagic;
	DWORD nFlags; // FTO_xxx
	FILETIME  ftModified;
	wchar_t   szExt[16]; // Если больше - не показываем
	#ifndef _UNICODE
	char      szExtO[16];
	#endif
	TCHAR     szDesc[128];
};

struct FileMethod
{
	DWORD nMagic;
	DWORD nFlags; // FTO_xxx
	FileType* pft;
	FILETIME  ftModified;
	wchar_t   szName[MAX_PATH+1]; // Если больше - не показываем
	wchar_t  *pszRealName;
	TCHAR     szSort[2]; // '*' для метода по умолчанию
	
	wchar_t   szType[MAX_PATH+1];

	//HKEY      hRegRoot;
	wchar_t   szRegPath[MAX_PATH*2];

#ifdef _UNICODE
	wchar_t  *pszExecute;
#else
	char     *pszExecuteA;
	char     *pszExecuteO;
	char      szNameO[MAX_PATH+1];
#endif

};

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
	FileType* pTypes, *pCurType;
	//
	int nMethCount, nMaxMethCount;
	wchar_t szDefaultMethod[MAX_PATH+1];
	FileMethod* pMethods;
	
	//
	PanelMode pPanelModes[10], ExtPanelMode, MethPanelMode;
	const TCHAR* sExtPanelTitles[2];
	const TCHAR* sMethPanelTitles[3];

	//
	wchar_t sTemp1[MAX_PATH+1], sTemp2[MAX_PATH+1], sTemp3[MAX_PATH+1];
	TCHAR sDialogExecBuf[8000];
	
public:
	FTPlugin()
	{
		szDir[0] = szDefaultMethod[0] = 0;
		UpdateTitle();
		nFileTypesCount = nMaxTypesCount = nMethCount = nMaxMethCount = 0;
		pTypes = pCurType = NULL;
		pMethods = NULL;
		memset(pPanelModes, 0, sizeof(pPanelModes));
		memset(&ExtPanelMode, 0, sizeof(ExtPanelMode));
		memset(&MethPanelMode, 0, sizeof(MethPanelMode));
		
		sExtPanelTitles[0] = GetMsg(FTTitleName);
		sExtPanelTitles[1] = GetMsg(FTTitleDescription);
		ExtPanelMode.ColumnTypes = GetMsg(FTListColumnTypes);
		ExtPanelMode.ColumnWidths = GetMsg(FTListColumnWidth);
		ExtPanelMode.ColumnTitles = (TCHAR**)sExtPanelTitles;
		ExtPanelMode.DetailedStatus = TRUE;
		
		sMethPanelTitles[0] = GetMsg(FTTitleName);
		sMethPanelTitles[1] = GetMsg(FTTitleSort);
		sMethPanelTitles[2] = GetMsg(FTTitleCommand);
		MethPanelMode.ColumnTypes = GetMsg(FTMethColumnTypes);
		MethPanelMode.ColumnWidths = GetMsg(FTMethColumnWidth);
		MethPanelMode.ColumnTitles = (TCHAR**)sMethPanelTitles;
		MethPanelMode.DetailedStatus = TRUE;
		
		pPanelModes[0] = ExtPanelMode;
	};
	
public:
	void UpdateTitle()
	{
		lstrcpy(szTitle, _T("FileTypes:"));
		
		nDirLen = (int)lstrlenW(szDir);
		if (nDirLen)
		{
			while (nDirLen && szDir[nDirLen-1] == L'\\')
				szDir[nDirLen--] = 0;
		}

		pCurType = NULL;
		for (int i=0; i<nFileTypesCount; i++)
		{
			if (!lstrcmpiW(szDir, pTypes[i].szExt))
			{
				pCurType = pTypes+i; break;
			}
		}
		if (!pCurType)
		{
			szDir[0] = 0; nDirLen = 0;
		}
		
		if (szDir[0])
		{
			#ifndef _UNICODE
			lstrcpy_t(szDirA, MAX_PLUG_PATH, szDir);
			lstrcpyn(szTitle+10, szDirA, MAX_PATH*2);
			#else
			lstrcpy_t(szTitle+10, MAX_PATH*2, szDir);
			#endif
		} 
		#ifndef _UNICODE
		else
		{
			szDirA[0] = 0;
		}
		#endif

		if (szDir[0])
			pPanelModes[0] = MethPanelMode;
		else
			pPanelModes[0] = ExtPanelMode;
	};
	void FreeMethods()
	{
		if (pMethods)
		{
			for (int i = 0; i < nMethCount; i++)
			{
				#ifdef _UNICODE
				if (pMethods[i].pszExecute)
					free(pMethods[i].pszExecute);
				#else
				if (pMethods[i].pszExecuteA)
					free(pMethods[i].pszExecuteA);
				if (pMethods[i].pszExecuteO)
					free(pMethods[i].pszExecuteO);
				#endif
			}
			free(pMethods); pMethods = NULL;
		}
		nMethCount = nMaxMethCount = 0;
		szDefaultMethod[0] = 0;
	};
	void FreeTypeList()
	{
		FreeMethods();
		if (pTypes)
		{
			free(pTypes); pTypes = NULL;
		}
		nFileTypesCount = nMaxTypesCount = 0;
	};
	BOOL AddType(const wchar_t* pszExt, FILETIME ftModified, DWORD anFlags/*FTO_xxx*/)
	{
		_ASSERTE(nMaxTypesCount > 0 && pTypes != NULL);
		if (nFileTypesCount >= nMaxTypesCount)
		{
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
		
		if (anFlags & (FTO_PROGID|FTO_USERCHOICE))
		{
			for (int i = 0; i < nFileTypesCount; i++)
			{
				if (lstrcmpW(pszExt, pTypes[i].szExt) == 0)
				{
					pTypes[nFileTypesCount].nFlags |= anFlags;
					return TRUE; // уже добавлено, обновляем только флаги
				}
			}
			
		}

		pTypes[nFileTypesCount].nMagic = FTMAGIC_TYPE;
		pTypes[nFileTypesCount].nFlags = anFlags;
		lstrcpyW(pTypes[nFileTypesCount].szExt, pszExt);
#ifndef _UNICODE
		lstrcpy_t((char*)(pTypes[nFileTypesCount].szExtO), 16, pszExt);
#endif
		pTypes[nFileTypesCount].ftModified = ftModified;
		pTypes[nFileTypesCount].szDesc[0] = 0;
		nFileTypesCount++;
		return TRUE;
	};
	BOOL AddMethod(const wchar_t* pszType, const wchar_t* pszName, wchar_t** pszExecute, FILETIME ftModified,
		FileType* pft, /*HKEY hRoot,*/ const wchar_t* pszRegPath, DWORD anFlags/*FTO_xxx*/)
	{
		_ASSERTE(nMaxMethCount > 0 && pMethods != NULL);
		_ASSERTE(pszExecute && *pszExecute);
		
		if (nMethCount >= nMaxMethCount)
		{
			int nMaxMth = nMaxMethCount + 1024;
			FileMethod* pNewMethods = (FileMethod*)malloc(nMaxMth*sizeof(FileMethod));
			if (!pNewMethods)
				return FALSE;
			memmove(pNewMethods, pMethods, sizeof(FileMethod)*nMethCount);
			free(pMethods);
			pMethods = pNewMethods; pNewMethods = NULL;
			nMaxMethCount = nMaxMth;
		}

		// Одноименные - не добавлять
		for (int i = 0; i < nMethCount; i++)
		{
			if (lstrcmpW(pMethods[i].szName, pszName) == 0)
				return TRUE;
		}

		pMethods[nMethCount].nMagic = FTMAGIC_METH;
		pMethods[nMethCount].nFlags = anFlags;
		//pMethods[nMethCount].hRegRoot = hRoot;
		if (lstrlen(pszRegPath) >= NUM_ITEMS(pMethods[nMethCount].szRegPath))
		{
			_ASSERTE(lstrlen(pszRegPath) < NUM_ITEMS(pMethods[nMethCount].szRegPath));
			pMethods[nMethCount].szRegPath[0] = 0;
		}
		else
		{
			lstrcpyW(pMethods[nMethCount].szRegPath, pszRegPath);
		}

		pMethods[nMethCount].pft = pft;
		
		_ASSERTE(lstrlenW(pszName) <= (MAX_PATH-1));
		_ASSERTE(lstrlenW(pszType) <= (MAX_PATH));
		if (szDefaultMethod[0] && lstrcmpiW(pszName, szDefaultMethod) == 0)
		{
			//pMethods[nMethCount].szName[0] = L'*';
			pMethods[nMethCount].szSort[0] = L'*'; pMethods[nMethCount].szSort[1] = 0;
			//lstrcpyW(pMethods[nMethCount].szName+1, pszName);
			//pMethods[nMethCount].pszRealName = pMethods[nMethCount].szName+1;
		}
		else
		{
			//lstrcpyW(pMethods[nMethCount].szName, pszName);
			pMethods[nMethCount].szSort[0] = 0;
			//pMethods[nMethCount].pszRealName = pMethods[nMethCount].szName;
		}
		lstrcpyW(pMethods[nMethCount].szName, pszName);
		pMethods[nMethCount].pszRealName = pMethods[nMethCount].szName;

		lstrcpyW(pMethods[nMethCount].szType, pszType);
		pMethods[nMethCount].ftModified = ftModified;

#ifdef _UNICODE
		pMethods[nMethCount].pszExecute = *pszExecute;
		*pszExecute = NULL;
#else
		lstrcpy_t(pMethods[nMethCount].szNameO, MAX_PATH+1, pMethods[nMethCount].szName);
		
		if (*pszExecute && **pszExecute)
		{
			int nLen = lstrlenW(*pszExecute) + 1;
			
			pMethods[nMethCount].pszExecuteA = (char*)malloc(nLen);
			WideCharToMultiByte(CP_ACP, 0, *pszExecute, nLen, pMethods[nMethCount].pszExecuteA, nLen, 0,0);
			
			pMethods[nMethCount].pszExecuteO = (char*)malloc(nLen+1);
			WideCharToMultiByte(CP_OEMCP, 0, *pszExecute, nLen, pMethods[nMethCount].pszExecuteO, nLen, 0,0);
			
		}
		else
		{
			pMethods[nMethCount].pszExecuteA = NULL;
			pMethods[nMethCount].pszExecuteO = NULL;
		}
#endif

		nMethCount++;
		
		return TRUE;
	};
	void LoadTypeDesc(FileType* pft)
	{
		pft->szDesc[0] = 0;
		sTemp1[0] = L'.'; lstrcpyW(sTemp1+1, pft->szExt);
		if (0==GetString(sTemp1, L"", sTemp2, MAX_PATH+1))
		{
			if (0==GetString(sTemp2, L"", sTemp3, MAX_PATH+1))
			{
				lstrcpy_t(pft->szDesc, 128, sTemp3);
			}
			else
			{
				lstrcpy_t(pft->szDesc, 128, sTemp2);
			}
		}
	};
	HANDLE LoadTypeList()
	{
		wchar_t szName[255];
		DWORD nSize, i;
		FILETIME  ftModified;
		LONG nRegRc = 0;
		LONG nMaxTypes = max(2048, nMaxTypesCount);
		
		FreeTypeList();
		
		pTypes = (FileType*)malloc(nMaxTypes*sizeof(FileType));
		if (pTypes == NULL)
		{
			_ASSERTE(pTypes!=NULL);
			goto wrap;
		}
		nMaxTypesCount = nMaxTypes;
		

		for (int t = 0; t <= 1; t++)
		{
			HKEY hRoot = HKEY_CLASSES_ROOT;
			if (t)
			{
				if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts", 0, KEY_READ, &hRoot) != 0)
				{
					continue;
				}
			}
			
			for (i = 0;; i++)
			{
				nSize = 255;
				nRegRc = RegEnumKeyExW(hRoot, i, szName, &nSize, NULL, NULL, NULL, &ftModified);
				if (nRegRc == ERROR_SUCCESS)
				{
					if (szName[0] == L'.' && szName[1] && lstrlenW(szName) < 16)
					{
						// Переведем в нижний регистр, для удобства сравнения/поиска
						int nNameLen = lstrlenW(szName);
						CharLowerBuffW(szName, nNameLen);

						DWORD nFlags = 0;
						
						// Проверить, а что там есть, собстно
						if (t)
						{
							HKEY hUserChoice = NULL;
							lstrcpyW(szName+nNameLen, L"\\UserChoice");
							if (RegOpenKeyExW(hRoot, szName, 0, KEY_READ, &hUserChoice) == 0)
							{
								nFlags = FTO_USERCHOICE;
								szName[nNameLen] = 0;
								RegCloseKey(hUserChoice);
							}
							else
							{
								lstrcpyW(szName+nNameLen, L"\\OpenWithProgids");
								if (RegOpenKeyExW(hRoot, szName, 0, KEY_READ, &hUserChoice) == 0)
								{
									nFlags = FTO_PROGID;
									szName[nNameLen] = 0;
									RegCloseKey(hUserChoice);
								}
								else
								{
									continue; // Другое пока не поддерживается
								}
							}
						}
						
						// Добавляем/обновляем
						if (!AddType(szName+1, ftModified, nFlags))
						{
							_ASSERT(FALSE);
							break;
						}
					}
				}
				else if (nRegRc == ERROR_NO_MORE_ITEMS)
				{
					break; // Ключи кончились
				}
				else if (nRegRc == ERROR_MORE_DATA)
				{
					// Слишком длинное имя ключа - не интересует
				}
				else
				{
					_ASSERT(nRegRc==0);
				}
			} // for (i = 0;; i++)
			
			if (hRoot && hRoot != HKEY_CLASSES_ROOT)
				RegCloseKey(hRoot);
		} // for (int t = 0; t <= 1; t++)

		for (i = 0; i<(DWORD)nFileTypesCount; i++)
			LoadTypeDesc(pTypes+i);
		
		
		return (HANDLE)this;
	wrap:
		delete this;
		return INVALID_HANDLE_VALUE;
	};
	
	// hkFrom - для удобства. Это всегда подключ в HKCR
	BOOL LoadMethodsFrom(HKEY hkFrom, LPCWSTR asType, DWORD anFlags)
	{
		// Показать настроенные методы запуска
		LONG nRegRc;
		HKEY hkSubst = NULL;
		wchar_t szPath[MAX_PATH*3], szName[MAX_PATH+1], szSubstName[MAX_PATH+1];

		// "HKCR\AcroExch.Document" заменить на "HKCR\AcroExch.Document.7"
		// по значению HKCR\AcroExch.Document\CurVer\(Default)"
		nRegRc = GetString(L"CurVer", L"", szSubstName, MAX_PATH, hkFrom);
		if (nRegRc == 0 && *szSubstName)
		{
			nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, szSubstName, 0, KEY_READ, &hkSubst);
			if (nRegRc == 0)
			{
				hkFrom = hkSubst;
				asType = szSubstName;
			}
		}

		//szPath[0] = L'.'; lstrcpyW(szPath+1, szDir);
		//nRegRc = GetString(szPath, L"", szType, MAX_PATH);
		//if (nRegRc != 0)
		//	return FALSE;
		//lstrcpyW(szPath, szType); lstrcatW(szPath, L"\\shell");
		
		lstrcpyW(szPath, L"shell");
		nRegRc = GetString(szPath, L"", szDefaultMethod, MAX_PATH, hkFrom);
		if (nRegRc) szDefaultMethod[0] = 0;
		
		
		HKEY hk = NULL;
		DWORD nSize, i;
		FILETIME  ftModified;
		wchar_t* pszExecute = NULL;
		
		nRegRc = RegOpenKeyExW(hkFrom, szPath, 0, KEY_READ, &hk);
		if (nRegRc)
		{
			if (hkSubst)
				RegCloseKey(hkSubst);
			return FALSE;
		}
		
		//LONG nMaxMth = 32;
		//pMethods = (FileMethod*)malloc(nMaxMth*sizeof(FileMethod));
		if (pMethods == NULL)
		{
			_ASSERTE(pMethods!=NULL);
			if (hkSubst)
				RegCloseKey(hkSubst);
			return FALSE;
		}
		//nMaxMethCount = nMaxMth;
		

		wchar_t* pszSlash = szPath + lstrlenW(szPath);
		*(pszSlash++) = L'\\';
		
		for (i = 0;; i++)
		{
			nSize = MAX_PATH-1;
			nRegRc = RegEnumKeyExW(hk, i, szName, &nSize, NULL, NULL, NULL, &ftModified);
			if (nRegRc == ERROR_SUCCESS)
			{
				if (lstrlenW(szName) >= (MAX_PATH))
					continue;
				
				lstrcpyW(pszSlash, szName); lstrcatW(pszSlash, L"\\command");
				pszExecute = GetString(szPath, L"", hkFrom);
				if (!pszExecute) pszExecute = lstrdup(L"");
					
				if (!AddMethod(asType, szName, &pszExecute, ftModified, pCurType, /*HKEY_CLASSES_ROOT,*/ szPath, anFlags))
				{
					_ASSERT(FALSE);
					if (pszExecute) { free(pszExecute); pszExecute = NULL; }
					break;
				}

			}
			else if (nRegRc == ERROR_NO_MORE_ITEMS)
			{
				break; // Ключи кончились
			}
			else if (nRegRc == ERROR_MORE_DATA)
			{
				// Слишком длинное имя ключа - не интересует
			}
			else
			{
				_ASSERT(nRegRc==0);
			}
		}
		
		if (pszExecute) { free(pszExecute); pszExecute = NULL; }
		
		if (hk)
		{
			RegCloseKey(hk); hk = NULL;
		}
		if (hkSubst)
		{
			RegCloseKey(hkSubst);
		}
		return TRUE;
	};
	BOOL LoadMethods()
	{
		// Показать настроенные методы запуска
		
		BOOL lbHKCR = FALSE;
		BOOL lbProgId = FALSE;
		BOOL lbUser = FALSE;
		
		FreeMethods();
		
		LONG nRegRc;
		HKEY hkFrom;
		wchar_t szPath[MAX_PATH*3], szType[MAX_PATH+1];
		
		LONG nMaxMth = 32;
		pMethods = (FileMethod*)malloc(nMaxMth*sizeof(FileMethod));
		if (pMethods == NULL)
		{
			_ASSERTE(pMethods!=NULL);
			return FALSE;
		}
		nMaxMethCount = nMaxMth;

		// Пользовательские настройки (приоритетные)
		lstrcpyW(szPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.");
		lstrcatW(szPath, szDir);
		int nPathSlash = lstrlenW(szPath);
		lstrcpyW(szPath+nPathSlash, L"\\UserChoice");
		nRegRc = GetString(szPath, L"Progid", szType, MAX_PATH, HKEY_CURRENT_USER);
		if (nRegRc == 0)
		{
			nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, szType, 0, KEY_READ, &hkFrom);
			if (nRegRc == 0)
			{
				lbUser = LoadMethodsFrom(hkFrom, szType, FTO_USERCHOICE);
				RegCloseKey(hkFrom);
			}
		}

		lstrcpyW(szPath+nPathSlash, L"\\OpenWithProgids");
		HKEY hkProgId = NULL;
		nRegRc = RegOpenKeyExW(HKEY_CURRENT_USER, szPath, 0, KEY_READ, &hkProgId);
		if (nRegRc == 0)
		{
			for (DWORD i = 0;; i++)
			{
				DWORD nCchNameMax = ARRAYSIZE(szType);
				nRegRc = RegEnumValueW(hkProgId, i, szType, &nCchNameMax, NULL, NULL, NULL, NULL);
				if (nRegRc != 0)
					break;
				nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, szType, 0, KEY_READ, &hkFrom);
				if (nRegRc == 0)
				{
					if (LoadMethodsFrom(hkFrom, szType, FTO_USERCHOICE))
						lbProgId = TRUE;
					RegCloseKey(hkFrom);
				}
			}
			RegCloseKey(hkProgId);
		}
		
		// HKCR (classic) - добавляем только те, которые не были добавлены в приоритетных настройках
		szPath[0] = L'.'; lstrcpyW(szPath+1, szDir);
		nRegRc = GetString(szPath, L"", szType, MAX_PATH);
		if (nRegRc == 0)
		{
			nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, szType, 0, KEY_READ, &hkFrom);
			if (nRegRc == 0)
			{
				lbHKCR = LoadMethodsFrom(hkFrom, szType, FTO_HKCR);
				RegCloseKey(hkFrom);
			}
		}

		//lbUser = LoadMethodsFrom(FTO_USERCHOICE);

		return (lbHKCR || lbProgId ||lbUser);
	}
	
	void EditMethod(FileMethod* pM)
	{

	    int height = 13, width = 60;
	    wchar_t szFullPath[MAX_PATH*3];
	    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	    CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}};
	    if (GetConsoleScreenBufferInfo(hOut, &sbi))
	    {
	    	width = sbi.dwSize.X - 8;
	    }
	    
	    int cw = width - 4;
	    int iw = cw - 2;

		wsprintfW(szFullPath, L"%s\\shell\\%s", pM->szType, pM->pszRealName);

		TCHAR szMethodName[MAX_PATH];
		lstrcpy(szMethodName, GetMsg(FTMethodDlg));
		int nLen = lstrlen(szMethodName);
		if (0==GetString(szFullPath, L"", sTemp1, MAX_PATH))
		{
			lstrcpy_t(szMethodName+nLen, MAX_PATH-nLen, sTemp1);
		}
		else
		{
			lstrcpy_t(szMethodName+nLen, MAX_PATH-nLen, pM->pszRealName);
		}


		enum
		{
			FTMDTitle = 0,
			FTMDMethodName,
			FTMDMethodLabel,
			FTMDMethodValue,
			//
			FTMDOK,
			FTMDCancel
		};

	    FarDialogItem items[] =
	    {
	        // Common options
	        {DI_DOUBLEBOX,  3,  1,  cw, height - 2, false,  0,              0,                  0,},        //FTMDTitle

			{DI_TEXT,       5,  3, 0,  0,          0,      0,              0,                  0,},        //FTMDMethodName
	        {DI_TEXT,       5,  5, 0,  0,          0,      0,              0,                  0,},        //FTMDMethodLabel
	        {DI_EDIT,       5,  6, iw, 0,          false,  NULL,           0,       0,},        //FTMDMethodValue

	        {DI_BUTTON,     0,  9, 0,  0,          true,   true,           DIF_CENTERGROUP,    true,},     //FTMDOK
	        {DI_BUTTON,     0,  9, 0,  0,          true,   false,          DIF_CENTERGROUP,    false,},    //FTMDCancel
	    };
	    
		TCHAR *pszTitle = pM->pft->szDesc;
#ifdef _UNICODE
		if (!*pszTitle) pszTitle = pM->pft->szExt;
#else
		if (!*pszTitle) pszTitle = pM->pft->szExtO;
#endif
		SETTEXT(items[FTMDTitle], pszTitle);
	    
		SETTEXT(items[FTMDMethodName], szMethodName);
	    SETTEXT(items[FTMDMethodLabel], GetMsg(FTMethodLabel));

#ifdef _UNICODE
	    SETTEXT(items[FTMDMethodValue], pM->pszExecute);
#else
		items[FTMDMethodValue].Flags |= DIF_VAREDIT;
		if (lstrlen(pM->pszExecuteO) < sizeof(sDialogExecBuf))
		{
			lstrcpy(sDialogExecBuf, pM->pszExecuteO);
			items[FTMDMethodValue].Ptr.PtrData = sDialogExecBuf;
			items[FTMDMethodValue].Ptr.PtrLength = sizeof(sDialogExecBuf);
		}
		else
		{
			items[FTMDMethodValue].Ptr.PtrData = pM->pszExecuteO;
			items[FTMDMethodValue].Ptr.PtrLength = lstrlen(pM->pszExecuteO)+1;
		}
#endif
		SETTEXT(items[FTMDOK], GetMsg(FTOK));
		SETTEXT(items[FTMDCancel], GetMsg(FTCancel));

		items[FTMDMethodValue].Focus = TRUE;
	    
	    int dialog_res = 0;
	
		#ifdef _UNICODE
		HANDLE hDlg = psi.DialogInit ( psi.ModuleNumber, -1, -1, width, height,
			GetMsg(FTMethodDlg), items, NUM_ITEMS(items), 0, 0/*Flags*/, NULL, 0/*DlgProcParam*/ );
		#endif


	    #ifndef _UNICODE
		dialog_res = psi.DialogEx(psi.ModuleNumber,-1,-1,width,height, GetMsg(FTMethodDlg), items, NUM_ITEMS(items), 0, 0,
			NULL, NULL);
	    #else
	    dialog_res = psi.DialogRun ( hDlg );
	    #endif

	    if (dialog_res != -1 && dialog_res != FTMDCancel)
	    {
			const TCHAR *pszData = GetDataPtr(FTMDMethodValue);
			if (pszData)
			{
				wsprintfW(szFullPath, L"%s\\shell\\%s\\command", pM->szType, pM->pszRealName);
				HKEY hk = NULL;
				if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szFullPath, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL))
				{
					wsprintfW(szFullPath, L"Software\\Classes\\%s\\shell\\%s\\command", pM->szType, pM->pszRealName);
					if (RegCreateKeyExW(HKEY_CURRENT_USER, szFullPath, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL))
						hk = NULL;							
				}
				if (hk)
				{
					#ifndef _UNICODE
					_ASSERTE(pszData == sDialogExecBuf || pszData == pM->pszExecuteO);
					OemToCharBuff(pszData, (TCHAR*)pszData, lstrlen(pszData));
					#endif
					RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)pszData, sizeof(TCHAR)*(lstrlen(pszData)+1));
					RegCloseKey(hk);
				}
			}

			// Перечитать реестр
			LoadMethods();
			
			// И обновить панель
			#ifdef _UNICODE
			psi.Control((HANDLE)this, FCTL_UPDATEPANEL, TRUE, 0);
			#else
			psi.Control((HANDLE)this, FCTL_UPDATEPANEL, (void*)TRUE);
			#endif
			psi.Control((HANDLE)this, FCTL_REDRAWPANEL, F757NA NULL);
	    }
	    #ifdef _UNICODE
	    psi.DialogFree ( hDlg );
	    #endif
	    
	    return;
	};
	
	void EditCurrentItem(BOOL abRegBrowser)
	{
		PanelInfo inf; memset(&inf, 0, sizeof(inf));
		INT_PTR nCurLen = 0;
		PluginPanelItem* item=NULL;
		FileMethod* pfm = NULL;
		FileType* pft = NULL;

		if (psi.Control((HANDLE)this, FCTL_GETPANELINFO, F757NA &inf))
		{
			if (inf.ItemsNumber>0 && inf.CurrentItem>0)
			{
				#ifdef _UNICODE
				nCurLen = psi.Control((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, NULL);
				item = (PluginPanelItem*)calloc(nCurLen,1);
				if (!psi.Control((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (LONG_PTR)item))
				{
					free(item);
					return;
				}
				#else
				item = &(inf.PanelItems[inf.CurrentItem]);
				#endif
				
				if (!abRegBrowser)
				{
					if ((item->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						pfm = (FileMethod*)item->UserData;
						if (pfm && pfm>=pMethods && pfm<(pMethods+nMethCount))
							EditMethod(pfm);
					}
				}
				else
				{
					// Открыть в плагине "Registry Browser"
					TCHAR szMacro[MAX_PATH*6]; szMacro[0] = 0;
					TCHAR szRegPath[MAX_PATH*3]; szRegPath[0] = 0;
					if ((item->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						pfm = (FileMethod*)item->UserData;
						if (pfm && pfm>=pMethods && pfm<(pMethods+nMethCount))
						{
							sTemp1[0] = _T('.');
							lstrcpyW(sTemp1+1, pfm->pft->szExt);
							if (0==GetString(sTemp1, L"", sTemp2, MAX_PATH))
							{
								lstrcpy_t(szRegPath, MAX_PATH, sTemp2);
								lstrcpy(szRegPath+lstrlen(szRegPath), _T("\\\\shell\\\\"));							
								lstrcpy_t(szRegPath+lstrlen(szRegPath), MAX_PATH, pfm->pszRealName);
							}
						}
					}
					else
					{
						pft = (FileType*)item->UserData;
						if (pft && pft>=pTypes && pft<(pTypes+nFileTypesCount))
						{
							szRegPath[0] = _T('.');
							lstrcpy_t(szRegPath+1, MAX_PATH, pft->szExt);
						}
					}
					if (szRegPath[0])
					{
						wsprintf(szMacro,
						#ifdef _UNICODE
							GetMsg(FTMacro2)
						#else
							GetMsg(FTMacro1)
						#endif
							, szRegPath);
					}

					if (szMacro[0])
					{
						ActlKeyMacro km = {MCMD_POSTMACROSTRING};
						km.Param.PlainText.SequenceText = szMacro;
						psi.AdvControl(psi.ModuleNumber, ACTL_KEYMACRO, &km);
					}
				}
			}
		}

#ifdef _UNICODE
		if (item)
		{
			free(item); item = NULL;
		}
#endif
	};
	
public:	
	~FTPlugin()
	{
		FreeTypeList();
	};
};

#if defined(__GNUC__)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
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
	pi->CommandPrefix = psi.GetMsg(psi.ModuleNumber, FTPluginPrefix); // "ftypes"
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

	if ((OpenFrom & 0xFF) == OPEN_COMMANDLINE && Item)
	{
		SetDirectoryW ((HANDLE)pPlugin, (LPCTSTR)Item, 0);
	}

	return pPlugin->LoadTypeList();
}

int WINAPI _export GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
	int nCount = 0, i;
	PluginPanelItem* ppi = NULL;

	if (pPlugin->szDir[0] != 0)
	{
		// Показать настроенные методы запуска
		if (!pPlugin->LoadMethods())
		{
			*pItemsNumber = 0;
			_ASSERTE(*pPanelItem == NULL);
		}
		else
		{
			nCount = pPlugin->nMethCount;
			*pItemsNumber = nCount;
			ppi = (PluginPanelItem*)calloc(nCount, sizeof(PluginPanelItem));
			*pPanelItem = ppi;

			MCHKHEAP

			for (i=0; i<nCount; i++)
			{
				ppi[i].UserData = (DWORD_PTR)(pPlugin->pMethods+i);
				ppi[i].FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
				ppi[i].FindData.ftCreationTime = pPlugin->pMethods[i].ftModified;
				ppi[i].FindData.ftLastAccessTime = pPlugin->pMethods[i].ftModified;
				ppi[i].FindData.ftLastWriteTime = pPlugin->pMethods[i].ftModified;
#ifdef _UNICODE
				ppi[i].FindData.lpwszFileName = pPlugin->pMethods[i].szName;
				ppi[i].Description = pPlugin->pMethods[i].pszExecute;
				ppi[i].FindData.nFileSize = lstrlen(pPlugin->pMethods[i].pszExecute)*2;
#else
				lstrcpy(ppi[i].FindData.cFileName, pPlugin->pMethods[i].szNameO);
				ppi[i].Description = pPlugin->pMethods[i].pszExecuteO;
				ppi[i].FindData.nFileSizeLow = lstrlen(pPlugin->pMethods[i].pszExecuteO);
#endif
				ppi[i].Owner = pPlugin->pMethods[i].szSort;
			}
		}
		
		
	}
	else
	{
		nCount = pPlugin->nFileTypesCount;
		*pItemsNumber = nCount;
		ppi = (PluginPanelItem*)calloc(nCount, sizeof(PluginPanelItem));
		*pPanelItem = ppi;

		MCHKHEAP

		for (i=0; i<nCount; i++)
		{
			ppi[i].UserData = (DWORD_PTR)(pPlugin->pTypes+i);
			ppi[i].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			ppi[i].FindData.ftCreationTime = pPlugin->pTypes[i].ftModified;
			ppi[i].FindData.ftLastAccessTime = pPlugin->pTypes[i].ftModified;
			ppi[i].FindData.ftLastWriteTime = pPlugin->pTypes[i].ftModified;

#ifdef _UNICODE
			ppi[i].FindData.lpwszFileName = pPlugin->pTypes[i].szExt;
#else
			lstrcpy(ppi[i].FindData.cFileName, pPlugin->pTypes[i].szExtO);
#endif
			ppi[i].Description = pPlugin->pTypes[i].szDesc;
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
	Info->Flags = OPIF_USEFILTER|OPIF_USESORTGROUPS|OPIF_USEHIGHLIGHTING|OPIF_ADDDOTS|OPIF_RAWSELECTION;
	MCHKHEAP

    Info->StartPanelMode = _T('0');
    Info->StartSortMode = SM_NAME;
    Info->StartSortOrder = 0;
	
	Info->PanelModesNumber = 1;
	Info->PanelModesArray = pPlugin->pPanelModes;
}

void WINAPI _export ClosePluginW(HANDLE hPlugin)
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
	if (pPlugin)
	{
		delete pPlugin;
	}
}


int WINAPI _export SetDirectoryW ( HANDLE hPlugin, LPCTSTR Dir, int OpMode )
{
	FTPlugin* pPlugin = (FTPlugin*)hPlugin;
	if (!pPlugin)
		return FALSE;

	if (Dir[0] == _T('\\'))
	{
		pPlugin->szDir[0] = 0;
		pPlugin->UpdateTitle();
		while (*Dir == _T('\\'))
			Dir++;
	}

	const TCHAR *pszSlash = _tcschr(Dir, _T('\\'));
	if (pszSlash)
	{
		TCHAR szToken[MAX_PATH];
		while (pszSlash && Dir[0])
		{
			if ((pszSlash - Dir) >= MAX_PATH)
				return FALSE;

			memcpy(szToken, Dir, (pszSlash-Dir)*sizeof(TCHAR));
			szToken[(pszSlash-Dir)] = 0;
			if (!SetDirectoryW(hPlugin, szToken, OpMode))
				return FALSE;

			Dir = pszSlash+1;
			while (*Dir == _T('\\'))
				Dir++;
			pszSlash = _tcschr(Dir, _T('\\'));
		}
	}
		
	if (Dir[0] == _T('.') && Dir[1] == _T('.') && Dir[2] == 0)
	{
		if (pPlugin->szDir[0] == 0)
			return TRUE; // и так в корне?

		// поскольку у нас только один уровень - не заморачиваемся
		pPlugin->szDir[0] = 0;
		pPlugin->UpdateTitle();

		return TRUE;
	}
	
	if (pPlugin->szDir[0])
		return FALSE; // у нас только один уровень!
	
	lstrcpy_t(pPlugin->szDir, MAX_PLUG_PATH, Dir);
	MCHKHEAP
	pPlugin->nDirLen = lstrlenW(pPlugin->szDir);
	
	MCHKHEAP
	pPlugin->UpdateTitle();
	return TRUE;
}

int WINAPI _export ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState)
{
	if (((Key & 0xFF) == VK_F4) && ControlState == 0)
	{
		FTPlugin* pPlugin = (FTPlugin*)hPlugin;
		if (pPlugin->szDir[0])
		{
			pPlugin->EditCurrentItem(FALSE);
			return TRUE;
		}
	}
	else if (((Key & 0xFF) == 'J') && ControlState == PKF_CONTROL)
	{
		FTPlugin* pPlugin = (FTPlugin*)hPlugin;
		pPlugin->EditCurrentItem(TRUE);
		return TRUE;
	}
	else if (((Key & 0xFF) == 'R') && ControlState == PKF_CONTROL)
	{
		FTPlugin* pPlugin = (FTPlugin*)hPlugin;
		if (pPlugin->szDir[0])
		{
			pPlugin->LoadMethods();
			return FALSE; // продолжить в фаре
		}
	}
	return FALSE;
}

int WINAPI CompareW(HANDLE hPlugin,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,unsigned int Mode)
{
	//if (Mode == SM_OWNER)
	{
		if (Item1->UserData && Item2->UserData)
		{
			FileMethod *p1 = (FileMethod*)Item1->UserData;
			FileMethod *p2 = (FileMethod*)Item2->UserData;
			if (p1->nMagic == FTMAGIC_METH && p2->nMagic == FTMAGIC_METH)
			{
				if (p1->szSort[0] == _T('*') && p2->szSort[0] != _T('*'))
					return -1;
				if (p1->szSort[0] != _T('*') && p2->szSort[0] == _T('*'))
					return 1;
			}
		}
	}
	//if (!_tcscmp(FILENAMEPTR(Item1->FindData), DUMP_FILE_NAME))
	//	return -1;
	//else if (!_tcscmp(FILENAMEPTR(Item2->FindData), DUMP_FILE_NAME))
	//	return 1;

	return -2; // использовать внутреннюю сортировку
}
