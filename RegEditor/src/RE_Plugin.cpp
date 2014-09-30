
#include "header.h"
// Helpers
#include "TokenHelper.h"
#include <Tlhelp32.h>
#include <commctrl.h>
// Guids
#include "RE_Guids.h"
#include "../../common/DlgBuilder.hpp"

#include "Hooks/RegEditExe.h"

//#define EXECUTE_TRAP

REPlugin *gpActivePlugin = NULL; // Устанавливается на время вызова из ФАР функций нашего плагина
u64 gnOpMode = 0;


//TODO: поскольку теперь RegFolder может использоваться в более чем одном экземпляре плагина
//нужно обеспечить блокировку(?) или уведомление об обновлении, или автосоздание копии при
//перечитывании ключа. Иначе у фара может сорвать крышу?
//Хотя фар копирует данные себе в структуры, так что собственно наши указатели у него
//постоянно не используются. Так что достаточно только обновить соответствующие панели при изменениях.

REPlugin::REPlugin()
{
	gpPluginList->Register(this);

	REGFILETIME ft; SYSTEMTIME st;
	GetSystemTime(&st); SystemTimeToFileTime(&st, (PFILETIME)&ft);
	
	// Изначально - пустые
	mp_Worker = NULL;
	ms_RemoteServer[0] = 0; ms_RemoteLogin[0] = 0; //= ms_RemotePassword[0] = 0;
	mb_RemoteMode = false;
	mpsz_HostFile = NULL;
	mb_InMacroEdit = FALSE;
	mn_ActivatedCount = 0;
	mb_FindInContents = -1;
	mb_ShowRegFileMenu = FALSE;
	mb_Wow64on32 = cfg->getWow64on32();
	mb_Virtualize = FALSE;

	// Буфер для отображения "измененности" панели
	mn_PanelTitleMax = 0; mpsz_PanelTitle = NULL;
	
	// путь и тип
	memset(&m_Key, 0, sizeof(m_Key)); // First entry - zero memory
	m_Key.Init(RE_WINAPI, mb_Wow64on32, mb_Virtualize);

	// элементы
	//m_Items.Init(&m_Key);
	//2010-07-17 пока не будем создавать, может и не нужно будет (например, только импорт реестра, или еще что)
	//mp_Items = Worker()->GetFolder(&m_Key);
	mp_Items = NULL;
	//memset(&m_TempItems, 0, sizeof(m_TempItems));
	
	// other Instance variables
	mb_ForceReload = FALSE;
	//mb_EnableKeyMonitoring = FALSE;
	mb_SortingSelfDisabled = false;
	mb_InitialReverseSort = false;
	mn_InitialSortMode = SM_NAME;
	LoadCurrentSortMode(&mn_InitialSortMode, &mb_InitialReverseSort);
}

REPlugin::~REPlugin()
{
	m_Key.Release();
	//m_Items.FinalRelease();
	//mp_Items = NULL;
	CloseItems();
	//m_TempItems.FinalRelease();
	
	// Если больше нет ни одного экземпляра - будет выход
	gpPluginList->Unregister(this);
	
	// Удалить
	SafeDelete(mp_Worker);
	
	//...
	SafeFree(mpsz_HostFile);
	SafeFree(mpsz_PanelTitle);
}

DWORD REPlugin::getWow64on32()
{
	if (!this)
	{
		InvalidOp();
		return cfg->getWow64on32();
	}
	return mb_Wow64on32;
}

void REPlugin::setWow64on32(DWORD abWow64on32)
{
	if (!this)
	{
		InvalidOp();
		return;
	}
	mb_Wow64on32 = abWow64on32;
	if (mp_Worker)
		mp_Worker->mb_Wow64on32 = abWow64on32;
	m_Key.mb_Wow64on32 = abWow64on32;
	m_Key.Update();
	CloseItems();
	mb_ForceReload = TRUE;
}

DWORD REPlugin::getVirtualize()
{
	if (!this)
	{
		InvalidOp();
		return FALSE;
	}
	return mb_Virtualize;
}

void REPlugin::setVirtualize(DWORD abVirtualize)
{
	if (!this)
	{
		InvalidOp();
		return;
	}
	mb_Virtualize = abVirtualize;
	if (mp_Worker)
		mp_Worker->mb_Virtualize = abVirtualize;
	m_Key.mb_Virtualize = abVirtualize;
	m_Key.Update();
	CloseItems();
	mb_ForceReload = TRUE;
}

// asKey может быть
// "\\" -- перейти в m_Key.mh_Root=NULL (список из HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, и т.п.)
// ".." -- перейти на уровень вверх
// "."  -- ничего не делать
// <KeyName> - перейти в под-ключ. проверяется существование ключа
BOOL REPlugin::ChDir(LPCWSTR asKey, BOOL abSilence)
{
	// Слеш может быть только один и только когда он единвстенный символ
	if (asKey[0] == L'\\')
	{
		if (asKey[1] != 0) {
			_ASSERTE(asKey[0] == L'\\' && asKey[1] == 0);
			return FALSE;
		}
	}
	else if (wcschr(asKey, L'\\'))
	{
		_ASSERTE(wcschr(asKey, L'\\') == NULL);
		return FALSE;
	}

	CloseItems();
	
	return m_Key.ChDir(asKey, abSilence, this);
}

BOOL REPlugin::SubKeyExists(LPCWSTR asSubkey)
{
	BOOL lbSuccess = FALSE;
	HREGKEY hTest = NULLHKEY, hSub = NULLHKEY;

	if ((m_Key.mh_Root || m_Key.eType == RE_HIVE) && asSubkey && *asSubkey)
	{
		if (Worker())
		{
			lbSuccess = (mp_Worker->ExistKey(m_Key.mh_Root, m_Key.mpsz_Key, asSubkey) == 0);
		}
	}
	
	return lbSuccess;
}

BOOL REPlugin::CheckKeyAvailable(RegPath* apKey, BOOL abSilence /*= FALSE*/)
{
	BOOL lbSuccess = TRUE;
	HREGKEY hTest = NULLHKEY;

	_ASSERTE(apKey->eRights != eRightsNoAdjust);
	apKey->eRights = eRightsSimple;

	if ((apKey->mh_Root || apKey->eType == RE_HIVE) && apKey->mpsz_Key && *apKey->mpsz_Key)
	{
		if (Worker() == NULL)
		{
			_ASSERTE(mp_Worker != NULL);
			lbSuccess = FALSE;
		}
		else
		{
			if (0 != mp_Worker->OpenKeyEx(apKey->mh_Root, apKey->mpsz_Key, 0, KEY_READ|KEY_WRITE, &hTest, &apKey->nKeyFlags, &apKey->eRights) != 0)
			{
				if (0 != mp_Worker->OpenKeyEx(apKey->mh_Root, apKey->mpsz_Key, 0, KEY_READ, &hTest, &apKey->nKeyFlags, &apKey->eRights) != 0)
				{
					if (!abSilence)
						CantOpenKey(&m_Key, FALSE);
					lbSuccess = FALSE;
				}
			}
			
			if (hTest)
			{
				mp_Worker->CloseKey(&hTest);
			}
		}
	}
	
	if (lbSuccess)
	{
		apKey->Update(); // Показать в заголовке, с какими правами открыли ключ
	}
	
	return lbSuccess;
}

LPCTSTR REPlugin::GetPanelTitle()
{
	if (!this)
		return _T("");

	LPCTSTR pszTitle = m_Key.mpsz_Title ? m_Key.mpsz_Title : _T("");

	if (mp_Worker && mp_Worker->bDirty && cfg->sRegTitleDirty[0])
	{
		// Нужно прилепить к заголовку панели метку " (*)" - *.reg файл был изменен
		int nLen = lstrlen(pszTitle);
		int nAllLen = nLen + cfg->TitleAddLen();
		if (nAllLen >= mn_PanelTitleMax || !mpsz_PanelTitle)
		{
			mn_PanelTitleMax = nAllLen + MAX_PATH;
			if (mpsz_PanelTitle) SafeFree(mpsz_PanelTitle);
			mpsz_PanelTitle = (TCHAR*)calloc(nAllLen,sizeof(TCHAR));
		}
		lstrcpy(mpsz_PanelTitle, pszTitle);
		lstrcpy(mpsz_PanelTitle+nLen, cfg->sRegTitleDirty);

		pszTitle = mpsz_PanelTitle;
	}

	return pszTitle;
}

BOOL REPlugin::SetDirectory(RegItem* pKey, BOOL abSilence)
{
	if (!pKey || !pKey->pszName || (pKey->nFlags & REGF_DELETED))
		return FALSE;

	//LPCWSTR pszSlash = NULL;
	//size_t nTokenLen = 0;
	//HREGKEY hTest = NULL;

	BOOL lbSuccess = TRUE;

	pKey->pFolder->AddRef();

	// Сохранить текущий ключ, чтобы вернуть его, если не получится сменить папку
	RegPath LastKey = {RE_UNDEFINED};
	LastKey.Init(&m_Key);

	MCHKHEAP;

	CloseItems();

	lbSuccess = m_Key.ChDir(pKey, abSilence, this);
	if (!lbSuccess) {
		_ASSERTE(lbSuccess);
		lbSuccess = FALSE;
		goto wrap;
	}
	MCHKHEAP;

	_ASSERTE(lbSuccess);
	
	lbSuccess = CheckKeyAvailable(&m_Key);

	//if ((m_Key.mh_Root || m_Key.eType == RE_HIVE) && m_Key.mpsz_Key && *m_Key.mpsz_Key)
	//{
	//	if (m_Key.eRights == eRightsNoAdjust) {
	//		_ASSERTE(m_Key.eRights != eRightsNoAdjust);
	//		m_Key.eRights = eRightsSimple;
	//	}
	//	
	//	if (Worker() == NULL)
	//	{
	//		_ASSERTE(mp_Worker != NULL);
	//		lbSuccess = FALSE;
	//	}
	//	else if (mp_Worker->OpenKeyEx(m_Key.mh_Root, m_Key.mpsz_Key, 0, KEY_READ, &hTest, &m_Key.eRights) != 0)
	//	{
	//		CantOpenKey(&m_Key, FALSE);
	//		lbSuccess = FALSE;
	//	} else {
	//		m_Key.Update(); // Показать в заголовке, с какими правами открыли ключ
	//		mp_Worker->CloseKey(hTest); hTest = NULL;
	//	}
	//}

	if (lbSuccess && !abSilence)
	{
		if (m_Key.eType == RE_WINAPI && !mb_RemoteMode)
		{
			MCHKHEAP;
			cfg->SetLastRegPath(m_Key.mh_Root, m_Key.mpsz_Key);
			MCHKHEAP;
		}
	}

	MCHKHEAP;

wrap:
	if (!lbSuccess)
	{
		CloseItems();
		m_Key.Release();
		m_Key.Init(&LastKey);
	}
	pKey->pFolder->Release();
	return lbSuccess;
}

BOOL REPlugin::SetDirectory(LPCWSTR Dir, BOOL abSilence)
{
	wchar_t szSubKey[MAX_PATH+1]; _ASSERTE(MAX_REGKEY_NAME <= MAX_PATH);
	LPCWSTR pszSlash = NULL;
	size_t nTokenLen = 0;
	//HREGKEY hTest = NULL;

	BOOL lbSuccess = TRUE;
	//BOOL lbNeedClosePlugin = FALSE;
	BOOL lbMultiDir = (wcschr(Dir, L'\\') != NULL);

	// Сохранить текущий ключ, чтобы вернуть его, если не получится сменить папку
	RegPath LastKey = {RE_UNDEFINED};
	LastKey.Init(&m_Key);

	// Сбросим наверное сразу, чтобы потом не было проблем с незакрытыми элементами
	// при сбросе Worker'а.
	CloseItems();

	MCHKHEAP;

	if (Dir[0] == L'\\' && Dir[1] == L'\\')
	{
		// Переход к сетевому реестру
		if (m_Key.eType != RE_WINAPI)
		{
			SafeDelete(mp_Worker);
			m_Key.Release();
			m_Key.Init(RE_WINAPI, mb_Wow64on32, mb_Virtualize);
		}
		Dir += 2;
		if (Dir[0] == 0)
		{
			// Local
			mb_RemoteMode = FALSE;
			ms_RemoteServer[0] = 0;
			if (Worker() == NULL)
				goto wrap;
			mp_Worker->ConnectLocal();
			m_Key.SetTitlePrefix((LPCTSTR)NULL, TRUE);
			m_Key.SetServer((LPCTSTR)NULL);
		}
		else
		{
			// Remote Server
			LPCWSTR pszSlash = wcschr(Dir, L'\\');
			if (!pszSlash) pszSlash = Dir + lstrlenW(Dir);
			size_t nLen = pszSlash - Dir;
			if (nLen >= (countof(ms_RemoteServer)-3)) nLen = countof(ms_RemoteServer)-3;
			ms_RemoteServer[0] = ms_RemoteServer[1] = L'\\';
			wmemmove(ms_RemoteServer+2, Dir, nLen);
			ms_RemoteServer[nLen+2] = 0;
			mb_RemoteMode = true;
			Dir = pszSlash;

			SafeDelete(mp_Worker);
			if (Worker() == NULL)
				goto wrap;
			// Т.к. mp_Worker только что был удален - то Worker() уже создал подключение
			//mp_Worker->ConnectRemote(ms_RemoteServer);
			_ASSERTE(mp_Worker->eType == m_Key.eType && mp_Worker->bRemote == mb_RemoteMode && mb_RemoteMode == true);
			_ASSERTE(0 == lstrcmpiW(mp_Worker->sRemoteServer,ms_RemoteServer));

			m_Key.SetTitlePrefix((LPCTSTR)NULL, TRUE);
			m_Key.SetServer(ms_RemoteServer);

			//ChDir(L"\\", TRUE); -- переходить в корень - не будем, может быть удобно прыгать по серверам.
		}
	}

	if (Dir[0] == L'\\')
	{
		//lbNeedClosePlugin = (m_Key.mh_Root == NULL && !(m_Key.mpsz_Key && *m_Key.mpsz_Key));
		if (!ChDir(L"\\", abSilence))
		{
			_ASSERTE(FALSE);
			lbSuccess = FALSE;
			goto wrap;
		}
		Dir++;
	}

	MCHKHEAP;

	if (*Dir)
	{
		m_Key.AllocateAddLen(lstrlenW(Dir));
	}

	MCHKHEAP;

	


	// Поехали. Поддерживаются относительные пути.
	while (*Dir)
	{
		//TODO: Потенциальная засада - в принципе, ключи(? значения-точно могут) могут содержать обратные слеши..
		pszSlash = wcschr(Dir, L'\\');
		if (!pszSlash) pszSlash = Dir + lstrlenW(Dir); // последний токен
		nTokenLen = pszSlash - Dir;
		if (nTokenLen > MAX_REGKEY_NAME) // длина имени ключа не может быть более MAX_REGKEY_NAME
		{
			_ASSERTE(nTokenLen <= MAX_REGKEY_NAME);
			if (!abSilence) KeyNameTooLong(Dir);
			lbSuccess = FALSE;
			goto wrap;
		}
		MCHKHEAP;


		//TODO: В ANSI версии из-за потерь конвертации Unicode->Oem ключ мог быть испорчен.
		//TODO: хорошо бы в этом случае выбрать ключ из существующих?

		wmemmove(szSubKey, Dir, (int)nTokenLen);
		szSubKey[nTokenLen] = 0;
		MCHKHEAP;

		//if (szSubKey[0] == L'.' && szSubKey[1] == L'.' && szSubKey[2] == 0)
		//{
		//	lbNeedClosePlugin = (m_Key.mh_Root == NULL && !(m_Key.mpsz_Key && *m_Key.mpsz_Key));
		//}
		//else
		//{
		//	lbNeedClosePlugin = FALSE;
		//}

		// Apply token to plugin (always UNICODE!)
		if (!ChDir(szSubKey, abSilence))
		{
			//_ASSERTE(FALSE);
			lbSuccess = FALSE;
			goto wrap;
		}
		MCHKHEAP;

		// Next token
		if (*pszSlash == _T('\\'))
		{
			Dir = pszSlash+1;
		}
		else
		{
			_ASSERTE(*pszSlash == 0);
			break;
		}
	}
	MCHKHEAP;

	_ASSERTE(lbSuccess);

	//if (lbSuccess && lbNeedClosePlugin)
	//{
	//	psiControl((HANDLE)this, FCTL_CLOSEPLUGIN, F757NA 0);
	//	goto wrap;
	//}
	
	if (lbMultiDir && !(m_Key.mpsz_Key && *m_Key.mpsz_Key))
		lbMultiDir = FALSE;

	// Проверяем наличие (доступность)
	lbSuccess = CheckKeyAvailable(&m_Key, abSilence);
	
	if (lbMultiDir && !lbSuccess)
	{
		// Значит был запрос вида cd "HKLM\Software\Far\Macro\xxx", и если "xxx"
		// не найден - попробовать открыть один из родительских ключей.
		// Идем с конца, т.к. часть родительских ключей может быть недоступна, 
		// при доступности дочерних.
		while (*m_Key.mpsz_Key)
		{
			if (!ChDir(L"..", TRUE/*abSilence*/))
				goto wrap;
			if (CheckKeyAvailable(&m_Key, TRUE/*abSilence*/))
			{
				lbSuccess = TRUE;
				break; // Нашли доступный ключ
			}
		}
	}

	//if (m_Key.mh_Root && m_Key.mpsz_Key && *m_Key.mpsz_Key)
	//{
	//	if (Worker() == NULL)
	//	{
	//		_ASSERTE(mp_Worker != NULL);
	//		lbSuccess = FALSE;
	//	}
	//	else if (mp_Worker->OpenKeyEx(m_Key.mh_Root, m_Key.mpsz_Key, 0, KEY_READ, &hTest) != 0)
	//	{
	//		CantOpenKey(&m_Key, FALSE);
	//		lbSuccess = FALSE;
	//	} else {
	//		mp_Worker->CloseKey(hTest); hTest = NULL;
	//	}
	//}

	if (lbSuccess && !abSilence)
	{
		if (m_Key.eType == RE_WINAPI && !mb_RemoteMode)
		{
			MCHKHEAP;
			cfg->SetLastRegPath(m_Key.mh_Root, m_Key.mpsz_Key);
			MCHKHEAP;
		}
	}

	MCHKHEAP;

wrap:
	if (!lbSuccess)
	{
		CloseItems();
		m_Key.Release();
		m_Key.Init(&LastKey);
	}
	LastKey.Release();
	return lbSuccess;
}

void REPlugin::KeyNotExist(RegPath* pKey, LPCTSTR asSubKey)
{
	// Ругнуться, что ключ asSubKey не существует в m_Key.mh_Root\m_Key.mpsz_Key
	const TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg(REM_KeyNotExists),
		asSubKey,
		GetMsg(REM_CurrentPath),
		(pKey->mpsz_Dir && *pKey->mpsz_Dir) ? pKey->mpsz_Dir : _T("\\")
	};
	psi.Message(_PluginNumber(guid_Msg1), FMSG_WARNING|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}

#ifndef _UNICODE
void REPlugin::KeyNotExist(RegPath* pKey, LPCWSTR asSubKey)
{
	// Ругнуться, что ключ asSubKey не существует в m_Key.mh_Root\m_Key.mpsz_Key
	TCHAR  sKeyName[0x4000]; lstrcpy_t(sKeyName, countof(sKeyName), asSubKey);
	KeyNotExist(pKey, sKeyName);
}
#endif

void REPlugin::KeyNameTooLong(LPCWSTR asSubKey)
{
	// Ругнуться, что ключ asSubKey не существует в m_Key.mh_Root\m_Key.mpsz_Key
	TCHAR sSubKey[512]; lstrcpy_t(sSubKey, countof(sSubKey), asSubKey);
	const TCHAR* sLines[] =
	{
		GetMsg(REPluginName),
		GetMsg(REM_KeyNameTooLong),
		sSubKey,
	};
	psi.Message(_PluginNumber(guid_Msg2), FMSG_WARNING|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}

void REPlugin::ValueNotExist(RegPath* pKey, LPCWSTR asSubKey)
{
	TCHAR  sValueName[0x4000]; lstrcpy_t(sValueName, countof(sValueName), asSubKey);
	TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		sValueName,
		GetMsg(REM_ValueNotExists),
		pKey->mpsz_Dir
	};
	psi.Message(_PluginNumber(guid_Msg3), FMSG_WARNING|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}

BOOL REPlugin::ValueOperationFailed(RegPath* pKey, LPCWSTR asValueName, BOOL abModify, BOOL abAllowContinue)
{
	if (cfg->bSkipAccessDeniedMessage)
		return FALSE; // молча - Cancel

	TCHAR  sValueName[0x4000];
	lstrcpy_t(sValueName, countof(sValueName), (asValueName && *asValueName) ? asValueName : REGEDIT_DEFAULTNAME);
	TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg(abModify ? REM_CantOpenValueWrite : REM_CantOpenValueRead),
		sValueName,
		pKey->mpsz_Dir,
		GetMsg(REBtnContinue),
		GetMsg(REBtnCancel),
	};
	int nLines = countof(sLines), nBtns = 2;
	if (!abAllowContinue)
	{
		nLines--; nBtns--;
		sLines[nLines-1] = GetMsg(REBtnOK);
	}
	int nRc = psi.Message(_PluginNumber(guid_Msg4), FMSG_WARNING|FMSG_ERRORTYPE, NULL, 
		sLines, nLines, nBtns);
	return (abAllowContinue && (nRc == 0));
}

BOOL REPlugin::DeleteFailed(RegPath* pKey, LPCWSTR asName, BOOL abKey, BOOL abAllowContinue)
{
	if (cfg->bSkipAccessDeniedMessage)
		return TRUE; // молча - Continue

	TCHAR  sValueName[0x4000]; lstrcpy_t(sValueName, countof(sValueName), asName);
	TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg(abKey ? REM_CantDeleteKey : REM_CantDeleteValue),
		sValueName,
		pKey->mpsz_Dir,
		GetMsg(REBtnContinue),
		GetMsg(REBtnCancel),
	};
	int nLines = countof(sLines), nBtns = 2;
	if (!abAllowContinue)
	{
		nLines--; nBtns--;
		sLines[nLines-1] = GetMsg(REBtnOK);
	}
	int nRc = psi.Message(_PluginNumber(guid_Msg5), FMSG_WARNING|FMSG_ERRORTYPE, NULL, 
		sLines, nLines, nBtns);
	return (abAllowContinue && (nRc == 0));
}

//// 1 - continue, 0 - break to editor, -1 - stop
//int REPlugin::RegFormatFailed(LPCTSTR asFile, DWORD nLine, DWORD nCol)
//{
//	TCHAR  sLineInfo[128];
//	wsprintf(sLineInfo, GetMsg(REFormatFailLineInfo), (nLine+1), (nCol+1));
//	
//	const TCHAR* sLines[] = 
//	{
//		GetMsg(REPluginName),
//		GetMsg(REFormatFailLabel),
//		asFile ? asFile : _T(""),
//		sLineInfo,
//		GetMsg(REBtnContinue),
//		GetMsg(REBtnEditor),
//		GetMsg(REBtnCancel),
//	};
//	int nBtn = psi.Message(PluginNumber, FMSG_WARNING, NULL, 
//		sLines, countof(sLines), 3);
//	if (nBtn == 0)
//		return 1;
//	else if (nBtn == 1)
//		return 0;
//	return -1;
//}

// 0-read, 1-write, -1-delete
void REPlugin::CantOpenKey(RegPath* pKey, int abModify)
{
	if (gnOpMode & (OPM_SILENT|OPM_FIND))
		return;
	if (cfg->bSkipAccessDeniedMessage)
		return;

	TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg((abModify==1) ? REM_CantOpenKeyWrite : ((abModify==0) ? REM_CantOpenKeyRead : REM_CantOpenKeyDelete)),
		pKey->mpsz_Dir
	};
	psi.Message(_PluginNumber(guid_Msg6), FMSG_WARNING|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}

void REPlugin::CantOpenKey(RegPath* pKey, LPCWSTR asSubKey, BOOL abModify)
{
	if (cfg->bSkipAccessDeniedMessage)
		return;

	TCHAR  sSubkeyName[0x4000]; lstrcpy_t(sSubkeyName, countof(sSubkeyName), asSubKey);
	TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		sSubkeyName,
		GetMsg(abModify ? REM_CantOpenKeyWrite : REM_CantOpenKeyRead),
		pKey->mpsz_Dir
	};
	psi.Message(_PluginNumber(guid_Msg7), FMSG_WARNING|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}

void REPlugin::CantLoadSaveKey(LPCWSTR asSubKey, LPCTSTR asFile, BOOL abSave)
{
	TCHAR  sSubkeyName[0x4000]; lstrcpy_t(sSubkeyName, countof(sSubkeyName), asSubKey);
	const TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg((abSave == 2) ? REM_CantRenameKey : (abSave ? REM_CantSaveKey : REM_CantLoadKey)),
		sSubkeyName,
		asFile
	};
	psi.Message(_PluginNumber(guid_Msg8), FMSG_WARNING|FMSG_ERRORTYPE|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}
#ifndef _UNICODE
void REPlugin::CantLoadSaveKey(LPCWSTR asSubKey, LPCWSTR asFile, BOOL abSave)
{
	TCHAR* pszFile = lstrdup_t(asFile);
	CantLoadSaveKey(asSubKey, pszFile, abSave);
	SafeFree(pszFile);
}
#endif

void REPlugin::CloseItems()
{
	if (mp_Items)
	{
		mp_Items->Release();
	}
	mp_Items = NULL;
}

BOOL REPlugin::AllowCacheFolder()
{
	//TODO: Хорошо бы не кешировать, но нужно освобождать память.
	//  а из-за рекурсии в Far.Find элементы должны жить...
	//if (gnOpMode & (OPM_FIND))
	//	return FALSE;

	// Чтобы в кеш не попадали папки ключей, полученные из 
	// *.reg файлов, кустов, удаленных серверов и прочее
	// Любое различие (другой файл, сервер, логин) приводит
	// к некорректности сохраненных данных!
	if (m_Key.eType == RE_WINAPI && !mb_RemoteMode)
		return TRUE;

	return FALSE;
}

// В некоторых случаях (например при хождении по панели с включенным QView)
// после SetDirectory не выполняется GetFindDataW, в результате теряется mp_Items (== NULL)
void REPlugin::CheckItemsLoaded()
{
	if ((mp_Items == NULL) || mb_ForceReload)
	{
		LoadItems(FALSE, 0);
	}
}

