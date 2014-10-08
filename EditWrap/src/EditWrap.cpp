
#include <Windows.h>
#include <crtdbg.h>
#include <tchar.h>
#include "../../common/plugin.h"
#include "../../common/FarHelper.h"

#if FAR_UNICODE>=2184

	#undef FAR_UNICODE
	#define FAR_UNICODE FARMANAGERVERSION_BUILD

	// String GUID is used in code
	const wchar_t guid_PluginGuidS[] = L"580F7F4F-7E64-4367-84C1-5A6EB66DAB1F";
	GUID guid_PluginGuid = { /* 580f7f4f-7e64-4367-84c1-5a6eb66dab1f */
		0x580f7f4f,
		0x7e64,
		0x4367,
		{0x84, 0xc1, 0x5a, 0x6e, 0xb6, 0x6d, 0xab, 0x1f}
	};
	GUID guid_EditWrapMenuItem = { /* f5a6fa34-13a9-40f6-9103-3ffed1f2a9c8 */
		0xf5a6fa34,
		0x13a9,
		0x40f6,
		{0x91, 0x03, 0x3f, 0xfe, 0xd1, 0xf2, 0xa9, 0xc8}
	};
	GUID guid_EditWrapMenuWork = { /* 2b302398-bbe9-4aff-b90e-7114a33432f5 */
		0x2b302398,
		0xbbe9,
		0x4aff,
		{0xb9, 0x0e, 0x71, 0x14, 0xa3, 0x34, 0x32, 0xf5}
	};

#endif

#include "version.h"

struct PluginStartupInfo psi;
struct FarStandardFunctions FSF;

BOOL    gbLastWrap = FALSE;
int    *gpnWrappedEditors = NULL;
size_t  gnWrappedEditors = 0;

TCHAR gsWordDiv[256] = _T(" \t\r\n~!%^&*()+|{}:\"<>?`-=\\[];',./");
TCHAR gsPuctuators[256] = _T(" \t\r\n!?;,.");
inline bool IsSpaceOrNull(TCHAR x) { return x==_T(' ') || x==_T('\t') || x==0;  }

#define szMsgEditWrapPlugin _T("EditWrap")
#define szMsgItemToggleWrap _T("&1. Toggle wrap")
#define szMsgItemToggleWordWrap _T("&2. Toggle word wrap")
#define szMsgItemWrap _T("&3. Wrap")
#define szMsgItemWordWrap _T("&4. Word wrap")
#define szMsgItemUnWrap _T("&5. Unwrap")

HMODULE ghInstance=NULL;

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

int WINAPI GetMinFarVersionW(void)
{
	#if FAR_UNICODE>=2184
	#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
	return MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD);
	#else
	return FARMANAGERVERSION;
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
		//Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10), VS_RELEASE);
		Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,MVV_4, VS_RELEASE);
		
		Info->Guid = guid_PluginGuid;
		Info->Title = szMsgEditWrapPlugin;
		Info->Description = szMsgEditWrapPlugin;
		Info->Author = L"ConEmu.Maximus5@gmail.com";
	}
#endif

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
	static TCHAR szMenu[MAX_PATH];
	lstrcpy(szMenu, szMsgEditWrapPlugin);
    static TCHAR *pszMenu[1];
    pszMenu[0] = szMenu;

	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR|PF_DISABLEPANELS;

	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_EditWrapMenuItem;
		pi->PluginMenu.Strings = pszMenu;
		pi->PluginMenu.Count = 1;
	#else
		pi->PluginMenuStrings = pszMenu;
		pi->PluginMenuStringsNumber = 1;
		pi->Reserved = 0x45644664; // EdFd
	#endif
}

void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo)
{
	::psi = *aInfo;
	::FSF = *aInfo->FSF;
	::psi.FSF = &::FSF;
}

void   WINAPI ExitFARW(
	#if FAR_UNICODE>=2000
		void*
	#else
		void
	#endif
)
{
}

