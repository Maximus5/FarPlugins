// Convert.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>

int Oem2Utf(int nCP, char* pszFile)
{
	HANDLE hFile = CreateFile(pszFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Failed to open file for reading: %s\n", pszFile);
		return 101;
	}
	UINT nSize = GetFileSize(hFile, NULL);
	char* pszBuf = (char*)calloc(nSize*4+6,1);
	wchar_t* pwszBuf = (wchar_t*)calloc(nSize+1,2);
	if (!pszBuf || !pwszBuf) {
		printf("Memory allocation error!\n");
		return 102;
	}
	DWORD nBytes = 0;
	if (!ReadFile(hFile, pszBuf, nSize, &nBytes, 0) || nBytes != nSize) {
		printf("File reading error!\n");
		return 103;
	}
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	
	if (!MultiByteToWideChar(nCP, 0, pszBuf, nSize+1, pwszBuf, nSize+1)) {
		printf("Conversion %s -> UNICODE failed!\n", nCP==CP_OEMCP ? "OEM" : "ANSI");
		return 104;
	}
	pszBuf[0] = (char)0xEF; pszBuf[1] = (char)0xBB; pszBuf[2] = (char)0xBF;
	if (!WideCharToMultiByte(CP_UTF8,0,pwszBuf,nSize+1,pszBuf+3,nSize*4,0,0)) {
		printf("Conversion UNICODE -> UTF-8 failed!\n");
		return 105;
	}
	
	nSize = strlen(pszBuf);
	if (!WriteFile(hFile, pszBuf, nSize, &nBytes, 0) || nSize != nBytes) {
		printf("File writing error!\n");
		return 106;
	}
	CloseHandle(hFile);

	return 0;
}

int Utf2Oem(int nCP, char* pszFile)
{
	HANDLE hFile = CreateFile(pszFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Failed to open file for reading: %s\n", pszFile);
		return 101;
	}
	UINT nSize = GetFileSize(hFile, NULL);
	char* pszBuf = (char*)calloc(nSize*4+6,1);
	wchar_t* pwszBuf = (wchar_t*)calloc(nSize+1,2);
	if (!pszBuf || !pwszBuf) {
		printf("Memory allocation error!\n");
		return 102;
	}
	DWORD nBytes = 0;
	if (!ReadFile(hFile, pszBuf, nSize, &nBytes, 0) || nBytes != nSize) {
		printf("File reading error!\n");
		return 103;
	}
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	
	// ќтрезать UTF-8 BOM
	char* pszUTF = pszBuf;
	if (pszBuf[0] == (char)0xEF && pszBuf[1] == (char)0xBB && pszBuf[2] == (char)0xBF) {
		pszUTF+=3;
	} else {
		// —читаем, что файл уже сконвертирован!
		CloseHandle(hFile);
		return  0;
	}

	if (!MultiByteToWideChar(CP_UTF8, 0, pszUTF, -1, pwszBuf, nSize+1)) {
		printf("Conversion UTF-8 -> UNICODE failed!\n");
		return 104;
	}
	if (!WideCharToMultiByte(nCP, 0, pwszBuf, -1, pszBuf, nSize*4, 0, 0)) {
		printf("Conversion UNICODE -> %s failed!\n", nCP==CP_OEMCP ? "OEM" : "ANSI");
		return 104;
	}

	nSize = strlen(pszBuf);
	if (!WriteFile(hFile, pszBuf, nSize, &nBytes, 0) || nSize != nBytes) {
		printf("File writing error!\n");
		return 106;
	}
	SetEndOfFile(hFile);
	CloseHandle(hFile);
	
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("Convert.exe <OEM/ANSI> <OEM text file>\n");
		printf("  Loads OEM(ANSI) text, converts it to UTF-8, and saves INPLACE!\n");
		printf("Convert.exe <UTF2OEM/UTF2ANSI> <UTF-8 text file>\n");
		printf("  Loads UTF-8 text, converts it to OEM(ANSI), and saves INPLACE!\n");
		return 100;
	}

	

	if (stricmp(argv[1],"UTF2OEM")==0)
		return Utf2Oem(CP_OEMCP, argv[2]);
	if (stricmp(argv[1],"UTF2ANSI")==0 || stricmp(argv[1],"UTF2ACP")==0)
		return Utf2Oem(CP_ACP, argv[2]);	

	int nCP = CP_OEMCP;
	if (stricmp(argv[1],"ANSI")==0 || stricmp(argv[1],"ACP")==0)
		nCP = CP_ACP;
	return Oem2Utf(nCP, argv[2]);
}
