//дополнительные описания к ключам
const TCHAR *HelpTopics[] =
{
  _T("DisplayName"),
  _T(""),
  _T("InstallLocation"),
  _T("ModifyPath"),
  _T("UninstallString"),
  _T("Publisher"),
  _T("URLInfoAbout"),
  _T("URLUpdateInfo"),
  _T("Comments"),
  _T("DisplayVersion"),
  _T("InstallDate") // Must be last item!
};

enum
{
  LIST_BOX,
  DMU_UPDATE = DM_USER+1
};

enum
{
  DisplayName,
  RegLocation,
  InstallLocation,
  ModifyPath,
  UninstallString,
  Publisher,
  URLInfoAbout,
  URLUpdateInfo,
  Comments,
  DisplayVersion,
  InstallDate
};

const int KeysCount = ARRAYSIZE(HelpTopics);
struct RegKeyPath {
  HKEY Root;
  const TCHAR* Path;
} UninstKeys[] = {
  { HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
  { HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
};
int nCount, nRealCount;
FarList FL;
FarListItem* FLI = NULL;
int ListSize;
HANDLE hStdout;

struct Options
{
  int WhereWork; //<- TechInfo
  int EnterFunction; //<- TechInfo
  int UseElevation; //<- TechInfo
} Opt;

struct KeyInfo
{
  TCHAR Keys[KeysCount][MAX_PATH];
#ifdef FARAPI18
  TCHAR ListItem[MAX_PATH];
#endif
  bool Avail[KeysCount];
  RegKeyPath RegKey;
  FILETIME RegTime;
  REGSAM RegView;
  TCHAR SubKeyName[MAX_PATH];
  bool WindowsInstaller;
  bool Hidden;
  bool NoModify, NoRepair;
  bool CanModify, CanRepair;
} *p = NULL;

bool ValidGuid(const TCHAR* guid) {
  const unsigned c_max_guid_len = 38;
  wchar_t buf[c_max_guid_len + 1];
  ZeroMemory(buf, sizeof(buf));
  unsigned l = lstrlen(guid);
  for (unsigned i = 0; (i < c_max_guid_len) && (i < l); i++) buf[i] = guid[i];
  IID iid;
  return IIDFromString(buf, &iid) == S_OK;
}

//чтение реестра
bool FillReg(KeyInfo& key, TCHAR* Buf, RegKeyPath& RegKey, REGSAM RegView)
{
  HKEY userKey;
  DWORD regType;
  TCHAR fullN[MAX_PATH*2], *pszNamePtr;
  LONG ExitCode;
  DWORD bufSize, dwTest;

  memset(&key, 0, sizeof(key));

  key.RegKey = RegKey;
  key.RegView = RegView;
  StringCchCopy(key.SubKeyName,ARRAYSIZE(key.SubKeyName),Buf);
  StringCchCopy(fullN,ARRAYSIZE(fullN),key.RegKey.Path);
  StringCchCat(fullN,ARRAYSIZE(fullN),_T("\\"));
  pszNamePtr = fullN + _tcslen(fullN);
  StringCchCat(fullN,ARRAYSIZE(fullN),key.SubKeyName);
  if (RegOpenKeyEx(key.RegKey.Root, fullN, 0, KEY_READ | RegView, &userKey) != ERROR_SUCCESS)
    return FALSE;
  // "InstallWIX_{GUID}"
  if (memcmp(key.SubKeyName, _T("InstallWIX_{"), 12*sizeof(TCHAR)) == 0
      && ValidGuid(key.SubKeyName+11))
  {
    // Это может быть "ссылка" на гуид продукта
    *pszNamePtr = 0;
    StringCchCat(fullN,ARRAYSIZE(fullN),key.SubKeyName+11);
    HKEY hTestKey;
    if (RegOpenKeyEx(key.RegKey.Root, fullN, 0, KEY_READ | RegView, &hTestKey) == ERROR_SUCCESS)
    {
      key.Hidden = true;
      RegCloseKey(hTestKey);
    }
  }
  key.WindowsInstaller = (RegQueryValueEx(userKey,_T("WindowsInstaller"),0,NULL,NULL,NULL) == ERROR_SUCCESS) && ValidGuid(key.SubKeyName);
  key.NoModify = (RegQueryValueEx(userKey,_T("NoModify"),0,NULL,(LPBYTE)&dwTest,&(bufSize)) == ERROR_SUCCESS) && (dwTest!=0);
  key.NoRepair = (RegQueryValueEx(userKey,_T("NoRepair"),0,NULL,(LPBYTE)&dwTest,&(bufSize)) == ERROR_SUCCESS) && (dwTest!=0);
  TCHAR sKeyTime[64]; int nKeyTimeLen;
  if (RegQueryInfoKey(userKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&key.RegTime) != ERROR_SUCCESS)
  {
    memset(&key.RegTime, 0, sizeof(key.RegTime));
    //key.Keys[InstallDate][0] = 0;
    sKeyTime[0] = 0; nKeyTimeLen = 0;
  }
  else
  {
    SYSTEMTIME st; FILETIME ft;
    FileTimeToLocalFileTime(&key.RegTime, &ft);
    FileTimeToSystemTime(&ft, &st);
    wsprintf(sKeyTime, _T(" / %02u.%02u.%04u %u:%02u:%02u"), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    nKeyTimeLen = lstrlen(sKeyTime) + 1;
  }
  for (int i=0;i<KeysCount;i++)
  {
    bufSize = MAX_PATH * sizeof(TCHAR);
	if (HelpTopics[i][0] == 0)
	{
      ExitCode = ERROR_SUCCESS;
	  if (RegView)
	  {
		  wsprintf(key.Keys[i], _T("%s[%u]\\...\\"),
			  (RegKey.Root == HKEY_LOCAL_MACHINE) ? _T("HKLM") : _T("HKCU"),
			  (RegView == KEY_WOW64_64KEY) ? 64 : 32);
	  }
	  else
	  {
		  wsprintf(key.Keys[i], _T("%s\\...\\"),
			  (RegKey.Root == HKEY_LOCAL_MACHINE) ? _T("HKLM") : _T("HKCU"));
	  }
	  int nRootLen = lstrlen(key.Keys[i]);
      lstrcpyn(key.Keys[i]+nRootLen, key.SubKeyName, ARRAYSIZE(key.Keys[i])-nRootLen);
	}
	else
	{
	  if (i == ModifyPath && key.NoModify)
	    continue;
      if (i == InstallDate) bufSize -= nKeyTimeLen * sizeof(TCHAR);
      ExitCode = RegQueryValueEx(userKey,HelpTopics[i],0,&regType,(LPBYTE)key.Keys[i],&bufSize);
	}
    key.Keys[i][ARRAYSIZE(key.Keys[i]) - 1] = 0;
    if (ExitCode != ERROR_SUCCESS || lstrcmp(key.Keys[i],_T("")) == 0)
    {
      if ((i == UninstallString && !key.WindowsInstaller) || i == DisplayName)
      {
        RegCloseKey(userKey);
        return FALSE;
      }
      if (i == InstallDate && nKeyTimeLen)
	  {
        StringCchCopy(key.Keys[i],ARRAYSIZE(key.Keys[i]),sKeyTime+3);
        key.Avail[i] = TRUE;
	  }
	  else
	  {
        StringCchCopy(key.Keys[i],ARRAYSIZE(key.Keys[i]),_T(""));
        key.Avail[i] = FALSE;
	  }
    }
    else
    {
      key.Avail[i] = TRUE;
      if (i == InstallDate && nKeyTimeLen) StringCchCat(key.Keys[i], ARRAYSIZE(key.Keys[i]), sKeyTime);
	}
  }
  RegCloseKey(userKey);

  if ((key.Keys[ModifyPath][0] == 0) && (key.Keys[UninstallString][0] == 0))
    key.Hidden = true;

  if (key.WindowsInstaller)
  {
    key.CanModify = !key.NoModify;
    key.CanRepair = !key.NoRepair;
  }
  else
  {
    key.CanModify = (key.Keys[ModifyPath][0] != 0);
  }

  return TRUE;
}

#ifdef FARAPI17
#define DM_GETDLGITEMSHORT DM_GETDLGITEM
#endif

LONG_PTR WINAPI EntryDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
  switch(Msg)
  {
    case DN_INITDIALOG:
    {
      FarDialogItem item;
      for (unsigned id = 0; Info.SendDlgMessage(hDlg, DM_GETDLGITEMSHORT, id, reinterpret_cast<LONG_PTR>(&item)); id++)
      {
        if (item.Type == DI_EDIT)
          Info.SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, id, 0);
      }
    }
    break;
    case DN_KEY:
    {
      if ((Param2 == KEY_PGUP) || (Param2 == KEY_PGDN))
      {
        TCHAR sMacro[32];
        if (Param2 == KEY_PGUP)
          lstrcpy(sMacro, _T("Esc Up F3"));
        else
          lstrcpy(sMacro, _T("Esc Down F3"));

        ActlKeyMacro m = {MCMD_POSTMACROSTRING};
        m.Param.PlainText.SequenceText = sMacro;
        m.Param.PlainText.Flags = KSFLAGS_DISABLEOUTPUT;
        Info.AdvControl(Info.ModuleNumber, ACTL_KEYMACRO, &m);
        
        return TRUE;
      }
    }
    break;
  }
  return Info.DefDlgProc(hDlg,Msg,Param1,Param2);
}

//заполнение пункта диалога
void FillDialog(FarDialogItem & DialogItem, int Type, int X1, int Y1, int X2, int Y2,
                int Flags, int nData)
{
  const TCHAR* s = nData != -1 ? GetMsg(nData) : _T("");
#ifdef FARAPI17
  lstrcpy(DialogItem.Data, s);
#endif
#ifdef FARAPI18
  DialogItem.PtrData = s;
#endif

  DialogItem.X1 = X1;
  DialogItem.X2 = X2;
  DialogItem.Y1 = Y1;
  DialogItem.Y2 = Y2;

  DialogItem.Flags = Flags;
  DialogItem.Type = Type;
  DialogItem.Selected = 0;
  DialogItem.DefaultButton = 0;
  DialogItem.Focus = 0;

  if (Type == DI_BUTTON)
  {
    DialogItem.DefaultButton = 1;
    DialogItem.Focus = 1;
  }
}

void DisplayEntry(int Sel)
{
  unsigned sx = 70;

  unsigned max_len = 0;
  unsigned cnt = 0;
  for (int i=0;i<KeysCount;i++) {
    if (p[Sel].Avail[i]) {
      unsigned len = lstrlen(p[Sel].Keys[i]);
      if (len + 3 > sx) sx = len + 3;
      cnt++;
    }
  }

  unsigned con_sx = 80;
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (con != INVALID_HANDLE_VALUE) {
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    if (GetConsoleScreenBufferInfo(con, &con_info)) {
      con_sx = con_info.srWindow.Right - con_info.srWindow.Left + 1;
    }
  }

  if (sx + 10 > con_sx) sx = con_sx - 10;
  unsigned sy = cnt * 2;

  unsigned di_cnt = cnt * 2 + 2;
  FarDialogItem* DialogItems = new FarDialogItem[di_cnt];
  unsigned y = 2;
  unsigned idx = 1;
  for (int i=0;i<KeysCount;i++) {
    if (p[Sel].Avail[i]) {
      FillDialog(DialogItems[idx], DI_TEXT, 5, y, 0, y, 0, MName + i);
      idx++;
      y++;
      FillDialog(DialogItems[idx], DI_EDIT, 5, y, sx + 2, y, DIF_READONLY, -1);
      y++;
#ifdef FARAPI17
      if (lstrlen(p[Sel].Keys[i]) < ARRAYSIZE(DialogItems[idx].Data)) lstrcpy(DialogItems[idx].Data, p[Sel].Keys[i]);
      else lstrcpyn(DialogItems[idx].Data, p[Sel].Keys[i], ARRAYSIZE(DialogItems[idx].Data));
      if (i != DisplayName && i != UninstallString) //DisplayName, UninstallString у нас и так в кодировке OEM
        CharToOem(DialogItems[idx].Data, DialogItems[idx].Data); //Переводим в OEM кодировку
#endif
#ifdef FARAPI18
      DialogItems[idx].PtrData = p[Sel].Keys[i];
#endif
      idx++;
    }
  }
  FillDialog(DialogItems[0], DI_DOUBLEBOX, 3, 1, sx + 4, sy + 2, 0, p[Sel].WindowsInstaller ? MUninstallEntryMSI : MUninstallEntry);
  
#ifdef FARAPI17
  Info.DialogEx(Info.ModuleNumber, -1, -1, sx + 8, sy + 4, _T("UninstallEntry"), DialogItems, di_cnt, 0, 0, EntryDlgProc, 0);
#endif
#ifdef FARAPI18
  HANDLE h_dlg = Info.DialogInit(Info.ModuleNumber, -1, -1, sx + 8, sy + 4, _T("UninstallEntry"), DialogItems, di_cnt, 0, 0, EntryDlgProc, 0);
  if (h_dlg != INVALID_HANDLE_VALUE) {
    Info.DialogRun(h_dlg);
    Info.DialogFree(h_dlg);
  }
#endif
  delete[] DialogItems;
}

BOOL IsUserAdmin()
{
	//OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
	//GetVersionEx(&osv);
	//// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
	//if (osv.dwMajorVersion < 6)
	//	return FALSE;
	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;

	b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup); 
	if(b) 
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) 
		{
			b = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}
	return(b);
}