enum FoldWorkMode
{
	fwm_None = -1,
	fwm_ToggleWrap = 1,
	fwm_ToggleWordWrap = 2,
	fwm_Wrap = 3,
	fwm_WordWrap = 4,
	fwm_Unwrap = 5,
};

//void SetMenuItem(FarMenuItem* pItems, int nIdx, LPCTSTR pszBegin, LPCTSTR pszEnd)
//{
//	#ifdef _UNICODE
//	wchar_t* psz = (wchar_t*)calloc(64,sizeof(wchar_t));
//	pItems[nIdx].Text = psz;
//	#else
//	char* psz = pItems[nIdx].Text;
//	#endif
//	
//	#define MENU_PART 4
//	
//	if (nIdx < 9)
//		wsprintf(psz, _T("&%i. "), nIdx+1);
//	else if (nIdx == 9)
//		lstrcpy(psz, _T("&0. "));
//	else
//		lstrcpy(psz, _T("   "));
//	
//	int iStart = lstrlen(psz);
//	int i = iStart;
//	lstrcpyn(psz+i, pszBegin, MENU_PART+1 /*+1 т.к. включая \0*/);
//	i = lstrlen(psz);
//	int iFin = iStart+MENU_PART+1;
//	while (i < iFin)
//		psz[i++] = _T(' ');
//	psz[i] = 0;
//	if (pszEnd)
//		lstrcpyn(psz+i, pszEnd, MENU_PART+1 /*+1 т.к. включая \0*/);
//	i = lstrlen(psz);
//	iFin = iStart+MENU_PART*2+2;
//	while (i < iFin)
//		psz[i++] = _T(' ');
//	psz[i] = 0;
//	lstrcat(psz, pszEnd ? _T("Stream") : _T("Block"));
//}

FoldWorkMode ChooseWorkMode()
{
	#if FAR_UNICODE>=1900
	FarMenuItem
	#else
	FarMenuItemEx
	#endif
		Items[] =
	{
		{0, szMsgItemToggleWrap},
		{0, szMsgItemToggleWordWrap},
		{MIF_SEPARATOR},
		{0, szMsgItemWrap},
		{0, szMsgItemWordWrap},
		{0, szMsgItemUnWrap},
	};

	int nSel = -1;
	#if FAR_UNICODE>=1900
	nSel = psi.Menu(&guid_PluginGuid, &guid_EditWrapMenuWork,
		-1,-1, 0, /*FMENU_CHANGECONSOLETITLE|*/FMENU_WRAPMODE,
		szMsgEditWrapPlugin, NULL, NULL,
		NULL, NULL, Items, ARRAYSIZE(Items));
	#else
	nSel = psi.Menu(psi.ModuleNumber,
		-1, -1, 0, FMENU_WRAPMODE|FMENU_USEEXT,
		szMsgEditWrapPlugin,NULL,NULL,
		0, NULL, (FarMenuItem*)Items, ARRAYSIZE(Items));
	#endif

	switch (nSel)
	{
	case 0:
		return fwm_ToggleWrap;
	case 1:
		return fwm_ToggleWordWrap;
	case 3:
		return fwm_Wrap;
	case 4:
		return fwm_WordWrap;
	case 5:
		return fwm_Unwrap;
	}

	return fwm_None;
}

