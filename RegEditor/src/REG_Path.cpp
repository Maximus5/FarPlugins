
#include "header.h"

void RegPath::Init(RegPath* apKey)
{
	MCHKHEAP;
	Init(apKey->eType, apKey->mb_Wow64on32, apKey->mb_Virtualize, apKey->mh_Root, apKey->mpsz_Key, &(apKey->ftModified), apKey->mpsz_TitlePrefix, apKey->eRights, apKey->nKeyFlags, apKey->mpsz_Server);
}

void RegPath::Init(RegWorkType aType, DWORD abWow64on32, BOOL abVirtualize, HKEY ahRoot /*= NULL*/, LPCWSTR asKey /*= NULL*/, REGFILETIME* aftModified /*= NULL*/, LPCTSTR asPrefix /*= NULL*/, RegKeyOpenRights aRights /*= eRightsSimple*/, DWORD anKeyFlags /*= 0*/, LPCTSTR asServer /*= NULL*/)
{
	_ASSERTE(aType != RE_UNDEFINED);
	_ASSERTE(!mpsz_Key && !mpsz_Title && !mpsz_Dir && !mpsz_TitlePrefix && !mpsz_Server);
	// Инициализация переменных (в 0)
	eType = aType;
	mh_Root = ahRoot;
	mpsz_Key = NULL;
	mpsz_Title = NULL; mn_PrefixLen = 0;
	mpsz_Dir = NULL;
	mn_MaxKeySize = NULL;
	if (aftModified)
		ftModified = *aftModified;
	else
		ftModified.QuadPart = 0;
	mpsz_TitlePrefix = NULL; mn_PrefixLen = 0;
	nKeyFlags = 0;
	pszComment = NULL;
	mpsz_Server = NULL; mn_ServerLen = 0;
	eRights = aRights;
	nKeyFlags = anKeyFlags;
	mb_Wow64on32 = abWow64on32;
	mb_Virtualize = abVirtualize;
	
	//// Host пока отдельно
	//mpsz_HostFile = NULL;

	MCHKHEAP;


	SetTitlePrefix(asPrefix, TRUE);
	SetServer(asServer, TRUE);


	// Теперь - выделить память
	int nAddLen = lstrlenW(asKey);
	AllocateAddLen(max(nAddLen,MAX_PATH));
	if (asKey)
		lstrcpyW(mpsz_Key, asKey);
	else
		mpsz_Key[0] = 0;


	MCHKHEAP;

	// Обновить заголовки и пути
	Update();
}

void RegPath::SetServer(LPCSTR asServer, BOOL abNoUpdate /*= FALSE*/)
{
	SafeFree(mpsz_Server);
	if (asServer && *asServer)
	{
		int nLen = lstrlenA(asServer);
		mpsz_Server = (TCHAR*)malloc((nLen+2)*sizeof(TCHAR));
		#ifdef _UNICODE
		lstrcpy_t(mpsz_Server, nLen+1, asServer);
		#else
		lstrcpyn(mpsz_Server, asServer, nLen+1);
		#endif
		//mpsz_Server[nLen] = _T('\\'); mpsz_Server[nLen+1] = 0;
		if (mpsz_Title && (mn_ServerLen < nLen))
			ReallocTitleDir();
		mn_ServerLen = nLen;
	}
	else
	{
		mn_ServerLen = 0;
	}
	if (!abNoUpdate)
		Update();
}

void RegPath::SetServer(LPCWSTR asServer, BOOL abNoUpdate /*= FALSE*/)
{
	SafeFree(mpsz_Server);
	if (asServer && *asServer)
	{
		int nLen = lstrlenW(asServer);
		mpsz_Server = (TCHAR*)malloc((nLen+2)*sizeof(TCHAR));
		lstrcpy_t(mpsz_Server, nLen+1, asServer);
		//mpsz_Server[nLen] = _T('\\'); mpsz_Server[nLen+1] = 0;
		if (mpsz_Title && (mn_ServerLen < nLen))
			ReallocTitleDir();
		mn_ServerLen = nLen;
	}
	else
	{
		mn_ServerLen = 0;
	}
	if (!abNoUpdate)
		Update();
}

