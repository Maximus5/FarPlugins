
#include "header.h"
#include "RE_CmdLine.h"
#include "RE_Guids.h"

OpenPluginArg::OpenPluginArg()
{
	memset(this, 0, sizeof(*this));
}

void OpenPluginArg::ReleaseMem()
{
	int i;
	switch (eAction)
	{
	case aBrowseLocal:
		SafeFree(wsBrowseLocalKey);
		break;
	case aBrowseRemote:
		SafeFree(wsBrowseRemoteKey);
		SafeFree(wsBrowseRemoteLogin);
		SafeFree(wsBrowseRemotePassword);
		break;
	case aBrowseFileReg:
	case aBrowseFileHive:
		SafeFree(wsBrowseFileName);
		break;
	case aExportKeysValues:
		for (i = 0; i < nExportKeysOrValuesCount; i++)
			SafeFree(ppszExportKeysOrValues[i]);
		SafeFree(ppszExportKeysOrValues);
		SafeFree(pszExportDestFile);
		break;
	case aImportRegFile:
		SafeFree(pszSourceFile);
		break;
	case aMountHive:
		SafeFree(pszMountHiveFilePathName);
		SafeFree(pszMountHiveKey);
		break;
	case aUnmountHive:
		SafeFree(pszUnmountHiveKey);
		break;
		
	default:
		_ASSERTE(pszSourceFile == NULL);
	};
}

OpenPluginArg::~OpenPluginArg()
{
	ReleaseMem();
}

// –егистронезависимое сравнение символа
//#define CHREQ(s,i,c) ( ((((DWORD)(s)[i])|0x20) == (((DWORD)c)|0x20)) || ((((DWORD)(s)[i])&~0x20) == (((DWORD)c)&~0x20)) )
//bool OpenPluginArg::CHREQ(const wchar_t* s, int i, wchar_t c)
//{
//	//bool bRc = ( ((((DWORD)(s)[i])|0x20) == (((DWORD)c)|0x20)) || ((((DWORD)(s)[i])&~0x20) == (((DWORD)c)&~0x20)) );
//	bool bRc = (s[i] == c) || (s[i] == (c&~0x20));
//	return bRc;
//}

bool OpenPluginArg::IsFilePath(LPCWSTR apsz)
{
	if (apsz[0] == L'\\' && apsz[1] == L'\\' && apsz[2])
		return true; // считаем сетевым путем
	if (((apsz[0] >= L'A' && apsz[0] <= L'Z') || (apsz[0] >= L'a' && apsz[0] <= L'z'))
		&& apsz[1] == L':')
		return true; // полный путь с диском
	// путь может быть относительным путем или просто именем файла, но это потом
	return false;
}

//bool OpenPluginArg::IsRegPath(LPCWSTR apsz, HKEY* phRootKey /*= NULL*/, LPCWSTR* ppszSubKey /*= NULL*/, BOOL abCheckExist /*= FALSE*/)
//{
//	if (*apsz == L'\\' || *apsz == L'"' || *apsz == L'[') apsz++;
//
//	HKEY hRoot = NULL;
//	wchar_t sFirstToken[MAX_REGKEY_NAME+1];
//	LPCWSTR pszSlash = wcschr(apsz, L'\\');
//	int nLen;
//	if (pszSlash) {
//		nLen = pszSlash - apsz;
//		if (nLen > MAX_REGKEY_NAME)
//			return false;
//		memmove(sFirstToken, apsz, 2*nLen);
//		sFirstToken[nLen] = 0;
//	} else {
//		nLen = lstrlenW(apsz);
//		if (nLen > MAX_REGKEY_NAME)
//			return false;
//		lstrcpynW(sFirstToken, apsz, MAX_REGKEY_NAME+1);
//	}
//	if (!StringKeyToHKey(sFirstToken, &hRoot))
//		return false;
//	if ((((ULONG_PTR)hRoot) < HKEY__FIRST) || (((ULONG_PTR)hRoot) > HKEY__LAST))
//		return false;
//	
//	if (phRootKey)
//		*phRootKey = hRoot;
//
//	if (!pszSlash)
//	{
//		if (ppszSubKey) *ppszSubKey = apsz+lstrlenW(apsz);
//		return true;
//	}
//
//	apsz = pszSlash+1;
//	if (ppszSubKey) *ppszSubKey = apsz;
//
//	if (!abCheckExist)
//	{
//		return true;
//	}
//	
//	pszSlash = wcschr(apsz, L'\\');
//	if (pszSlash) {
//		nLen = pszSlash - apsz;
//		if (nLen > MAX_REGKEY_NAME)
//			return false;
//		memmove(sFirstToken, apsz, 2*nLen);
//		sFirstToken[nLen] = 0;
//	} else {
//		nLen = lstrlenW(apsz);
//		if (nLen > MAX_REGKEY_NAME)
//			return false;
//		lstrcpynW(sFirstToken, apsz, MAX_REGKEY_NAME+1);
//	}
//
//	MRegistryWinApi reg;
//	if (reg.ExistKey(hRoot, sFirstToken, NULL) == 0)
//		return true;
//
//	return false;
//}

