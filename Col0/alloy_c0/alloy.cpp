
/*
Copyright (c) 2014-2017 Maximus5
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
#include <crtdbg.h>

#ifndef nullptr
#define nullptr NULL
#endif

#include "../../common/plugin.h"
#include "../../common/DlgBuilder.hpp"
const void* PluginDialogBuilder::gpDialogBuilderPtr;
#include "../../common/far3l/PluginSettings.hpp"
#include "../../common/MArray.h"

// {02F09F74-8B40-41D6-8756-D84447121D85}
static GUID guid_PluginGuid =
{ 0x02f09f74, 0x8b40, 0x41d6, { 0x87, 0x56, 0xd8, 0x44, 0x47, 0x12, 0x1d, 0x85 } };
// {2CA70D46-4800-4571-9170-32983EC6D6D8}
static GUID guid_Config =
{ 0x2ca70d46, 0x4800, 0x4571, { 0x91, 0x70, 0x32, 0x98, 0x3e, 0xc6, 0xd6, 0xd8 } };


/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;

wchar_t gsMasks[2048] = L"*.avi,*.mkv,*.wmv";
wchar_t gsLAP1[MAX_PATH] = L"";
wchar_t gsLAP2[MAX_PATH] = L"";
wchar_t gsMark[32] = L"@";

struct Opt {
	LPCWSTR  pszName;
	wchar_t* pszValue;
	size_t   cchMax;
} gOpt[] = {
	{L"Masks", gsMasks, ARRAYSIZE(gsMasks)},
	{L"LAP1",  gsLAP1,  ARRAYSIZE(gsLAP1)},
	{L"LAP2",  gsLAP2,  ARRAYSIZE(gsLAP2)},
	{L"Mark",  gsMark,  ARRAYSIZE(gsMark)},
	{NULL},
};


class CLAP
{
protected:
	wchar_t ms_LapFile[MAX_PATH];
	HANDLE mh_Notification;
	bool mb_Changed;

public:
	// Simple Hasher
	static DWORD CalcCRC(const BYTE *pData, size_t cchSize)
	{
		static DWORD CRCtable[] =
		{
			0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
			0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 
			0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
			0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
			0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
			0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 
			0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
			0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
			0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
			0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 
			0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
			0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
			0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
			0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 
			0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
			0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
			0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
			0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
			0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
			0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
			0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
			0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 
			0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
			0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
			0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
			0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 
			0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
			0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
			0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
			0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 
			0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
			0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
			0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
			0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 
			0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
			0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
			0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
			0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 
			0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
			0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
			0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
			0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 
			0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
		};
	
		DWORD crc = 0xFFFFFFFF;

		for (const BYTE* p = pData; cchSize; cchSize--)
		{
			crc = ( crc >> 8 ) ^ CRCtable[(unsigned char) ((unsigned char) crc ^ *p++ )];
		}

		return (crc ^ 0xFFFFFFFF);
	}

protected:
	struct PlayedFile
	{
		DWORD PathNameCRC32;
		wchar_t* LowerCased;
	};
	MArray<PlayedFile> m_Files;

protected:
	void Reset(bool bReqReload)
	{
		mb_Changed = bReqReload;

		if (mh_Notification)
		{
			FindCloseChangeNotification(mh_Notification);
			mh_Notification = NULL;
		}

		Clear();
	}

	void Clear()
	{
		// Reset loaded contents
		for (INT_PTR i = m_Files.size()-1; i >= 0; i--)
		{
			free(m_Files[i].LowerCased);
		}
		m_Files.clear();
	}

	void StartNotification()
	{
		if (!*ms_LapFile)
			return;

		wchar_t szFolder[MAX_PATH];
		lstrcpyn(szFolder, ms_LapFile, ARRAYSIZE(szFolder));

		wchar_t* pszSlash = wcsrchr(szFolder, L'\\');
		if (pszSlash)
		{
			if (wcschr(szFolder, L'\\') < pszSlash)
				pszSlash[0] = 0;
			else
				pszSlash[1] = 0;
		}

		mh_Notification = FindFirstChangeNotification(szFolder, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE);
		if (mh_Notification == INVALID_HANDLE_VALUE)
			mh_Notification = NULL;
	}

	void ReloadLapFile()
	{
		mb_Changed = false;

		HANDLE hFile = CreateFile(ms_LapFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile && (hFile != INVALID_HANDLE_VALUE))
		{
			int nSize = (DWORD)GetFileSize(hFile, NULL);
			if (nSize > 0)
			{
				LPBYTE ptrData = (LPBYTE)malloc(nSize+3);
				if (ptrData)
				{
					DWORD nRead = 0;
					if (ReadFile(hFile, ptrData, nSize, &nRead, NULL) && nRead)
					{
						ptrData[nRead] = ptrData[nRead+1] = 0;
						wchar_t* pszData = NULL;
						if (*((LPWORD)ptrData) == 0xFEFF)
						{
							pszData = (wchar_t*)(ptrData+2);
						}
						else if (ptrData[0] == 0xEF && ptrData[1] == 0xBB && ptrData[2] == 0xBF)
						{
							wchar_t* from_utf8 = (wchar_t*)malloc(nRead*sizeof(*from_utf8));
							int new_len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)(ptrData+3), nRead-1, from_utf8, nRead);
							if (new_len > 0)
							{
								free(ptrData);
								ptrData = (LPBYTE)from_utf8;
								pszData = (wchar_t*)(ptrData);
							}
						}
						else
						{
							_ASSERTE(FALSE && "CP 1200 BOM not found!");
						}
						
						if (pszData)
						{
							while (*pszData)
							{
								while (*pszData == L'\r' || *pszData == L'\n') pszData++;

								wchar_t* pszEnd = wcspbrk(pszData, L"\r\n");
								if (!pszEnd) pszEnd = pszData + lstrlen(pszEnd);

								if ((pszEnd > pszData) && (*pszData != L'>'))
								{
									size_t cchLen = (pszEnd - pszData);
									if (cchLen < 0x8000)
									{
										PlayedFile f = {};
										f.LowerCased = (wchar_t*)malloc((cchLen+1)*sizeof(*f.LowerCased));
										if (f.LowerCased)
										{
											memmove(f.LowerCased, pszData, cchLen*sizeof(*f.LowerCased));
											f.LowerCased[cchLen] = 0;
											CharLowerBuff(f.LowerCased, cchLen);
											f.PathNameCRC32 = CalcCRC((BYTE*)f.LowerCased, cchLen*sizeof(*f.LowerCased));
											m_Files.push_back(f);
										}
									}
								}

								pszData = pszEnd;
							}
						}
					}
					free(ptrData);
				}
			}
			CloseHandle(hFile);
		}
	}

	void CheckNeedReload()
	{
		DWORD nMod = WAIT_TIMEOUT;
		if (!mb_Changed)
		{
			if (mh_Notification)
			{
				nMod = WaitForSingleObject(mh_Notification, 0);
				if (nMod == WAIT_OBJECT_0)
					mb_Changed = true;
			}
		}

		if (!mb_Changed)
			return;

		// Reload
		Clear();
		ReloadLapFile();

		// [Re]start notification
		if ((nMod == WAIT_OBJECT_0) && mh_Notification)
			FindNextChangeNotification(mh_Notification);
		else if (!mh_Notification)
			StartNotification();
	}

public:
	CLAP()
		: mh_Notification(NULL)
		, mb_Changed(false)
	{
		ms_LapFile[0] = 0;
	}

	~CLAP()
	{
		Reset(false);
	}

	void SetLapFile(LPCWSTR asLapFile)
	{
		if (!asLapFile || !*asLapFile)
		{
			ms_LapFile[0] = 0;
			Reset(false);
			return;
		}
		if (lstrcmpi(ms_LapFile, asLapFile) == 0)
		{
			// LAP-file was not changed
			return;
		}
		lstrcpyn(ms_LapFile, asLapFile, ARRAYSIZE(ms_LapFile));

		Reset(true);
	}

	bool WasPlayed(DWORD PathNameCrc, LPCWSTR asLowerCasedPath)
	{
		CheckNeedReload();

		for (INT_PTR i = m_Files.size()-1; i >= 0; i--)
		{
			if (PathNameCrc == m_Files[i].PathNameCRC32)
			{
				if (lstrcmpi(m_Files[i].LowerCased, asLowerCasedPath) == 0)
				{
					return true;
				}
			}
		}

		return false;
	}
};

CLAP gLap1, gLap2;

BOOL WINAPI DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

/*
extern "C"
BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
*/