// Загрузить (если были изменения) список подключей и значений
//WARNING: В режиме OPM_FIND нафиг не нужно загружать описания! И кешировать тоже бы не нужно!
//Но так просто не получится. При рекурсивном поиске наступает облом, т.к. элементы меняются по указателям.
BOOL REPlugin::LoadItems(BOOL abSilence, u64 OpMode)
{
	BOOL lbForceReload = FALSE;
	//mb_EnableKeyMonitoring = !abSilence;

	if ((OpMode & OPM_FIND) == 0)
		mb_FindInContents = -1;
	else if (mb_FindInContents == -1)
		mb_FindInContents = 0; // Установится в "1" при первом запросе содержимого файла при поиске


	if (mp_Items)
	{
		bool lbEqual = m_Key.IsEqual(&(mp_Items->key));
		if (!lbEqual)
		{
			_ASSERTE(lbEqual);
			CloseItems();
			mb_ForceReload = TRUE;
		}
	}

	MRegistryBase* pWorker = Worker();
	if (!pWorker)
	{
		InvalidOp();
		return FALSE;
	}
	
	
	if (!mp_Items)
	{
		// Чтобы в кеш не попадали папки ключей, полученные из 
		// *.reg файлов, кустов, удаленных серверов и прочее
		// Любое различие (другой файл, сервер, логин) приводит
		// к некорректности сохраненных данных!
		//if (m_Key.eType == RE_WINAPI && !mb_RemoteMode)
		//if (AllowCacheFolder())
		//{
		mp_Items = pWorker->GetFolder(&m_Key, OpMode);
		if (!mp_Items)
		{
			_ASSERTE(mp_Items!=NULL);
			InvalidOp();
			return FALSE;
		}
		//} else {
		//	mp_Items = &m_TempItems;
		//	mp_Items->key.Init(m_Key.eType, m_Key.mh_Root, m_Key.mpsz_Key, m_Key.ftModified);
		//	mp_Items->Reset();
		//}
		// Чтобы была произведена проверка на изменения, без учета таймаутов
		// (при заходе в папку нужны строго актуальные данные)
		mp_Items->mb_ForceRegistryCheck = TRUE;
	}

	#ifdef _DEBUG
	bool lbEqual;
	{
		lbEqual = m_Key.IsEqual(&(mp_Items->key));
		_ASSERTE(lbEqual);
	}
	#endif
	

	if (mb_ForceReload)
	{
		lbForceReload = TRUE;
		mb_ForceReload = FALSE;
	}

	m_Key.Update();
	mp_Items->key.Update();

	//if (!lbForceReload && mp_Items && mp_Items->mp_Items && cfg->bRefreshChanges) {
	//	if (mp_Items->CheckRegistryChanged(this)) {
	//		lbForceReload = TRUE;
	//	}
	//}
	
	
	BOOL lbLoadDesc = (cfg->bLoadDescriptions && !abSilence);
	
	BOOL lbRc = mp_Items->LoadKey(this, pWorker, eKeysFirst, lbForceReload, abSilence, lbLoadDesc);

	#ifdef _DEBUG
	{
		lbEqual = m_Key.IsEqual(&(mp_Items->key));
		_ASSERTE(lbEqual);
	}
	#endif

	if (mp_Items->bShowKeysAsDirs != cfg->bShowKeysAsDirs)
	{
		mp_Items->bShowKeysAsDirs = cfg->bShowKeysAsDirs;
		if (cfg->bShowKeysAsDirs)
		{
			for (UINT i = 0; i < mp_Items->mn_ItemCount; i++)
			{
				PanelItemAttributes(mp_Items->mp_PluginItems[i]) = 
					(mp_Items->mp_Items[i].nValueType == REG__KEY)
						? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
			}
		} else {
			for (UINT i = 0; i < mp_Items->mn_ItemCount; i++)
			{
				PanelItemAttributes(mp_Items->mp_PluginItems[i]) = FILE_ATTRIBUTE_NORMAL;
			}
		}
	}

	m_Key.ftModified = mp_Items->key.ftModified;

	if (!abSilence)
	{
		if (cfg->bUnsortLargeKeys && cfg->nLargeKeyCount && mp_Items->mn_ItemCount >= cfg->nLargeKeyCount) {
			// Сброс сортировки
			ChangeFarSorting(true);
		}
	}
	
	return lbRc;
}

void REPlugin::FreeFindData(struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	if (!mp_Worker) {
		InvalidOp();
		return;
	}
	// Он сам разберется - нужно освобождать, или оно кешированное
	mp_Worker->FreeFindData(PanelItem,ItemsNumber);
}

void REPlugin::SetForceReload()
{
	mb_ForceReload = TRUE;
}

void REPlugin::EditKeyPermissions(BOOL abVisual)
{
	//TODO: Hive?
	if (m_Key.eType != RE_WINAPI)
		return;
	
	RegItem* pItem = GetCurrentItem();
	if (pItem == NULL && m_Key.mh_Root == NULL)
		return; // самый корень плагина, выделен ".."
	
	//HREGKEY hkRoot = m_Key.mh_Root;
	//if (m_Key.mh_Root == NULL) {
	//	if (!StringKeyToHKey(pItem->pszName, &hkRoot)) {
	//		_ASSERTE(FALSE);
	//		return;
	//	}
	//}
	//
	//wchar_t* pszKeyPath = NULL;
	//int nFullLen = 0, nKeyLen = 0, nSubkeyLen = 0;
	//if (m_Key.mpsz_Key && *m_Key.mpsz_Key)
	//	nFullLen += (nKeyLen = lstrlenW(m_Key.mpsz_Key));
	//if (m_Key.mh_Root != NULL && pItem && pItem->nValueType == REG__KEY)
	//	nFullLen += (nSubkeyLen = lstrlenW(pItem->pszName));
	//
	//pszKeyPath = (wchar_t*)malloc((nFullLen+3)*2);
	//pszKeyPath[0] = 0;
	//if (nKeyLen > 0) {
	//	lstrcpyW(pszKeyPath, m_Key.mpsz_Key);
	//	if (nSubkeyLen > 0) {
	//		pszKeyPath[nKeyLen] = L'\\';
	//		lstrcpyW(pszKeyPath+nKeyLen+1, pItem->pszName);
	//	}
	//} else if (nSubkeyLen > 0 && m_Key.mh_Root != NULL) {
	//	lstrcpyW(pszKeyPath, pItem->pszName);
	//}
	
	MRegistryBase *pWorker = Worker();
	if (pWorker)
	{
		pWorker->EditKeyPermissions(&m_Key, pItem, abVisual);
	}
	else
	{
		_ASSERTE(pWorker!=NULL);
	}
	
	//SafeFree(pszKeyPath);
	
	//// Чтобы кейбар обновился, а то он остается в режиме CtrlShift
	//UpdatePanel(false);
	//RedrawPanel(NULL);
}

// *ppPluginPaneItem - указатель на mp_Items->mp_PluginItems[i]
RegItem* REPlugin::GetCurrentItem(const PluginPanelItem** ppPluginPaneItem /*= NULL*/)
{
	if (ppPluginPaneItem) *ppPluginPaneItem = NULL;

	if (!mp_Items)
		return NULL;
	if (mp_Items->mn_ItemCount < 1)
		return NULL;

	RegItem* pItem = NULL;
	PanelInfo inf = {sizeof(inf)};
	INT_PTR nCurLen = 0;
	PluginPanelItem* item=NULL;

	if (psiControl((HANDLE)this, FCTL_GETPANELINFO, F757NA &inf))
	{
		if (inf.ItemsNumber>0 && inf.CurrentItem>0)
		{
			#ifdef _UNICODE
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, NULL);
			item = (PluginPanelItem*)calloc(nCurLen,1);
			#if FAR_UNICODE>=3000
			FarGetPluginPanelItem gppi = {sizeof(gppi), nCurLen, item};
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (FarDlgProcParam2)&gppi);
			#elif FAR_UNICODE>=1906
			FarGetPluginPanelItem gppi = {nCurLen, item};
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (FarDlgProcParam2)&gppi);
			#else
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (FarDlgProcParam2)item);
			#endif
			if (!nCurLen)
			{
				SafeFree(item);
				return NULL;
			}
			#else
			item = &(inf.PanelItems[inf.CurrentItem]);
			#endif
			
			pItem = (RegItem*)PanelItemUserData(*item);
			if (pItem && pItem->nMagic != REGEDIT_MAGIC) {
				_ASSERTE(pItem->nMagic == REGEDIT_MAGIC);
				pItem = NULL;
			}
		}
	}

#ifdef _UNICODE
	SafeFree(item);
#endif

	if (pItem)
	{
		//Так нельзя. при поиске. Используется один плаг, но mp_Items - разные!
		//size_t nIdx = pItem - mp_Items->mp_Items;
		DWORD nIdx = pItem->nItemIndex;
		if (pItem->pFolder == NULL || nIdx >= pItem->pFolder->mn_ItemCount)
		{
			InvalidOp();
			pItem = NULL;
		}
		else if (ppPluginPaneItem)
		{
			*ppPluginPaneItem = pItem->pFolder->mp_PluginItems+nIdx;
		}
	}

	return pItem;
};

MRegistryBase* REPlugin::Worker()
{
	//TODO: Сделать несколько типов mp_Worker, чтобы не разрушать их, а хранить постоянное, на время жизни плагина
	MCHKHEAP;
	if (mp_Worker)
	{
		// В плагине сменился тип
		if (mp_Worker->eType != m_Key.eType || mp_Worker->bRemote != mb_RemoteMode
			|| (mp_Worker->bRemote
				&& 0!=lstrcmpiW(mp_Worker->sRemoteServer,ms_RemoteServer)))
				//&& 0!=lstrcmpiW(mp_Worker->sRemoteLogin,ms_RemoteLogin)))
		{
			CloseItems();
			SafeDelete(mp_Worker);
		}
	}
	if (!mp_Worker)
	{
		if (m_Key.eType != RE_WINAPI)
		{
			_ASSERTE(m_Key.eType == RE_WINAPI);
			InvalidOp();
			return NULL;
		}
		
		mp_Worker = new MRegistryWinApi(mb_Wow64on32, mb_Virtualize);
		if (mb_RemoteMode)
			mp_Worker->ConnectRemote(ms_RemoteServer);
		//mp_Worker = MRegistryBase::CreateWorker(m_Key.eType, mb_RemoteMode, ms_RemoteServer, ms_RemoteLogin, ms_RemotePassword);
		//if (!mp_Worker)
		//	InvalidOp();
	}
	return mp_Worker;
}

void REPlugin::LoadCurrentSortMode(int *pnSortMode, bool *pbReverseSort)
{
	PanelInfo pi = {sizeof(pi)};
	if (!psiControl(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, F757NA &pi))
		return;
	*pnSortMode = pi.SortMode;
	*pbReverseSort = (pi.Flags & PFLAGS_REVERSESORTORDER) == PFLAGS_REVERSESORTORDER;
}

void REPlugin::ChangeFarSorting(bool abFastAccess)
{
	int  nNewSortMode = abFastAccess ? SM_UNSORTED : mn_InitialSortMode;
	//int  nNewNumeric = 0;
	int  nNewReverse = mb_InitialReverseSort ? 1 : 0;

	mb_SortingSelfDisabled = abFastAccess;

	#ifdef _UNICODE
		//psiControl((HANDLE)this, FCTL_SETNUMERICSORT, nNewNumeric, NULL);
		psiControl((HANDLE)this, FCTL_SETSORTMODE, nNewSortMode, NULL);
		psiControl((HANDLE)this, FCTL_SETSORTORDER, nNewReverse, NULL);
	#else
		//TODO: FAR 1.75 валится, аналогично тому, что было в 2.0
		PanelRedrawInfo pri = {0,0};
		psiControl((HANDLE)this, FCTL_REDRAWPANEL, &pri);
		//psiControl((HANDLE)this, FCTL_SETNUMERICSORT, &nNewNumeric);
		psiControl((HANDLE)this, FCTL_SETSORTMODE, &nNewSortMode);
		psiControl((HANDLE)this, FCTL_SETSORTORDER, &nNewReverse);
	#endif
}

// Показать диалог создания нового значения
void REPlugin::NewItem()
{
	MCHKHEAP;
	//if (m_Key.eType != RE_WINAPI
	//	|| m_Key.mh_Root == NULL)
	if ((m_Key.mh_Root == NULL) && (m_Key.eType != RE_REGFILE))
		return; // В этих случаях - низя
	if (!mp_Items || !mp_Items->key.IsEqual(&m_Key))
	{
		_ASSERTE(mp_Items && mp_Items->key.IsEqual(&m_Key));
		return;
	}

	//RegItem* pItem = NULL;

	PluginDialogBuilder val(psi, RENewValueTitle, _T("NewValue"), &guid_NewValue);
	FarDialogItem* p = NULL;

	TCHAR sValueName[16384]; lstrcpy(sValueName, GetMsg(RENewValueName));

	val.AddText(REValueNameLabel);
	p = val.AddEditField(sValueName, countof(sValueName), 56, _T("RegistryValueName"));
	val.InitFocus(p);

	val.AddSeparator();
	REGTYPE nValueType = 0;
	int nValueTypeBtn = 0;
	int nTypes[] = {
		RE_REG_SZ,
		RE_REG_EXPAND_SZ,
		RE_REG_MULTI_SZ,
		RE_REG_BINARY,
		RE_REG_DWORD,
		RE_REG_QWORD,
	};
	val.AddRadioButtons(&nValueTypeBtn, countof(nTypes), nTypes, 0, TRUE);

	//TODO: Добавить кнопку "&Edit" для создания значения через редактор *.reg
	val.AddOKCancel(REBtnOK, REBtnCancel);
	BOOL lbOk = val.ShowDialog();
	if (!lbOk || nValueTypeBtn < 0 || nValueTypeBtn >= (int)countof(nTypes))
		return;
	MCHKHEAP;

	if (sValueName[0] == 0)
		nValueTypeBtn = 0; // Значение по умолчанию всегда REG_SZ

	// Получить в nValueType ИД типа реестра (сейчас в nValueTypeBtn индекс кнопки)
	if (!RegTypeMsgId2RegType(nTypes[nValueTypeBtn], &nValueType))
	{
		_ASSERTE(RegTypeMsgId2RegType(nTypes[nValueTypeBtn], &nValueType));
		return;
	}
	MCHKHEAP;

	// Запускаем редактирование
	wchar_t wsValueName[16384]; lstrcpy_t(wsValueName, countof(wsValueName), sValueName);
	RegItem item = {REGEDIT_MAGIC, nValueType, wsValueName, (sValueName[0] == 0)};
	// Панель обновит сама EditValue
	EditValue(mp_Items, &item, nValueType);
	//if (EditValue(mp_Items, &item, nValueType) == 1)
	//{
	//	// UpdatePanel & позиционировать курсор на созданный элемент
	//	SetForceReload();
	//	UpdatePanel(false);
	//	RedrawPanel(&item); // Аргумент - имя элемента (точнее, ссылка на RegItem)
	//}

	MCHKHEAP;
}

void REPlugin::RenameOrCopyItem(BOOL abCopyOnly /*= FALSE*/)
{
	MCHKHEAP;
	if ((m_Key.eType != RE_WINAPI && m_Key.eType != RE_REGFILE && m_Key.eType != RE_HIVE)
		|| (m_Key.eType == RE_WINAPI && m_Key.mh_Root == NULL))
		return; // В этих случаях - низя
	if (!mp_Items || !mp_Items->key.IsEqual(&m_Key))
	{
		_ASSERTE(mp_Items);
		_ASSERTE(!mp_Items || mp_Items->key.IsEqual(&m_Key));
		return;
	}

	RegItem* pItem = GetCurrentItem();
	if (!pItem)
		return; // Требуется текущий элемент
	if (pItem->nValueType != REG__KEY && pItem->bDefaultValue)
	{
		// "RegEditor\nCan't rename default value!"
		Message(REM_CantRenameDefaultValue);
		return;
	}
	

	MCHKHEAP;
	PluginDialogBuilder val(psi, 
		(pItem->nValueType==REG__KEY) ?
			(abCopyOnly ? RECopyKeyTitle : RERenameKeyTitle) :
			(abCopyOnly ? RECopyValueTitle : RERenameValueTitle), 
		(pItem->nValueType==REG__KEY) ? _T("RenameKey") : _T("RenameValue"),
		(pItem->nValueType==REG__KEY) ? &guid_RenameKey : &guid_RenameValue);
	FarDialogItem* p = NULL;
	LONG hRc;
	int nValueType = -1;
	int nTypes[] = {
		RE_REG_NONE,
		RE_REG_SZ,
		RE_REG_EXPAND_SZ,
		RE_REG_MULTI_SZ,
		RE_REG_LINK,
		RE_REG_FULL_RESOURCE_DESCRIPTOR,
		RE_REG_BINARY,
		RE_REG_DWORD,
		RE_REG_DWORD_BIG_ENDIAN,
		RE_REG_RESOURCE_LIST,
		RE_REG_RESOURCE_REQUIREMENTS_LIST,
		RE_REG_QWORD,
	};
	REGTYPE nDataType;
	DWORD nDataSize;
	MRegistryBase* pWorker = Worker();
	TCHAR sNewName[16384];
	wchar_t wsNewName[16384];
	
	// Проверить наличие значения/подключа
	if (pItem->nValueType == REG__KEY)
	{
		hRc = pWorker->ExistKey(m_Key.mh_Root, m_Key.mpsz_Key, pItem->pszName);
		if (hRc != 0) KeyNotExist(&m_Key, pItem->pszName);
	} else {
		hRc = pWorker->ExistValue(m_Key.mh_Root, m_Key.mpsz_Key, pItem->pszName, &nDataType, &nDataSize);
		if (hRc != 0) ValueNotExist(&m_Key, pItem->pszName);
	}
	if (hRc != 0)
		return;
	MCHKHEAP;
	

	// Добавить в диалог имя значения/ключа
	lstrcpy_t(sNewName, countof(sNewName), pItem->pszName);
	val.AddText((pItem->nValueType==REG__KEY) ? RERenameKeyLabel : RERenameValueLabel);
	p = val.AddEditField(sNewName, countof(sNewName), 56+14, (pItem->nValueType == REG__KEY) ? _T("RegistrySubkeyName") : _T("RegistryValueName"));
	val.InitFocus(p);

	BOOL bDontChangeType = FALSE;
	if (pItem->nValueType != REG__KEY)
	{
		val.AddSeparator();
		if (nDataType > REG_QWORD) {
			//nDataType = REG_NONE;
			bDontChangeType = TRUE;
		}
		for (UINT n = 0; n < countof(nTypes); n++) {
			if ((DWORD)(nTypes[n] - RE_REG_NONE) == nDataType) {
				nValueType = n; break;
			}
		}
		//nValueType = nDataType;
		val.AddRadioButtons(&nValueType, countof(nTypes), nTypes, false, bDontChangeType ? DIF_DISABLE : 0, TRUE);
	}

	//TODO: Добавить кнопку "&Edit" для создания значения через редактор *.reg
	val.AddOKCancel(REBtnOK, REBtnCancel);
	if (!val.ShowDialog())
		return;
	MCHKHEAP;
	if (pItem->nValueType != REG__KEY && !bDontChangeType)
	{
		// Получить в nValueType ИД типа реестра (сейчас в нем индекс кнопки)
		if (nValueType < 0 || nValueType > REG_QWORD)
		{
			_ASSERTE(nValueType >= 0 && nValueType <= REG_QWORD);
			return;
		}
	}
	REGTYPE nNewDataType = nDataType;
	if (!bDontChangeType && nValueType >= 0 && nValueType < (int)countof(nTypes))
		if (!RegTypeMsgId2RegType(nTypes[nValueType], &nNewDataType))
			nNewDataType = nDataType;

	if (!*sNewName)
	{
		Message( (pItem->nValueType == REG__KEY) ? REM_EmptyKeyNameNotAllowed : REM_EmptyValueNameNotAllowed);
		return;
	}
	
	lstrcpy_t(wsNewName, countof(wsNewName), sNewName);
	// Сравниваем регистрозависимо, разрешим изменение регистра для ключей/значений
	if (lstrcmpW(wsNewName, pItem->pszName) == 0)
	{
		// Имя не изменилось, может изменился тип?
		if (pItem->nValueType != REG__KEY) {
			if (pItem->nValueType == nNewDataType)
				return; // Nothing to do
		} else {
			return; // Nothing to do
		}
	}
	MCHKHEAP;
	

	// Проверить наличие значения/подключа с новым именем, только если имя новое (регистроНЕзависимо)
	if (lstrcmpiW(wsNewName, pItem->pszName) != 0)
	{
		if (pItem->nValueType == REG__KEY)
		{
			hRc = pWorker->ExistKey(m_Key.mh_Root, m_Key.mpsz_Key, wsNewName);
		} else {
			hRc = pWorker->ExistValue(m_Key.mh_Root, m_Key.mpsz_Key, wsNewName);
		}
		if (hRc == 0)
		{
			// Запросить строгое подтверждение на переименование
			TCHAR* pszLines[10]; int nLines = 0;
			pszLines[nLines++] = GetMsg(REM_Warning);
			pszLines[nLines++] = sNewName;
			pszLines[nLines++] = GetMsg((pItem->nValueType == REG__KEY) ? REM_KeyAlreadyExists : REM_ValueAlreadyExists);
			pszLines[nLines++] = GetMsg((pItem->nValueType == REG__KEY) ? REM_KeyWillBeErased  : REM_ValueWillBeErased);
			pszLines[nLines++] = GetMsg(REBtnContinue);
			pszLines[nLines++] = GetMsg(REBtnCancel);
			int iBtn = psi.Message(_PluginNumber(guid_Msg9), FMSG_WARNING, _T("RenameKey"), pszLines, nLines, 2);
			if (iBtn != 0)
				return;
		}
	}

	// Собственно переименование
	BOOL lbChanged = FALSE;
	MCHKHEAP;
	if (pItem->nValueType == REG__KEY)
	{
		pWorker->RenameKey(&mp_Items->key, abCopyOnly, pItem->pszName, wsNewName, &lbChanged);

	} else {
		HREGKEY hKey = NULLHKEY;
		LPBYTE pData = (LPBYTE)malloc(nDataSize);
		DWORD nSize = nDataSize;
		LPCWSTR pszComment = NULL;
		MCHKHEAP;
		if ((hRc = mp_Items->OpenKey(pWorker, &hKey, KEY_ALL_ACCESS)) != 0)
			CantOpenKey(&mp_Items->key, TRUE);
		// Считать текущее значение
		if (hRc == 0 && (hRc = pWorker->QueryValueEx(hKey, pItem->pszName, NULL, NULL, pData, &nSize, &pszComment)) != 0)
			ValueOperationFailed(&mp_Items->key, pItem->pszName, FALSE/*abModify*/);
		// придется удалить старое значение, если различие только в регистре
		if (hRc == 0 && lstrcmpiW(wsNewName, pItem->pszName) == 0 && lstrcmpW(wsNewName, pItem->pszName) != 0) {
			if ((hRc = pWorker->DeleteValue(hKey, pItem->pszName)) != 0)
				ValueOperationFailed(&mp_Items->key, pItem->pszName, TRUE/*abModify*/);
		}
		// Записать под новым именем и, возвожно, с новым типом
		// !!! Warning !!! Преобразование типа (REG_MULTI_SZ <--> REG_SZ) не выполняется! То есть в строку REG_SZ попадут \0, а в REG_MULTI_SZ не будет разбивки на строки
		if (hRc == 0 && (hRc = pWorker->SetValueEx(hKey, wsNewName, 0, nNewDataType, pData, nSize, pszComment)) != 0)
			ValueOperationFailed(&mp_Items->key, wsNewName, TRUE/*abModify*/);
		else
			lbChanged = TRUE;
		// Удалить старое значение, если оно различается регистроНЕзависимо
		if (!abCopyOnly)
		{
			if (hRc == 0 && lstrcmpiW(wsNewName, pItem->pszName) != 0) {
				if ((hRc = pWorker->DeleteValue(hKey, pItem->pszName)) != 0)
					ValueOperationFailed(&mp_Items->key, pItem->pszName, TRUE/*abModify*/);
			}
		}
		// Закрыть ключ
		if (hKey)
			mp_Items->CloseKey(pWorker, &hKey);
		MCHKHEAP;
	}
	
	if (lbChanged) {
		// UpdatePanel & позиционировать курсор на новый элемент
		RegItem item = {REGEDIT_MAGIC, nValueType, wsNewName, pItem->bDefaultValue};
		SetForceReload();
		UpdatePanel(false);
		RedrawPanel(&item); // Аргумент - имя элемента (точнее, ссылка на RegItem)
	}
	MCHKHEAP;
}

BOOL REPlugin::ChooseImportDataType(LPCWSTR asName, REGTYPE* pnDataType, BOOL* pbUnicodeStrings, BOOL* pbForAll)
{
	MCHKHEAP;
	PluginDialogBuilder val(psi, REImportValueTitle, _T("ImportValue"), &guid_ImportValue);
	FarDialogItem* p = NULL;
	int nID = 0;
	int nValueType = 1;
	int nTypes[] = {
		RE_REG_NONE,
		RE_REG_SZ,
		RE_REG_EXPAND_SZ,
		RE_REG_MULTI_SZ,
		RE_REG_LINK,
		RE_REG_FULL_RESOURCE_DESCRIPTOR,
		RE_REG_BINARY,
		RE_REG_DWORD,
		RE_REG_DWORD_BIG_ENDIAN,
		RE_REG_QWORD,
		RE_REG_RESOURCE_LIST,
		RE_REG_RESOURCE_REQUIREMENTS_LIST,
	};
	REGTYPE nDataType = *pnDataType;
	MRegistryBase* pWorker = Worker();
	TCHAR sNewName[MAX_PATH+1];

	for (UINT n = 0; n < countof(nTypes); n++)
	{
		if ((DWORD)(nTypes[n] - RE_REG_NONE) == nDataType)
		{
			nValueType = n; break;
		}
	}

	// Добавить в диалог имя значения/ключа
	lstrcpy_t(sNewName, countof(sNewName), asName);
	val.AddText(REImportValueLabel);
	val.AddText(sNewName);

	val.AddSeparator();
	nID = val.AddRadioButtons(&nValueType, countof(nTypes), nTypes, 0, TRUE);
	val.InitFocus(val.GetItemByIndex(nID+nValueType));

	val.AddSeparator();
	if (pbForAll)
		val.StartColumns();
	val.AddCheckbox(REImportUnicodeStrings, pbUnicodeStrings);
	if (pbForAll)
	{
		val.ColumnBreak();
		val.AddCheckbox(REImportForAll, pbForAll);
		val.EndColumns();
	}

	val.AddOKCancel(REBtnOK, REBtnCancel);
	if (!val.ShowDialog())
		return FALSE;
	//MCHKHEAP;
	//if (pItem->nValueType != REG__KEY && !bDontChangeType)
	//{
	//	// Получить в nValueType ИД типа реестра (сейчас в нем индекс кнопки)
	//	if (nValueType < 0 || nValueType > REG_QWORD)
	//	{
	//		_ASSERTE(nValueType >= 0 && nValueType <= REG_QWORD);
	//		return FALSE;
	//	}
	//}
	if (nValueType >= 0 && nValueType < (int)countof(nTypes)
		&& RegTypeMsgId2RegType(nTypes[nValueType], pnDataType))
	{
		return TRUE;
	}
	return FALSE;
}

// Выбрать тип импорта reg-файла в панель плагина
// Например, на панели открыт [HKEY_CURRENT_USER\Software\111]
// F5 на reg-е содержащем [HKEY_CURRENT_USER\Software\222\FM] что с ним делать:
// ris_Here: создать [HKEY_CURRENT_USER\Software\111\FM]
// ris_ValuesHere: значения и подключи из [HKEY_CURRENT_USER\Software\222\FM] положить в [HKEY_CURRENT_USER\Software\111]
// ris_Native, ris_Import32, ris_Import64: импорт как есть, в корень реестра (с делением на 32/64 битный реестр)
BOOL REPlugin::ChooseImportStyle(LPCWSTR asName, LPCWSTR asFromKey, DWORD anAllowed/*bitmask of RegImportStyle*/,
								 RegImportStyle& rnImportStyle, BOOL* pbForAll)
{
	MCHKHEAP;
	PluginDialogBuilder val(psi, REImportStyleKeyTitle, _T("ImportKey"), &guid_ImportKey);
	FarDialogItem* p = NULL;
	int nID = 0;

	int nImportStyle = 0;
	RegImportStyle nStylesId[] = {ris_Here, ris_ValuesHere, ris_AsRaw, ris_Native, ris_Import32, ris_Import64};
	DWORD nStylesMsg[] = {REActionImportHere, REActionImportValuesHere, REActionImportRegAsRaw, REActionImport, REActionImport32, REActionImport64};
	int nStyles[10] = {};
	int nStylesCount = 0;
	for (UINT n = 0; n < countof(nStylesId); n++)
	{
		if ((anAllowed & nStylesId[n]))
		{
			if (rnImportStyle == nStylesId[n])
				nImportStyle = nStylesCount;
			nStyles[nStylesCount++] = nStylesMsg[n];
		}
	}

	TCHAR sFileName[MAX_PATH+1], sKeyName[MAX_PATH+1];

	// Добавить в диалог имя файла/ключа
	if (asName)
	{
		lstrcpy_t(sFileName, countof(sFileName), asName);
		p = val.AddEditField(sFileName, countof(sFileName), 40);
		p->Flags |= DIF_READONLY;
		val.AddTextBefore(p, REImportStyleFileLabel);
	}
	if (asFromKey)
	{
		lstrcpy_t(sKeyName, countof(sKeyName), asFromKey);
		p = val.AddEditField(sKeyName, countof(sKeyName), 40);
		p->Flags |= DIF_READONLY;
		val.AddTextBefore(p, REImportStyleKeyLabel);
	}

	if (asName || asFromKey)
		val.AddSeparator();

	nID = val.AddRadioButtons(&nImportStyle, nStylesCount, nStyles, 0, FALSE);
	val.InitFocus(val.GetItemByIndex(nID+nImportStyle));

	if (pbForAll)
	{
		val.AddSeparator();
		val.AddCheckbox(REImportForAll, pbForAll);
	}

	val.AddOKCancel(REBtnOK, REBtnCancel);
	if (!val.ShowDialog())
		return FALSE;
	if (nImportStyle < 0 || nImportStyle >= nStylesCount)
	{
		InvalidOp();
		return FALSE;
	}

	for (int n = 0; n < nStylesCount; n++)
	{
		if (nStyles[nImportStyle] == nStylesMsg[n])
		{
			rnImportStyle = nStylesId[n];
			return TRUE;
		}
	}

	InvalidOp();
	return FALSE;
}