void OpenPluginArg::SkipSpaces(wchar_t*& psz)
{
	while (*psz==_T(' ') || *psz==_T('\t') || *psz==_T('\r') || *psz==_T('\n') || *psz==(TCHAR)0xA0)
		psz++;
}

// ѕолучить токен
wchar_t* OpenPluginArg::GetToken(wchar_t*& psz, BOOL abForceLast /*= FALSE*/)
{
	SkipSpaces(psz);
	wchar_t* pszStart = psz;

	wchar_t wc;
	if (*psz == L'"')
	{
		// ƒо закрывающей кавычки, или до конца строки
		psz++; pszStart = psz;
		while ((wc = *psz) != 0)
		{
			if (wc == L'"') break;
			psz++;
		}
		_ASSERTE(*psz==0 || *psz==L'"');
		if (*psz == L'"') {
			*psz = 0; psz++;
		}

	} else if (abForceLast) {
		// Ќичего мен€ть не нужно

	} else {
		// ƒо пробела?
		pszStart = psz;
		while ((wc = *psz) != 0)
		{
			if (wc == L' ') break;
			psz++;
		}
		_ASSERTE(*psz==0 || *psz==L' ');
		if (*psz == L' ') {
			*psz = 0; psz++;
		}
	}

	return pszStart;
}

