//DEBUG:
//F:\VCProject\FarPlugin\#FAR180\far.exe
//C:\Temp\1000-very-long-path\F1..567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\F2..567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890

#define PRINT_FAR_VERSION
//#define SHOW_INVALID_OP_ON_DIR

#include "header.h"
#include "../../common/DlgBuilder.hpp"
#include "RE_CmdLine.h"
#include "version.h"
#include "RE_Guids.h"

HINSTANCE g_hInstance = NULL;
struct PluginStartupInfo psi;
struct FarStandardFunctions FSF;
HANDLE ghHeap = NULL;
//DWORD gnFarVersion = 0;
const void* PluginDialogBuilder::gpDialogBuilderPtr = NULL;
FarVersionInfo gFarVersion = {0};

//#if defined(__GNUC__)
#ifndef CRTSTARTUP
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			g_hInstance = (HINSTANCE)hModule;
			break;
	}
	return TRUE;
}
#endif
//#endif

#ifdef _UNICODE
	#if FARMANAGERVERSION_BUILD<1900
		#define OPEN_ANALYSE 9
		struct AnalyseInfo
		{
			int StructSize;
			const wchar_t *FileName;
			const unsigned char *Buffer;
			DWORD BufferSize;
			DWORD OpMode;
		};
		struct AnalyseInfo *gpLastAnalyse = NULL;
	#endif
#endif

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  //BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  //void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo);
  //void WINAPI ExitFARW(void);
  //HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item);
  //int WINAPI GetMinFarVersionW(void);
  //void WINAPI GetPluginInfoW(struct PluginInfo *pi);
  //int WINAPI GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
  //void WINAPI FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
  //void WINAPI GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info);
  //void WINAPI ClosePluginW(HANDLE hPlugin);
  //int WINAPI SetDirectoryW ( HANDLE hPlugin, LPCTSTR Dir, int OpMode );
  //int WINAPI ProcessEventW(HANDLE hPlugin,int Event,void *Param);
  //#ifdef _UNICODE
  //int WINAPI ProcessSynchroEventW(int Event, void *Param);
  //int WINAPI ProcessEditorEventW(int Event, void *Param);
  //int WINAPI AnalyseW(const AnalyseInfo *pData);
  //#endif
  //#if FAR_UNICODE>=1900
  //int WINAPI ProcessPanelInputW(HANDLE hPlugin,const struct ProcessPanelInputInfo *Info);
  //#else
  //int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState);
  //#endif
#ifdef __cplusplus
};
#endif

//BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
//{
//  return DllMain(hDll, dwReason,lpReserved);
//}
#endif

#ifdef CRTSTARTUP
extern "C"{
BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return TRUE;
};
};
#endif


#if FAR_UNICODE>=1906

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	//memset(Info, 0, sizeof(GlobalInfo));
	_ASSERTE(Info->StructSize == sizeof(GlobalInfo));
	
	Info->MinFarVersion = /*FARMANAGERVERSION*/
		MAKEFARVERSION(
			FARMANAGERVERSION_MAJOR,
			FARMANAGERVERSION_MINOR,
			FARMANAGERVERSION_REVISION,
			FARMANAGERVERSION_BUILD,
			FARMANAGERVERSION_STAGE);
	
	// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,FARMANAGERVERSION_BUILD,VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"RegEditor";
	Info->Description = L"Registry Editor plugin";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#endif

int WINAPI GetMinFarVersionW(void)
{
	int nVer =
	#ifdef _UNICODE
		#if FAR_UNICODE>=1906
			// Чтобы в Far2 не загружался
			MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD)
		#else
			// 1607 - только там поправлено падение фара при FCTL_SETNUMERICSORT
			MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,max(FARMANAGERVERSION_BUILD,1607))
		#endif
	#else
	FARMANAGERVERSION
	#endif
	;
	return nVer;
}


void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
	if (!cfg)
	{
		_ASSERTE(cfg!=NULL);
		return;
	}
	
	MCHKHEAP;
	
	//memset(pi, 0, sizeof(struct PluginInfo));
	//pi->StructSize = sizeof(struct PluginInfo);
	#if FAR_UNICODE>=1906
	_ASSERTE(pi->StructSize >= sizeof(struct PluginInfo));
	#else
	pi->StructSize = sizeof(struct PluginInfo);
	#endif

	pi->Flags = cfg->bAddToPluginsMenu ? 0 : PF_DISABLEPANELS;

	static TCHAR szMenu[MAX_PATH];
	lstrcpy(szMenu, GetMsg(REPluginName)); // "RegEditor"
    static TCHAR *pszMenu[1];
    pszMenu[0] = szMenu;
	
	#if FAR_UNICODE>=1906
		if (cfg->bAddToDisksMenu)
		{
			pi->DiskMenu.Guids = &guid_DiskMenu;
			pi->DiskMenu.Strings = pszMenu;
			pi->DiskMenu.Count = 1;
		}
	#else
	    static int nDiskNumber[1];
	    nDiskNumber[0] = cfg->cDiskMenuHotkey[0] - _T('0');
		pi->DiskMenuStringsNumber = cfg->bAddToDisksMenu ? 1 : 0;
		pi->DiskMenuStrings = cfg->bAddToDisksMenu ? pszMenu : NULL;
	#endif
	
#ifdef _UNICODE
	#if FAR_UNICODE>=1906
		// тут их уже нет
	#else
		// Пока оставим, т.к. плагин может работать и в "старых" сборках 2.0
		if (gFarVersion.Build < 1692)
			pi->DiskMenuNumbers = cfg->bAddToDisksMenu ? nDiskNumber : NULL;
		else
			pi->DiskMenuNumbers = NULL;
	#endif
#else
	pi->DiskMenuNumbers = cfg->bAddToDisksMenu ? nDiskNumber : NULL;
#endif

	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_PluginMenu;
		pi->PluginMenu.Strings = pszMenu;
		pi->PluginMenu.Count = 1;
	#else
		pi->PluginMenuStrings = pszMenu;
		pi->PluginMenuStringsNumber = 1;
	#endif
	
	#if FAR_UNICODE>=1906
		pi->PluginConfig.Guids = &guid_ConfigMenu;
		pi->PluginConfig.Strings = pszMenu;
		pi->PluginConfig.Count = 1;
	#else
		pi->PluginConfigStrings = pszMenu;
		pi->PluginConfigStringsNumber = 1;
	#endif
	
	pi->CommandPrefix = cfg->sCommandPrefix;
	
	#ifndef FAR_UNICODE
	pi->Reserved = REGEDIT_MAGIC; //'RgEd';
	#endif

	MCHKHEAP;
}

void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo)
{
	_ASSERTE(MAX_REGKEY_NAME <= MAX_PATH);

#ifdef _DEBUG
	BOOL bDump = FALSE;
#endif

	if (ghHeap == NULL)
	{
		#ifdef MVALIDATE_POINTERS
			ghHeap = HeapCreate(0, 1<<20, 0);
		#else
			ghHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 1<<20, 0);
		#endif

		#ifdef _DEBUG
		if (bDump) xf_dump();
		#endif
	}

	MCHKHEAP;
	g_hInstance = GetModuleHandle(aInfo->ModuleName);
	::psi = *aInfo;
	::FSF = *aInfo->FSF;
	::psi.FSF = &::FSF;

	#if FAR_UNICODE>=1906
		VersionInfo vi = {0};
		psi.AdvControl(PluginNumber, ACTL_GETFARMANAGERVERSION, 0, &vi);
		gFarVersion.Major = vi.Major;
		gFarVersion.Minor = vi.Minor;
		gFarVersion.Revision = vi.Revision;
		gFarVersion.Build = vi.Build;
		gFarVersion.Stage = vi.Stage;
	#else
		DWORD nVersion = 0;
		psi.AdvControl(PluginNumber, ACTL_GETFARVERSION, &nVersion);
		gFarVersion.Major = HIBYTE(LOWORD(nVersion));
		gFarVersion.Minor = LOBYTE(LOWORD(nVersion));
		gFarVersion.Build = HIWORD(nVersion);
	#endif
	_ASSERTE(gFarVersion.Major!=0);
	/*
	HIWORD:         = номер билда   (FAR 1.70.387 = 0x0183)
	LOWORD:  HIBYTE = major version (FAR 1.70.387 = 0x01)
			 LOBYTE = minor version (FAR 1.70.387 = 0x46)
	*/

	// Создаст (если надо) cfg и загрузит настройки
#if FAR_UNICODE>=1900
	RegConfig::Init(aInfo->ModuleName);
#else
	RegConfig::Init(psi.RootKey, aInfo->ModuleName);
#endif
	// Для регистрации плагинов
	if (!gpPluginList)
	{
		gpPluginList = new REPluginList();
	}
	MCHKHEAP;

	#ifdef _DEBUG
	if (bDump) xf_dump();
	#endif
}

#if FAR_UNICODE>=2041
void   WINAPI ExitFARW(const struct ExitInfo *Info)
#else
void   WINAPI ExitFARW(void)
#endif
{
	#if defined(_UNICODE) && (FARMANAGERVERSION_BUILD<2462)
	SafeFree(gpLastAnalyse);
	#endif

	MCHKHEAP;
	SafeDelete(cfg);
	SafeDelete(gpPluginList);
	MCHKHEAP;

	if (ghHeap) {
		#ifdef TRACK_MEMORY_ALLOCATIONS
		xf_dump();
		#endif

		HANDLE h = ghHeap;
		ghHeap = NULL;
		HeapDestroy(h);		
	}
}


