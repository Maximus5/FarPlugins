
#pragma once

class CScreenRestore;

extern CScreenRestore *gpAction/* = NULL*/;

extern struct PluginStartupInfo psi;

#if FAR_UNICODE>=1906
// Plugin GUID
extern GUID guid_PluginGuid;
#define PluginNumber &guid_PluginGuid
#define _PluginNumber(g) &guid_PluginGuid, &g
#define psiControl psi.PanelControl
#else
#define PluginNumber psi.ModuleNumber
#define _PluginNumber(g) PluginNumber
#define psiControl psi.Control
#endif

#if defined(_UNICODE) && (FAR_UNICODE>=2800)
	#include "far3l/PluginSettings.hpp"
#endif

#if defined(_UNICODE) && (FAR_UNICODE>=1988)
	#define PanelItemFileNamePtr(p) (p).FileName
	#define PanelItemAltNamePtr(p) (p).AlternateFileName
	#define PanelItemAttributes(p) (p).FileAttributes
	#define PanelItemCreation(p) (p).CreationTime
	#define PanelItemAccess(p) (p).LastAccessTime
	#define PanelItemWrite(p) (p).LastWriteTime
	#define PanelItemChange(p) (p).ChangeTime
	#define PanelItemFileSize(p) (p).FileSize
	#define PanelItemPackSize(p) (p).FileSize // There is NO PackSize in latest Far
	#define PanelItemUserData(p) (p).UserData.Data
	#define MenuItemIsSeparator(p) (((p).Flags & MIF_SEPARATOR) == MIF_SEPARATOR)
	#define MenuItemSetSeparator(p) (p).Flags |= MIF_SEPARATOR
	#define MenuItemIsSelected(p) (((p).Flags & MIF_SELECTED) == MIF_SELECTED)
	#define MenuItemSetSelected(p) (p).Flags |= MIF_SELECTED
	//
	#define SETMENUTEXT(itm,txt) itm.Text = txt;
	#define FARDLGPARM void*
	#define FARDLGRET intptr_t
	#define FARINT intptr_t
	#define FARPTR void*
	#define USERDATAPTR void*
	#define F757NA 0, 
	#define FADV1988 0,
	#define _GetCheck(i) (int)psi.SendDlgMessage(hDlg,DM_GETCHECK,i,0)
	#define GetDataPtr(i) ((const TCHAR *)psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
	#define DlgGetTextLength(hDlg,CtrlId) psi.SendDlgMessage(hDlg,DM_GETTEXT,CtrlId,0)
	#define SETTEXT(itm,txt) itm.PtrData = txt
	#define SETTEXTPRINT(itm,fmt,arg) wsprintf(pszBuf, fmt, arg); SETTEXT(itm,pszBuf); pszBuf+=lstrlen(pszBuf)+2;
	#define FILENAMEPTR PanelItemFileNamePtr
#elif defined(_UNICODE)
	#define PanelItemFileNamePtr(p) (p).FindData.lpwszFileName
	#define PanelItemAltNamePtr(p) (p).FindData.lpwszAlternateFileName
	#define PanelItemAttributes(p) (p).FindData.dwFileAttributes
	#define PanelItemCreation(p) (p).FindData.ftCreationTime
	#define PanelItemAccess(p) (p).FindData.ftLastAccessTime
	#define PanelItemWrite(p) (p).FindData.ftLastWriteTime
	#define PanelItemChange(p) (p).FindData.ftLastWriteTime
	#define PanelItemFileSize(p) (p).FindData.nFileSize
	#define PanelItemPackSize(p) (p).FindData.nPackSize
	#define PanelItemUserData(p) (p).UserData
	#define MenuItemIsSeparator(p) (p).Separator
	#define MenuItemSetSeparator(p) (p).Separator = TRUE
	#define MenuItemIsSelected(p) (p).Selected
	#define MenuItemSetSelected(p) (p).Selected = TRUE
	//
	#define SETMENUTEXT(itm,txt) itm.Text = txt;
	#define FARDLGPARM LPARAM
	#define FARDLGRET LONG_PTR
	#define FARINT int
	#define FARPTR LONG_PTR
	#define USERDATAPTR DWORD_PTR
	#define F757NA 0, (LONG_PTR)
	#define FADV1988
	#define _GetCheck(i) (int)psi.SendDlgMessage(hDlg,DM_GETCHECK,i,0)
	#define GetDataPtr(i) ((const TCHAR *)psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
	#define DlgGetTextLength(hDlg,CtrlId) psi.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, CtrlId, 0)
	#define SETTEXT(itm,txt) itm.PtrData = txt
	#define SETTEXTPRINT(itm,fmt,arg) wsprintf(pszBuf, fmt, arg); SETTEXT(itm,pszBuf); pszBuf+=lstrlen(pszBuf)+2;
	#define FILENAMEPTR PanelItemFileNamePtr