BOOL REPlugin::ConfirmExport(
		BOOL abAllowHives, int* pnExportMode, BOOL* pbHiveAsSubkey, int ItemsNumber,
		LPCTSTR asDestPathOrFile, BOOL abDestFileSpecified, LPCTSTR asNameForLabel,
		wchar_t** ppwszDestPath, wchar_t* sDefaultName/*[MAX_PATH]*/, wchar_t* wsDefaultExt/*[5]*/,
		int nTitleMsgId /*= REExportDlgTitle*/, int nExportLabelMsgId /* = (ItemsNumber == 1) ? REExportItemLabel : REExportItemsLabel*/,
		int nCancelMsgId /*= REBtnCancel*/
		)
{
	// Диалог подтверждения экспорта
	BOOL lbExportRc = FALSE;

	/*
	г=============================== Copy ===============================¬
	¦ Copy "Far2" to                                                     ¦
	¦ T:\VCProject\FarPlugin\RegEditor\_Test\Far2.reg                   v¦
	¦--------------------------------------------------------------------¦
	¦ Export keys as                                                     ¦
	¦ ( ) REGEDIT4 files                                                 ¦
	¦ (•) REGEDIT5 (Unicode) files                                       ¦
	¦ ( ) Binary files ("hives")                                         ¦
	¦--------------------------------------------------------------------¦
	¦                         { OK } [ Cancel ]                          ¦
	L====================================================================-	
	*/

	PluginDialogBuilder cpy(psi, nTitleMsgId, _T("ExportKey"), &guid_ExportKey);
	FarDialogItem* p = NULL; //, *p1, *p2;
	int nLen;
	TCHAR sLabel[67];
	BOOL bUnicodeRawStrings = TRUE;
	#ifndef _UNICODE
	bUnicodeRawStrings = FALSE;
	#endif
	LPCTSTR pszFormat = GetMsg(nExportLabelMsgId);
	nLen = lstrlen(pszFormat);
	if (nLen >= 40)
	{
		InvalidOp();
		lbExportRc = FALSE; goto wrap;
	}

	int nMaxNameLen = countof(sLabel) - 4 - nLen;
	TCHAR sPart[61];
	_ASSERTE(nMaxNameLen < countof(sPart));

	nLen = lstrlen(asNameForLabel);
	if (nLen > nMaxNameLen)
	{
		#ifdef _UNICODE
		lstrcpyn(sPart, asNameForLabel, nMaxNameLen-1);
		lstrcat(sPart, L"\x2026");
		#else
		lstrcpyn(sPart, asNameForLabel, nMaxNameLen-3);
		lstrcat(sPart, "...");
		#endif
	}
	else
	{
		lstrcpyn(sPart, asNameForLabel, nMaxNameLen);
	}
	if (ItemsNumber == 1)
	{
		wsprintf(sLabel, pszFormat, sPart);
	}
	else
	{
		wsprintf(sLabel, pszFormat, ItemsNumber, sPart);
	}


	nLen = lstrlen(asDestPathOrFile);
	if ((size_t)nLen >= (countof(ms_CreatedSubkeyBuffer)-2-MAX_PATH))
	{
		InvalidOp();
		lbExportRc = FALSE; goto wrap;
	}
	lstrcpy(ms_CreatedSubkeyBuffer, asDestPathOrFile);
	if (!abDestFileSpecified && ms_CreatedSubkeyBuffer[nLen-1] != _T('\\'))
	{
		ms_CreatedSubkeyBuffer[nLen++] = _T('\\');
		ms_CreatedSubkeyBuffer[nLen] = 0;
	}

	cpy.AddText(sLabel);
	p = cpy.AddEditField(ms_CreatedSubkeyBuffer, countof(ms_CreatedSubkeyBuffer), 66, _T("RegistryExportPath"));
	cpy.InitFocus(p);

	cpy.AddSeparator(REExportFormatLabel);
	int nExportTypes[] = {REExportReg4, REExportReg5, REExportCmd, REExportHive, REExportRaw};
	int nRadio = cpy.AddRadioButtons(pnExportMode, countof(nExportTypes), nExportTypes, 0, TRUE);
	if (!abAllowHives)
	{
		// Сюда мы попадаем если сохраняется существующий reg-файл
		cpy.GetItemByIndex(nRadio+(REExportCmd-REExportReg4))->Flags |= DIF_DISABLE;
		cpy.GetItemByIndex(nRadio+(REExportHive-REExportReg4))->Flags |= DIF_DISABLE;
		cpy.GetItemByIndex(nRadio+(REExportRaw-REExportReg4))->Flags |= DIF_DISABLE;
	}
	cpy.AddSeparator();
	cpy.StartColumns();
	p = cpy.AddCheckbox(REExportRawUnicode, &bUnicodeRawStrings);
	if (!abAllowHives)
		p->Flags |= DIF_DISABLE;
	cpy.ColumnBreak();
	BOOL lbDummy1 = FALSE;
	p = cpy.AddCheckbox(REExportHiveRoot, pbHiveAsSubkey ? pbHiveAsSubkey : &lbDummy1);
	if (!(abAllowHives && pbHiveAsSubkey))
		p->Flags |= DIF_DISABLE;
	cpy.EndColumns();
	//if (abAllowHives && pbHiveAsSubkey)
	//{
	//	p2 = cpy.AddCheckbox(REExportHiveRoot, pbHiveAsSubkey);
	//	int Index = cpy.GetItemIndex(p2);
	//	p1 = cpy.GetItemByIndex(Index - 2);
	//	cpy.MoveItemAfter(p1,p2);
	//}
	//if (abAllowHives)
	//{
	//	p2 = cpy.AddCheckbox(REExportRawUnicode, &bUnicodeRawStrings);
	//	int Index = cpy.GetItemIndex(p2);
	//	p1 = cpy.GetItemByIndex(Index - ((abAllowHives && pbHiveAsSubkey) ? 2 : 1));
	//	cpy.MoveItemAfter(p1,p2);
	//}
	
	cpy.AddOKCancel(REBtnOK, nCancelMsgId);
	if (!cpy.ShowDialog())
	{
		lbExportRc = FALSE; goto wrap; // Отмена пользователем
	}

	//if (!abAllowHives && (*pnExportMode >= (REExportHive-REExportReg4)))
	//	(*pnExportMode)++;

	if ((*pnExportMode) == (REExportCmd-REExportReg4))
	{
		int nConfRc = 0;
		if (bUnicodeRawStrings)
			nConfRc = Message(REM_CmdWarnUtf8, 0/*FMSG_WARNING*/, 2, _T("ExportKey"));			
		else
			nConfRc = Message(REM_CmdWarnOEM, 0/*FMSG_WARNING*/, 2, _T("ExportKey"));
		if (nConfRc != 0)
		{
			lbExportRc = FALSE; goto wrap; // Отмена пользователем
		}
	}

	if (*pnExportMode > (REExportRaw-REExportReg4))
	{
		InvalidOp();
		lbExportRc = FALSE; goto wrap;
	}
	if (bUnicodeRawStrings
		&& ((*pnExportMode == (REExportRaw-REExportReg4))
		 	|| (*pnExportMode == (REExportCmd-REExportReg4))))
 	{
		*pnExportMode |= 0x100;
	}

	*ppwszDestPath = ExpandPath(ms_CreatedSubkeyBuffer);
	wchar_t* pszFilePart = (wchar_t*)PointToName(*ppwszDestPath);
	if (pszFilePart && *pszFilePart)
	{
		wchar_t* pszFileExt = (wchar_t*)PointToExt(pszFilePart);
		// Учесть, что юзер мог ввести расширение в диалоге, тогда ".reg" не добавлять
		if (pszFileExt && *pszFileExt == L'.')
		{
			if (wsDefaultExt) *wsDefaultExt = 0; // сбросить расширение по умолчанию (.reg) - оно уже добавлено в имя файла
			// Если на конце файла точка ('.') - все концевые точки отрезать и ".reg" не добавлять
			wchar_t* pszDot = pszFileExt;
			while (*pszDot == L'.') pszDot++;
			if (!*pszDot) *pszFileExt = 0;
		}
		// Теперь можно вернуть скорректированное значение
		lstrcpynW(sDefaultName, pszFilePart, MAX_PATH);
		// Оставить в pwszDestPath только папку!
		*pszFilePart = 0;
	}

	lbExportRc = TRUE;
wrap:
	return lbExportRc;
}

BOOL REPlugin::ConfirmCopyMove(BOOL abMove, int ItemsNumber, int nCopyLabelMsgId,
							   LPCTSTR asNameForLabel, LPCTSTR asDestPathOrFile
		//BOOL abAllowHives, int* pnExportMode, BOOL* pbHiveAsSubkey, int ItemsNumber,
		//LPCTSTR asDestPathOrFile, BOOL abDestFileSpecified, LPCTSTR asNameForLabel,
		//wchar_t** ppwszDestPath, wchar_t* sDefaultName/*[MAX_PATH]*/, wchar_t* wsDefaultExt/*[5]*/,
		//int nTitleMsgId /*= REExportDlgTitle*/, int nExportLabelMsgId /* = (ItemsNumber == 1) ? REExportItemLabel : REExportItemsLabel*/,
		//int nCancelMsgId /*= REBtnCancel*/
		)
{
	// Диалог подтверждения экспорта
	BOOL lbExportRc = FALSE;

	PluginDialogBuilder cpy(psi, RECopyDlgTitle, _T("CopyMove"), &guid_CopyMove);
	FarDialogItem* p = NULL; //, *p1, *p2;
	int nLen;
	TCHAR sLabel[67];
	LPCTSTR pszFormat = GetMsg(nCopyLabelMsgId);
	nLen = lstrlen(pszFormat);
	if (nLen >= 40)
	{
		InvalidOp();
		lbExportRc = FALSE; goto wrap;
	}

	int nMaxNameLen = countof(sLabel) - 4 - nLen;
	TCHAR sPart[61];
	_ASSERTE(nMaxNameLen < countof(sPart));

	nLen = lstrlen(asNameForLabel);
	if (nLen > nMaxNameLen)
	{
		#ifdef _UNICODE
		lstrcpyn(sPart, asNameForLabel, nMaxNameLen-1);
		lstrcat(sPart, L"\x2026");
		#else
		lstrcpyn(sPart, asNameForLabel, nMaxNameLen-3);
		lstrcat(sPart, "...");
		#endif
	}
	else
	{
		lstrcpyn(sPart, asNameForLabel, nMaxNameLen);
	}
	if (ItemsNumber == 1)
	{
		wsprintf(sLabel, pszFormat, sPart);
	}
	else
	{
		wsprintf(sLabel, pszFormat, ItemsNumber, sPart);
	}


	nLen = lstrlen(asDestPathOrFile);
	if ((size_t)nLen >= (countof(ms_CreatedSubkeyBuffer)-2-MAX_PATH))
	{
		InvalidOp();
		lbExportRc = FALSE; goto wrap;
	}
	lstrcpy(ms_CreatedSubkeyBuffer, asDestPathOrFile);
	if (ms_CreatedSubkeyBuffer[nLen-1] != _T('\\'))
	{
		ms_CreatedSubkeyBuffer[nLen++] = _T('\\');
		ms_CreatedSubkeyBuffer[nLen] = 0;
	}

	cpy.AddText(sLabel);
	p = cpy.AddEditField(ms_CreatedSubkeyBuffer, countof(ms_CreatedSubkeyBuffer), 66, _T("RegistryCopyTarget"));
	cpy.InitFocus(p);
	p->Flags |= DIF_READONLY;

	
	cpy.AddOKCancel(REBtnOK, REBtnCancel);
	if (!cpy.ShowDialog())
	{
		lbExportRc = FALSE; goto wrap; // Отмена пользователем
	}

	lbExportRc = TRUE;
wrap:
	return lbExportRc;
}

BOOL REPlugin::ExportItems(PluginPanelItem *PanelItem,int ItemsNumber,int Move,
						   const TCHAR *DestPath,u64 OpMode,const TCHAR** pszDestModified/*=NULL*/)
{
	BOOL lbExportRc = FALSE;
	wchar_t sDefaultName[MAX_PATH];
	//MFileTxt file(mb_Wow64on32, mb_Virtualize);
	MFileTxt* pFile = NULL;
	int nExportMode = (cfg->bCreateUnicodeFiles ? REExportReg5 : REExportReg4) - REExportReg4;
	wchar_t* pwszDestPath = NULL;
	wchar_t wsDefaultExt[5]; lstrcpyW(wsDefaultExt, L".reg");

	BOOL lbRawData = (OpMode & (OPM_QUICKVIEW|OPM_VIEW|OPM_EDIT|OPM_FIND)) != 0;
	BOOL lbSilence = (OpMode & (OPM_SILENT|OPM_FIND|OPM_QUICKVIEW)) != 0;
	
	// При экспорте единственного ключа можно выгрузить его самостоятельно (FALSE)
	// или ключ будет (виден) в корне hive (TRUE)
	BOOL lbHiveAsSubkey = (ItemsNumber != 1), lbCanChangeAsSubkey = (ItemsNumber == 1);


#ifdef _DEBUG
	REPlugin *ppp = this;
#endif

	RegFolder* expFolder = PrepareExportPanel(PanelItem, ItemsNumber, sDefaultName, MAX_FILE_NAME-5); // в путь учесть ".reg"

	// Если вызов из панели поиска (F3 на найденной строке)
	if (!lbRawData && expFolder->mn_ItemCount == 1 && ((OpMode & OPM_SILENT) == OPM_SILENT))
	{
		if (!expFolder->mp_Items)
		{
			_ASSERTE(expFolder->mp_Items != NULL);
		} else {
			// То хорошо бы по возможности показать RawData (кроме BYTE, WORD, DWORD?)
			REGTYPE nValType = expFolder->mp_Items->nValueType;
			if (nValType == REG_SZ || nValType == REG_MULTI_SZ || nValType == REG_EXPAND_SZ)
				lbRawData = TRUE;
			else if (expFolder->mp_Items->nDataSize > 4)
				lbRawData = TRUE;
		}
	}

#ifdef _DEBUG
	_ASSERTE(ppp == this);
#endif
	if (!expFolder)
	{
		lbExportRc = FALSE; goto wrap;
	}



	if (lbRawData && (expFolder->mn_ItemCount == 0 || expFolder->mp_Items->nValueType == REG__KEY))
		lbRawData = FALSE;
		
	if (lbCanChangeAsSubkey)
	{
		if (expFolder->mn_ItemCount > 1 
			|| (expFolder->mn_ItemCount == 1 && expFolder->mp_Items->nValueType != REG__KEY))
		{
			lbCanChangeAsSubkey = FALSE; lbHiveAsSubkey = TRUE;
		}
	}

	if (lbRawData)
	{
		// RawData - значит что хотят редактировать одно значение
		// во встроенном редакторе FAR БЕЗ преобразований в hex
		if (ItemsNumber != 1
			// При включенном "Ключи как файлы" - здесь будет 0.
			// Если если нажать Enter на ключе (как файл), а Enter не перехвачен
			|| expFolder->mn_ItemCount != 1
			)
		{
			_ASSERTE(ItemsNumber == 1);
			_ASSERTE(expFolder->mn_ItemCount == 1);
			if (!lbSilence) InvalidOp();
			lbExportRc = FALSE; goto wrap;
		}

		//TODO: ANSI версия обламывается на длинных именах значений. Фар не может открыть получившийся файл.
		lstrcpynW(sDefaultName, 
			expFolder->mp_Items->pszName ? expFolder->mp_Items->pszName : REGEDIT_DEFAULTNAME,
			countof(sDefaultName));
		wsDefaultExt[0] = 0; // в RAW режиме расширение НЕ добавлем
		pwszDestPath = ExpandPath(DestPath);
	}
	else if (!lbSilence)
	{
		// Диалог подтверждения экспорта
		/*
		г=============================== Copy ===============================¬
		¦ Copy "Far2" to                                                     ¦
		¦ T:\VCProject\FarPlugin\RegEditor\_Test\Far2.reg                   v¦
		¦--------------------------------------------------------------------¦
		¦ Export keys as                                                     ¦
		¦ ( ) REGEDIT4 files                                                 ¦
		¦ (•) REGEDIT5 (Unicode) files                                       ¦
		¦ ( ) Binary files ("hives")                                         ¦
		¦--------------------------------------------------------------------¦
		¦                         { OK } [ Cancel ]                          ¦
		L====================================================================-	
		*/

		TCHAR sLabel[70];
		//if (ItemsNumber == 1)
		//	lstrcpyn(sLabel, FILENAMEPTR(PanelItem->FindData), countof(sLabel));
		//else
		lstrcpy_t(sLabel, countof(sLabel), sDefaultName);

		lbExportRc = ConfirmExport(
			TRUE/*abAllowHives*/, &nExportMode,
			lbCanChangeAsSubkey ? &lbHiveAsSubkey : NULL, ItemsNumber,
			DestPath, FALSE/*abDestFileSpecified*/, sLabel,
			&pwszDestPath, sDefaultName, wsDefaultExt,
			REExportDlgTitle, (ItemsNumber == 1) ? REExportItemLabel : REExportItemsLabel);

		if (!lbExportRc)
			goto wrap;
			
		if (nExportMode == (REExportHive - REExportReg4))
		{
			lbExportRc = ExportItems2Hive(expFolder, lbHiveAsSubkey, Move, pwszDestPath, sDefaultName, wsDefaultExt, pszDestModified);
			goto wrap;
		}
		if ((nExportMode & 0xF) == (REExportRaw - REExportReg4))
		{
			lbExportRc = ExportItems2Raws(expFolder, (nExportMode & 0x100) == 0x100, Move, pwszDestPath);
			goto wrap;
		}

	} else {
		// "Тихий" режим, подтверждений не требуется, Но путь развернуть нужно
		pwszDestPath = ExpandPath(DestPath);
	}

	BOOL lbUseUnicode = FALSE, lbNoBOM = FALSE;
	if ((nExportMode & 0xF) == (REExportCmd - REExportReg4))
	{
		lbUseUnicode = (nExportMode & 0x100) == 0x100;
		lbNoBOM = TRUE;
		pFile = new MFileTxtCmd(mb_Wow64on32, mb_Virtualize);
		lstrcpyW(wsDefaultExt, L".cmd");
	}
	else
	{
		lbUseUnicode = nExportMode == (REExportReg5 - REExportReg4);
		pFile = new MFileTxtReg(mb_Wow64on32, mb_Virtualize);
	}

	// Создаем файл
	if (pFile->FileCreate(pwszDestPath, sDefaultName, wsDefaultExt, lbUseUnicode, !lbSilence, lbNoBOM))
	{
		if (lbRawData)
		{
			// По идее, сюда попадаем только для QView
			if (expFolder->mp_Items && expFolder->mn_ItemCount > 0)
			{
				DWORD nDataSize = 0;
				REGTYPE nDataType;
				LPBYTE pValue = NULL;
				if (ValueDataGet(expFolder, expFolder->mp_Items, &pValue, &nDataSize, &nDataType, lbSilence) == 0 && pValue)
				{
					// Тут хорошо бы нормальные переводы пихать - построчно писать ...
					if (nDataType == REG_MULTI_SZ)
					{
						lbExportRc = pFile->FileWriteMSZ((wchar_t*)pValue, nDataSize>>1);
					}
					else
					{
						#ifdef _UNICODE
						lbExportRc = pFile->FileWriteBuffered(pValue, nDataSize);
						#else
						// REG_MULTI_SZ уже обработан выше, но пусть будет для "красивости" условия
						if (nDataType == REG_SZ || nDataType == REG_MULTI_SZ || nDataType == REG_EXPAND_SZ)
							lbExportRc = pFile->FileWrite((wchar_t*)pValue, nDataSize>>1);
						else
							lbExportRc = pFile->FileWriteBuffered(pValue, nDataSize);
						#endif
					}
				}
				SafeFree(pValue);
			}

		}
		else
		{
			// Сформировать заголовок
			lbExportRc = pFile->FileWriteHeader(mp_Worker);
			//if (nExportMode == (REExportReg5-REExportReg4)) {
			//	// (BOM уже записан в pFile->FileCreateTemp)
			//	lbExportRc = pFile->FileWrite(L"Windows Registry Editor Version 5.00\r\n");
			//} else {
			//	LPCSTR pszHeader = "REGEDIT4\r\n";
			//	int nSize = lstrlenA(pszHeader);
			//	lbExportRc = pFile->FileWriteBuffered(pszHeader, nSize);
			//}
			//if (mp_Worker && mp_Worker->eType == RE_HIVE)
			//{
			//	//lbExportRc = pFile->FileWriteBuffered(pszHeader, nSize);
			//	//lbExportRc = pFile->FileWriteBuffered(pszHeader, nSize);
			//	wchar_t* pwszHost = UnmakeUNCPath_w(((MFileHive*)mp_Worker)->GetFilePathName());
			//	lbExportRc = pFile->FileWrite(L"; ") &&
			//		pFile->FileWrite(pwszHost) &&
			//		pFile->FileWrite(L"\r\n");
			//	SafeFree(pwszHost);
			//}

			// Запускаем экспорт
			if (lbExportRc)
			{
				_ASSERTE(gpProgress == NULL);
				if (!lbSilence)
				{
					gpProgress = new REProgress(GetMsg(REExportDlgTitle));
				}
				// Теперь, собственно экспорт
				lbExportRc = expFolder->ExportToFile(this, Worker(), pFile, cfg->bCreateUnicodeFiles);
				// Удалить, если прогресс создавали
				if (gpProgress)
				{
					SafeDelete(gpProgress);
				}
			}
		}
		
		pFile->FileClose();

		// При успешном экспорте - вернуть реально созданный путь
		if (lbExportRc && pszDestModified) {
			lstrcpyn(ms_CreatedSubkeyBuffer, pFile->GetShowFilePathName(), countof(ms_CreatedSubkeyBuffer));
			*pszDestModified = ms_CreatedSubkeyBuffer;
		}

		if (!lbExportRc)
			pFile->FileDelete();
	}

wrap:
	if (expFolder)
	{
		expFolder->Release();
		SafeDelete(expFolder);
	}
	if (pFile)
	{
		delete pFile;
		pFile = NULL;
	}
	SafeFree(pwszDestPath);
	return lbExportRc;
}