FARINT WINAPI SetDirectoryW (
#if FAR_UNICODE>=1900
	const SetDirectoryInfo *Info
#else
	HANDLE hPlugin, LPCTSTR Dir, int OpMode 
#endif
	)
{
#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel; LPCTSTR Dir = Info->Dir; u64 _OpMode = Info->OpMode;
#else
	DWORD _OpMode = OpMode;
#endif

	CPluginActivator a(hPlugin,_OpMode);
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	if (!pPlugin)
	{
		_ASSERTE(pPlugin);
		InvalidOp();
		return FALSE;
	}
	// Сразу отсечем некорректные аргументы
	if (!Dir || !*Dir)
	{
		_ASSERTE(Dir && *Dir);
		InvalidOp();
		return FALSE;
	}

	MCHKHEAP;
	
	BOOL lbSilence = (_OpMode & (OPM_FIND|OPM_SILENT)) != 0;
	int  nRc = 0;

	// Закрытие плагина (нужно только в сетевом режиме - в других Фар закрывает плагин сам)
	if (pPlugin->m_Key.eType == RE_WINAPI && pPlugin->mb_RemoteMode)
	{
		if ((Dir[0] ==_T('\\') && Dir[1] == 0) || (Dir[0] ==_T('.') && Dir[1] ==_T('.') && Dir[2] == 0))
		{
			if (pPlugin->m_Key.mh_Root == NULL && !(pPlugin->m_Key.mpsz_Key && *pPlugin->m_Key.mpsz_Key))
			{
				#if FAR_UNICODE>=1906
				psiControl(hPlugin, FCTL_CLOSEPANEL, 0, NULL);
				#else
				psiControl(hPlugin, FCTL_CLOSEPLUGIN, F757NA 0);
				#endif
				return TRUE;
			}
		}
	}


	// Чтобы обработать "странные" папки, которые нельзя просто отобразить в фар
	// (например "..") нужно проверить, стоит ли курсор на
	if (_tcschr(Dir, _T('\\')) == NULL)
	{
		const PluginPanelItem* ppi = NULL;
		RegItem* p = pPlugin->GetCurrentItem(&ppi);
		if (p && ppi && FILENAMEPTR(*ppi))
		{
			if (lstrcmpi(FILENAMEPTR(*ppi), Dir) == 0)
			{
				p->pFolder->AddRef();
				nRc = pPlugin->SetDirectory(p, lbSilence);
				p->pFolder->Release();
				goto fin;
			}
		}
	}


	// Для удобства - сделаем копию памяти
	bool lbNoDebracket = false;
	wchar_t* pwszDir = NULL;
#ifdef _UNICODE
	pwszDir = lstrdup(Dir);
#else
	pwszDir = lstrdup_w(Dir);
#endif


	//!! переход из TempPanel дает путь без "\"
	//!! юзер может ввести cd HKCU\Software находясь НЕ в корне

	if (*pwszDir != L'\\' && !pPlugin->m_Key.IsEmpty())
	{
		wchar_t* pwszSlash = wcschr(pwszDir, L'\\');
		if (pwszSlash) *pwszSlash = 0;
		HREGKEY hkTest = NULL;

		// Сразу проверим, начало pwszDir реально есть подключом?
		if (pPlugin->SubKeyExists(pwszDir))
		{
			lbNoDebracket = true;
		}
		// Если начала в текущем ключе подключем нет - то проверить, может это HREGKEY имелся в виду?
		else if (StringKeyToHKey(pwszDir, &hkTest))
		{
			// Он, перейти в корень реестра
			pPlugin->SetDirectory(L"\\", TRUE);
		}

		// Вернуть слеш
		if (pwszSlash) *pwszSlash = L'\\';
	}

	if (!lbNoDebracket)
	{
		// Это может быть строка, скопированная из *.reg
		// "[HKEY_CURRENT_USER\Software\Far\Users]"
		DebracketRegPath(pwszDir);
	}

	// Собственно смена (может применить часть пути, если хвост не существует)
	nRc = pPlugin->SetDirectory(pwszDir, lbSilence);
	SafeFree(pwszDir);

	MCHKHEAP;


fin:

	#ifdef SHOW_INVALID_OP_ON_DIR
		if (!nRc && !(gnOpMode & (OPM_FIND|OPM_SILENT))) {
			REPlugin::MessageFmt(REM_SetDirectoryFailed, Dir);
		}
	#endif
	return nRc;
}



#ifdef SHOW_INVALID_OP_ON_DIR
	#define InvalidCmd() REPlugin::MessageFmt(REM_OpenPluginPrefixFailed, (TCHAR*)Item); InvalidOp()
#else
	#define InvalidCmd()
#endif

#if FAR_UNICODE>=1900
// Для удобства у нас используется эта функция, но в экспортах для Far3 ее уже нет
HANDLE WINAPI OpenFilePluginW(const TCHAR *Name,const unsigned char *Data,int DataSize,u64 OpMode);
#endif

// Экспортируется как OpenW для Far3
HANDLE WINAPI OpenPluginW(
#if FAR_UNICODE>=1900
	const OpenInfo *Info
#else
	int OpenFrom,INT_PTR Item
#endif
						  )
{
#if FAR_UNICODE>=1900
	int OpenFrom = Info->OpenFrom;
	INT_PTR Item = Info->Data;
#endif

#ifdef _DEBUG
	BOOL bDump = FALSE;
	if (bDump) xf_dump();
#endif

	#if defined(_UNICODE) && (FARMANAGERVERSION_BUILD>=2572)
	const HANDLE PanelStop = PANEL_STOP;
	const HANDLE PanelNone = NULL;
	#else
	const HANDLE PanelStop = (HANDLE)-2;
	const HANDLE PanelNone = INVALID_HANDLE_VALUE;
	#endif
	

	#ifdef _UNICODE
	// После AnalyseW вызывается OpenPluginW, а не OpenFilePluginW
	if (OpenFrom == OPEN_ANALYSE)
	{
		AnalyseInfo* p;
		
		#if (FARMANAGERVERSION_BUILD<2462)
		p = gpLastAnalyse;
		#else
		OpenAnalyseInfo* pa = (OpenAnalyseInfo*)Info->Data;
		if ((pa == NULL) || (pa->StructSize < sizeof(*pa)))
		{
			InvalidOp();
			return PanelNone;
		}
		p = pa->Info;
		#endif
		
		HANDLE hPlugin = OpenFilePluginW(p->FileName, (const unsigned char *)p->Buffer, p->BufferSize, 0/*OpMode*/);
		
		#if defined(_UNICODE) && (FARMANAGERVERSION_BUILD<2462)
		SafeFree(gpLastAnalyse);
		#endif
		
		if (hPlugin == (HANDLE)-2)
		{
			hPlugin = PanelStop;
		}
		else if (hPlugin == INVALID_HANDLE_VALUE)
		{
			hPlugin = PanelNone;
		}
		return hPlugin;
	}
	#if defined(_UNICODE) && (FARMANAGERVERSION_BUILD<2462)
	SafeFree(gpLastAnalyse);
	#endif
	#endif


	REPlugin* pPlugin = NULL;

	MCHKHEAP;

	pPlugin = new REPlugin();
	if (!pPlugin)
		return PanelNone;
	
	CPluginActivator a((HANDLE)pPlugin,0);
	
	bool lbDirSet = false;
	LPCTSTR pszCommand = NULL;
	LPCTSTR pszHostFile = NULL;

	#ifndef _UNICODE
		#define OPEN_FROM_MASK 0xFF
	#endif

	#if FAR_UNICODE>=3000
	int OpenFromMask = OpenFrom;
	#else
	int OpenFromMask = (OpenFrom & OPEN_FROM_MASK);
	#endif

	if ((OpenFromMask == OPEN_COMMANDLINE) && Item)
	{
		#if FAR_UNICODE>=3000
		pszCommand = ((OpenCommandLineInfo*)Item)->CommandLine;
		#else
		pszCommand = (LPCTSTR)Item;
		#endif
	}
	#if FAR_UNICODE>=1900
	else if ((OpenFromMask == OPEN_SHORTCUT) && Item)
	{
		OpenShortcutInfo* pInfo = (OpenShortcutInfo*)Item;

		if (pInfo->HostFile && *pInfo->HostFile)
		{
			BOOL lbLoadRc = pPlugin->LoadRegFile(pInfo->HostFile, FALSE, FALSE, FALSE);
			if (!lbLoadRc)
			{
				SafeDelete(pPlugin);
				gpActivePlugin = NULL;
				InvalidCmd();
				return PanelNone;
			}
		}

		pszCommand = pInfo->ShortcutData ? pInfo->ShortcutData : L"";
	}
	#endif

	if (pszCommand)
	{
		OpenPluginArg Args;
		MCHKHEAP;
		switch (Args.ParseCommandLine(pszCommand))
		{
		case aBrowseLocal:
			{
				pPlugin->ChDir(L"\\", TRUE);
				if (Args.wsBrowseLocalKey && *Args.wsBrowseLocalKey)
				{
					pPlugin->SetDirectory(Args.wsBrowseLocalKey, FALSE);
					lbDirSet = true;
				}
			} break;
		case aBrowseRemote:
			{
				if (!pPlugin->ConnectRemote(Args.wsBrowseRemoteServer, Args.wsBrowseRemoteLogin, Args.wsBrowseRemotePassword))
				{
					SafeDelete(pPlugin);
					gpActivePlugin = NULL;
					InvalidCmd();
					return PanelNone;
				}
				if (Args.wsBrowseRemoteKey && *Args.wsBrowseRemoteKey)
				{
					pPlugin->SetDirectory(L"\\", FALSE);
					pPlugin->SetDirectory(Args.wsBrowseRemoteKey, FALSE);
				}
				lbDirSet = true;
			} break;
		case aBrowseFileReg:
			{
				//gpProgress = new REProgress(GetMsg(RELoadingRegFileTitle), TRUE); -- создает сам плагин!
				BOOL lbLoadRc = pPlugin->LoadRegFile(Args.wsBrowseFileName, FALSE, FALSE, FALSE);
				//SafeDelete(gpProgress);
				if (!lbLoadRc)
				{
					SafeDelete(pPlugin);
					gpActivePlugin = NULL;
					InvalidCmd();
					return PanelNone;
				}
				lbDirSet = true;
			} break;
		case aBrowseFileHive:
			{
				BOOL lbLoadRc = pPlugin->LoadHiveFile(Args.wsBrowseFileName, FALSE, FALSE);
				if (!lbLoadRc)
				{
					SafeDelete(pPlugin);
					gpActivePlugin = NULL;
					InvalidCmd();
					return PanelNone;
				}
				lbDirSet = true;
			} break;
		case aExportKeysValues:
			{
				// Экспортировать и сразу выйти
				//pPlugin->Export(nExportFormat, ppszExportKeysOrValues, nExportKeysOrValuesCount, pszExportDestFile);
				InvalidOp();
				SafeDelete(pPlugin);
				gpActivePlugin = NULL;
				return PanelNone;
			} break;
		case aImportRegFile:
			{
				// Импортировать и сразу выйти
				if (cfg->bConfirmImport)
				{
					int nConfirm = REPlugin::MessageFmt(REM_ImportConfirm, Args.pszSourceFile, 0, _T("ImportKey"), FMSG_MB_YESNO, 0);
					if (nConfirm != 0)
						return PanelNone;
				}

				MFileReg fileReg(cfg->getWow64on32(), FALSE);
				MRegistryWinApi winReg(cfg->getWow64on32(), FALSE);

				gpProgress = new REProgress(GetMsg(REImportDlgTitle), TRUE);
				LONG hRc = MFileReg::LoadRegFile(Args.pszSourceFile, FALSE, &winReg, FALSE, &fileReg);
				SafeDelete(gpProgress);
				if (hRc == 0)
				{
					if (cfg->bShowImportResult)
						REPlugin::MessageFmt(REM_ImportSuccess, Args.pszSourceFile, 0, NULL, FMSG_MB_OK);
				}
				else
				{
					REPlugin::MessageFmt(REM_ImportFailed, Args.pszSourceFile);
				}

				// В любом случае, панель - не открываем
				SafeDelete(pPlugin);
				gpActivePlugin = NULL;
				return PanelNone;
			} break;
		case aMountHive:
			{
				LONG hRc = MFileHive::GlobalMountHive(Args.pszMountHiveFilePathName, Args.hRootMountKey, Args.pszMountHiveSubKey);
				if (hRc != 0)
					REPlugin::MessageFmt(REM_Mount_Fail, Args.pszUnmountHiveKey, hRc, _T("MountKey"));
				// В любом случае, панель - не открываем
				SafeDelete(pPlugin);
				gpActivePlugin = NULL;
				return PanelNone;
			} break;
		case aUnmountHive:
			{
				if (1 == REPlugin::MessageFmt(REM_Unmount_Confirm, Args.pszUnmountHiveKey, 0, _T("UnmountKey"), FMSG_WARNING, 2))
				{
					LONG hRc = MFileHive::GlobalUnmountHive(Args.hRootUnmountKey, Args.pszUnmountHiveSubKey);
					if (hRc != 0)
						REPlugin::MessageFmt(REM_Unmount_Fail, Args.pszUnmountHiveKey, hRc, _T("UnmountKey"));
				}
				// В любом случае, панель - не открываем
				SafeDelete(pPlugin);
				gpActivePlugin = NULL;
				return PanelNone;
			} break;
		default:
			// Ошибка разбора уже должна быть показана
			SafeDelete(pPlugin);
			gpActivePlugin = NULL;
			InvalidCmd();
			return PanelNone;
		}

		////TODO: Сделать нормальный разбор ком.строки
		//if (pszCommand[0]==_T('f') && pszCommand[1]==_T('i') && pszCommand[2]==_T('l')
		//	&& pszCommand[3]==_T('e') && pszCommand[4]==_T(':'))
		//{
		//	if (!pPlugin->LoadRegFile(pszCommand+5, FALSE)) {
		//		delete pPlugin;
		//		return INVALID_HANDLE_VALUE;
		//	}
		//	lbDirSet = true;
		//	
		//} else {
		//	pPlugin->ChDir(L"\\", TRUE);
		//	if (Item && *(LPCTSTR)Item) {
		//		SetDirectoryW((HANDLE)pPlugin, (LPCTSTR)Item, 0);
		//		lbDirSet = true;
		//	}
		//}
	}

	MCHKHEAP;
	
	if (!lbDirSet && cfg->pszLastRegPath && cfg->pszLastRegPath[0])
	{
		#ifdef _UNICODE
			#if FAR_UNICODE>=1900
				SetDirectoryInfo dir = {sizeof(SetDirectoryInfo),
					(HANDLE)pPlugin,
					cfg->pszLastRegPath,
					0,
					NULL};
				SetDirectoryW(&dir);
			#else
				SetDirectoryW((HANDLE)pPlugin, cfg->pszLastRegPath, 0);
			#endif
		#else
			int nLen = lstrlenW(cfg->pszLastRegPath)+1;
			char* pszA = (char*)malloc(nLen);
			lstrcpy_t(pszA, nLen, cfg->pszLastRegPath);
			SetDirectoryW((HANDLE)pPlugin, pszA, 0);
		#endif
		lbDirSet = true;
	}

	MCHKHEAP;

	return (HANDLE)pPlugin;
}

