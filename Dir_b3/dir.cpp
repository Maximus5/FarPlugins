
/* **************************
   This module may be used as
   1) Observer module (*.so)
   2) MultiArc module (*.fmt)
 * ************************** */

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <stdlib.h>
#include <windows.h>
#include "plugin.hpp"
#include "fmt.hpp"
#include <crtdbg.h>
#include <cstdint>
#include <vector>
#include <string>

/******************************************************
 Том в устройстве D имеет метку DIST_ALX
 Серийный номер тома: 356D-B9B3

 Содержимое папки D:\!Develop!\Dir

15.11.2008  17:36    <DIR>          .
15.11.2008  17:36    <DIR>          ..
15.11.2008  17:39    <DIR>          dir010327
15.11.2008  17:46    <DIR>          dirparse
15.11.2008  18:11    <DIR>          [2008.11.15]
15.11.2008  18:36                 0 !!
               1 файлов              0 байт

 Содержимое папки D:\!Develop!\Dir\dir010327

15.11.2008  17:39    <DIR>          .
15.11.2008  17:39    <DIR>          ..
27.03.2001  08:47             7 581 dir.cpp
27.03.2001  09:03             6 144 dir.fmt
29.03.2001  18:11             2 600 dir.txt
               3 файлов         16 325 байт

     Всего файлов:
               9 файлов         26 795 байт
              11 папок     396 525 568 байт свободно
 Volume in drive C is w2k3
 Volume Serial Number is F0AC-A340

 Directory of C:\Programs\Develop\mingw32\mingw

31.10.2008  12:30    <DIR>          .
31.10.2008  12:30    <DIR>          ..
31.10.2008  12:30    <DIR>          bin
04.02.2008  04:19            35 821 COPYING-gcc-tdm.txt
09.08.2007  17:18            26 930 COPYING.lib-gcc-tdm.txt
31.10.2008  12:30    <DIR>          doc
31.10.2008  13:06    <DIR>          include
31.10.2008  12:30    <DIR>          info
31.10.2008  13:06    <DIR>          lib
28.08.2008  16:39    <DIR>          libexec
31.10.2008  12:30    <DIR>          man
31.10.2008  12:30    <DIR>          mingw32
26.05.2005  10:15            21 927 pthreads-win32-README
28.08.2008  16:58            14 042 README-gcc-tdm.txt
               4 File(s)         98 720 bytes

******************************************************/

/*
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
*/

struct DirInfo;
DirInfo* gpCur = nullptr;

//static struct PluginStartupInfo Info;
struct DirInfo
{
private:
	HANDLE Handle, MapHandle;
	char* Data;
	char* Pos;
	char* Edge;
	char Prefix[MAX_PATH - 2];
	char Descrip[80];
	char Oem2Ansi[4096];
	int nPrefixLen;
	wchar_t Utf8[4096];
	wchar_t Wide[MAX_PATH];
	char Buf[8192]; // To get a line
	size_t nStrLen;
	size_t nSkipLen;
	bool FormatOk;

	enum class CodePages
	{
		NotChecked = 0,
		OEM = 1,
		ANSI = 2,
		UTF8 = 3,
	};
	CodePages CP_Check;

	bool isRus = false;

	// offsets for WIN XP/2003/VISTA:
	size_t DATA_DISP; // = 0;
	size_t TIME_DISP; // = 12;
	size_t SIZE_DISP; // = 19;
	size_t NAME_DISP; // = 36;
	size_t ID_DIR_DISP; // = 21;
	bool offsetsChecked;

	enum class DateFormat
	{
		NotChecked = 0,
		DD_MM_YYYY = 1,
		MM_DD_YYYY = 2,
	};
	DateFormat dfCheck;