INT_PTR FindExceed(wchar_t* pszCopy, INT_PTR iLine, INT_PTR iFrom, INT_PTR iFind, int iMaxWidth, int TabSize)
{
	INT_PTR nExceed = 0;

	// В строке могут быть '\0', так что проверки типа wcschr надежны только в положительном варианте
	wchar_t* pszTab = wcschr(pszCopy+iFrom, L'\t');
	if (pszTab && (pszTab > (pszCopy + iFrom + iMaxWidth)))
	{
		nExceed = iFind;
	}
	else
	{
		INT_PTR TabPos = 1;

		// Проходим по всем символам до позиции поиска, если она ещё в пределах строки,
		// либо до конца строки, если позиция поиска за пределами строки
		for (nExceed = iFrom; (nExceed < iFind) && (TabPos <= iMaxWidth); nExceed++)
		{
			// Обрабатываем табы
			if (pszCopy[nExceed] == L'\t')
			{
				// Расчитываем длину таба с учётом настроек и текущей позиции в строке
				TabPos += TabSize-(TabPos%TabSize);
			}
			else
				TabPos++;
		}

		if ((TabPos > iMaxWidth) && (nExceed > iFrom) && (pszCopy[nExceed-1] == L'\t'))
		{
			nExceed--; // чтобы "конец" таба за экран не вылезал
		}
		//	bExceed = true;
	}

	//return bExceed;
	return nExceed;
}

void DoWrap(BOOL abWordWrap, EditorInfo &ei, int iMaxWidth)
{
	INT_PTR iRc = 0;
	INT_PTR cchMax = 0;
	TCHAR* pszCopy = NULL;
	TCHAR szEOL[4];
	INT_PTR iFrom, iTo, iEnd, iFind;
	bool bWasModifed = (ei.CurState & ECSTATE_MODIFIED) && !(ei.CurState & ECSTATE_SAVED);

	gbLastWrap = TRUE;
	
	for (INT_PTR i = 0; i < ei.TotalLines; i++)
	{
		//bool lbCurLine = (i == ei.CurLine);
		
		EditorGetString egs = {FARSTRUCTSIZE(egs)};
		egs.StringNumber = i;
		iRc = EditCtrl(ECTL_GETSTRING, &egs);
		if (!iRc)
		{
			_ASSERTE(iRc!=0);
			goto wrap;
		}
		_ASSERTE(egs.StringText!=NULL);
		
		if ((egs.StringLength <= iMaxWidth)
			&& ((egs.StringLength <= 0) || !(egs.StringText && wcschr(egs.StringText, L'\t'))))
		{
			// Эту строку резать не нужно
			continue;
		}
		
		lstrcpyn(szEOL, egs.StringEOL?egs.StringEOL:_T(""), ARRAYSIZE(szEOL));
		
		if (egs.StringLength >= cchMax || !pszCopy)
		{
			if (pszCopy)
				free(pszCopy);
			cchMax = egs.StringLength + 255;
			pszCopy = (TCHAR*)malloc(cchMax*sizeof(*pszCopy));
			if (!pszCopy)
			{
				_ASSERTE(pszCopy!=NULL);
				goto wrap;
			}
		}
		// Делаем копию, над которой можем издеваться
		memmove(pszCopy, egs.StringText, egs.StringLength*sizeof(*pszCopy));
		pszCopy[egs.StringLength] = 0; // на всякий случай, хотя вроде должен быть ASCIIZ
		
		bool lbFirst = 0;
		iFrom = 0; iEnd = egs.StringLength;
		while (iFrom < iEnd)
		{
			//iTo = min(iEnd,(iFrom+iMaxWidth));
			iTo = FindExceed(pszCopy, i, iFrom, min(iEnd/*+1*/,(iFrom+iMaxWidth)), iMaxWidth, ei.TabSize);
			iFind = iTo;
			if (iFind >= iEnd)
			{
				iFind = iTo = iEnd;
			}
			else if (abWordWrap
				/*&& (((egs.StringLength - iFrom) > iMaxWidth) || IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize))*/
				)
			{
				while (iFind > iFrom)
				{
					if (IsSpaceOrNull(pszCopy[iFind-1]))
						break;
					//{
					//	// Если есть табы - нужно учитывать их ширину
					//	//TODO: Optimize, по хорошему, если есть табы, нужно оптимизировать расчет экранной позиции
					//	bool bExceed = IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize);

					//	if (!bExceed)
					//		break;
					//}
					iFind--;
				}
				// Если по пробелам порезать не удалось, попробуем по другим знакам?
				if (iFind == iFrom)
				{
					iFind = iTo;
					while (iFind > iFrom)
					{
						if (_tcschr(gsPuctuators, pszCopy[iFind]) && !_tcschr(gsWordDiv, pszCopy[iFind-1]))
							break;
						//{
						//	// Если есть табы - нужно учитывать их ширину
						//	//TODO: Optimize, по хорошему, если есть табы, нужно оптимизировать расчет экранной позиции
						//	bool bExceed = IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize);

						//	if (!bExceed)
						//		break;
						//}
						iFind--;
					}
					if (iFind == iFrom)
					{
						iFind = iTo;
						while (iFind > iFrom)
						{
							if (_tcschr(gsWordDiv, pszCopy[iFind]) && !_tcschr(gsWordDiv, pszCopy[iFind-1]))
								break;
							//{
							//	// Если есть табы - нужно учитывать их ширину
							//	//TODO: Optimize, по хорошему, если есть табы, нужно оптимизировать расчет экранной позиции
							//	bool bExceed = IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize);

							//	if (!bExceed)
							//		break;
							//}
							iFind--;
						}
					}
				}
			}
			if (iFind == iFrom)
				iFind = iTo;

			if (iFind < iEnd)
			{
				// Для ECTL_INSERTSTRING нужно установить курсор
				EditorSetPosition eset = {FARSTRUCTSIZE(eset)};
				eset.CurLine = i;
				eset.TopScreenLine = -1;
				EditCtrl(ECTL_SETPOSITION, &eset);
				// Теперь можно разорвать строку
				EditCtrl(ECTL_INSERTSTRING, NULL);
				// Чтобы вернуть курсор на исходную строку
				if (i < ei.CurLine)
					ei.CurLine++;
			}
			// И менять ее данные
			EditorSetString esset = {FARSTRUCTSIZE(esset)};
			esset.StringNumber = i;
			esset.StringText = pszCopy+iFrom;
			esset.StringEOL = (iFind == iEnd) ? szEOL : _T("");
			esset.StringLength = (iFind - iFrom);
			EditCtrl(ECTL_SETSTRING, &esset);

			// Накрутить счетчики
			if (iFind < iEnd)
			{
				i++; ei.TotalLines++; // т.к. вставили строку
			}
			
			// Следующая часть строки
			iFrom = iFind;
		}
	}

	// Обновить позицию курсора
	{
		EditorSetPosition eset = {FARSTRUCTSIZE(eset)};
		eset.CurLine = ei.CurLine;
		eset.TopScreenLine = -1;
		EditCtrl(ECTL_SETPOSITION, &eset);
	}
	
wrap:
	if (pszCopy)
		free(pszCopy);

#ifdef _UNICODE
	// Сброс флага "редактирован"
	//TODO: bis-сборка?
	if (!bWasModifed)
		EditCtrl(ECTL_DROPMODIFEDFLAG, NULL);
#endif
}

