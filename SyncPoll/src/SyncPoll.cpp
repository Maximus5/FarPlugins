
/*
Copyright (c) 2014 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "header.h"
#include <ctype.h>
#include "version.h"

// {6197AF6C-4755-49A5-84A1-8F227BF4790E}
static GUID guid_PluginGuid =
{ 0x6197af6c, 0x4755, 0x49a5, { 0x84, 0xa1, 0x8f, 0x22, 0x7b, 0xf4, 0x79, 0x0e } };

PluginStartupInfo psi;
FarStandardFunctions fsf;

HANDLE hThread = NULL;
DWORD nThreadPID = 0;
HANDLE hExit = NULL;

DWORD WINAPI PollingThread(LPVOID lpThreadParameter)
{
	while (WaitForSingleObject(hExit, 100))
	{
		psi.AdvControl(&guid_PluginGuid, ACTL_SYNCHRO, 0, NULL);
	}
	return 0;
}

void StartThread()
{
	if (hThread)
		return;
	if (!hExit)
		hExit = CreateEvent(NULL, FALSE, FALSE, NULL);
	hThread = CreateThread(NULL, 0, PollingThread, NULL, 0, &nThreadPID);
	_ASSERTE(hThread!=NULL);
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->MinFarVersion = /*FARMANAGERVERSION*/
		MAKEFARVERSION(
			FARMANAGERVERSION_MAJOR,
			FARMANAGERVERSION_MINOR,
			FARMANAGERVERSION_REVISION,
			FARMANAGERVERSION_BUILD,
			FARMANAGERVERSION_STAGE);
	
	// Build: YYMMDDX (YY - две цифры года, MM - мес€ц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"SyncPoll";
	Info->Description = L"SyncPoll";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *xInfo)
{
	psi = *xInfo;
	fsf = *xInfo->FSF;
	psi.FSF = &fsf;

	StartThread();
}

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
	pi->Flags = PF_PRELOAD;
}

LONG gnInCall = 0;

HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	InterlockedDecrement(&gnInCall);
	OpenMacroInfo* pm;
	if (Info->OpenFrom == OPEN_FROMMACRO)
	{
		pm = (OpenMacroInfo*)Info->Data;
	}
	return NULL;
}

intptr_t WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo *Info)
{
	if (Info->Event != SE_COMMONSYNCHRO)
		return 0;
	if (gnInCall > 0)
		return 0;
#if 1
	OutputDebugString(L"Posting macro\n");
	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	//mcr.SequenceText = L"i = 0; if APanel.Plugin then i=i+1; end; if PPanel.Plugin then i=i+2; end; Plugin.Call(\"6197AF6C-4755-49A5-84A1-8F227BF4790E\",i)";
	mcr.SequenceText = L"Plugin.Call(\"6197AF6C-4755-49A5-84A1-8F227BF4790E\",APanel.Plugin and \"\" or APanel.Path0,PPanel.Plugin and \"\" or PPanel.Path0)";
	INT_PTR i = psi.MacroControl(&guid_PluginGuid, MCTL_SENDSTRING, MSSC_POST, &mcr);
	MacroParseResult* mpr;
	if (!i)
	{
		mpr = (MacroParseResult*)calloc(4096,1);
		mpr->StructSize = sizeof(*mpr);
		psi.MacroControl(&guid_PluginGuid, MCTL_GETLASTERROR, 4096, mpr);
		free(mpr);
	}
	else
		InterlockedIncrement(&gnInCall);
#endif
#if 0
	wchar_t szDbg[100];
	static LONG nCount; nCount++;
	wsprintf(szDbg, L"%i: PanelControl(FCTL_GETPANELINFO, PANEL_ACTIVE)...", nCount);
	PanelInfo piA = {sizeof(piA)}, piP = {sizeof(piP)};
	INT_PTR nARc = psi.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &piA);
	OutputDebugString(L" and PANEL_PASSIVE...");
	INT_PTR nPRc = psi.PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, &piP);
	OutputDebugString(L" Done\n");
#endif
	return 0;
}

void WINAPI ExitFARW(const struct ExitInfo *Info)
{
	if (hThread)
	{
		SetEvent(hExit);
		if (WAIT_TIMEOUT == WaitForSingleObject(hThread, 1000))
			TerminateThread(hThread, 100);
		CloseHandle(hThread);
		hThread = NULL;
	}
	if (hExit)
	{
		CloseHandle(hExit);
		hExit = NULL;
	}
}