	enum class TimeFormat
	{
		NotChecked = 0,
		Time24H = 1,
		Time12H_AM_PM = 2,
	};
	TimeFormat tfCheck;

public:
	static DirInfo* Create()
	{
		auto* obj = static_cast<DirInfo*>(calloc(1, sizeof(DirInfo)));;
		if (!obj)
			return nullptr;
		obj->DATA_DISP = 0;
		obj->TIME_DISP = 12;
		obj->SIZE_DISP = 19;
		obj->NAME_DISP = 36;
		obj->ID_DIR_DISP = 21;
		return obj;
	}

	static int64_t AtoI(char *Str, int Len)
	{
		int64_t Ret=0;
		for (; Len && *Str; Len--, Str++)
			if (*Str>='0' && *Str<='9')
				(Ret*=10)+=*Str-'0';
		return Ret;
	}

	bool IsUtf8File() const
	{
		return CP_Check == CodePages::UTF8;
	}

	// Returns: 0 if no match, or 1-based index of matched Mask
	int Compare(const char* Str, const char** MaskList, bool MaskHasNonEng = false, int StrMaxLen = -1)
	{
		for (int i = 0; MaskList[i] != nullptr; ++i)
		{
			const char* mask = MaskList[i];
			const int maskLen = lstrlenA(mask);

			auto compareWith = [&](const char* cmpWith)
			{
				const int cmpLen = (StrMaxLen < 0) ? maskLen : min(StrMaxLen, maskLen);
				const auto iCmp = CompareString(LOCALE_USER_DEFAULT, 0, Str, cmpLen, mask, cmpLen);
				return (iCmp == CSTR_EQUAL);
			};

			if (compareWith(mask))
			{
				return (i + 1);
			}

			if (MaskHasNonEng && (i > 0))
			{
				if ((CP_Check == CodePages::NotChecked) && (maskLen < static_cast<int>(ARRAYSIZE(Oem2Ansi))))
				{
					CharToOemBuff(mask, Oem2Ansi, maskLen);
					if (compareWith(Oem2Ansi))
					{
						CP_Check = CodePages::OEM;
						return (i + 1);
					}
				}
				if ((CP_Check == CodePages::NotChecked || CP_Check == CodePages::UTF8) && (maskLen < static_cast<int>(ARRAYSIZE(Oem2Ansi))))
				{
					const int nWide = MultiByteToWideChar(CP_ACP, 0, mask, maskLen, Utf8, ARRAYSIZE(Utf8) - 1);
					if (nWide > 0)
					{
						Utf8[nWide] = 0;
						const int nUtf8 = WideCharToMultiByte(CP_UTF8, 0, Utf8, nWide + 1, Oem2Ansi, ARRAYSIZE(Utf8) - 1, nullptr, nullptr);
						if (nUtf8 > 0)
						{
							const size_t maxCompare = (StrMaxLen < 0) ? nUtf8 : min(nUtf8, StrMaxLen);
							if (memcmp(Str, Oem2Ansi, maxCompare) == 0)
							{
								CP_Check = CodePages::UTF8;
								return (i + 1);
							}
						}
					}
				}
			}
		}

		return 0;
	}

	bool GetS(char *pBuf, DWORD nBufLen)
	{
		if (!pBuf || !nBufLen)
		{
			_ASSERTE(pBuf && nBufLen);
			return false;
		}
		const char *Start=pBuf;
		if (Pos >= Edge)
		{
			nStrLen = 0;
			return false;
		}
		while (Pos < Edge && *Pos!='\r' && *Pos!='\n' && (--nBufLen)) //Maks " && (--BufLen)"
			*pBuf++ = *Pos++;
		*pBuf=0;
		for (; Pos < Edge && (*Pos=='\r' || *Pos=='\n'); Pos++)
			;
		nStrLen = pBuf - Start;
		return true;
	};