BOOL IsFileSupported(const TCHAR *Name, const unsigned char *Data, int DataSize, int &nFormat)
{
	if (!Name || !Data || DataSize < 4)
		return FALSE;

	nFormat = -1; // 0 - REG4, 1 - REG5, 2 - HIVE

	if (!DetectFileFormat(Data, DataSize, &nFormat, NULL, NULL))
		return FALSE;
	
	LPCTSTR pszExt = PointToExt(Name);
	// Во избежание ложных попыток открытий файла как Hive
	// требуем, чтобы у файла не было расширения
	if (nFormat == 2 && pszExt && cfg->bBrowseRegHives != 1)
		return FALSE;

	// Проверяем настройки
	if ((nFormat == 0 || nFormat == 1) && cfg->bBrowseRegFiles)
	{
		// Разрешено
	}
	else if (nFormat == 2 && cfg->bBrowseRegHives)
	{
		// Разрешено
	}
	else
	{
		// Запрещено настройками
		return FALSE;
	}
	
	return TRUE;
}

#ifdef FAR_UNICODE
#if (FARMANAGERVERSION_BUILD<2462)
#define AnalyseRcType int
#else
#define AnalyseRcType HANDLE
#endif
AnalyseRcType WINAPI AnalyseW(const struct AnalyseInfo *pData)
{
	int nFormat = -1;

	#if (FARMANAGERVERSION_BUILD<2462)
	SafeFree(gpLastAnalyse);
	#endif

	if (!IsFileSupported(pData->FileName, (const unsigned char *)pData->Buffer, pData->BufferSize, nFormat))
	{
		#if (FARMANAGERVERSION_BUILD>=2572)
		return FALSE;
		#elif (FARMANAGERVERSION_BUILD<2462)
		return FALSE;
		#else
		return INVALID_HANDLE_VALUE;
		#endif
	}

	#if (FARMANAGERVERSION_BUILD<2462)
	// Запомнить
	int nAllSize = (int)sizeof(*gpLastAnalyse) + pData->BufferSize;
	gpLastAnalyse = (struct AnalyseInfo*)malloc(nAllSize);
	*gpLastAnalyse = *pData;
	gpLastAnalyse->Buffer = (void*)(((const unsigned char*)gpLastAnalyse) + sizeof(*gpLastAnalyse));
	memmove((void*)gpLastAnalyse->Buffer, pData->Buffer, pData->BufferSize);
	#endif

	return (AnalyseRcType)TRUE;
}

#if (FARMANAGERVERSION_BUILD>=2462)
void WINAPI CloseAnalyseW(const struct CloseAnalyseInfo *Info)
{
	return;
}
#endif

#endif

// Far3 note: Для удобства у нас используется эта функция, но в экспортах для Far3 ее уже нет
HANDLE WINAPI OpenFilePluginW(const TCHAR *Name,const unsigned char *Data,int DataSize
	#ifdef _UNICODE
		#if FAR_UNICODE>=1900
			,u64 OpMode
		#else
			,int OpMode
		#endif
	#endif
	)
{
	if (!Name || !Data || DataSize < 4)
		return INVALID_HANDLE_VALUE;

	#ifdef _UNICODE
		#if FAR_UNICODE>=1900
			u64 _OpMode = OpMode;
		#else
			DWORD _OpMode = OpMode;
		#endif
	#else
		u64 _OpMode = 0;
	#endif
		
	#ifdef _UNICODE
	// После AnalyseW вызывается OpenPluginW, а не OpenFilePluginW
	//SafeFree(gpLastAnalyse); -- пока нельзя. Он освобождается в OpenPluginW
	#endif

	int nFormat = -1; // 0 - REG4, 1 - REG5, 2 - HIVE
	BOOL lbSilence = FALSE;
	#ifdef _UNICODE
	lbSilence = ((_OpMode & OPM_SILENT) == OPM_SILENT);
	#endif
	
	if (!IsFileSupported(Name, Data, DataSize, nFormat))
		return INVALID_HANDLE_VALUE;



	REPlugin* pPlugin = new REPlugin();
	if (!pPlugin) return INVALID_HANDLE_VALUE;
	CPluginActivator a((HANDLE)pPlugin
		#ifdef _UNICODE
			,_OpMode
		#else
		,0
		#endif
		);
	
	// Сюда приходит полный путь к файлу, поэтому дублируем только в Ansi версии
	#ifdef _UNICODE
	LPCWSTR pszName = Name;
	#else
	wchar_t* pszName = lstrdup_w(Name);
	#endif

	//gnOpMode |= OPM_DISABLE_NONMODAL_EDIT;

	//gpProgress = new REProgress(GetMsg(RELoadingRegFileTitle), TRUE); -- низя, pPlugin->LoadRegFile сам прогресс создает
	//TODO: Сделать abDelayLoading (точнее сделать загрузку файла по требованию, а не при попытке открытия)
	BOOL lbLoadRc = FALSE;
	if (nFormat == 0 || nFormat == 1)
		lbLoadRc = pPlugin->LoadRegFile(pszName, lbSilence, FALSE/*abDelayLoading*/, !lbSilence);
	else
		lbLoadRc = pPlugin->LoadHiveFile(pszName, lbSilence, FALSE/*abDelayLoading*/);
	//SafeDelete(gpProgress);

	gnOpMode = 0;
	
	//WARNING!!!
	//TODO: После добавления AnalyzeW не наколоться с возвратом в FAR чтобы 
	//TODO: он не должен выполнять ShellAssociation

	#ifndef _UNICODE
	SafeFree(pszName);
	#endif
	if (!lbLoadRc)
	{
		SafeDelete(pPlugin);
		gpActivePlugin = NULL;
		// Иначе пойдет запуск RegEdit.exe по shell association
		return (HANDLE)-2;
	}
	
	return (HANDLE)pPlugin;
}

