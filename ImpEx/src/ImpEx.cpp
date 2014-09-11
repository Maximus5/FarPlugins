// ImpEx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <malloc.h>
#include "common.h"
#include "ConEmuSupport.h"
#include "version.h"

//TODO:
//1. В заголовке плагина показывать "ImpEx:<PEFileName> [<CurFolderName>]"
//2. CTRLZ - всплывать свой диалог со строками Owner/Description

#ifdef FAR_UNICODE
#pragma message("Defining FAR3 guids")
#endif

#if FAR_UNICODE>=1906
// Plugin GUID {D0704272-AEFA-40DF-9141-DFCB4C9E6125}
static GUID guid_PluginGuid = 
{ 0xd0704272, 0xaefa, 0x40df, { 0x91, 0x41, 0xdf, 0xcb, 0x4c, 0x9e, 0x61, 0x25 } };
// Menus
// {C3709136-AF6F-4194-84E4-461E3BB524DC}
static GUID guid_PluginMenu = 
{ 0xc3709136, 0xaf6f, 0x4194, { 0x84, 0xe4, 0x46, 0x1e, 0x3b, 0xb5, 0x24, 0xdc } };
// {3B71A16B-9D3D-4F80-8A50-748074800549}
static GUID guid_DiskMenu = 
{ 0x3b71a16b, 0x9d3d, 0x4f80, { 0x8a, 0x50, 0x74, 0x80, 0x74, 0x80, 0x5, 0x49 } };
// {0AA5CF70-29A5-4DB8-BC1E-09A03E517D95}
static GUID guid_ConfigMenu = 
{ 0xaa5cf70, 0x29a5, 0x4db8, { 0xbc, 0x1e, 0x9, 0xa0, 0x3e, 0x51, 0x7d, 0x95 } };
#endif


#ifdef _UNICODE
#define OPEN_ANALYSE 9
struct AnalyseData
{
	int StructSize;
	const wchar_t *lpwszFileName;
	const unsigned char *pBuffer;
	DWORD dwBufferSize;
	int OpMode;
};
struct AnalyseData *gpLastAnalyse = NULL;
#endif

bool gbUseMenu = true, gbUseEnter = true, gbUseCtrlPgDn = true, gbLibRegUnreg = true, gbLibLoadUnload = true;
bool gbUseUndecorate = true;
bool gbDecimalIds = true;
UnDecorateSymbolName_t UnDecorate_Dbghelp = NULL;
HMODULE ghDbghelp = NULL;
void LoadSettings();
bool IsKeyPressed(WORD vk);
//#define IMPEX_USEMENU TRUE
//#define IMPEX_USEENTER FALSE
//#define IMPEX_USECTRLPGDN TRUE

PluginStartupInfo psi;
FarStandardFunctions fsf;
#if !defined(FAR_UNICODE)
TCHAR* gsRootKey = NULL;
#endif

BOOL gbSilentMode = FALSE;
//BOOL gbAutoMode = FALSE;

HWND hConEmuWnd = NULL;
HANDLE hConEmuCtrlPressed = NULL;

void WINAPI SetStartupInfoW (struct PluginStartupInfo *Info)
{
	psi = *Info;
	fsf = *Info->FSF;
	psi.FSF = &fsf;

	#if !defined(FAR_UNICODE)
	size_t nLen = lstrlen(Info->RootKey);
	gsRootKey = (TCHAR*)calloc((nLen+32),sizeof(TCHAR));
	lstrcpy(gsRootKey, Info->RootKey);
	if (gsRootKey[nLen] != _T('\\'))
		gsRootKey[nLen++] = _T('\\');
	lstrcpy(gsRootKey+nLen, _T("ImpEx"));
	#endif
}

#ifndef FAR_UNICODE
int WINAPI GetMinFarVersionW ()
{
	return FARMANAGERVERSION;
}
#else
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
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"ImpEx";
	Info->Description = L"PE Import/Export browser";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}