	BOOL openArchive(int* Type)
	{
		//Handle=CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		//                  FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (Handle != INVALID_HANDLE_VALUE)
		{
			MapHandle = CreateFileMapping(Handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if (MapHandle)
			{
				Pos = Data = static_cast<char*>(MapViewOfFile(MapHandle, FILE_MAP_READ, 0, 0, 0));
				if (Data)
				{
					*Type = 0;
					*Prefix = 0; nPrefixLen = 0;
					*Descrip = 0;
					nSkipLen = 0;
					Edge = Data + GetFileSize(Handle, nullptr);

					PluginPanelItem Item{};
					ArcItemInfo Info{};
					const bool lbRc = (getArcItem(&Item, &Info) == GETARC_SUCCESS);

					nStrLen = nSkipLen = 0;
					*Prefix = 0; nPrefixLen = 0;
					Pos = Data;

					if (lbRc)
					{
						//static const char *MsgItems[]={ "", "In mode branch?"};
						//isBtanch=!Info.Message(Info.ModuleNumber, FMSG_MB_YESNO, 0, MsgItems, 2, 0);
						return true;
					}
				}
				CloseHandle(MapHandle); MapHandle = nullptr;
			}
			CloseHandle(Handle);
		}
		Handle = nullptr;
		return false;
	}

	int getArcItem(struct PluginPanelItem* Item, struct ArcItemInfo* Info)
	{
		const char* LabelHeader[3] = { " Volume in", " Том", nullptr };
		const char* SerialHeader[3] = { " Volume Serial", " Серийный", nullptr };
		const char* DirHeader[3] = { " Directory of", " Содержимое папки", nullptr  };
		const char* DirEndHeader[3] = { "     Total Files", "     Всего файлов:", nullptr };
		const char* DirOrJunc[3] = { "<DIR>", "<JUNCTION>", nullptr };

		size_t nLines = 0;
		int matchRc;

		//if (isBtanch) nSkipLen=0;
		while (GetS(Buf, ARRAYSIZE(Buf) - 2))
		{
			nLines++;
			if ((matchRc = Compare(Buf, LabelHeader, true)) > 0)
			{
				FormatOk = true;
				isRus = (matchRc >= 2);
				//if (isRus && strncmp(Buf, LabelHeader[0], lstrlenA(LabelHeader[0])))
				//{
				//	OemToCharBuff(Buf, Buf, lstrlenA(Buf));
				//}
				if (CP_Check == CodePages::OEM)
				{
					OemToCharBuff(Buf, Buf, lstrlenA(Buf));
					CP_Check = CodePages::ANSI;
				}

				*Prefix = 0; nPrefixLen = 0; nSkipLen = 0;

				// 26.11.2008 Maks -
				//lstrcpy(Descrip, "  Label:");
				Descrip[0] = 0;
				// 26.11.2008 Maks - лидирующий пробел не нужен
				//if (isRus?*(Buf+20)=='и':*(Buf+19)=='i' ) lstrcat(Descrip, Buf+(isRus?31:21));
				char* lpszPtr = nullptr;
				if (isRus ? *(Buf + 20) == 'и' : *(Buf + 19) == 'i') lpszPtr = Buf + (isRus ? 31 : 21);
				if (!lpszPtr) continue;
				while (*lpszPtr == ' ') lpszPtr++;

				// 26.11.2008 Maks - Сразу вернем элемент если есть метка диска
				if (!*lpszPtr) continue;
				memset(Item, 0, sizeof(PluginPanelItem));

				lstrcpy(Item->FindData.cFileName, "<VOLUMELABEL:");
				// Ограничить длину метки
				if (lstrlenA(lpszPtr) >= (MAX_PATH - 20))
					lpszPtr[MAX_PATH - 20] = 0;
				lstrcat(Item->FindData.cFileName, lpszPtr);
				lstrcat(Item->FindData.cFileName, ">");

				Item->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				Item->UserData = 0;

				SYSTEMTIME st;
				GetSystemTime(&st);
				FILETIME ft;
				SystemTimeToFileTime(&st, &ft);
				LocalFileTimeToFileTime(&ft, &Item->FindData.ftLastWriteTime);

				lstrcpy(Info->Description, lpszPtr);
				return GETARC_SUCCESS;
			}
			if (Compare(Buf, SerialHeader) > 0)
			{
				FormatOk = true;
				/* // 26.11.2008 Maks - Volume Serial - не интересно
				char temp[80];
				lstrcpy(temp, Descrip);
				lstrcpy(Descrip, Buf+(isRus?21:24));
				lstrcat(Descrip, temp);
				*/
				continue;
			}
			if (Compare(Buf, DirHeader) > 0)
			{
				FormatOk = true;
				if (!nSkipLen)
					nSkipLen = nStrLen;
				else
				{
					lstrcpyn(Prefix, Buf + nSkipLen + (Buf[nSkipLen] == '\\' ? 1 : 0), ARRAYSIZE(Prefix) - 2);
					nPrefixLen = lstrlen(Prefix);
					if (nPrefixLen && Prefix[nPrefixLen - 1] != '\\')
					{
						Prefix[nPrefixLen++] = '\\';
						Prefix[nPrefixLen] = 0;
					}
				}
				continue;
			}

			if (!FormatOk)
			{
				// Если поля заголовка не нашли в первых 128 строках - выходим
				if (nLines > 128)
					break;
				else
					continue;
			}

			if (Compare(Buf, DirEndHeader) > 0)
			{
				nSkipLen = 0; *Prefix = 0; nPrefixLen = 0;
				continue;
			}
			//2008-11-30 check if we can continue
			if (nStrLen > NAME_DISP && *Buf != ' ' && *(Buf + 1) != ' ')
			{
				memset(Item, 0, sizeof(PluginPanelItem));

				SYSTEMTIME st{};
				st.wDayOfWeek = st.wSecond = st.wMilliseconds = 0;
				st.wDay = static_cast<WORD>(AtoI(Buf + DATA_DISP, 2));
				st.wMonth = static_cast<WORD>(AtoI(Buf + DATA_DISP + 3, 2));
				st.wYear = static_cast<WORD>(AtoI(Buf + DATA_DISP + 6, 4));
				st.wHour = static_cast<WORD>(AtoI(Buf + TIME_DISP, 2));
				st.wMinute = static_cast<WORD>(AtoI(Buf + TIME_DISP + 3, 2));
				if (dfCheck == DateFormat::NotChecked)
				{
					if (st.wMonth > 12 && st.wMonth <= 31 && st.wDay <= 12)
					{
						dfCheck = DateFormat::MM_DD_YYYY;
					}
					else if (st.wDay > 12 && st.wDay <= 31 && st.wMonth <= 12)
					{
						dfCheck = DateFormat::DD_MM_YYYY;
					}
				}
				if (dfCheck == DateFormat::MM_DD_YYYY)
				{
					std::swap(st.wMonth, st.wDay);
				}
				if (tfCheck == TimeFormat::NotChecked)
				{
					if (st.wHour > 12 && st.wHour <= 23 && st.wMinute <= 60)
					{
						tfCheck = TimeFormat::Time24H;
					}
					else if (st.wHour <= 12 && st.wMinute <= 60
						&& (Buf[TIME_DISP + 6] == 'A' || Buf[TIME_DISP + 6] == 'P') && Buf[TIME_DISP + 7] == 'M')
					{
						tfCheck = TimeFormat::Time12H_AM_PM;
						NAME_DISP += 3;
						ID_DIR_DISP += 3;
						SIZE_DISP += 3;
					}
				}
				if (tfCheck == TimeFormat::Time12H_AM_PM)
				{
					// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
					switch (Buf[TIME_DISP + 6])
					{
					case 'A':
						if (st.wHour == 12)
							st.wHour = 0;
						break;
					case 'P':
						if (st.wHour < 12)
							st.wHour += 12;
						break;
					}
				}
				//st.wYear+=st.wYear<50?2000:1900;
				FILETIME ft{};
				SystemTimeToFileTime(&st, &ft);
				LocalFileTimeToFileTime(&ft, &Item->FindData.ftLastWriteTime);

				if ((nStrLen == NAME_DISP + 1 && *(Buf + NAME_DISP) == '.') ||
					(nStrLen == NAME_DISP + 2 && *(Buf + NAME_DISP + 1) == '.'))
				{
					continue;
				}

				//BUGBUG: replace with LongUnicodeString
				if (*Prefix)
				{
					lstrcpy(Item->FindData.cFileName, Prefix);
				}
				if (CP_Check == CodePages::NotChecked)
				{
					//TODO: Check if it's UTF8?
					const int iUtf = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Buf, -1, Utf8, ARRAYSIZE(Utf8));
					//BUGBUG: ?? OEM ?
					const int iWide = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, Buf, -1, Wide, ARRAYSIZE(Wide));
					if (iUtf > 0 && iWide > 0 && iUtf < iWide)
					{
						CP_Check = CodePages::UTF8;
					}
				}

				bool copied = false;
				const int cchMaxLen = ARRAYSIZE(Item->FindData.cFileName) - nPrefixLen;
				if (CP_Check == CodePages::UTF8)
				{
					const int iUtf = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Buf, -1, Utf8, ARRAYSIZE(Utf8));
					if (iUtf > static_cast<int>(NAME_DISP))
					{
						const int iWide = WideCharToMultiByte(CP_UTF8, 0, Utf8 + NAME_DISP, -1,
							Item->FindData.cFileName + nPrefixLen, cchMaxLen, nullptr, nullptr);
						copied = (iWide > 0 && iWide < cchMaxLen);
					}
				}
				if (!copied)
				{
					lstrcpyn(Item->FindData.cFileName + nPrefixLen, Buf + NAME_DISP, cchMaxLen);
				}

				switch (CP_Check)
				{
				case CodePages::ANSI:
					Item->UserData = CP_ACP; break;
				case CodePages::OEM:
					Item->UserData = CP_OEMCP; break;
				case CodePages::UTF8:
					Item->UserData = CP_UTF8; break;
				case CodePages::NotChecked:
					Item->UserData = 0;
				}

				if (Compare(Buf + ID_DIR_DISP, DirOrJunc) > 0)
				{
					Item->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				}
				else
				{
					FARINT64 nFileSize;
					nFileSize.i64 = AtoI(Buf + SIZE_DISP, 16);
					Item->FindData.nFileSizeLow = nFileSize.Part.LowPart;
					Item->FindData.nFileSizeHigh = nFileSize.Part.HighPart;
				}

				//lstrcpy(Info->Description, Descrip); -- Maks put VolumeID in Description
				char* Temp = Pos; // support of embedded descriptions inside DIR file
				if (GetS(Buf, 4095))
				{
					static const char* MaskList[] = { " @", nullptr };
					if (Compare(Buf, MaskList) > 0)
					{
						lstrcpyn(Info->Description, Buf + 2, 256);
					}
					else
					{
						Pos = Temp;
					}
				}
				return GETARC_SUCCESS;
			}
		}
		return GETARC_EOF;
	}

	bool SetFileHandle(HANDLE handle)
	{
		if (Handle != nullptr)
		{
			_ASSERTE(Handle == nullptr);
			return false;
		}
		if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		Handle = handle;

		return true;
	}

	BOOL closeArchive()
	{
		if (Data)
			UnmapViewOfFile(Data);
		Data = nullptr;

		if (MapHandle && (MapHandle != INVALID_HANDLE_VALUE))
			CloseHandle(MapHandle);
		MapHandle = nullptr;

		if (Handle && (Handle != INVALID_HANDLE_VALUE))
			CloseHandle(Handle);
		Handle = nullptr;

		DirInfo* p = this;
		if (p == gpCur)
			gpCur = nullptr;
		free(p);

		return true;
	}
};



