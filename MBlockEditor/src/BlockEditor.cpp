//TODO: Глюкавит ShiftTab

/*
Copyright (c) 2008-2011 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <windows.h>
#include <tchar.h>
#include <malloc.h>
#include "version.h"
#include "BlockEditorRes.h"

#define _FAR_NO_NAMELESS_UNIONS

#include "../../common/plugin.h"


#ifdef MDEBUG
//#include "/VCProject/MLib/MLibDef.h"
#endif

#if FAR_UNICODE>=1906
GUID guid_BlockEditor = { /* D82D6847-0C7B-4BF4-9A31-B0B929707854 */
	0xD82D6847,
	0x0C7B,
	0x4BF4,
	{0x9A, 0x31, 0xB0, 0xB9, 0x29, 0x70, 0x78, 0x54}
};

GUID guid_BlockEditorPluginMenu = { /* 186dd20d-43e9-4a39-8d4b-9e74e9c5b7a1 */                                                  
    0x186dd20d,                                                                                               
    0x43e9,                                                                                                   
    0x4a39,                                                                                                   
    {0x8d, 0x4b, 0x9e, 0x74, 0xe9, 0xc5, 0xb7, 0xa1}                                                          
};

GUID guid_Menu1 = { /* 965a8475-e260-472c-81f7-8d66a6c34e3b */
    0x965a8475,
    0xe260,
    0x472c,
    {0x81, 0xf7, 0x8d, 0x66, 0xa6, 0xc3, 0x4e, 0x3b}
};
GUID guid_Menu2 = { /* 161d063a-4bf9-498d-adab-4aec1a0892cf */
    0x161d063a,
    0x4bf9,
    0x498d,
    {0xad, 0xab, 0x4a, 0xec, 0x1a, 0x08, 0x92, 0xcf}
};

#endif

#ifndef MDEBUG
	#define MCHKHEAP
	#define _ASSERTE(x)
#else
	#include <crtdbg.h>
	#define MCHKHEAP _ASSERTE(_CrtCheckMemory())
#endif

#define InvalidOp()

HMODULE ghInstance=NULL;

#if defined(__GNUC__)
extern "C"{
  BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  void WINAPI SetStartupInfoW(struct PluginStartupInfo *Info);
  void WINAPI GetPluginInfoW( struct PluginInfo *Info );
#if FAR_UNICODE>=1906
  void WINAPI GetGlobalInfoW(struct GlobalInfo *Info);
#else
  int  WINAPI GetMinFarVersionW ();
#endif
  HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item);
};
#endif



#ifdef CRTSTARTUP
	extern "C"{
		BOOL WINAPI _DllMainCRTStartup(HANDLE hModule,DWORD dwReason,LPVOID lpReserved)
		{
		    if (ghInstance==NULL)
		        ghInstance = (HMODULE)hModule;
		    return TRUE;
		};
	};
#else
	BOOL APIENTRY DllMain(HANDLE hModule,DWORD dwReason,LPVOID lpReserved)
	{
	    if (ghInstance==NULL)
	        ghInstance = (HMODULE)hModule;
	    return TRUE;
	}
#endif

#ifndef _UNICODE
DWORD gdwFarVersion = MAKEFARVERSION(1,70,70);
#endif
PluginStartupInfo psi;

void WINAPI SetStartupInfoW(struct PluginStartupInfo *Info)
{
	memset(&psi, 0, sizeof(psi));
	memmove(&psi, Info, Info->StructSize);
  
	#ifndef _UNICODE
		gdwFarVersion = (DWORD)psi.AdvControl(psi.ModuleNumber, ACTL_GETFARVERSION, NULL);
	#endif
}

#if FAR_UNICODE>=1906
	void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
	{
		//static wchar_t szTitle[16]; _wcscpy_c(szTitle, L"ConEmu");
		//static wchar_t szDescr[64]; _wcscpy_c(szTitle, L"ConEmu support for Far Manager");
		//static wchar_t szAuthr[64]; _wcscpy_c(szTitle, L"ConEmu.Maximus5@gmail.com");
		
		Info->StructSize = sizeof(GlobalInfo);
		Info->MinFarVersion = FARMANAGERVERSION;

		// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
		Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10), VS_RELEASE);
		
		Info->Guid = guid_BlockEditor;
		Info->Title = L"MBlockEditor";
		Info->Description = L"Tabulate and comment in the Editor";
		Info->Author = L"ConEmu.Maximus5@gmail.com";
	}
	
	int WINAPI GetMinFarVersionW()
	{
		#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
		return MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD);
	}
#else
	int WINAPI GetMinFarVersionW()
	{
		#ifdef _UNICODE
			return MAKEFARVERSION(2,0,1017);
		#else
			return MAKEFARVERSION(1,70,146);
		#endif
	}
#endif



void WINAPI GetPluginInfoW( struct PluginInfo *Info )
{
    memset(Info, 0, sizeof(PluginInfo));
    
    #if FAR_UNICODE>=1906
    Info->StructSize = sizeof(struct PluginInfo);
    #endif

    MCHKHEAP;

    static TCHAR *szMenu[1];
#ifdef _UNICODE
    int nLen = lstrlenA(szMsgBlockEditorPlugin)+1;
    MCHKHEAP;
    szMenu[0]=(WCHAR*)malloc(nLen*2);
    MultiByteToWideChar(0,0,szMsgBlockEditorPlugin,nLen,szMenu[0],nLen);
    MCHKHEAP;
#else
    szMenu[0]=szMsgBlockEditorPlugin;
#endif
    MCHKHEAP;

    Info->Flags = PF_DISABLEPANELS | PF_EDITOR;

#ifdef _UNICODE
	#if FAR_UNICODE>=1906
		Info->PluginMenu.Guids = &guid_BlockEditorPluginMenu;
		Info->PluginMenu.Strings = szMenu;
		Info->PluginMenu.Count = 1;
	#else
	    Info->PluginMenuStrings = szMenu;
	    Info->PluginMenuStringsNumber = 1;
		Info->Reserved = 1296198763; //'MBlk';
	#endif
#else
    Info->PluginMenuStrings = szMenu;
    Info->PluginMenuStringsNumber = 1;
#endif
}

enum EWorkMode
{
	ewmUndefined = 0,
	ewmFirst = 1,
	ewmTabulateRight = 1,
	ewmTabulateLeft = 2,
	ewmCommentFirst = 3,
	ewmCommentAuto = ewmCommentFirst,
	ewmUncommentAuto = 4,
	ewmCommentBlock = 5,
	ewmCommentStream = 6,
	ewmCommentLast = ewmCommentStream,
	ewmLastValidCall = ewmCommentLast,
	// Use Internally
	ewmUncommentBlock = 7,
	ewmUncommentStream = 8,
	ewmLastInternal = ewmUncommentStream
};

#if FAR_UNICODE>=1906
HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	return OpenPluginW(Info->OpenFrom, Info->Data);
}
#endif

INT_PTR EditCtrl(int Cmd, void* Parm, INT_PTR cbSize = 0)
{
	INT_PTR iRc;
	#if FAR_UNICODE>=1906
	iRc = psi.EditorControl(-1, (EDITOR_CONTROL_COMMANDS)Cmd, cbSize, Parm);
	#else
	iRc = psi.EditorControl(Cmd, Parm);
	#endif
	return iRc;
}

struct FarSetHandle
{
	HANDLE hKey;
	#if defined(_UNICODE) && (FAR_UNICODE>=1906)
	INT_PTR nSubKey;
	#endif
};

