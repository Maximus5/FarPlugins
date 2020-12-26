#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <tchar.h>
#include <strsafe.h>
#include "plugin.hpp"
#include "memory.h"
#define realloc my_realloc
#ifndef nullptr
#define nullptr NULL
#endif
#ifndef _ASSERTE
#define _ASSERTE(x)
#endif
#include "DlgBuilder.hpp"
#include "eplugin.cpp"
#include "farcolor.hpp"
#include "farkeys.hpp"
#include "farlang.h"
#include "registry.cpp"
#include "uninstall.hpp"

#ifdef FARAPI18
#  define SetStartupInfo SetStartupInfoW
#  define GetPluginInfo GetPluginInfoW
#  define OpenPlugin OpenPluginW
#  define Configure ConfigureW
#endif

#ifdef FARAPI17
int WINAPI GetMinFarVersion(void)
{
	return MAKEFARVERSION(1,75,2555);
}
#endif
#ifdef FARAPI18
int WINAPI GetMinFarVersion(void)
{
	return FARMANAGERVERSION;
}
int WINAPI GetMinFarVersionW(void)
{
	return FARMANAGERVERSION;
}
#endif

void WINAPI SetStartupInfo(const struct PluginStartupInfo *psInfo)
{
	Info = *psInfo;
	FSF = *psInfo->FSF;
	Info.FSF = &FSF;
	InitHeap();
	StringCchCopy(PluginRootKey,ARRAYSIZE(PluginRootKey),Info.RootKey);
	StringCchCat(PluginRootKey,ARRAYSIZE(PluginRootKey),_T("\\UnInstall"));
	ReadRegistry();
}

void WINAPI GetPluginInfo(struct PluginInfo *Info)
{
	static const TCHAR *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetMsg(MPlugIn);
	Info -> StructSize = sizeof(*Info);
	Info -> PluginMenuStrings = PluginMenuStrings;
	Info -> PluginConfigStrings = PluginMenuStrings;

	if(Opt.WhereWork & 2)
		Info -> Flags |= PF_EDITOR;

	if(Opt.WhereWork & 1)
		Info -> Flags |= PF_VIEWER;

	Info -> PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
	Info -> PluginConfigStringsNumber = ARRAYSIZE(PluginMenuStrings);
}

void ResizeDialog(HANDLE hDlg)
{
	CONSOLE_SCREEN_BUFFER_INFO con_info;
	GetConsoleScreenBufferInfo(hStdout, &con_info);
	unsigned con_sx = con_info.srWindow.Right - con_info.srWindow.Left + 1;
	int max_items = con_info.srWindow.Bottom - con_info.srWindow.Top + 1 - 7;
	int s = ((ListSize>0) && (ListSize<max_items) ? ListSize : (ListSize>0 ? max_items : 0));
	SMALL_RECT NewPos = { 2, 1, con_sx - 7, s + 2 };
	SMALL_RECT OldPos;
	Info.SendDlgMessage(hDlg,DM_GETITEMPOSITION,LIST_BOX,reinterpret_cast<LONG_PTR>(&OldPos));

	if(NewPos.Right!=OldPos.Right || NewPos.Bottom!=OldPos.Bottom)
	{
		COORD coord;
		coord.X = con_sx - 4;
		coord.Y = s + 4;
		Info.SendDlgMessage(hDlg,DM_RESIZEDIALOG,0,reinterpret_cast<LONG_PTR>(&coord));
		coord.X = -1;
		coord.Y = -1;
		Info.SendDlgMessage(hDlg,DM_MOVEDIALOG,TRUE,reinterpret_cast<LONG_PTR>(&coord));
		Info.SendDlgMessage(hDlg,DM_SETITEMPOSITION,LIST_BOX,reinterpret_cast<LONG_PTR>(&NewPos));
	}
}