BOOL WINAPI _export IsArchive(char* Name, const unsigned char* Data, int DataSize)
{
	_ASSERTE(gpCur != nullptr);
	static const char* ID[] = { " Volume in drive ", "Queued to drive ", " Том в устройстве ", nullptr };
	const char* strData = reinterpret_cast<const char*>(Data);
	const int cmpRc = gpCur->Compare(strData, ID, true, DataSize);
	return (cmpRc > 0);
}

BOOL WINAPI _export OpenArchive(char* Name, int* Type)
{
	_ASSERTE(gpCur == nullptr);
	gpCur = DirInfo::Create();
	if (!gpCur)
		return FALSE;
	if (!gpCur->SetFileHandle(CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr)))
		return FALSE;
	return gpCur->openArchive(Type);
}

int WINAPI _export GetArcItem(struct PluginPanelItem* Item, struct ArcItemInfo* Info)
{
	if (!gpCur)
		return GETARC_EOF;
	return gpCur->getArcItem(Item, Info);
}

BOOL WINAPI _export CloseArchive(struct ArcInfo* Info)
{
	if (gpCur)
	{
		gpCur->closeArchive();
	}
	return TRUE;
}

//??эту функцию надо бы выбросить, чтобы модуль не появлялся в меню MultiArc,
//но тогда нарушается работа самого MultiArc :(
BOOL WINAPI _export GetFormatName(int Type, char* FormatName, char* DefaultExt)
{
	if (Type == 0)
	{
		lstrcpy(FormatName, "DIR");
		lstrcpy(DefaultExt, "dir");
		return true;
	}
	return false;
}