void RegPath::SetTitlePrefix(LPCSTR asPrefix, BOOL abNoUpdate /*= FALSE*/)
{
	SafeFree(mpsz_TitlePrefix);
	if (asPrefix && *asPrefix)
	{
		int nLen = lstrlenA(asPrefix);
		if (nLen > 32) nLen = 32;
		mpsz_TitlePrefix = (TCHAR*)malloc((nLen+1)*2);
		#ifdef _UNICODE
		lstrcpy_t(mpsz_TitlePrefix, nLen+1, asPrefix);
		#else
		lstrcpyn(mpsz_TitlePrefix, asPrefix, nLen+1);
		#endif
	}
	if (!abNoUpdate)
		Update();
}

void RegPath::SetTitlePrefix(LPCWSTR asPrefix, BOOL abNoUpdate /*= FALSE*/)
{
	SafeFree(mpsz_TitlePrefix);
	if (asPrefix && *asPrefix)
	{
		int nLen = lstrlenW(asPrefix);
		//if (nLen > 32) nLen = 32;
		mpsz_TitlePrefix = (TCHAR*)malloc((nLen+1)*2);
		lstrcpy_t(mpsz_TitlePrefix, nLen+1, asPrefix);
		if (mpsz_Title && (mn_PrefixLen < nLen))
			ReallocTitleDir();
		mn_PrefixLen = nLen;
	}
	else
	{
		mn_PrefixLen = 0;
	}
	if (!abNoUpdate)
		Update();
}

// освободить память
void RegPath::Release()
{
	SafeFree(mpsz_Key);
	SafeFree(mpsz_Title);
	SafeFree(mpsz_Dir);
	//if (mpsz_HostFile) { SafeFree(mpsz_HostFile); mpsz_HostFile = NULL; }
	SafeFree(mpsz_TitlePrefix); mn_PrefixLen = 0;
	SafeFree(mpsz_Server); mn_ServerLen = 0;
	mn_MaxKeySize = 0;
	eRights = eRightsSimple;
	nKeyFlags = 0;
}

bool RegPath::IsEmpty()
{
	if (mpsz_Key && *mpsz_Key)
		return false;
	else if (mh_Root)
		return false;
	else
		return true;
}

// сравнить ключи
bool RegPath::IsEqual(struct RegPath *p)
{
	if (!p)
		return false;
	if (eType != p->eType || mh_Root != p->mh_Root)
		return false;
	
	if (mpsz_Key == p->mpsz_Key)
		return true;
		
	if (!mpsz_Key || !p->mpsz_Key)
		return false;

	if (mb_Wow64on32 != p->mb_Wow64on32)
		return false;
		
	int nCmp = lstrcmpiW(mpsz_Key, p->mpsz_Key);
	return (nCmp == 0);
}

void RegPath::ReallocTitleDir()
{
	// TCHAR(Unicode or OEM) path (make a copy with enough storage) + HKEY_xxx prefix
	TCHAR* pszNewT = (TCHAR*)calloc(mn_MaxKeySize+40+mn_ServerLen+1, sizeof(TCHAR));
	SafeFree(mpsz_Dir);
	mpsz_Dir = pszNewT;

	// + "REG[x64]:" + prefix + HKEY_xxx + " (*)"
	pszNewT = (TCHAR*)calloc(mn_MaxKeySize+cfg->TitleAddLen()+mn_ServerLen+mn_PrefixLen, sizeof(TCHAR));
	SafeFree(mpsz_Title);
	mpsz_Title = pszNewT;
}