// По идее, вызывается только в визуальном режиме
BOOL REPlugin::ExportItems2Hive(RegFolder* expFolder,BOOL abHiveAsSubkey,int Move,LPCWSTR pwszDestPath,LPCWSTR pwszDefaultName,LPCWSTR pwszDefaultExt, const TCHAR** pszDestModified)
{
	if (!mp_Worker)
	{
		InvalidOp();
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	LONG hRc = 0;
	BOOL bPrivileged = FALSE;
	HREGKEY hParent = NULLHKEY, hKey = NULLHKEY;
	
	// Сформировать полное имя файла назначения
	wchar_t* pwszTargetFile = NULL;
	int nLen = 0, nDirLen = 0, nFileLen = 0;
	if (pwszDestPath) nLen += (nDirLen = lstrlenW(pwszDestPath))+2;
	if (pwszDefaultName) nLen += (nFileLen = lstrlenW(pwszDefaultName))+2;
	if (pwszDefaultExt) nLen += lstrlenW(pwszDefaultExt)+2;
	pwszTargetFile = (wchar_t*)calloc(nLen+1,2);
	if (pwszDestPath)
	{
		lstrcpyW(pwszTargetFile, pwszDestPath);
		if (pwszDestPath[nDirLen-1] != L'\\')
			lstrcatW(pwszTargetFile, L"\\");
	}
	if (pwszDefaultName)
	{
		CopyFileName(pwszTargetFile+lstrlenW(pwszTargetFile), nFileLen+1, pwszDefaultName);
	}
	/*if (pwszDefaultExt)
	{
		lstrcatW(pwszTargetFile, pwszDefaultExt);
	}*/
	
	
	if (m_Key.eType == RE_WINAPI || m_Key.eType == RE_HIVE)
	{
		if (!abHiveAsSubkey)
		{
			// Проверим, в каких случаях недопустимо
			if (expFolder->mn_ItemCount == 1 && expFolder->mp_Items->nValueType != REG__KEY)
			{
				InvalidOp();
				goto wrap;
			}
			else if (expFolder->mn_ItemCount > 1)
			{
				InvalidOp();
				goto wrap;
			}
			// Осталось выбросить единственный ключ в бинарный файл
			if (!cfg->BackupPrivilegesAcuire(FALSE/*abRequireRestore*/))
				goto wrap;
			bPrivileged = TRUE;
			
			if ((hRc = mp_Worker->OpenKeyEx(expFolder->key.mh_Root, expFolder->key.mpsz_Key, 0, KEY_READ, &hParent, NULL)) != 0)
			{
				CantOpenKey(&expFolder->key, FALSE);
			}
			if (hRc == 0)
			{
				if (expFolder->mn_ItemCount == 1)
				{
					if ((hRc = mp_Worker->OpenKeyEx(hParent, expFolder->mp_Items->pszName, 0, KEY_READ, &hKey, NULL)) != 0)
						CantOpenKey(&expFolder->key, expFolder->mp_Items->pszName, FALSE);
				} else {
					hKey = hParent; hParent = NULL;
				}
			}
			if (hRc == 0 && (hRc = mp_Worker->SaveKey(hKey, pwszTargetFile)) != 0)
				REPlugin::CantLoadSaveKey(
					(expFolder->mn_ItemCount == 1) ? expFolder->mp_Items->pszName : expFolder->key.mpsz_Key,
					pwszTargetFile, TRUE/*abSave*/);
			if (hKey)
			{
				mp_Worker->CloseKey(&hKey);
			}
			if (hParent)
			{
				mp_Worker->CloseKey(&hParent);
			}
			lbRc = (hRc == 0);
		}
		else
		{
			InvalidOp(); //TODO:
		}
	}
	else if (m_Key.eType == RE_REGFILE)
	{
		InvalidOp(); //TODO:
	}
	else
	{
		InvalidOp();
		goto wrap;
	}
	
wrap:
	SafeFree(pwszTargetFile);
	if (bPrivileged) cfg->BackupPrivilegesRelease();
	return lbRc;
}

// Выбросить "сырые" данные. Каждое значение - в свой файл
BOOL REPlugin::ExportItems2Raws(RegFolder* expFolder,BOOL abUnicodeStrings,int Move,LPCWSTR pwszDestPath)
{
	if (!mp_Worker)
	{
		InvalidOp();
		return FALSE;
	}
	if (!pwszDestPath)
	{
		InvalidOp();
		return FALSE;
	}
	if (!expFolder)
	{
		InvalidOp();
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	LONG hRc = 0;
	HREGKEY hParent = NULLHKEY;
	REGTYPE DataType;
	DWORD nMaxDataSize = 65535*sizeof(wchar_t), nDataSize;
	LPBYTE ptrData = (LPBYTE)malloc(nMaxDataSize);
	DWORD nMaxAnsiSize = 65535;
	char* pszDataAnsi = abUnicodeStrings ? NULL : (char*)malloc(nMaxAnsiSize);
	MFileTxtReg txt(mb_Wow64on32, mb_Virtualize);

	if ((hRc = mp_Worker->OpenKeyEx(expFolder->key.mh_Root, expFolder->key.mpsz_Key, 0, KEY_READ, &hParent, NULL)) != 0)
	{
		CantOpenKey(&expFolder->key, FALSE);
		goto wrap;
	}

	for (UINT i = 0; i < expFolder->mn_ItemCount; i++)
	{
		LPCWSTR pszName = expFolder->mp_Items[i].pszName;
		txt.ReleasePointers();

		if (expFolder->mp_Items[i].nValueType != REG__KEY)
		{
			//if (!txt.FileWriteBuffered(expFolder->mp_Items[i].da
			hRc = mp_Worker->QueryValueEx(hParent, pszName, NULL, &DataType,
				ptrData, &(nDataSize = nMaxDataSize));
			if (hRc == ERROR_MORE_DATA)
			{
				_ASSERTE(nDataSize > nMaxDataSize);
				nMaxDataSize = nDataSize+512;
				free(ptrData);
				ptrData = (LPBYTE)malloc(nMaxDataSize);
				hRc = mp_Worker->QueryValueEx(hParent, pszName, NULL, &DataType,
					ptrData, &(nDataSize = nMaxDataSize));
			}
			if (hRc != 0)
			{
				if (!REPlugin::ValueOperationFailed(&expFolder->key, pszName, FALSE, ((i+1) < expFolder->mn_ItemCount)))
					goto wrap;
			}

			if (!txt.FileCreate(pwszDestPath,
				pszName ? pszName : REGEDIT_DEFAULTNAME,
				L"", abUnicodeStrings, TRUE, TRUE))
				goto wrap; // ошибка
			if (nDataSize > 0)
			{
				if (abUnicodeStrings || !(DataType == REG_SZ || DataType == REG_EXPAND_SZ || DataType == REG_MULTI_SZ))
				{
					if (!txt.FileWriteBuffered(ptrData, nDataSize))
						goto wrap;
				}
				else
				{
					if ((nDataSize % 2) != 0)
					{
						InvalidOp();
						goto wrap;
					}
					DWORD nAnsiLen = nDataSize >> 1;
					if (!pszDataAnsi || (nAnsiLen > nMaxAnsiSize))
					{
						SafeFree(pszDataAnsi);
						nMaxAnsiSize = nAnsiLen + 512;
						pszDataAnsi = (char*)malloc(nMaxAnsiSize);
					}
					if (!pszDataAnsi)
					{
						InvalidOp();
						goto wrap;
					}
					WideCharToMultiByte(cfg->nAnsiCodePage, 0, (wchar_t*)ptrData, nAnsiLen, pszDataAnsi, nAnsiLen, 0,0);
					if (!txt.FileWriteBuffered(pszDataAnsi, nAnsiLen))
						goto wrap;
				}
			}
			txt.FileClose();
		}
		else
		{
			if (!txt.FileCreate(pwszDestPath, 
				pszName ? pszName : REGEDIT_DEFAULTNAME,
				L".reg", abUnicodeStrings, TRUE)
				|| !txt.FileWriteHeader(mp_Worker))
				goto wrap; // ошибка

			RegFolder* expKey = new RegFolder;
			expKey->Init(&expFolder->key);
			expKey->AllocateItems(1, 0, MAX_PATH, 0);
			expKey->AddItem(pszName, lstrlenW(expFolder->mp_Items[i].pszName),
				expFolder->mft_LastSubkey, NULL, NULL, FILE_ATTRIBUTE_DIRECTORY, REG__KEY, 0, 0, NULL);
			BOOL lbExpRc = expKey->ExportToFile(this, mp_Worker, &txt, abUnicodeStrings);
			expKey->Release();
			delete expKey;
			txt.FileClose();

			if (!lbExpRc)
				goto wrap;
		}
	}

	lbRc = TRUE;
wrap:
	SafeFree(pszDataAnsi);
	SafeFree(ptrData);
	if (hParent)
	{
		mp_Worker->CloseKey(&hParent);
	}
	return lbRc;
}

// Редактирование текущего или выделенных элементов
void REPlugin::EditItem(bool abOnlyCurrent, bool abForceExport, bool abViewOnly, bool abRawData /*= false*/)
{
	BOOL lbExportRc = FALSE;
	MFileTxtReg file(mb_Wow64on32, mb_Virtualize);

	MCHKHEAP;
	wchar_t sDefaultName[MAX_PATH];
#ifdef _DEBUG
	REPlugin *ppp = this;
#endif

	RegFolder* expFolder = PrepareExportKey(abOnlyCurrent, sDefaultName, MAX_FILE_NAME-5, false); // в путь учесть ".reg"

#ifdef _DEBUG
	_ASSERTE(ppp == this);
#endif
	MCHKHEAP;
	if (!expFolder)
		goto wrap;

	if (abRawData)
	{
		if (expFolder->mn_ItemCount == 1 && expFolder->mp_Items[0].nValueType != REG__KEY)
		{
			EditValueRaw(expFolder, expFolder->mp_Items, abViewOnly);
			goto wrap;
		}
		abForceExport = true;
		abRawData = false;		
	}

	if (!abRawData && !abForceExport && expFolder->mn_ItemCount == 1 && expFolder->mp_Items[0].nValueType != REG__KEY)
	{
		// Редактирование одиночного значения
		if (EditValue(expFolder, expFolder->mp_Items, 0/*nNewValueType*/) >= 0) {
			goto wrap;
		}
		// Иначе - продолжить через выгрузку в *.reg
	}
	if (abRawData)
	{
		_ASSERTE(!abForceExport && abOnlyCurrent);
		if (abForceExport || !abOnlyCurrent)
		{
			InvalidOp();
			goto wrap;
		}
	}

	if (file.FileCreateTemp(sDefaultName, L".reg", cfg->bCreateUnicodeFiles))
	{
		// Сформировать заголовок
		lbExportRc = file.FileWriteHeader(mp_Worker);
		//if (cfg->bCreateUnicodeFiles) {
		//	// (BOM уже записан в file.FileCreateTemp)
		//	lbExportRc = file.FileWrite(L"Windows Registry Editor Version 5.00\r\n");
		//} else {
		//	LPCSTR pszHeader = "REGEDIT4\r\n";
		//	int nSize = lstrlenA(pszHeader);
		//	lbExportRc = file.FileWriteBuffered(pszHeader, nSize);
		//}
		//if (mp_Worker && mp_Worker->eType == RE_HIVE)
		//{
		//	//lbExportRc = file.FileWriteBuffered(pszHeader, nSize);
		//	//lbExportRc = file.FileWriteBuffered(pszHeader, nSize);
		//	wchar_t* pwszHost = UnmakeUNCPath_w(((MFileHive*)mp_Worker)->GetFilePathName());
		//	lbExportRc = file.FileWrite(L"; ") &&
		//		file.FileWrite(pwszHost) &&
		//		file.FileWrite(L"\r\n");
		//	SafeFree(pwszHost);
		//}

		BOOL lbSelfCreated = FALSE;
		if (!gpProgress)
		{
			gpProgress = new REProgress(GetMsg(REExportDlgTitle));
			lbSelfCreated = TRUE;
		}

		// Запускаем экспорт
		if (lbExportRc)
			lbExportRc = expFolder->ExportToFile(this, Worker(), &file, cfg->bCreateUnicodeFiles);

		if (gpProgress && lbSelfCreated)
		{
			SafeDelete(gpProgress);
		}

		file.FileClose();
		
		if (!lbExportRc)
			file.FileDelete();
	}

	MCHKHEAP;
	

	BOOL lbImported = FALSE;	
	if (lbExportRc)
	{
		TCHAR sTitle[MAX_PATH+1];
		lstrcpy_t(sTitle, countof(sTitle), sDefaultName[0] ? sDefaultName : REGEDIT_DEFAULTNAME);
		if (!abViewOnly)
		{
			// GetShowFilePathName() возвращает то же имя файла, но без префикса "\\?\" и в кодировке (Oem/Unicode)
			while (true)
			{
				//TODO: Сделать немодальным (читай !todo.txt)
				int nEdtRc = psi.Editor(file.GetShowFilePathName(), sTitle, 0,0,-1,-1,
					EF_DISABLEHISTORY/*EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONCLOSE|EF_ENABLE_F6*/,
					#ifdef _UNICODE
					1, 1, cfg->EditorCodePage()
					#else
					0, 1
					#endif
				);
				if (nEdtRc == EEC_MODIFIED)
				{
					//TODO: Применение изменений, следанных в редакторе *.reg
					//psi.Message(PluginNumber, FMSG_WARNING|FMSG_MB_OK|FMSG_ALLINONE, NULL, 
					//	(const TCHAR * const *)_T("RegEditor\nImporing of *.reg not implemented yet, sorry."), 2,0);
					// Запускаем импорт
					MFileReg fileReg(mb_Wow64on32, mb_Virtualize);
					gpProgress = new REProgress(GetMsg(REImportDlgTitle), TRUE);
					LONG hImpRc = MFileReg::LoadRegFile(file.GetFilePathName(), FALSE, Worker(), FALSE, &fileReg);
					SafeDelete(gpProgress);
					if (hImpRc != 0)
					{
						REPlugin::MessageFmt(REM_ImportFailed, file.GetFilePathName(), hImpRc);
					}
					lbImported = TRUE; // Даже если были ошибки - нужно обновить, может что-то и проимпортировалось
				}
				//TODO: Если были ошибки и запрошен возврат в редактор для правки - на повтор
				break;
			}
			// Поскольку редактор немодальный - удалить временный файл
			file.FileDelete();
		} else {
			psi.Viewer(file.GetShowFilePathName(), sTitle, 0,0,-1,-1,
				VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY
				#ifdef _UNICODE
				, cfg->EditorCodePage()
				#endif
				);
		}
	}

	MCHKHEAP;
	if (lbImported)
	{
		SetForceReload();
		UpdatePanel(false);
		RedrawPanel(NULL);
	}
wrap:
	if (expFolder)
	{
		expFolder->Release();
		SafeDelete(expFolder);
	}
}

RegFolder* REPlugin::PrepareExportPanel(PluginPanelItem *PanelItem, int ItemsNumber, wchar_t *psDefaultName/*[nMaxDefaultLen]*/, int nMaxDefaultLen)
{
	//bool abOnlyCurrent
	wchar_t sDefaultName[MAX_PATH+1]; sDefaultName[0] = 0;

	MCHKHEAP;
	
	//PanelInfo inf; memset(&inf, 0, sizeof(inf));
	//INT_PTR nCurLen = 0, nMaxLen = 0;
	//PluginPanelItem* item=NULL;
	RegItem* pItem = NULL;
	
	if (m_Key.eType == RE_UNDEFINED)
	{
		_ASSERTE(m_Key.eType != RE_UNDEFINED);
		return NULL;
	}
	
	bool lbSucceeded = false;
	RegFolder *expFolder = new RegFolder; // не кешируем, т.к. грузить сейчас нужно всё подряд
	expFolder->Init(&m_Key);


	// Если выделен ".." то запустить редактирование текущего ключа
	if (ItemsNumber == 0)
	{
		if (expFolder->key.mh_Root == NULL)
		{
			goto wrap; // мы и так в корне, редактировать выше - нечего
		}
		if (!expFolder->key.mpsz_Key || !*expFolder->key.mpsz_Key)
		{
			_ASSERTE(expFolder->key.mpsz_Key && expFolder->key.mpsz_Key[0]);
			goto wrap;
		}
		// Экспортируемое имя - имя текущего ключа
		LPCWSTR pszLastName = PointToName(expFolder->key.mpsz_Key);
		#ifdef _DEBUG
		int nNameLen = lstrlenW(pszLastName);
		_ASSERTE(nNameLen < MAX_REGKEY_NAME);
		#endif
		lstrcpynW(sDefaultName, pszLastName, MAX_REGKEY_NAME+1);
		#ifdef _DEBUG
		nNameLen = lstrlenW(sDefaultName);
		#endif
		lbSucceeded = true; // OK!
		
	}
	else
	{
		_ASSERTE(ItemsNumber>0);
		pItem = (RegItem*)PanelItemUserData(PanelItem[0]);
		if (ItemsNumber > 1 || (pItem == NULL) /*|| (pItem && pItem->nValueType != REG__KEY)*/)
		{
			// Имя брать из родительского ключа!
			if (expFolder->key.mpsz_Key[0])
			{
				lstrcpynW(sDefaultName, PointToName(expFolder->key.mpsz_Key), MAX_REGKEY_NAME+1);
			}
			else
			{
				if (!HKeyToStringKey(expFolder->key.mh_Root, sDefaultName, countof(sDefaultName)))
					lstrcpynW(sDefaultName, L"InvalidDefaultName", MAX_REGKEY_NAME+1);
			}
		}
		
		// Не совсем корректно. Могут быть значения с длинными именами,
		// но сервис сам перевыделит память при необходимости
		expFolder->AllocateItems(ItemsNumber,0,MAX_PATH+1,0);

		_ASSERTE(PanelItem != NULL);
		for (int n = 0; n < ItemsNumber; n++)
		{
			pItem = (RegItem*)PanelItemUserData(PanelItem[n]);
			if (pItem == NULL)
			{
				if ((PanelItemAttributes(PanelItem[n]) & FILE_ATTRIBUTE_DIRECTORY)
					&& FILENAMEPTR(PanelItem[n])[0] == _T('.')
					&& FILENAMEPTR(PanelItem[n])[1] == _T('.')
					&& FILENAMEPTR(PanelItem[n])[2] == 0
					&& (ItemsNumber == 1))
				{
					_ASSERTE(sDefaultName[0] != 0);
					break;
				}
				else if (pItem->nMagic != REGEDIT_MAGIC)
				{
					_ASSERTE(pItem != NULL);
					_ASSERTE(pItem && pItem->nMagic == REGEDIT_MAGIC);
					InvalidOp();
					goto wrap;
				}
			}

			if (ItemsNumber == 1)
			{
				// Если выделен только один - брать имя из него
				lstrcpynW(sDefaultName, pItem->pszName, MAX_REGKEY_NAME+1);
			}
			if (ItemsNumber == 1 && pItem->nValueType == REG__KEY)
			{
				// И сразу сменить папку!
				if (!expFolder->key.ChDir(pItem, false, this))
					goto wrap;
			}
			else
			{
				_ASSERTE(pItem->bDefaultValue || (!pItem->bDefaultValue || pItem->nValueType == REG__KEY));
				expFolder->AddItem(
					pItem->bDefaultValue ? NULL : pItem->pszName,
					pItem->bDefaultValue ? 0 : lstrlenW(pItem->pszName),
					m_Key.ftModified, NULL, NULL,
					(pItem->nValueType == REG__KEY) ? FILE_ATTRIBUTE_DIRECTORY : 0,
					pItem->nValueType, 0, NULL, NULL);
			}
		}
		lbSucceeded = true; // OK!
	}

	MCHKHEAP;
wrap:
	if (lbSucceeded && sDefaultName[0] == 0)
	{
		_ASSERTE(sDefaultName[0] != 0);
		lbSucceeded = false;
	}
	if (!lbSucceeded)
	{
		expFolder->Release();
		SafeDelete(expFolder);
	}
	else
	{
		if (psDefaultName)
		{
			CopyFileName(psDefaultName, nMaxDefaultLen, sDefaultName);
		}
	}

	MCHKHEAP;
	
	return expFolder;
}

RegFolder* REPlugin::PrepareExportKey(
			bool abOnlyCurrent,
			wchar_t *psDefaultName/*[nMaxDefaultLen]*/, int nMaxDefaultLen,
			bool abFavorItem) // если true, элемент - ключ, и он один - то добавить его в список, а не делать ChDir
{
	wchar_t sDefaultName[MAX_PATH+1]; sDefaultName[0] = 0;

	MCHKHEAP;
	
	PanelInfo inf = {sizeof(inf)};
	INT_PTR nCurLen = 0, nMaxLen = 0;
	PluginPanelItem* item=NULL;
	RegItem* pItem = NULL;
	
	if (m_Key.eType == RE_UNDEFINED)
	{
		_ASSERTE(m_Key.eType != RE_UNDEFINED);
		return NULL;
	}
	
	bool lbSucceeded = false;
	int nLen;
	RegFolder *expFolder = new RegFolder; // не кешируем, т.к. грузить сейчас нужно всё подряд
	expFolder->Init(&m_Key);

	if (psiControl((HANDLE)this, FCTL_GETPANELINFO, F757NA &inf))
	{
		// Если выделен ".." то запустить редактирование текущего ключа
		if (inf.ItemsNumber == 0 
			|| (inf.CurrentItem == 0 && (abOnlyCurrent || inf.SelectedItemsNumber == 0)))
		{
			if (expFolder->key.mh_Root == NULL)
			{
				if (m_Key.eType == RE_REGFILE || m_Key.eType == RE_HIVE)
				{
					LPCTSTR pszFile = PointToName(mpsz_HostFile);
					lstrcpy_t(sDefaultName, countof(sDefaultName), pszFile ? pszFile : _T(""));
					lbSucceeded = true; // ОК, выгружаем весь файл
				}
				goto wrap; // мы и так в корне, редактировать выше - нечего
			}
			// Ключ может быть пустым (например, выгружаем весь HKCR), но память должна быть выделена, иначе это сбой
			if (!expFolder->key.mpsz_Key /*|| !*expFolder->key.mpsz_Key*/)
			{
				_ASSERTE(expFolder->key.mpsz_Key /*&& expFolder->key.mpsz_Key[0]*/);
				InvalidOp();
				goto wrap;
			}
			int nNameLen = 0;
			if (!*expFolder->key.mpsz_Key)
			{
				HKeyToStringKey(expFolder->key.mh_Root, sDefaultName, countof(sDefaultName));
			}
			else
			{
				// Экспортируемое имя - имя текущего ключа
				LPCWSTR pszLastName = PointToName(expFolder->key.mpsz_Key);
				nNameLen = lstrlenW(pszLastName);
				_ASSERTE(nNameLen < MAX_REGKEY_NAME);
				lstrcpynW(sDefaultName, pszLastName, countof(sDefaultName));
			}
			#ifdef _DEBUG
			nNameLen = lstrlenW(sDefaultName);
			#endif
			lbSucceeded = true; // OK!
			
		}
		else if (abOnlyCurrent && inf.CurrentItem>0)
		{
			_ASSERTE(item == NULL);
			#ifdef _UNICODE
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, NULL);
			item = (PluginPanelItem*)calloc(nCurLen,1);
			#if FAR_UNICODE>=3000
			FarGetPluginPanelItem gppi = {sizeof(gppi), nCurLen, item};
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (FarDlgProcParam2)&gppi);
			#elif FAR_UNICODE>=1906
			FarGetPluginPanelItem gppi = {nCurLen, item};
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (FarDlgProcParam2)&gppi);
			#else
			nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, inf.CurrentItem, (FarDlgProcParam2)item);
			#endif
			if (!nCurLen)
			{
				goto wrap;
			}
			#else
			item = &(inf.PanelItems[inf.CurrentItem]);
			#endif
			
			pItem = (RegItem*)PanelItemUserData(*item);
			if (pItem == NULL || pItem->nMagic != REGEDIT_MAGIC)
			{
				#ifdef _DEBUG
				if (!pItem)
					_ASSERTE(pItem != NULL);
				else
					_ASSERTE(pItem->nMagic == REGEDIT_MAGIC);
				#endif
				goto wrap;
			}
			
			lstrcpynW(sDefaultName, pItem->pszName, MAX_PATH+1);
			if (pItem->nValueType == REG__KEY)
			{
				if (!expFolder->key.ChDir(pItem, false, this))
					goto wrap;
			}
			else
			{
				nLen = lstrlenW(pItem->pszName)+1;
				if (pItem->nValueType != REG__KEY)
				{
					expFolder->AllocateItems(0,1,0,nLen+1);
					_ASSERTE(pItem->bDefaultValue || (pItem->nValueType != REG__KEY));
					expFolder->AddItem(
						pItem->bDefaultValue ? NULL : pItem->pszName,
						nLen, m_Key.ftModified, NULL, NULL,
						0/*значение*/, pItem->nValueType, 0, NULL, NULL);
				}
			}
			lbSucceeded = true; // OK!
			
		}
		else if (inf.SelectedItemsNumber>0)
		{
			if (inf.SelectedItemsNumber > 1)
			{
				// Имя брать из родительского ключа!
				if (!expFolder->key.mpsz_Key || !*expFolder->key.mpsz_Key)
				{
					LPCWSTR pszKey = HKeyToStringKey(expFolder->key.mh_Root);
					lstrcpynW(sDefaultName, pszKey ? pszKey : L"Registry", MAX_PATH+1);
				}
				else
				{
					lstrcpynW(sDefaultName, PointToName(expFolder->key.mpsz_Key), MAX_PATH+1);
				}
			}
			
			// Не совсем корректно. Могут быть значения с длинными именами,
			// но сервис сам перевыделит память при необходимости
			expFolder->AllocateItems(inf.SelectedItemsNumber,0,MAX_PATH+1,0);

			_ASSERTE(item == NULL);
			for (INT_PTR n = 0; n < (INT_PTR)inf.SelectedItemsNumber; n++)
			{
				#ifdef _UNICODE
				nCurLen = psiControl((HANDLE)this, FCTL_GETSELECTEDPANELITEM, n, NULL);
				if (!item || nCurLen > nMaxLen)
				{
					SafeFree(item);
					nMaxLen = nCurLen + 512;
					item = (PluginPanelItem*)calloc(nMaxLen,1);
				}
				#if FAR_UNICODE>=3000
				FarGetPluginPanelItem gppi = {sizeof(gppi), nCurLen, item};
				nCurLen = psiControl((HANDLE)this, FCTL_GETSELECTEDPANELITEM, n, (FarDlgProcParam2)&gppi);
				#elif FAR_UNICODE>=1906
				FarGetPluginPanelItem gppi = {nCurLen, item};
				nCurLen = psiControl((HANDLE)this, FCTL_GETSELECTEDPANELITEM, n, (FarDlgProcParam2)&gppi);
				#else
				nCurLen = psiControl((HANDLE)this, FCTL_GETSELECTEDPANELITEM, n, (FarDlgProcParam2)item);
				#endif
				if (!nCurLen)
				{
					goto wrap;
				}
				#else
				item = &(inf.SelectedItems[n]);
				#endif
				
				pItem = (RegItem*)PanelItemUserData(*item);
				if (pItem == NULL || pItem->nMagic != REGEDIT_MAGIC)
				{
					_ASSERTE(pItem != NULL);
					_ASSERTE(pItem->nMagic == REGEDIT_MAGIC);
					goto wrap;
				}

				// Если выделен только один - брать имя из него
				if (inf.SelectedItemsNumber == 1)
				{
					lstrcpynW(sDefaultName, pItem->pszName, MAX_PATH+1);
				}
				// Если выделен только один КЛЮЧ - перейти в него
				if (inf.SelectedItemsNumber == 1 && pItem->nValueType == REG__KEY && !abFavorItem)
				{
					// И сразу сменить папку!
					if (!expFolder->key.ChDir(pItem, false, this))
						goto wrap;
				}
				else
				{
					_ASSERTE((pItem->bDefaultValue && (pItem->nValueType != REG__KEY)) || !pItem->bDefaultValue);
					expFolder->AddItem(
						pItem->bDefaultValue ? NULL : pItem->pszName,
						pItem->bDefaultValue ? 0 : lstrlenW(pItem->pszName), m_Key.ftModified, NULL, NULL,
						(pItem->nValueType == REG__KEY) ? FILE_ATTRIBUTE_DIRECTORY : 0,
						pItem->nValueType, 0, NULL, NULL);
				}
			}
			lbSucceeded = true; // OK!
		}
	}

	MCHKHEAP;
	
wrap:
	if (lbSucceeded && sDefaultName[0] == 0)
	{
		_ASSERTE(sDefaultName[0] != 0);
		lbSucceeded = false;
	}
	if (!lbSucceeded)
	{
		expFolder->Release();
		SafeDelete(expFolder);
	}
	else
	{
		if (psDefaultName)
		{
			CopyFileName(psDefaultName, nMaxDefaultLen, sDefaultName);
		}
	}
	
	#ifdef _UNICODE
	SafeFree(item);
	#endif

	MCHKHEAP;
	
	return expFolder;
}

void REPlugin::RedrawPanel(RegItem* pNewSel /*= NULL*/)
{
	PanelRedrawInfo pri = {};
	#if FAR_UNICODE>=3000
	pri.StructSize = sizeof(pri);
	#endif
	PanelRedrawInfo* ppri = NULL;
	if (pNewSel == (RegItem*)1)
	{
		ppri = &pri;
	}
	else if (pNewSel)
	{
		// Нужно получить содержимое панели (сортированное), чтобы найти нужный элемент
		RegItem* pItem = NULL;
		PanelInfo inf = {sizeof(inf)};
		INT_PTR nCurLen = 0, nMaxLen = 0;
		PluginPanelItem* item=NULL;

		if (psiControl((HANDLE)this, FCTL_GETPANELINFO, F757NA &inf))
		{
			ppri = &pri;
			pri.TopPanelItem = inf.TopPanelItem;

			for (INT_PTR i = 0; i < (INT_PTR)inf.ItemsNumber; i++)
			{
				#ifdef _UNICODE
				nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, i, NULL);
				if (!item || nCurLen > nMaxLen)
				{
					SafeFree(item);
					nMaxLen = nCurLen + 512;
					item = (PluginPanelItem*)calloc(nMaxLen,1);
				}
				#if FAR_UNICODE>=3000
				FarGetPluginPanelItem gppi = {sizeof(gppi), nCurLen, item};
				nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, i, (FarDlgProcParam2)&gppi);
				#elif FAR_UNICODE>=1906
				FarGetPluginPanelItem gppi = {nCurLen, item};
				nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, i, (FarDlgProcParam2)&gppi);
				#else
				nCurLen = psiControl((HANDLE)this, FCTL_GETPANELITEM, i, (FarDlgProcParam2)item);
				#endif
				if (!nCurLen)
				{
					continue;
				}
				#else
				item = &(inf.PanelItems[i]);
				#endif

				pItem = (RegItem*)PanelItemUserData(*item);
				if (pItem && pItem->nMagic == REGEDIT_MAGIC) {
					if (pItem->bDefaultValue && pNewSel->bDefaultValue)
					{
						pri.CurrentItem = i; break;
					} else {
						int iCmp;
						iCmp = lstrcmpiW(pItem->pszName, pNewSel->pszName);
						if (iCmp == 0)
						{
							pri.CurrentItem = i; break;
						}
					}
				}
			}
		}

		#ifdef _UNICODE
		SafeFree(item);
		#endif
	}

	// Теперь - перерисовываем, с возможным перепозиционированием курсора
	psiControl((HANDLE)this, FCTL_REDRAWPANEL, F757NA ppri);
}

void REPlugin::UpdatePanel(bool abResetSelection)
{
	MCHKHEAP;
	#ifdef _UNICODE
		psiControl((HANDLE)this, FCTL_UPDATEPANEL, abResetSelection ? 0 : 1, NULL);
	#else
		psiControl((HANDLE)this, FCTL_UPDATEPANEL, (void*)(abResetSelection ? 0 : 1));
	#endif
	MCHKHEAP;
}

void REPlugin::OnIdle()
{
	if (mp_Items)
	{
		MCHKHEAP;
		// Разрешить мониторинг изменений реестра (ключа, открытого на панели)
		mp_Items->MonitorRegistryChange(TRUE);

		_ASSERTE(mp_Items!=NULL);

		// cfg->bRefreshChanges; 0-только по CtrlR, 1-автоматически
		if (cfg->bRefreshChanges && mp_Items && mp_Items->CheckRegistryChanged(this, Worker()))
		{
			mp_Items->ResetRegistryChanged();
			SetForceReload();
			UpdatePanel(false);
			RedrawPanel();
		}
		// А этот флаг выставляется когда завершает свою работу фоновая нить чтения описаний подключей
		else if (mp_Items && mp_Items->CheckLoadingFinished())
		{
			mp_Items->ResetLoadingFinished();
			UpdatePanel(false);
			RedrawPanel();
		}
		MCHKHEAP;
	}
}