void DoUnwrap(EditorInfo &ei)
{
	INT_PTR iRc = 0;
	INT_PTR cchMax = 0, cchPos = 0;
	TCHAR* pszCopy = NULL;
	TCHAR szEOL[4];
	bool bWasModifed = (ei.CurState & ECSTATE_MODIFIED) && !(ei.CurState & ECSTATE_SAVED);

	gbLastWrap = FALSE;
	
	for (INT_PTR i = 0; i < ei.TotalLines; i++)
	{
		EditorGetString egs = {FARSTRUCTSIZE(egs)};
		egs.StringNumber = i;
		iRc = EditCtrl(ECTL_GETSTRING, &egs);
		if (!iRc)
		{
			_ASSERTE(iRc!=0);
			goto wrap;
		}
		_ASSERTE(egs.StringText!=NULL);
		
		if (egs.StringEOL && *egs.StringEOL)
		{
			// В этой строке есть EOL, ее не сворачивали
			continue;
		}
		
		cchPos = 0;
		szEOL[0] = 0;
		INT_PTR j = i;
		while (j < ei.TotalLines)
		{
			if (!pszCopy || ((egs.StringLength + cchPos + 65536) > cchMax))
			{
				cchMax = egs.StringLength + cchPos + 65536;
				TCHAR* pszNew = (TCHAR*)malloc(cchMax*sizeof(*pszCopy));
				if (!pszNew)
				{
					_ASSERTE(pszNew!=NULL);
					goto wrap;
				}
				if (pszCopy)
				{
					if (cchPos > 0)
						memmove(pszNew, pszCopy, cchPos*sizeof(*pszCopy));
					free(pszCopy);
				}
				pszCopy = pszNew;
			}
			
			if (egs.StringLength > 0)
			{
				memmove(pszCopy+cchPos, egs.StringText, egs.StringLength*sizeof(*pszCopy));
				cchPos += egs.StringLength;
			}

			bool lbApplyAndBreak = false;

			if (*szEOL)
			{
				lbApplyAndBreak = true;
			}

			// Получить следующую строку
			if ((j+1) >= ei.TotalLines)
			{
				lbApplyAndBreak = true;
			}
			else if (!lbApplyAndBreak)
			{
				egs.StringNumber = ++j;
				iRc = EditCtrl(ECTL_GETSTRING, &egs);
				if (!iRc)
				{
					_ASSERTE(iRc!=0);
					goto wrap;
				}
				_ASSERTE(egs.StringText!=NULL);
				if (egs.StringEOL && *egs.StringEOL)
				{
					// В этой строке есть EOL, ее не сворачивали
					lstrcpyn(szEOL, egs.StringEOL?egs.StringEOL:_T(""), ARRAYSIZE(szEOL));
				}
			}
			
			if (lbApplyAndBreak)
			{
				EditorSetString esset = {FARSTRUCTSIZE(esset)};
				esset.StringNumber = i;
				esset.StringText = pszCopy;
				esset.StringEOL = szEOL;
				esset.StringLength = cchPos;
				EditCtrl(ECTL_SETSTRING, &esset);
				
				for (INT_PTR k = i+1; k <= j; k++)
				{
					// Для ECTL_DELETESTRING нужно установить курсор
					EditorSetPosition eset = {FARSTRUCTSIZE(eset)};
					eset.CurLine = i+1;
					eset.TopScreenLine = -1;
					iRc = EditCtrl(ECTL_SETPOSITION, &eset);
					_ASSERTE(iRc);
					// Удаляем "свернутое"
					iRc = EditCtrl(ECTL_DELETESTRING, NULL);
					_ASSERTE(iRc);
					ei.TotalLines--;
					if (ei.CurLine > i)
						ei.CurLine--;
				}
				
				// Выход из while
				break;
			}
		}
	}
	
	// Обновить позицию курсора
	{
		EditorSetPosition eset = {FARSTRUCTSIZE(eset)};
		eset.CurLine = ei.CurLine;
		eset.TopScreenLine = -1;
		EditCtrl(ECTL_SETPOSITION, &eset);
	}
	
wrap:
	if (pszCopy)
		free(pszCopy);

#ifdef _UNICODE
	// Сброс флага "редактирован"
	//TODO: bis-сборка?
	if (!bWasModifed)
		EditCtrl(ECTL_DROPMODIFEDFLAG, NULL);
#endif
}