FarSetHandle OpenKey(LPCTSTR pszSubKey)
{
	FarSetHandle Key = {NULL};
	#if FAR_UNICODE>=3000

		FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_BlockEditor, INVALID_HANDLE_VALUE};
		FarSettingsItem fsi = {sizeof(fsi)};
		bool lbKeyOpened = psi.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) != 0;
		if (lbKeyOpened)
		{
			Key.hKey = sc.Handle;
			if (pszSubKey && *pszSubKey)
			{
				FarSettingsValue fsv = {sizeof(fsv), 0, pszSubKey};
				Key.nSubKey = psi.SettingsControl(sc.Handle, SCTL_OPENSUBKEY, 0, &fsv) != 0;
				if (!Key.nSubKey)
				{
					psi.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
					Key.hKey = NULL;
				}
			}
		}

	#elif FAR_UNICODE>=1906

		FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_BlockEditor, INVALID_HANDLE_VALUE};
		FarSettingsItem fsi = {0};
		bool lbKeyOpened = psi.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) != 0;
		if (lbKeyOpened)
		{
			Key.hKey = sc.Handle;
			if (pszSubKey && *pszSubKey)
			{
				FarSettingsValue fsv = {0, pszSubKey};
				Key.nSubKey = psi.SettingsControl(sc.Handle, SCTL_OPENSUBKEY, 0, &fsv) != 0;
				if (!Key.nSubKey)
				{
					psi.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
					Key.hKey = NULL;
				}
			}
		}

	#else
	
		TCHAR* pszKey = NULL;
		int nLen = (pszSubKey ? lstrlen(pszSubKey) : 0) + 2 + lstrlen(psi.RootKey);
		pszKey = (TCHAR*)calloc(nLen, sizeof(TCHAR));
		lstrcpy(pszKey, psi.RootKey); 
		lstrcat(pszKey, _T("\\"));
		lstrcat(pszKey, pszSubKey);
		
		if (RegOpenKeyEx(HKEY_CURRENT_USER, pszKey, 0, KEY_READ, (HKEY*)&Key.hKey) != 0)
			Key.hKey = NULL;
			
		free(pszKey);
		
	#endif
	return Key;
}

void CloseKey(FarSetHandle& Key)
{
	#if FAR_UNICODE>=1906
		psi.SettingsControl(Key.hKey, SCTL_FREE, 0, 0);
	#else
		if (Key.hKey)
			RegCloseKey((HKEY)Key.hKey);
	#endif
	memset(&Key, 0, sizeof(Key));
}

BOOL QueryValue(const FarSetHandle& Key, LPCTSTR pszName, LPTSTR pszValue, int cchValueMax)
{
	BOOL lbRc = FALSE;
	#if FAR_UNICODE>=3000
		FarSettingsItem fsi = {sizeof(fsi), Key.nSubKey};
		fsi.Name = pszName;
		fsi.Type = FST_STRING;
		if (psi.SettingsControl(Key.hKey, SCTL_GET, 0, &fsi))
		{
			lstrcpynW(pszValue, fsi.String, cchValueMax-1);
			lbRc = TRUE;
		}
		else
		{
			*pszValue = 0; *(pszValue+1) = 0;
		}
	#elif FAR_UNICODE>=1906
		FarSettingsItem fsi = {Key.nSubKey};
		fsi.Name = pszName;
		fsi.Type = FST_STRING;
		if (psi.SettingsControl(Key.hKey, SCTL_GET, 0, &fsi))
		{
			lstrcpynW(pszValue, fsi.String, cchValueMax-1);
			lbRc = TRUE;
		}
		else
		{
			*pszValue = 0; *(pszValue+1) = 0;
		}
	#else
		DWORD dwSize = cchValueMax*sizeof(*pszValue);
		DWORD dwResult = dwSize-2*sizeof(TCHAR);
		if ((RegQueryValueEx((HKEY)Key.hKey, pszName, NULL, NULL, (LPBYTE)pszValue, &dwResult) == 0)
			&& dwResult < dwSize) // ASCIIZ
		{
			lbRc = TRUE;
		}
		else
		{
			*pszValue = 0; *(pszValue+1) = 0;
		}
	#endif
	if (lbRc)
	{
		TCHAR* psz = _tcschr(pszValue, L'\n');
		while (psz)
		{
			*psz = 0;
			psz = _tcschr(psz+1, L'\n');
		}
	}
	return lbRc;
}

void SetMenuItem(FarMenuItem* pItems, int nIdx, LPCTSTR pszBegin, LPCTSTR pszEnd)
{
	#ifdef _UNICODE
	wchar_t* psz = (wchar_t*)calloc(64,sizeof(wchar_t));
	pItems[nIdx].Text = psz;
	#else
	char* psz = pItems[nIdx].Text;
	#endif
	
	#define MENU_PART 4
	
	if (nIdx < 9)
		wsprintf(psz, _T("&%i. "), nIdx+1);
	else if (nIdx == 9)
		lstrcpy(psz, _T("&0. "));
	else
		lstrcpy(psz, _T("   "));
	
	int iStart = lstrlen(psz);
	int i = iStart;
	lstrcpyn(psz+i, pszBegin, MENU_PART+1 /*+1 т.к. включая \0*/);
	i = lstrlen(psz);
	int iFin = iStart+MENU_PART+1;
	while (i < iFin)
		psz[i++] = _T(' ');
	psz[i] = 0;
	if (pszEnd)
		lstrcpyn(psz+i, pszEnd, MENU_PART+1 /*+1 т.к. включая \0*/);
	i = lstrlen(psz);
	iFin = iStart+MENU_PART*2+2;
	while (i < iFin)
		psz[i++] = _T(' ');
	psz[i] = 0;
	lstrcat(psz, pszEnd ? _T("Stream") : _T("Block"));
}

/* global variables */
EditorGetString egs;
EditorInfo ei;
EditorSetString ess;
EditorSelect es;
int nMode;
BOOL lbSkipNonSpace, lbSkipCommentMenu;
LPCTSTR psComment, psCommentBegin, psCommentEnd;
int nCommentBeginLen, nCommentEndLen;
BOOL lbExpandTabs;
int nStartLine;
int nEndLine;
int Y1, Y2;
int X1, X2;
bool lbCurLineShifted;
int nInsertSlash;
int nMaxStrLen;
bool lbStartChanged, lbEndChanged; // измнены ли первая и последняя строки выделения?
bool lbMultiComment;
/* end of global variables */

void FindMaxStringLen()
{
    for (egs.StringNumber = nStartLine;
         egs.StringNumber < ei.TotalLines;
         egs.StringNumber++)
    {
        EditCtrl(ECTL_GETSTRING,&egs);

        MCHKHEAP;

        if (egs.StringNumber==nStartLine)
		{
            X1 = egs.SelStart;
        }

        if (egs.SelEnd!=-1 && X2==-1)
		{
            X2 = egs.SelEnd;
            Y2 = nEndLine;
        }

        MCHKHEAP;

        if (egs.StringLength>nMaxStrLen)
            nMaxStrLen = egs.StringLength;

        if ((ei.BlockType != BTYPE_NONE) && (egs.SelStart==-1))
		{
            break;
        }
		else if ((ei.BlockType == BTYPE_NONE) || 
            (egs.SelStart>0) || (egs.SelStart==0 && (egs.SelEnd==-1 || egs.SelEnd>egs.SelStart))) 
        {
            MCHKHEAP;
            nEndLine = egs.StringNumber;
            //if (nEmptyLine!=-1) nEmptyLine=-1;
            if (nMode == ewmCommentBlock || nMode == ewmCommentAuto)
			{
                int nPos = 0, nIdx=0; BOOL lbSpace=FALSE;
                while ((lbSpace=(egs.StringText[nIdx]==_T(' '))) || egs.StringText[nIdx]==_T('\t'))
				{
                    nIdx++;
                    if (lbSpace)
                        nPos++;
                    else
                        nPos += ei.TabSize;
                }
                if (nPos || (nInsertSlash==-1))
				{
                    if (nInsertSlash==-1)
                        nInsertSlash = nPos;
                    else if (nInsertSlash>nPos)
                        nInsertSlash = nPos;
                }
            }
            MCHKHEAP;

            if (ei.BlockType == BTYPE_NONE)
                break;

        }
		else
		{
            //nEmptyLine = egs.StringNumber;
            break;
        }
    }
    //if (Y2==-1)
    Y2 = nEndLine;
    if (X2==0) Y2++;

	// Скорректировать X1/X2 если они выходят ЗА пределы строки
	if (X1 > 0)
	{
		egs.StringNumber = Y1;
		if (EditCtrl(ECTL_GETSTRING,&egs) && X1 > egs.StringLength)
			X1 = egs.StringLength;
	}
	if (X2 > 0)
	{
		egs.StringNumber = Y2;
		if (EditCtrl(ECTL_GETSTRING,&egs) && X2 > egs.StringLength)
			X2 = egs.StringLength;
	}
}