#endif

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
	LoadSettings();

	#if FAR_UNICODE>=1906
	_ASSERTE(pi->StructSize >= sizeof(struct PluginInfo));
	#else
	pi->StructSize = sizeof(struct PluginInfo);
	#endif

    static TCHAR *szMenu[1];
    szMenu[0]=_T("ImpEx");

    pi->Flags = 0;
    pi->CommandPrefix = _T("ImpEx");


	#if FAR_UNICODE>=1906
		pi->DiskMenu.Guids = NULL;
		pi->DiskMenu.Strings = NULL;
		pi->DiskMenu.Count = 0;
	#else
		pi->DiskMenuStrings = NULL;
		pi->DiskMenuNumbers = NULL;
		pi->DiskMenuStringsNumber = 0;
	#endif

	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_PluginMenu;
		pi->PluginMenu.Strings = gbUseMenu ? szMenu : NULL;
		pi->PluginMenu.Count = gbUseMenu ? 1 : 0;
	#else
		pi->PluginMenuStrings = gbUseMenu ? szMenu : NULL;
		pi->PluginMenuStringsNumber = gbUseMenu ? 1 : 0;
	#endif
	
	#if FAR_UNICODE>=1906
		pi->PluginConfig.Guids = NULL;
		pi->PluginConfig.Strings = NULL;
		pi->PluginConfig.Count = 0;
	#else
		pi->PluginConfigStrings = NULL; //szMenu;
		pi->PluginConfigStringsNumber = 0; //1;
	#endif
}

void WINAPI GetOpenPluginInfoW(
		#if FAR_UNICODE>2041
			struct OpenPanelInfo *Info
		#else
			HANDLE hPlugin, struct OpenPluginInfo *Info
		#endif
		)
{
	#if FAR_UNICODE>2041
	HANDLE hPlugin = Info->hPanel;
	#endif
	if (!hPlugin || (hPlugin == INVALID_HANDLE_VALUE))
	{
		_ASSERTE(FALSE && "Invalid plugin handle!");
		return;
	}
	((MImpEx*)hPlugin)->getOpenPluginInfo(Info);
}

FARINT WINAPI GetFindDataW(
	#if FAR_UNICODE>2041
	struct GetFindDataInfo *Info
	#else
	HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode 
	#endif
	)
{
	int iRc;
	#if FAR_UNICODE>2041
	int iCount = 0;
	iRc = ((MImpEx*)Info->hPanel)->getFindData(&Info->PanelItem, &iCount, Info->OpMode);
	Info->ItemsNumber = iCount;
	#else
	iRc = ((MImpEx*)hPlugin)->getFindData(pPanelItem, pItemsNumber, OpMode);
	#endif
	return iRc;
}

void WINAPI FreeFindDataW(
	#if FAR_UNICODE>2041
	const struct FreeFindDataInfo *Info
	#else
	HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber
	#endif
	)
{
	#if FAR_UNICODE>2041
	((MImpEx*)Info->hPanel)->freeFindData(Info->PanelItem, Info->ItemsNumber);
	#else
	((MImpEx*)hPlugin)->freeFindData(PanelItem, ItemsNumber);
	#endif
}

void WINAPI ClosePluginW(
	#if FAR_UNICODE>2041
	const struct ClosePanelInfo *Info
	#else
	HANDLE hPlugin
	#endif
	)
{
	MImpEx *pPlugin;
	#if FAR_UNICODE>2041
	pPlugin = (MImpEx*)Info->hPanel;
	#else
	pPlugin = (MImpEx*)hPlugin;
	#endif

	if (pPlugin)
	{
		delete pPlugin;
	}
}

//#ifdef _UNICODE
//int WINAPI GetFilesW( HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode )
//{
//  static MString lsBuffer;
//  LPWSTR  pszBuffer = lsBuffer.GetBuffer(MAX_PATH+wcslen(*DestPath));
//  wcscpy(pszBuffer, *DestPath);
//  *DestPath = pszBuffer;
//  return ((MImpEx*)hPlugin)->getFiles(PanelItem, ItemsNumber, Move, pszBuffer, OpMode );
//}
//#else
//int WINAPI GetFilesW( HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, LPSTR DestPath, int OpMode )
//{
//  return ((MImpEx*)hPlugin)->getFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode );
//}
//#endif

