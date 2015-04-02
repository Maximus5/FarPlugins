
/*
Copyright (c) 2010 Maximus5
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


//#include <windows.h>
//#include <TCHAR.H>
#include "headers.hpp"
#include "dizlist.hpp"
#include "config.hpp"
//#include "imports.hpp"
#include "version.h"

#ifndef ARRAYSIZE
	#define ARRAYSIZE(array) (sizeof(array)/sizeof(*array))
#endif

/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;

// Diz object
DizList diz;

// {F7BE9D38-3B74-427F-8DB7-92130356FA30}
const GUID guid_DizC0 = 
{ 0xf7be9d38, 0x3b74, 0x427f, { 0x8d, 0xb7, 0x92, 0x13, 0x3, 0x56, 0xfa, 0x30 } };


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    return TRUE;
}


#if defined(__GNUC__)

extern
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     );

#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  int WINAPI GetMinFarVersionW(void);
  void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info);
  void WINAPI GetPluginInfoW(struct PluginInfo *Info);
  int WINAPI GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData);
  void WINAPI FreeCustomDataW(wchar_t *CustomData);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  DllMain(hDll, dwReason, lpReserved);
  return TRUE;
}
#endif

#if 0
int WINAPI GetMinFarVersionW(void)
{
    return MAKEFARVERSION(3,0,3000,VS_RELEASE);
}
#endif

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	//_ASSERTE(Info->StructSize >= sizeof(GlobalInfo));
	Info->MinFarVersion = MAKEFARVERSION(3,0,0,3000,VS_RELEASE);

	// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);
	
	Info->Guid = guid_DizC0;
	Info->Title = L"diz_c0";
	Info->Description = L"Show string from descript.ion in the C0 column";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
    ::psi = *Info;
    ::fsf = *(Info->FSF);
    
//    ifn.Load();
    ReadConfig();
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->Flags = PF_PRELOAD;
}

intptr_t WINAPI GetContentFieldsW(const struct GetContentFieldsInfo *Info)
{
	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"diz")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
			return 1;
	}
	return 0;
}

intptr_t WINAPI GetContentDataW(struct GetContentDataInfo *Info)
{
	if (!Info)
		return FALSE;
	LPCWSTR FilePath = Info->FilePath;
	wchar_t** CustomData = NULL;

	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"diz")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
		{
			CustomData = (wchar_t**)(Info->Values + i);
		}
	}

	if (!FilePath || !CustomData)
		return FALSE;
	
	const wchar_t* pszSlash = wcsrchr(FilePath, L'\\');
	if (!pszSlash || pszSlash <= FilePath) return FALSE;
	if (pszSlash[1] == 0) return FALSE; // Если хотят диз именно для папки - то нужно без слеша
	string  strPath(FilePath, pszSlash-FilePath);
	
	// оптимизацией чтения диз-файла занимается сам diz
	if (diz.Read(strPath) == 0)
	{
		// Если диз пустой - сразу выходим
		return FALSE;
	}

	
	const wchar_t* pszDiz = diz.GetDizTextAddr(pszSlash+1, L"", 0/*???*/);
	//if (!pszDiz || pszDiz[0] == 0) -- ConvertNameToShort занимает очень много времени
	//{
	//	string strShort;
	//	ConvertNameToShort(FilePath, strShort);
	//	pszDiz = diz.GetDizTextAddr(pszSlash+1, strShort, 0/*???*/);
	//}
	if (!pszDiz || pszDiz[0] == 0)
	{
		return FALSE;
	}
	
	size_t nLen = wcslen(pszDiz)+1;
	*CustomData = (wchar_t*)malloc(nLen*2);
	wcscpy(*CustomData, pszDiz);
	// Заменить некоторые символы
	wchar_t* pszTab = wcspbrk(*CustomData, L"\t");
	while (pszTab)
	{
		*pszTab = L' ';
		pszTab = wcspbrk(pszTab+1, L"\t");
	}

	return TRUE;
}

void WINAPI FreeContentDataW(const struct GetContentDataInfo *Info)
{
	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"diz")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
		{
			if (Info->Values[i])
			{
				wchar_t* ptr = (wchar_t*)Info->Values[i];
				if (ptr) free(ptr);
			}
		}
	}
}