// –азбор
int OpenPluginArg::ParseCommandLine(LPCTSTR asCmdLine)
{
	_ASSERTE(eAction == aInvalid);
	while (*asCmdLine==_T(' ') || *asCmdLine==_T('\t') || *asCmdLine==_T('\r') || *asCmdLine==_T('\n') || *asCmdLine==(TCHAR)0xA0)
		asCmdLine++;

	// ≈сли ком-строка пуста€ - сразу выходим с aBrowseLocal
	if (!asCmdLine || !*asCmdLine || (asCmdLine[0] == L'\\' && asCmdLine[1] == 0))
	{
		MCHKHEAP;
		return (eAction = aBrowseLocal);
	}

	int nLen = lstrlen(asCmdLine);
	wchar_t* pszCmdLine = (wchar_t*)malloc((nLen+2)*2);
	lstrcpy_t(pszCmdLine, nLen+1, asCmdLine);
	wchar_t* psz = pszCmdLine;
	bool bErrShown = false;

	// ѕрефикс "reg:reg:HKEY_xxx\..."
	if (CHREQ(psz,0,L'r') && CHREQ(psz,1,L'e') && CHREQ(psz,2,L'g') && CHREQ(psz,3,L':'))
	{
		eAction = aBrowseLocal; psz += 4; SkipSpaces(psz);
		// GetToken все поправит с учетом возможных кавычек
		wsBrowseLocalKey = lstrdup(GetToken(psz, TRUE));
		// Ёто может быть строка, скопированна€ из *.reg
		// "[HKEY_CURRENT_USER\Software\Far\Users]"
		DebracketRegPath(wsBrowseLocalKey);
		// “ут - допустим и пустой путь (откроет последний запомненный путь реестра)
		MCHKHEAP;
	}
	// ѕрефикс "reg:file:<*.reg or hive filename>"
	else if ((CHREQ(psz,0,L'f') && CHREQ(psz,1,L'i') && CHREQ(psz,2,L'l') && CHREQ(psz,3,L'e') && CHREQ(psz,4,L':')))
	//	|| IsFilePath(psz))
	{
		eAction = aBrowseFileReg; psz += 5; SkipSpaces(psz);
		// GetToken все поправит с учетом возможных кавычек
		wsBrowseFileName = ExpandPath(GetToken(psz, TRUE));
		if (!wsBrowseFileName || !*wsBrowseFileName) {
			REPlugin::Message(REM_CmdLineFail_OpenRegFile); bErrShown = true;
			ReleaseMem();
			eAction = aInvalid;
		}
		MCHKHEAP;
	}
	// reg:export:[/reg4 | /reg5 | /hive] "HKEY_xxx[\path[\[value]]]" ["HKEY_xxx[\path[\[value]]]" [...]] "<TargetDir>\[<TagetFile>]"
	else if (CHREQ(psz,0,L'e') && CHREQ(psz,1,L'x') && CHREQ(psz,2,L'p') && CHREQ(psz,3,L'o') && CHREQ(psz,4,L'r') && CHREQ(psz,5,L't') && CHREQ(psz,6,L':'))
	{
		eAction = aExportKeysValues; psz += 7; SkipSpaces(psz);
		int nMaxCount = 20;
		nExportKeysOrValuesCount = 0; nExportFormat = cfg->bCreateUnicodeFiles ? eExportReg5 : eExportReg4;
		ppszExportKeysOrValues = (wchar_t**)calloc(nMaxCount,sizeof(wchar_t*));
		while (*psz)
		{
			wchar_t* pszToken = GetToken(psz);
			if (!pszToken || !*pszToken) break;
			if (*pszToken == L'/') {
				if (lstrcmpiW(pszToken, L"/reg4") == 0)
					nExportFormat = eExportReg4;
				else if (lstrcmpiW(pszToken, L"/reg5") == 0)
					nExportFormat = eExportReg5;
				else if (lstrcmpiW(pszToken, L"/hive") == 0)
					nExportFormat = eExportHive;
			} else {
				// ≈сли не хватило указателей - добавить
				if (nExportKeysOrValuesCount >= nMaxCount) {
					nMaxCount = nExportKeysOrValuesCount + 20;
					wchar_t** ppszNew = (wchar_t**)calloc(nMaxCount,sizeof(wchar_t*));
					memmove(ppszNew, ppszExportKeysOrValues, sizeof(wchar_t*)*nExportKeysOrValuesCount);
					SafeFree(ppszExportKeysOrValues);
					ppszExportKeysOrValues = ppszNew;
				}
				ppszExportKeysOrValues[nExportKeysOrValuesCount] = lstrdup(pszToken);
				nExportKeysOrValuesCount++;
			}
		}
		// ѕоследний параметр - им€ результирующего файла
		if (nExportKeysOrValuesCount > 0) {
			pszExportDestFile = ExpandPath(ppszExportKeysOrValues[nExportKeysOrValuesCount-1]);
			SafeFree(ppszExportKeysOrValues[nExportKeysOrValuesCount-1]);
			nExportKeysOrValuesCount--;
		}
		if (nExportKeysOrValuesCount == 0 || pszExportDestFile == NULL) {
			REPlugin::Message(REM_CmdLineFail_Export); bErrShown = true;
			ReleaseMem();
			eAction = aInvalid;
		}
		MCHKHEAP;
	}
	// reg:import:<*.reg file name>
	else if (CHREQ(psz,0,L'i') && CHREQ(psz,1,L'm') && CHREQ(psz,2,L'p') && CHREQ(psz,3,L'o') && CHREQ(psz,4,L'r') && CHREQ(psz,5,L't') && CHREQ(psz,6,L':'))
	{
		eAction = aImportRegFile; psz += 7; SkipSpaces(psz);
		// GetToken все поправит с учетом возможных кавычек
		pszSourceFile = ExpandPath(GetToken(psz, TRUE));
		if (!pszSourceFile || !*pszSourceFile) {
			REPlugin::Message(REM_CmdLineFail_Import); bErrShown = true;
			ReleaseMem();
			eAction = aInvalid;
		}
		MCHKHEAP;
	}
	// reg:mount:"<hive file name>" "HKEY_USERS\NewKeyName"
	else if (CHREQ(psz,0,L'm') && CHREQ(psz,1,L'o') && CHREQ(psz,2,L'u') && CHREQ(psz,3,L'n') && CHREQ(psz,4,L't') && CHREQ(psz,5,L':'))
	{
		eAction = aMountHive; psz += 6; SkipSpaces(psz);
		// GetToken все поправит с учетом возможных кавычек
		pszMountHiveFilePathName = ExpandPath(GetToken(psz));
		if (!pszMountHiveFilePathName || !*pszMountHiveFilePathName) {
			REPlugin::Message(REM_CmdLineFail_HiveFile); bErrShown = true;
			ReleaseMem();
			eAction = aInvalid;
		} else {
			pszMountHiveKey = lstrdup(GetToken(psz));
			if (pszMountHiveKey && *pszMountHiveKey) {
				if (wcschr(pszMountHiveKey, L'\\')) {
					if (!IsRegPath(pszMountHiveKey, &hRootMountKey, &pszMountHiveSubKey, FALSE))
						*pszMountHiveKey = 0;
				} else {
					hRootMountKey = HKEY_USERS; pszMountHiveSubKey = pszMountHiveKey;
				}
			}
			if (!pszMountHiveKey || !*pszMountHiveKey)
			{
				REPlugin::Message(REM_CmdLineFail_HiveKey); bErrShown = true;
				ReleaseMem();
				eAction = aInvalid;
			}
		}
		MCHKHEAP;
	}
	// reg:unmount:HKEY_USERS\NewKeyName
	else if (CHREQ(psz,0,L'u') && CHREQ(psz,1,L'n') && CHREQ(psz,2,L'm') && CHREQ(psz,3,L'o') && CHREQ(psz,4,L'u') && CHREQ(psz,5,L'n') && CHREQ(psz,6,L't') && CHREQ(psz,7,L':'))
	{
		eAction = aUnmountHive; psz += 8; SkipSpaces(psz);
		// GetToken все поправит с учетом возможных кавычек
		pszUnmountHiveKey = lstrdup(GetToken(psz));
		if (pszUnmountHiveKey && *pszUnmountHiveKey) {
			if (wcschr(pszUnmountHiveKey, L'\\')) {
				if (!IsRegPath(pszUnmountHiveKey, &hRootUnmountKey, &pszUnmountHiveSubKey, TRUE))
					*pszUnmountHiveKey = 0;
			} else {
				hRootUnmountKey = HKEY_USERS; pszUnmountHiveSubKey = pszUnmountHiveKey;
			}
		}
		if (!pszUnmountHiveKey || !*pszUnmountHiveKey)
		{
			REPlugin::Message(REM_CmdLineFail_Unmount); bErrShown = true;
			ReleaseMem();
			eAction = aInvalid;
		}
		MCHKHEAP;
	}
	else
	{
		// »наче - это может быть либо ключ реестра в форме
		// reg:HKEY_xxx\path
		// reg:HKCR\path
		// reg:\HKEY_xxx\path
		// reg:\HKCR\path
		// ..
		// Ћибо файл с путем или без
		// reg:<*.reg or hive filename>
		// ..
		// »ли сетевой реестр
		// reg:"/server=<ServerName>" "/user=<Login>" "/password=<Pwd>" [HKEY_xxx[\path]]
		// reg:\\<ServerName>\HKEY_xxx\...
		
		if (psz[0] == L'\\' && psz[1] == L'\\' && psz[2] && psz[2] != L'\\')
		{
			wchar_t* pszKeyStart = wcschr(psz+2, L'\\');
			if (pszKeyStart) *(pszKeyStart++) = 0;
			eAction = aBrowseRemote;
			wsBrowseRemoteServer = lstrdup(psz+2);
			wsBrowseRemoteKey = lstrdup(pszKeyStart ? pszKeyStart : L"");
			
		}
		else if ((CHREQ(psz,0,L'h') && CHREQ(psz,1,L'k'))
			|| ((*psz==L'\\' || *psz==L'[') && CHREQ(psz,1,L'h') && CHREQ(psz,2,L'k'))
			|| (*psz==L'"' && CHREQ(psz,1,L'h') && CHREQ(psz,2,L'k'))
			|| (*psz==L'"' && (*(psz+1)==L'\\' || *(psz+1)==L'[') && CHREQ(psz,2,L'h') && CHREQ(psz,3,L'k'))
			)
		{
			// Ёто может быть путь реестра HKEY или HKCR,...
			if (IsRegPath(psz))
			{
				eAction = aBrowseLocal; SkipSpaces(psz);
				// GetToken все поправит с учетом возможных кавычек
				wchar_t* pszKeyStart = GetToken(psz, TRUE);
				DebracketRegPath(pszKeyStart);
				if (*pszKeyStart == L'\\') pszKeyStart++;
				wsBrowseLocalKey = lstrdup(pszKeyStart);
			}
			MCHKHEAP;
		}
		// Ёто может быть строка, скопированна€ из *.reg
		// "[HKEY_CURRENT_USER\Software\Far\Users]"

		// ѕытаемс€ подобрать дальше
		if (eAction == aInvalid)
		{
			//wchar_t* wsBrowseRemoteKey;
			//wchar_t* wsBrowseRemoteServer;
			//wchar_t* wsBrowseRemoteLogin;
			//wchar_t* wsBrowseRemotePassword;
			if (*psz == L'/' || (*psz == L'"' && *(psz+1) == L'/'))
			{
				eAction = aBrowseRemote;
				while (*psz)
				{
					wchar_t* pszToken = GetToken(psz), *pszEq;
					if (!pszToken || !*pszToken) break;
					if (*pszToken == L'/') {
						pszEq = wcschr(pszToken, L'=');
						if (!pszEq) {
							ReleaseMem();
							eAction = aInvalid; break;
						}
						*pszEq = 0;
						if (CHREQ(pszToken,1,L's')) // /server
							wsBrowseRemoteServer = lstrdup(pszEq+1);
						else if (CHREQ(pszToken,1,L'u') || CHREQ(pszToken,1,L'l')) // /user, /login
							wsBrowseRemoteLogin = lstrdup(pszEq+1);
						else if (CHREQ(pszToken,1,L'p')) // /password
							wsBrowseRemotePassword = lstrdup(pszEq+1);
					} else {
						wchar_t* pszKeyStart = pszToken;
						if (!wsBrowseRemoteServer || !*wsBrowseRemoteServer)
						{
							if (pszKeyStart[0] == L'\\' && pszKeyStart[1] == L'\\' && pszKeyStart[2] && pszKeyStart[2] != L'\\')
							{
								wchar_t* pszKeySlash = wcschr(pszKeyStart+2, L'\\');
								if (pszKeySlash) *pszKeySlash = 0;
								wsBrowseRemoteServer = lstrdup(pszKeyStart+2);
								if (pszKeySlash)
									pszKeyStart = pszKeySlash + 1;
								else
									pszKeyStart = pszKeyStart + lstrlenW(pszKeyStart);
							}
							// —ервера таки нет
							if (!wsBrowseRemoteServer || !*wsBrowseRemoteServer)
								break;
						}
						if (*pszKeyStart == L'\\') pszKeyStart++;
						wsBrowseRemoteKey = lstrdup(pszKeyStart);
					}
				}
				if ((eAction == aBrowseRemote) && (!wsBrowseRemoteServer || !*wsBrowseRemoteServer))
				{
					REPlugin::Message(REM_CmdLineFail_Remote); bErrShown = true;
					ReleaseMem();
					eAction = aInvalid;
				} else if (eAction == aInvalid) {
					REPlugin::Message(REM_CmdLineFail_RemoteArg); bErrShown = true;
				}
				MCHKHEAP;
			}
			else // «начит остаетс€ только "‘айл"
			{
				// GetToken все поправит с учетом возможных кавычек
				wchar_t* pszToken = GetToken(psz, TRUE);
				if (!ValidatePath(pszToken))
				{
					ReleaseMem();
					eAction = aInvalid; bErrShown = false; // ниже будет показана REM_CmdLineFail_UnknownCommand
				}
				else
				{
					wsBrowseFileName = ExpandPath(pszToken);
					if (!wsBrowseFileName || !*wsBrowseFileName) {
						REPlugin::Message(REM_CmdLineFail_OpenRegFile); bErrShown = true;
						ReleaseMem();
						eAction = aInvalid;
					} else {
						//TODO: Ёто может быть куст (hive)!
						eAction = aBrowseFileReg;
					}
				}
				MCHKHEAP;
			}
		}
	}

	MCHKHEAP;

	if (eAction == aBrowseFileReg) {
		_ASSERTE(wsBrowseFileName);
		LPCWSTR pszExt = PointToExt(wsBrowseFileName);
		if (!pszExt || lstrcmpiW(pszExt, L".reg") != 0)
			eAction = aBrowseFileHive;
	}

	// Result
	SafeFree(pszCmdLine);
	// ќшибка?
	if (eAction == aInvalid && !bErrShown)
	{
		//REPlugin::Message(REM_CmdLineFail_UnknownCommand);
		LPCTSTR sLines[] = {
			GetMsg(REPluginName),
			GetMsg(REM_CmdLineFail_UnknownCommand),
			asCmdLine ? asCmdLine : _T("\"\"")
		};
		psi.Message(_PluginNumber(guid_UnknownCmd), FMSG_WARNING|FMSG_MB_OK, _T("CommandLine"),
			sLines, countof(sLines), 0);
	}
	MCHKHEAP;
	return eAction;
}