BOOL IsFileSupported(const TCHAR *Name, const unsigned char *Data, int DataSize)
{
	if (!Name || !*Name)
		return FALSE; // ShiftF1
	
	_ASSERTE(sizeof(IMAGE_FILE_HEADER) < sizeof(IMAGE_DOS_HEADER));
	if (!Data || DataSize < sizeof(IMAGE_DOS_HEADER))
	{
		return FALSE;
	}

	LPCTSTR pszNameOnly = psi.FSF->PointToName(Name);
	LPCTSTR pszExt = pszNameOnly ? _tcsrchr(pszNameOnly, _T('.')) : NULL;
	if (!pszExt) pszExt = _T("");
	
	bool lbFormat = false;
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)Data;
	PIMAGE_FILE_HEADER pImgFileHdr = (PIMAGE_FILE_HEADER)Data;
	if ( pDosHeader->e_magic == IMAGE_DOS_SIGNATURE )
	{
		lbFormat = true;
	}
	else if ( pDosHeader->e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE )
	{
		lbFormat = true;
	}
	else if ( IsValidMachineType(pImgFileHdr->Machine, TRUE) )
	{
		if ( 0 == pImgFileHdr->SizeOfOptionalHeader && lstrcmpi(pszExt, _T(".obj")) == 0 )	// 0 optional header
			lbFormat = true;
		else if ( pImgFileHdr->SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER )
			lbFormat = true;
	}
	else if ( 0 == strncmp((char *)Data, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) )
	{
		lbFormat = true;
	}

	if (!lbFormat)
		return FALSE;

	LoadSettings();
	if (!gbUseEnter && !gbUseCtrlPgDn)
	{
		return FALSE;
	}
	else if (!gbUseEnter)
	{
		if (!IsKeyPressed(VK_CONTROL))
			return FALSE;
	}
	
	return TRUE;
}

#ifdef _UNICODE
int WINAPI AnalyseW(const AnalyseData *pData)
{
	int nFormat = -1;

	SAFEFREE(gpLastAnalyse);

	if (!IsFileSupported(pData->lpwszFileName, pData->pBuffer, pData->dwBufferSize))
		return 0;

	// Запомнить
	int nAllSize = (int)sizeof(*gpLastAnalyse) + pData->dwBufferSize;
	gpLastAnalyse = (struct AnalyseData*)malloc(nAllSize);
	*gpLastAnalyse = *pData;
	gpLastAnalyse->pBuffer = ((const unsigned char*)gpLastAnalyse) + sizeof(*gpLastAnalyse);
	memmove((void*)gpLastAnalyse->pBuffer, pData->pBuffer, pData->dwBufferSize);

	return TRUE;
}

#if (FARMANAGERVERSION_BUILD>=2462)
void WINAPI CloseAnalyseW(const struct CloseAnalyseInfo *Info)
{
	return;
}
#endif
#endif