static LONG_PTR WINAPI DlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	static TCHAR Filter[MAX_PATH];
	static TCHAR spFilter[MAX_PATH];
	static FarListTitles ListTitle;

	switch(Msg)
	{
		case DN_RESIZECONSOLE:
		{
			Info.SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
			ResizeDialog(hDlg);
			Info.SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
		}
		return TRUE;
		case DMU_UPDATE:
		{
			int OldPos = static_cast<int>(Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,LIST_BOX,0));

			if(Param1)
				UpDateInfo();

			ListSize = 0;
			int NewPos = -1;

			if(OldPos >= 0 && OldPos < nCount)
			{
				if(!*Filter || strstri(p[OldPos].Keys[DisplayName],Filter))  //без учета регистра в OEM кодировке
					NewPos = OldPos;
			}

			for(int i = 0; i < nCount; i++)
			{
				const TCHAR* DispName = p[i].Keys[DisplayName], *Find;

				if(*Filter)
					Find = strstri(DispName,Filter);
				else
					Find = DispName;

				if(Find != nullptr)  //без учета регистра в OEM кодировке
				{
					FLI[i].Flags &= ~LIF_HIDDEN;

					if(Param2 && (i == OldPos))
					{
						if(FLI[i].Flags & LIF_CHECKED)
						{
							FLI[i].Flags &= ~LIF_CHECKED;
						}
						else
						{
							FLI[i].Flags |= LIF_CHECKED;
						}
					}

					//без учета регистра - а кодировка ANSI
					if(NewPos == -1 && Find == DispName)
						NewPos = i;

					ListSize++;
				}
				else
					FLI[i].Flags |= LIF_HIDDEN;
			}

			if(Param1 == 0 && Param2)
			{
				// Снятие или установка пометки (Ins)
				if(Param2 == 1)
				{
					for(int i = (OldPos+1); i < nCount; i++)
					{
						if(!(FLI[i].Flags & LIF_HIDDEN))
						{
							OldPos = i; break;
						}
					}

					NewPos = OldPos;
				}
				// Снятие или установка пометки (RClick)
				else if(Param2 == 2)
				{
					NewPos = OldPos;
				}
			}
			else if(NewPos == -1)
			{
				NewPos = OldPos;
			}

			Info.SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
			Info.SendDlgMessage(hDlg,DM_LISTSET,LIST_BOX,reinterpret_cast<LONG_PTR>(&FL));
			StringCchPrintf(spFilter,ARRAYSIZE(spFilter), GetMsg(MFilter),Filter,ListSize,nCount);
			ListTitle.Title = spFilter;
			ListTitle.TitleLen = lstrlen(spFilter);
			Info.SendDlgMessage(hDlg,DM_LISTSETTITLES,LIST_BOX,reinterpret_cast<LONG_PTR>(&ListTitle));
			ResizeDialog(hDlg);
			struct FarListPos FLP;
			FLP.SelectPos = NewPos;
			FLP.TopPos = -1;
			Info.SendDlgMessage(hDlg,DM_LISTSETCURPOS,LIST_BOX,reinterpret_cast<LONG_PTR>(&FLP));
			Info.SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
		}
		break;
		case DN_INITDIALOG:
		{
			StringCchCopy(Filter,ARRAYSIZE(Filter),_T(""));
			ListTitle.Bottom = const_cast<TCHAR*>(GetMsg(MBottomLine));
			ListTitle.BottomLen = lstrlen(GetMsg(MBottomLine));
			//подстраиваемся под размеры консоли
			Info.SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
			ResizeDialog(hDlg);
			Info.SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
			//заполняем диалог
			Info.SendDlgMessage(hDlg,DMU_UPDATE,1,0);
		}
		break;
		case DN_MOUSECLICK:
		{
			if(Param1 == LIST_BOX)
			{
				MOUSE_EVENT_RECORD *mer = (MOUSE_EVENT_RECORD *)Param2;

				if(mer->dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
				{
					// find list on-screen coords (excluding frame and border)
					SMALL_RECT list_rect;
					Info.SendDlgMessage(hDlg, DM_GETDLGRECT, 0, reinterpret_cast<LONG_PTR>(&list_rect));
					list_rect.Left += 2;
					list_rect.Top += 1;
					list_rect.Right -= 2;
					list_rect.Bottom -= 1;

					if((mer->dwEventFlags == 0) && (mer->dwMousePosition.X > list_rect.Left) && (mer->dwMousePosition.X < list_rect.Right) && (mer->dwMousePosition.Y > list_rect.Top) && (mer->dwMousePosition.Y < list_rect.Bottom))
					{
						DlgProc(hDlg, DN_KEY, LIST_BOX, KEY_ENTER);
						return TRUE;
					}

					// pass message to scrollbar if needed
					if((mer->dwMousePosition.X == list_rect.Right) && (mer->dwMousePosition.Y > list_rect.Top) && (mer->dwMousePosition.Y < list_rect.Bottom)) return FALSE;

					return TRUE;
				}
				else if(mer->dwButtonState == RIGHTMOST_BUTTON_PRESSED)
				{
					Info.SendDlgMessage(hDlg,DMU_UPDATE,0,2);
					return TRUE;
				}
			}
		}
		break;
		case DN_KEY:

			switch(Param2)
			{
				case KEY_F8:
				{
					if(ListSize)
					{
						TCHAR DlgText[MAX_PATH + 200];
						StringCchPrintf(DlgText, ARRAYSIZE(DlgText), GetMsg(MConfirm), p[Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,LIST_BOX,NULL)].Keys[DisplayName]);

						if(EMessage((const TCHAR * const *) DlgText, 0, 2) == 0)
						{
							if(!DeleteEntry(static_cast<int>(Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,LIST_BOX,NULL))))
								DrawMessage(FMSG_WARNING, 1, "%s",GetMsg(MPlugIn),GetMsg(MDelRegErr),GetMsg(MBtnOk),NULL);

							Info.SendDlgMessage(hDlg,DMU_UPDATE,1,0);
						}
					}
				}
				return TRUE;
				case(KEY_F9|KEY_SHIFT|KEY_ALT):
				case(KEY_F9):
				{
					Configure(0);
				}
				return TRUE;
				case KEY_CTRLR:
				{
					Info.SendDlgMessage(hDlg,DMU_UPDATE,1,0);
				}
				return TRUE;
				case KEY_INS:
				{
					Info.SendDlgMessage(hDlg,DMU_UPDATE,0,1);
				}
				return TRUE;
				case KEY_ENTER:
				case KEY_SHIFTENTER:
				{
					if(ListSize)
					{
						int liChanged = 0;
						int liSelected = 0, liFirst = -1;

						for(int i = 0; i < nCount; i++)
						{
							if(FLI[i].Flags & LIF_CHECKED)
							{
								if(liFirst == -1)
									liFirst = i;

								liSelected++;
							}
						}

						if(liSelected <= 1)
						{
							int liAction = (Param2 == KEY_ENTER) ? Opt.EnterAction : Opt.ShiftEnterAction;
							int pos = (liFirst == -1) ? static_cast<int>(Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,LIST_BOX,NULL)) : liFirst;
							liChanged = ExecuteEntry(pos, liAction, (Opt.RunLowPriority!=0));
						}
						else
						{
							int liAction = (Param2 == KEY_ENTER) ? Opt.EnterAction : Opt.ShiftEnterAction;
							bool LowPriority = (Opt.RunLowPriority!=0);

							// Обязательно ожидание - два инсталлятора сразу недопускаются
							if(liAction == Action_Menu)
							{
								if(EntryMenu(0, liAction, LowPriority, liSelected) < 0)
									return TRUE;
							}
							else if(liAction == Action_Uninstall)
								liAction = Action_UninstallWait;
							else if(liAction == Action_Modify)
								liAction = Action_ModifyWait;
							else if(liAction == Action_Repair)
								liAction = Action_RepairWait;

							for(int pos = 0; pos < nCount; pos++)
							{
								if(!(FLI[pos].Flags & LIF_CHECKED))
									continue;

								struct FarListPos FLP;
								FLP.SelectPos = pos;
								FLP.TopPos = -1;
								Info.SendDlgMessage(hDlg,DM_LISTSETCURPOS,LIST_BOX,reinterpret_cast<LONG_PTR>(&FLP));
								int li = ExecuteEntry(pos, liAction, LowPriority);

								if(li == -1)
									break; // отмена

								if(li == 1)
									liChanged = 1;
							}
						}

						if(liChanged == 1)
						{
							Info.SendDlgMessage(hDlg,DMU_UPDATE,1,0);
						}
					}
				}
				return TRUE;
				case KEY_SHIFTINS:
				case KEY_CTRLV:
				{
					TCHAR * bufP = FSF.PasteFromClipboard();
					static TCHAR bufData[MAX_PATH];

					if(bufP)
					{
						StringCchCopy(bufData,ARRAYSIZE(bufData),bufP);
						FSF.DeleteBuffer(bufP);
						unQuote(bufData);
						FSF.LStrlwr(bufData);

						for(int i = lstrlen(bufData); i >= 1; i--)
							for(int j = 0; j < nCount; j++)
								if(strnstri(p[j].Keys[DisplayName],bufData,i))
								{
									lstrcpyn(Filter,bufData,i+1);
									Info.SendDlgMessage(hDlg,DMU_UPDATE,0,0);
									return TRUE;
								}
					}
				}
				return TRUE;
				case KEY_DEL:
				{
					if(lstrlen(Filter) > 0)
					{
						StringCchCopy(Filter,ARRAYSIZE(Filter),_T(""));
						Info.SendDlgMessage(hDlg,DMU_UPDATE,0,0);
					}
				}
				return TRUE;
				case KEY_F3:
				case KEY_F4:
				{
					if(ListSize)
					{
						DisplayEntry(static_cast<int>(Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,LIST_BOX,NULL)));
						Info.SendDlgMessage(hDlg,DM_REDRAW,NULL,NULL);
					}
				}
				return TRUE;
				case KEY_F2:
				{
					Opt.SortByDate = !Opt.SortByDate;
					Info.SendDlgMessage(hDlg,DMU_UPDATE,1,0);
				}
				return TRUE;
				case KEY_BS:
				{
					if(lstrlen(Filter))
					{
						Filter[lstrlen(Filter)-1] = '\0';
						Info.SendDlgMessage(hDlg,DMU_UPDATE,0,0);
					}
				}
				return TRUE;
				default:
				{
					if(Param2 >= KEY_SPACE && Param2 < KEY_FKEY_BEGIN)
					{
						struct FarListInfo ListInfo;
						Info.SendDlgMessage(hDlg,DM_LISTINFO,LIST_BOX,reinterpret_cast<LONG_PTR>(&ListInfo));

						if((lstrlen(Filter) < sizeof(Filter)) && ListInfo.ItemsNumber)
						{
							int filterL = lstrlen(Filter);
							Filter[filterL] = FSF.LLower(static_cast<unsigned>(Param2));
							Filter[filterL+1] = '\0';
							Info.SendDlgMessage(hDlg,DMU_UPDATE,0,0);
							return TRUE;
						}
					}
				}
			}

			return FALSE;
		case DN_CTLCOLORDIALOG:
			return Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void *)COL_MENUTEXT);
		case DN_CTLCOLORDLGLIST:

			if(Param1 == LIST_BOX)
			{
				FarListColors *Colors = (FarListColors *)Param2;
				int ColorIndex[] = { COL_MENUBOX, COL_MENUBOX, COL_MENUTITLE, COL_MENUTEXT, COL_MENUHIGHLIGHT, COL_MENUBOX, COL_MENUSELECTEDTEXT, COL_MENUSELECTEDHIGHLIGHT, COL_MENUSCROLLBAR, COL_MENUDISABLEDTEXT, COL_MENUARROWS, COL_MENUARROWSSELECTED, COL_MENUARROWSDISABLED };
				int Count = ARRAYSIZE(ColorIndex);

				if(Count > Colors->ColorCount)
					Count = Colors->ColorCount;

				for(int i = 0; i < Count; i++)
					Colors->Colors[i] = static_cast<BYTE>(Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, reinterpret_cast<void *>(ColorIndex[i])));

				return TRUE;
			}

			break;
	}

	return Info.DefDlgProc(hDlg,Msg,Param1,Param2);
}