FARINT WINAPI PutFilesW(
#if FAR_UNICODE>=1900
	const struct PutFilesInfo *Info
#else
	HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,
	#ifdef _UNICODE
	const wchar_t *SrcPath,
	#endif
	int OpMode
#endif
	)
{
#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	struct PluginPanelItem *PanelItem = Info->PanelItem;
	int ItemsNumber = Info->ItemsNumber;
	int Move = Info->Move;
	const wchar_t *SrcPath = Info->SrcPath;
	u64 _OpMode = Info->OpMode;
#else
	DWORD _OpMode = OpMode;
#endif

	int liRc = 2; // Ok, фар курсор двигать не будет
	CPluginActivator a(hPlugin,_OpMode);
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	REPlugin* pOpposite = gpPluginList->GetOpposite(pPlugin);
	wchar_t* pszFullPath = NULL;
	wchar_t* pszUnc = NULL;
	HREGKEY hKey = NULL;
	void* ptrData = NULL;
	DWORD nDataSize = 0;
	BOOL lbSilence = (_OpMode & OPM_SILENT) == OPM_SILENT;
	
	if (pOpposite == NULL)
	{
		if (!lbSilence)
		{
			gpProgress = new REProgress(GetMsg(REImportDlgTitle), TRUE, _T(" "));
		}
		// Если с другой стороны - файловая панель
		//REPlugin* pLoader = new REPlugin();
		REGTYPE nDataType = REG_SZ; BOOL bUnicodeStrings = cfg->bCreateUnicodeFiles; BOOL bForAll = FALSE;
		BOOL bForAllReg = FALSE; RegImportStyle nImportStyle = ris_Here;
		for (int i = 0; i < ItemsNumber; i++)
		{
			if (pszFullPath) free(pszFullPath);
			pszFullPath = ExpandPath(FILENAMEPTR(PanelItem[i]));
			if (!pszFullPath)
			{
				InvalidOp();
				liRc = 0;
				break;
			}
			if (pszUnc) free(pszUnc);
			pszUnc = MakeUNCPath(pszFullPath);
			if (!pszUnc)
			{
				pszUnc = pszFullPath; pszFullPath = NULL;
			}
			LPCWSTR pszName = PointToName(pszUnc);

			if (gpProgress)
				gpProgress->SetStep(i, TRUE, pszName);
			
			if (!(PanelItemAttributes(PanelItem[i]) & FILE_ATTRIBUTE_DIRECTORY))
			{
				int nFormat = -1;
				
				if (DetectFileFormat(pszUnc, &nFormat, NULL, NULL))
				{
					if (nFormat == 0 || nFormat == 1)
					{
						//// Просто импорт *.reg файла в реестр
						//BOOL lbLoadRc = pLoader->LoadRegFile(pszUnc, FALSE, TRUE, TRUE);
						//if (lbLoadRc)
						//{
						//	// -2 в случае ошибки или отказа
						//	if (pLoader->ShowRegMenu(true, pPlugin->Worker()) == -2)
						//		break;
						//}
						MFileReg fileReg(pPlugin->getWow64on32(), pPlugin->getVirtualize());
						LONG hRc = MFileReg::LoadRegFile(pszUnc, lbSilence, NULL, FALSE, &fileReg);
						if (hRc == 0)
						{
							if (gpProgress)
								gpProgress->SetStep(i, TRUE, pszName);
						
							// Продолжаем, теперь можно показать юзеру опции импорта, и выполнить импорт в реестр
							wchar_t* pszRootKey = fileReg.GetFirstKeyFilled();
							if (!bForAllReg)
							{
								DWORD nAllowed = fileReg.GetAllowedImportStyles() | ris_Here|ris_ValuesHere|ris_AsRaw;
								if (!pPlugin->ChooseImportStyle(pszName, pszRootKey,
										nAllowed, nImportStyle,
										((i+1) < ItemsNumber) ? &bForAllReg : NULL))
								{
									liRc = -1;
									SafeFree(pszRootKey);
									break;
								}
							}
							
							if (nImportStyle == ris_AsRaw)
							{
								// ниже будет импортирован как одно значение
							}
							else
							{
								RegPath pathFrom; memset(&pathFrom, 0, sizeof(RegPath));
								HREGKEY hkRoot = NULL; LPCWSTR pszSubKey = NULL;
								
								if (pszRootKey && IsRegPath(pszRootKey, &hkRoot, &pszSubKey, FALSE) && pszSubKey)
								{
									if (nImportStyle == ris_Here)
									{
										wchar_t* pszSlash = (wchar_t*)wcsrchr(pszSubKey, L'\\');
										if (pszSlash)
										{
											*pszSlash = NULL;
										}
									}
									else if (nImportStyle == ris_ValuesHere)
									{
										// Кидаются значения из ключа
									}
									else
									{
										pszSubKey = NULL;
									}
									pathFrom.Init(RE_REGFILE, pPlugin->getWow64on32(), pPlugin->getVirtualize(), hkRoot, pszSubKey);
								}
								else
								{
									InvalidOp();
									liRc = -1;
									SafeFree(pszRootKey);
									break;
									pathFrom.Init(RE_REGFILE, pPlugin->getWow64on32(), pPlugin->getVirtualize());
								}
								bool lbFailed = false;
								RegFolder folderFrom; //-- memset(&folderFrom, 0, sizeof(folderFrom));
								folderFrom.bForceRelease = TRUE;
								folderFrom.Init(&pathFrom);
								MRegistryBase* pTarget = pPlugin->Worker();
								HREGKEY hKey = NULL;
								LONG hRc = pPlugin->mp_Items->CreateKey(pTarget, &hKey, KEY_WRITE);
								if (hRc != 0)
								{
									pPlugin->CantOpenKey(&pPlugin->mp_Items->key, TRUE);
									lbFailed = true;
								}
								else
								{
									if (!folderFrom.Transfer(pPlugin, &fileReg, pPlugin->mp_Items, pTarget))
										//LoadKey(pPlugin, &fileReg, eValuesFirst, TRUE, FALSE, FALSE, pTarget, hKey))
										lbFailed = TRUE;
									pTarget->CloseKey(&hKey);
								}
								folderFrom.Release();
								if (lbFailed)
								{
									liRc = -1;
									break;
								}
							}
							SafeFree(pszRootKey);

							// если (nImportStyle == ris_AsRaw), то ниже будет импортирован как одно значение
						}
						else
						{
							REPlugin::MessageFmt(REM_ImportFailed, pszUnc);
							liRc = -1;
							break;
						}
					}
					else
					{
						InvalidOp();
					}
				}
				else
				{
					nImportStyle = ris_AsRaw;
				}
				
				if (nImportStyle == ris_AsRaw)
				{
					// Видимо, это "значение". Спросить юзера, как импортировать будем
					// а возможно это reg-файл, который попросили импортить как одно значение
					if (!bForAll)
					{
						if (!pPlugin->ChooseImportDataType(pszName, &nDataType, &bUnicodeStrings,
									((i+1) < ItemsNumber) ? &bForAll : NULL))
						{
							liRc = -1;
							break;
						}
					}
					MFileTxtReg txt(pPlugin->getWow64on32(), pPlugin->getVirtualize());
					if (MFileTxt::LoadData(pszUnc, &ptrData, &nDataSize) != 0)
					{
						liRc = -1;
						break;
					}
					if (nDataType == REG_SZ || nDataType == REG_EXPAND_SZ || nDataType == REG_MULTI_SZ)
					{
						// MFileTxt::LoadData выделяет дополнительно 3 '\0' байта в конце данных, специально для строк реестра
						if (!bUnicodeStrings && nDataSize)
						{
							// в реестр нужно кидать и заверающий '\0'
							nDataSize += sizeof(char);
							wchar_t* pwsz = (wchar_t*)malloc(nDataSize*2);
							if (!pwsz)
							{
								InvalidOp();
								liRc = -1;
								break;
							}
							if (!MultiByteToWideChar(cfg->nAnsiCodePage, 0, (char*)ptrData, nDataSize, pwsz, nDataSize))
							{
								InvalidOp();
								SafeFree(pwsz);
								liRc = -1;
								break;
							}
							if (0 != pPlugin->ValueDataSet(pPlugin->mp_Items, pszName, (LPBYTE)pwsz, nDataSize*2, nDataType))
							{
								SafeFree(pwsz);
								liRc = -1;
								break;
							}
							SafeFree(pwsz);
						}
						else
						{
							// в реестр нужно кидать и заверающий '\0'
							nDataSize += sizeof(wchar_t);
							if (0 != pPlugin->ValueDataSet(pPlugin->mp_Items, pszName, (LPBYTE)ptrData, nDataSize, nDataType))
							{
								liRc = -1;
								break;
							}
						}
					}
					else
					{
						// Binary, просто записываем "как есть"
						if (0 != pPlugin->ValueDataSet(pPlugin->mp_Items, pszName, (LPBYTE)ptrData, nDataSize, nDataType))
						{
							liRc = -1;
							break;
						}
					}
					SafeFree(ptrData);
				}
			}
			else
			{
				//TODO: Обработать папки как ключи?
				InvalidOp();
				liRc = -1;
				break;
			}
			SafeFree(pszUnc);
			SafeFree(pszFullPath);
			PanelItem[i].Flags &= ~PPIF_SELECTED;
		}
		//delete pLoader;
		if (!lbSilence && gpProgress)
		{
			SafeDelete(gpProgress);
		}
	}
	else
	{
		psi.Message(_PluginNumber(guid_Msg14), FMSG_WARNING|FMSG_MB_OK|FMSG_ALLINONE, NULL, 
			(const TCHAR * const *)_T("RegEditor\nCopying items from panel to panel not supported now, sorry."), 2,0);
		liRc = -1;
	}

	SafeFree(pszFullPath);
	SafeFree(pszUnc);
	SafeFree(ptrData);

	pPlugin->SetForceReload();

	return liRc;
}

// Если функция выполнила свои действия успешно, то верните TRUE. В противном случае - FALSE.
#if FAR_UNICODE>=1900
FARINT WINAPI DeleteFilesW(const DeleteFilesInfo *Info)
{
	CPluginActivator a(Info->hPanel,Info->OpMode);
	// Выполняем удаление, подтверждение запрашивает класс плагина
	REPlugin* pPlugin = (REPlugin*)Info->hPanel;
	pPlugin->CheckItemsLoaded();
	return pPlugin->DeleteItems(Info->PanelItem, Info->ItemsNumber);
}
#else
int WINAPI DeleteFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	CPluginActivator a(hPlugin,(DWORD)OpMode);
	// Выполняем удаление, подтверждение запрашивает класс плагина
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	pPlugin->CheckItemsLoaded();
	return pPlugin->DeleteItems(PanelItem, ItemsNumber);
}
#endif