HANDLE WINAPI OpenFilePluginW(
  const TCHAR *Name,
  const unsigned char *Data,
  int DataSize
#ifdef _UNICODE
  ,int OpMode
#endif
)
{
	if (!Name || !*Name)
		return INVALID_HANDLE_VALUE; // ShiftF1

	#ifdef _UNICODE
	if ((OpMode & OPM_FIND) != 0)
		return INVALID_HANDLE_VALUE;
	#endif
		
	if (!IsFileSupported(Name, Data, DataSize))
		return INVALID_HANDLE_VALUE;
	
	//_ASSERTE(sizeof(IMAGE_FILE_HEADER) < sizeof(IMAGE_DOS_HEADER));
	//if (
	//	#ifdef _UNICODE
	//	(OpMode & OPM_FIND) != 0 ||
	//	#endif
	//	!Data || DataSize < sizeof(IMAGE_DOS_HEADER)
	//   )
	//{
	//	return INVALID_HANDLE_VALUE;
	//}
	//
	//LPCTSTR pszNameOnly = psi.FSF->PointToName(Name);
	//LPCTSTR pszExt = pszNameOnly ? _tcsrchr(pszNameOnly, _T('.')) : NULL;
	//if (!pszExt) pszExt = _T("");
	//
	//bool lbFormat = false;
	//PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)Data;
	//PIMAGE_FILE_HEADER pImgFileHdr = (PIMAGE_FILE_HEADER)Data;
	//if ( pDosHeader->e_magic == IMAGE_DOS_SIGNATURE )
	//{
	//	lbFormat = true;
	//}
	//else if ( pDosHeader->e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE )
	//{
	//	lbFormat = true;
	//}
	//else if ( IsValidMachineType(pImgFileHdr->Machine, TRUE) )
	//{
	//	if ( 0 == pImgFileHdr->SizeOfOptionalHeader && lstrcmpi(pszExt, _T(".obj")) == 0 )	// 0 optional header
	//		lbFormat = true;
	//	else if ( pImgFileHdr->SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER )
	//		lbFormat = true;
	//}
	//else if ( 0 == strncmp((char *)Data, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) )
	//{
	//	lbFormat = true;
	//}
	//
	//if (!lbFormat)
	//	return INVALID_HANDLE_VALUE;
	//
	//LoadSettings();
	//if (!gbUseEnter && !gbUseCtrlPgDn)
	//{
	//	return INVALID_HANDLE_VALUE;
	//}
	//else if (!gbUseEnter)
	//{
	//	if (!IsKeyPressed(VK_CONTROL))
	//		return INVALID_HANDLE_VALUE;
	//}


	HANDLE hPlugin = NULL;
	
    //TCHAR* pszFull = NULL;
    
    //#ifdef _UNICODE
	//	int nLen = (int)((FarStandardFunctions*)fsf)->ConvertPath(CPM_FULL, Name, NULL, 0);
	//	if (nLen > 0) {
	//		pszFull = (TCHAR*)calloc(nLen,sizeof(TCHAR));
	//		((FarStandardFunctions*)fsf)->ConvertPath(CPM_FULL, Name, pszFull, nLen);
	//	}
	//#endif
	
	#ifdef _UNICODE
	gbSilentMode = (OpMode & (OPM_SILENT | OPM_FIND)) != 0;
	#else
	gbSilentMode = TRUE;
	#endif
	//gbAutoMode = TRUE;
	
	hPlugin = MImpEx::Open(/*pszFull ? pszFull :*/ Name, false);
	
	//if (pszFull) {
	//	free(pszFull);
	//}
	
	return hPlugin;
}