#if !defined(FAR_UNICODE) || (FAR_UNICODE<3000)
bool CheckScrollEnabled(const SMALL_RECT& rcFar)
{
	bool bScrollExists = false;

	#if 0
	GUID FarGuid = {};
	PluginSettings fs(FarGuid, psi.SettingsControl);
	bScrollExists = fs.Get(FSSF_EDITOR, L"ShowScrollBar", false);
	if (bScrollExists)
		return true;
	#endif

	// Нет способа проверить через API, включена ли прокрутка в конкретном экземпляре редактора
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	GetConsoleScreenBufferInfo(hOut, &csbi);
	CHAR_INFO ch[2]; COORD crSize = {1,2}; COORD crRead = {0,0};
	int iTop = 1; // csbi.dwSize.Y - rcFar.Bottom;
	SMALL_RECT srRegion = {rcFar.Right, rcFar.Top+iTop, rcFar.Right, rcFar.Top+1+iTop};
	if (ReadConsoleOutput(hOut, ch, crSize, crRead, &srRegion))
	{
		if ((ch[0].Char.UnicodeChar == L'▲')
			&& ((ch[1].Char.UnicodeChar == L'▓') || (ch[1].Char.UnicodeChar == L'░')))
		{
			bScrollExists = true;
		}
	}

	return bScrollExists;
}
#endif