BOOL PrepareCommentParams()
{
	if (nMode == ewmCommentAuto)
	{
		if (psCommentBegin && *psCommentBegin
			&& psCommentEnd && *psCommentEnd)
		{
			if ((ei.BlockType != BTYPE_NONE) && (!psComment || !*psComment))
			{
				nMode = ewmCommentStream;
			}
			else if (((X1 >= 0 && X2 >= 0) && (X1 || X2)))
			{
				// Если есть из чего выбирать - проверить, а не захватывает ли X2 всю строку?
				if (!X1 && X2 && psComment && *psComment)
				{
					egs.StringNumber = Y2;
					if (EditCtrl(ECTL_GETSTRING,&egs)
						&& egs.StringLength == X2)
					{
						// Раз захватывает всю строку целиком - выбираем "блочное" комментирование
						nMode = ewmCommentBlock;
					}
				}
				if (nMode == ewmCommentAuto)
					nMode = ewmCommentStream;
			}
			if ((nMode == ewmCommentAuto) && (ei.BlockType == BTYPE_NONE)
				&& (!psComment || !*psComment))
			{
				egs.StringNumber = ei.CurLine;
				if (EditCtrl(ECTL_GETSTRING,&egs) && egs.StringLength)
				{
					X1 = 0;
					X2 = egs.StringLength;
					if (lbSkipNonSpace)
					{
						while (X1 < X2 && X1 < ei.CurPos &&
							(egs.StringText[X1] == _T(' ') || egs.StringText[X1] == _T('\t')))
							X1++;
					}
					Y1 = Y2 = ei.CurLine;
					nMode = ewmCommentStream;
					ei.BlockType = BTYPE_STREAM;
				}
			}
		}
		
		if (nMode != ewmCommentStream)
			nMode = ewmCommentBlock;
	}
	if (nMode == ewmCommentStream)
	{
		if (!psCommentBegin || !psCommentEnd)
		{
			nMode = ewmCommentBlock;
		}
		else
		{
			nCommentBeginLen = lstrlen(psCommentBegin);
			nCommentEndLen = lstrlen(psCommentEnd);
		}
	}
	if (nMode == ewmCommentBlock)
	{
		nCommentBeginLen = nCommentEndLen = lstrlen(psComment);
	}

	if ((nMode == ewmCommentBlock) || (nMode == ewmCommentStream))
	{
		if (nCommentBeginLen<1)
			return FALSE;
		// Проверка, может быть настроено несколько допустимых комментариев?
		if (nMode == ewmCommentBlock)
			lbMultiComment = (psComment[nCommentBeginLen+1] != 0);
		else
			lbMultiComment = (psCommentBegin[nCommentBeginLen+1] != 0);
		// Дать пользователю выбрать, чем комментировать
		if (lbMultiComment && !lbSkipCommentMenu)
		{
			// Выбирать будем и блочные и потоковые
			int nCount = 0;
			LPCTSTR psz = psComment;
			while (psz && *psz)
			{
				nCount ++;
				psz += lstrlen(psz)+1;
			}
			psz = psCommentBegin;
			while (psz && *psz)
			{
				nCount ++;
				psz += lstrlen(psz)+1;
			}
			if (!nCount)
				return FALSE;
			// Сформировать меню
			FarMenuItem* pItems = (FarMenuItem*)calloc(nCount, sizeof(FarMenuItem));
			//#if FAR_UNICODE>=1906
			//FarMenuItem pItems[] =
			//{
			//	{0, L"&1 Tabulate right"},
			//};
			//#else
			//FarMenuItem pItems[] =
			//{
			//	{_T("&1 Tabulate right")},
			//};
			//#endif
			int nBlockBegin = -1;
			int nStreamBegin = -1;
			nCount = 0;
			FarMenuItem* pCur = NULL;
			if (psComment && *psComment)
			{
				psz = psComment;
				nBlockBegin = nCount;
				while (*psz)
				{
					if (!pCur && (nMode == ewmCommentBlock))
						pCur = pItems+nCount;
					SetMenuItem(pItems, nCount, psz, NULL);
					nCount ++;
					psz += lstrlen(psz)+1;
				}
			}
			if (psCommentBegin && *psCommentBegin)
			{
				psz = psCommentBegin;
				LPCTSTR pszEnd = psCommentEnd;
				nStreamBegin = nCount;
				while (*psz)
				{
					if (!pCur && (nMode == ewmCommentStream))
						pCur = pItems+nCount;
					SetMenuItem(pItems, nCount, psz, pszEnd);
					nCount ++;
					psz += lstrlen(psz)+1;
					if (*pszEnd)
						pszEnd += lstrlen(pszEnd)+1;
				}
			}
			if (pCur)
			{
				#if FAR_UNICODE>=1900
				pCur->Flags |= MIF_SELECTED;
				#else
				pCur->Selected = TRUE;
				#endif
			}

			int nSel = -1;
			#if FAR_UNICODE>=1900
			nSel = psi.Menu(&guid_BlockEditor, &guid_Menu1,
				-1,-1, 0, /*FMENU_CHANGECONSOLETITLE|*/FMENU_WRAPMODE,
				_T(szMsgBlockEditorPlugin), NULL, NULL,
				NULL, NULL, pItems, nCount);
			#else
			nSel = psi.Menu(psi.ModuleNumber,
				-1, -1, 0, FMENU_WRAPMODE,
				_T(szMsgBlockEditorPlugin),NULL,NULL,
				0, NULL, pItems, nCount);
			#endif
			#ifdef _UNICODE
			for (int i = 0; i < nCount; i++)
			{
				if (pItems[i].Text)
					free((void*)pItems[i].Text);
			}
			#endif
			free(pItems);
			if (nSel < 0)
				return FALSE;
			// Смотрим, что юзер выбрал
			if (nStreamBegin != -1 && nSel >= nStreamBegin)
			{
				nMode = ewmCommentStream;
				nSel -= nStreamBegin;
				// Указатель на выбранный комментарий
				while ((nSel--) > 0)
				{
					if (*psCommentBegin)
						psCommentBegin += lstrlen(psCommentBegin)+1;
					if (*psCommentEnd)
						psCommentEnd += lstrlen(psCommentEnd)+1;
				}
				nCommentBeginLen = lstrlen(psCommentBegin);
				nCommentEndLen = lstrlen(psCommentEnd);
			}
			else if (nBlockBegin != -1 && nSel >= nBlockBegin)
			{
				nMode = ewmCommentBlock;
				nSel -= nBlockBegin;
				// Указатель на выбранный комментарий
				while ((nSel--) > 0)
				{
					if (*psComment)
						psComment += lstrlen(psComment)+1;
				}
				nCommentBeginLen = nCommentEndLen = lstrlen(psComment);
			}
			else
			{
				InvalidOp();
				return FALSE;
			}
			if (!nCommentBeginLen)
				return FALSE;
		}
	}
	return TRUE;
}

BOOL DoTabRight()
{
	// Поехали
    TCHAR* lsText = (TCHAR*)calloc(nMaxStrLen,sizeof(TCHAR));
    if (!lsText) return FALSE;
    MCHKHEAP;
    for (egs.StringNumber = nStartLine;
         egs.StringNumber <= nEndLine;
         egs.StringNumber++)
    {
        MCHKHEAP;
        EditCtrl(ECTL_GETSTRING,&egs);
        
        //if (nMode == ewmTabulateRight)
		{
            MCHKHEAP;
            if (*egs.StringText==0) continue;
            if (lbExpandTabs)
			{
                //lsText.Fill(_T(' '), ei.TabSize);
                for(int i=0; i<ei.TabSize; i++) lsText[i]=_T(' ');
                lsText[ei.TabSize]=0;
            }
			else
			{
                //lsText = _T("\t");
                lsText[0]=_T('\t'); lsText[1]=0;
            }
            MCHKHEAP;
            //lsText += egs.StringText;
            lstrcat(lsText, egs.StringText);
            egs.StringText = lsText;

			if (egs.StringNumber == ei.CurLine)
				lbCurLineShifted = true;

        }

        MCHKHEAP;
		ess.StringLength = lstrlen(egs.StringText);
		ess.StringText = (TCHAR*)egs.StringText;
		ess.StringNumber = egs.StringNumber;
		ess.StringEOL = (TCHAR*)egs.StringEOL;
		EditCtrl(ECTL_SETSTRING,&ess);
        MCHKHEAP;
    }
    free(lsText);
    return TRUE;
}