#else
	#define PanelItemFileNamePtr(p) (p).FindData.cFileName
	#define PanelItemAltNamePtr(p) (p).FindData.cAlternateFileName
	#define PanelItemAttributes(p) (p).FindData.dwFileAttributes
	#define PanelItemFlags(p) (p).FileAttributes
	#define PanelItemCreation(p) (p).FindData.ftCreationTime
	#define PanelItemAccess(p) (p).FindData.ftLastAccessTime
	#define PanelItemWrite(p) (p).FindData.ftLastWriteTime
	#define PanelItemChange(p) (p).FindData.ftLastWriteTime
	#define PanelItemFileSize(p) (p).FindData.nFileSizeLow // только DWORD !!!
	#define PanelItemPackSize(p) (p).PackSize // только DWORD !!!
	#define PanelItemUserData(p) (p).UserData
	#define MenuItemIsSeparator(p) (p).Separator
	#define MenuItemSetSeparator(p) (p).Separator = TRUE
	#define MenuItemIsSelected(p) (p).Selected
	#define MenuItemSetSelected(p) (p).Selected = TRUE
	//
    #define SETMENUTEXT(itm,txt) lstrcpy(itm.Text, txt);
	#define FARDLGPARM LPARAM
	#define FARDLGRET LONG_PTR
    #define FARINT int
	#define FARPTR void*
	#define USERDATAPTR DWORD_PTR
    #define F757NA (void*)
	#define FADV1988
    #define _GetCheck(i) items[i].Selected
    #define GetDataPtr(i) items[i].Ptr.PtrData
	#define DlgGetTextLength(hDlg,CtrlId) psi.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, CtrlId, 0)
    #define SETTEXT(itm,txt) lstrcpy(itm.Data, txt)
    #define SETTEXTPRINT(itm,fmt,arg) wsprintf(itm.Data, fmt, arg)
	#define FILENAMEPTR(p) (p).FindData.cFileName
#endif

inline INT_PTR EditCtrl(int Cmd, void* Parm, INT_PTR cbSize = 0)
{
	INT_PTR iRc;
	#if FAR_UNICODE>=1906
	iRc = psi.EditorControl(-1, (EDITOR_CONTROL_COMMANDS)Cmd, cbSize, Parm);
	#else
	iRc = psi.EditorControl(Cmd, Parm);
	#endif
	return iRc;
}

inline TCHAR* DlgGetText(HANDLE hDlg, int CtrlId)
{
	INT_PTR nSrcLen = DlgGetTextLength(hDlg, CtrlId);
	TCHAR* pszSrc = NULL;
	if (nSrcLen > 0)
	{
		pszSrc = (wchar_t*)calloc(nSrcLen+1, sizeof(*pszSrc));
		#if FAR_UNICODE>=2400
		FarDialogItemData did = {sizeof(did), nSrcLen, pszSrc};
		psi.SendDlgMessage(hDlg, DM_GETTEXT, CtrlId, &did);
		#else
		psi.SendDlgMessage(hDlg, DM_GETTEXTPTR, CtrlId, (FARDLGPARM)pszSrc);
		#endif
	}
	return pszSrc;
}

#if 0
struct CPluginAnalyse
{
public:
	TCHAR          *FileName;
	LPBYTE          Buffer;
	size_t          BufferSize;
	OPERATION_MODES OpMode;

public:
	static CPluginAnalyse* Create(const AnalyseInfo *Info)
	{
		int cchNameMax = Info->FileName ? (lstrlen(Info->FileName)+1) : 0;
		INT_PTR nAllSize = sizeof(CPluginAnalyse)
			+ cchNameMax*sizeof(*Info->FileName)
			+ Info->BufferSize;
		CPluginAnalyse* pRc = (CPluginAnalyse*)malloc(nAllSize);
		if (!pRc)
			return NULL;
		LPBYTE ptr = (LPBYTE)(pRc+1);
		if (Info->Buffer)
		{
			pRc->Buffer = ptr;
			pRc->BufferSize = Info->BufferSize;
			memmove(ptr, Info->Buffer, Info->BufferSize);
			ptr += Info->BufferSize;
		}
		else
		{
			pRc->Buffer = NULL;
			pRc->BufferSize = 0;
		}
		if (Info->FileName)
		{
			pRc->FileName = (TCHAR*)ptr;
			lstrcpyn(pRc->FileName, Info->FileName, cchNameMax);
		}
		else
		{
			pRc->FileName = NULL;
		}
		pRc->OpMode = Info->OpMode;
		return pRc;
	};
};
#endif

class CScreenRestore
{
protected:
	HANDLE hScreen;
	const TCHAR *pszTitle, *pszMsg;
	CScreenRestore *pLastAction;
public:
	void Restore()
	{
		if (hScreen)
		{
			psi.RestoreScreen(hScreen);
			psi.Text(0,0,0,0);
			hScreen = NULL;
		}
	}
	void Save()
	{
		if (hScreen) Restore();
		// Вернет NULL, если FAR в режиме DisableScreenOutput
		hScreen = psi.SaveScreen(0,0,-1,-1);
	}
	void Message(const TCHAR* aszMsg, BOOL abForce=TRUE)
	{
		if (abForce || !hScreen)
		{
			if (hScreen) Restore();
			Save();
		}

		_ASSERTE(aszMsg!=NULL);
		pszMsg = aszMsg;

		// Только если разрешен вывод на экран!
		if (hScreen != NULL)
		{		
			const TCHAR *MsgItems[]={pszTitle,aszMsg};
			psi.Message(_PluginNumber(guid_PluginGuid),0,NULL,MsgItems,sizeof(MsgItems)/sizeof(MsgItems[0]),0);
		}
	}
protected:
	void RefreshMessage() {
		if (pszMsg) Message(pszMsg, TRUE);
	}
public:
	CScreenRestore(const TCHAR* aszMsg, const TCHAR* aszTitle=NULL) : hScreen(NULL)
	{
		pLastAction = gpAction; //nLastTick = -1;
		pszTitle = aszTitle ? aszTitle : _T("");
		pszMsg = NULL;

		if (aszMsg == NULL) {
			Save();
		} else {
			Message(aszMsg, TRUE);			
		}

		gpAction = this;
	};
	virtual ~CScreenRestore()
	{
		Restore();
		if (gpAction == this) {
			if (pLastAction != this) {
				gpAction = pLastAction;
				if (gpAction)
					gpAction->RefreshMessage();
			} else {
				_ASSERTE(pLastAction!=this);
				gpAction = NULL;
			}
		} else {
			_ASSERTE(gpAction == this);
		}
	};
};