HANDLE WINAPI OpenPluginW(
#if FAR_UNICODE>=2000
	OpenInfo *Info
#else
	int OpenFrom,INT_PTR Item
#endif
)
{
#if FAR_UNICODE>=1900
	int OpenFrom = Info->OpenFrom;
	INT_PTR Item = Info->Data;
#endif
	WindowInfo wi = {FARSTRUCTSIZE(wi)};
	EditorInfo ei = {FARSTRUCTSIZE(ei)};
	INT_PTR iRc = 0;
	INT_PTR cchMax = 0;
	SMALL_RECT rcFar = {};
	int iMaxWidth = 79;
	int iRightPad = 0; bool bRightPadSpecified = false;
	EditorUndoRedo ur = {FARSTRUCTSIZE(ur)};
	FoldWorkMode WorkMode = fwm_None;
	int *pWrappedEditor = NULL;
	int *pnFreeEditorId = NULL;

	#if FAR_UNICODE>=2000
	wi.StructSize = sizeof(wi);
	#endif
	wi.Pos = -1;

	#ifdef _UNICODE
	if (psi.AdvControl(PluginNumber, ACTL_GETFARRECT, FADV1988 &rcFar))
	{
		iMaxWidth = rcFar.Right - rcFar.Left;
		#if !defined(FAR_UNICODE) || (FAR_UNICODE<3000)
		// Будет ниже, через макрос
		if (CheckScrollEnabled(rcFar))
		{
			iRightPad = 1; bRightPadSpecified = true;
		}
		#endif
	}
	#endif

	/*
#ifdef _DEBUG
	iMaxWidth = 20;
#endif
	*/
	
	psi.AdvControl(PluginNumber, ACTL_GETWINDOWINFO, FADV1988 &wi);
	if (wi.Type != WTYPE_EDITOR)
	{
		_ASSERTE(wi.Type != WTYPE_EDITOR);
		goto wrap;
	}
    
	iRc = EditCtrl(ECTL_GETINFO, &ei);
	if (!iRc)
	{
		_ASSERTE(iRc!=0);
		goto wrap;
	}
	
	if (gpnWrappedEditors)
	{
		
		for (size_t i = 0; i < gnWrappedEditors; i++)
		{
			if (gpnWrappedEditors[i] == ei.EditorID)
			{
				pWrappedEditor = gpnWrappedEditors+i;
				break;
			}
			if (!pnFreeEditorId && gpnWrappedEditors[i] == -1)
			{
				pnFreeEditorId = gpnWrappedEditors+i;
			}
		}
	}
	gbLastWrap = (pWrappedEditor != NULL);
	if (!pWrappedEditor)
	{
		if (pnFreeEditorId)
		{
			pWrappedEditor = pnFreeEditorId;
		}
		else
		{
			size_t nNewCount = gnWrappedEditors + 128;
			int *pnNew = (int*)malloc(nNewCount*sizeof(*pnNew));
			if (pnNew)
			{
				if (gpnWrappedEditors && gnWrappedEditors)
					memmove(pnNew, gpnWrappedEditors, gnWrappedEditors*sizeof(*pnNew));
				if (gpnWrappedEditors)
					free(gpnWrappedEditors);
				for (size_t k = gnWrappedEditors; k < nNewCount; k++)
					pnNew[k] = -1;
				gpnWrappedEditors = pnNew;
				pWrappedEditor = gpnWrappedEditors + gnWrappedEditors;
				gnWrappedEditors = nNewCount;
			}
			else
			{
				_ASSERTE(pnNew!=NULL);
			}
		}
	}
	
	
	// "callplugin/Plugin.Call"?
	#ifdef _UNICODE
	if ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO)
	{
		#if FAR_UNICODE>=2184
		FoldWorkMode fw = fwm_None;
		OpenMacroInfo* p = (OpenMacroInfo*)Item;
		if (p->StructSize >= sizeof(*p) && p->Count)
		{
			if (p->Values[0].Type == FMVT_INTEGER)
				fw = (FoldWorkMode)(int)p->Values[0].Integer;
			#if FAR_UNICODE>=3000
			else if (p->Values[0].Type == FMVT_DOUBLE)
				fw = (FoldWorkMode)(int)p->Values[0].Double;
			#endif
			if (p->Count > 1)
			{
				if (p->Values[1].Type == FMVT_INTEGER)
					iRightPad = (int)p->Values[1].Integer;
				#if FAR_UNICODE>=3000
				else if (p->Values[1].Type == FMVT_DOUBLE)
					iRightPad = (int)p->Values[1].Double;
				#endif
				bRightPadSpecified = true;
			}
		}
		Item = fw;
		#endif

		if (Item >= fwm_ToggleWrap && Item <= fwm_Unwrap)
		{
			WorkMode = (FoldWorkMode)Item;
		}
	}
	#endif

	// Показать меню: что делать
	if (WorkMode == fwm_None)
		WorkMode = ChooseWorkMode();
	if (WorkMode == fwm_None)
	{
		goto wrap;
	}

	if (WorkMode == fwm_ToggleWrap)
		WorkMode = gbLastWrap ? fwm_Unwrap : fwm_Wrap;
	else if (WorkMode == fwm_ToggleWordWrap)
		WorkMode = gbLastWrap ? fwm_Unwrap : fwm_WordWrap;

	// Подготовка места под скролл и отступ
	if (WorkMode == fwm_Wrap || WorkMode == fwm_WordWrap)
	{
		#if FAR_UNICODE>3000
		if (!bRightPadSpecified)
		{
			// Нет способа проверить через API, включена ли прокрутка в конкретном экземпляре редактора
			wchar_t szMacroRepost[100];
			wsprintf(szMacroRepost, L"Plugin.Call(\"%s\",%i,Editor.Set(15,-1))", guid_PluginGuidS, (int)WorkMode);
			MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
			mcr.SequenceText = szMacroRepost;
			psi.MacroControl(&guid_PluginGuid, MCTL_SENDSTRING, 0, &mcr);
			goto wrap;
		}
		#endif

		// Откусывание места по скролл или отступ от правого края
		if ((iRightPad > 0) && (iRightPad <= 10) && (iRightPad < iMaxWidth))
		{
			iMaxWidth -= iRightPad;
		}
	}

	ur.Command = EUR_BEGIN;
	EditCtrl(ECTL_UNDOREDO, &ur);

	if (WorkMode == fwm_Wrap || WorkMode == fwm_WordWrap)
	{
		DoWrap((WorkMode == fwm_WordWrap), ei, iMaxWidth);
		if (pWrappedEditor)
			*pWrappedEditor = ei.EditorID;
	}
	else if (WorkMode == fwm_Unwrap)
	{
		DoUnwrap(ei);
		if (pWrappedEditor)
			*pWrappedEditor = -1;
	}
	else
	{
		_ASSERTE(WorkMode == fwm_Unwrap || WorkMode == fwm_Wrap || WorkMode == fwm_WordWrap);
	}
	
wrap:
	if (ur.Command == EUR_BEGIN)
	{
		ur.Command = EUR_END;
		EditCtrl(ECTL_UNDOREDO, &ur);
	}

	return FAR_PANEL_NONE;
}