// В случае успеха возвращаемое значение должно быть равно 1.
// В случае провала возвращается 0.
// Если функция была прервана пользователем, то должно возвращаться -1. 
FARINT WINAPI MakeDirectoryW(
#if FAR_UNICODE>=1900
	MakeDirectoryInfo *Info
#else
	HANDLE hPlugin, 
	#ifdef _UNICODE
	const wchar_t **Name
	#else
	char *Name/*[NM]*/
	#endif
	,int OpMode
#endif
	)
{
#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	const wchar_t **Name = &(Info->Name);
	u64 _OpMode = Info->OpMode;
#else
	DWORD _OpMode = OpMode;
#endif

	CPluginActivator a(hPlugin,_OpMode);
	const TCHAR* pszCreatedName = NULL;
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	pPlugin->CheckItemsLoaded();
	int liRc = -1;
#ifdef _UNICODE
	// Name
	//Указатель на буфер, содержащий имя каталога, передаётся плагином FAR Manager'у. Если флаг OPM_SILENT параметра OpMode не установлен, вы можете позволить пользователю изменить его, но в этом случае Name должен указывать на буфер плагина, содержащий новый каталог. Буфер должны быть валиден после возвращения из функции. 
	liRc = pPlugin->CreateSubkey(*Name, &pszCreatedName, _OpMode);
	if (liRc == 1 && pszCreatedName && *pszCreatedName)
		*Name = pszCreatedName;
#else
	// Name
	//Имя каталога. Если флаг OpMode OPM_SILENT не установлен, вы можете позволить пользователю изменить его, но в этом случае новое имя должно быть скопировано в Name (максимум NM байт).
	liRc = pPlugin->CreateSubkey(Name, &pszCreatedName, _OpMode);
	if (liRc == 1 && pszCreatedName && *pszCreatedName)
		lstrcpyn(Name, pszCreatedName, NM);
#endif
	// В случае успеха возвращаемое значение должно быть равно 1. В случае провала возвращается 0. Если функция была прервана пользователем, то должно возвращаться -1. 
	return liRc;
}

// ******************************
FARINT WINAPI GetFilesW(
#if FAR_UNICODE>=1900
	GetFilesInfo *Info
#else
	HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,
	#ifdef _UNICODE
	const wchar_t **DestPath
	#else
	char *DestPath
	#endif
	,int OpMode
#endif
	)
{
#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	struct PluginPanelItem *PanelItem = Info->PanelItem;
	int ItemsNumber = Info->ItemsNumber;
	int Move = Info->Move;
	const wchar_t **DestPath = &(Info->DestPath);
	u64 _OpMode = Info->OpMode;
#else
	DWORD _OpMode = OpMode;
#endif

	REPlugin* pPlugin = (REPlugin*)hPlugin;
	REPlugin* pOpposite = gpPluginList->GetOpposite(pPlugin);

	pPlugin->CheckItemsLoaded();

	if ((_OpMode & OPM_FIND) == OPM_FIND)
	{
		pPlugin->mb_FindInContents = 1;

		RegItem* pItem = (RegItem*)PanelItemUserData(PanelItem[0]);
		if (pItem && pItem->nMagic == REGEDIT_MAGIC && pItem->ptrData)
		{
			BOOL lbRc = FALSE;
			wchar_t *pwszDestPath = NULL;
			BOOL lbUnicode = FALSE;
			#ifdef _UNICODE
			pwszDestPath = (wchar_t*)*DestPath;
			lbUnicode = TRUE;
			#else
			pwszDestPath = lstrdup_w(DestPath);
			#endif

			u64 nPrev = gnOpMode;
			gnOpMode = _OpMode;

			MFileTxtReg file(pPlugin->getWow64on32(), pPlugin->getVirtualize());
			if (file.FileCreate((LPCWSTR)pwszDestPath, pItem->pszName ? pItem->pszName : REGEDIT_DEFAULTNAME,
					L""/*asExtension*/, lbUnicode, FALSE/*abConfirmOverwrite*/))
			{
				lbRc = file.FileWriteBuffered(pItem->ptrData, pItem->nDataSize);
				file.FileClose();
			}

			gnOpMode = nPrev;

			#ifndef _UNICODE
			SafeFree(pwszDestPath);
			#endif

			return lbRc ? 1 : 0;
		}
	}

	CPluginActivator a(hPlugin,_OpMode);
	BOOL lbRc = FALSE;
	
	//TODO: Если на обеих панелях открыт наш плагин - обойти экспорт в ФС
	
	//TODO: Вернуть измененный путь?
	#ifdef _UNICODE
	lbRc = pPlugin->ExportItems(PanelItem, ItemsNumber, Move, *DestPath, _OpMode);
	#else
	lbRc = pPlugin->ExportItems(PanelItem, ItemsNumber, Move, DestPath, _OpMode);
	#endif
	return lbRc;
}

// *************************
FARINT WINAPI GetFindDataW(
#if FAR_UNICODE>=1900
						GetFindDataInfo *Info
#else
						HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode
#endif
						)
{
#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	struct PluginPanelItem **pPanelItem = &(Info->PanelItem);
	size_t *pItemsNumber = &(Info->ItemsNumber);
	u64 _OpMode = Info->OpMode;
#else
	DWORD _OpMode = OpMode;
#endif

	CPluginActivator a(hPlugin,_OpMode);
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	MCHKHEAP;

	BOOL lbSilence = (_OpMode & (OPM_FIND)) != 0;

	if (pPlugin->mb_ShowRegFileMenu)
	{
		if (lbSilence || cfg->bBrowseRegFiles != 1)
		{
			pPlugin->mb_ShowRegFileMenu = FALSE;
		}
		else
		{
			// Сюда мы попадаем, если юзер нажал Enter (или CtrlPgDn) на *.reg файле
			// Предложить на выбор: войти или импортировать
			MRegistryWinApi winReg(pPlugin->getWow64on32(), pPlugin->getVirtualize());
			if (pPlugin->ShowRegMenu(false, &winReg) < 0)
				return FALSE;
		}
	}

	// Выполнит перечтение ключа только при необходимости!
	if (!pPlugin->LoadItems(lbSilence, _OpMode))
		return FALSE;
		
	if (!pPlugin->mp_Items)
	{
		_ASSERTE(pPlugin->mp_Items!=NULL);
	}
	else
	{
		pPlugin->mp_Items->AddRef();
		*pItemsNumber = pPlugin->mp_Items->mn_ItemCount;
		*pPanelItem = pPlugin->mp_Items->mp_PluginItems;

		if (pPlugin->mb_SortingSelfDisabled)
		{
			if ((!cfg->bUnsortLargeKeys || (cfg->nLargeKeyCount == 0)) ||
				(pPlugin->mp_Items && (pPlugin->mp_Items->mn_ItemCount < cfg->nLargeKeyCount)))
			{
				// Чтобы не зациклиться - сразу сбросим!
				pPlugin->mb_SortingSelfDisabled = false;
				// Far problem: если сейчас дернуть сортировку - будет сразу запущен цикл сортировки,
				// занимающий длительное время для больших ключей.
				// То есть очень длительное время будет занимать ВХОД в подключ HKCR, хотя казалось бы...
				pPlugin->UpdatePanel(false);
				// Вернуть сортировку
				pPlugin->ChangeFarSorting(false);
			}
		}
	}

	MCHKHEAP;

	return TRUE;
}

#if FAR_UNICODE>=1900
void WINAPI FreeFindDataW(const FreeFindDataInfo *Info)
{
	CPluginActivator a(Info->hPanel,OPM_SILENT);
	REPlugin* pPlugin = (REPlugin*)Info->hPanel;
	// Плагин сам разберется, нужно ли освобождать память, или она закеширована
	pPlugin->FreeFindData(Info->PanelItem, Info->ItemsNumber);
	return;
}
#else
void WINAPI FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	CPluginActivator a(hPlugin,OPM_SILENT);
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	// Плагин сам разберется, нужно ли освобождать память, или она закеширована
	pPlugin->FreeFindData(PanelItem, ItemsNumber);
	return;
}
#endif