// Убедиться, что выделено достаточно памяти под все поля
void RegPath::AllocateAddLen(int anAddLen)
{
	int nCurLen = 0;
	if (mpsz_Key != NULL)
	{
		nCurLen = lstrlenW(mpsz_Key);
		if ((nCurLen + anAddLen + 1) <= mn_MaxKeySize)
			return; // выделять память не требуется
	}

	mn_MaxKeySize = nCurLen + anAddLen + 1;
	// Path (make a copy with enough storage)
	MCHKHEAP;
	wchar_t* pszNew = (wchar_t*)malloc(mn_MaxKeySize*sizeof(wchar_t));
	if (mpsz_Key)
	{
		lstrcpyW(pszNew, mpsz_Key);
		SafeFree(mpsz_Key);
	}
	else
	{
		pszNew[0] = 0;
	}
	mpsz_Key = pszNew;

	// mpsz_Dir & mpsz_Title
	ReallocTitleDir();
	
	//// TCHAR(Unicode or OEM) path (make a copy with enough storage) + HKEY_xxx prefix
	//TCHAR* pszNewT = (TCHAR*)calloc(mn_MaxKeySize+40, sizeof(TCHAR));
	//SafeFree(mpsz_Dir);
	//mpsz_Dir = pszNewT;
	//// + "REG[x64]:" + prefix + HKEY_xxx + " (*)"
	//pszNewT = (TCHAR*)calloc(mn_MaxKeySize+cfg->TitleAddLen()+mn_ServerLen+mn_PrefixLen, sizeof(TCHAR));
	//SafeFree(mpsz_Title);
	//mpsz_Title = pszNewT;
	
	// mpsz_HostFile - не трогаем, он не меняется
	
	Update();
}

// Обновить все поля в соответствии с новым ключом (mpsz_Key)
void RegPath::Update()
{
	MCHKHEAP;

	mpsz_Title[0] = 0;
	mpsz_Dir[0] = 0;
	
	LPCTSTR pszBackupRestore = (eRights == eRightsSimple) ? _T("") : GetMsg((eRights == eRightsBackup) ? REPanelBackupPrefix : REPanelBackupRestorePrefix);
	_ASSERTE(pszBackupRestore!=NULL);

	TCHAR* pszDirPtr = mpsz_Dir;
	if (mpsz_Server)
	{
		lstrcpy(mpsz_Dir, mpsz_Server);
		pszDirPtr += mn_ServerLen;
	}

	if (mh_Root)
	{
		if (mh_Root != HKEY__HIVE)
		{
			wchar_t szRoot[40];
			if (mh_Root && !IsKeyPredefined(mh_Root))
			{
				pszDirPtr[0] = 0;
			}
			else if (!HKeyToStringKey(mh_Root, szRoot, 40))
			{
				// Неизвестный ключ!
				_ASSERTE(mh_Root==HKEY_CLASSES_ROOT);
			} else {
				*pszDirPtr = _T('\\');
				lstrcpy_t(pszDirPtr+1, 39, szRoot);
			}
			MCHKHEAP;
		}
	}
	else if (eType == RE_HIVE)
	{
		mh_Root = HKEY__HIVE; // Это требуется для "красивой" выгрузки в *.reg
	}
	
	if (*mpsz_Key)
	{
		lstrcat(pszDirPtr, _T("\\"));
		lstrcpy_t(pszDirPtr+lstrlen(pszDirPtr), mn_MaxKeySize, mpsz_Key);
	}
	
	MCHKHEAP;
	lstrcpy(mpsz_Title, cfg->sRegTitlePrefix);
	lstrcat(mpsz_Title, BitSuffix(mb_Wow64on32, mb_Virtualize));
	if (pszBackupRestore && *pszBackupRestore) lstrcat(mpsz_Title, pszBackupRestore);
	lstrcat(mpsz_Title, _T(":"));
	MCHKHEAP;
	if (mpsz_TitlePrefix && *mpsz_TitlePrefix)
	{
		lstrcat(mpsz_Title, mpsz_TitlePrefix);
		MCHKHEAP;
		lstrcat(mpsz_Title, _T(":"));
	}
	if (mpsz_Server && *mpsz_Server)
	{
		lstrcat(mpsz_Title, mpsz_Server);
		lstrcat(mpsz_Title, _T("\\"));
	}
	MCHKHEAP;
	lstrcat(mpsz_Title, (*pszDirPtr == _T('\\')) ? (pszDirPtr+1) : pszDirPtr);
	MCHKHEAP;
#ifdef _DEBUG
	if (mpsz_Title[0] == mpsz_Title[0])
		mpsz_Title[0] = mpsz_Title[0];
#endif
}