int WINAPI GetMinFarVersionW()
{
	// Чтобы в Far2 не загружался
	return MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD);
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	::psi = *Info;
	::fsf = *(Info->FSF);

	LPCWSTR psz;
	PluginSettings S(guid_PluginGuid, psi.SettingsControl);
	for (size_t i = 0; gOpt[i].pszName; i++)
	{
		if ((psz = S.Get(0, gOpt[i].pszName, gOpt[i].pszValue)) != NULL)
			lstrcpyn(gOpt[i].pszValue, psz, gOpt[i].cchMax);
	}
	gLap1.SetLapFile(gsLAP1);
	gLap2.SetLapFile(gsLAP2);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = PF_PRELOAD;
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->MinFarVersion = MAKEFARVERSION(3,0,0,4040,VS_RELEASE);

	Info->Version = MAKEFARVERSION(3,1,0,0,VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"LightAlloy for C0";
	Info->Description = L"Shows 'Played in LightAllow' in C0 column";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

bool CheckFileName(LPCWSTR FileName)
{
	// gsMasks - comma separated masks
	bool bMatch = (*gsMasks) && (fsf.ProcessName(gsMasks, (wchar_t*)FileName, 0, PN_CMPNAMELIST) != 0);
	return bMatch;
}

intptr_t WINAPI GetContentFieldsW(const struct GetContentFieldsInfo *Info)
{
	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"alloy")
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
		if (!fsf.LStricmp(Info->Names[i], L"alloy")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
		{
			CustomData = (wchar_t**)(Info->Values + i);
		}
	}

	if (!FilePath || !CustomData)
		return FALSE;

	// Don't process "." and ".." items
	LPCWSTR pszName = wcsrchr(FilePath, L'\\');
	if (pszName) pszName++; else pszName = FilePath;
	if (!*pszName /*|| !lstrcmp(pszName, L".") || !lstrcmp(pszName, L"..")*/)
		return FALSE;
	if (!CheckFileName(pszName))
		return FALSE;

	if (FilePath[0] == L'\\' && FilePath[1] == L'\\' && FilePath[2] == L'?' && FilePath[3] == L'\\')
		FilePath += 4;

	int iLen = lstrlen(FilePath);
	wchar_t* pszLowerCased = (wchar_t*)malloc((iLen+2)*sizeof(*pszLowerCased));
	if (!pszLowerCased)
		return FALSE;

	lstrcpyn(pszLowerCased, FilePath, iLen+1);
	CharLowerBuff(pszLowerCased, iLen);
	DWORD crc = CLAP::CalcCRC((BYTE*)pszLowerCased, iLen*sizeof(*pszLowerCased));

	// Need be lower-cased
	bool bPlayed = (gLap1.WasPlayed(crc, pszLowerCased) || gLap2.WasPlayed(crc, pszLowerCased));

	// Return "Played mark"
	if (bPlayed)
		*CustomData = gsMark;

	free(pszLowerCased);

	return bPlayed;
}

