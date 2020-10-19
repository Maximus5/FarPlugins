// Dir_fmt_exe.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <tuple>
#include <memory>
#include <vector>
#include <tchar.h>
#include <windows.h>

#include "plugin.hpp"
#include "fmt.hpp"

int help();
int fail(int nCode);
BOOL WINAPI _export OpenArchive(char *Name, int *Type);
int WINAPI _export GetArcItem(struct PluginPanelItem *Item, struct ArcItemInfo *Info);
BOOL WINAPI _export CloseArchive(struct ArcInfo *Info);
int CreateItem ( PluginPanelItem& Item );

enum class WorkMode
{
	ExtractNoPath,
	ExtractWithPath,
	Information,
	ListFiles,
};

WorkMode work_mode;

int main(int argc, char* argv[])
{
	int nRc = 0;

	printf("Arg count: %i\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	if (argc <= 2)
		return help();

	const auto* mode = argv[1];
	if (strcmp(mode, "e") == 0)
	{
		if (argc <= 3)
			return help();
		work_mode = WorkMode::ExtractNoPath;
	}
	else if (strcmp(mode, "x") == 0)
	{
		if (argc <= 3)
			return help();
		work_mode = WorkMode::ExtractWithPath;
	}
	else if (strcmp(mode, "i") == 0)
	{
		work_mode = WorkMode::Information;
	}
	else if (strcmp(mode, "l") == 0)
	{
		work_mode = WorkMode::ListFiles;
	}
	else
	{
		return help();
	}

	int nType = 0;
	if (!OpenArchive(argv[2], &nType))
		return fail(101);

#ifdef _DEBUG
	if (!IsDebuggerPresent())
		MessageBox(nullptr, "Wait", "Dir.Fmt", 0);
#endif


	std::vector<TCHAR*> psNeeds;
	std::unique_ptr<TCHAR[]> pszList;
	HANDLE hList = INVALID_HANDLE_VALUE;
	while (work_mode == WorkMode::ExtractNoPath || work_mode == WorkMode::ExtractWithPath) {
		hList = CreateFile(argv[3], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0);
		if (hList == INVALID_HANDLE_VALUE) {
			nRc = 102; break;
		}
		DWORD dwSize, dwHigh = 0;
		dwSize = GetFileSize(hList, &dwHigh);
		if (dwHigh != 0 || dwSize == static_cast<DWORD>(-1) || dwSize == 0) {
			nRc = 103; break;
		}
		
		// ReSharper disable once CppJoinDeclarationAndAssignment
		pszList = std::make_unique<TCHAR[]>(dwSize + 1);
		if (!pszList) {
			nRc = 104; break;
		}
		if (!ReadFile(hList, pszList.get(), dwSize, &dwSize, 0)) {
			nRc = 105; break;
		}

		OemToCharBuff(pszList.get(), pszList.get(), dwSize);

		TCHAR* psz = pszList.get();
		TCHAR* pszLine = nullptr;
		while ((pszLine = _tcschr(psz, _T('\r')))) {
			*pszLine = 0;
			if (pszLine[1] == _T('\n')) pszLine += 2; else pszLine++;
			psNeeds.push_back(psz);
			psz = pszLine;
		}

		break;
	}

	if (hList != INVALID_HANDLE_VALUE)
		CloseHandle(hList);

	const auto prevCp = GetConsoleOutputCP();

	uint32_t fileCount = 0;
	uint32_t dirCount = 0;
	if (nRc == 0)
	{
		SetConsoleOutputCP(1251);

		PluginPanelItem item{};
		ArcItemInfo info{};

		//MessageBox(nullptr, "Wait", "Dir.fmt.exe", MB_OK);

		wchar_t utfBuffer[MAX_PATH + 32];
		bool switchedToUtf8 = false;
		HANDLE hStdOut = NULL;
		//TCHAR szFromDir[MAX_PATH+1]; szFromDir[0]=0;
		while (GETARC_SUCCESS == GetArcItem(&item, &info)) {
			if (item.UserData == CP_UTF8)
			{
				if (!hStdOut)
				{
					hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
					switchedToUtf8 = true;
				}
			}

			if (item.UserData == CP_OEMCP)
			{
				OemToCharBuff(item.FindData.cFileName, item.FindData.cFileName, strlen(item.FindData.cFileName));
			}
			
			if (item.FindData.cFileName[0] == _T('<'))
				continue; // <VOLUMELABEL:...>
			//if (Item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			if (strchr(item.FindData.cFileName, '\\')) {
				std::ignore = 0;
			}

			if (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				++dirCount;
			else
				++fileCount;

			if (work_mode == WorkMode::ListFiles) {
				if (switchedToUtf8 && hStdOut && hStdOut != INVALID_HANDLE_VALUE && item.UserData == CP_UTF8)
				{
					swprintf_s(utfBuffer, L"%c '", (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 'D' : 'F');
					MultiByteToWideChar(CP_UTF8, 0, item.FindData.cFileName, -1, utfBuffer + 3, MAX_PATH + 1);
					wcscat_s(utfBuffer, L"'\n");
					DWORD written = 0;
					WriteConsoleW(hStdOut, utfBuffer, wcslen(utfBuffer), &written, nullptr);
				}
				else
				{
					printf("%c '%s'\n", (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 'D' : 'F',
						item.FindData.cFileName);
				}
			}

			// ReSharper disable once IdentifierTypo
			for (auto* psTest : psNeeds)
			{
				BOOL lbOurFile = FALSE;
				if (_tcsnicmp(psTest, item.FindData.cFileName, strlen(psTest)) == 0)
				{
					lbOurFile = TRUE;
					if (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						psTest += _tcslen(psTest);
						if (*(psTest - 1) != _T('\\')) {
							if (psTest[1] != _T('\n')) {
								nRc = 200;
								break;
							}
							*psTest = _T('\\');
							psTest[1] = 0;
						}
					}
				}

				if (lbOurFile) {
					nRc = CreateItem(item);
					break;
				}
			}
		}

		CloseArchive(nullptr);
	}

	if (work_mode == WorkMode::Information || work_mode == WorkMode::ListFiles)
		printf("Directories: %u\nFiles: %u\n", dirCount, fileCount);

	if (nRc != 0)
		printf("Can't extract\nRc: %i\n", nRc);
	else
		printf("Rc: %i\n", nRc);
	SetConsoleOutputCP(prevCp);
	return nRc;
}

int help()
{
	printf("Dir.fmt.exe {e|x} \"dir_file\" \"file_list\"\n");
	return 100;
}

int fail(int nCode)
{
	printf("Can't extract!\nRc: %i\n", nCode);
	return nCode;
}

int CreateItem(PluginPanelItem& Item)
{
	int nRc = 0;
	// ReSharper disable once CppEntityAssignedButNoRead
	DWORD dwLastError = 0;

	TCHAR* pszFile = Item.FindData.cFileName;
	if (work_mode == WorkMode::ExtractNoPath) {
		TCHAR* pszSlash = _tcsrchr(pszFile, _T('\\'));
		if (pszSlash)
			pszFile = pszSlash + 1;
	}

	if (Item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		printf("Creating folder: %s", pszFile);
		CreateDirectory(pszFile, nullptr);
		printf("\n");
	}
	else {
		printf("Creating file: %s", pszFile);
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL/*Item.FindData.dwFileAttributes*/, nullptr);

		if (hFile == INVALID_HANDLE_VALUE) {
			nRc = 110;
		}
		else {
			SetFilePointer(hFile, Item.FindData.nFileSizeLow, reinterpret_cast<PLONG>(&Item.FindData.nFileSizeHigh), FILE_BEGIN);
			SetEndOfFile(hFile);
			if (!SetFileTime(hFile, nullptr, nullptr, &Item.FindData.ftLastWriteTime))
				dwLastError = GetLastError();
			CloseHandle(hFile);
			if (!SetFileAttributes(pszFile, Item.FindData.dwFileAttributes | FILE_ATTRIBUTE_HIDDEN))
				dwLastError = GetLastError();
		}
		printf("\n");
	}

	std::ignore = dwLastError;
	return nRc;
}