BOOL DoTabLeft()
{
	// Поехали
    TCHAR* lsText = (TCHAR*)calloc(nMaxStrLen,sizeof(TCHAR));
    if (!lsText) return FALSE;
    MCHKHEAP;
    for (egs.StringNumber = nStartLine;
         egs.StringNumber <= nEndLine;
         egs.StringNumber++)
    {
        MCHKHEAP;
        EditCtrl(ECTL_GETSTRING,&egs);
        
		
		//if (nMode == ewmTabulateLeft)
		{
            if (*egs.StringText==0) continue;
            MCHKHEAP;
            if (*egs.StringText==_T('\t'))
			{
                egs.StringText++;
                if (egs.StringNumber==nStartLine)
                    if (X1) X1--;
                if (egs.StringNumber==nEndLine)
                    if (X2) X2--;
                MCHKHEAP;
				if (egs.StringNumber == ei.CurLine)
					lbCurLineShifted = true;
            }
			else if (*egs.StringText==_T(' '))
			{
                int nSpaces = 0;
                while (nSpaces<ei.TabSize && *egs.StringText==_T(' '))
				{
                    egs.StringText++; nSpaces++;
                }
                MCHKHEAP;
                if (egs.StringNumber==nStartLine)
                    X1 = (X1>nSpaces) ? (X1-nSpaces) : 0;
                if (egs.StringNumber==nEndLine)
                    X2 = (X2>nSpaces) ? (X2-nSpaces) : 0;
				if (egs.StringNumber == ei.CurLine)
					lbCurLineShifted = true;
            }
            //lstrcpy(lsText, egs.StringText);

        }

        MCHKHEAP;
		ess.StringLength = lstrlen(egs.StringText);
		ess.StringText = (TCHAR*)egs.StringText;
		ess.StringNumber = egs.StringNumber;
		ess.StringEOL = (TCHAR*)egs.StringEOL;
		EditCtrl(ECTL_SETSTRING,&ess);
        MCHKHEAP;
    }
    free(lsText);
    return TRUE;
}

void SetCommentDefaults(LPCTSTR psExt)
{
    if (_tcsicmp(psExt, _T(".bat"))==0 || _tcsicmp(psExt, _T(".cmd"))==0)
	{
        psComment = _T("rem \0REM \0");
        lbSkipNonSpace = FALSE;
		lbSkipCommentMenu = TRUE;
    }
	else if (_tcsicmp(psExt, _T(".sql"))==0)
	{
        psComment = _T("--");
		psCommentBegin = _T("/*");
		psCommentEnd = _T("*/");
		lbSkipNonSpace = FALSE;
    }
	else if (_tcsicmp(psExt, _T(".bas"))==0 || _tcsicmp(psExt, _T(".vbs"))==0)
	{
        psComment = _T("'");
	}
	else if (_tcsicmp(psExt, _T(".cpp"))==0 || _tcsicmp(psExt, _T(".c"))==0 || _tcsicmp(psExt, _T(".cxx"))==0 ||
		_tcsicmp(psExt, _T(".hpp"))==0 || _tcsicmp(psExt, _T(".h"))==0 || _tcsicmp(psExt, _T(".hxx"))==0)
	{
		psCommentBegin = _T("/*");
		psCommentEnd = _T("*/");
	}
	else if (_tcsicmp(psExt, _T(".htm"))==0 || _tcsicmp(psExt, _T(".html"))==0 || _tcsicmp(psExt, _T(".xml"))==0)
	{
		psCommentBegin = _T("<!--");
		psCommentEnd = _T("-->");
		psComment = _T("");
	}
	else if (_tcsicmp(psExt, _T(".php"))==0)
	{
		psCommentBegin = _T("<!--\0/*\0");
		psCommentEnd = _T("-->\0*/\0");
		psComment = _T("//\0#\0");
	}
	else if (_tcsicmp(psExt, _T(".ps1"))==0 || _tcsicmp(psExt, _T(".psm1"))==0)
	{
		psCommentBegin = _T("<#");
		psCommentEnd = _T("#>");
		psComment = _T("#");
	}
	else if (_tcsicmp(psExt, _T(".lua"))==0 || _tcsicmp(psExt, _T(".psm1"))==0)
	{
		psComment = _T("--");
	}
}

void LoadCommentSettings(LPCTSTR pszFileName, TCHAR (&szComment)[100], TCHAR (&szCommentBegin)[100], TCHAR (&szCommentEnd)[100])
{
	// CommentFromBegin
	FarSetHandle hk = {NULL};
	TCHAR szTemp[16] = {0};
	TCHAR szKey[MAX_PATH];
	//lstrcpy(szKey, psi.RootKey); 
	//if (lstrlen(szKey) < (MAX_PATH - 15))
	//{
	#if defined(_UNICODE) && FARMANAGERVERSION_BUILD>=2460
	*szKey = 0;
	#else
	lstrcpy(szKey, _T("MBlockEditor"));
	#endif

	//[HKEY_CURRENT_USER\Software\Far2\Plugins\MBlockEditor]
	//;; value "off" means 'comments are inserted at first non-space symbol'
	//;; value "on"  means '... inserted at the line beginning'
	//"CommentFromBegin"="off"
	hk = OpenKey(szKey);
	if (hk.hKey != NULL)
	{
		//DWORD dwSize;
		if (QueryValue(hk, _T("CommentFromBegin"), szTemp, ARRAYSIZE(szTemp)))
		{
			if (*szTemp)
				lbSkipNonSpace = lstrcmpi(szTemp, _T("on")) != 0;
		}
		CloseKey(hk);
	}
	//}

    LPCTSTR psExt = _tcsrchr(pszFileName, _T('.'));
    if (psExt)
	{
		if ((lstrlen(szKey) + 2 + lstrlen(psExt)) < MAX_PATH)
		{
			#if defined(_UNICODE) && FARMANAGERVERSION_BUILD>=2460
			*szKey = 0;
			#else
			lstrcat(szKey, _T("\\"));
			#endif
			lstrcat(szKey, psExt);
			
			hk = OpenKey(szKey);
			if (hk.hKey != NULL)
			{
				//DWORD dwSize, dwSize2;
				// May be overrided for each extension
				if (QueryValue(hk, _T("CommentFromBegin"), szTemp, ARRAYSIZE(szTemp)))
				{
					if (*szTemp)
					{
						lbSkipNonSpace = lstrcmpi(szTemp, _T("on")) != 0;
					}
				}
				// Для "множественного" комментирования может быть запрещен показ меню
				if (QueryValue(hk, _T("CommentSkipMenu"), szTemp, ARRAYSIZE(szTemp)))
				{
					if (*szTemp)
					{
						lbSkipCommentMenu = lstrcmpi(szTemp, _T("on")) == 0;
					}
				}
				// Настройка "блочного" комментирования (на каждой строке)
				if (QueryValue(hk, _T("Comment"), szComment, ARRAYSIZE(szComment)))
				{
					// Чтобы можно было запретить "блочный" комментарий (для html например)
					// нужно указать "Comment"=""
					//if (*szComment)
					//{
					psComment = szComment;
					// Имеет приоритет над встроенным в программу
					psExt = NULL;
					//}
				}
				// Комментирование может быть и "потоковое" (начало и конец выделения)
				if (QueryValue(hk, _T("CommentBegin"), szCommentBegin, ARRAYSIZE(szCommentBegin)) &&
					QueryValue(hk, _T("CommentEnd"), szCommentEnd, ARRAYSIZE(szCommentEnd)))
				{
					if (*szCommentBegin && *szCommentEnd)
					{
						psCommentBegin = szCommentBegin;
						psCommentEnd = szCommentEnd;
						// Имеет приоритет над встроенным в программу
						psExt = NULL;
					}
				}
				CloseKey(hk);
			}
		}
	}			

	// Если НЕ NULL - значит в настройках плагина ничего прописано не было
	if (psExt)
		SetCommentDefaults(psExt);
}