void WINAPI 
GetOpenPluginInfoW(
		#if FAR_UNICODE>=1900
		struct OpenPanelInfo *Info
		#else
		HANDLE hPlugin,
		struct OpenPluginInfo *Info
		#endif
	)
{
	#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	#endif
	//CPluginActivator a(hPlugin,0); -- иначе все сбивается, если на обеих панелях открыт Reg2
	
	MCHKHEAP;
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	_ASSERTE(pPlugin);
	_ASSERTE(pPlugin->m_Key.mpsz_Dir && pPlugin->m_Key.mpsz_Title);
	pPlugin->CheckItemsLoaded();
#if FAR_UNICODE>=1900
	Info->CurDir = (pPlugin->m_Key.mpsz_Dir && *pPlugin->m_Key.mpsz_Dir) ? pPlugin->m_Key.mpsz_Dir : _T("\\");
#else
	Info->CurDir = pPlugin->m_Key.mpsz_Dir ? pPlugin->m_Key.mpsz_Dir : _T("");
#endif
	//Info->PanelTitle = pPlugin->m_Key.mpsz_Title ? pPlugin->m_Key.mpsz_Title : _T("");
	Info->PanelTitle = pPlugin->GetPanelTitle();
	Info->HostFile = pPlugin->mpsz_HostFile;
#if FAR_UNICODE>=1900
	Info->Flags = OPIF_DISABLESORTGROUPS|OPIF_ADDDOTS|OPIF_SHORTCUT;
#else
	Info->Flags = OPIF_USEFILTER|/*OPIF_USESORTGROUPS|*/OPIF_USEHIGHLIGHTING|OPIF_ADDDOTS;
#endif
	Info->Format = cfg->sCommandPrefix;
#if FAR_UNICODE>=1900
	Info->ShortcutData = Info->CurDir;
#endif
	//Info->StartSortMode = 0;
	// -- перемещено в GetFindDataW
	//if (pPlugin->mb_SortingSelfDisabled)
	//{
	//	if ((!cfg->bUnsortLargeKeys || (cfg->nLargeKeyCount == 0)) ||
	//		(pPlugin->mp_Items && (pPlugin->mp_Items->mn_ItemCount < cfg->nLargeKeyCount)))
	//	{
	//		// Вернуть сортировку
	//		pPlugin->ChangeFarSorting(false);
	//	}
	//}
	MCHKHEAP;

	static TCHAR sNil[2];
#if FAR_UNICODE>=1900
	static KeyBarLabel kbl[32];
	//// = {
	//	{{VK_F1,SHIFT_PRESSED}, GetMsg(REBarShiftF1), GetMsg(REBarShiftF1L)},
	//	//{{VK_F2,SHIFT_PRESSED}, 
	//	//	GetMsg((pPlugin->m_Key.eType == RE_REGFILE) ? REBarShiftF2 : (pPlugin->m_Key.eType == RE_WINAPI) ? REBarShiftF2Virt : sNil),
	//	//	GetMsg((pPlugin->m_Key.eType == RE_REGFILE) ? REBarShiftF2L : (pPlugin->m_Key.eType == RE_WINAPI) ? REBarShiftF2VirtL : sNil),
	//	{{VK_F3,0}, GetMsg(REBarF3), GetMsg(REBarF3L)},
	//	{{VK_F3,LEFT_ALT_PRESSED|SHIFT_PRESSED}, GetMsg(REBarAltShiftF3), GetMsg(REBarAltShiftF3L)},
	//	//{{VK_F3,SHIFT_PRESSED}, GetMsg((pPlugin->m_Key.eType == RE_WINAPI) ? REBarShiftF3 : sNil), GetMsg((pPlugin->m_Key.eType == RE_WINAPI) ? REBarShiftF3L : sNil)},
	//};
	int idx = 0;
	#define ADDLABEL(vk,c,m) \
		{ \
			int MsgIdx = m; \
			kbl[idx].Key.VirtualKeyCode = vk; kbl[idx].Key.ControlKeyState = c; \
			kbl[idx].Text = (MsgIdx<0) ? _T("") : MsgIdx ? GetMsg(MsgIdx) : sNil; \
			kbl[idx].Text = (MsgIdx<0) ? _T("") : MsgIdx ? GetMsg(MsgIdx+1) : sNil; \
			idx++; \
		}
	// Комбинации с F1
	ADDLABEL(VK_F1, SHIFT_PRESSED, REBarShiftF1);
	// Комбинации с F2
	ADDLABEL(VK_F2, SHIFT_PRESSED, (pPlugin->m_Key.eType == RE_REGFILE) ? REBarShiftF2 : (pPlugin->m_Key.eType == RE_WINAPI) ? REBarShiftF2Virt : 0); // сохранить reg
	// Комбинации с F3
	ADDLABEL(VK_F3, 0, REBarF3);
	//kbt.AltTitles[2] = GetMsg(REBarAltF3);
	ADDLABEL(VK_F3, LEFT_ALT_PRESSED|SHIFT_PRESSED, REBarAltShiftF3);
	ADDLABEL(VK_F3, SHIFT_PRESSED, (pPlugin->m_Key.eType == RE_WINAPI) ? REBarShiftF3 : 0); // переключить x32/x64
	ADDLABEL(VK_F3, LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED, REBarCtrlAltF3); // Права...
	// Комбинации с F4
	ADDLABEL(VK_F4, 0, REBarF4);
	ADDLABEL(VK_F4, LEFT_ALT_PRESSED, REBarAltF4);
	ADDLABEL(VK_F4, LEFT_ALT_PRESSED|SHIFT_PRESSED, REBarAltShiftF4);
	ADDLABEL(VK_F4, LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED, REBarCtrlAltF4); // Права...
	ADDLABEL(VK_F4, SHIFT_PRESSED, REBarShiftF4);
	// Комбинации с F5
	ADDLABEL(VK_F5, 0, REBarF5);
	ADDLABEL(VK_F5, SHIFT_PRESSED, REBarShiftF5);
	// Комбинации с F6
	ADDLABEL(VK_F6, 0, REBarF5); // No Move! пока
	ADDLABEL(VK_F6, SHIFT_PRESSED, REBarShiftF6);
	ADDLABEL(VK_F6, LEFT_ALT_PRESSED, -1); // спрятать MkLink
	// Комбинации с F7
	ADDLABEL(VK_F7, 0, REBarF7);
	ADDLABEL(VK_F7, LEFT_ALT_PRESSED|SHIFT_PRESSED, REBarAltShiftF7);
	ADDLABEL(VK_F7, SHIFT_PRESSED, REBarShiftF7);
	// Fin
	_ASSERTE(idx<=ARRAYSIZE(kbl));
	static KeyBarTitles kbt = {idx, kbl};
#else
	static KeyBarTitles kbt = {{NULL}};
	// Комбинации с F1
	kbt.ShiftTitles[0] = GetMsg(REBarShiftF1);
	// Комбинации с F2
	kbt.ShiftTitles[1] = (pPlugin->m_Key.eType == RE_REGFILE) ? GetMsg(REBarShiftF2) : (pPlugin->m_Key.eType == RE_WINAPI) ? GetMsg(REBarShiftF2Virt) : sNil; // сохранить reg
	// Комбинации с F3
	kbt.Titles[2] = GetMsg(REBarF3);
	//kbt.AltTitles[2] = GetMsg(REBarAltF3);
	kbt.AltShiftTitles[2] = GetMsg(REBarAltShiftF3);
	kbt.ShiftTitles[2] = (pPlugin->m_Key.eType == RE_WINAPI) ? GetMsg(REBarShiftF3) : sNil; // переключить x32/x64
	kbt.CtrlAltTitles[2] = GetMsg(REBarCtrlAltF3); // Права...
	// Комбинации с F4
	kbt.Titles[3] = GetMsg(REBarF4);
	kbt.AltTitles[3] = GetMsg(REBarAltF4);
	kbt.AltShiftTitles[3] = GetMsg(REBarAltShiftF4);
	kbt.CtrlAltTitles[3] = GetMsg(REBarCtrlAltF4); // Права...
	kbt.ShiftTitles[3] = GetMsg(REBarShiftF4);
	// Комбинации с F5
	kbt.Titles[4] = GetMsg(REBarF5);
	kbt.ShiftTitles[4] = GetMsg(REBarShiftF5);
	// Комбинации с F6
	kbt.Titles[5] = GetMsg(REBarF5); // No Move! пока
	kbt.ShiftTitles[5] = GetMsg(REBarShiftF6);
	kbt.AltTitles[5] = _T(""); // спрятать MkLink
	// Комбинации с F7
	kbt.Titles[6] = GetMsg(REBarF7);
	kbt.AltShiftTitles[6] = GetMsg(REBarAltShiftF7);
	kbt.ShiftTitles[6] = GetMsg(REBarShiftF7);
#endif
	// Fin
	Info->KeyBar = &kbt;
}

// В Far3 имя подменяется (в *.def) на ClosePanelW
void WINAPI ClosePluginW(
#if FAR_UNICODE>=1900
		const struct ClosePanelInfo *Info
#else
		HANDLE hPlugin
#endif
		)
{
	#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	#endif
	if (hPlugin != INVALID_HANDLE_VALUE)
	{
		//CPluginActivator a(hPlugin,0); -- не надо, а то деструктор активатора пытается в pPlugin смотреть
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		gpActivePlugin = pPlugin;
		pPlugin->PreClosePlugin(); // предложить сохранить .reg файл
		MCHKHEAP;
		SafeDelete(pPlugin);
		MCHKHEAP;
		gpActivePlugin = NULL;
	}
	else
	{
		_ASSERTE(hPlugin != INVALID_HANDLE_VALUE);
	}
}


FARINT WINAPI ConfigureW(
#if FAR_UNICODE>=1900
		const struct ConfigureInfo *Info
#else
		int ItemNumber
#endif
					  )
{
	MCHKHEAP;
	if (!cfg)
	{
		_ASSERTE(cfg!=NULL);
		return FALSE;
	}
	// Возвращает TRUE, если фар нужно передернуть
	int nRc = cfg->Configure();
	MCHKHEAP;
	return nRc;
}

// В Far3 имя подменяется (в *.def) на ProcessPanelEventW
int WINAPI ProcessEventW(
#if FAR_UNICODE>=1900
		const struct ProcessPanelEventInfo *Info
#else
		HANDLE hPlugin, int Event, void *Param
#endif
		)
{
	#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	int Event = Info->Event;
	void *Param = Info->Param;
	#endif
	if (Event == FE_IDLE) {
		CPluginActivator a(hPlugin,0);
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		pPlugin->CheckItemsLoaded();
		pPlugin->OnIdle();
	}
	return FALSE;
}

#ifdef _UNICODE
FARINT WINAPI ProcessSynchroEventW(
#if FAR_UNICODE>=1900
		const struct ProcessSynchroEventInfo *Info
#else
		int Event, void *Param
#endif
		)
{
	#if FAR_UNICODE>=1900
	int Event = Info->Event;
	void *Param = Info->Param;
	#endif
	if (Event == SE_COMMONSYNCHRO && Param)
	{
		SynchroArg* pArg = (SynchroArg*)Param;
		if (pArg->nEvent == REGEDIT_SYNCHRO_DESC_FINISHED)
		{
			_ASSERTE(pArg->pPlugin!=NULL);
			if (gpPluginList->IsValid(pArg->pPlugin))
			{
				CPluginActivator a((HANDLE)pArg->pPlugin,0);
				pArg->pPlugin->CheckItemsLoaded();
				pArg->pPlugin->OnIdle();
			}
		}
	}
	return 0;
}
#endif

#if FAR_UNICODE>=1900
FARINT WINAPI CompareW(const CompareInfo *Info)
{
	// для скорости - не ставим: CPluginActivator a(Info->hPanel,0);
	REPlugin* pPlugin = (REPlugin*)Info->hPanel;
	return pPlugin->Compare(Info->Item1, Info->Item2, Info->Mode);
}
#else
int WINAPI CompareW(HANDLE hPlugin, const struct PluginPanelItem *Item1, const struct PluginPanelItem *Item2, unsigned int Mode)
{
	// для скорости - не ставим: CPluginActivator a(hPlugin,0);
	REPlugin* pPlugin = (REPlugin*)hPlugin;
	return pPlugin->Compare(Item1, Item2, Mode);
}
#endif

