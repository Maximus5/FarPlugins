
#include "header.h"
#include "RE_Guids.h"

/*

	Отображение прогресса операций

*/

REProgress *gpProgress = NULL;
CScreenRestore *gpAction = NULL;
REProgress::REProgress(const TCHAR* aszTitle, BOOL abGraphic /*= FALSE*/, const TCHAR* aszFileInfo /*= NULL*/)
	: CScreenRestore(NULL, aszTitle)
{
	_ASSERTE(gpProgress == NULL);
	bEscaped = FALSE;
	bGraphic = abGraphic;
	nAllCount = nCurrent = 0;
	psPercentTitle = psLastTitle = NULL;
	#ifdef _UNICODE
	cFull = 0x2588; cHollow = 0x2591;
	#else
	cFull = '\xB0'; cHollow = '\xDB';
	#endif
	nProgressLen = countof(sProgress)-1;
	if (bGraphic)
	{
		for (int i = 0; i < nProgressLen; i++)
			sProgress[i] = cHollow;
		sProgress[nProgressLen] = 0;
		pszFormat = NULL;
		psLastTitle = (TCHAR*)malloc(2048*sizeof(TCHAR));
		if (!GetConsoleTitle(psLastTitle, 2048)) {
			SafeFree(psLastTitle);
		} else if (aszTitle) {
			int nLen = lstrlen(aszTitle);
			psPercentTitle = (TCHAR*)malloc((nLen+64)*sizeof(TCHAR));
			wsprintf(psPercentTitle, _T("{  0%%} %s"), aszTitle);
			SetConsoleTitle(psPercentTitle);
		}
	} else {
		pszFormat = GetMsg(REExportItemsProgress);
		wsprintf(sProgress, pszFormat, 0, 1);
	}
	nStepDuration = 1000; nCounter = 0; sFileInfo[0] = 0;
	if (aszFileInfo) lstrcpyn(sFileInfo, aszFileInfo, countof(sFileInfo));
	nStartTick = GetTickCount();
	Update(/*(aszFileInfo && aszFileInfo[0]) ? aszFileInfo : NULL*/);
}
REProgress::REProgress(const TCHAR* aszMsg, const TCHAR* aszTitle)
	: CScreenRestore(aszMsg, aszTitle)
{
	_ASSERTE(gpProgress == NULL);
	bEscaped = FALSE;
	bGraphic = FALSE;
	nAllCount = nCurrent = 0;
	psPercentTitle = psLastTitle = NULL;
	nProgressLen = countof(sProgress)-1;
	psLastTitle = (TCHAR*)malloc(2048*sizeof(TCHAR));
	if (!GetConsoleTitle(psLastTitle, 2048)) {
		SafeFree(psLastTitle);
	} else if (aszTitle) {
		SetConsoleTitle(aszTitle);
	}
	nStartTick = GetTickCount();
}
REProgress::~REProgress()
{
	Restore();
	if (bEscaped)
		REPlugin::Message(REM_OperationCancelledByUser);
	if (bGraphic) {
		#ifdef _UNICODE
		#if FARMANAGERVERSION_BUILD>=2570
		psi.AdvControl(PluginNumber, ACTL_SETPROGRESSSTATE, FADV1988 (void*)TBPS_NOPROGRESS);
		#else
		psi.AdvControl(PluginNumber, ACTL_SETPROGRESSSTATE, FADV1988 (void*)PS_NOPROGRESS);
		#endif
		#endif
	}
	if (psLastTitle) {
		SetConsoleTitle(psLastTitle);
	}
	SafeFree(psLastTitle);
	SafeFree(psPercentTitle);
}
BOOL REProgress::CheckForEsc(BOOL abForceRead /*= FALSE*/)
{
	if (bEscaped)
		return TRUE;
	if (!abForceRead)
		return FALSE;

	INPUT_RECORD *InputRec;
	DWORD NumberOfEvents, ReadCnt,i;
	HANDLE Console = GetStdHandle(STD_INPUT_HANDLE);

	if(GetNumberOfConsoleInputEvents(Console,&NumberOfEvents) && NumberOfEvents)
	{
		if((InputRec=(INPUT_RECORD *)calloc(NumberOfEvents,sizeof(INPUT_RECORD))) != NULL)
		{
			if(PeekConsoleInput(Console,InputRec,NumberOfEvents,&ReadCnt))
			{
				i=0;
				while (i < ReadCnt)
				{
					if (InputRec[i].EventType == KEY_EVENT
						&& InputRec[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
					{
						BOOL lbKeyDown = InputRec[i].Event.KeyEvent.bKeyDown;
						while (((i+1) < ReadCnt)
							&& InputRec[i+1].EventType == KEY_EVENT
							&& InputRec[i+1].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
						{
							if (!lbKeyDown && InputRec[i].Event.KeyEvent.bKeyDown)
								lbKeyDown = TRUE;
							i++;
						}
						// Останов - только если кнопка была НАЖАТА, а не "отпущена"
						if (lbKeyDown)
						{
							// Подтвердить останов
							bEscaped = (psi.Message(_PluginNumber(guid_ConfirmCancel),FMSG_WARNING|FMSG_ALLINONE,NULL,
								(const TCHAR * const *)GetMsg(REM_ConfirmOperationCancel),0,2) == 0);
							if (!PeekConsoleInput(Console,InputRec,NumberOfEvents,&ReadCnt))
								ReadCnt = 0;
						}
						else
						{
							ReadCnt = i+1;
						}
						// После отображенного диалога - событий в буфере может уже и не быть
						//ReadConsoleInput(Console,InputRec,i+1,&ReadCnt);
						if(ReadCnt)
						{
							//if (InputRec[0].EventType == KEY_EVENT && InputRec[0].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
							{
								ReadConsoleInput(Console,InputRec,ReadCnt,&ReadCnt);
							}
						}
						break;
					}
					i++;
				}
			}
			SafeFree(InputRec);
		}
	}

	return bEscaped;
}
void REProgress::Update(/*LPCTSTR asFileName = NULL*/)
{
	// hScreen == NULL, если FAR в режиме DisableScreenOutput
	if (hScreen != NULL)
	{
		if (bGraphic)
		{
			_ASSERTE(hScreen!=NULL);
			int nCount = 0, nPercent = 0;
			if (nAllCount > 0 && nCurrent > 0) {
				if (nCurrent >= nAllCount) {
					_ASSERTE(nCurrent <= nAllCount);
					nCount = nProgressLen;
					nPercent = 100;
				} else {
					nCount = (int)((nCurrent * nProgressLen) / nAllCount);
					if (nCount < 0) {
						_ASSERTE(nCount >= 0);
						nCount = 0;
					}
					nPercent = (int)((nCurrent * 100) / nAllCount);
					if (nPercent < 0) nPercent = 0;
				}
			}
			for (int i = 0; i < nCount; i++) sProgress[i] = cFull;
			for (int i = nCount; i < nProgressLen; i++) sProgress[i] = cHollow;
			sProgress[nProgressLen] = 0;

			#ifdef _UNICODE
			#if FAR_UNICODE>=1900
			#define PROGRESSVALUE ProgressValue
			#endif
			PROGRESSVALUE pv = {};
			#if FAR_UNICODE>=3000
			pv.StructSize = sizeof(pv);
			#endif
			pv.Completed = nCurrent;
			pv.Total = nAllCount;
			#if FARMANAGERVERSION_BUILD>=2570
			psi.AdvControl(PluginNumber, ACTL_SETPROGRESSSTATE, FADV1988 (void*)TBPS_NORMAL);
			#else
			psi.AdvControl(PluginNumber, ACTL_SETPROGRESSSTATE, FADV1988 (void*)PS_NORMAL);
			#endif
			psi.AdvControl(PluginNumber, ACTL_SETPROGRESSVALUE, FADV1988 (void*)&pv);
			#endif

			if (sFileInfo[0])
			{
				pszMsg = sProgress;
				const TCHAR *MsgItems[]={pszTitle,sProgress,sFileInfo};
				psi.Message(_PluginNumber(guid_PluginGuid),FMSG_LEFTALIGN,NULL,MsgItems,sizeof(MsgItems)/sizeof(MsgItems[0]),0);
			}
			else
			{
				Message(sProgress, FALSE);
			}

			if (!psPercentTitle) {
				_ASSERTE(psPercentTitle!=NULL);
			} else {
				TCHAR sDur[32]; sDur[0] = 0;
				DWORD nDelta = (GetTickCount() - nStartTick) / 1000;
				//if (nDelta && nPercent) {
					//unsigned __int64 nAllDuration = ((unsigned __int64)nElapsed) * 100 / nPercent;
					//nDelta = (DWORD)((nAllDuration - nElapsed) / 1000);

				DWORD nHours = nDelta / 3600;
				nDelta %= 3600;
				DWORD nMinutes = nDelta / 60;
				nDelta %= 60;
				DWORD nSeconds = nDelta;

				wsprintf(sDur, _T("  %i:%02i:%02i"), nHours, nMinutes, nSeconds);
				//}

				wsprintf(psPercentTitle, _T("{%3i%%%s} %s"), nPercent, sDur, pszTitle);
				SetConsoleTitle(psPercentTitle);
			}

		} else {
			wsprintf(sProgress, pszFormat, (DWORD)nCurrent, (DWORD)nAllCount);
			Message(sProgress, FALSE);
		}
		
		//TODO: Обновить заголовок консоли - процентики показать
		nLastTick = GetTickCount();
		nCounter = 0;
	}

	// Проверить, не была ли нажата клавиша Esc?
	CheckForEsc(TRUE);
}
BOOL REProgress::IncAll(size_t anAddAllCount /*= 1*/)
{
	nAllCount += anAddAllCount;
	if ((GetTickCount() - nLastTick) >= nStepDuration)
		Update();
	return (!bEscaped);
}
BOOL REProgress::Step(size_t anProcessed /*= 1*/)
{
	nCurrent += anProcessed;
	if ((GetTickCount() - nLastTick) >= nStepDuration)
		Update();
	return (!bEscaped);
}
// Absolute size
BOOL REProgress::SetAll(unsigned __int64 anAllCount)
{
	nAllCount = anAllCount;
	if ((GetTickCount() - nLastTick) >= nStepDuration)
		Update();
	return (!bEscaped);
}
BOOL REProgress::SetStep(unsigned __int64 anAllStep, BOOL abForce /*= FALSE*/, LPCWSTR asFileName /*= NULL*/)
{
	nCurrent = anAllStep; nCounter++;

	//if (asFileName) {
	//	int nLen = lstrlen(asFileName), nSize = countof(sFileInfo);
	//	if (nLen >= nSize) {
	//		lstrcpy_t(sFileInfo, nSize-3, asFileName);
	//		lstrcat(sFileInfo, _T("..."));
	//	} else {
	//		lstrcpy_t(sFileInfo, nSize, asFileName);
	//	}
	//}

	if (abForce
		|| (/*nCounter >= 10 &&*/ (GetTickCount() - nLastTick) >= nStepDuration)
		)
	{
		if (asFileName && *asFileName)
		{
			int nLen = lstrlenW(asFileName), nSize = countof(sFileInfo);
			if (nLen >= nSize)
			{
				lstrcpy_t(sFileInfo, nSize-3, asFileName);
				lstrcat(sFileInfo, _T("..."));
			}
			else
			{
				lstrcpy_t(sFileInfo, nSize, asFileName);
			}
		}
		else if (sFileInfo[0])
		{
			// Иначе на экране появляются неприятные артефакты от удаленной строки
			sFileInfo[0] = _T(' '); sFileInfo[1] = 0;
		}

		Update(/*sFileInfo[0] ? sFileInfo : NULL*/);
	}
	return (!bEscaped);
}

void REProgress::ShowDuration()
{
	DWORD nDelta = GetTickCount() - nStartTick;
	//nDelta /= 1000;
	DWORD nHours = nDelta / 3600000;
	nDelta %= 3600000;
	DWORD nMinutes = nDelta / 60000;
	nDelta %= 60000;
	DWORD nSeconds = nDelta / 1000;
	nDelta %= 1000;
	
	TCHAR sDur[32];
	wsprintf(sDur, _T("%i:%02i:%02i.%03i"), nHours, nMinutes, nSeconds, nDelta);
	REPlugin::MessageFmt(REM_Duration, sDur, 0, 0, FMSG_MB_OK);
}