BOOL DoComment()
{
	// Поехали
    TCHAR* lsText = (TCHAR*)calloc(nMaxStrLen,sizeof(TCHAR));
    if (!lsText) return FALSE;
    MCHKHEAP;
    for (egs.StringNumber = nStartLine;
         egs.StringNumber <= nEndLine;
         egs.StringNumber++)
    {
        MCHKHEAP;
        EditCtrl(ECTL_GETSTRING,&egs);
        
		//if (nMode == ewmCommentBlock || nMode == ewmCommentStream)
		{
            MCHKHEAP;
            int nPos = 0, nIdx = 0; BOOL lbSpace=FALSE;
            while (lbSkipNonSpace
            	   && ((lbSpace=(egs.StringText[nIdx]==_T(' '))) || egs.StringText[nIdx]==_T('\t')))
            {
                if (nPos >= nInsertSlash)
					break;
                nIdx++;
                if (lbSpace)
                    nPos++;
                else
                    nPos += ei.TabSize;
            }
			// Чтобы после комментирования курсор встал на первый символ ПОСЛЕ комментария (//)
			if ((ei.CurPos < nIdx) && (ei.BlockType == BTYPE_NONE) /*&& (nMode == ewmCommentBlock)*/)
			{
				ei.CurPos = nIdx;
			}

            MCHKHEAP;

			if (nMode == ewmCommentBlock ||
				(nMode == ewmCommentStream
				 && ((egs.StringNumber == nStartLine) || (egs.StringNumber == nEndLine))))
			{
				LPCTSTR psCommCurr = NULL;
				if (nMode == ewmCommentBlock)
					psCommCurr = psComment;
				else
					psCommCurr = (egs.StringNumber == nStartLine) ? psCommentBegin : psCommentEnd;

				//// nIdx - пропуск пробельных символов
				//if (egs.StringText[nIdx])
				//{
					// Если НЕ конец строки
					//lsText = egs.StringText;
					//lsText.Insert(nIdx,_T("//"));

				lsText[0] = 0;
				MCHKHEAP;
				if (nMode == ewmCommentBlock)
				{
					// сначала скопировать пробельные символы
					if (nIdx)
						memcpy(lsText, egs.StringText, nIdx*sizeof(TCHAR));
					MCHKHEAP;
					lstrcpy(lsText+nIdx, psCommCurr);
					MCHKHEAP;
					lstrcat(lsText, egs.StringText+nIdx);
					//}
					//else
					//{
					//	lstrcpy(lsText, egs.StringText);
					//	lstrcat(lsText, psCommCurr);
					//}
				}
				else if (nMode == ewmCommentStream)
				{
					size_t n = 0;
					int nStartLnX1 = X1 ? X1 : nIdx;
					if (egs.StringNumber == nStartLine)
					{
						if (nStartLnX1)
						{
							memcpy(lsText+n, egs.StringText, nStartLnX1*sizeof(TCHAR));
							n += nStartLnX1;
						}
						memcpy(lsText+n, psCommentBegin, nCommentBeginLen*sizeof(TCHAR));
						n += nCommentBeginLen;
						lsText[n] = 0;
					}
					if (egs.StringNumber == nEndLine)
					{
						if (egs.StringNumber == nStartLine)
						{
							if (Y1 == Y2)
							{
								if (X2 <= nStartLnX1)
								{
									InvalidOp();
									break;
								}
								memcpy(lsText+n, egs.StringText+nStartLnX1, (X2 - nStartLnX1)*sizeof(TCHAR));
								n += (X2 - nStartLnX1);
							}
							else
							{
								lstrcpy(lsText+n, egs.StringText+nStartLnX1);
								n += lstrlen(egs.StringText+nStartLnX1);
							}
						}
						else
						{
							if ((Y2 == nEndLine) && (X2 > 0))
							{
								memcpy(lsText+n, egs.StringText, X2*sizeof(TCHAR));
								n += X2;
							}
							else
							{
								lstrcpy(lsText+n, egs.StringText);
								n += lstrlen(egs.StringText);
							}
						}
						memcpy(lsText+n, psCommentEnd, nCommentEndLen*sizeof(TCHAR));
						n += nCommentEndLen;
						lsText[n] = 0;
						if ((Y2 == nEndLine) && egs.StringText[X2])
							lstrcpy(lsText+n, egs.StringText+X2);
					}
					else
					{
						if (egs.StringNumber == nStartLine)
							lstrcpy(lsText+n, egs.StringText+nStartLnX1);
						else
							lstrcpy(lsText+n, egs.StringText);
					}
					
				}
				MCHKHEAP;
				//}
				//else
				//{
				//	//nPos = lsText.Find(_T("//"));
				//	//if(nPos==-1) nPos=0; else nPos+=2;
				//	//lsText.SetAt(nPos,0);

				//	TCHAR* psComm = _tcsstr(lsText, psCommCurr);
				//	if (psComm==NULL) psComm=lsText; else psComm+=lstrlen(psCommCurr);
				//	MCHKHEAP;
				//	*psComm =0;
				//	MCHKHEAP;
				//}
				egs.StringText = lsText;

				// Коррекция будущего выделения
				if (egs.StringNumber == nStartLine && nMode != ewmCommentStream)
				{
					if ((X1 > nIdx) && (X1 || Y1==Y2))
						X1 += nCommentBeginLen;
				}
				if (egs.StringNumber == nEndLine) // БЕЗ else - может быть однострочное выделение
				{
					if (X2 || Y1==Y2)
						X2 += nCommentEndLen;
					if (nMode == ewmCommentStream && Y1==Y2)
						X2 += nCommentBeginLen;
				}
				if (egs.StringNumber == ei.CurLine)
					lbCurLineShifted = true;
			}
        }

        MCHKHEAP;
		ess.StringLength = lstrlen(egs.StringText);
		ess.StringText = (TCHAR*)egs.StringText;
		ess.StringNumber = egs.StringNumber;
		ess.StringEOL = (TCHAR*)egs.StringEOL;
		EditCtrl(ECTL_SETSTRING,&ess);
        MCHKHEAP;
    }
    free(lsText);
    return TRUE;
}