BOOL CheckForEsc()
{
	BOOL bEscaped = FALSE;
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
					if (InputRec[i].EventType == KEY_EVENT && InputRec[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
					{
						while (((i+1) < ReadCnt)
							&& InputRec[i+1].EventType == KEY_EVENT
							&& InputRec[i+1].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
								i++;
						bEscaped = TRUE;
						ReadConsoleInput(Console,InputRec,i+1,&ReadCnt);
						break;
					}
					i++;
				}
			}
			free(InputRec);
		}
	}

	return bEscaped;
}

bool ExecuteEntry(int Sel, bool WaitEnd)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  HANDLE hScreen; //for SaveScreen/RestoreScreen

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  TCHAR cmd_line[MAX_PATH];
  if (p[Sel].WindowsInstaller)
  {
    StringCchCopy(cmd_line, ARRAYSIZE(cmd_line), _T("msiexec /x"));
    StringCchCat(cmd_line, ARRAYSIZE(cmd_line), p[Sel].SubKeyName);
  }
  else StringCchCopy(cmd_line, ARRAYSIZE(cmd_line), p[Sel].Keys[UninstallString]);

  hScreen = Info.SaveScreen(0,0,-1,-1); //Это необходимо сделать, т.к. после запущенных программ нужно обновить окно ФАРа
  
  BOOL ifCreate = FALSE, bElevationFailed = false;

  // MSI сам выполнит повышение прав когда потребуется
  if (!p[Sel].WindowsInstaller && Opt.UseElevation)
  {
    // Required elevation
    SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = _T("runas");
    sei.lpFile = cmd_line;
    sei.nShow = SW_SHOWNORMAL;
    ifCreate = ShellExecuteEx(&sei);
    if (ifCreate)
      pi.hProcess = sei.hProcess;
    else
      bElevationFailed = true;
  }

  // Если Elevation не запускался
  if (!ifCreate)
  {
    ifCreate = CreateProcess   // Start the child process.
    (
      NULL,                         // No module name (use command line).
      cmd_line,                     // Command line.
      NULL,                         // Process handle not inheritable.
      NULL,                         // Thread handle not inheritable.
      FALSE,                        // Set handle inheritance to FALSE.
      0,                            // No creation flags.
      NULL,                         // Use parent's environment block.
      NULL,                         // Use parent's starting directory.
      &si,                          // Pointer to STARTUPINFO structure.
      &pi                           // Pointer to PROCESS_INFORMATION structure.
    );
  }

  if (!ifCreate) //not Create
  {
    DWORD dwErr = GetLastError();
    if ((dwErr == 0x2E4) && !bElevationFailed)
    {
      // Required elevation
      SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
      sei.fMask = SEE_MASK_NOCLOSEPROCESS;
      sei.lpVerb = _T("runas");
      sei.lpFile = cmd_line;
      sei.nShow = SW_SHOWNORMAL;
      ifCreate = ShellExecuteEx(&sei);
      if (ifCreate)
        pi.hProcess = sei.hProcess;
      else
        dwErr = GetLastError();
    }
    if (!ifCreate) //not Create
    {
      TCHAR szErrCode[32];
      const TCHAR *pszErrInfo = szErrCode;
      if (dwErr == 0x000004C7)
        pszErrInfo = GetMsg(MCancelledByUser);
      else
        wsprintf(szErrCode, _T("ErrorCode=0x%08X"), dwErr);
      if (hScreen)
        Info.RestoreScreen(hScreen);
      DrawMessage(FMSG_WARNING, 1, "%s",GetMsg(MPlugIn),GetMsg(MRunProgErr),cmd_line,pszErrInfo,GetMsg(MOK),NULL);
      return FALSE;
    }
  }

  TCHAR SaveTitle[MAX_PATH];
  GetConsoleTitle(SaveTitle,ARRAYSIZE(SaveTitle));
  SaveTitle[ARRAYSIZE(SaveTitle) - 1] = 0;
  SetConsoleTitle(cmd_line);

  if (pi.hProcess)
  {
    if (WaitEnd) // Wait until child process exits.
    {
      DrawMessage(0, 0, "%s", GetMsg(MPlugIn), GetMsg(MWaitingCompletion), cmd_line,NULL);
      while (true)
      {
        if (WaitForSingleObject(pi.hProcess, 500) == WAIT_OBJECT_0)
          break;
        if (CheckForEsc())
          break;
      }
    }
    // Close process and thread handles.
    CloseHandle(pi.hProcess);
  }
  if (pi.hThread)
    CloseHandle(pi.hThread);

  SetConsoleTitle(SaveTitle);

  if (hScreen)
  {
    Info.RestoreScreen(NULL);
    Info.RestoreScreen(hScreen);
  }
  return TRUE;
}