// asKey может быть
// "\\" -- перейти в m_Key.mh_Root=NULL (список из HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, и т.п.)
// ".." -- перейти на уровень вверх
// "."  -- ничего не делать
// <KeyName> - перейти в под-ключ. проверяется существование ключа
// Если abRaw - то "." и ".." считаются ключами, и в них осуществляется заход!
BOOL RegPath::ChDir(LPCWSTR asKey, BOOL abSilence, REPlugin* pPlugin, BOOL abRawKeyName /*= FALSE*/)
{
	MCHKHEAP;
	nKeyFlags = 0;
	pszComment = NULL;
	if (asKey[0] == L'\\')
	{
		_ASSERTE(asKey[1] == 0);
		mh_Root = NULL; //(eType == RE_HIVE) ? HKEY__HIVE : NULL;
		mpsz_Key[0] = 0;
		Update(); // Обновит OEM & Title
		return TRUE;
	}
	
	if (!abRawKeyName && asKey[0] == L'.')
	{
		if (asKey[1] == 0)
			return TRUE; // "." - ничего не делать
		if (asKey[1] == L'.' && asKey[2] == 0)
		{
			// Переход на уровень вверх
			if (mpsz_Key[0] /*&& (mh_Root || eType == RE_HIVE)*/)
			{
				// Отрезать один слеш
				wchar_t* pszSlash = wcsrchr(mpsz_Key, L'\\');
				if (pszSlash)
					*pszSlash = 0;
				else
					mpsz_Key[0] = 0;
			}
			else
			{
				mh_Root = NULL; //(eType == RE_HIVE) ? HKEY__HIVE : NULL;
				mpsz_Key[0] = 0;
			}
			//TODO: по хорошему надо бы проверить существует ли ключ, в который мы попали
			//TODO: т.к. родительский ключ мог быть полностью удален.
			Update();
			return TRUE;
		}
	}
	MCHKHEAP;
	
	if (mh_Root == NULL && eType == RE_WINAPI)
	{
		// "Переход" в что-то из HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, и т.п.
		if (!StringKeyToHKey(asKey, &mh_Root))
		{
			if (!abSilence && pPlugin) pPlugin->KeyNotExist(this, asKey);
			return FALSE;
		}
		mpsz_Key[0] = 0;
		MCHKHEAP;
	}
	else
	{
		if (!TestKey(asKey))
		{
			if (!abSilence && pPlugin) pPlugin->KeyNotExist(this, asKey);
			return FALSE;
		}
		MCHKHEAP;
		// По идее, память уже должна быть выделена из функции SetDirectoryW, но удостоверимся
		AllocateAddLen(lstrlenW(asKey)+2); // + '\\'
		// Добавляем
		MCHKHEAP;
		if (mpsz_Key[0])
		{
			lstrcatW(mpsz_Key, L"\\");
			MCHKHEAP;
			lstrcatW(mpsz_Key, asKey);
			MCHKHEAP;
		}
		else
		{
			lstrcpyW(mpsz_Key, asKey);
			MCHKHEAP;
		}
	}
	
	Update();
	return TRUE;
}

BOOL RegPath::ChDir(RegItem* apKey, BOOL abSilence, REPlugin* pPlugin)
{
	BOOL lbRc = ChDir(apKey->pszName, abSilence, pPlugin, TRUE);
	if (lbRc) {
		nKeyFlags = apKey->nFlags;
		pszComment = apKey->pszComment;
	}
	return lbRc;
}

BOOL RegPath::TestKey(LPCWSTR asSubKey)
{
	if (!asSubKey || !*asSubKey)
		return FALSE;
	//TODO: Проверить существование текущего ключа m_Key.mh_Root\m_Key.mpsz_Key
	//TODO: Если asSubKey!=NULL - проверяется подключ
	return TRUE;
}

BOOL RegPath::IsKeyPredefined(HKEY ahKey)
{
	if (ahKey && (((ULONG_PTR)ahKey) & HKEY__PREDEFINED_MASK) == HKEY__PREDEFINED_TEST)
		return TRUE;
	return FALSE;
}