BOOL DoUnComment()
{
	// Поехали
    TCHAR* lsText = (TCHAR*)calloc(nMaxStrLen,sizeof(TCHAR));
    if (!lsText) return FALSE;
	bool lbFirstUncommented = false;
	LPCTSTR pszBlockComment = psComment; // сохранить, для перебора

	//lbMultiComment - Может быть настроено несколько типов комментариев
	if (nMode != ewmUncommentAuto)
	{
		if (nMode == ewmCommentBlock)
			lbMultiComment = (psComment[lstrlen(psComment)+1] != 0);
		else
			lbMultiComment = (psCommentBegin[lstrlen(psCommentBegin)+1] != 0);
	}

    MCHKHEAP;
    for (egs.StringNumber = nStartLine;
         egs.StringNumber <= nEndLine;
         egs.StringNumber++)
    {
        MCHKHEAP;
        if (!EditCtrl(ECTL_GETSTRING,&egs))
			break;
		MCHKHEAP;

		// При снятии потокового комментария, если весь комментарий не был выделен,
		// следующая строка может оказаться длиннее текущей
		if (egs.StringLength > nMaxStrLen)
		{
			nMaxStrLen = egs.StringLength;
			free(lsText);
			lsText = (TCHAR*)calloc(nMaxStrLen,sizeof(TCHAR));
			if (!lsText) return FALSE;
		}
        
		//if (nMode == ewmUncommentAuto || nMode == ewmUncommentBlock || nMode == ewmUncommentStream)
		{
			// Убрать комментарий
            if (*egs.StringText==0)
			{
				if (egs.StringNumber == nStartLine)
					lbStartChanged = false;
				else if (egs.StringNumber == nEndLine)
					lbEndChanged = false;
				continue;
			}
            //lsText = egs.StringText;
            //int nSlash = lsText.Find(_T("//"));
            //if (nSlash>=0) lsText.Delete(nSlash,2);

			if (nMode == ewmUncommentAuto)
			{
				if (psComment && *psComment && !psCommentBegin)
				{
					nMode = ewmUncommentBlock;
					lbMultiComment = (psComment[lstrlen(psComment)+1] != 0);
				}
				else if (psCommentBegin && psCommentEnd && !(psComment && *psComment))
				{
					nMode = ewmUncommentStream;
					lbMultiComment = (psCommentBegin[lstrlen(psCommentBegin)+1] != 0);
				}
				else
				{
					// Нужно опеределить, какой комментарий используется в блоке :(
					LPCTSTR pszTest;
					LPCTSTR pszBegin = psCommentBegin, pszEnd = psCommentEnd;
					// Сравнить с началом блока
					if (nMode == ewmUncommentAuto)
					{
						pszTest = egs.StringText;
						if (egs.StringNumber == nStartLine && X1 >= 0)
							pszTest += X1;
						while (*pszTest == _T(' ') || *pszTest == _T('\t'))
							pszTest++;
						pszBegin = psCommentBegin; pszEnd = psCommentEnd;
						while (*pszBegin && nMode != ewmUncommentStream)
						{
							int nBegin = lstrlen(pszBegin);
							if (_tcsncmp(pszTest, pszBegin, nBegin) == 0)
							{
								nMode = ewmUncommentStream;
								break;
							}
							// Следующий
							pszBegin += lstrlen(pszBegin)+1;
							if (*pszEnd)
								pszEnd += lstrlen(pszEnd)+1;
						}
					}
					if (nMode == ewmUncommentAuto)
					{
						pszTest = egs.StringText;
						if (egs.StringNumber == nStartLine && X1 >= 0)
							pszTest += X1;
						// Поскольку здесь мы ищем "назад", то пропуск пробелов будет ошибкой
						// -- while (*pszTest == _T(' ') || *pszTest == _T('\t')) pszTest++;
						pszBegin = psCommentBegin; pszEnd = psCommentEnd;
						while (*pszBegin && nMode != ewmUncommentStream)
						{
							int nBegin = lstrlen(pszBegin);
							for (int i = nBegin; i >= 1; i--)
							{
								if ((pszTest - egs.StringText) >= i
									&& _tcsncmp(pszTest-i, pszBegin, nBegin) == 0)
								{
									nMode = ewmUncommentStream;
									if (!lbFirstUncommented)
									{
										ei.CurPos -= i;
										X1 -= i;
									}
									break;
								}
							}
							if (nMode == ewmUncommentStream)
								break; // ибо for :(
							// Следующий
							pszBegin += lstrlen(pszBegin)+1;
							if (*pszEnd)
								pszEnd += lstrlen(pszEnd)+1;
						}
					}
					if (nMode == ewmUncommentStream)
					{
						//if (lbMultiComment) -- если был Auto - то lbMultiComment не был установлен, но и не надо, тип комментарования уже выбрали
						//{

						// Мог изменится "активный" тип комментирования
						lbMultiComment = false;
						psCommentBegin = pszBegin; psCommentEnd = pszEnd;
						// Длины - обновляются ниже, строго всегда

						//}
					}
					else
					{
						nMode = ewmUncommentBlock;
						lbMultiComment = (psComment[lstrlen(psComment)+1] != 0);
					}
				}
				if (nMode == ewmUncommentStream)
				{
					nCommentBeginLen = lstrlen(psCommentBegin);
					nCommentEndLen = lstrlen(psCommentEnd);
				}
				else
				{
					nCommentBeginLen = nCommentEndLen = lstrlen(psComment);
					// если lbMultiComment - нужно будет еще выбрать тип комментирования и обновить длины
				}
			}

			if (nMode == ewmUncommentBlock ||
				(nMode == ewmUncommentStream
				 && (!lbFirstUncommented || (egs.StringNumber == nEndLine))))
			{
				MCHKHEAP;
				LPCTSTR psComm = NULL, psCommCurr = NULL;
				if (nMode == ewmUncommentBlock)
				{
					psCommCurr = psComment;
				}
				else
				{
					psCommCurr = lbFirstUncommented ? psCommentEnd : psCommentBegin;

					if (egs.StringNumber == nStartLine)
					{
						LPCTSTR pszTest = egs.StringText;
						if (egs.StringNumber == nStartLine && X1 >= 0)
							pszTest += X1;
						while (*pszTest == _T(' ') || *pszTest == _T('\t'))
							pszTest++;
						if (_tcsncmp(pszTest, psCommentBegin, nCommentBeginLen) == 0)
							psComm = pszTest;
						// Это по идее не нужно, т.к. X1 подкорректирован выше
						//else if ((pszTest - egs.StringText) >= nCommentBeginLen
						//	&& _tcsncmp(pszTest-nCommentBeginLen, psCommentBegin, nCommentBeginLen) == 0)
						//	psComm = pszTest-nCommentBeginLen;
					}
				}
				
				if (!psComm)
				{
					if (nMode == ewmUncommentBlock)
					{
						LPCTSTR pszTest = egs.StringText;
						while (*pszTest == _T(' ') || *pszTest == _T('\t'))
							pszTest++;
						if (lbMultiComment)
						{
							LPCTSTR pszMulti = pszBlockComment;
							while (*pszMulti)
							{
								if (_tcsncmp(pszTest, pszMulti, lstrlen(pszMulti)) == 0)
								{
									psComm = pszTest;
									psCommCurr = psComment = pszMulti;
									nCommentBeginLen = nCommentEndLen = lstrlen(pszMulti);
									break;
								}		
								pszMulti += lstrlen(pszMulti)+1;
							}
						}
						else
						{
							if (_tcsncmp(pszTest, psCommCurr, lstrlen(psCommCurr)) == 0)
								psComm = pszTest;
						}		
						// Если таки не с начала строки, но есть в середине - поставить туда курсор?
						if (!psComm && (es.BlockType == BTYPE_NONE))
						{
							if (lbMultiComment)
							{
								LPCTSTR pszMulti = pszBlockComment;
								while (*pszMulti)
								{
									if ((psComm = _tcsstr(egs.StringText, pszMulti)) != NULL)
									{
										psCommCurr = psComment = pszMulti;
										nCommentBeginLen = nCommentEndLen = lstrlen(pszMulti);
										break;
									}		
									pszMulti += lstrlen(pszMulti)+1;
								}
							}
							else
							{
								psComm = _tcsstr(egs.StringText, psCommCurr);
							}		
							if (psComm)
							{
								EditorSetPosition esp;
								esp.CurLine = -1;
								esp.CurTabPos = -1;
								esp.TopScreenLine = -1;
								esp.LeftPos = -1;
								esp.CurPos = (int)(psComm - egs.StringText);
								esp.Overtype=-1;
								EditCtrl(ECTL_SETPOSITION,&esp);
								psComm = NULL; // но не убирать, а то "http://www..."
							}
						}
					}
					else
					{
						psComm = _tcsstr(egs.StringText, psCommCurr);
					}
				}

				if (!psComm)
				{
					if (egs.StringNumber == nStartLine)
						lbStartChanged = false;
					if (egs.StringNumber == nEndLine)
						lbEndChanged = false;
				}
				else
				{
					MCHKHEAP;
					if (nMode == ewmUncommentBlock)
					{
						// Коррекция будущего выделения
						if (egs.StringNumber == nStartLine)
						{
							if ((X1 > (psComm - egs.StringText)) && (X1 || Y1==Y2))
								X1 -= lstrlen(psCommCurr);
						}
						if (egs.StringNumber == nEndLine) // БЕЗ else - может быть однострочное выделение
						{
							if (X2 || Y1==Y2)
								X2 -= lstrlen(psCommCurr);
						}

						MCHKHEAP;
						if (psComm > egs.StringText)
						{
							// Комментарий НЕ с начала строки
							memcpy(lsText, egs.StringText, (psComm-egs.StringText)*sizeof(TCHAR));
							lstrcpy(lsText+(psComm-egs.StringText), psComm+lstrlen(psCommCurr));
							egs.StringText = lsText;
						}
						else
						{
							// Комментарий с начала строки - просто "отбросить" кусок (передвинуть указатель)
							egs.StringText += lstrlen(psCommCurr);
						}
						MCHKHEAP;
					}
					else
					{
						MCHKHEAP;
						size_t n = 0;
						LPCTSTR pszNextPart = egs.StringText;
						LPCTSTR pszEndComm = NULL;
						if (!lbFirstUncommented && (psComm >= egs.StringText))
						{
							if (psComm > egs.StringText)
							{
								memcpy(lsText+n, egs.StringText, (psComm-egs.StringText)*sizeof(TCHAR));
								n += (psComm-egs.StringText);
								MCHKHEAP;
							}
							pszNextPart = psComm + lstrlen(psCommCurr);
						}

						MCHKHEAP;
						if (egs.StringNumber == nEndLine)
						{
							if (lbFirstUncommented)
								psComm = egs.StringText;
							else
								psComm += nCommentBeginLen;
							// нужно отбросить и закрывающий коментатор
							pszEndComm = egs.StringText + X2;
							if (_tcsncmp(pszEndComm, psCommentEnd, nCommentEndLen) == 0)
							{
								// OK, выделение НЕ включало закрывающий коментатор
							}
							else if ((pszEndComm - psComm) >= nCommentEndLen
								&& _tcsncmp(pszEndComm-nCommentEndLen, psCommentEnd, nCommentEndLen) == 0)
							{
								// выделение включало закрывающий коментатор
								pszEndComm -= nCommentEndLen;
							}
							else
							{
								// Просто найти в строке закрывающий комментатор
								if (!lbFirstUncommented && psComm)
								{
									// Если в этой строке убирали "открывающий комментатор" - искать нужно после него
									pszEndComm = _tcsstr(psComm+nCommentBeginLen, psCommentEnd);
								}
								else
								{
									pszEndComm = _tcsstr(egs.StringText, psCommentEnd);
								}
								// Если выделения нет, убираем потоковый комментарий, и на этой строке закрывающего нет - 
								// то искать вниз до упора
								if (!pszEndComm && (ei.BlockType == BTYPE_NONE) && (nEndLine < ei.TotalLines))
									nEndLine++;
							}
							MCHKHEAP;
							if (pszEndComm)
							{
								_ASSERTE(nMaxStrLen > (pszEndComm-psComm));
								memcpy(lsText+n, psComm, (pszEndComm-psComm)*sizeof(TCHAR));
								MCHKHEAP;
								n += (pszEndComm-psComm);
								lstrcpy(lsText+n, pszEndComm+nCommentEndLen);
								MCHKHEAP;
							}
							else
							{
								// Просто докопировать остаток (чтобы не потерять ничего)
								lstrcpy(lsText+n, pszNextPart);
								MCHKHEAP;
							}
						}
						else
						{
							lstrcpy(lsText+n, pszNextPart);
							MCHKHEAP;
						}

						// Коррекция будущего выделения
						if (egs.StringNumber == nStartLine)
						{
							if ((X1 > (psComm - egs.StringText)) && (X1 || Y1==Y2))
								X1 -= nCommentBeginLen;
						}
						if (egs.StringNumber == nEndLine) // БЕЗ else - может быть однострочное выделение
						{
							if ((X2 || Y1==Y2) && (pszEndComm && (pszEndComm - egs.StringText) < X2))
								X2 -= nCommentEndLen;
							if (Y1 == Y2)
								X2 -= nCommentBeginLen;
						}

						egs.StringText = lsText;
					}
					MCHKHEAP;

					if (egs.StringNumber == ei.CurLine)
						lbCurLineShifted = true;
					if (!lbFirstUncommented)
						lbFirstUncommented = true;
				}
			}
        }

        MCHKHEAP;
		ess.StringLength = lstrlen(egs.StringText);
		ess.StringText = (TCHAR*)egs.StringText;
		ess.StringNumber = egs.StringNumber;
		ess.StringEOL = (TCHAR*)egs.StringEOL;
		EditCtrl(ECTL_SETSTRING,&ess);
        MCHKHEAP;
    }
    free(lsText);
    return TRUE;
}

