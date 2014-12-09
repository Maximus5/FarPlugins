
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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

#include "../../common/plugin.h"

// {CB7DD093-BCC3-4BA7-9910-A197F1B3ED5A}
static GUID guid_NilC0 =
{ 0xcb7dd093, 0xbcc3, 0x4ba7, { 0x99, 0x10, 0xa1, 0x97, 0xf1, 0xb3, 0xed, 0x5a } };


/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;

BOOL WINAPI DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

extern "C"
BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}

int WINAPI GetMinFarVersionW()
{
	// Чтобы в Far2 не загружался
	return MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD);
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	::psi = *Info;
	::fsf = *(Info->FSF);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = PF_PRELOAD;
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->MinFarVersion = MAKEFARVERSION(3,0,0,4040,VS_RELEASE);

	Info->Version = MAKEFARVERSION(3,0,0,0,VS_RELEASE);
	
	Info->Guid = guid_NilC0;
	Info->Title = L"nil_c0";
	Info->Description = L"Just returns empty string for C0 column";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

int WINAPI GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData)
{
	if (!FilePath || !CustomData)
		return FALSE;

	// Don't process "." and ".." items
	LPCWSTR pszName = wcsrchr(FilePath, L'\\');
	if (pszName) pszName++; else pszName = FilePath;
	if (!*pszName /*|| !lstrcmp(pszName, L".") || !lstrcmp(pszName, L"..")*/)
		return FALSE;

	// Just return empty string
	*CustomData = (wchar_t*)calloc(1,sizeof(wchar_t));
	return (*CustomData != NULL);
}

void WINAPI FreeCustomDataW(wchar_t *CustomData)
{
	if (CustomData)
		free(CustomData);
}
