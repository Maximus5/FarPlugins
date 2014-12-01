
/*
Copyright (c) 2009 ConEmu.Maximus5@gmail.com
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS OR
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

#include <windows.h>
#include <crtdbg.h>
#ifdef FAR_UNICODE
#pragma message("FAR_UNICODE defined")
#else
#pragma message("FAR_UNICODE NOT defined")
#endif
#include "version.h"
#include "../../common/plugin.h"


struct PluginStartupInfo psi;
struct FarStandardFunctions fsf;

#ifdef FAR_UNICODE
// {8F1F399D-1577-415A-9AC9-83B529F838DE}
static GUID guid_PluginGuid =
{ 0x8f1f399d, 0x1577, 0x415a, { 0x9a, 0xc9, 0x83, 0xb5, 0x29, 0xf8, 0x38, 0xde } };
// {35F0D0C8-FA74-42A2-92E8-15A47785BB0F}
static GUID guid_PluginMenu =
{ 0x35f0d0c8, 0xfa74, 0x42a2, { 0x92, 0xe8, 0x15, 0xa4, 0x77, 0x85, 0xbb, 0x0f } };
#endif

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	return TRUE;
}

#if defined(__GNUC__)
#define CRTSTARTUP
#else
#define DllMainCRTStartup _DllMainCRTStartup
#endif

#if defined(CRTSTARTUP)

#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  #if defined(__GNUC__)
  void WINAPI SetStartupInfoW(struct PluginStartupInfo *aInfo);
  #endif
#ifdef __cplusplus
};
#endif

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  return DllMain(hDll, dwReason,lpReserved);
}
#endif


#if FAR_UNICODE>=1906
void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	_ASSERTE(Info->StructSize == sizeof(GlobalInfo));
	
	Info->MinFarVersion = /*FARMANAGERVERSION*/
		MAKEFARVERSION(
			FARMANAGERVERSION_MAJOR,
			FARMANAGERVERSION_MINOR,
			FARMANAGERVERSION_REVISION,
			FARMANAGERVERSION_BUILD,
			FARMANAGERVERSION_STAGE);
	
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,FARMANAGERVERSION_BUILD,VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"Clipboard fixer";
	Info->Description = L"Clipboard fixer";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
#else
void WINAPI GetGlobalInfoW(void *Info)
{
}
#endif

// minimal(?) FAR version 2.0 alpha build 1587
int WINAPI GetMinFarVersionW(void)
{
	int nVer =
	#ifdef _UNICODE
		#if FAR_UNICODE>=1906
			// Чтобы в Far2 не загружался
			MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD)
		#else
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
    static WCHAR *szMenu[1], szMenu1[255];
    szMenu[0] = szMenu1;
    lstrcpyW(szMenu[0], L"Clipboard fixer");

    #ifndef FAR_UNICODE
    if (!pi->StructSize)
		pi->StructSize = sizeof(struct PluginInfo);
	#endif

	pi->Flags = PF_EDITOR | PF_DIALOG;

	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_PluginMenu;
		pi->PluginMenu.Strings = szMenu;
		pi->PluginMenu.Count = 1;
	#else
		pi->PluginMenuStrings = szMenu;
		pi->PluginMenuStringsNumber = 1;
	#endif

	#ifndef FAR_UNICODE
	// Far 2.x callplugin id
	pi->Reserved = 0x436C4678;
	#endif
}

void DoTheJob()
{
	if (OpenClipboard(NULL))
	{
		if (CF_TEXT == EnumClipboardFormats(0))
		{
			if (IsClipboardFormatAvailable(CF_UNICODETEXT))
			{
				HANDLE hText = GetClipboardData(CF_TEXT);
				HANDLE hUText = GetClipboardData(CF_UNICODETEXT);
				if (hText && hUText) {
					LPCSTR pszText = (LPCSTR)GlobalLock(hText);
					LPCWSTR pwszText = (LPCWSTR)GlobalLock(hUText);
					if (pszText && pwszText)
					{
						BOOL lbNeedFix = FALSE;
						LPCSTR psz = pszText;
						LPCWSTR pwsz = pwszText;
						while (*psz && *pwsz)
						{
							if (*psz > 127 && *pwsz == *psz)
							{
								lbNeedFix = TRUE;
								break;
							}
							psz++; pwsz++;
						}
						if (lbNeedFix)
						{
							if (pwszText) { GlobalUnlock(hUText); pwszText = NULL; }
							size_t nLen = lstrlenA(pszText);
							if ((hUText = GlobalAlloc(GMEM_MOVEABLE, (nLen+1)*2)) != NULL)
							{
								wchar_t *pwszNew = (wchar_t*)GlobalLock(hUText);
								if (pwszNew)
								{
									MultiByteToWideChar(CP_ACP, 0,
										pszText, nLen+1, pwszNew, nLen+1);
									GlobalUnlock(hUText);
									SetClipboardData(CF_UNICODETEXT, hUText);
								}
								else
								{
									// неудача
									GlobalUnlock(hUText);
									GlobalFree(hUText);
								}
							}
						}
					}
					if (pszText) GlobalUnlock(hText);
					if (pwszText) GlobalUnlock(hUText);
				}
			}
		}
		CloseClipboard();
	}
}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item)
{
	DoTheJob();
	return PANEL_STOP;
}

HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	DoTheJob();
	return NULL;
}

void WINAPI SetStartupInfoW(struct PluginStartupInfo *aInfo)
{
	::psi = *((struct PluginStartupInfo*)aInfo);
	::fsf = *((struct PluginStartupInfo*)aInfo)->FSF;
	::psi.FSF = &::fsf;
}