void Reselect()
{
	if (nMode <= ewmLastInternal && es.BlockType != BTYPE_NONE)
    {
		if (es.BlockType==BTYPE_STREAM)
		{
			es.BlockStartLine = min(Y2,Y1);
			es.BlockStartPos = (Y1 == Y2) ? (min(X1,X2)) : ((Y1 < Y2) ? X1 : X2);

			// небольшая коррекция, если позиции равны
			if(X1 == X2)
				es.BlockStartPos += (Y1 < Y2) ? 1: -1;

			es.BlockHeight = max(Y1,Y2)-min(Y1,Y2)+1;

			if (Y1 < Y2)
				es.BlockWidth = X2-X1/*+1*/;
			else if (Y1 > Y2)
				es.BlockWidth = X1-X2+1;
			else if (Y1 == Y2)
				es.BlockWidth = max(X1,X2)-min(X1,X2);

			if(X1 == X2)
			{
				if(Y1 < Y2)
					es.BlockStartPos--;
				else
					es.BlockStartPos++;
			}
		}
		else
		{
			//#ifndef _UNICODE
			// В FAR 1.7x build 2479 возникала проблема если
			// TAB не заменяется проблами и идет выделение верт.блока
			// gdwFarVersion == 0x09af014b
			if (!lbExpandTabs)
			{
				EditorConvertPos ecp;
				ecp.StringNumber = Y1; ecp.SrcPos = ecp.DestPos = X1;
				if (EditCtrl(ECTL_REALTOTAB, &ecp))
					X1 = ecp.DestPos;
				ecp.SrcPos = ecp.DestPos = X2;
				if (EditCtrl(ECTL_REALTOTAB, &ecp))
					X2 = ecp.DestPos;
			}
			//#endif

			es.BlockStartLine=min(Y2,Y1);
			es.BlockStartPos=(Y1==Y2) ? (min(X1,X2)) : (Y1 < Y2?X1:X2);
			
			// небольшая коррекция, если позиции равны
			if(X1 == X2)
				es.BlockStartPos+=(Y1 < Y2?1:-1);
			
			es.BlockHeight=max(Y1,Y2)-min(Y1,Y2)+1;
			
			if(Y1 < Y2)
				es.BlockWidth=X2-X1/*+1*/;
			else if(Y1 > Y2)
				es.BlockWidth=X1-X2+1;
			else if (Y1 == Y2)
				es.BlockWidth=max(X1,X2)-min(X1,X2);
			
			if(X1 == X2)
			{
				if(Y1 < Y2)
					es.BlockStartPos--;
				else
					es.BlockStartPos++;
			}
		}

        MCHKHEAP;
        EditCtrl(ECTL_SELECT,(void*)&es);
    }
}