HANDLE WINAPI OpenPluginW(
	#if FAR_UNICODE>=2041
	const struct OpenInfo *Info
	#else
	int OpenFrom, INT_PTR Item
	#endif
	)
{
	#if FAR_UNICODE>=2041
	OPENFROM OpenFrom = Info->OpenFrom;
	INT_PTR Item = Info->Data;
	#endif

	#if defined(_UNICODE) && (FARMANAGERVERSION_BUILD>=2572)
	const HANDLE PanelStop = PANEL_STOP;
	const HANDLE PanelNone = NULL;
	#else
	const HANDLE PanelStop = (HANDLE)-2;
	const HANDLE PanelNone = INVALID_HANDLE_VALUE;
	#endif

	HANDLE hPlugin = PanelNone;

	//gbAutoMode = FALSE;
	gbSilentMode = FALSE;
	
	#ifdef _UNICODE
	// После AnalyseW вызывается OpenPluginW, а не OpenFilePluginW
	if (OpenFrom == OPEN_ANALYSE)
	{
		hPlugin = OpenFilePluginW(gpLastAnalyse->lpwszFileName,
			gpLastAnalyse->pBuffer, gpLastAnalyse->dwBufferSize, 0/*OpMode*/);
		SAFEFREE(gpLastAnalyse);
		if (hPlugin == (HANDLE)-2)
			hPlugin = PanelStop;
		return hPlugin;
	}
	SAFEFREE(gpLastAnalyse);
	#endif

	
    if (OpenFrom==OPEN_PLUGINSMENU)
    {
        // Плагин пытаются открыть из плагиновского меню
		PanelInfo pi = {};
		#if FAR_UNICODE>=2041
		pi.StructSize = sizeof(pi);
		#endif
        if (psiControl(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, F757NA &pi))
        {
			
            if (pi.PanelType==PTYPE_FILEPANEL && pi.ItemsNumber>0 /*&& pi.CurrentItem>0*/)
            {
                //
                BOOL    lbOpenMode = FALSE;
				PluginPanelItem* item=NULL;
				LPCTSTR pszFileName = NULL;
                
                // Если же выделения нет, но курсор стоит на файле
                if (pi.CurrentItem<=0)
                	return PanelNone;
                	
				
				#if defined(_UNICODE)
					INT_PTR nSize = psiControl(INVALID_HANDLE_VALUE, FCTL_GETPANELITEM, pi.CurrentItem, NULL);
					if (!nSize)
						return PanelNone;
					item = (PluginPanelItem*)calloc(nSize,1);
					#if FAR_UNICODE>=3000
					FarGetPluginPanelItem gpi = {sizeof(gpi), nSize, item};
					psiControl(INVALID_HANDLE_VALUE, FCTL_GETPANELITEM, pi.CurrentItem, &gpi);
					#else
					psiControl(INVALID_HANDLE_VALUE, FCTL_GETPANELITEM, pi.CurrentItem, (FARPTR)item);
					#endif
				#else
					item = &pi.PanelItems[pi.CurrentItem];
				#endif

				pszFileName = FILENAMEPTR(*item);
                if (pszFileName && *pszFileName && _tcscmp(pszFileName, _T(".."))!=0)
                {
                    lbOpenMode = TRUE;
                }

                
                if (lbOpenMode)
                {
                    int nLen = 0;
                    TCHAR* pszFull = NULL;
                    
                    #ifdef _UNICODE
						nLen = (int)fsf.ConvertPath(CPM_FULL, pszFileName, NULL, 0);
						if (nLen > 0)
						{
							pszFull = (TCHAR*)calloc(nLen,sizeof(TCHAR));
							fsf.ConvertPath(CPM_FULL, pszFileName, pszFull, nLen);
						}
                    #else
                    	int nDirLen = lstrlen(pi.CurDir);
                    	nLen = nDirLen+3+lstrlen(pszFileName);
                    	pszFull = (TCHAR*)calloc(nLen,sizeof(TCHAR));
                    	lstrcpy(pszFull, pi.CurDir);
                    	if (nDirLen && pszFull[nDirLen-1] != '\\')
                    	{
                    		pszFull[nDirLen++] = '\\';
                    		pszFull[nDirLen] = 0;
                    	}
                    	lstrcpy(pszFull+nDirLen, pszFileName);
                    #endif

					if (pszFull)
					{
                    	hPlugin = MImpEx::Open(pszFull, true);
                    	free(pszFull);
                	}
                }
                
				#ifdef _UNICODE
					SAFEFREE(item);
				#elif FARBUILD>=757
					psiControl(INVALID_HANDLE_VALUE, FCTL_FREEPANELITEM, 0, (LONG_PTR)&item);
				#endif
            }
        }
    }
    else if (OpenFrom==OPEN_COMMANDLINE)
    {
		if (Item)
		{
			TCHAR *pszTemp = NULL;
			TCHAR *pszFull = NULL;
			LPCTSTR pszName = (LPCTSTR)Item;
			if (*pszName == _T('"'))
			{
				pszTemp = _tcsdup(pszName+1);
				TCHAR* pszQ = _tcschr(pszTemp, _T('"'));
				if (pszQ) *pszQ = 0;
				pszName = pszTemp;
			}

			#ifdef _UNICODE
				int nLen = (int)fsf.ConvertPath(CPM_FULL, pszName, NULL, 0);
				if (nLen > 0)
				{
					pszFull = (TCHAR*)calloc(nLen,sizeof(TCHAR));
					fsf.ConvertPath(CPM_FULL, pszName, pszFull, nLen);
					pszName = pszFull;
				}
			#else
				pszFull = (TCHAR*)calloc(MAX_PATH*2,sizeof(TCHAR));
				TCHAR* pszFilePart = NULL;
				if (GetFullPathName(pszName, MAX_PATH*2, pszFull, &pszFilePart))
					pszName = pszFull;								
			#endif

            hPlugin = MImpEx::Open(pszName, true);
			if (pszTemp) free(pszTemp);
			if (pszFull) free(pszFull);
		}
    }
    return hPlugin;
}