void WINAPI FreeContentDataW(const struct GetContentDataInfo *Info)
{
	//if (CustomData)
	//	free(CustomData);
}

intptr_t WINAPI ConfigureW(const struct ConfigureInfo *Info)
{
	// "LightAlloy for C0 configuration"
	PluginDialogBuilder dlg(psi, 0, NULL, &guid_Config);
	dlg.AddText(1); // "Supported video files:"
	dlg.AddEditField(gsMasks, ARRAYSIZE(gsMasks), 50, NULL);
	dlg.AddText(2); // "Primary LighAlloy *.lap file:"
	dlg.AddEditField(gsLAP1, ARRAYSIZE(gsLAP1), 50, NULL);
	dlg.AddText(3); // "Primary LighAlloy *.lap file:"
	dlg.AddEditField(gsLAP2, ARRAYSIZE(gsLAP2), 50, NULL);
	dlg.AddText(4); // "Marker:"
	dlg.AddEditField(gsMark, ARRAYSIZE(gsMark), 50, NULL);

	dlg.AddOKCancel(5, 6, true);

	if (!dlg.ShowDialog())
		return FALSE;

	PluginSettings S(guid_PluginGuid, psi.SettingsControl);
	for (size_t i = 0; gOpt[i].pszName; i++)
	{
		S.Set(0, gOpt[i].pszName, gOpt[i].pszValue);
	}

	gLap1.SetLapFile(gsLAP1);
	gLap2.SetLapFile(gsLAP2);

	return TRUE;
}