HANDLE WINAPI OpenPlugin(int /*OpenFrom*/, INT_PTR /*Item*/)
{
	ReadRegistry();
	struct FarDialogItem DialogItems[1];
	ZeroMemory(DialogItems, sizeof(DialogItems));
	p = NULL;
	FLI = NULL;
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	UpDateInfo();
	DialogItems[0].Type = DI_LISTBOX;
	DialogItems[0].Flags = DIF_LISTNOAMPERSAND;
	DialogItems[0].X1 = 2;
	DialogItems[0].Y1 = 1;
#ifdef FARAPI17
	Info.DialogEx(Info.ModuleNumber,-1,-1,0,0,"Contents",DialogItems,ARRAYSIZE(DialogItems),0,0,DlgProc,0);
#endif
#ifdef FARAPI18
	HANDLE h_dlg = Info.DialogInit(Info.ModuleNumber,-1,-1,0,0,L"Contents",DialogItems,ARRAYSIZE(DialogItems),0,0,DlgProc,0);

	if(h_dlg != INVALID_HANDLE_VALUE)
	{
		Info.DialogRun(h_dlg);
		Info.DialogFree(h_dlg);
	}

#endif
	FLI = (FarListItem *) realloc(FLI, 0);
	p = (KeyInfo *) realloc(p, 0);
	return INVALID_HANDLE_VALUE;
}