FARINT WINAPI SetDirectoryW (
	#if FAR_UNICODE>=2041
		const struct SetDirectoryInfo *Info
	#else
		HANDLE hPlugin, LPCTSTR Dir, int OpMode
	#endif
	)
{
    // В случае удачного завершения возвращаемое значение должно равняться TRUE. Иначе функция должна возвращать FALSE. 
	#if FAR_UNICODE>=2041
	return ((MImpEx*)Info->hPanel)->setDirectory ( Info->Dir, Info->OpMode );
	#else
    return ((MImpEx*)hPlugin)->setDirectory ( Dir, OpMode );
	#endif
}

FARINT WINAPI ConfigureW(
	#if FAR_UNICODE>=2041
	const struct ConfigureInfo *Info
	#else
	int ItemNumber
	#endif
	)
{
	//TODO
    return TRUE;
}

FARINT WINAPI CompareW(
	#if FAR_UNICODE>=2041
	const struct CompareInfo *Info
	#else
	HANDLE hPlugin,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,unsigned int Mode
	#endif
	)
{
	LPCTSTR pszName1, pszName2;
	#if FAR_UNICODE>=2041
	pszName1 = FILENAMEPTR(*Info->Item1);
	pszName2 = FILENAMEPTR(*Info->Item2);
	#else
	pszName1 = FILENAMEPTR(*Item1);
	pszName2 = FILENAMEPTR(*Item2);
	#endif

	if (!_tcscmp(pszName1, DUMP_FILE_NAME))
		return -1;
	else if (!_tcscmp(pszName2, DUMP_FILE_NAME))
		return 1;
	
	return -2; // использовать внутреннюю сортировку
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

	if (IsKey(VK_F3))
	{
		MImpEx* pPlugin = (MImpEx*)hPlugin;
		// AltF3 - просмотр кода экспортируемого элемента
		if (IsModAlt)
		{
			if (pPlugin->ViewCode())
				return TRUE;
		}
	}

	return FALSE; // отдать фару
}

void merror(LPCTSTR asFormat, ...)
{
	if (gbSilentMode) return;
	
    va_list args;
    va_start(args,asFormat);
    TCHAR szBuffer[2048]; szBuffer[0] = 0;
    
    lstrcpy(szBuffer, _T("ImpEx\n"));
    
    int nLen = wvsprintf(szBuffer+lstrlen(szBuffer), asFormat, args);
	if (!nLen || !szBuffer[0])
		lstrcpy(szBuffer, _T("<Empty error text>"));
	
	psi.Message(_PluginNumber(guid_PluginGuid), FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING, 
		NULL, (const TCHAR * const *)szBuffer, 0,0);
}

FARINT WINAPI GetFilesW(
	#if FAR_UNICODE>=2041
		struct GetFilesInfo *Info
	#else
		HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move,
		#ifdef _UNICODE
			const wchar_t **DestPath, int OpMode
		#else
			LPSTR DestPath, int OpMode
		#endif
	#endif
	)
{
#ifdef _UNICODE
	#if FAR_UNICODE>=2041
	HANDLE hPlugin = Info->hPanel;
	PluginPanelItem* PanelItem = Info->PanelItem;
	int ItemsNumber = Info->ItemsNumber;
	int Move = Info->Move;
	const wchar_t **DestPath = &Info->DestPath;
	int OpMode = Info->OpMode;
	#endif
	static wchar_t szBuffer[MAX_PATH*2];
	wcscpy(szBuffer, *DestPath);
	*DestPath = szBuffer;
	return ((MImpEx*)hPlugin)->getFiles(PanelItem, ItemsNumber, Move, szBuffer, OpMode );
#else
	return ((MImpEx*)hPlugin)->getFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode );
#endif
}

