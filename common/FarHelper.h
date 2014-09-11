
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

#if defined(_UNICODE) && (FAR_UNICODE>=1988)
	#define PanelItemFileNamePtr(p) (p).FileName
	#define PanelItemAltNamePtr(p) (p).AlternateFileName
	#define PanelItemAttributes(p) (p).FileAttributes
	#define PanelItemCreation(p) (p).CreationTime
	#define PanelItemAccess(p) (p).LastAccessTime
	#define PanelItemWrite(p) (p).LastWriteTime
	#define PanelItemChange(p) (p).ChangeTime
	#define PanelItemFileSize(p) (p).FileSize
	#define PanelItemPackSize(p) (p).PackSize
	#define PanelItemUserData(p) (p).UserData.Data
	#define MenuItemIsSeparator(p) (((p).Flags & MIF_SEPARATOR) == MIF_SEPARATOR)
	#define MenuItemSetSeparator(p) (p).Flags |= MIF_SEPARATOR
	#define MenuItemIsSelected(p) (((p).Flags & MIF_SELECTED) == MIF_SELECTED)
	#define MenuItemSetSelected(p) (p).Flags |= MIF_SELECTED
	//
	#define SETMENUTEXT(itm,txt) itm.Text = txt;
	#define FARINT intptr_t
	#define FARPTR void*
	#define USERDATAPTR void*
	#define F757NA 0, 
	#define FADV1988 0,
	#define _GetCheck(i) (int)psi.SendDlgMessage(hDlg,DM_GETCHECK,i,0)
	#define GetDataPtr(i) ((const TCHAR *)psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
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
	#define FARINT int
	#define FARPTR LONG_PTR
	#define USERDATAPTR DWORD_PTR
	#define F757NA 0, (LONG_PTR)
	#define FADV1988
	#define _GetCheck(i) (int)psi.SendDlgMessage(hDlg,DM_GETCHECK,i,0)
	#define GetDataPtr(i) ((const TCHAR *)psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
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
    #define FARINT int
	#define FARPTR void*
	#define USERDATAPTR DWORD_PTR
    #define F757NA (void*)
	#define FADV1988
    #define _GetCheck(i) items[i].Selected
    #define GetDataPtr(i) items[i].Ptr.PtrData
    #define SETTEXT(itm,txt) lstrcpy(itm.Data, txt)
    #define SETTEXTPRINT(itm,fmt,arg) wsprintf(itm.Data, fmt, arg)
	#define FILENAMEPTR(p) (p).FindData.cFileName
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
