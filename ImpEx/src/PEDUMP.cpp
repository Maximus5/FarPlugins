//==================================
// PEDUMP - Matt Pietrek 1994-2001
// FILE: PEDUMP.CPP
//==================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "objdump.h"
#include "exedump.h"
#include "dbgdump.h"
#include "libdump.h"
#include "romimage.h"
#include "common.h"
#include "extrnvar.h"
#include "resdump.h"

// Global variables set here, and used elsewhere in the program
BOOL fShowRelocations = TRUE;
BOOL fShowRawSectionData = TRUE;
BOOL fShowSymbolTable = TRUE;
BOOL fShowLineNumbers = TRUE;
BOOL fShowIATentries = TRUE;
BOOL fShowPDATA = TRUE;
BOOL fShowResources = TRUE;
PBYTE g_pMappedFileBase = 0;
ULARGE_INTEGER g_FileSize = {{0,0}};
PIMAGE_NT_HEADERS32 gpNTHeader32 = NULL;
PIMAGE_NT_HEADERS64 gpNTHeader64 = NULL;
bool g_bIs64Bit = false;
bool g_bUPXed = false;


/*
char HelpText[] = 
"PEDUMP - Win32/Win64 EXE/OBJ/LIB/DBG file dumper - 2001 Matt Pietrek\n\n"
"Syntax: PEDUMP [switches] filename\n\n"
"  /A    include everything in dump\n"
"  /B    show base relocations\n"
"  /H    include hex dump of sections\n"
"  /I    include Import Address Table thunk addresses\n"
"  /L    include line number information\n"
"  /P    include PDATA (runtime functions)\n"
"  /R    include detailed resources (stringtables and dialogs)\n"
"  /S    show symbol table\n";
*/

//
// Open up a file, memory map it, and call the appropriate dumping routine
//
bool DumpFile(MPanelItem* pRoot, LPCTSTR filename, bool abForceDetect)
{
	bool lbSucceeded = false;
    HANDLE hFile;
    HANDLE hFileMapping;
    PIMAGE_DOS_HEADER dosHeader;
    
	gpNTHeader32 = NULL;
	gpNTHeader64 = NULL;
	g_bIs64Bit = false;
	g_bUPXed = false;

	// Очистка переменных!
	//g_pStrResEntries = 0;
	g_pDlgResEntries = 0;
	//g_cStrResEntries = 0;
	g_cDlgResEntries = 0;
	g_pMiscDebugInfo = 0;
	g_pCVHeader = 0;
	g_pCOFFHeader = 0;
	g_pCOFFSymbolTable = 0;
	PszLongnames = 0;


	// Открываем файл
    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                    
	if ( hFile == INVALID_HANDLE_VALUE )
		hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        merror(_T("Couldn't open file with CreateFile()\n%s\nErrCode=%u"), filename, GetLastError());
        return false;
    }

	g_FileSize.LowPart = GetFileSize(hFile, &g_FileSize.HighPart);
	if (!abForceDetect)
	{
		MEMORYSTATUSEX mem = {sizeof(MEMORYSTATUSEX)};
		GlobalMemoryStatusEx(&mem);
		if (g_FileSize.HighPart || 
			(g_FileSize.LowPart > mem.ullAvailPhys && g_FileSize.LowPart > mem.ullAvailPageFile))
		{
			// выходим по тихому, ибо abForceDetect == false
			// Too large file
			CloseHandle(hFile);
			return false;			
		}
	}

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if ( hFileMapping == 0 )
    {
        CloseHandle(hFile);
        merror(_T("Couldn't open file mapping with CreateFileMapping()\n%s\nErrCode=%u"), filename, GetLastError());
        return false;
    }

    g_pMappedFileBase = (PBYTE)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);
    if ( g_pMappedFileBase == 0 )
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        merror(_T("Couldn't map view of file with MapViewOfFile()\n%s\nErrCode=%u"), filename, GetLastError());
        return false;
    }

    pRoot->AddText(_T("Dump of file "));
	pRoot->AddText(filename);
	pRoot->AddText(_T("\n\n"));
    
    dosHeader = (PIMAGE_DOS_HEADER)g_pMappedFileBase;
	PIMAGE_FILE_HEADER pImgFileHdr = (PIMAGE_FILE_HEADER)g_pMappedFileBase;

	__try
	{
		if ( dosHeader->e_magic == IMAGE_DOS_SIGNATURE )
		{
			lbSucceeded = DumpExeFile( pRoot, dosHeader );
		}
		else if ( dosHeader->e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE )
		{
			lbSucceeded = DumpDbgFile( pRoot, (PIMAGE_SEPARATE_DEBUG_HEADER)g_pMappedFileBase );
		}
		else if ( IsValidMachineType(pImgFileHdr->Machine) )
		{
			if ( 0 == pImgFileHdr->SizeOfOptionalHeader )	// 0 optional header
			{
				lbSucceeded = DumpObjFile( pRoot, pImgFileHdr );					// means it's an OBJ
			}
			else if ( 	pImgFileHdr->SizeOfOptionalHeader
						== IMAGE_SIZEOF_ROM_OPTIONAL_HEADER )
			{
				lbSucceeded = DumpROMImage( pRoot, (PIMAGE_ROM_HEADERS)pImgFileHdr );
			}
		}
		else if ( 0 == strncmp((char *)g_pMappedFileBase, IMAGE_ARCHIVE_START,
                                       					IMAGE_ARCHIVE_START_SIZE) )
		{
			lbSucceeded = DumpLibFile( pRoot, g_pMappedFileBase );
		}
		else
		{
    		if (abForceDetect)
        		merror(_T("Unrecognized file format\n"));
			lbSucceeded = false;
		}
	}__except(EXCEPTION_EXECUTE_HANDLER){
		
		pRoot->AddText(_T("\n\n!!! Exception !!!\n"));

		if (abForceDetect)
			merror(_T("Exception, while dumping\n"));

		lbSucceeded = false;
	}

    UnmapViewOfFile(g_pMappedFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
    
    return lbSucceeded;
}