FARINT WINAPI ProcessEditorEventW(
#if FAR_UNICODE>=1900
		const struct ProcessEditorEventInfo *Info
#else
		int Event, void *Param
#endif
		)
{
	#if FAR_UNICODE>=1900
	int Event = Info->Event;
	void *Param = Info->Param;
	#endif

	if (gpActivePlugin)
	{
		if (Event == EE_GOTFOCUS)
		{
			if (MFileTxt::bBadMszDoubleZero)
			{
				MFileTxt::bBadMszDoubleZero = FALSE;
				REPlugin::Message(REM_BadDoubleZero);
				// Финт ушами, чтобы установить в редакторе статус "Изменен"
				#if FAR_UNICODE>=1900
				psi.EditorControl(-1, ECTL_INSERTTEXT,0, _T(" \x8"));
				#else
				psi.EditorControl(ECTL_INSERTTEXT,_T(" \x8"));
				#endif
			}
		}
		#if defined(_UNICODE) && (FAR_UNICODE<3000)
		// Если редактируется Sequence макроса - попытаемся его проверить :)
		// Для Lua-Far - бессмысленно, макросы в реестре больше не хранятся
		else if (gpActivePlugin->mb_InMacroEdit && Event == EE_SAVE)
		{
			EditorInfo ei = {0};
			int nLen = -1;
			#if FAR_UNICODE>=1900
			nLen = psi.EditorControl(-1, ECTL_GETINFO, 0, &ei);
			#else
			nLen = psi.EditorControl(ECTL_GETINFO, &ei);
			#endif
			if (ei.TotalLines > 0 && ei.CodePage != 1201) // Reverse unicode - не поддерживаем
			{
				//wchar_t* pszFilePathName = NULL;
				//nLen = psi.EditorControl(ECTL_GETFILENAME, NULL);
				//if (nLen > 0 && (pszFilePathName = (wchar_t*)calloc(nLen+1,2)))
				//{
				//	nLen = psi.EditorControl(ECTL_GETFILENAME, pszFilePathName);
				//	LPCWSTR pszFileName = PointToName(pszFilePathName);
				//	//TODO: Хорошо бы еще и на родительский ключ смотреть,
				//	//TODO: ибо для Vars/Common имена НЕ "Sequence", а проверить бы хотелось
				//	if (pszFileName && lstrcmpiW(pszFileName, L"Sequence") == 0)
				//	{
						// Проверяем. Сначала нужно получить сам текст макроса.
						wchar_t *pszMacro = NULL, *psz = NULL;
						int *pnStart = (int*)calloc(ei.TotalLines,sizeof(int));
						size_t nMaxLen = 0, nAllLen = 0; int nMaxLineLen = 2048;
						EditorGetString egs;
						egs.StringNumber = 0;
						#if FAR_UNICODE>=1900
						psi.EditorControl(-1, ECTL_GETSTRING, 0, &egs);
						#else
						psi.EditorControl(ECTL_GETSTRING,&egs);
						#endif
						if ((egs.StringLength*2+3) > nMaxLineLen)
							nMaxLineLen = egs.StringLength*2+3;
						nMaxLen = nMaxLineLen * ei.TotalLines;
						pszMacro = (wchar_t*)calloc(nMaxLen,2);
						if (!pszMacro)
						{
							InvalidOp(); // не хватило памяти
						} else {
							psz = pszMacro;
							int nLine = 0;
							for (egs.StringNumber = 0; egs.StringNumber < ei.TotalLines; egs.StringNumber++)
							{
								// первая строка (==0) уже получена
								if (egs.StringNumber)
								{
									#if FAR_UNICODE>=1900
									psi.EditorControl(-1, ECTL_GETSTRING,0, &egs);
									#else
									psi.EditorControl(ECTL_GETSTRING,&egs);
									#endif
								}
								// Памяти хватает?
								if ((nAllLen + egs.StringLength + 2) >= nMaxLen)
								{
									// Нужно выделить еще памяти
									nMaxLen = nMaxLen + min(16384,nMaxLen);
									wchar_t* pszNew = (wchar_t*)calloc(nMaxLen,2);
									if (!pszNew)
									{
										psz = NULL; break;
									}
								}
								// добавляем
								if (ei.CodePage == 1200)
								{
									wmemmove(psz, egs.StringText, egs.StringLength);								
								} else {
								}
								pnStart[nLine++] = nAllLen;
								psz += egs.StringLength; *(psz++) = L'\n'; *psz = 0;
								nAllLen += (egs.StringLength + 1);
							}
							// Проверяем, хватило ли памяти, все ли считано
							if (psz)
							{
								// Можно проверять макрос
								MacroParseResult* Result = NULL;
								#if FAR_UNICODE>=1900
								MacroSendMacroText mcr = {sizeof(mcr)};
								mcr.SequenceText = pszMacro;
								mcr.Flags = KMFLAGS_SILENTCHECK;
								#define MCTLARG0 ((gFarVersion.Build>=2159) ? &guid_PluginGuid : (GUID*)INVALID_HANDLE_VALUE)
								if (psi.MacroControl(MCTLARG0, MCTL_SENDSTRING, MSSC_CHECK, &mcr) == FALSE)
								{
									size_t iRcSize = psi.MacroControl(MCTLARG0, MCTL_GETLASTERROR, 0, NULL);
									Result = (MacroParseResult*)calloc(iRcSize,1);
									if (Result)
									{
										Result->StructSize = sizeof(*Result);
										psi.MacroControl(MCTLARG0, MCTL_GETLASTERROR, iRcSize, Result);
									}
								}
								else
								{
									Result = (MacroParseResult*)calloc(sizeof(MacroParseResult),1);
								}
								#else
								ActlKeyMacro mcr = {MCMD_CHECKMACRO};
								mcr.Param.PlainText.SequenceText = pszMacro;
								mcr.Param.PlainText.Flags = KSFLAGS_SILENTCHECK;
								psi.AdvControl(PluginNumber, ACTL_KEYMACRO, &mcr);
								Result = &mcr.Param.MacroResult;
								#endif
								
								//if (!psi.AdvControl(PluginNumber, ACTL_KEYMACRO, &mcr))
								//{
								//	REPlugin::Message(REM_MPEC_INTPARSERERROR);
								//}							
								//else
								if (!Result)
								{
									InvalidOp();
								}
								else if (Result->ErrCode == MPEC_INTPARSERERROR || Result->ErrCode > MPEC_CONTINUE_OTL)
								{
									REPlugin::Message(REM_MPEC_INTPARSERERROR);
								}
								else
								if (Result->ErrCode != 0)
								{
									// Ошибка!
									EditorSetPosition esp = {Result->ErrPos.Y, 0, -1, -1, -1, -1};
									#if FAR_UNICODE>=1900
									psi.EditorControl(-1, ECTL_SETPOSITION, 0, &esp);
									#else
									psi.EditorControl(ECTL_SETPOSITION, &esp);
									#endif
									int nMsgId = REM_MPEC_UNRECOGNIZED_KEYWORD + (Result->ErrCode - MPEC_UNRECOGNIZED_KEYWORD);
									LPCWSTR pszMsgFormat = GetMsg(nMsgId);
									if (pszMsgFormat)
									{
										int nLine = Result->ErrPos.Y, nCol = Result->ErrPos.X;
										//if (nLine > 0 && nLine < ei.TotalLines)
										//{
										//	if (nCol >= pnStart[nLine])
										//		nCol -= pnStart[nLine];
										//}
										//for (int i = ei.TotalLines-1; i > 0; i--)
										//{
										//	if (pnStart[i] <= 
										//}
										nLen = Result->ErrSrc ? lstrlen(Result->ErrSrc) : 0;
										nLen += lstrlen(pszMsgFormat) + 64;
										psz = (wchar_t*)calloc(nLen, 2);
										if (wcsstr(pszMsgFormat, L"%s"))
											wsprintfW(psz, pszMsgFormat, Result->ErrSrc ? Result->ErrSrc : L"", nLine+1, nCol+1);
										else
											wsprintfW(psz, pszMsgFormat, nLine+1, nCol+1);
										REPlugin::Message(psz);
										SafeFree(psz);
									} else {
										REPlugin::Message(REM_MPEC_INTPARSERERROR);
									}
								}

								#if FAR_UNICODE>=1900
								if (Result)
									free(Result);
								#endif
							}
						}
						SafeFree(pszMacro);
						SafeFree(pnStart);
				//	}
				//	SafeFree(pszFilePathName);
				//}
			}
		}
		#endif
	}
	return 0;
}


