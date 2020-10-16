// Dir_fmt_exe.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <vector>
#include <tchar.h>
#include <windows.h>

#include "\VCProject\MLib\farsdk\ascii\plugin.hpp"
#include "\VCProject\MLib\farsdk\ascii\fmt.hpp"

int help();
int fail(int nCode);
BOOL WINAPI _export OpenArchive(char *Name, int *Type);
int WINAPI _export GetArcItem(struct PluginPanelItem *Item, struct ArcItemInfo *Info);
BOOL WINAPI _export CloseArchive(struct ArcInfo *Info);
int CreateItem ( PluginPanelItem& Item );

BOOL lbNoPath=FALSE;

int main(int argc, char* argv[])
{
	int nRc = 0;
	printf ("Arg count: %i\n", argc);
	for (int i=0; i<argc; i++) {
		printf(argv[i]); printf("\n");
	}

	if (argc<=3)
		return help();

	if (strcmp(argv[1],"e") && strcmp(argv[1],"x"))
		return help();
	lbNoPath = strcmp(argv[1],"e")==0;

	int nType=0;
	if (!OpenArchive(argv[2], &nType))
		return fail(101);

#ifdef _DEBUG
	if (!IsDebuggerPresent())
		MessageBox(NULL, "Wait", "Dir.Fmt", 0);
#endif

	HANDLE hList = CreateFile(argv[3], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hList==INVALID_HANDLE_VALUE)
		return fail(102);
	DWORD dwSize, dwHigh=0;
	dwSize = GetFileSize(hList, &dwHigh);
	if (dwHigh!=0 || dwSize==-1 || dwSize==0) return fail(103);
	TCHAR* pszList = (TCHAR*)calloc(dwSize+1, sizeof(TCHAR));
	if (!pszList) return fail(104);
	if (!ReadFile(hList, pszList, dwSize, &dwSize, 0)) nRc = 105;
	OemToCharBuff(pszList, pszList, dwSize);
	CloseHandle(hList);

	SetConsoleOutputCP(1251);

	if (nRc==0)
	{
		std::vector<TCHAR*> psNeeds;
		TCHAR *psz = pszList;
		TCHAR *pszLine = NULL;
		while ((pszLine=_tcschr(psz, _T('\r')))) {
			*pszLine = 0;
			if (pszLine[1]==_T('\n')) pszLine+=2; else pszLine++;
			psNeeds.push_back(psz);
			psz = pszLine;
		}


		PluginPanelItem Item; memset(&Item, 0, sizeof(Item));
		ArcItemInfo Info; memset(&Info, 0, sizeof(Info));

		//MessageBox(NULL, "Wait", "Dir.fmt.exe", MB_OK);

		//TCHAR szFromDir[MAX_PATH+1]; szFromDir[0]=0;
		while (GETARC_SUCCESS==GetArcItem(&Item, &Info)) {
			OemToCharBuff(Item.FindData.cFileName, Item.FindData.cFileName, strlen(Item.FindData.cFileName));
			if (Item.FindData.cFileName[0]==_T('<')) continue; // <VOLUMELABEL:...>
			//if (Item.FindData.dwFileAttributes&&FILE_ATTRIBUTE_DIRECTORY) continue;
			if (strchr(Item.FindData.cFileName,'\\')) {
				int nDbg=0;
			}

			for (std::vector<TCHAR*>::iterator iter=psNeeds.begin(); iter!=psNeeds.end(); iter++)
			{
				TCHAR *psTest = *iter;
				BOOL lbOurFile=FALSE;
				if (_tcsnicmp(psTest, Item.FindData.cFileName, strlen(psTest))==0)
				{
					lbOurFile=TRUE;
					if (Item.FindData.dwFileAttributes&&FILE_ATTRIBUTE_DIRECTORY) {
						psTest+=_tcslen(psTest);
						if (*(psTest-1)!=_T('\\')) {
							if (psTest[1]!='\n') {
								nRc = 200;
								break;
							}
							*psTest=_T('\\');
							psTest[1] = 0;
						}
					}
				}

				if (lbOurFile) {
					nRc = CreateItem(Item);
					break;
				}
			}
		}

		CloseArchive(NULL);
	}
	free(pszList);
	printf("Rc: %i\n", nRc);
	SetConsoleOutputCP(866);
	return nRc;
}

int help()
{
	printf ("Dir.fmt.exe {e|x} \"dir_file\" \"file_list\"\n");
	return 100;
}

int fail(int nCode)
{
	printf ("Can't extract!\n");
	return nCode;
}

int CreateItem ( PluginPanelItem& Item )
{
	int nRc = 0;
	DWORD dwLastError=0;

	TCHAR* pszFile = Item.FindData.cFileName;
	if (lbNoPath) {
		TCHAR *pszSlash = _tcsrchr(pszFile, _T('\\'));
		if (pszSlash)
			pszFile = pszSlash+1;
	}

	if (Item.FindData.dwFileAttributes&&FILE_ATTRIBUTE_DIRECTORY) {
		printf("Creating folder: %s", pszFile);
		CreateDirectory(pszFile, NULL);
		printf("\n");
	} else {
		printf("Creating file: %s", pszFile);
		HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL/*Item.FindData.dwFileAttributes*/, NULL);

		if (hFile==INVALID_HANDLE_VALUE) {
			nRc = 110;
		} else {
			SetFilePointer(hFile, Item.FindData.nFileSizeLow, (long*)&Item.FindData.nFileSizeHigh, FILE_BEGIN);
			SetEndOfFile(hFile);
			if (!SetFileTime(hFile, NULL, NULL, &Item.FindData.ftLastWriteTime))
				dwLastError = GetLastError();
			CloseHandle(hFile);
			if (!SetFileAttributes(pszFile, Item.FindData.dwFileAttributes|FILE_ATTRIBUTE_HIDDEN))
				dwLastError = GetLastError();
		}
		printf("\n");
	}
	return nRc;
}