//
// process all the command line arguments and return a pointer to
// the filename argument.
//
/*
PSTR ProcessCommandLine(int argc, char *argv[])
{
    int i;
    
    for ( i=1; i < argc; i++ )
    {
        _strupr(argv[i]);
        
        // Is it a switch character?
        if ( (argv[i][0] == '-') || (argv[i][0] == '/') )
        {
            if ( argv[i][1] == 'A' )
            {
                fShowRelocations = TRUE;
                fShowRawSectionData = TRUE;
                fShowSymbolTable = TRUE;
                fShowLineNumbers = TRUE;
                fShowIATentries = TRUE;
                fShowPDATA = TRUE;
				fShowResources = TRUE;
            }
            else if ( argv[i][1] == 'H' )
                fShowRawSectionData = TRUE;
            else if ( argv[i][1] == 'L' )
                fShowLineNumbers = TRUE;
            else if ( argv[i][1] == 'P' )
                fShowPDATA = TRUE;
            else if ( argv[i][1] == 'B' )
                fShowRelocations = TRUE;
            else if ( argv[i][1] == 'S' )
                fShowSymbolTable = TRUE;
            else if ( argv[i][1] == 'I' )
                fShowIATentries = TRUE;
            else if ( argv[i][1] == 'R' )
                fShowResources = TRUE;
        }
        else    // Not a switch character.  Must be the filename
        {
            return argv[i];
        }
    }

	return NULL;
}
*/

/*
int main(int argc, char *argv[])
{
    PSTR filename;
    
    if ( argc == 1 )
    {
        printf( HelpText );
        return 1;
    }
    
    filename = ProcessCommandLine(argc, argv);
    if ( filename )
        DumpFile( filename );

#ifdef _DEBUG
	if (IsDebuggerPresent()) {
		printf("Press any key..."); _getch();
	}
#endif

    return 0;
}
*/