typedef WINADVAPI LSTATUS (APIENTRY *FRegDeleteKeyExA)(__in HKEY hKey, __in LPCSTR lpSubKey, __in REGSAM samDesired, __reserved DWORD Reserved);
typedef WINADVAPI LSTATUS (APIENTRY *FRegDeleteKeyExW)(__in HKEY hKey, __in LPCWSTR lpSubKey, __in REGSAM samDesired, __reserved DWORD Reserved);

bool DeleteEntry(int Sel)
{
  HMODULE h_mod = LoadLibrary(_T("advapi32.dll"));
  FRegDeleteKeyExA RegDeleteKeyExA = reinterpret_cast<FRegDeleteKeyExA>(GetProcAddress(h_mod, "RegDeleteKeyExA"));
  FRegDeleteKeyExW RegDeleteKeyExW = reinterpret_cast<FRegDeleteKeyExW>(GetProcAddress(h_mod, "RegDeleteKeyExW"));
  FreeLibrary(h_mod);

  HKEY userKey;
  LONG ret = RegOpenKeyEx(p[Sel].RegKey.Root, p[Sel].RegKey.Path, 0, DELETE | p[Sel].RegView, &userKey);
  if (ret != ERROR_SUCCESS) return false;
  if (RegDeleteKeyEx)
    ret = RegDeleteKeyEx(userKey, p[Sel].SubKeyName, p[Sel].RegView, 0);
  else
    ret = RegDeleteKey(userKey, p[Sel].SubKeyName);
  RegCloseKey(userKey);
  if (ret != ERROR_SUCCESS) return false;
  return true;
}