void UpdateCursorPos()
{
	if (nMode <= ewmLastInternal && lbCurLineShifted)
	{
		EditorSetPosition esp;
		esp.CurLine = -1;
		esp.CurPos = esp.CurTabPos = -1;
		esp.TopScreenLine = -1;
		esp.LeftPos = -1;
		switch (nMode)
		{
		case ewmTabulateRight:
			esp.CurTabPos = ei.CurTabPos+ei.TabSize;
			break;
		case ewmTabulateLeft:
			esp.CurTabPos = max(0,ei.CurTabPos-ei.TabSize);
			break;
		case ewmCommentBlock:
			//WARNING: для потоковых комментариев - доработать
			esp.CurPos = ei.CurPos+lstrlen(psComment);
			break;
		case ewmCommentStream:
			//WARNING: для потоковых комментариев - доработать
			//esp.CurPos = ei.CurPos+lstrlen(psCommentBegin);
			if ((Y1 == Y2) && (X2 == (ei.CurPos+nCommentBeginLen+nCommentEndLen)))
				esp.CurPos = ei.CurPos + nCommentBeginLen + nCommentEndLen;
			else if (Y1 == ei.CurLine)
				esp.CurPos = ei.CurPos + ((ei.CurPos > X1 && (ei.BlockType != BTYPE_NONE)) ? nCommentBeginLen : 0);
			else if (Y2 == ei.CurLine)
				esp.CurPos = ei.CurPos + ((ei.CurPos >= X2 && (ei.BlockType != BTYPE_NONE)) ? nCommentEndLen : 0);
			else
				esp.CurPos = ei.CurPos;
			break;
		case ewmUncommentBlock:
			esp.CurPos = max(0,ei.CurPos-lstrlen(psComment));
			break;
		case ewmUncommentStream:
			//WARNING: для потоковых комментариев - доработать
			//esp.CurPos = max(0,ei.CurPos-lstrlen(psCommentBegin));
			if ((Y1 == Y2) && (ei.CurPos >= (X2+nCommentBeginLen+nCommentEndLen)))
				esp.CurPos = ei.CurPos - nCommentBeginLen - nCommentEndLen;
			else if (Y1 == ei.CurLine)
			{
				if (ei.CurPos >= (X1+nCommentBeginLen))
					esp.CurPos = ei.CurPos - nCommentBeginLen;
				else
					esp.CurPos = ei.CurPos;
			}
			else if (Y2 == ei.CurLine)
				esp.CurPos = ei.CurPos - nCommentEndLen;
			else
				esp.CurPos = ei.CurPos;
			if (esp.CurPos < 0)
				esp.CurPos = 0;
			break;
		default:
			esp.CurTabPos = ei.CurTabPos;
		}

		if (esp.CurPos > 0)
		{
			egs.StringNumber = ei.CurLine;
			if (EditCtrl(ECTL_GETSTRING,&egs) && esp.CurPos > egs.StringLength)
				esp.CurPos = egs.StringLength;
		}

		esp.Overtype=-1;
		
		//TODO: сдвиг курсора при обычной табуляции
		EditCtrl(ECTL_SETPOSITION,&esp);
	}
}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item)
{
    
    memset(&egs, 0, sizeof(egs));
    memset(&ei, 0, sizeof(ei));
    memset(&ess, 0, sizeof(ess));
    memset(&es, 0, sizeof(es));
	nMode = ewmUndefined;

	#if FAR_UNICODE>=3000
	egs.StructSize = sizeof(egs);
	ei.StructSize = sizeof(ei);
	ess.StructSize = sizeof(ess);
	es.StructSize = sizeof(es);
	#endif

    MCHKHEAP;

    EditCtrl(ECTL_GETINFO,&ei);
    //if (ei.BlockType == BTYPE_NONE)
    //    return INVALID_HANDLE_VALUE;
    es.BlockType = ei.BlockType;

    //TODO: Если выделения нет, и хотят Tab/ShiftTab можно переслать в ФАР
    //ACTL_POSTKEYSEQUENCE TAB/требуемое количество BS


    MCHKHEAP;
    //CStaticMenu lmMenu(_ATOT(szMsgBlockEditorPlugin));

    /*lmMenu.AddMenuItem ( _T("&1 Tabulate right") );
    lmMenu.AddMenuItem ( _T("&2 Tabulate left") );
    lmMenu.AddMenuItem ( _T("&3 Comment") );
    lmMenu.AddMenuItem ( _T("&4 UnComment") );*/
    //psi.Menu(

#ifdef _UNICODE
	#if FARMANAGERVERSION_BUILD>=2460
	if ((OpenFrom == OPEN_FROMMACRO))
	{
		OpenMacroInfo* pi = (OpenMacroInfo*)Item;
		if (pi && (pi->StructSize >= sizeof(*pi)) && (pi->Count > 0))
		{
			int nArg = ewmUndefined;
			if (pi->Values[0].Type == FMVT_INTEGER)
				nArg = (int)pi->Values[0].Integer;
			else if (pi->Values[0].Type == FMVT_DOUBLE)
				nArg = (int)pi->Values[0].Double; // We need only integer commands
			// Validate
			if ((nArg >= ewmFirst) && (nArg <= ewmLastValidCall))
			{
				nMode = nArg;
			}
		}
	}
	#else
	if (((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO) && (Item >= ewmFirst && Item <= ewmLastValidCall))
	{
		nMode = (int)Item;
	}
	#endif
#endif


	if (nMode == ewmUndefined)
	{
		#if FAR_UNICODE>=1906
		FarMenuItem pItems[] =
		{
			{0, L"&1 Tabulate right"},
			{0, L"&2 Tabulate left"},
			{0, L"&3 Comment auto"},
			{0, L"&4 UnComment"},
			{0, L"&5 Comment block"},
			{0, L"&6 Comment stream"}
		};
		#else
		FarMenuItem pItems[] =
		{
			{_T("&1 Tabulate right")},
			{_T("&2 Tabulate left")},
			{_T("&3 Comment auto")},
			{_T("&4 UnComment")},
			{_T("&5 Comment block")},
			{_T("&6 Comment stream")}
		};
		#endif

		MCHKHEAP;

		//int nMode = lmMenu.DoModal();
		#if FAR_UNICODE>=1900
		nMode = psi.Menu(&guid_BlockEditor, &guid_Menu2,
			-1,-1, 0, /*FMENU_CHANGECONSOLETITLE|*/FMENU_WRAPMODE,
			_T(szMsgBlockEditorPlugin), NULL, NULL,
			NULL, NULL, pItems, ARRAYSIZE(pItems));
		#else
		nMode = psi.Menu(psi.ModuleNumber,
			-1, -1, 0, FMENU_WRAPMODE,
			_T(szMsgBlockEditorPlugin),NULL,NULL,
			0, NULL, pItems, ARRAYSIZE(pItems) );
		#endif
		if (nMode < 0)
			return INVALID_HANDLE_VALUE;
		nMode++;
	}
    MCHKHEAP;

    if (nMode < ewmFirst || nMode > ewmLastValidCall)
        return INVALID_HANDLE_VALUE;

	#ifdef _UNICODE
	EditorUndoRedo eur = {};
	#if FAR_UNICODE>=3000
	eur.StructSize = sizeof(eur);
	#endif
	eur.Command = EUR_BEGIN;
	EditCtrl(ECTL_UNDOREDO,&eur);

	INT_PTR nLen = EditCtrl(ECTL_GETFILENAME,NULL);
	wchar_t *pszFileName = (wchar_t*)calloc(nLen+1,2);
	EditCtrl(ECTL_GETFILENAME,pszFileName,nLen);
	#else
	LPCSTR pszFileName = ei.FileName;
	#endif

	lbSkipNonSpace = TRUE;
	lbSkipCommentMenu = FALSE;
    psComment = _T("//");
    psCommentBegin = NULL;
    psCommentEnd = NULL;
	nCommentBeginLen = 0;
	nCommentEndLen = 0;
	// Обязательно инициализируем все 0-ми, т.к. может быть MSZ
	TCHAR szComment[100] = {0}, szCommentBegin[100] = {0}, szCommentEnd[100] = {0};
    if ((nMode >= ewmCommentFirst && nMode <= ewmCommentLast) && pszFileName)
    	LoadCommentSettings(pszFileName, szComment, szCommentBegin, szCommentEnd);


	#ifdef _UNICODE
	if (pszFileName)
	{
		free(pszFileName); pszFileName = NULL;
	}
	#endif


    if (ei.TabSize<1) ei.TabSize=1;
    lbExpandTabs = (ei.Options & (EOPT_EXPANDALLTABS|EOPT_EXPANDONLYNEWTABS))!=0;

    MCHKHEAP;

    nStartLine = (ei.BlockType == BTYPE_NONE) ? ei.CurLine : ei.BlockStartLine;
    nEndLine = nStartLine;
    //int nEmptyLine = -1;
    Y1 = nStartLine; Y2 = -1;
    X1 = 0; X2 = -1;
	lbCurLineShifted = false;
    nInsertSlash = -1;
    nMaxStrLen = 0;
	lbStartChanged = true; lbEndChanged = true; // измнены ли первая и последняя строки выделения?
	lbMultiComment = false;

    MCHKHEAP;

	// Пробежаться по выделению, определить максимальную длину строки
	FindMaxStringLen();

    MCHKHEAP;

    if (!PrepareCommentParams())
    	return INVALID_HANDLE_VALUE;


    nMaxStrLen += 10+ei.TabSize; // с запасом на символы комментирования
	if (psCommentBegin && psCommentEnd)
		nMaxStrLen += lstrlen(psCommentBegin) + lstrlen(psCommentEnd);
	if (psComment)
		nMaxStrLen += lstrlen(psComment);

    MCHKHEAP;

    if (ei.BlockType == BTYPE_NONE)
        X1 = ei.CurPos;


	// Поехали
	if (nMode == ewmTabulateRight)
		DoTabRight();
	else if (nMode == ewmTabulateLeft)
		DoTabLeft();
	else if (nMode == ewmCommentBlock || nMode == ewmCommentStream)
		DoComment();
	else if (nMode == ewmUncommentAuto || nMode == ewmUncommentBlock || nMode == ewmUncommentStream)
		DoUnComment();
	

	if (X1<0)
	{
		//_ASSERTE(X1>=0);
		X1 = 0;
	}
	if (X2<0)
	{
		//_ASSERTE(X2>=0);
		X2 = 0;
	}

	if (nMode == ewmTabulateRight)
	{
		int nSize = lbExpandTabs ? ei.TabSize : 1;
		if (X1 || Y1==Y2) X1 += nSize;
		if (X2 || Y1==Y2) X2 += nSize;
	}
	else if (nMode == ewmTabulateLeft)
	{
		int nSize = lbExpandTabs ? ei.TabSize : 1;
		// X1&X2 корректируются выше, при обработке ewmTabulateLeft
	}
	//else if (nMode == ewmComment)
	//{
	//	int nCmtLen = lstrlen(psComment);
	//	//if (X1 || Y1==Y2) X1 += nCmtLen;
	//	if (X2 || Y1==Y2) X2 += nCmtLen;
	//}
	//else if (nMode == ewmUncomment)
	//{
	//	int nCmtLen = lstrlen(psComment);
	//	//if (lbStartChanged && (X1 || Y1==Y2))
	//	//	X1 = (X1>nCmtLen) ? (X1-nCmtLen) : 0;
	//	if (lbEndChanged && (X2 || Y1==Y2))
	//		X2 = (X2>nCmtLen) ? (X2-nCmtLen) : 0;
	//}

    MCHKHEAP;

	// Подвинуть курсор
	UpdateCursorPos();	

	// Обновить выделение
	Reselect();

	#ifdef _UNICODE
	eur.Command = EUR_END;
	EditCtrl(ECTL_UNDOREDO,&eur);
	#endif

    MCHKHEAP;

    return INVALID_HANDLE_VALUE;
}