// FALSE для дальнейшей обработки FAR.
// TRUE, если плагин самостоятельно обработал клавишу. 
#if FAR_UNICODE>=1900
FARINT WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info)
#else
int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState)
#endif
{
	#if FAR_UNICODE>=1900
	HANDLE hPlugin = Info->hPanel;
	#endif

#ifndef FAR_UNICODE
	wchar_t szDbg[128]; wsprintfW(szDbg, L"ProcessKeyW(0x%X,0x%X)\n", Key, ControlState);
	OutputDebugStringW(szDbg);
#endif
	CPluginActivator a(hPlugin,0);

#if FAR_UNICODE>=1900
	if (Info->Rec.EventType != KEY_EVENT)
	{
		// Приходит "мусор" в Info->Rec.Event.KeyEvent
		//_ASSERTE(!(Info->Rec.EventType==MENU_EVENT && Info->Rec.Event.KeyEvent.wVirtualKeyCode==VK_F3));
		return FALSE;
	}
	#define IsKey(VK) (Info->Rec.Event.KeyEvent.wVirtualKeyCode == VK)
	#define ALL_MODS (LEFT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
	#define IsModNone ((Info->Rec.Event.KeyEvent.dwControlKeyState & ALL_MODS) == 0)
	//
	bool IsShift = ((Info->Rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) == SHIFT_PRESSED);
	bool IsAlt = (((Info->Rec.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED) == LEFT_ALT_PRESSED) || ((Info->Rec.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED) == RIGHT_ALT_PRESSED));
	bool IsCtrl = (((Info->Rec.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) == LEFT_CTRL_PRESSED) || ((Info->Rec.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED) == RIGHT_CTRL_PRESSED));
	//
	#define IsModShift (IsShift && !IsAlt && !IsCtrl)
	#define IsModAlt (!IsShift && IsAlt && !IsCtrl)
	#define IsModCtrl (!IsShift && !IsAlt && IsCtrl)
	#define IsModAltShift (IsShift && IsAlt && !IsCtrl)
	#define IsModCtrlAlt (!IsShift && IsAlt && IsCtrl)
	#define IsModCtrlShift (IsShift && !IsAlt && IsCtrl)
#else
	#define IsKey(VK) ((Key & 0xFF) == VK)
	#define IsModNone (ControlState == 0)
	#define IsModShift (ControlState == PKF_SHIFT)
	#define IsModAlt (ControlState == PKF_ALT)
	#define IsModAltShift (ControlState == (PKF_ALT|PKF_SHIFT))
	#define IsModCtrl (ControlState == PKF_CONTROL)
	#define IsModCtrlAlt (ControlState == (PKF_CONTROL|PKF_ALT))
	#define IsModCtrlShift (ControlState == (PKF_CONTROL|PKF_SHIFT))
#endif

	if (IsKey(VK_F1))
	{
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		pPlugin->CheckItemsLoaded();
		// ShiftF1 - Отобразить и перейти на закладку
		if (IsModShift)
		{
			pPlugin->ShowBookmarks();
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F1)
	if (IsKey(VK_F2))
	{
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		pPlugin->CheckItemsLoaded();
		// ShiftF2 - Save opened *.reg file
		if (IsModShift)
		{
			if (pPlugin->m_Key.eType == RE_REGFILE)
			{
				pPlugin->SaveRegFile();
			}
			else if (pPlugin->m_Key.eType == RE_WINAPI)
			{
				//cfg->bVirtualize = !cfg->bVirtualize;
				pPlugin->setVirtualize(pPlugin->getVirtualize()==0);
				pPlugin->SetForceReload();
				pPlugin->UpdatePanel(false);
				pPlugin->RedrawPanel();
			}
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F2)
	else if (IsKey(VK_F3) || IsKey(VK_CLEAR))
	{
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		pPlugin->CheckItemsLoaded();
		// F3 - авто редактирование (DWORD/String)
		if (IsModNone /*|| IsModAlt*/)
		{
			pPlugin->EditItem(true, true, true);
			return TRUE;
		}
		// AltShiftF3 - редактирование в *.reg ВЫДЕЛЕННЫХ элементов (значений или ключей)
		else if (IsModAltShift)
		{
			pPlugin->EditItem(false, true, true);
			return TRUE;
		}
		// CtrlShiftF3 - открыть во встроенном просмотрщике RAW данные (Unicode only)
		else if (IsModCtrlShift)
		{
			pPlugin->EditItem(true/*abOnlyCurrent*/, false/*abForceExport*/, true/*abViewOnly*/, true/*abRawData*/);
			return TRUE;
		}
		// ShiftF3 - переключение [32]<-->[64] битный реестр
		else if (IsModShift)
		{
			// Кнопку перехватываем всегда, а вот опцию дергаем только в x64 OS
			//if (cfg->is64bitOs)
			{
				//cfg->bWow64on32 = !cfg->bWow64on32;
				switch (pPlugin->getWow64on32())
				{
					case 0:
						pPlugin->setWow64on32(1);
						break;
					case 1:
						pPlugin->setWow64on32(2);
						break;
					default:
						pPlugin->setWow64on32(0);
						break;
				}
				//if (cfg->bWow64on32 == 0) {
				//	cfg->bWow64on32 = 1;
				//} else if (cfg->bWow64on32 == 1) {
				//	cfg->bWow64on32 = 2;
				//} else {
				//	cfg->bWow64on32 = 0;
				//}
				pPlugin->SetForceReload();
				pPlugin->UpdatePanel(false);
				pPlugin->RedrawPanel();
			}
			return TRUE;
		}
		// CtrlAltF3 - права
		else if (IsModCtrlAlt)
		{
			MCHKHEAP;
			pPlugin->EditKeyPermissions(FALSE);
			MCHKHEAP;
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F3)
	else if (IsKey(VK_F4))
	{
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		pPlugin->CheckItemsLoaded();
		// F4 - авто редактирование (DWORD/String)
		if (IsModNone)
		{
			pPlugin->EditItem(true, false, false);
			return TRUE;
		}
		// AltF4 - редактирование в *.reg текущего элемента
		else if (IsModAlt)
		{
			pPlugin->EditItem(true, true, false);
			return TRUE;
		}
		// AltShiftF4 - редактирование в *.reg ВЫДЕЛЕННЫХ элементов (значений или ключей)
		else if (IsModAltShift)
		{
			pPlugin->EditItem(false, true, false);
			return TRUE;
		}
		// CtrlShiftF4 - открыть во встроенном редакторе RAW данные (Unicode only)
		else if (IsModCtrlShift)
		{
			pPlugin->EditItem(true/*abOnlyCurrent*/, false/*abForceExport*/, false/*abViewOnly*/, true/*abRawData*/);
			return TRUE;
		}
		// ShiftF4 - создать новое значение
		else if (IsModShift)
		{
			pPlugin->NewItem();
			return TRUE;
		}
		// CtrlAltF4 - права
		else if (IsModCtrlAlt)
		{
			MCHKHEAP;
			pPlugin->EditKeyPermissions(TRUE);
			MCHKHEAP;
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F4)
	else if (IsKey(VK_F5))
	{
		// F5 - если на соседней панели наш плагин - выполнить копирование БЕЗ экспорта в ФС
		if (IsModNone)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			REPlugin* pOpposite = gpPluginList->GetOpposite(pPlugin);
			if (pOpposite)
			{
				pPlugin->Transfer(pOpposite, FALSE);
				return TRUE;
			}
		}
		// ShiftF5 - Сделать копию ключа реестра или значения (аналогично ShiftF6)
		else if (IsModShift)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			pPlugin->CheckItemsLoaded();
			pPlugin->RenameOrCopyItem(TRUE/*abCopyOnly*/);
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F5)
	else if (IsKey(VK_F6))
	{
		// F6 - если на соседней панели наш плагин - выполнить перемещение БЕЗ экспорта в ФС
		if (IsModNone)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			REPlugin* pOpposite = gpPluginList->GetOpposite(pPlugin);
			if (pOpposite)
			{
				pPlugin->Transfer(pOpposite, TRUE);
				return TRUE;
			}
		}
		// ShiftF6 - переименовать значение или ключ
		else if (IsModShift)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			pPlugin->CheckItemsLoaded();
			pPlugin->RenameOrCopyItem(FALSE/*abCopyOnly*/);
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F6)
	else if (IsKey(VK_F7))
	{
		// AltShiftF7 - Подключиться к удаленной машине
		if (IsModAltShift)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			pPlugin->ConnectRemote();
			return TRUE;
		}
		// ShiftF7
		else if (IsModShift)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			cfg->bShowKeysAsDirs = !cfg->bShowKeysAsDirs;
			pPlugin->SetForceReload();
			pPlugin->UpdatePanel(false);
			pPlugin->RedrawPanel();
			return TRUE;
		}
	} // if ((Key & 0xFF) == VK_F7)
	//else if ((Key & 0xFF) == 'A')
	//{
	//	// CtrlA - Права на ключ (пока только просмотр)
	//	if (IsModCtrl)
	//	{
	//		REPlugin* pPlugin = (REPlugin*)hPlugin;
	//		MCHKHEAP;
	//		pPlugin->EditKeyPermissions();
	//		MCHKHEAP;
	//		return TRUE;
	//	}
	//} // if ((Key & 0xFF) == 'A')
	else if (IsKey('J'))
	{
		// CtrlJ - Открыть текущий ключ в RegEdit.exe, аналог ShiftEnter
		if (IsModCtrl)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			pPlugin->CheckItemsLoaded();
			pPlugin->JumpToRegedit();
			return TRUE; // Обработали
		}
	} // if ((Key & 0xFF) == 'J')
	else if (IsKey('R'))
	{
		// CtrlR - сбросить кеш текущего ключа
		if (IsModCtrl)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			MCHKHEAP;
			pPlugin->SetForceReload();
			MCHKHEAP;
			return FALSE; //  пусть Фар перечитает сам
		}
	} // if ((Key & 0xFF) == 'R')
	else if (IsKey('Z'))
	{
		// CtrlZ - редактировать в стиле "Diz"
		if (IsModCtrl)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			pPlugin->CheckItemsLoaded();
			pPlugin->EditDescription();
			return TRUE; // Обработали
		}
	} // if ((Key & 0xFF) == 'Z')
	else if (IsKey(VK_RETURN))
	{
		// ShiftEnter - Открыть текущий ключ в RegEdit.exe, аналог CtrlJ
		if (IsModShift)
		{
			REPlugin* pPlugin = (REPlugin*)hPlugin;
			pPlugin->CheckItemsLoaded();
			pPlugin->JumpToRegedit();
			return TRUE; // Обработали
		}
	} // if ((Key & 0xFF) == VK_RETURN)
	
	// Это условие стоит особняком, т.к. проверяется не просто кнопка - а еще и варианты...
	// Вобщем, чтобы не наколоться с Else...
	if (!cfg->bShowKeysAsDirs // лучше заход в папку обрабатывать самим - это гарантирует корректную обработку "левых" имен
		&& ((IsKey(VK_RETURN) && IsModNone)
			|| (IsKey(VK_NEXT) && IsModCtrl)))
	{
		REPlugin* pPlugin = (REPlugin*)hPlugin;
		pPlugin->CheckItemsLoaded();
		RegItem* p = pPlugin->GetCurrentItem();
		if (p && p->nValueType == REG__KEY && p->pszName)
		{
			_ASSERTE(wcschr(p->pszName,L'\\') == NULL);
			if (pPlugin->SetDirectory(p, FALSE))
			{
				pPlugin->SetForceReload();
				pPlugin->UpdatePanel(false);
				pPlugin->RedrawPanel((RegItem*)1);
			}
			return TRUE;
		}
	} // Enter или CtrlPgDn в режиме "Ключи как файлы"
	
	return FALSE; // для дальнейшей обработки FAR.
}