//сравнить строки
int __cdecl CompareEntries(const void* item1, const void* item2)
{
  return FSF.LStricmp(reinterpret_cast<const KeyInfo*>(item1)->Keys[DisplayName], reinterpret_cast<const KeyInfo*>(item2)->Keys[DisplayName]);
}

#define JUMPREALLOC 50
void EnumKeys(RegKeyPath& RegKey, REGSAM RegView = 0) {
  HKEY hKey;
  if (RegOpenKeyEx(RegKey.Root, RegKey.Path, 0, KEY_READ | RegView, &hKey) != ERROR_SUCCESS)
    return;
  DWORD cSubKeys;
  if (RegQueryInfoKey(hKey,NULL,NULL,NULL,&cSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
    return;

  TCHAR Buf[MAX_PATH];
  for (DWORD fEnumIndex=0; fEnumIndex<cSubKeys; fEnumIndex++)
  {
    DWORD bufSize = ARRAYSIZE(Buf);
    FILETIME ftLastWrite;
    if (RegEnumKeyEx(hKey, fEnumIndex, Buf, &bufSize, NULL, NULL, NULL, &ftLastWrite) != ERROR_SUCCESS)
      return;
    if (nCount >= nRealCount)
    {
      nRealCount += JUMPREALLOC;
      p = (KeyInfo *) realloc(p, sizeof(KeyInfo) * nRealCount);
    }
    if (FillReg(p[nCount], Buf, RegKey, RegView))
    {
#ifdef FARAPI17
      CharToOem(p[nCount].Keys[DisplayName], p[nCount].Keys[DisplayName]);
      CharToOem(p[nCount].Keys[UninstallString], p[nCount].Keys[UninstallString]);
#endif
      nCount++;
    }
  }
  RegCloseKey(hKey);
}
#undef JUMPREALLOC

typedef WINBASEAPI VOID (WINAPI *FGetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);

#define EMPTYSTR _T("  ")
//Обновление информации
void UpDateInfo(void)
{
  HMODULE h_mod = LoadLibrary(_T("kernel32.dll"));
  FGetNativeSystemInfo GetNativeSystemInfo = reinterpret_cast<FGetNativeSystemInfo>(GetProcAddress(h_mod, "GetNativeSystemInfo"));
  FreeLibrary(h_mod);
  bool is_os_x64 = false;
  if (GetNativeSystemInfo)
  {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    is_os_x64 = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
  }

  nCount = nRealCount = 0;
  for (int i=0;i<ARRAYSIZE(UninstKeys);i++)
  {
    if (is_os_x64 && (UninstKeys[i].Root == HKEY_LOCAL_MACHINE))
    {
      EnumKeys(UninstKeys[i], KEY_WOW64_64KEY);
      EnumKeys(UninstKeys[i], KEY_WOW64_32KEY);
    }
    else
    {
      EnumKeys(UninstKeys[i]);
    }
  }
  p = (KeyInfo *) realloc(p, sizeof(KeyInfo) * nCount);
  FSF.qsort(p, nCount, sizeof(KeyInfo), CompareEntries);

  FLI = (FarListItem *) realloc(FLI, sizeof(FarListItem) * nCount);
  ZeroMemory(FLI, sizeof(FarListItem) * nCount);

  FL.ItemsNumber = nCount;
  FL.Items = FLI;

  const TCHAR* sx86 = GetMsg(MListHKLMx86);
  const TCHAR* sx64 = GetMsg(MListHKLMx64);
  const TCHAR* sHKLM = GetMsg(MListHKLM);
  const TCHAR* sHKCU = GetMsg(MListHKCU);

  //const TCHAR* sMSI = GetMsg(MListMSI);
  //size_t nLen = _tcslen(sMSI);
  //TCHAR* sMSI0 = (TCHAR*)malloc((nLen+1)*sizeof(TCHAR));
  //for (size_t n=0; n<nLen; n++) sMSI0[n] = _T(' ');
  //sMSI0[nLen] = 0;
  
  TCHAR FirstChar[4];
  FirstChar[0] = _T('&');
  FirstChar[1] = 0;
  FirstChar[2] = _T(' ');
  FirstChar[3] = 0;
  for (int i=0;i<nCount;i++)
  {
#ifdef FARAPI17
    size_t MaxSize = ARRAYSIZE(FLI[i].Text);
#endif
#ifdef FARAPI18
    size_t MaxSize = ARRAYSIZE(p[i].ListItem);
    FLI[i].Text = p[i].ListItem;
#endif
    TCHAR* text = const_cast<TCHAR*>(FLI[i].Text);

    if (FirstChar[1] != FSF.LUpper(p[i].Keys[DisplayName][0]))
    {
      FirstChar[1] = FSF.LUpper(p[i].Keys[DisplayName][0]);
      StringCchCopy(text, MaxSize, FirstChar);
    }
    else
      StringCchCopy(text, MaxSize, EMPTYSTR);
      
    if (p[i].RegKey.Root == HKEY_LOCAL_MACHINE)
    {
      if (is_os_x64)
        StringCchCat(text, MaxSize, (p[i].RegView == KEY_WOW64_64KEY) ? sx64 : sx86);
      else
        StringCchCat(text, MaxSize, sHKLM);
    }
    else
      StringCchCat(text, MaxSize, sHKCU);
    
    StringCchCat(text, MaxSize, _T(" "));
    StringCchCat(text, MaxSize, (p[i].WindowsInstaller) ? _T("W") : _T(" "));
    StringCchCat(text, MaxSize, (p[i].CanModify) ? _T("M") : _T(" "));
    StringCchCat(text, MaxSize, (p[i].CanRepair) ? _T("R") : _T(" "));
    StringCchCat(text, MaxSize, (p[i].Hidden) ? _T("-") : _T(" "));
    StringCchCat(text, MaxSize, _T(" "));

    //if ((p[i].Keys[ModifyPath][0] == 0) && (p[i].Keys[UninstallString][0] == 0))
    //if (p[i].Hidden)
    //  StringCchCat(text, MaxSize, _T(" - "));
    //else
    //  StringCchCat(text, MaxSize, _T("   "));

    size_t nCurLen = _tcslen(text);
    //StringCchCat(text, MaxSize, p[i].Keys[DisplayName]);
    StringCchCopyN(text+nCurLen, MaxSize-nCurLen, p[i].Keys[DisplayName], MaxSize-nCurLen-1);
    text[MaxSize-1] = 0;
  }

  ListSize = nCount;
}
#undef EMPTYSTR

//-------------------------------------------------------------------

void ReadRegistry()
{
  //TechInfo
  if (GetRegKey(HKCU,_T(""),_T("WhereWork"),Opt.WhereWork,3))
    if ((Opt.WhereWork<0) || (Opt.WhereWork>3))
      Opt.WhereWork=3;
  SetRegKey(HKCU,_T(""),_T("WhereWork"),(DWORD) Opt.WhereWork);

  if (GetRegKey(HKCU,_T(""),_T("EnterFunction"),Opt.EnterFunction,0))
    if ((Opt.EnterFunction<0) || (Opt.EnterFunction>1))
      Opt.EnterFunction=0;
  SetRegKey(HKCU,_T(""),_T("EnterFunction"),(DWORD) Opt.EnterFunction);

  if (GetRegKey(HKCU,_T(""),_T("UseElevation"),Opt.UseElevation,1))
    if ((Opt.UseElevation<0) || (Opt.UseElevation>1))
      Opt.UseElevation=1;
  SetRegKey(HKCU,_T(""),_T("UseElevation"),(DWORD) Opt.UseElevation);
}