int REPlugin::Compare(const struct PluginPanelItem *Item1, const struct PluginPanelItem *Item2, unsigned int Mode)
{
//#ifndef _UNICODE
//	if (mb_SortingSelfDisabled) {
//		return (Item1 < Item2) ? -1 : 1;
//	}
//#endif

	//int iRc = -2;
	switch (Mode)
	{
	case SM_DEFAULT:
	case SM_NAME:
	case SM_EXT:
		{
			RegItem *pItem1 = (RegItem*)PanelItemUserData(*Item1);
			if (!pItem1 || pItem1->nMagic != REGEDIT_MAGIC) return -2;
			RegItem *pItem2 = (RegItem*)PanelItemUserData(*Item2);
			if (!pItem2 || pItem2->nMagic != REGEDIT_MAGIC) return -2;

			//if ((pItem1->nValueType == REG__KEY) && (pItem2->nValueType == REG__KEY))
			//{
			//	if (Item1->FindData.dwFileAttributes != Item2->FindData.dwFileAttributes)
			//	{
			//		return (Item1->FindData.dwFileAttributes & REG_ATTRIBUTE_DELETED) ? -1 : 1;
			//	}
			//}

			if (!cfg->bShowKeysAsDirs)
			{
				if ((pItem1->nValueType == REG__KEY) != (pItem2->nValueType == REG__KEY))
					return (pItem1->nValueType == REG__KEY) ? -1 : 1;
			}

			if (pItem1->bDefaultValue != pItem2->bDefaultValue)
			{
				return pItem1->bDefaultValue ? -1 : 1;
			}
			else
			{
				// Сначала посмотрим, нужно ли нам вообще сравнивать строки
				if ((pItem1->nValueType == REG__KEY) == (pItem2->nValueType == REG__KEY))
				{
					// Если в панели отображены два одноименных ключа/значения (один - пометка удален)
					// то первым показываем удаленный
					if (PanelItemAttributes(*Item1) != PanelItemAttributes(*Item2))
					{
						int iRc;
						iRc = lstrcmpi(PanelItemFileNamePtr(*Item1), PanelItemFileNamePtr(*Item2));
						// Да, ключи одноименные
						if (iRc == 0)
						{
							return (PanelItemAttributes(*Item1) & REG_ATTRIBUTE_DELETED) ? -1 : 1;
						}
					}
				}
				//return (iRc<0) ? -1 : ((iRc>0) ? 1 : 0);
				return -2; // Use FAR default sorting
			}
		}
	}

	if (!cfg->bShowKeysAsDirs)
	{
		RegItem *pItem1 = (RegItem*)PanelItemUserData(*Item1);
		if (!pItem1 || pItem1->nMagic != REGEDIT_MAGIC) return -2;
		RegItem *pItem2 = (RegItem*)PanelItemUserData(*Item2);
		if (!pItem2 || pItem2->nMagic != REGEDIT_MAGIC) return -2;

		if ((pItem1->nValueType == REG__KEY) != (pItem2->nValueType == REG__KEY))
			return (pItem1->nValueType == REG__KEY) ? -1 : 1;
	}

	return -2; // Use FAR default sorting
}

//class DwordDialogBuilder;
//DwordDialogBuilder* pDwordDlg = NULL;

class DwordDialogBuilder : public PluginDialogBuilder
{
public:
	int nFirstBaseId;
	int nValueId;
	int nCurrentBase, nValueSize;
	int nValueSizeId, nAltValueId;
	TCHAR sValueAlt[32];

public:
	DwordDialogBuilder(int TitleMessageID, const TCHAR *aHelpTopic)
		: PluginDialogBuilder(psi, TitleMessageID, aHelpTopic, &guid_DwordDialogBuilder)
	{
		//pDwordDlg = this;
	}

	virtual ~DwordDialogBuilder()
	{
		//pDwordDlg = NULL;
	}