void WINAPI ExitFARW(
	#if FAR_UNICODE>=2041
	const struct ExitInfo *Info
	#endif
	)
{
	#ifdef _UNICODE
	SAFEFREE(gpLastAnalyse);
	#endif

	#if !defined(FAR_UNICODE)
	if (gsRootKey)
	{
		free(gsRootKey); gsRootKey = NULL;
	}
	#endif

	if (hConEmuCtrlPressed)
	{
		CloseHandle(hConEmuCtrlPressed); hConEmuCtrlPressed = NULL;
	}

	if (ghDbghelp)
	{
		FreeLibrary(ghDbghelp);
		ghDbghelp = NULL;
		UnDecorate_Dbghelp = NULL;
	}
}

void LoadSettings()
{
#if defined(FAR_UNICODE)
	PluginSettings settings(guid_PluginGuid, psi.SettingsControl);
	// gbUseMenu, gbUseEnter, gbUseCtrlPgDn, gbLibRegUnreg, gbLibLoadUnload;
	struct tag_Settings {
		bool* pbVal; LPCTSTR psName;
	};
	struct tag_Settings Settings[] =
	{
		{&gbUseMenu, _T("UseMenu")},
		{&gbUseEnter, _T("UseEnter")},
		{&gbUseCtrlPgDn, _T("UseCtrlPgDn")},
		{&gbLibRegUnreg, _T("LibRegUnreg")},
		{&gbLibLoadUnload, _T("LibLoadUnload")},
		{&gbUseUndecorate, _T("UseUndecorate")},
		{&gbDecimalIds, _T("DecimalIds")},
	};
	for (int i = 0; i < countof(Settings); i++)
	{
		*Settings[i].pbVal = settings.Get(0, Settings[i].psName, *Settings[i].pbVal);
		// случай первого запуска
		settings.Set(0, Settings[i].psName, *Settings[i].pbVal);
	}
#else
	if (!gsRootKey) return;

	bool b;
	HKEY h;
	DWORD nSize, nType;
	if (!RegCreateKeyEx(HKEY_CURRENT_USER, gsRootKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &h, NULL))
	{
		// gbUseMenu, gbUseEnter, gbUseCtrlPgDn, gbLibRegUnreg, gbLibLoadUnload;
		struct tag_Settings {
			bool* pbVal; LPCTSTR psName;
		};
		struct tag_Settings Settings[] =
		{
			{&gbUseMenu, _T("UseMenu")},
			{&gbUseEnter, _T("UseEnter")},
			{&gbUseCtrlPgDn, _T("UseCtrlPgDn")},
			{&gbLibRegUnreg, _T("LibRegUnreg")},
			{&gbLibLoadUnload, _T("LibLoadUnload")},
			{&gbUseUndecorate, _T("UseUndecorate")},
			{&gbDecimalIds, _T("DecimalIds")},
		};
		for (int i = 0; i < countof(Settings); i++)
		{
			if (RegQueryValueEx(h, Settings[i].psName, NULL, &nType, (LPBYTE)&b, &(nSize=1)))
				RegSetValueEx(h, Settings[i].psName, 0, REG_BINARY, (LPBYTE)Settings[i].pbVal, 1);
			else
				*Settings[i].pbVal = b;
		}
		RegCloseKey(h);
	}
#endif
}

bool IsKeyPressed(WORD vk)
{
	USHORT st = GetKeyState(vk);
	if ((st & 0x8000))
		return true;

	if (vk == VK_CONTROL)
	{
		// что-то в ConEmu GetKeyState иногда ничего не дает? возвращается 0
		if (hConEmuWnd && !IsWindow(hConEmuWnd))
		{
			hConEmuWnd = NULL;
		}
		if (!hConEmuWnd)
		{
			hConEmuWnd = GetConEmuHwnd();
		}
		if (hConEmuWnd)
		{
			if (!hConEmuCtrlPressed)
			{
				TCHAR szName[64]; DWORD dwPID = GetCurrentProcessId();
				wsprintf(szName, CEKEYEVENT_CTRL, dwPID);
				hConEmuCtrlPressed = OpenEvent(SYNCHRONIZE, FALSE, szName);
			}
			
			if (hConEmuCtrlPressed)
			{
				DWORD dwWait = WAIT_TIMEOUT;
				if (vk == VK_CONTROL)
					dwWait = WaitForSingleObject(hConEmuCtrlPressed, 0);
				return (dwWait == WAIT_OBJECT_0);
			}
		}
	}
	return false;
}