int WINAPI Configure(int ItemNumber)
{
	PluginDialogBuilder Config(Info, MPlugIn, _T("Configuration"));
	FarDialogItem *p1, *p2;
	BOOL bShowInViewer = (Opt.WhereWork & 1) != 0;
	BOOL bShowInEditor = (Opt.WhereWork & 2) != 0;
	//BOOL bEnterWaitCompletion = (Opt.EnterFunction != 0);
	BOOL bUseElevation = (Opt.UseElevation != 0);
	BOOL bLowPriority = (Opt.RunLowPriority != 0);
	BOOL bForceMsiUse = (Opt.ForceMsiUse != 0);
	Config.AddCheckbox(MShowInEditor, &bShowInEditor);
	Config.AddCheckbox(MShowInViewer, &bShowInViewer);
	//Config.AddCheckbox(MEnterWaitCompletion, &bEnterWaitCompletion);
	Config.AddCheckbox(MUseElevation, &bUseElevation);
	Config.AddCheckbox(MLowPriority, &bUseElevation);
	Config.AddCheckbox(MForceMsiUse, &bForceMsiUse);
	Config.AddSeparator();
	FarList AEnter, AShiftEnter;
	AEnter.ItemsNumber = AShiftEnter.ItemsNumber = 7;
	AEnter.Items = (FarListItem*)calloc(AEnter.ItemsNumber,sizeof(FarListItem));
	AShiftEnter.Items = (FarListItem*)calloc(AEnter.ItemsNumber,sizeof(FarListItem));

	for(int i = 0; i < AEnter.ItemsNumber; i++)
	{
#ifdef FARAPI18
		AEnter.Items[i].Text = GetMsg(MActionUninstallWait+i);
		AShiftEnter.Items[i].Text = AEnter.Items[i].Text;
#else
		StringCchCopy(AEnter.Items[i].Text,ARRAYSIZE(AEnter.Items[i].Text),GetMsg(MActionUninstallWait+i));
		StringCchCopy(AShiftEnter.Items[i].Text,ARRAYSIZE(AShiftEnter.Items[i].Text),AEnter.Items[i].Text);
#endif
	}

	p1 = Config.AddText(MEnterAction); p2 = Config.AddComboBox(23, &AEnter, &Opt.EnterAction);
	Config.MoveItemAfter(p1,p2);
	p1 = Config.AddText(MShiftEnterAction); p2 = Config.AddComboBox(23, &AShiftEnter, &Opt.ShiftEnterAction);
	Config.MoveItemAfter(p1,p2);
	Config.AddOKCancel(MBtnOk, MBtnCancel);

	if(Config.ShowDialog())
	{
		Opt.WhereWork = (bShowInViewer ? 1 : 0) | (bShowInEditor ? 2 : 0);
		//Opt.EnterFunction = bEnterWaitCompletion;
		Opt.UseElevation = bUseElevation;
		Opt.RunLowPriority = bLowPriority;
		Opt.ForceMsiUse = bForceMsiUse;
		SetRegKey(HKCU,_T(""),_T("WhereWork"),(DWORD) Opt.WhereWork);
		SetRegKey(HKCU,_T(""),_T("EnterAction"),(DWORD) Opt.EnterAction);
		SetRegKey(HKCU,_T(""),_T("ShiftEnterAction"),(DWORD) Opt.ShiftEnterAction);
		SetRegKey(HKCU,_T(""),_T("UseElevation"),(DWORD) Opt.UseElevation);
		SetRegKey(HKCU,_T(""),_T("RunLowPriority"),(DWORD) Opt.RunLowPriority);
		SetRegKey(HKCU,_T(""),_T("ForceMsiUse"),(DWORD) Opt.ForceMsiUse);
	}

	return FALSE;
}