	TCHAR* FormatNumber(TCHAR* pszText, TCHAR* pszAlt, u64 nValue64)
	{
		if (nValueSize == 8)
		{
			BOOL lbLarge = (nValue64 > 0xFFFFFFFFLL);
			if (nCurrentBase == 0)
			{
				_tcsprintf(pszText, lbLarge ? _T("%016I64x") : _T("%08I64x"), nValue64);
				if (pszAlt) _tcsprintf(pszAlt, _T(" = %I64u"), nValue64);
			}
			else if (nCurrentBase == 1)
			{
				_tcsprintf(pszText, _T("%I64u"), nValue64);
				if (pszAlt) _tcsprintf(pszAlt, lbLarge ? _T(" = 0x%016I64X") : _T(" = 0x%08I64X"), nValue64);
			}
			else
			{
				_tcsprintf(pszText, _T("%I64i"), (__int64)nValue64);
				if (pszAlt) _tcsprintf(pszAlt, lbLarge ? _T(" = 0x%016I64X") : _T(" = 0x%08I64X"), nValue64);
			}
		}
		else if (nValueSize == 4)
		{
			if (nCurrentBase == 0)
			{
				wsprintf(pszText, _T("%08x"), (DWORD)(nValue64 & 0xFFFFFFFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = %u"), (DWORD)(nValue64 & 0xFFFFFFFF));
			}
			else if (nCurrentBase == 1)
			{
				wsprintf(pszText, _T("%u"), (DWORD)(nValue64 & 0xFFFFFFFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = 0x%08X"), (DWORD)(nValue64 & 0xFFFFFFFF));
			}
			else
			{
				wsprintf(pszText, _T("%i"), (int)(DWORD)(nValue64 & 0xFFFFFFFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = 0x%08X"), (DWORD)(nValue64 & 0xFFFFFFFF));
			}
		}
		else if (nValueSize == 2)
		{
			if (nCurrentBase == 0)
			{
				wsprintf(pszText, _T("%04x"), (DWORD)(nValue64 & 0xFFFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = %u"), (DWORD)(nValue64 & 0xFFFF));
			}
			else if (nCurrentBase == 1)
			{
				wsprintf(pszText, _T("%u"), (DWORD)(nValue64 & 0xFFFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = 0x%04X"), (DWORD)(nValue64 & 0xFFFF));
			}
			else
			{
				wsprintf(pszText, _T("%i"), (int)(short)(WORD)(nValue64 & 0xFFFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = 0x%04X"), (DWORD)(nValue64 & 0xFFFF));
			}
		}
		else if (nValueSize == 1)
		{
			if (nCurrentBase == 0)
			{
				wsprintf(pszText, _T("%02x"), (DWORD)(nValue64 & 0xFF));
				wsprintf(pszAlt, _T(" = %u"), (DWORD)(nValue64 & 0xFF));
			}
			else if (nCurrentBase == 1)
			{
				wsprintf(pszText, _T("%u"), (DWORD)(nValue64 & 0xFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = 0x%02X"), (DWORD)(nValue64 & 0xFF));
			}
			else
			{
				wsprintf(pszText, _T("%i"), (char)(BYTE)(nValue64 & 0xFF));
				if (pszAlt) wsprintf(pszAlt, _T(" = 0x%02X"), (DWORD)(nValue64 & 0xFF));
			}
		}
		else
		{
			_ASSERTE(nValueSize == 1 || nValueSize == 2 || nValueSize == 4 || nValueSize == 8);
			return NULL;
		}
		return pszText;
	}

	void UpdateValues(HANDLE hDlg, int nNewBase)
	{
		// Сменить значение
		const TCHAR* pszText = NULL;
		#ifdef _UNICODE
		pszText = ((const TCHAR *)psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,nValueId,0));
		#else
		LONG_PTR nLen = psi.SendDlgMessage(hDlg,DM_GETTEXTLENGTH,nValueId,0);
		TCHAR *pszData = (TCHAR*)calloc(nLen+1,1);
		psi.SendDlgMessage(hDlg,DM_GETTEXTPTR,nValueId,(FarDlgProcParam2)pszData);
		pszText = pszData;
#endif

		TCHAR sNewValue[32], sNewValueAlt[64];
		while (*pszText == L' ') pszText++;
		int nOldBase = nCurrentBase;
		nCurrentBase = nNewBase;
		if (*pszText == 0)
		{
			FormatNumber(sNewValue, sNewValueAlt, 0);
		}
		else
		{
			TCHAR* endptr;
			if (nValueSize == 8)
			{
				if (nOldBase == 0)
				{
					u64 nValue = _tcstoui64(pszText, &endptr, 16);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else if (nOldBase == 1)
				{
					u64 nValue = _tcstoui64(pszText, &endptr, 10);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else
				{
					__int64 nValue = _tcstoi64(pszText, &endptr, 10);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
			}
			else if (nValueSize == 4)
			{
				if (nOldBase == 0)
				{
					DWORD nValue = _tcstoul(pszText, &endptr, 16);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else if (nOldBase == 1)
				{
					DWORD nValue = _tcstoul(pszText, &endptr, 10);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else
				{
					int nValue = _tcstol(pszText, &endptr, 10);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
			}
			else if (nValueSize == 2)
			{
				if (nOldBase == 0)
				{
					WORD nValue = (WORD)(_tcstoul(pszText, &endptr, 16) & 0xFFFF);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else if (nOldBase == 1)
				{
					WORD nValue = (WORD)(_tcstoul(pszText, &endptr, 10) & 0xFFFF);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else
				{
					short nValue = (short)(_tcstol(pszText, &endptr, 10) & 0xFFFF);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
			}
			else
			{
				if (nOldBase == 0)
				{
					BYTE nValue = (BYTE)(_tcstoul(pszText, &endptr, 16) & 0xFF);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else if (nOldBase == 1)
				{
					BYTE nValue = (BYTE)(_tcstoul(pszText, &endptr, 10) & 0xFF);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
				else
				{
					char nValue = (char)(_tcstol(pszText, &endptr, 10) & 0xFF);
					FormatNumber(sNewValue, sNewValueAlt, nValue);
				}
			}
		}
		if (nNewBase != nOldBase)
		{
			psi.SendDlgMessage(hDlg,DM_SETTEXTPTR,nValueId,(FarDlgProcParam2)sNewValue);
		}
		else
		{
			const TCHAR* psz1 = pszText; const TCHAR* psz2 = sNewValue;
			while (*psz1 == _T('0') || *psz1 == _T(' ')) psz1++;
			while (*psz2 == _T('0') || *psz2 == _T(' ')) psz2++;
			if (lstrcmpi(psz1, psz2))
				lstrcat(sNewValueAlt, GetMsg(REValueErrorLabel));
		}
		psi.SendDlgMessage(hDlg,DM_SETTEXTPTR,nAltValueId,(FarDlgProcParam2)sNewValueAlt);
		//BUG: Far ставит курсор на новый элемент, но думает, что фокус еще в радио :(
		//psi.SendDlgMessage(hDlg,DM_SETFOCUS,nValueId,0);
		#ifndef _UNICODE
		SafeFree(pszData);
		#endif
	}

	//static LONG_PTR WINAPI DwordDialogBuilderProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
	virtual BOOL PluginDialogProc(HANDLE hDlg, int Msg, int Param1, FarDlgProcParam2 Param2, LONG_PTR& lResult)
	{
		if (Msg == DN_INITDIALOG)
		{
			TCHAR sSize[64]; wsprintf(sSize, GetMsg(REValueSizeStatic), this->nValueSize);
			psi.SendDlgMessage(hDlg,DM_SETTEXTPTR,this->nValueSizeId,(FarDlgProcParam2)sSize);
			psi.SendDlgMessage(hDlg,DM_SETTEXTPTR,this->nAltValueId,(FarDlgProcParam2)this->sValueAlt);
		}
		else if (Msg == DN_EDITCHANGE)
		{
			if (Param1 == this->nValueId)
			{
				// Обновить Alt-значение
				this->UpdateValues(hDlg, this->nCurrentBase);
			}
		}
		else if (Msg == DN_BTNCLICK)
		{
			_ASSERTE(this!=NULL);
			if (Param1 >= this->nFirstBaseId && Param1 <= (this->nFirstBaseId + 2) && Param2)
			{
				// Сменить значение
				this->UpdateValues(hDlg, (Param1 - this->nFirstBaseId));
			}
		}

		return FALSE; // позвать psi.DefDlgProc(hDlg, Msg, Param1, Param2);
	}

	//virtual int DoShowDialog()
	//{
	//	int Width = DialogItems [0].X2+4;
	//	int Height = DialogItems [0].Y2+2;
	//	#ifdef UNICODE
	//		DialogHandle = Info.DialogInit(PluginNumber, -1, -1, Width, Height,
	//			HelpTopic, DialogItems, DialogItemsCount, 0, 0, DwordDialogBuilderProc, (FarDlgProcParam2)this);
	//		return Info.DialogRun(DialogHandle);
	//	#else
	//		return Info.DialogEx(PluginNumber, -1, -1, Width, Height,
	//			HelpTopic, DialogItems, DialogItemsCount, 0, 0, DwordDialogBuilderProc, (FarDlgProcParam2)this);
	//	#endif
	//}
};

// Вернуть TRUE если нажата кнопка OK
BOOL REPlugin::EditNumber(wchar_t** pName, LPVOID pNumber, REGTYPE* pnDataType, DWORD cbSize, BOOL bNewValue)
{
	DwordDialogBuilder Edt(REChangeNumber, _T("ChangeNumber"));
	FarDialogItem* p = NULL;
	FarDialogItem* pNameCtrl = NULL;

	MCHKHEAP;

	TCHAR sValue[32], sName[0x4000], sCurName[0x4000];
	//DialogBuilderListItem TypesListItems[] =
	//{
	//	{ RE_REG_BINARY, REG_BINARY },
	//	{ RE_REG_DWORD,  REG_DWORD  },
	//};
	int nValueBase = 0; // Hex
	int nBases[] = {
		REValueBaseHex,
		REValueBaseUnsigned,
		REValueBaseSigned,
	};
	int nValueType = ((*pnDataType) == REG_BINARY) ? 0 : 1;
	int nTypes[] = {
		RE_REG_BINARY,
		(cbSize == 8) ? RE_REG_QWORD : RE_REG_DWORD,
		//RE_REG_QWORD,
	};
	Edt.nCurrentBase = nValueBase;
	Edt.nValueSize = cbSize;
	
	if (cbSize == 8)
	{
		if (*((u64*)pNumber) > 0xFFFFFFFFLL)
			_tcsprintf(sValue, _T("%016I64x"), *((u64*)pNumber));
		else
			_tcsprintf(sValue, _T("%08I64x"), *((u64*)pNumber));
		_tcsprintf(Edt.sValueAlt, _T(" = %I64u"), *((u64*)pNumber));
	}
	else if (cbSize == 4)
	{
		wsprintf(sValue, _T("%08x"), *((DWORD*)pNumber));
		wsprintf(Edt.sValueAlt, _T(" = %u"), *((DWORD*)pNumber));
	}
	else if (cbSize == 2)
	{
		wsprintf(sValue, _T("%04x"), (DWORD)*((WORD*)pNumber));
		wsprintf(Edt.sValueAlt, _T(" = %u"), (DWORD)*((WORD*)pNumber));
	}
	else if (cbSize == 1)
	{
		wsprintf(sValue, _T("%02x"), (DWORD)*((BYTE*)pNumber));
		wsprintf(Edt.sValueAlt, _T(" = %u"), (DWORD)*((BYTE*)pNumber));
	}
	else
	{
		_ASSERTE(cbSize == 1 || cbSize == 2 || cbSize == 4 || cbSize == 8);
		return FALSE;
	}

	BOOL bDefaultValue = (pName && *pName == 0);

	lstrcpy_t(sName, countof(sName), (pName && *pName) ? *pName : REGEDIT_DEFAULTNAME);
	lstrcpy(sCurName, sName);

	Edt.AddText(REValueNameLabel);
	if (bNewValue)
	{
		pNameCtrl = Edt.AddEditField(sName, countof(sName), 56, _T("RegistryValueName"));
		//pNameCtrl->Flags |= DIF_READONLY;
	}
	else
	{
		pNameCtrl = Edt.AddText(sName);
		pNameCtrl->X2 = 61;
		//pNameCtrl = Edt.AddEditField(sName, countof(sName), 56, _T("RegistryValueName"));
		//pNameCtrl->Flags |= DIF_DISABLE;
	}

	//p = Edt.AddComboBox((int *) &nDataType, 12, TypesListItems, countof(TypesListItems),
	//	DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE|((cbSize == 4) ? 0 :DIF_DISABLE));
	//Edt.AddTextBefore(p, REValueTypeLabel);

	Edt.AddSeparator();

	Edt.StartColumns();
	Edt.AddText(REValueTypeLabel);
	Edt.AddRadioButtons(&nValueType, countof(nTypes), nTypes, false, ((cbSize == 4 || cbSize == 8) ? 0 : DIF_DISABLE));
	p = Edt.AddText(REValueSizeStatic);
	Edt.nValueSizeId = Edt.GetItemIndex(p);
	Edt.ColumnBreak();
	Edt.AddText(REValueBaseLabel);
	Edt.nFirstBaseId = Edt.AddRadioButtons(&nValueBase, countof(nBases), nBases);
	Edt.EndColumns();

	Edt.AddSeparator(REValueDataLabel);
	//Edt.StartColumns();
	p = Edt.AddEditField(sValue, 21, 22);
	Edt.nValueId = Edt.GetItemIndex(p);
	Edt.InitFocus(p);
	//Edt.ColumnBreak();
	p = Edt.AddText(REValueShowAltStatic);
	p->X2 = 40;
	Edt.nAltValueId = Edt.GetItemIndex(p);
	Edt.MoveItemAfter(Edt.GetItemByIndex(Edt.nValueId), p);
	//Edt.EndColumns();

	Edt.AddOKCancel(REBtnOK, REBtnCancel);
	BOOL lbOk = FALSE;
	while (true)
	{
		lbOk = Edt.ShowDialog();
		if (!bNewValue && lbOk && sName[0] == 0)
		{
			int nErr = Message(REM_NumberRequireName, FMSG_WARNING|FMSG_ALLINONE, 2, _T("ChangeNumber"));
			if (nErr != 0)
			{
				lbOk = FALSE; break;
			}
			Edt.InitFocus(pNameCtrl);
			continue;
		}
		break;
	}
	MCHKHEAP;
	if (lbOk)
	{
		if (bNewValue)
		{
			if (pName && *pName)
			{
				// Если пользователь изменил имя - его нужно вернуть в *pName
				if (lstrcmp(sCurName, sName))
				{
					SafeFree(*pName);
				}
			}
			if (*pName == NULL)
			{
				if (bDefaultValue && sName[0] && lstrcmp(sName, REGEDIT_DEFAULTNAME_T) == 0)
					sName[0] = 0;
				int nLen = lstrlen(sName);
				*pName = (wchar_t*)malloc((nLen+1)*2);
				lstrcpy_t(*pName, nLen+1, sName);
			}
		}

		// Для sizeof(DWORD) допустимо менять REG_BINARY <--> REG_DWORD
		if (cbSize == 4)
		{
			*pnDataType = (nValueType == 0) ? REG_BINARY : REG_DWORD;
		}
		else if (cbSize == 8)
		{
			*pnDataType = (nValueType == 0) ? REG_BINARY : REG_QWORD;
		}

		const TCHAR* pszText = sValue;
		while (*pszText == L' ') pszText++;
		if (*pszText == 0)
		{
			if (cbSize == 8)
				*((u64*)pNumber) = 0;
			else if (cbSize == 4)
				*((LPDWORD)pNumber) = 0;
			else if (cbSize == 2)
				*((LPWORD)pNumber) = 0;
			else
				*((LPBYTE)pNumber) = 0;
		}
		else
		{
			TCHAR* endptr;
			if (cbSize == 8)
			{
				if (nValueBase == 0)
					*((u64*)pNumber) = _tcstoui64(pszText, &endptr, 16);
				else if (nValueBase == 1)
					*((u64*)pNumber) = _tcstoui64(pszText, &endptr, 10);
				else
					*((__int64*)pNumber) = _tcstoi64(pszText, &endptr, 10);

			}
			else if (cbSize == 4)
			{
				if (nValueBase == 0)
					*((DWORD*)pNumber) = _tcstoul(pszText, &endptr, 16);
				else if (nValueBase == 1)
					*((DWORD*)pNumber) = _tcstoul(pszText, &endptr, 10);
				else
					*((int*)pNumber) = _tcstol(pszText, &endptr, 10);

			}
			else if (cbSize == 2)
			{
				if (nValueBase == 0)
					*((WORD*)pNumber) = (WORD)(_tcstoul(pszText, &endptr, 16) & 0xFFFF);
				else if (nValueBase == 1)
					*((WORD*)pNumber) = (WORD)(_tcstoul(pszText, &endptr, 10) & 0xFFFF);
				else
					*((short*)pNumber) = (short)(_tcstol(pszText, &endptr, 10) & 0xFFFF);

			}
			else
			{
				if (nValueBase == 0)
					*((BYTE*)pNumber) = (BYTE)(_tcstoul(pszText, &endptr, 16) & 0xFF);
				else if (nValueBase == 1)
					*((BYTE*)pNumber) = (BYTE)(_tcstoul(pszText, &endptr, 10) & 0xFF);
				else
					*((char*)pNumber) = (char)(_tcstol(pszText, &endptr, 10) & 0xFF);
			}
		}
	}

	MCHKHEAP;
	return lbOk;
}

// Вернуть TRUE если текст был изменен
BOOL REPlugin::EditString(wchar_t* pName, wchar_t** pText, REGTYPE nDataType, DWORD* cbSize, BOOL bNewValue /*= FALSE*/)
{
	//_ASSERTE(pName != NULL); // А вот *pName может быть == NULL
	MCHKHEAP;

	BOOL lbExportRc = FALSE;
	BOOL lbDataChanged = FALSE;
	
	mb_InMacroEdit = FALSE;
	BOOL lbIsMacro = FALSE;
	#ifdef _UNICODE
	// Проверить в m_Key.mpsz_Key наличие ключа "\\KeyMacros\\"
	if ((cfg->bCheckMacroSequences || cfg->bCheckMacrosInVars) &&
		m_Key.mpsz_Key && *m_Key.mpsz_Key && pName && *pName && lstrcmpi(pName, REGEDIT_DEFAULTNAME))
	{
		wchar_t* pszUpper = lstrdup(m_Key.mpsz_Key);
		CharUpperBuff(pszUpper, lstrlen(pszUpper));

		wchar_t* pszTest = pszUpper;
		// Для *.reg - тут у нас "HKEYxxxx\SOFTWARE\..."
		if (pszTest[0] == L'H' && pszTest[1] == 'K')
		{
			pszTest = wcschr(pszUpper, L'\\');
			if (pszTest) pszTest++;
		}
		if (pszTest && wmemcmp(pszTest, L"SOFTWARE\\FAR\\", 13) != 0)
		{
			LPCWSTR pszSubkey = wcsstr(pszTest, L"\\KEYMACROS\\");
			if (pszSubkey)
			{
				if (pName[0]==L'%' && pName[1]==L'%' && pName[2])
					lbIsMacro = cfg->bCheckMacrosInVars;
				else if (lstrcmpi(pName, L"Sequence") == 0)
					lbIsMacro = cfg->bCheckMacroSequences;
			}
		}
		
		SafeFree(pszUpper);
	}
	#endif
	
	BOOL lbUseUnicode = 
		#ifdef _UNICODE
		(nDataType == REG_SZ || nDataType == REG_EXPAND_SZ || nDataType == REG_MULTI_SZ)
			? cfg->bCreateUnicodeFiles : FALSE
		#else
		FALSE /* в FAR 1.7x - строки только в ANSI! */
		#endif
			;
	
	MFileTxtReg file(mb_Wow64on32, mb_Virtualize);
	if (file.FileCreateTemp(pName, L"", lbUseUnicode))
	{
		if (bNewValue)
		{
			lbExportRc = TRUE; // Новое значение - пока пустое!
		}
		else if (nDataType == REG_SZ || nDataType == REG_EXPAND_SZ)
		{
			//TODO: а для остальных типов - проверить наличие \0?
			DWORD nLen = (*cbSize) >> 1;
			// Сразу отрежем завершающий 0, если он есть
			if (nLen > 0 && (*pText)[nLen-1] == 0)
				nLen --;
			lbExportRc = nLen ? file.FileWrite(*pText, nLen) : TRUE;
		}
		else if (nDataType == REG_MULTI_SZ)
		{
			// Обработать REG_MULTI_SZ - заменить все \0 на \r\n
			lbExportRc = file.FileWriteMSZ(*pText, (*cbSize) >> 1);
			//wchar_t* psz = *pText;
			//wchar_t* pszEnd = (wchar_t*)(((LPBYTE)*pText) + *cbSize);
			//#ifdef _DEBUG
			//if (pszEnd > psz) {
			//	_ASSERTE(*(pszEnd-1) == 0);
			//}
			//#endif
			//lbExportRc = TRUE; // Может быть пустой!
			//while (psz < pszEnd)
			//{
			//	if (*psz) {
			//		int nLen = lstrlenW(psz);
			//		if (!(lbExportRc = file.FileWrite(psz, nLen)))
			//			break; // ошибка записи
			//		psz += nLen+1;
			//	} else {
			//		if ((psz+1) >= pszEnd)
			//			break;
			//		psz++; // просто записать перевод строки
			//	}
			//	if (!(lbExportRc = file.FileWrite(L"\r\n")))
			//		break; // ошибка записи
			//}
			////// Сразу отрежем завершающий 0, если он есть
			////if (nDataType != REG_MULTI_SZ && nLen > 0 && (*pText)[nLen-1] == 0)
			////	nLen --;
			////lbExportRc = file.FileWrite(*pText, nLen);
		}
		else
		{
			//
			lbExportRc = file.FileWriteBuffered(*pText, *cbSize);
		}
		
		file.FileClose();
		
		if (!lbExportRc)
			file.FileDelete();
	}	

	MCHKHEAP;

	if (lbExportRc)
	{
		TCHAR sTitle[MAX_PATH+1]; lstrcpy_t(sTitle, countof(sTitle), pName ? pName : REGEDIT_DEFAULTNAME);
		mb_InMacroEdit = lbIsMacro;
		int nEdtRc = psi.Editor(file.GetShowFilePathName(), sTitle, 0,0,-1,-1,
			EF_DISABLEHISTORY/*EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONCLOSE|EF_ENABLE_F6*/,
			#ifdef _UNICODE
			1, 1, cfg->EditorCodePage()
			#else
			0, 1
			#endif
			);
		MCHKHEAP;
		mb_InMacroEdit = FALSE;
		// Применить изменения в **psText
		if (nEdtRc == EEC_MODIFIED)
		{
			LONG     lLoadRc = -1;
			wchar_t* pszNewText = NULL;
			DWORD    cbNewSize = 0;
			if (nDataType == REG_SZ || nDataType == REG_EXPAND_SZ)
				lLoadRc = MFileTxt::LoadText(file.GetFilePathName(), lbUseUnicode, &pszNewText, &cbNewSize);
			else if (nDataType == REG_MULTI_SZ)
				lLoadRc = MFileTxt::LoadTextMSZ(file.GetFilePathName(), lbUseUnicode, &pszNewText, &cbNewSize);
			else
				lLoadRc = MFileTxt::LoadData(file.GetFilePathName(), (void**)&pszNewText, &cbNewSize);
			if (lLoadRc != 0)
			{
				//TODO: Показать ошибку
				InvalidOp();
			}
			else
			{
				//if (nDataType == REG_MULTI_SZ)
				//{
				//	//TODO: Провеить тип перевода строк в редакторе?
				//	// Преобразовать '\r\n' в '\0'
				//	//if (lbUseUnicode)
				//	{
				//		wchar_t* pszBase = pszNewText;
				//		wchar_t* pszEnd = (wchar_t*)(((LPBYTE)pszNewText)+cbNewSize);
				//		wchar_t* pszDst = pszBase;
				//		wchar_t* pszLnStart = pszBase;
				//		wchar_t* pszLnEnd;
				//		while (pszLnStart < pszEnd)
				//		{
				//			if (*pszLnStart == 0 && (pszLnStart+1) >= pszEnd)
				//			{
				//				break;
				//			}
				//			pszLnEnd = pszLnStart;
				//			while (pszLnEnd < pszEnd)
				//			{
				//				if (*pszLnEnd == L'\r' && *(pszLnEnd+1) == L'\n') {
				//					*pszLnEnd = 0;
				//					break;
				//				}
				//				pszLnEnd++;
				//			}
				//
				//			if (pszDst != pszLnStart) {
				//				_ASSERTE(pszDst < pszLnStart);
				//				memmove(pszDst, pszLnStart, (pszLnEnd - pszLnStart + 1)*2);
				//			}
				//			pszDst += (pszLnEnd - pszLnStart)+1;
				//			pszLnStart = pszLnEnd+2;
				//		}
				//		_ASSERTE(pszDst < pszEnd);
				//		*pszDst = 0;
				//		cbNewSize = (pszDst - pszBase + 1)*2;
				//	}
				//	//else {
				//	//	//TODO:
				//	//}
				//}
				if (bNewValue || cbNewSize != *cbSize)
					lbDataChanged = TRUE;
				else if (memcmp(pszNewText, *pText, cbNewSize) != 0)
					lbDataChanged = TRUE;
				SafeFree(*pText);
				*pText = pszNewText; pszNewText = NULL;
				*cbSize = cbNewSize;
			}
			SafeFree(pszNewText);
		}
		MCHKHEAP;

		// И удалить файл и папку
		file.FileDelete();
	}

	MCHKHEAP;
	return lbDataChanged;
}

void REPlugin::EditValueRaw(RegFolder* pFolder, RegItem *pItem, bool abViewOnly)
{
	_ASSERTE(pFolder && pItem && pItem->pszName);

	LONG hRc = 0;
	DWORD nDataSize = 0;
	REGTYPE nDataType;
	LPBYTE pValue = NULL;
	TCHAR sTitle[MAX_PATH+1];
	MFileTxtReg file(mb_Wow64on32, mb_Virtualize);

	MCHKHEAP;
	
	hRc = ValueDataGet(pFolder, pItem, &pValue, &nDataSize, &nDataType);
	if (hRc != 0)
	{
		goto wrap;
	}
	
	
	// Поскольку RAW - то никакого Unicode BOM
	if (!file.FileCreateTemp(pItem->pszName, L"", FALSE))
		goto wrap;
	if (!file.FileWriteBuffered(pValue, nDataSize))
		goto wrap;
	file.FileClose();
	SafeFree(pValue);
	

	
	lstrcpy_t(sTitle, countof(sTitle), pItem->pszName ? pItem->pszName : REGEDIT_DEFAULTNAME);
	if (!abViewOnly)
	{
		int nEdtRc = psi.Editor(file.GetShowFilePathName(), sTitle, 0,0,-1,-1,
			EF_DISABLEHISTORY/*EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONCLOSE|EF_ENABLE_F6*/,
			#ifdef _UNICODE
			1, 1, FARCP_AUTODETECT
			#else
			0, 1
			#endif
		);
		if (nEdtRc == EEC_MODIFIED)
		{
			// Применение изменений, следанных в редакторе
			LONG     lLoadRc = -1;
			//DWORD    cbNewSize = 0;
			lLoadRc = MFileTxt::LoadData(file.GetFilePathName(), (void**)&pValue, &nDataSize);
			if (lLoadRc != 0) {
				InvalidOp();
			} else {
				ValueDataSet(pFolder, pItem, pValue, nDataSize, nDataType);
			}
		}
	} else {
		psi.Viewer(file.GetShowFilePathName(), sTitle, 0,0,-1,-1,
			VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY
			#ifdef _UNICODE
			, FARCP_AUTODETECT
			#endif
			);
	}
	
	
	SetForceReload();
	UpdatePanel(false);
	RedrawPanel(NULL);
wrap:
	file.FileDelete(); // временный файл
	SafeFree(pValue);  // буфер
}

// Returns:
//   1 - ok, изменения, нужно обновить панель
//	 0 - изменений нет, но редактирование прошло успешно, или отменено пользователем
//  -1 - типа данных не поддерживается, вывалить простыню hex-ов
int REPlugin::EditValue(RegFolder* pFolder, RegItem *pItem, REGTYPE nNewValueType /*= 0*/)
{
	MCHKHEAP;
	int liRc = -1;
	BOOL lbApplyData = FALSE;
	//MRegistryBase* pWorker = Worker();
	//HREGKEY hKey = NULL;
	DWORD nDataSize = sizeof(DWORD);
	REGTYPE nDataType; //, nValue;
	LPWSTR pszValueName = pItem->bDefaultValue ? NULL : lstrdup(pItem->pszName);
	LPBYTE pValue = (LPBYTE)calloc(nDataSize,1);
	//LONG hRc = pFolder->OpenKey(pWorker, &hKey);
	LONG hRc = 0;

	hRc = ValueDataGet(pFolder, pItem, &pValue, &nDataSize, &nDataType);

	MCHKHEAP;

	// При ошибке открытия ключа
	if (hRc != 0 && (nDataType == (DWORD)-1))
	{
		//CantOpenKey(&pFolder->key, FALSE);
		liRc = 0;
		goto wrap;
	}
	
	if (hRc == 0 && nDataType == REG_BINARY && cfg->bEditBinaryAsText 
		&& !(nDataSize==1 || nDataSize==2 || nDataSize==4 || nDataSize==8))
	{
		EditValueRaw(pFolder, pItem, false);
		MCHKHEAP;
		liRc = 0; // панель при необходимости обновит сама EditValueRaw
		goto wrap;
	}

	//hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL,
	//	&nDataType, (LPBYTE)&nValue, &(nDataSize = sizeof(nValue)));
	//if (hRc == ERROR_MORE_DATA)
	//{
	//	pValue = (LPBYTE)calloc(nDataSize+2,1); // +wchar_t (\0) с учетом "кривых" строк в реестре
	//	hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL,
	//		&nDataType, pValue, &nDataSize);
	//}
	
	// Если при создании нового значения такое имя уже есть,
	// спрашиваем подтверждение и проверяем допустимость типа
	if (hRc == 0 && nNewValueType != 0)
	{
		BOOL lbValueFailed = FALSE, lbForceMultiSZ = FALSE;
		// Если преобразование типа невозможно
		if (nNewValueType == nDataType)
		{
			// Если типы и так совпадают - ok
		}
		else if ((nDataSize == 4) &&
			((nNewValueType == REG_BINARY && nDataType == REG_DWORD)
			|| (nNewValueType == REG_DWORD && nDataType == REG_BINARY))
			)
		{
			// BYTE[4] <--> DWORD
			nDataType = nNewValueType;
		}
		else if ((nDataSize == 8) &&
			((nNewValueType == REG_BINARY && nDataType == REG_QWORD)
			|| (nNewValueType == REG_QWORD && nDataType == REG_BINARY))
			)
		{
			// BYTE[8] <--> QWORD
			nDataType = nNewValueType;
		}
		else if ((nNewValueType == REG_SZ || nNewValueType == REG_EXPAND_SZ) && nDataType == REG_MULTI_SZ)
		{
			// чтобы в тексте не появлились левые '\0' - оставим REG_MULTI_SZ
			nNewValueType = REG_MULTI_SZ; lbForceMultiSZ = TRUE;
		}
		else if ((nNewValueType == REG_SZ || nNewValueType == REG_EXPAND_SZ || nNewValueType == REG_MULTI_SZ)
			&& (nDataType == REG_SZ || nDataType == REG_EXPAND_SZ))
		{
			// допустимые преобразования строк
			nDataType = nNewValueType;
		}
		else
		{
			// Преобразование невозможно, сбрасываем содержимое
			hRc = ERROR_FILE_NOT_FOUND;
			lbValueFailed = TRUE;
		}
		MCHKHEAP;
		// Запросить строгое подтверждение на переименование
		TCHAR sValueName[16384]; lstrcpy_t(sValueName, countof(sValueName), pItem->pszName ? pItem->pszName : REGEDIT_DEFAULTNAME);
		TCHAR* pszLines[10]; int nLines = 0;
		pszLines[nLines++] = GetMsg(REM_Warning);
		pszLines[nLines++] = GetMsg(REM_ValueAlreadyExists);
		if (lbValueFailed)
			pszLines[nLines++] = GetMsg(REM_ValueWillBeErased);
		if (lbForceMultiSZ)
			pszLines[nLines++] = GetMsg(REM_ValueForcedToMultiSZ);
		pszLines[nLines++] = sValueName;
		pszLines[nLines++] = GetMsg(REBtnContinue);
		pszLines[nLines++] = GetMsg(REBtnCancel);
		int iBtn = psi.Message(_PluginNumber(guid_Msg10), FMSG_WARNING, _T("NewValue"), pszLines, nLines, 2);
		if (iBtn != 0)
		{
			liRc = 0; // Отмена пользователем
			goto wrap;
		}
		MCHKHEAP;
		
	}

	MCHKHEAP;
	if (hRc != 0 && hRc != ERROR_FILE_NOT_FOUND && nNewValueType != 0)
	{
		// показать ошибку "Нет прав на чтение"
		SetLastError(hRc);
		// Создание нового значения
		if (!ValueOperationFailed(&pFolder->key, pItem->pszName, FALSE, TRUE))
		{
			liRc = -1;
			goto wrap;
		}
		hRc = ERROR_FILE_NOT_FOUND;
	}

	MCHKHEAP;
	if (hRc == ERROR_FILE_NOT_FOUND && nNewValueType != 0)
	{
		*((LPDWORD)pValue) = 0;
		switch (nNewValueType)
		{
		case REG_BINARY:
			nDataSize = 1; hRc = 0; break;
		case REG_DWORD:
			nDataSize = 4; hRc = 0; break;
		case REG_QWORD:
			nDataSize = 8; pValue = (LPBYTE)calloc(8,1); hRc = 0; break;
		case REG_SZ:
		case REG_EXPAND_SZ:
		case REG_MULTI_SZ:
			nDataSize = 2; hRc = 0; break;
		}
		if (hRc == 0)
			nDataType = nNewValueType;
	}

	MCHKHEAP;
	if (hRc != 0)
	{
		// показать ошибку "Значение отсутствует!"
		SetLastError(hRc);
		if (nNewValueType != 0)
		{
			// Ошибка уже показана
			liRc = -1;
			goto wrap;
		} else {
			ValueOperationFailed(&pFolder->key, pItem->pszName, FALSE);
			liRc = -1;
			goto wrap;
		}
	}
	//// Ключ на чтение больше не требуется
	//pFolder->CloseKey(pWorker, hKey); _ASSERTE(hKey==NULL);
	MCHKHEAP;

	if (
		((nDataType == REG_DWORD || nDataType == REG_BINARY) && nDataSize == sizeof(DWORD))
		|| ((nDataType == REG_QWORD || nDataType == REG_BINARY) && nDataSize == sizeof(u64))
		|| (nDataType == REG_BINARY && (nDataSize == sizeof(WORD) || nDataSize == sizeof(BYTE)))
		)
	{
		lbApplyData = EditNumber(&pszValueName, pValue, &nDataType, nDataSize, (nNewValueType!=0));
		MCHKHEAP;
		if (!pszValueName ||!*pszValueName) {
			// Для DWORD/BINARY пустое имя значения недопустимо!
			_ASSERTE(pszValueName && *pszValueName);
			lbApplyData = FALSE;
		}
		if (lbApplyData && (nNewValueType != 0))
		{
			// При изменениях имени значения - нужно его вернуть в буфер.
			// Это нужно для последующего позиционирования, при обновлении панели.
			// Новое значение создается из REPlugin::NewItem(), там RegItem локальный
			// и pszName указывает на sValueName[16384].
			_ASSERTE(pszValueName && !IsBadWritePtr((wchar_t*)pItem->pszName,(lstrlenW(pszValueName)+1)*2));
			// Скопировать
			lstrcpynW((wchar_t*)pItem->pszName, pszValueName, 16384);
			MCHKHEAP;
		}
	}
	else if (nDataType == REG_SZ || nDataType == REG_MULTI_SZ || nDataType == REG_EXPAND_SZ)
	{
		// В редакторе строк - изменение имени невозможно
		MCHKHEAP;
		lbApplyData = EditString(pszValueName, (wchar_t**)&pValue, nDataType, &nDataSize);
		MCHKHEAP;
	}
	else
	{
		// Редактирование этого типа не поддерживается, пусть делается выгрузка в *.reg
		lbApplyData = FALSE;
		liRc = -1;
		goto wrap;
	}

	if (lbApplyData)
	{
		liRc = 0;
		// Имя могло изменится?
		MCHKHEAP;
		hRc = ValueDataSet(pFolder, pszValueName, pValue, nDataSize, nDataType);
		MCHKHEAP;
		if (hRc != 0)
			goto wrap;
		//hRc = pFolder->CreateKey(pWorker, &hKey, KEY_SET_VALUE);
		//if (hRc != 0) {
		//	CantOpenKey(&pFolder->key, TRUE);
		//	goto wrap;
		//}
		//hRc = pWorker->SetValueEx(hKey, pszValueName, 0, nDataType, (LPBYTE)pValue, nDataSize);
		//if (hRc != 0) {
		//	ValueOperationFailed(&pFolder->key, pszValueName, TRUE);
		//	goto wrap;
		//}
		
		// UpdatePanel
		SetForceReload();
		UpdatePanel(false);
		RegItem item = {REGEDIT_MAGIC, nDataType, pszValueName, pItem->bDefaultValue};
		RedrawPanel(&item);
		
		liRc = 1;
	} else {
		liRc = 0; // Отмена пользователем
	}

	MCHKHEAP;
wrap:
	//// закрываем, если открыт остался
	//pFolder->CloseKey(pWorker, hKey);
	SafeFree(pValue);
	if (pszValueName && pszValueName != pItem->pszName)
	{
		SafeFree(pszValueName);
	}
	MCHKHEAP;
	return liRc;
}


BOOL REPlugin::DeleteItems(struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	if (!((m_Key.eType == RE_WINAPI || m_Key.eType == RE_HIVE) && m_Key.mh_Root != NULL)
		&& !(m_Key.eType == RE_REGFILE))
		return FALSE; // В этих случаях - низя
	if (!mp_Items || !mp_Items->key.IsEqual(&m_Key))
	{
		_ASSERTE(mp_Items && mp_Items->key.IsEqual(&m_Key));
		return FALSE;
	}

	//if (psi.Message(PluginNumber, FMSG_WARNING|FMSG_ALLINONE, _T("DeleteItems"),
	//	(const TCHAR * const *)GetMsg(REM_ConfirmDelete), 1, 2) != 0)
	//	return FALSE;
	if (Message(REM_WarningDelete, FMSG_WARNING|FMSG_ALLINONE, 2, _T("DeleteItems")) != 0)
		return FALSE;
	bool bDelConfirm = false;
	#ifdef _UNICODE
		#if FARMANAGERVERSION_BUILD>=2541
			GUID FarGuid = {};
			FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
			if (psi.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
			{
				FarSettingsItem fsi1 = {}, fsi2 = {};
				#if FAR_UNICODE>=3000
				fsi1.StructSize = sizeof(fsi1); fsi2.StructSize = sizeof(fsi2);
				#endif
				fsi1.Root = FSSF_CONFIRMATIONS; fsi1.Name = L"Delete";
				fsi2.Root = FSSF_CONFIRMATIONS; fsi2.Name = L"DeleteFolder";
				if (psi.SettingsControl(sc.Handle, SCTL_GET, 0, &fsi1))
				{
					_ASSERTE(fsi1.Type == FST_QWORD);
					bDelConfirm = (fsi1.Number != 0);
				}
				if (!bDelConfirm && psi.SettingsControl(sc.Handle, SCTL_GET, 0, &fsi2))
				{
					_ASSERTE(fsi2.Type == FST_QWORD);
					bDelConfirm = (fsi2.Number != 0);
				}
				psi.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
			}
		#else
			bDelConfirm = (psi.AdvControl(PluginNumber,ACTL_GETCONFIRMATIONS,FADV1988 NULL)&(FCS_DELETE|FCS_DELETENONEMPTYFOLDERS))!=0;
		#endif
	#else
		bDelConfirm = (psi.AdvControl(PluginNumber,ACTL_GETCONFIRMATIONS,FADV1988 NULL)&(FCS_DELETE|FCS_DELETENONEMPTYFOLDERS))!=0;
	#endif
	if (bDelConfirm)
	{
		int nKeys = 0, nValues = 0;
		for (int i = 0; i < ItemsNumber; i++)
		{
			RegItem* pItem = (RegItem*)PanelItemUserData(PanelItem[i]);
			if (pItem && pItem->nMagic == REGEDIT_MAGIC)
			{
				if (pItem->nValueType == REG__KEY)
					nKeys++;
				else
					nValues++;
			}
		}
		if ((nKeys+nValues) > 1)
		{
			TCHAR sConfirm[0x512];
			wsprintf(sConfirm, GetMsg(REM_ConfirmDelete), nKeys, nValues);
			if (0 != psi.Message(_PluginNumber(guid_Msg11), FMSG_WARNING|FMSG_ALLINONE, _T("DeleteItems"), (const TCHAR * const *)sConfirm, 1, 2))
				return FALSE;
		}
	}

	// Поехали
	BOOL lbRc = FALSE;
	HREGKEY hKey = NULLHKEY;
	MRegistryBase* pWorker = Worker();
	//LONG hRc = pWorker->OpenKeyEx(m_Key.mh_Root, m_Key.mpsz_Key, 0, KEY_ALL_ACCESS, &hKey);
	DWORD samDesired = DELETE;
	// Подобрать минимально необходимые права на ключ
	for (int i = 0; i < ItemsNumber && (samDesired != (DELETE|KEY_ENUMERATE_SUB_KEYS|KEY_SET_VALUE)); i++)
	{
		RegItem* pItem = (RegItem*)PanelItemUserData(PanelItem[i]);
		//BOOL lbContinue = TRUE;
		if (pItem && pItem->nMagic == REGEDIT_MAGIC) {
			if (pItem->nValueType == REG__KEY)
				samDesired |= KEY_ENUMERATE_SUB_KEYS;
			else
				samDesired |= KEY_SET_VALUE;
		}
	}
	LONG hRc = mp_Items->OpenKey(pWorker, &hKey, samDesired);
	if (hRc != 0)
	{
		CantOpenKey(&m_Key, -1);
	}
	else
	{
		//TODO: Добавить прогресс
		for (int i = 0; i < ItemsNumber; i++)
		{
			RegItem* pItem = (RegItem*)PanelItemUserData(PanelItem[i]);
			BOOL lbContinue = TRUE;
			if (pItem && pItem->nMagic == REGEDIT_MAGIC) {
				if (pItem->nValueType == REG__KEY) {
					hRc = pWorker->DeleteSubkeyTree(hKey, pItem->pszName);
				} else {
					hRc = pWorker->DeleteValue(hKey, pItem->bDefaultValue ? L"" : pItem->pszName);
				}
				if (hRc != 0)
				{
					SetLastError(hRc);
					lbContinue = DeleteFailed(&mp_Items->key, pItem->pszName, (pItem->nValueType == REG__KEY), ((i+1)<ItemsNumber));
					if (!lbContinue) {
						// lbRc не сбрасываем, т.к. возможно придется обновить панели (если хоть что-то удалили)
						break;
					}
				} else {
					lbRc = TRUE; // Если удалено хотя бы одно - вернуть TRUE для обновления панелей
				}
			}
		}
		mp_Items->CloseKey(pWorker, &hKey);
	}

	if (lbRc)
		mp_Items->Reset(); // требуется повторное чтение ключа

	return lbRc;
}

// В случае успеха возвращаемое значение должно быть равно 1.
// В случае провала возвращается 0.
// Если функция была прервана пользователем, то должно возвращаться -1. 
int REPlugin::CreateSubkey(const TCHAR* aszSubkey, const TCHAR** pszCreated, u64 OpMode)
{
	//if (m_Key.eType != RE_WINAPI
	//	|| m_Key.mh_Root == NULL)
	if ((m_Key.mh_Root == NULL) && (m_Key.eType != RE_REGFILE))
		return -1; // В этих случаях - создавать низя
	if (!mp_Items || !mp_Items->key.IsEqual(&m_Key))
	{
		_ASSERTE(mp_Items && mp_Items->key.IsEqual(&m_Key));
		return -1;
	}


	lstrcpyn(ms_CreatedSubkeyBuffer, aszSubkey, countof(ms_CreatedSubkeyBuffer));

	// Если разрешено изменение имени создаваемой папки
	if (!(OpMode & OPM_SILENT))
	{
		PluginDialogBuilder val(psi, RECreateKeyTitle, _T("CreateSubkey"), &guid_CreateSubkey);

		val.AddText(RECreateKeyLabel);
		val.AddEditField(ms_CreatedSubkeyBuffer, countof(ms_CreatedSubkeyBuffer), 57, _T("RegistrySubkeyName"));

		val.AddOKCancel(REBtnOK, REBtnCancel);
		if (!val.ShowDialog())
			return -1;
	}
	int nLen = lstrlen(ms_CreatedSubkeyBuffer);
	// Отрезать хвостовые слеши
	while (nLen > 0 && ms_CreatedSubkeyBuffer[nLen-1] == _T('\\'))
		ms_CreatedSubkeyBuffer[--nLen] = 0;
	// Пустое имя ключа - отмена
	if (nLen < 1)
		return -1; // Отмена
	if (nLen > MAX_REGKEY_NAME)
	{
		SetLastError(ERROR_DS_NAME_TOO_LONG);
		return 0;
	}


	wchar_t* pszSubkey = NULL;
	#ifdef _UNICODE
	pszSubkey = ms_CreatedSubkeyBuffer;
	#else
	pszSubkey = (wchar_t*)malloc((nLen+1)*2);
	lstrcpy_t(pszSubkey, nLen+1, ms_CreatedSubkeyBuffer);
	#endif


	BOOL lbRc = 0;
	LONG hRc = 0;
	HREGKEY hCreatedKey = NULLHKEY;	
	MRegistryBase* pWorker = Worker();

	if (pszSubkey[0] == L'\\') {
		// Если путь задали в абсолютном формате (\Software\Far2\Plugins\RegEditor)
		hRc = pWorker->CreateKeyEx(m_Key.mh_Root, pszSubkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hCreatedKey, NULL, &m_Key.nKeyFlags);
	} else {
		HREGKEY hKey = NULLHKEY;
		// Путь относительно текущего ключа (ссылки типа ".." не преобразуются, все отдается в API как есть)
		hRc = mp_Items->OpenKey(pWorker, &hKey, KEY_CREATE_SUB_KEY);
		if (hRc == 0)
		{
			//Создаем новый ключ
			hRc = pWorker->CreateKeyEx(hKey, pszSubkey, 0, NULL, 0, KEY_WRITE, NULL, &hCreatedKey, NULL, NULL);
			//Закрываем родительский ключ
			mp_Items->CloseKey(pWorker, &hKey);
		}
	}
	// Сразу закрываем, сам ключ нам не нужен
	if (hCreatedKey) mp_Items->CloseKey(pWorker, &hCreatedKey);
	// Результат
	if (hRc == 0)
	{
		lbRc = 1;
		// вернуть ссылку на созданное имя, оно должно вернуться в ФАР
		*pszCreated = (ms_CreatedSubkeyBuffer[0] == _T('\\')) ? (ms_CreatedSubkeyBuffer+1) : ms_CreatedSubkeyBuffer;
		TCHAR *pszSlash = _tcsrchr(ms_CreatedSubkeyBuffer, _T('\\'));
		// Хоть реестр и поддерживает создание дерева субключей, но фару нужно вернуть корневой,
		// чтобы он смог правильно установить курсор
		if (pszSlash) *pszSlash = 0;
		// Чтобы ключ был перечитан
		SetForceReload();
	} else {
		SetLastError(hRc);
	}

	// Освободить буфер
	if (pszSubkey && (void*)pszSubkey != (void*)ms_CreatedSubkeyBuffer)
		SafeFree(pszSubkey);
	return lbRc;
}

void REPlugin::MemoryAllocFailed(size_t nSize, int nMsgID /*= REM_CantAllocateMemory*/)
{
	LPCTSTR pszFormat = GetMsg(nMsgID);
	if (!pszFormat) {
		InvalidOp();
		return;
	}

	TCHAR szMessage[MAX_PATH*3];
	_ASSERTE(lstrlen(pszFormat) < MAX_PATH*2);
	wsprintf(szMessage, pszFormat, (nSize > 0xFFFFFFFF) ? (DWORD)0xFFFFFFFF : (DWORD)nSize);

	REPlugin::Message(szMessage, FMSG_WARNING|FMSG_MB_OK, 0, NULL);
}

int REPlugin::Message(int nMsgID, DWORD nFlags/*= FMSG_WARNING|FMSG_MB_OK*/, int nBtnCount/*= 0*/, LPCTSTR asHelpTopic/* = NULL*/)
{
	int iRc = Message(GetMsg(nMsgID), nFlags, nBtnCount, asHelpTopic);
	return iRc;
}

int REPlugin::Message(LPCTSTR asMsg, DWORD nFlags/*= FMSG_WARNING|FMSG_MB_OK*/, int nBtnCount/*= 0*/, LPCTSTR asHelpTopic/* = NULL*/)
{
	if (!asMsg) {
		InvalidOp();
		return -1;
	}
	int iRc = psi.Message(_PluginNumber(guid_Msg12), nFlags|FMSG_ALLINONE, asHelpTopic, 
		(const TCHAR * const *)asMsg, 1, nBtnCount);
	return iRc;
}

int REPlugin::MessageFmt(int nFormatMsgID, LPCWSTR asArgument, DWORD nErrCode /*= 0*/, LPCTSTR asHelpTopic /*= NULL*/, DWORD nFlags /*= FMSG_WARNING|FMSG_MB_OK*/, int nBtnCount /*= 0*/)
{
	LPCTSTR pszFormat = GetMsg(nFormatMsgID);
	if (!pszFormat)
	{
		InvalidOp();
		return -1;
	}
	int nArgLen = lstrlenW(asArgument ? asArgument : L"");
	TCHAR* pszFile = NULL;
	if (asArgument)
	{
		if (IsUNCPath(asArgument))
			pszFile = UnmakeUNCPath_t(asArgument);
		else
			pszFile = lstrdup_t(asArgument);
	}

	TCHAR* pszMessage = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(pszFormat)+nArgLen+16));
	
	wsprintf(pszMessage, pszFormat, pszFile ? pszFile : _T(""));

	// Предыдущие функции могли нарушить LastError, поэтому выставим его
	if (nErrCode)
	{
		SetLastError(nErrCode);
		nFlags |= FMSG_ERRORTYPE;
	}

	int iRc = REPlugin::Message(pszMessage, nFlags, nBtnCount, asHelpTopic);
	SafeFree(pszMessage);
	return iRc;
}

#ifndef _UNICODE
int REPlugin::MessageFmt(int nFormatMsgID, LPCSTR asArgument, DWORD nErrCode /*= 0*/, LPCTSTR asHelpTopic /*= NULL*/, DWORD nFlags /*= FMSG_WARNING|FMSG_MB_OK*/, int nBtnCount /*= 0*/)
{
	LPCTSTR pszFormat = GetMsg(nFormatMsgID);
	if (!pszFormat) {
		InvalidOp();
		return -1;
	}
	int nArgLen = asArgument ? lstrlen(asArgument) : 0;

	TCHAR* pszMessage = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(pszFormat)+nArgLen+16));

	wsprintf(pszMessage, pszFormat, asArgument ? asArgument : _T(""));

	// Предыдущие функции могли нарушить LastError, поэтому выставим его
	if (nErrCode) {
		SetLastError(nErrCode);
		nFlags |= FMSG_ERRORTYPE;
	}

	int iRc = REPlugin::Message(pszMessage, nFlags, nBtnCount, asHelpTopic);
	SafeFree(pszMessage);
	return iRc;
}
#endif

BOOL REPlugin::ConfirmOverwriteFile(LPCWSTR asTargetFile, BOOL* pbAppendExisting, BOOL* pbUnicode)
{
	BY_HANDLE_FILE_INFORMATION fnd = {0};
	BYTE FileData[128]; DWORD nFileDataSize = 0;

	// -- FindFirstFile обламывается в симлинках!
	//HANDLE hFind = FindFirstFileW(asTargetFile, &fnd);
	//if (hFind == NULL || hFind == INVALID_HANDLE_VALUE)
	//	return TRUE;

	// Сразу пытаемся создать/открыть
	HANDLE hFile = CreateFileW(asTargetFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
	{
		DWORD nLastErr = GetLastError();
		if (nLastErr == ERROR_FILE_NOT_FOUND)
			return TRUE; // можно продолжить
		REPlugin::MessageFmt(REM_CantCreateTempFile, asTargetFile, nLastErr);
		return FALSE;
	}
	GetFileInformationByHandle(hFile, &fnd);
	if (fnd.nFileSizeLow || fnd.nFileSizeHigh)
	{
		// Если файл уже был создан - проверить что в нем записано, потребуется для Append
		ReadFile(hFile, FileData, 128, &nFileDataSize, NULL);
	}
	CloseHandle(hFile); hFile = NULL;

	// Проверить, юникодный ли файл? дописывать нужно в том же формате!
	BOOL lbExistUnicode = TRUE;
	if (nFileDataSize > 7)
	{
		if (FileData[0] == 0xFF && FileData[1] == 0xFE)
		{
			if ((memcmp(FileData+2, L"REGEDIT", 2*7) == 0) || (memcmp(FileData+2, L"Windows Registry", 2*16) == 0))
				lbExistUnicode = TRUE;
			else
				pbAppendExisting = NULL; // Дописывание невозможно
		}
		else if ((memcmp(FileData, "REGEDIT", 7) == 0) || (memcmp(FileData, "Windows Registry", 16) == 0))
		{
			lbExistUnicode = FALSE;
		}
		else if ((memcmp(FileData, L"REGEDIT", 2*7) == 0) || (memcmp(FileData, L"Windows Registry", 2*16) == 0))
		{
			lbExistUnicode = TRUE;
		}
		else
		{
			pbAppendExisting = NULL; // Дописывание невозможно
		}
	}
	else
	{
		pbAppendExisting = NULL; // Дописывание невозможно
	}

	TCHAR* pszShowPath = UnmakeUNCPath_t(asTargetFile);
	int nShowLen = lstrlen(pszShowPath);
	if (nShowLen > 62)
	{
		pszShowPath[62] = 0;
		pszShowPath[61] = _T('.');
		pszShowPath[60] = _T('.');
		pszShowPath[59] = _T('.');
	}
	TCHAR  szFileInfo[128]; SYSTEMTIME st; FILETIME ftl;

	FileTimeToLocalFileTime(&fnd.ftLastWriteTime, &ftl);
	FileTimeToSystemTime(&ftl, &st);
	wsprintf(szFileInfo, GetMsg(REM_FileExistInfo),
		fnd.nFileSizeLow, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	PluginDialogBuilder Cfrm(psi, REM_Warning, _T("SaveRegFile"), pbAppendExisting ? &guid_OverwriteRegFile : &guid_AppendRegFile);
	//FarDialogItem* p = NULL;

	//Cfrm.CenterText(
	Cfrm.AddText(REM_FileAlreadyExist)->Flags |= DIF_CENTERTEXT;
	//TODO: А если текст не помещается на экране?
	Cfrm.AddText(pszShowPath);
	Cfrm.AddSeparator();
	Cfrm.AddText(szFileInfo);

	int nBtnNames[3] = {REBtnOverwrite, REBtnCancel};
	int nBtnCount = 2;
	if (pbAppendExisting)
	{
		nBtnNames[1] = REBtnAppend; nBtnNames[2] = REBtnCancel; nBtnCount = 3;
	}
	//Cfrm.AddOKCancel(REBtnOverwrite, REBtnCancel);
	Cfrm.AddFooterButtons(nBtnNames, nBtnCount);

	Cfrm.DialogFlags = FDLG_WARNING;
	int nBtnNo = -1;
	BOOL bRc = Cfrm.ShowDialog(&nBtnNo); // Возвращает не ИД, а номер кнопки.

	if (pbAppendExisting)
	{
		*pbAppendExisting = (nBtnNo == 1);
		if (pbUnicode && (nBtnNo == 1))
		{
			// Дописывать нужно в том же формате, что исходный файл
			*pbUnicode = lbExistUnicode;
		}
	}

	SafeFree(pszShowPath);

	return bRc;
}

BOOL REPlugin::RegTypeMsgId2RegType(DWORD nMsgId, REGTYPE* pnRegType)
{
	if (nMsgId < RE_REG_NONE || nMsgId > RE_REG_QWORD) {
		if (pnRegType) *pnRegType = REG_BINARY;
		return FALSE;
	}
	if (pnRegType) *pnRegType = nMsgId - RE_REG_NONE;
	return TRUE;
}

void REPlugin::EditDescription()
{
	MCHKHEAP;
	if ((m_Key.mh_Root == NULL) && (m_Key.eType != RE_REGFILE))
		return; // В этих случаях - низя
	if (!mp_Items || !mp_Items->key.IsEqual(&m_Key))
	{
		_ASSERTE(mp_Items && mp_Items->key.IsEqual(&m_Key));
		return;
	}

	RegItem* pItem = GetCurrentItem();
	if (!pItem)
		return; // Требуется текущий элемент


	MCHKHEAP;
	PluginDialogBuilder val(psi, REChangeDescTitle, _T("EditValue"), &guid_EditValue);
	FarDialogItem* p = NULL;
	LONG hRc;
	DWORD nDataSize;
	REGTYPE nDataType;
	MRegistryBase* pWorker = Worker();
	TCHAR sNewName[16384];
	TCHAR sNewValue[16384];
	LPBYTE pValue = NULL; DWORD nMaxDataSize = 0;
	HREGKEY hKey = NULLHKEY, hParent = NULLHKEY;
	LPCWSTR pszValueName = (pItem->nValueType == REG__KEY) ? L"" : (pItem->bDefaultValue ? L"" : pItem->pszName);
	//BOOL lbChanged = FALSE;

	hRc = mp_Items->OpenKey(pWorker, &hParent);
	if (hRc != 0) {
		CantOpenKey(&mp_Items->key, FALSE);
		return;
	}
	if (pItem->nValueType == REG__KEY)
	{
		hRc = pWorker->OpenKeyEx(hParent, pItem->pszName, 0, KEY_READ, &hKey, NULL);
		if (hRc != 0) {
			CantOpenKey(&mp_Items->key, pItem->pszName, FALSE);
			return;
		}
	} else {
		hKey = hParent;
	}
	hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL,
		&nDataType, NULL, &nDataSize);
	if (hRc == 0 || hRc == ERROR_MORE_DATA)
	{
		nMaxDataSize = max(16384,(nDataSize+2));
		pValue = (LPBYTE)calloc(nMaxDataSize,1); // +wchar_t (\0) с учетом "кривых" строк в реестре
		hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL,
			&nDataType, pValue, &nDataSize);
		_ASSERTE(pszValueName!=NULL);
	}
	// Если для ключа запросили значение по умолчанию - строго строка
	else if (hRc == ERROR_FILE_NOT_FOUND && !*pszValueName) {
		pValue = (LPBYTE)malloc(2);
		nDataType = REG_SZ; *((wchar_t*)pValue) = 0; nDataSize = 2; hRc = 0;
	}
	// Ключ на чтение больше не требуется
	if (hKey && hKey != hParent) mp_Items->CloseKey(pWorker, &hKey); else hKey = NULL;
	mp_Items->CloseKey(pWorker, &hParent);
	if (hRc != 0)
	{
		ValueOperationFailed(&mp_Items->key, pItem->pszName, FALSE);
		SafeFree(pValue);
		return;
	}

	//TODO: Выполнить форматирование hex данных в sNewValue
	if (nDataType == REG_SZ || nDataType == REG_EXPAND_SZ)
	{
		if ((nDataSize>>1) >= countof(sNewValue)) {
			//TODO: Ругнуться на слишком длинное значение
			InvalidOp();
			return;
		}
		lstrcpy_t(sNewValue, countof(sNewValue), (wchar_t*)pValue);
	} else if (nDataType != REG_MULTI_SZ) {
		//TODO: Доделать многострочные
		SafeFree(pValue);
		return;
	} else {
		//TODO: Доделать НЕ строковые значения
		SafeFree(pValue);
		return;
	}
	SafeFree(pValue);


	// Добавить в диалог имя значения/ключа
	int nLen = lstrlenW(pItem->pszName);
	sNewName[0] = _T('\'');
	if (nLen>=40) {
		#ifdef _UNICODE
		lstrcat(sNewName, _T("\x2026")); /* "…" */
		#else
		lstrcat(sNewName, _T(">"));
		#endif
	}
	lstrcpy_t(sNewName+1, 40, pItem->pszName);
	lstrcat(sNewName, _T("'"));
	p = val.AddText(sNewName); 
	val.AddTextBefore(p, REChangeDescLabel);

	p = val.AddEditField(sNewValue, countof(sNewValue), 56+9);
	val.InitFocus(p);

	//TODO: Добавить кнопку "&Edit" для создания значения через редактор *.reg
	val.AddOKCancel(REBtnOK, REBtnCancel);
	if (!val.ShowDialog())
		return;


	// Apply
	hRc = mp_Items->OpenKey(pWorker, &hParent, KEY_WRITE);
	if (hRc != 0) {
		CantOpenKey(&mp_Items->key, FALSE);
		return;
	}
	if (pItem->nValueType == REG__KEY)
	{
		hRc = pWorker->OpenKeyEx(hParent, pItem->pszName, 0, KEY_WRITE, &hKey, NULL);
		if (hRc != 0) {
			CantOpenKey(&mp_Items->key, pItem->pszName, FALSE);
			return;
		}
	} else {
		hKey = hParent;
	}
	nLen = lstrlen(sNewValue);
	nDataSize = (nLen+1)*2;
	#ifndef _UNICODE
	pValue = (LPBYTE)calloc(nDataSize,1);
	lstrcpy_t((wchar_t*)pValue, nLen+1, sNewValue);
	#else
	_ASSERTE(pValue == NULL);
	#endif
	hRc = pWorker->SetValueEx(hKey, pszValueName, 0, nDataType, (LPBYTE)(pValue ? pValue : (LPBYTE)sNewValue), nDataSize);
	SafeFree(pValue);
	// Ключ на чтение больше не требуется
	if (hKey && hKey != hParent) mp_Items->CloseKey(pWorker, &hKey); else hKey = NULL;
	mp_Items->CloseKey(pWorker, &hParent);
	if (hRc != 0)
	{
		ValueOperationFailed(&mp_Items->key, pItem->pszName, FALSE);
		SafeFree(pValue);
		return;
	}


	// UpdatePanel
	SetForceReload();
	UpdatePanel(false);
	RedrawPanel(NULL);
	MCHKHEAP;
}

void REPlugin::JumpToRegedit()
{
	if (m_Key.eType != RE_WINAPI || mb_RemoteMode) {
		return; // для Remote registry и для *.reg/hive - не поддерживается?
	}

	const RegItem* pItem = GetCurrentItem();
	if (!pItem && !m_Key.mh_Root)
		return;
	
	DWORD nRegeditPID = 0;
	HANDLE hProcess = NULL;
	HWND hParent = NULL, hTree = NULL, hList = NULL;
	BOOL bHooked = FALSE;
		
	if (!gpPluginList->ConnectRegedit(nRegeditPID, hProcess, hParent, hTree, hList, bHooked))
		return; // Ошибка уже показана

	//// Открыть в RegEdit.exe текущий ключ
	//DWORD nRegeditPID = 0;
	//HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	//if (hSnap == INVALID_HANDLE_VALUE) {
	//	Message(REM_CreateToolhelp32SnapshotFailed, FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK);
	//	return;
	//}
	//PROCESSENTRY32 ent = {sizeof(PROCESSENTRY32)};
	//if (Process32First(hSnap, &ent))
	//{
	//	do {
	//		if (lstrcmpi(ent.szExeFile, _T("regedit.exe")) == 0) {
	//			nRegeditPID = ent.th32ProcessID;
	//			break;
	//		}
	//	} while (Process32Next(hSnap, &ent));
	//}
	//CloseHandle(hSnap);

	
	int nLen = 0;
	wchar_t* pszData = NULL;


	// Если RegEdit.exe еще не запущен - нужно запустить
	//if (nRegeditPID == 0)
	//{
	//	BOOL lbSucceeded = FALSE;
	//	//MRegistryWinApi reg;
	//	//HREGKEY hk = NULL;
	//	// Для ускорения запуска - попытаемся сбросить стартовый ключ?
	//	RegPath key = {RE_WINAPI}; key.Init(RE_WINAPI, HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit");
	//	if (0 != reg.CreateKeyEx(HKEY_CURRENT_USER, key.mpsz_Key, 0, 0, 0, KEY_WRITE, 0, &hk, 0))
	//	{			
	//		CantOpenKey(&key, TRUE);
	//	} else {
	//		nLen = m_Key.mpsz_Key ? lstrlenW(m_Key.mpsz_Key) : 1;
	//		if (pItem && pItem->pszName)
	//			nLen += 1 + lstrlenW(pItem->pszName);
	//		pszData = (wchar_t*)calloc(nLen+128,2);
	//		lstrcpyW(pszData, L"My Computer\\");
	//		HKeyToStringKey(m_Key.mh_Root, pszData+lstrlenW(pszData), 40);
	//		if (m_Key.mpsz_Key && *m_Key.mpsz_Key)
	//		{
	//			lstrcatW(pszData, L"\\");
	//			lstrcatW(pszData, m_Key.mpsz_Key);
	//		}
	//		if (pItem && pItem->nValueType == REG__KEY)
	//		{
	//			lstrcatW(pszData, L"\\");
	//			lstrcatW(pszData, pItem->pszName);
	//		}
	//		nLen = lstrlenW(pszData)+1;
	//		if (0 != reg.SetValueEx(hk, L"LastKey", 0, REG_SZ, (LPBYTE)pszData, nLen*2))
	//		{
	//			ValueOperationFailed(&key, L"LastKey", TRUE, FALSE);
	//		} else {
	//			lbSucceeded = TRUE;
	//		}
	//		reg.CloseKey(hk);
	//	}
	//	if (lbSucceeded)
	//	{
	//		DWORD nRc = (DWORD)ShellExecute(NULL, _T("open"), _T("regedit.exe"), NULL, NULL, SW_SHOWNORMAL);
	//		if (nRc < 32)
	//		{
	//			Message(REM_RegeditExeFailed, FMSG_WARNING|FMSG_MB_OK|FMSG_ERRORTYPE);
	//		}
	//	}
	//	return;
	//}

	//HANDLE hProcess = NULL; //OpenProcess(SYNCHRONIZE, FALSE, nRegeditPID);
	//	
	//// Если RegEdit.exe еще не запущен - нужно запустить
	//if (nRegeditPID == 0)
	//{
	//	DWORD nRc = (DWORD)ShellExecute(NULL, _T("open"), _T("regedit.exe"), NULL, NULL, SW_SHOWNORMAL);
	//	if (nRc < 32)
	//	{
	//		Message(REM_RegeditExeFailed, FMSG_WARNING|FMSG_MB_OK|FMSG_ERRORTYPE);
	//	}
	//}

	//#ifdef EXECUTE_TRAP
	//nLen = lstrcmp(pszData, m_Key.mpsz_Key);
	//pszData[8] = 0;
	//#endif
		
	// В уже запущенном RegEdit.exe выделить требуемый ключ
	nLen = m_Key.mpsz_Key ? lstrlenW(m_Key.mpsz_Key) : 1;
	if (pItem && pItem->pszName)
		nLen += 1 + lstrlenW(pItem->pszName);
	pszData = (wchar_t*)calloc(nLen+128,2);
	HKeyToStringKey(m_Key.mh_Root, pszData, 40);
	if (m_Key.mpsz_Key && *m_Key.mpsz_Key)
	{
		lstrcatW(pszData, L"\\");
		lstrcatW(pszData, m_Key.mpsz_Key);
	}
	if (pItem && pItem->nValueType == REG__KEY && pItem->pszName && *pItem->pszName)
	{
		lstrcatW(pszData, L"\\");
		lstrcatW(pszData, pItem->pszName);
	}
	
	LPCWSTR pszValueName = NULL;
	if (pItem && pItem->nValueType < REG__INTERNAL_TYPES && pItem->pszName)
		pszValueName = pItem->bDefaultValue ? L"" : pItem->pszName;

	int  nRc = 0;
	BOOL bProcessed = FALSE;
	DWORD dwErrCode = 0;
	
	if (bHooked)
	{
		int nValLen = 0, nKeyLen = lstrlenW(pszData)+1;
		if (pszValueName)
		{
			nValLen = lstrlenW(pszValueName)+1;
			if (nValLen >= 16384)
			{
				nValLen = 0;
				pszValueName = NULL;
			}
		}
		DWORD cbSize = (nKeyLen+nValLen+2)*2;
		wchar_t* pszWriteBuf = (wchar_t*)calloc(cbSize,1);
		lstrcpyW(pszWriteBuf, pszData);
		if (pszValueName)
		{
			pszWriteBuf[nKeyLen] = (wchar_t)1;
			lstrcpyW(pszWriteBuf+nKeyLen+1, pszValueName);
		}
		
		// Write pipe
		DWORD nPipeRc[3] = {0}, dwBytesRead = 0;
		wchar_t sPipeName[MAX_PATH];
		wsprintfW(sPipeName, L"\\\\.\\pipe\\FarRegEditor.%u", nRegeditPID);

		// Поднять окно RegEdit наверх
		SetForegroundWindow(hParent);
		
		bProcessed = CallNamedPipeW(sPipeName, pszWriteBuf, cbSize, nPipeRc, sizeof(nPipeRc), &dwBytesRead, 10000);
		if (!bProcessed || dwBytesRead != sizeof(nPipeRc) || nPipeRc[1] != 0)
		{
			dwErrCode = GetLastError();
			MessageFmt(REM_RegeditFailed_Pipe, sPipeName, dwErrCode, _T("JumpRegedit"));
		} else {
			nRc = 0;
			bProcessed = TRUE;
		}
		
		// Final
		SafeFree(pszWriteBuf);
	}


	// Если через пайп не удалось?
	if (!bProcessed)
	{
		#ifndef _WIN64
		if (cfg->is64bitOs)
			nRc = REM_RegeditFailed_NeedHandle;
		else
		#endif
		nRc = SelectTreeItem(hProcess, hParent, hTree, hList, pszData, pszValueName);
		
		// Если ошибка - показать
		if (nRc > 0)
		{
			DWORD nFlags = FMSG_WARNING|FMSG_MB_OK;
			if (nRc != REM_RegeditFailed_NeedHandle
				&& nRc != REM_RegeditFailed_TVM_GETITEMW
				&& nRc != REM_RegeditFailed_KeyNotFound
				&& nRc != REM_RegeditFailed_ValueNotFound)
				nFlags |= FMSG_ERRORTYPE;
			Message(nRc, nFlags, 0, _T("JumpRegedit"));
		} else if (nRc < 0) {
			InvalidOp();
		}
	}
	
	SafeFree(pszData);

	//// Найти окно RegEdit'а
	//DWORD dwNeedStyle = WS_VISIBLE|WS_THICKFRAME|WS_CAPTION|WS_MAXIMIZEBOX;
	//DWORD dwStyle, dwPID;
	//HWND hParent = NULL, hTree = NULL;
	//while ((hParent = FindWindowEx(NULL, hParent, 0,0)) != 0)
	//{
	//	if (GetWindowThreadProcessId(hParent, &dwPID) && dwPID == nRegeditPID)
	//	{
	//		dwStyle = (DWORD)GetWindowLongPtr(hParent, GWL_STYLE);
	//		if ((dwStyle & dwNeedStyle) == dwNeedStyle)
	//		{
	//			hTree = FindWindowEx(hParent, NULL, _T("SysTreeView32"), NULL);
	//			if (hTree)
	//				break;
	//		}
	//	}
	//}
	//if (!hTree)
	//{
	//	Message(REM_RegeditWindowFailed);
	//} else {
	
	//SetForegroundWindow(hTree);
	//// К сожалению, установить фокус на нужный контрол - не получится
	//SetFocus(hTree);
	////SendMessageW(hTree, WM_SETFOCUS, 0, 0);
	//if (hProcess) WaitForInputIdle(hProcess, 100); Sleep(250);
	//
	//HTREEITEM htvi = NULL, htviParent = NULL, hRoot = NULL;;
	//if (0 == SendMessageTimeout(hTree, TVM_GETNEXTITEM, TVGN_CARET, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&htvi))
	//{
	//	Message(REM_RegeditWindowHung, FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK);
	//	goto wrap;
	//} else if (htvi == NULL) {
	//	Message(REM_RegeditCommunicationFailed, FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK);
	//	goto wrap;
	//} else {
	//	// Поехали
	//	hRoot = (HTREEITEM)SendMessage(hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0);
	//	SendMessage(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
	//	htviParent = (HTREEITEM)SendMessage(hTree, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)htvi);
	//	while (htviParent && htviParent != hRoot)
	//	{
	//		SendMessage(hTree, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)htvi);
	//		htvi = htviParent;
	//		htviParent = (HTREEITEM)SendMessage(hTree, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)htvi);
	//	}
	//}
	//
	//// Поехали выделять
	//nLen = lstrlenW(pszData);
	//CharUpperBuffW(pszData, nLen);
	//for (wchar_t* psz = pszData; *psz; psz++)
	//{
	//	if (*psz != L'\\')
	//	{
	//		SendMessageW(hTree, WM_CHAR, *psz, 0);
	//		if (hProcess) WaitForInputIdle(hProcess, 100);
	//	} else {
	//		if (hProcess) WaitForInputIdle(hProcess, 1000); Sleep(100);
	//		SendMessageW(hTree, WM_KEYDOWN, VK_RIGHT, 0);
	//		Sleep(100);
	//	}
	//}
	//
	////hList = FindWindowEx(hParent, NULL, _T("SysListView32"), NULL);
	//if (pItem->nValueType != REG__KEY && hList)
	//{
	//	if (hProcess) WaitForInputIdle(hProcess, 100); Sleep(250);
	//	//SendMessageW(hList, WM_SETFOCUS, 0, 0);
	//	SetForegroundWindow(hList);
	//	SetFocus(hList);
	//	if (hProcess) WaitForInputIdle(hProcess, 100); Sleep(250);
	//	
	//	//SendMessageW(hTree, WM_KEYDOWN, VK_RIGHT, 0);
	//	//Sleep(250);
	//
	//	SafeFree(pszData);
	//	pszData = lstrdup(pItem->pszName);
	//	CharUpperBuffW(pszData, lstrlenW(pszData));
	//	
	//	SendMessageW(hList, WM_KEYDOWN, VK_HOME, 0);
	//	for (wchar_t* psz = pszData; *psz; psz++)
	//	{
	//		if (hProcess) WaitForInputIdle(hProcess, 100);
	//		SendMessageW(hList, WM_CHAR, *psz, 0);				
	//	}
	//}
	//
	//SetForegroundWindow(hParent);
	////// К сожалению, установить фокус на нужный контрол - не получится
	////SetFocus(hParent);
	//
	////if (hProcess) CloseHandle(hProcess); -- хэндл хранится в gpPluginList!
	//wrap:
	//	SafeFree(pszData);
}

BOOL REPlugin::ConnectRemote(LPCWSTR asServer /*= NULL*/, LPCWSTR asLogin /*= NULL*/, LPCWSTR asPassword /*= NULL*/)
{
	//BOOL lbConnected = TRUE;
	MRegistryBase *pNewWorker = NULL;
	TCHAR sServerName[128], sLogin[MAX_PATH], sPassword[MAX_PATH], sNetShare[128];
	PluginDialogBuilder srv(psi, REChangeDescTitle, _T("RemoteConnect"), &guid_RemoteConnect);
	int nBtns[] = {REBtnConnect,REBtnLocal,REBtnCancel};
	int nRc = 0;

	CloseItems();
	
	//TODO: Обработка логина и пароля (подключение под пользователем)
	//TODO: при обломе - lbConnected = FALSE, вернуть локальный реестр
	
	if (asServer && *asServer)
	{
		while (*asServer == L'\\' || *asServer == L'/') asServer++;
	}
	
	if (asServer && *asServer)
	{
		mb_RemoteMode = TRUE;
		ms_RemoteServer[0] = L'\\'; ms_RemoteServer[1] = L'\\';
		lstrcpynW(ms_RemoteServer+2, asServer, countof(ms_RemoteServer)-2);
		lstrcpynW(ms_RemoteLogin, asLogin, countof(ms_RemoteLogin));
		//lstrcpynW(ms_RemotePassword, asPassword, countof(ms_RemotePassword));
		pNewWorker = new MRegistryWinApi(mb_Wow64on32, mb_Virtualize);
		if (pNewWorker->ConnectRemote(asServer, asLogin, asPassword, NULL))
		{
			goto connected;
		}
	}

	lstrcpy_t(sServerName, countof(sServerName), ms_RemoteServer);
	lstrcpy_t(sLogin, countof(sLogin), ms_RemoteLogin);
	sPassword[0] = 0; lstrcpy(sNetShare, _T("IPC$"));
	

	srv.AddText(REConnectServerNameLabel);
	srv.AddEditField(sServerName, MAX_PATH, 60, _T("RegistryServer"));
	
	srv.AddText(REConnectUserNameLabel);
	srv.AddEditField(sLogin, MAX_PATH, 60, _T("RegistryLogin"));
	
	srv.AddText(REConnectPasswordLabel);
	srv.AddEditField(sPassword, MAX_PATH, 60, NULL)->Type = DI_PSWEDIT;
	
	srv.AddText(REConnectMedia);
	srv.AddEditField(sNetShare, MAX_PATH, 60, NULL);
	
	srv.AddFooterButtons(nBtns, countof(nBtns));

	while (true)
	{
		if (!srv.ShowDialog(&nRc))
			return FALSE;
		if (nRc != 0)
			break;
			
		// Подключение - перенести в MRegistryWinApi
			
		// Сразу попробовать подключиться
		//if (sServerName[0] && sLogin[0])
		{
			_ASSERTE(pNewWorker == NULL);
			pNewWorker = new MRegistryWinApi(mb_Wow64on32, mb_Virtualize);
			//wchar_t wsServerName[MAX_PATH];
			//lstrcpy_t(wsServerName, countof(wsServerName), sServerName);
			if (!pNewWorker->ConnectRemote(sServerName, sLogin, sPassword, sNetShare))
			{
				SafeDelete(pNewWorker);
				continue; // на повтор ввода
			}
			//NETRESOURCE res = {0};
			//TCHAR szRemoteName[MAX_PATH+1];
			//TCHAR* psz = sServerName; while (*psz == _T('\\') || *psz == _T('/')) psz++;
			//szRemoteName[0] = _T('\\'); szRemoteName[1] = _T('\\');
			//lstrcpyn(szRemoteName+2, psz, MAX_PATH-2);
			//lstrcat(szRemoteName, _T("\\"));
			//int nLen = lstrlen(szRemoteName);
			//lstrcpyn(szRemoteName+nLen, sNetShare, MAX_PATH-nLen);
			//res.dwType = RESOURCETYPE_ANY;
			//res.lpRemoteName = szRemoteName;
			//// Подключаемся
			//CScreenRestore pop(GetMsg(REM_RemoteConnecting), GetMsg(REPluginName));
			//DWORD dwErr = WNetAddConnection2(&res, sPassword, sLogin, CONNECT_TEMPORARY);
			//pop.Restore();
			//if (dwErr != 0)
			//{
			//	REPlugin::MessageFmt(REM_LogonFailed, sLogin, dwErr, _T("RemoteConnect"));
			//	continue;
			//}
			//// По завершении работы нужно отключиться
			//// WNetCancelConnection2("\\\\win7x64\\\IPC$", 0, 0);
			//// или
			//// net use \\win7x64\IPC$ /DELETE
		}
		break;
	}

	TCHAR* psz = sServerName; while (*psz == _T('\\') || *psz == _T('/')) psz++;
	if (nRc == 0)
	{
		_ASSERTE(pNewWorker!=NULL);
		mb_RemoteMode = TRUE;
		lstrcpynW(ms_RemoteServer, pNewWorker->sRemoteServer, countof(ms_RemoteServer));
		//if (*psz == 0) {
		//	ms_RemoteServer[0] = 0;
		//} else {
		//	ms_RemoteServer[0] = L'\\'; ms_RemoteServer[1] = L'\\';
		//	lstrcpy_t(ms_RemoteServer+2, countof(ms_RemoteServer)-2, psz);
		//}
		lstrcpy_t(ms_RemoteLogin, countof(ms_RemoteLogin), sLogin);
		//lstrcpy_t(ms_RemotePassword, countof(ms_RemotePassword), sPassword);
	}
	else if (nRc == 1)
	{
		mb_RemoteMode = FALSE;
		m_Key.SetTitlePrefix((LPCTSTR)NULL, TRUE);
		m_Key.SetServer((LPCTSTR)NULL);
		if (mp_Worker)
			mp_Worker->ConnectLocal();
	}
	else
	{
		return FALSE; // Не менять
	}

connected:	
	if (pNewWorker)
	{
		_ASSERTE(mp_Items == NULL);
		SafeDelete(mp_Worker);
		mp_Worker = pNewWorker;
	}
	
	m_Key.SetTitlePrefix((LPCTSTR)NULL, TRUE);
	m_Key.SetServer(ms_RemoteServer);

	ChDir(L"\\", TRUE);

	SetForceReload();
	UpdatePanel(true);
	RedrawPanel(NULL);
	
	return TRUE;
}

void REPlugin::ShowBookmarks()
{
	// Загрузить из реестра для текущего режима
	TCHAR* pszBookmarks = NULL, szNil[2] = {0};
	DWORD  nCount = 0;
	int    nSelected = -1;
	bool   bFirstCall = true;
	#if FAR_UNICODE>=1906
	//TODO: RIGHT_CTRL_PRESSED
	FarKey BreakKeys[] = {{VK_INSERT},{VK_DELETE},{VK_F4},{'R',LEFT_CTRL_PRESSED},{VK_TAB},{0}};
	#else
	int    BreakKeys[] = {VK_INSERT,VK_DELETE,VK_F4,(PKF_CONTROL<<16)|'R',VK_TAB,0};
	#endif
	MENUBREAKCODETYPE nBreakCode = -1;
	FarMenuItem* pMenuItems = NULL;

	if (m_Key.eType != RE_WINAPI)
	{
		cfg->bUseInternalBookmarks = TRUE;
		#if FAR_UNICODE>=1906
		//TODO: RIGHT_CTRL_PRESSED
		BreakKeys[3].VirtualKeyCode = 0; BreakKeys[3].ControlKeyState = 0;
		#else
		BreakKeys[3] = 0; // не показывать Favorites
		#endif
	}
	
	while (true)
	{
		cfg->BookmarksGet(&m_Key, mb_RemoteMode, &pszBookmarks, &nCount);
		
		// Создать элементы меню
		pMenuItems = (FarMenuItem*)calloc(nCount+1,sizeof(FarMenuItem));
		TCHAR* psz = pszBookmarks;
		for (UINT i = 0; i < nCount; i++)
		{
			if (*psz == 0 || (*psz == _T('-') && *(psz+1) == 0))
			{
				MenuItemSetSeparator(pMenuItems[i]);
			}
			else
			{
				#ifdef _UNICODE
				pMenuItems[i].Text = psz;
				#else
				lstrcpyn(pMenuItems[i].Text, psz, countof(pMenuItems[i].Text));
				#endif
			}
			psz += lstrlen(psz)+1;		
		}
		#ifdef _UNICODE
		pMenuItems[nCount].Text = szNil;
		#endif
		//FarMenuItem links[] =
		//{
		//#ifdef _UNICODE
		//	{_T("HKEY_CURRENT_USER\\Software\\Far2")},
		//	{_T("HKEY_LOCAL_MACHINE\\Software\\Far")},
		//#else
		//	{_T("HKEY_CURRENT_USER\\Software\\Far")},
		//	{_T("HKEY_LOCAL_MACHINE\\Software\\Far")},
		//#endif
		//	{_T("HKEY_CLASSES_ROOT")},
		//	{_T("HKEY_CLASSES_ROOT\\CLSID")},
		//	{_T("HKEY_CLASSES_ROOT\\TypeLib")},
		//	{_T("HKEY_CLASSES_ROOT\\Interface")},
		//};

		// Подсветить текущий, или наиболее соответствующий пункт
		if (bFirstCall && (nSelected < 0 || (DWORD)nSelected >= nCount)
			&& (m_Key.mpsz_Dir && *m_Key.mpsz_Dir))
		{
			for (UINT i = 0; i < nCount; i++)
			{
				if (MenuItemIsSeparator(pMenuItems[i]) || !*pMenuItems[i].Text)
					continue;
				if (lstrcmpi(m_Key.mpsz_Dir, pMenuItems[i].Text) == 0)
				{
					MenuItemSetSelected(pMenuItems[i]);
					break;
				}
				else if (!_tmemcmp(pMenuItems[i].Text, _T("HKLM\\"), 5)
					&& !_tmemcmp(m_Key.mpsz_Dir, _T("HKEY_LOCAL_MACHINE\\"), 19))
				{
					if (!lstrcmpi(m_Key.mpsz_Dir+19, pMenuItems[i].Text+5))
					{
						MenuItemSetSelected(pMenuItems[i]);
						break;
					}
				}
				else if (!_tmemcmp(pMenuItems[i].Text, _T("HKCU\\"), 5)
					&& !_tmemcmp(m_Key.mpsz_Dir, _T("HKEY_CURRENT_USER\\"), 18))
				{
					if (!lstrcmpi(m_Key.mpsz_Dir+18, pMenuItems[i].Text+5))
					{
						MenuItemSetSelected(pMenuItems[i]);
						break;
					}
				}
				else if (!_tmemcmp(pMenuItems[i].Text, _T("HKCR\\"), 5)
					&& !_tmemcmp(m_Key.mpsz_Dir, _T("HKEY_CLASSES_ROOT\\"), 18))
				{
					if (!lstrcmpi(m_Key.mpsz_Dir+18, pMenuItems[i].Text+5))
					{
						MenuItemSetSelected(pMenuItems[i]);
						break;
					}
				}
				else if (!_tmemcmp(pMenuItems[i].Text, _T("HKU\\"), 4)
					&& !_tmemcmp(m_Key.mpsz_Dir, _T("HKEY_USERS\\"), 11))
				{
					if (!lstrcmpi(m_Key.mpsz_Dir+11, pMenuItems[i].Text+4))
					{
						MenuItemSetSelected(pMenuItems[i]);
						break;
					}
				}
			}
		}
		else if (nSelected >= 0 && (DWORD)nSelected <= nCount)
		{
			MenuItemSetSelected(pMenuItems[nSelected]);
		}
		bFirstCall = false;

		
		nSelected = psi.Menu(_PluginNumber(guid_BookmarksMenu), -1,-1,0,
			FMENU_AUTOHIGHLIGHT/*|FMENU_CHANGECONSOLETITLE*/|FMENU_WRAPMODE,
			GetMsg(cfg->bUseInternalBookmarks ? REBookmarksTitle : REBookmarksExtTitle),
			GetMsg(cfg->bUseInternalBookmarks ? REBookmarksFooter : REBookmarksExtFooter),
			_T("Bookmarks"), 
			BreakKeys, &nBreakCode, pMenuItems, nCount+1);
		if (nSelected < 0)
		{
			SafeFree(pszBookmarks);
			SafeFree(pMenuItems);
			return;
		}
		if (nBreakCode != -1)
		{
			switch (nBreakCode)
			{
				case 0:
					// Ins
					cfg->BookmarkInsert(nSelected, &m_Key);
					break;
				case 1:
					// Del
					cfg->BookmarkDelete(nSelected);
					if ((nSelected+1) == nCount && nSelected > 0)
					{
						nSelected--;
						while (nSelected > 0 && MenuItemIsSeparator(pMenuItems[nSelected]))
							nSelected--;
					} else {
						while (nSelected > 0 && (nSelected+1) < (int)nCount && MenuItemIsSeparator(pMenuItems[nSelected+1]))
							nSelected--;
					}
					break;
				case 2:
					// F4
					cfg->BookmarksEdit();
					break;
				case 3:
					// Ctrl-R
					if (m_Key.eType == RE_WINAPI && cfg->bUseInternalBookmarks)
					{
						if (Message(REM_ConfirmBookmarkReset, FMSG_WARNING|FMSG_MB_YESNO, 0, _T("Bookmarks")) == 0)
							cfg->BookmarksReset(&m_Key, mb_RemoteMode);
					}
					break;
				case 4:
					// Tab
					cfg->bUseInternalBookmarks = !cfg->bUseInternalBookmarks;
					break;
			}
			SafeFree(pszBookmarks);
			SafeFree(pMenuItems);
			continue;
		}
		if (nSelected >= (int)nCount)
		{
			SafeFree(pszBookmarks);
			SafeFree(pMenuItems);
			return;
		}


		// Переходим	
		wchar_t sNewKey[16384];
		if (pMenuItems[nSelected].Text[0] == L'\\')
		{
			lstrcpy_t(sNewKey, countof(sNewKey), pMenuItems[nSelected].Text);
		} else {
			sNewKey[0] = L'\\';
			lstrcpy_t(sNewKey+1, countof(sNewKey)-1, pMenuItems[nSelected].Text);
		}
		
		SafeFree(pszBookmarks);
		SafeFree(pMenuItems);

		if (SetDirectory(sNewKey, FALSE))
		{
			// UpdatePanel & позиционировать курсор на созданный элемент
			CloseItems();
			UpdatePanel(true);
			RedrawPanel((RegItem*)1);
		} else {
			// Ругнуться, что ключ asSubKey не существует в m_Key.mh_Root\m_Key.mpsz_Key
			TCHAR* pszKeyT = lstrdup_t(sNewKey);
			const TCHAR* sLines[] = 
			{
				GetMsg(REPluginName),
				GetMsg(REM_KeyNotExists),
				pszKeyT
			};
			psi.Message(_PluginNumber(guid_Msg13), FMSG_WARNING|FMSG_MB_OK, NULL, 
				sLines, countof(sLines), 0);
			SafeFree(pszKeyT);
			continue;
		}
		
		return;
	}
}

// Открыть ключ, считать значение, закрыть ключ.
// Если в *ppData недостаточно памяти - выделяется новый блок, старый НЕ освобождается.
// Чтобы память не текла - передавать в *ppData NULL или ссылку на static данные.
// При ошибке открытия ключа - в *pnDataType возвращается (DWORD)-1
LONG REPlugin::ValueDataGet(RegFolder* pFolder, RegItem *pItem, LPBYTE* ppData, LPDWORD pcbSize, REGTYPE* pnDataType, BOOL abSilence)
{
	_ASSERTE(pFolder && pItem && ppData && pcbSize);
	_ASSERTE(*ppData == NULL || *pcbSize != 0);
	
	MRegistryBase* pWorker = Worker();
	LPCWSTR pszValueName = pItem->bDefaultValue ? NULL : pItem->pszName;
	HREGKEY hKey = NULLHKEY;
	LONG hRc = pFolder->OpenKey(pWorker, &hKey);

	if (hRc != 0) {
		if (pnDataType) *pnDataType = (DWORD)-1;
		if (!abSilence) CantOpenKey(&pFolder->key, FALSE);
		
	} else {
		hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL,
			pnDataType, *ppData, pcbSize);
		if ((*ppData == NULL) || (hRc == ERROR_MORE_DATA))
		{
			*ppData = (LPBYTE)calloc((*pcbSize)+3,1); // +wchar_t (\0) с учетом "кривых" строк в реестре
			hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL,
				pnDataType, *ppData, pcbSize);
		}
		
		if (hRc != 0 && pnDataType)
			*pnDataType = 0;

		// Ключ на чтение больше не требуется
		pFolder->CloseKey(pWorker, &hKey); //_ASSERTE(hKey==NULL);
	}
	
	return hRc;
}

// Открыь ключ на запись, записать значение, закрыть ключ.
LONG REPlugin::ValueDataSet(RegFolder* pFolder, RegItem *pItem, LPBYTE pData, DWORD cbSize, REGTYPE nDataType)
{
	_ASSERTE(pFolder && pItem && pData && cbSize);
	
	LONG hRc = ValueDataSet(pFolder, pItem->bDefaultValue ? NULL : pItem->pszName, pData, cbSize, nDataType);

	return hRc;
}

// Открыь ключ на запись, записать значение, закрыть ключ.
LONG REPlugin::ValueDataSet(RegFolder* pFolder, LPCWSTR asValueName, LPBYTE pData, DWORD cbSize, REGTYPE nDataType)
{
	_ASSERTE(pFolder && pData && cbSize);

	MRegistryBase* pWorker = Worker();
	HREGKEY hKey = NULLHKEY;
	LONG hRc = pFolder->CreateKey(pWorker, &hKey, KEY_SET_VALUE);

	if (hRc != 0)
	{
		CantOpenKey(&pFolder->key, TRUE);
		
	}
	else
	{
		hRc = pWorker->SetValueEx(hKey, asValueName, 0, nDataType, pData, cbSize);
		if (hRc != 0)
		{
			ValueOperationFailed(&pFolder->key, asValueName, TRUE);
		}

		// Ключ на чтение больше не требуется
		pFolder->CloseKey(pWorker, &hKey); //_ASSERTE(hKey==NULL);
	}
	
	return hRc;
}

void REPlugin::SaveRegFile(BOOL abInClose /*= FALSE*/)
{
	if (m_Key.eType != RE_REGFILE || !mpsz_HostFile
		|| !mp_Worker || mp_Worker->eType != RE_REGFILE)
	{
		//InvalidOp();
		return;
	}

	BOOL lbExportRc = FALSE;
	TCHAR sLabel[70];
	wchar_t sDefaultName[MAX_PATH];
	MFileReg *pFileReg = ((MFileReg*)mp_Worker);
	lstrcpyn(sLabel, PointToName(mpsz_HostFile), countof(sLabel));
	int nExportMode = (pFileReg->IsInitiallyUnicode() ? REExportReg5 : REExportReg4) - REExportReg4;
	wchar_t* pwszDestPath = NULL;
	RegFolder expFolder; // не кешируем, т.к. грузить сейчас нужно всё подряд
	MFileTxtReg file(mb_Wow64on32, mb_Virtualize);
	RegPath rootKey = {m_Key.eType};
	const TCHAR* pszShowFilePathName = pFileReg->GetShowFilePathName();
	wchar_t* pwszShowFilePathName = NULL;
	#ifdef _UNICODE
	pwszShowFilePathName = (wchar_t*)pszShowFilePathName;
	#else
	pwszShowFilePathName = lstrdup_w(pszShowFilePathName);
	#endif
	BOOL lbConfirmOverwrite = FALSE;

	expFolder.Init(&rootKey);

	lstrcpynW(sDefaultName, PointToName(pFileReg->GetFilePathName()), countof(sDefaultName));

	lbExportRc = ConfirmExport(
		FALSE/*abAllowHives*/, &nExportMode, NULL, 1,
		pszShowFilePathName, TRUE/*abDestFileSpecified*/, sLabel,
		&pwszDestPath, sDefaultName, NULL/*wsDefaultExt*/,
		RESaveRegDlgTitle, RESaveRegItemLabel,
		abInClose ? REBtnDiscard : REBtnCancel);

	int nPathLen = lstrlenW(pwszDestPath);
	if (lstrlenW(pwszShowFilePathName) <= nPathLen
		|| wcsncmp(pwszDestPath, pwszShowFilePathName, nPathLen) != 0
		|| wcscmp(sDefaultName, pwszShowFilePathName+nPathLen) != 0)
	{
		// Смена имени - подвтвердить возможную перезапись
		lbConfirmOverwrite = TRUE;
	}

	if (!lbExportRc)
		goto wrap;

	// Создаем файл
	if (file.FileCreate(pwszDestPath, sDefaultName, L"", nExportMode == (REExportReg5 - REExportReg4), lbConfirmOverwrite))
	{
		// Сформировать заголовок
		lbExportRc = file.FileWriteHeader(mp_Worker);
		//if (nExportMode == (REExportReg5-REExportReg4)) {
		//	// (BOM уже записан в file.FileCreateTemp)
		//	lbExportRc = file.FileWrite(L"Windows Registry Editor Version 5.00\r\n");
		//} else {
		//	LPCSTR pszHeader = "REGEDIT4\r\n";
		//	int nSize = lstrlenA(pszHeader);
		//	lbExportRc = file.FileWriteBuffered(pszHeader, nSize);
		//}

		// Запускаем экспорт
		if (lbExportRc)
		{
			_ASSERTE(gpProgress == NULL);
			gpProgress = new REProgress(GetMsg(RESaveRegDlgTitle));
			// Теперь, собственно экспорт
			lbExportRc = expFolder.ExportToFile(this, Worker(), &file, (nExportMode == (REExportReg5-REExportReg4)));
			SafeDelete(gpProgress);
		}

		file.FileClose();
	}

wrap:
	SafeFree(pwszDestPath);
	if (pwszShowFilePathName && (void*)pwszShowFilePathName != (void*)pszShowFilePathName)
		SafeFree(pwszShowFilePathName);
	expFolder.Release();

	if (lbExportRc && mp_Worker && mp_Worker->bDirty)
	{
		mp_Worker->bDirty = false;
		RedrawPanel(NULL);
	}
}

BOOL REPlugin::LoadRegFile(LPCWSTR asRegFilePathName, BOOL abSilence, BOOL abDelayLoading, BOOL abAllowUserMenu)
{
	_ASSERTE(asRegFilePathName && *asRegFilePathName);
	MCHKHEAP;
	SafeFree(mpsz_HostFile);
	MCHKHEAP;

	mb_ShowRegFileMenu = abAllowUserMenu;

	//REGFILETIME ft = {{0}};

	CloseItems();
	MCHKHEAP;
	m_Key.Release();
	m_Key.Init(RE_REGFILE, mb_Wow64on32, mb_Virtualize); //, NULL, NULL, ft);
	MCHKHEAP;
	ms_RemoteServer[0] = 0;
	mb_RemoteMode = FALSE;

	_ASSERTE(mp_Items == NULL);
	SafeDelete(mp_Worker);

	mp_Worker = new MFileReg(mb_Wow64on32, mb_Virtualize);
	if (!mp_Worker)
	{
		InvalidOp();
		return FALSE;
	}

	TCHAR szPathLabel[100];
	lstrcpyn(szPathLabel, GetMsg(RELoadingRegFileText), countof(szPathLabel));
	int nLen = lstrlen(szPathLabel);
	lstrcpy_t(szPathLabel+nLen, countof(szPathLabel)-nLen-1, PointToName(asRegFilePathName));

	if (!abSilence)
	{
		gpProgress = new REProgress(GetMsg(RELoadingRegFileTitle), TRUE, szPathLabel);
	}

	LONG hRc = MFileReg::LoadRegFile(asRegFilePathName, abSilence, NULL, abDelayLoading, (MFileReg*)mp_Worker);
	mp_Worker->bDirty = false;

	if (!abSilence)
	{
		SafeDelete(gpProgress);
	}

	if (hRc != 0)
	{
		//InvalidOp();
		SafeDelete(mp_Worker);
		SafeFree(mpsz_HostFile);
		// Иначе не перерисуется открытый редактор (если он был открыт в процессе)
		psi.AdvControl(PluginNumber, ACTL_REDRAWALL, FADV1988 0);
		return FALSE;
	}

	m_Key.SetTitlePrefix(PointToName(asRegFilePathName));

	ChDir(L"\\", TRUE);
	if (!abDelayLoading)
	{
		wchar_t* pszFirst = ((MFileReg*)mp_Worker)->GetFirstKeyFilled();
		if (pszFirst)
		{
			SetDirectory(pszFirst, FALSE);
			SafeFree(pszFirst);
		}
	}

	mpsz_HostFile = lstrdup_t(asRegFilePathName);
	return TRUE;	
}

BOOL REPlugin::LoadHiveFile(LPCWSTR asHiveFilePathName, BOOL abSilence, BOOL abDelayLoading)
{
	_ASSERTE(asHiveFilePathName && *asHiveFilePathName);
	MCHKHEAP;
	SafeFree(mpsz_HostFile);
	MCHKHEAP;

	mb_ShowRegFileMenu = FALSE;

	//REGFILETIME ft = {{0}};

	CloseItems();
	MCHKHEAP;
	m_Key.Release();
	m_Key.Init(RE_HIVE, mb_Wow64on32, mb_Virtualize); //, HKEY__HIVE); //, NULL, NULL, ft);
	MCHKHEAP;
	ms_RemoteServer[0] = 0;
	mb_RemoteMode = FALSE;

	_ASSERTE(mp_Items == NULL);
	SafeDelete(mp_Worker);

	mp_Worker = new MFileHive(mb_Wow64on32, mb_Virtualize);
	if (!mp_Worker) {
		InvalidOp();
		return FALSE;
	}

	//gpProgress = new REProgress(GetMsg(RELoadingRegFileTitle), TRUE); -- подключение куста времени долго занять не должно
	LONG hRc = ((MFileHive*)mp_Worker)->LoadHiveFile(asHiveFilePathName, abSilence, abDelayLoading);
	//SafeDelete(gpProgress);
	if (hRc != 0)
	{
		MessageFmt(REM_HiveOpenFailed, asHiveFilePathName, hRc);
		SafeDelete(mp_Worker);
		SafeFree(mpsz_HostFile);
		return FALSE;
	}

	m_Key.SetTitlePrefix(PointToName(asHiveFilePathName));

	ChDir(L"\\", TRUE);

	mpsz_HostFile = lstrdup_t(asHiveFilePathName);
	return TRUE;	
}

void REPlugin::PreClosePlugin()
{
	// Проверим, были ли изменения
	if (m_Key.eType != RE_REGFILE || !mpsz_HostFile
		|| !mp_Worker || mp_Worker->eType != RE_REGFILE
		|| !mp_Worker->bDirty)
		return;
	SaveRegFile(TRUE/*abInClose*/);
}


// вернет -1, если плагин нужно закрыть
//        -2, в случае ошибки или отказа пользователя
int REPlugin::ShowRegMenu(bool abForceImport, MRegistryBase* apTarget)
{
	if (!mb_ShowRegFileMenu && !abForceImport)
		return 0; // меню уже показали, или оно не нужно

	mb_ShowRegFileMenu = FALSE;

	if (m_Key.eType != RE_REGFILE || !mp_Worker || (mp_Worker->eType != RE_REGFILE))
		return 0; // вариантов пока нет

	// GetFilePathName() возвращает LPCWSTR, который нужен в ANSI для LoadRegFile
	LPCWSTR pszRegFile = ((MFileReg*)mp_Worker)->GetFilePathName();
	if (!pszRegFile || !*pszRegFile || !mpsz_HostFile || !*mpsz_HostFile)
	{
		_ASSERTE(pszRegFile && *pszRegFile);
		return 0; // странно, HOST-файл должен быть задан
	}


	FarMenuItem items[10]; memset(items, 0, sizeof(items));
	int nItems = 0;

	#ifdef _UNICODE
		#define SETMENUITEM(i,t) items[i].Text = t
	#else
		#define SETMENUITEM(i,t) lstrcpyn(items[i].Text, t, countof(items[i].Text))
	#endif
	
	int nImportNative = -1, nImport32 = -1, nImport64 = -1, nBrowse = -1;

	if (!abForceImport)
	{
		nBrowse = nItems; SETMENUITEM(nItems++, GetMsg(REActionBrowse));
		MenuItemSetSeparator(items[nItems++]);
	}

	DWORD nAllowed = mp_Worker->GetAllowedImportStyles();
	if ((nAllowed & ris_Native))
		nImportNative = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport));
	if ((nAllowed & ris_Import32))
		nImport32 = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport32));
	if ((nAllowed & ris_Import64))
		nImport64 = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport64));
	//if (cfg->is64bitOs)
	//{
	//	// ExistKey возвращает 0 - если ключ есть

	//	// Проверить, есть ли в reg-файле HKLM или HKCR
	//	if (mp_Worker->ExistKey(HKEY_LOCAL_MACHINE,0,0) && mp_Worker->ExistKey(HKEY_CLASSES_ROOT,0,0))
	//	{
	//		// В этом случае (HKCU) импорт ведется без разницы 32/64
	//		nImportNative = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport));
	//	}
	//	else
	//	{		
	//		if (mp_Worker->ExistKey(HKEY_LOCAL_MACHINE, L"SOFTWARE", L"Wow6432Node") == 0)
	//		{
	//			nImport64 = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport64));
	//		}
	//		else
	//		{
	//			nImportNative = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport));
	//			nImport32 = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport32));
	//			nImport64 = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport64));
	//		}
	//	}
	//}
	//else
	//{
	//	// x86 OS, различий веток реестра (32bit/64bit) нет вообще
	//	nImportNative = nItems; SETMENUITEM(nItems++, GetMsg(REActionImport));
	//}

	int nCmd;

	if (nItems == 0)
		nCmd = -1;
	//else if (abForceImport && nItems == 1)
	//	nCmd = 0;
	else	
		nCmd = psi.Menu(_PluginNumber(guid_ActionMenu), -1,-1,0, FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
						GetMsg(REPluginName), NULL, NULL/*HelpTopic*/, NULL,NULL, items, nItems);
	if (nCmd < 0)
		return -2; // Если юзер отказался - FAR сразу вернется в файловую панель
	if (nBrowse != -1 && nCmd == nBrowse)
		return 0; // Продолжить, т.е. открыть reg-файл на панели

	// Выполнить импорт
	MFileReg fileReg(mb_Wow64on32, mb_Virtualize);
	//MRegistryWinApi winReg;
	
	BOOL bSaveWow64on32 = apTarget->mb_Wow64on32;
	if (cfg->is64bitOs)
	{
		if (nCmd == nImportNative)
		{
			apTarget->mb_Wow64on32 = 2; // as native for FAR manager
		}
		else if (nCmd == nImport32)
		{
			apTarget->mb_Wow64on32 = 0; // Import to the 32 bit registry
		}
		else if (nCmd == nImport64)
		{
			apTarget->mb_Wow64on32 = 1; // Import to then 64 bit registry
		}
		else
		{
			_ASSERTE(nCmd == nImportNative || nCmd == nImport32 || nCmd == nImport64);
			apTarget->mb_Wow64on32 = 2; // as native for FAR manager
		}
	}
	

	gpProgress = new REProgress(GetMsg(REImportDlgTitle), TRUE);
	
	LONG hRc = MFileReg::LoadRegFile(pszRegFile, FALSE, apTarget, FALSE, &fileReg);
	
	// Сразу вернуть настройку
	apTarget->mb_Wow64on32 = bSaveWow64on32;
	
	SafeDelete(gpProgress);
	int res = -1;
	if (hRc == 0)
	{
		if (cfg->bShowImportResult)
			REPlugin::MessageFmt(REM_ImportSuccess, mpsz_HostFile, 0, NULL, FMSG_MB_OK);
	}
	else
	{
		REPlugin::MessageFmt(REM_ImportFailed, mpsz_HostFile);
		res = -2;
	}

	return res; // Не открывать панель плагина - возврат в FAR
}

BOOL REPlugin::Transfer(REPlugin* pDstPlugin, BOOL abMove)
{
	BOOL lbRc = FALSE;

	//RegFolder folderFrom; //-- memset(&folderFrom, 0, sizeof(folderFrom));
	//folderFrom.bForceRelease = TRUE;
	//folderFrom.Init(&m_Key);
	RegFolder* folderFrom = PrepareExportKey(false, NULL, 0, true); // в путь учесть ".reg"
	if (!folderFrom)
	{
		return FALSE;
	}

	HREGKEY hKey = NULLHKEY;
	LONG hRc = 0, nItemsNumber;
	LPCTSTR pszName4Label = NULL, pszDir = NULL;
	MRegistryBase* pSource = Worker();
	MRegistryBase* pTarget = pDstPlugin->Worker();
#ifndef _UNICODE
	TCHAR szName4Label[MAX_PATH];
#endif
	if (!pSource || !pTarget)
	{
		InvalidOp();
		goto wrap;
	}

	if (folderFrom->mn_ItemCount > 0)
	{
		nItemsNumber = folderFrom->mn_ItemCount;
		if (folderFrom->mn_ItemCount == 1)
		{
			if (!folderFrom->mp_Items->pszName || !*folderFrom->mp_Items->pszName)
			{
				pszName4Label = REGEDIT_DEFAULTNAME_T;
			}
			else
			{
			#ifdef _UNICODE
				pszName4Label = folderFrom->mp_Items->pszName;
			#else
				lstrcpy_t(szName4Label, countof(szName4Label), folderFrom->mp_Items->pszName);
				pszName4Label = szName4Label;
			#endif
			}
		}
		else
		{
			pszName4Label = PointToName(folderFrom->key.mpsz_Dir);
		}
	}
	else
	{
		nItemsNumber = 1;
		pszName4Label = PointToName(folderFrom->key.mpsz_Dir);
	}

	pszDir = pDstPlugin->m_Key.mpsz_Dir;
	if (pszDir && pszDir[0] == _T('\\') && pszDir[1] && pszDir[1] != _T('\\'))
		pszDir++;

	if (!ConfirmCopyMove(abMove, nItemsNumber,
			(nItemsNumber == 1) ? RECopyItemLabel : RECopyItemsLabel,
			pszName4Label ? pszName4Label : _T(""),
			pszDir ? pszDir : _T("")))
	{
		goto wrap;
	}

	hRc = pDstPlugin->mp_Items->CreateKey(pTarget, &hKey, KEY_WRITE);
	if (hRc != 0)
	{
		pDstPlugin->CantOpenKey(&pDstPlugin->mp_Items->key, TRUE);
	}
	else
	{
		lbRc = folderFrom->Transfer(pDstPlugin, pSource, pDstPlugin->mp_Items, pTarget);
		pTarget->CloseKey(&hKey);
		hKey = NULL;
	}

wrap:
	if (folderFrom)
	{
		folderFrom->Release();
		SafeDelete(folderFrom);
	}
	pDstPlugin->UpdatePanel(FALSE);
	pDstPlugin->RedrawPanel();
	this->UpdatePanel(lbRc!=FALSE);
	this->RedrawPanel();
	return lbRc;
}