BOOL WINAPI _export GetDefaultCommands(int Type, int Command, char* Dest)
{
	if (Type == 0)
	{
		if (Command == 0)
		{
			//extract (распаковка)
			lstrcpyA(Dest, "dir.fmt.exe x %%A %%L");
		}
		else if (Command == 1)
		{
			//extract without path (распаковка без путей)
			lstrcpyA(Dest, "dir.fmt.exe e %%A %%L");
		}
		else
		{
			*Dest = 0;
		}
		return true;
	}
	return false;
}

#include "ModuleDef.h"

int nLastItemIndex = -1;

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE* storage, StorageGeneralInfo* info)
{
	nLastItemIndex = -1;
	gpCur = DirInfo::Create();
	if (!gpCur)
		return SOR_INVALID_FILE;
	if (!gpCur->SetFileHandle(CreateFileW(params.FilePath, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr)))
		return SOR_INVALID_FILE;
	int Type = 0;
	if (gpCur->openArchive(&Type))
	{
		*storage = static_cast<HANDLE>(gpCur);
		lstrcpyW(info->Format, L"Dir");
		lstrcpyW(info->Compression, L"");
		lstrcpyW(info->Comment, L"");
		return SOR_SUCCESS;
	}
	gpCur->closeArchive();
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	DirInfo* p = static_cast<DirInfo*>(storage);
	if (p)
		p->closeArchive();
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	int iRc = GET_ITEM_NOMOREITEMS;
	_ASSERTE(item_index > nLastItemIndex);
	DirInfo* p = static_cast<DirInfo*>(storage);
	nLastItemIndex = item_index;
	PluginPanelItem Item{};
	ArcItemInfo Info{};
	if (p->getArcItem(&Item, &Info) == GETARC_SUCCESS)
	{
		item_info->Attributes = Item.FindData.dwFileAttributes;
		LARGE_INTEGER li; li.LowPart = Item.FindData.nFileSizeLow; li.HighPart = Item.FindData.nFileSizeHigh;
		item_info->Size = li.QuadPart;
		item_info->CreationTime = Item.FindData.ftLastWriteTime;
		item_info->ModificationTime = Item.FindData.ftLastWriteTime;
		MultiByteToWideChar(p->IsUtf8File() ? CP_UTF8 : CP_OEMCP, 0,
			Item.FindData.cFileName, -1, item_info->Path, ARRAYSIZE(item_info->Path));
		iRc = GET_ITEM_OK;
	}
	return iRc;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {C0651C24-3DCA-441C-8A42-C73664F942CF}
static const GUID MODULE_GUID = { 0xc0651c24, 0x3dca, 0x441c, { 0x8a, 0x42, 0xc7, 0x36, 0x64, 0xf9, 0x42, 0xcf } };;

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;

	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
