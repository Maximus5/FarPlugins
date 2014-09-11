//==================================
// PEDUMP - Matt Pietrek 1994-2001
// FILE: EXEDUMP.CPP
//==================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <delayimp.h>
#include <wintrust.h>
#include <Dbghelp.h>
#pragma hdrstop
#include "common.h"
#include "symboltablesupport.h"
#include "COFFSymbolTable.h"
#include "resdump.h"
#include "extrnvar.h"
#include "OS2.h"

static const TCHAR cszLibrary[] = _T("Library");
static const TCHAR cszOrdinal[] = _T("#Ord");
static const TCHAR cszHint[] = _T("Hint");
static const TCHAR cszEntryPoint[] = _T("Entry");
static const TCHAR cszCertRevTitle[] = _T("Revis");
static const TCHAR cszCertTypeTitle[] = _T("Certificate type");
static const TCHAR cszRVA[] = _T("RVA");
static const TCHAR cszSize[] = _T("Size");
static const TCHAR cszCount[] = _T("Count");
static const TCHAR cszAddress[] = _T("Address");

//============================================================================

// Bitfield values and names for the DllCharacteritics flags
WORD_FLAG_DESCRIPTIONS DllCharacteristics[] = 
{
	{ IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE /*0x0040*/, "DYNAMIC_BASE (ASLR)" }, // ASLR
	{ IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY /*0x0080*/, "FORCE_INTEGRITY" }, // Code Integrity Image
	{ IMAGE_DLLCHARACTERISTICS_NX_COMPAT /*0x0100*/, "NX_COMPAT" }, // Image is NX compatible
	{ IMAGE_DLLCHARACTERISTICS_NO_ISOLATION /*0x0200*/, "NO_ISOLATION" }, // Image understands isolation and doesn't want it
	{ IMAGE_DLLCHARACTERISTICS_NO_SEH /*0x0400*/, "NO_SEH" }, // Image does not use SEH.  No SE handler may reside in this image
	{ IMAGE_DLLCHARACTERISTICS_NO_BIND /*0x0800*/, "NO_BIND" }, // Do not bind this image.
	{ IMAGE_DLLCHARACTERISTICS_WDM_DRIVER /*0x2000*/, "WDM_DRIVER" }, // Driver uses WDM model
	{ IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE /*0x8000*/, "TERMINAL_SERVER_AWARE" },
// Old, obsolete flags
//      IMAGE_LIBRARY_PROCESS_INIT           0x0001     // Reserved.
//      IMAGE_LIBRARY_PROCESS_TERM           0x0002     // Reserved.
//      IMAGE_LIBRARY_THREAD_INIT            0x0004     // Reserved.
//      IMAGE_LIBRARY_THREAD_TERM            0x0008     // Reserved.
};

#define NUMBER_DLL_CHARACTERISTICS \
    (sizeof(DllCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

// Names of the data directory elements that are defined
char *ImageDirectoryNames[] = {
    "EXPORT",
	"IMPORT",
	"RESOURCE",
	"EXCEPTION",
	"SECURITY",
	"BASERELOC",
    "DEBUG",
	"ARCHITECTURE",
	"GLOBALPTR",
	"TLS",
	"LOAD_CONFIG",
    "BOUND_IMPORT",  		// These two entries added for NT 3.51
	"IAT",
	"DELAY_IMPORT",			// This entry added in NT 5 time frame
	"COM_DESCRPTR" };		// For the .NET runtime (previously called COM+ 2.0)

#define NUMBER_IMAGE_DIRECTORY_ENTRYS \
    (sizeof(ImageDirectoryNames)/sizeof(char *))

void DisplayDataDirectoryEntry(MPanelItem *pRoot, PSTR pszName, IMAGE_DATA_DIRECTORY & dataDirEntry )
{
	MPanelItem *pChild = pRoot;
	if (dataDirEntry.VirtualAddress) {
		pChild = pRoot->AddFile(pszName, dataDirEntry.Size);
		TCHAR szRVA[12], szSize[12];
		wsprintf(szRVA, _T("0x%08X"), dataDirEntry.VirtualAddress);
		wsprintf(szSize, _T("0x%08X"), dataDirEntry.Size);
		pChild->SetColumns(szRVA, szSize);
		//
		LPVOID ptr = NULL;
		if (g_bIs64Bit)
			ptr = GetPtrFromRVA(dataDirEntry.VirtualAddress, gpNTHeader64, g_pMappedFileBase);
		else
			ptr = GetPtrFromRVA(dataDirEntry.VirtualAddress, gpNTHeader32, g_pMappedFileBase);
		if (ptr)
			pChild->SetData((const BYTE*)ptr, dataDirEntry.Size);
	}
    pChild->printf( "  %-16s rva: %08X  size: %08X\n", pszName, dataDirEntry.VirtualAddress, dataDirEntry.Size );
}

//============================================================================
//
// Dump the IMAGE_OPTIONAL_HEADER from a PE file
//
template <class T> void DumpOptionalHeader(MPanelItem *pRoot,T* pImageOptionalHeader)		// 'T' is IMAGE_OPTIONAL_HEADER32/64
{
    UINT width = 30;
    char *s;
    UINT i;
 
	bool b64BitHeader = (IMAGE_NT_OPTIONAL_HDR64_MAGIC == pImageOptionalHeader->Magic);

	// „тобы при входе в корень сразу была видна "битность"
	pRoot->Root()->AddFlags(b64BitHeader ? _T("64BIT") : _T("32BIT"));

	//MPanelItem* pChild = pRoot->AddFolder(_T("Optional Header"));
	MPanelItem* pChild = pRoot->AddFile(b64BitHeader ? _T("PE64_Header.txt") : _T("PE32_Header.txt"));
	
	pChild->AddText(_T("<Optional Header>\n"));
	
	if (!ValidateMemory(pImageOptionalHeader, sizeof(*pImageOptionalHeader)))
	{
		pChild->SetErrorPtr(pImageOptionalHeader, sizeof(*pImageOptionalHeader)); return;
	}

	__try {
		pChild->printf("  %-*s%04X\n", width, "Magic", pImageOptionalHeader->Magic);
		pChild->printf("  %-*s%u.%02u\n", width, "linker version",
			pImageOptionalHeader->MajorLinkerVersion,
			pImageOptionalHeader->MinorLinkerVersion);
		pChild->printf("  %-*s%X\n", width, "size of code", pImageOptionalHeader->SizeOfCode);
		pChild->printf("  %-*s%X\n", width, "size of initialized data",
			pImageOptionalHeader->SizeOfInitializedData);
		pChild->printf("  %-*s%X\n", width, "size of uninitialized data",
			pImageOptionalHeader->SizeOfUninitializedData);
		pChild->printf("  %-*s%X\n", width, "entrypoint RVA",
			pImageOptionalHeader->AddressOfEntryPoint);
		pChild->printf("  %-*s%X\n", width, "base of code", pImageOptionalHeader->BaseOfCode);
		
		// 32/64 bit dependent code
		if ( b64BitHeader )
		{
			pChild->printf("  %-*s%I64X\n", width, "image base", pImageOptionalHeader->ImageBase);
		}
		else
		{
			// Can't refer to BaseOfData, since this field isn't in an IMAGE_NT_OPTIONAL_HDR64
			pChild->printf("  %-*s%X\n", width, "base of data", ((PIMAGE_OPTIONAL_HEADER32)pImageOptionalHeader)->BaseOfData );
			
			pChild->printf("  %-*s%X\n", width, "image base", pImageOptionalHeader->ImageBase);
		}
		// end of 32/64 bit dependent code

		pChild->printf("  %-*s%X\n", width, "section align",
			pImageOptionalHeader->SectionAlignment);
		pChild->printf("  %-*s%X\n", width, "file align", pImageOptionalHeader->FileAlignment);
		pChild->printf("  %-*s%u.%02u\n", width, "required OS version",
			pImageOptionalHeader->MajorOperatingSystemVersion,
			pImageOptionalHeader->MinorOperatingSystemVersion);
		pChild->printf("  %-*s%u.%02u\n", width, "image version",
			pImageOptionalHeader->MajorImageVersion,
			pImageOptionalHeader->MinorImageVersion);
		pChild->printf("  %-*s%u.%02u\n", width, "subsystem version",
			pImageOptionalHeader->MajorSubsystemVersion,
			pImageOptionalHeader->MinorSubsystemVersion);
		pChild->printf("  %-*s%X\n", width, "Win32 Version",
    		pImageOptionalHeader->Win32VersionValue);
		pChild->printf("  %-*s%X\n", width, "size of image", pImageOptionalHeader->SizeOfImage);
		pChild->printf("  %-*s%X\n", width, "size of headers",
				pImageOptionalHeader->SizeOfHeaders);
		pChild->printf("  %-*s%X\n", width, "checksum", pImageOptionalHeader->CheckSum);
		switch( pImageOptionalHeader->Subsystem )
		{
			case IMAGE_SUBSYSTEM_UNKNOWN: s = "UNKNOWN (0)"; break;
			case IMAGE_SUBSYSTEM_NATIVE: s = "Native"; break;
			case IMAGE_SUBSYSTEM_WINDOWS_GUI: s = "Windows GUI"; break;
			case IMAGE_SUBSYSTEM_WINDOWS_CUI: s = "Windows character"; break;
			case IMAGE_SUBSYSTEM_OS2_CUI: s = "OS/2 character"; break;
			case IMAGE_SUBSYSTEM_POSIX_CUI: s = "Posix character"; break;
			case IMAGE_SUBSYSTEM_NATIVE_WINDOWS: s = "Native Windows (Win9X driver)"; break;
			case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI: s = "Windows CE GUI"; break;
			case IMAGE_SUBSYSTEM_EFI_APPLICATION: s = "EFI_APPLICATION"; break;
			case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER: s = "EFI_BOOT_SERVICE_DRIVER"; break;
			case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER: s = "EFI_RUNTIME_DRIVER"; break;
			case IMAGE_SUBSYSTEM_EFI_ROM: s = "EFI_ROM";
			case IMAGE_SUBSYSTEM_XBOX: s = "XBOX";
			default: s = "unknown";
		}
		pChild->printf("  %-*s%04X (%s)\n", width, "Subsystem",
				pImageOptionalHeader->Subsystem, s);

	// Marked as obsolete in MSDN CD 9
		pChild->printf("  %-*s%04X\n", width, "DLL flags",
				pImageOptionalHeader->DllCharacteristics);
		WORD DllFlags = pImageOptionalHeader->DllCharacteristics;
		for ( i=0; i < NUMBER_DLL_CHARACTERISTICS; i++ )
		{
			if ( DllFlags & DllCharacteristics[i].flag )
			{
				pChild->printf( "  %-*s%04X %s\n", width, " ", DllCharacteristics[i].flag, DllCharacteristics[i].name );
				DllFlags &= ~DllCharacteristics[i].flag;
			}
		}
		if ( pImageOptionalHeader->DllCharacteristics )
		{
			if (DllFlags)
				pChild->printf( "  %-*s%04X %s\n", width, " ", DllFlags, "Unknown flags" );
			//else
			//	pChild->printf("\n");
		}

		PSTR pszSizeFmtString;

		if ( b64BitHeader )
			pszSizeFmtString = "  %-*s%I64X\n";
		else
			pszSizeFmtString = "  %-*s%X\n";

		pChild->printf( pszSizeFmtString, width, "stack reserve size",
			pImageOptionalHeader->SizeOfStackReserve);
		pChild->printf( pszSizeFmtString, width, "stack commit size",
			pImageOptionalHeader->SizeOfStackCommit);
		pChild->printf( pszSizeFmtString, width, "heap reserve size",
			pImageOptionalHeader->SizeOfHeapReserve);
		pChild->printf( pszSizeFmtString, width, "heap commit size",
			pImageOptionalHeader->SizeOfHeapCommit);

	#if 0
	// Marked as obsolete in MSDN CD 9
		pChild->printf("  %-*s%08X\n", width, "loader flags",
			pImageOptionalHeader->LoaderFlags);

		for ( i=0; i < NUMBER_LOADER_FLAGS; i++ )
		{
			if ( pImageOptionalHeader->LoaderFlags & 
				 LoaderFlags[i].flag )
				pChild->printf( "  %s", LoaderFlags[i].name );
		}
		if ( pImageOptionalHeader->LoaderFlags )
			pChild->printf("\n");
	#endif

		pChild->printf("  %-*s%X\n", width, "RVAs & sizes",
			pImageOptionalHeader->NumberOfRvaAndSizes);

		pChild->AddText(_T("\n"));
	}__except(EXCEPTION_EXECUTE_HANDLER){
		pChild->SetError(SZ_DUMP_EXCEPTION);
	}


	// ***************
	pChild = pRoot->AddFolder(_T("Data Directory"));
	__try
	{
		pChild->SetColumnsTitles(cszRVA, 10, cszSize, 10);
		pChild->AddText(_T("<Data Directory>\n"));
		for ( i=0; i < pImageOptionalHeader->NumberOfRvaAndSizes; i++)
		{
			DisplayDataDirectoryEntry( pChild, (i >= NUMBER_IMAGE_DIRECTORY_ENTRYS) ? "unused" : ImageDirectoryNames[i],
										pImageOptionalHeader->DataDirectory[i] );
		}
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		pChild->SetError(SZ_DUMP_EXCEPTION);
	}
}

template <class T> void DumpExeDebugDirectory(MPanelItem *pRoot,PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS32 or PIMAGE_NT_HEADERS64
{
    PIMAGE_DEBUG_DIRECTORY debugDir;
    PIMAGE_SECTION_HEADER header;
    DWORD va_debug_dir;
    DWORD size;
    
    va_debug_dir = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_DEBUG);

    if ( va_debug_dir == 0 )
        return;

    // If we found a .debug section, and the debug directory is at the
    // beginning of this section, it looks like a Borland file
    header = GetSectionHeader(".debug", pNTHeader);
    if ( header && (header->VirtualAddress == va_debug_dir) )
    {
        debugDir = MakePtr(PIMAGE_DEBUG_DIRECTORY, pImageBase, header->PointerToRawData);
        size = GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_DEBUG)
                * sizeof(IMAGE_DEBUG_DIRECTORY);
    }
    else    // Look for the debug directory
    {
        header = GetEnclosingSectionHeader( va_debug_dir, pNTHeader );
        if ( !header )
            return;

        size = GetImgDirEntrySize( pNTHeader, IMAGE_DIRECTORY_ENTRY_DEBUG );
    
		debugDir = (PIMAGE_DEBUG_DIRECTORY)GetPtrFromRVA( va_debug_dir, pNTHeader, pImageBase );
    }

    DumpDebugDirectory( pRoot, debugDir, size, pImageBase );
}


template <class T, class U, class V>
void DumpImportsOfOneModule(	MPanelItem *pRoot, LPCSTR asModuleFile,
								T* pINT, U* pIAT, V * pNTHeader,		// 'T', 'U' = IMAGE_THUNK_DATA, 'v' = IMAGE_NT_HEADERS
								PIMAGE_IMPORT_DESCRIPTOR pImportDesc,
								PBYTE pImageBase )
{
    PIMAGE_IMPORT_BY_NAME pOrdinalName;

	bool bIs64Bit = ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );

	char* pszFuncName = NULL;
	char szNameBuffer[MAX_PATH*2], szOrdinalBuffer[10], szBound[32];
	char* pszDecorateBuffer = NULL;

	pRoot->SetColumnsTitles(cszLibrary, 14, cszHint, 5);
	pRoot->Parent()->SetColumnsTitles(cszLibrary, 14, cszHint, 5);

	while ( 1 ) // Loop forever (or until we break out)
	{
		if ( pINT->u1.AddressOfData == 0 )
			break;

		ULONGLONG ordinal = -1;

		if ( bIs64Bit )
		{
			if ( IMAGE_SNAP_BY_ORDINAL64(pINT->u1.Ordinal) )
				ordinal = IMAGE_ORDINAL64(pINT->u1.Ordinal);
		}
		else
		{
			if ( IMAGE_SNAP_BY_ORDINAL32(pINT->u1.Ordinal) )
				ordinal = IMAGE_ORDINAL32(pINT->u1.Ordinal);			
		}

		pszDecorateBuffer = NULL;

		if ( ordinal != -1 )
		{
			//pRoot->printf( "  %4u", ordinal );
			sprintf(szOrdinalBuffer, "%4u", ordinal );
			sprintf(szNameBuffer, "(Ordinal@%u)", ordinal );
			pszFuncName = szNameBuffer;

		}
		else
		{
			// pINT->u1.AddressOfData is theoretically 32 or 64 bits, but in the on-disk representation,
			// we'll assume it's an RVA.  As such, we'll cast it to a DWORD.
			pOrdinalName = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(static_cast<DWORD>(pINT->u1.AddressOfData), pNTHeader, pImageBase);

			#ifdef _DEBUG
			LPBYTE ptr1 = (LPBYTE)GetPtrFromRVA(static_cast<DWORD>(pImportDesc->Name), pNTHeader, pImageBase);
			LPBYTE ptr2 = (LPBYTE)GetPtrFromRVA(static_cast<DWORD>(pImportDesc->OriginalFirstThunk), pNTHeader, pImageBase);
			LPBYTE ptr3 = (LPBYTE)GetPtrFromRVA(static_cast<DWORD>(pImportDesc->FirstThunk), pNTHeader, pImageBase);
			#endif
			
			//pRoot->printf("  %4u  %s", pOrdinalName->Hint, pOrdinalName->Name);
			sprintf(szOrdinalBuffer, "%4u", pOrdinalName->Hint );
			pszFuncName = (char*)pOrdinalName->Name;

			if (gbUseUndecorate && UnDecorate_Dbghelp)
			{
				_ASSERTE(sizeof(szNameBuffer[0])==1);
				szNameBuffer[0] = 0;
				if (UnDecorate_Dbghelp(pszFuncName, szNameBuffer, sizeof(szNameBuffer), UNDNAME_COMPLETE)
					&& szNameBuffer[0] && lstrcmpA(pszFuncName, szNameBuffer))
				{
					pszDecorateBuffer = pszFuncName;
					pszFuncName = szNameBuffer;
				}
			}
		}
		
		// If it looks like the image has been bound, append the
		// bound address
		if ( pImportDesc->TimeDateStamp ) {
			//pRoot->printf( " (Bound to: %08X)", pIAT->u1.Function );
			sprintf( szBound, " (Bound to: %08X)", pIAT->u1.Function );
		} else {
			szBound[0] = 0;
		}

		MPanelItem* pChild = pRoot->AddFile(pszFuncName, 0);

		if (pszDecorateBuffer == NULL)
			pChild->printf("  %s  %s%s\n", szOrdinalBuffer, pszFuncName, szBound);
		else
			pChild->printf("  %s  %s%s\n        %s\n", szOrdinalBuffer, pszFuncName, szBound, pszDecorateBuffer);

		pChild->SetColumns(asModuleFile, szOrdinalBuffer);

		if (SHOW_FUNC_WITH_DLL) {
			MPanelItem* pChild2 = pRoot->Parent()->AddFile(pszFuncName, 0);
			pChild2->AddText(pChild->GetText(), -1, TRUE);
			pChild2->SetColumns(asModuleFile, szOrdinalBuffer);
		}
		
		//pRoot->printf( "\n" );
		
		pINT++;         // Advance to next thunk
		pIAT++;         // advance to next thunk
	}	
}

//
// Dump the imports table (the .idata section) of a PE file
//
template <class T> void DumpImportsSection(MPanelItem *pRoot, PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS 
{
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
    DWORD importsStartRVA;

    // Look up where the imports section is (normally in the .idata section)
    // but not necessarily so.  Therefore, grab the RVA from the data dir.
    importsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_IMPORT);
    if ( !importsStartRVA )
        return;

    pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)GetPtrFromRVA(importsStartRVA,pNTHeader,pImageBase);
	if ( !pImportDesc )
		return;
            
	bool bIs64Bit = ( pNTHeader->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL64_HEADER );

	MPanelItem* pChild = pRoot->AddFolder(_T("Imports Table"));
    pChild->AddText("<Imports Table>:\n");
    
    while ( 1 )
    {
        // See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR
        if ( (pImportDesc->TimeDateStamp==0 ) && (pImportDesc->Name==0) )
            break;
        
		LPCSTR pszDll = (LPCSTR)GetPtrFromRVA(pImportDesc->Name, pNTHeader, pImageBase);

		MPanelItem* pDll = pChild->AddFolder(pszDll);
        pDll->printf("  %s\n", pszDll );

        pDll->printf("  Import Lookup Table RVA:  %08X\n",
      			pImportDesc->Characteristics);

        pDll->printf("  TimeDateStamp:            %08X", pImportDesc->TimeDateStamp );
		if ( (pImportDesc->TimeDateStamp != 0) && (pImportDesc->TimeDateStamp != -1) )
		{
			__time32_t timeStamp = pImportDesc->TimeDateStamp;
			pDll->printf( " -> %s", _ctime32( &timeStamp ) );
		}
		else
			pDll->printf( "\n" );

        pDll->printf("  ForwarderChain:           %08X\n", pImportDesc->ForwarderChain);
        pDll->printf("  DLL Name RVA:             %08X\n", pImportDesc->Name);
        pDll->printf("  Import Address Table RVA: %08X\n", pImportDesc->FirstThunk);
    
        DWORD rvaINT = pImportDesc->OriginalFirstThunk;
        DWORD rvaIAT = pImportDesc->FirstThunk;

        if ( rvaINT == 0 )   // No Characteristics field?
        {
            // Yes! Gotta have a non-zero FirstThunk field then.
            rvaINT = rvaIAT;
            
            if ( rvaINT == 0 )   // No FirstThunk field?  Ooops!!!
                return;
        }
        
        // Adjust the pointer to point where the tables are in the
        // mem mapped file.
        PIMAGE_THUNK_DATA pINT = (PIMAGE_THUNK_DATA)GetPtrFromRVA(rvaINT, pNTHeader, pImageBase);
		if (!pINT )
			return;

        PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)GetPtrFromRVA(rvaIAT, pNTHeader, pImageBase);
    
        pDll->printf("  Ordn  Name\n");
    
		bIs64Bit
			? DumpImportsOfOneModule( pDll, pszDll, (PIMAGE_THUNK_DATA64)pINT, (PIMAGE_THUNK_DATA64)pIAT, pNTHeader, pImportDesc, pImageBase )
			: DumpImportsOfOneModule( pDll, pszDll, (PIMAGE_THUNK_DATA32)pINT, (PIMAGE_THUNK_DATA32)pIAT, pNTHeader, pImportDesc, pImageBase );

        pImportDesc++;   // advance to next IMAGE_IMPORT_DESCRIPTOR
        pDll->printf("\n");
    }

    pChild->printf("\n");
}

//
// Dump the delayed imports table of a PE file
//

template <class T, class U> void DumpDelayedImportsImportNames( MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader, U* thunk, bool bUsingRVA, LPCSTR asModuleFile )	// T = PIMAGE_NT_HEADER, U = 'IMAGE_THUNK_DATA'
{
	pRoot->SetColumnsTitles(cszLibrary, 14, cszHint, 5);
	pRoot->Parent()->SetColumnsTitles(cszLibrary, 14, cszHint, 5);

    while ( 1 ) // Loop forever (or until we break out)
    {
        if ( thunk->u1.AddressOfData == 0 )
            break;

		ULONGLONG ordinalMask;
		if ( sizeof(thunk->u1.Ordinal) == sizeof(ULONGLONG) )	// Which ordinal mask should we use?
			ordinalMask = IMAGE_ORDINAL_FLAG64;
		else
			ordinalMask = IMAGE_ORDINAL_FLAG32;

		char fileName[MAX_PATH], szOrdinalBuffer[10];

        if ( thunk->u1.Ordinal & ordinalMask )
        {
			sprintf(fileName, "(Ordinal@%u)", (DWORD)(thunk->u1.Ordinal & 0xFFFF));
			sprintf(szOrdinalBuffer, "%4u", (DWORD)(thunk->u1.Ordinal & 0xFFFF));
            pRoot->printf( "    %4u", (thunk->u1.Ordinal & 0xFFFF) );
        }
        else
        {
            PIMAGE_IMPORT_BY_NAME pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)0 + thunk->u1.AddressOfData);

			pOrdinalName = bUsingRVA
				? (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA((DWORD)thunk->u1.AddressOfData, pNTHeader, pImageBase)
				: (PIMAGE_IMPORT_BY_NAME)GetPtrFromVA((PVOID)pOrdinalName, pNTHeader, pImageBase);
            
			lstrcpynA(fileName, (LPCSTR)pOrdinalName->Name, MAX_PATH);
			sprintf(szOrdinalBuffer, "%4u", (DWORD)(pOrdinalName->Hint));
			pRoot->printf("    %4u  %s", pOrdinalName->Hint, pOrdinalName->Name);
        }

		MPanelItem* pFunc = pRoot->AddFile(fileName);
		pFunc->SetColumns(asModuleFile, szOrdinalBuffer);

		if (SHOW_FUNC_WITH_DLL) {
			MPanelItem* pFunc2 = pRoot->Parent()->AddFile(fileName);
			pFunc2->SetColumns(asModuleFile, szOrdinalBuffer);
		}
        
        pRoot->printf( "\n" );

        thunk++;            // Advance to next thunk
    }

	pRoot->printf( "\n" );
}

template <class T> void DumpDelayedImportsSection(MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader, bool bIs64Bit )	// 'T' = PIMAGE_NT_HEADERS 
{
	DWORD delayImportStartRVA, delayImportSize;
    PCImgDelayDescr pDelayDesc;

    // Look up where the delay imports section is (normally in the .didat
	/// section) but not necessarily so.
 	delayImportStartRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	delayImportSize = GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    if ( !delayImportStartRVA || !delayImportSize )
        return;

	MPanelItem *pChild = pRoot->AddFolder(_T("Delay Imports Table"));

	// This code is more complicated than it needs to be, thanks to Microsoft.  When the
	// ImgDelayDescr was originally created for Win32, portability to Win64 wasn't
	// considered.  As such, MS used pointers, rather than RVAs in the data structures.
	// Finally, MS issued a new DELAYIMP.H, which issued a flag indicating whether the
	// field values are RVAs or VAs.  Unfortunately, Microsoft has been rather slow to
	// get this header file out into general distibution.  Currently, you can get it as
	// part of the Win64 headers, or as part of VC7.  In the meanwhile, we'll use some
	// preprocessor trickery so that we can use the new field names, while still compiling
	// with the original DELAYIMP.H.

	#if _DELAY_IMP_VER < 2
	#define rvaDLLName		szName
	#define rvaHmod			phmod
	#define rvaIAT			pIAT
	#define rvaINT			pINT
	#define rvaBoundIAT		pBoundIAT
	#define rvaUnloadIAT	pUnloadIAT
	#endif

    pDelayDesc = (PCImgDelayDescr)GetPtrFromRVA(delayImportStartRVA, pNTHeader, pImageBase);
	if ( !pDelayDesc )
		return;

	__try {
	            
		pChild->printf("<Delay Imports Table>:\n");
		
		int nDelaySizeLeft = (int)delayImportSize;
		int nDllNo = 0;
		while ( nDelaySizeLeft > 0 && pDelayDesc->rvaDLLName )
		{
			// from more recent DELAYIMP.H:
			// enum DLAttr {                   // Delay Load Attributes
			//    dlattrRva = 0x1,                // RVAs are used instead of pointers
			//    };
			nDllNo++;
			bool bUsingRVA = pDelayDesc->grAttrs & 1;
			bool bInvalid = false;

			DWORD dllNameRVA = (DWORD)pDelayDesc->rvaDLLName;
			PVOID dllNameVA = (PBYTE)0+(DWORD)pDelayDesc->rvaDLLName;

			PSTR pszDLLNameRVA  = (PSTR)GetPtrFromRVA(dllNameRVA, pNTHeader,pImageBase);
			PSTR pszDLLNameVA   = (PSTR)GetPtrFromVA(dllNameVA,pNTHeader,pImageBase);
			PSTR pszDLLName = bUsingRVA ? pszDLLNameRVA : pszDLLNameVA;

			char szDllName[MAX_PATH];
			if (!pszDLLName) {
				sprintf(szDllName, "(NULL@%i)", nDllNo);
				bInvalid = true;
			} else {
				lstrcpynA(szDllName, pszDLLName, MAX_PATH);
			}

			MPanelItem *pDll = pChild->AddFolder(szDllName);

			pDll->printf( "  %s\n", szDllName );
			pDll->printf( "    Attributes:                 %08X", pDelayDesc->grAttrs );
			if ( pDelayDesc->grAttrs & 1 )
				pDll->printf( "  dlattrRva" );
			pDll->printf( "\n" );
			pDll->printf( "    Name R(VA):                 %08X\n", pDelayDesc->rvaDLLName );
			pDll->printf( "    HMODULE R(VA):              %08X\n", pDelayDesc->rvaHmod);
			pDll->printf( "    Import Address Table R(VA): %08X\n", pDelayDesc->rvaIAT );
			pDll->printf( "    Import Names Table R(VA):   %08X\n", pDelayDesc->rvaINT );
			pDll->printf( "    Bound IAT R(VA):            %08X\n", pDelayDesc->rvaBoundIAT );
			pDll->printf( "    Unload IAT R(VA):           %08X\n", pDelayDesc->rvaUnloadIAT );	    
			pDll->printf( "    TimeDateStamp:              %08X", pDelayDesc->dwTimeStamp );

			if ( pDelayDesc->dwTimeStamp )
			{
				__time32_t timeStamp = pDelayDesc->dwTimeStamp;
				pDll->printf( " -> %s", _ctime32(&timeStamp) );
			}
			else
				pDll->printf( "\n" );

			//
			// Display the Import Names Table.
			
			
			PVOID thunkVA = (PBYTE)0 + (DWORD)pDelayDesc->rvaINT;

			PVOID pvThunkRVA = GetPtrFromRVA((DWORD)pDelayDesc->rvaINT, pNTHeader, pImageBase );
			PVOID pvThunkVA  = GetPtrFromVA(thunkVA, pNTHeader, pImageBase );
			PVOID pvThunk = bUsingRVA ? pvThunkRVA : pvThunkVA;
	    
			pDll->printf("    Ordn  Name\n");
	        
			if (!pvThunk) {
				pDll->printf("    <NULL>\n");
				bInvalid = true;
			} else {
				bIs64Bit
					? DumpDelayedImportsImportNames( pDll, pImageBase, pNTHeader, (PIMAGE_THUNK_DATA64)pvThunk, bUsingRVA, szDllName )
					: DumpDelayedImportsImportNames( pDll, pImageBase, pNTHeader, (PIMAGE_THUNK_DATA32)pvThunk, bUsingRVA, szDllName );
			}

			pDll->printf( "\n" );

			if (bInvalid)
				pDll->SetError(_T("Invalid Delay Import Table"));

			pDelayDesc++;	// Pointer math.  Advance to next delay import desc.
			nDelaySizeLeft -= sizeof(ImgDelayDescr);
		}

		pChild->printf("\n");
	} __except(EXCEPTION_EXECUTE_HANDLER) {

		pChild->SetError(SZ_DUMP_EXCEPTION);

	}

#if _DELAY_IMP_VER < 2 // Remove the alias names from the namespace
#undef szName
#undef phmod
#undef pIAT
#undef pINT
#undef pBoundIAT
#undef pUnloadIAT
#endif
}

//
// Dump the exports table (usually the .edata section) of a PE file
//
template <class T> void DumpExportsSection(MPanelItem *pRoot, PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS 
{
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    PIMAGE_SECTION_HEADER header;
    INT delta; 
    PSTR pszFilename;
    DWORD i;
    PDWORD pdwFunctions;
    PWORD pwOrdinals;
    DWORD *pszFuncNames;
    DWORD exportsStartRVA, exportsEndRVA;
    
    exportsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_EXPORT);
    exportsEndRVA = exportsStartRVA +
	   				GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT);

    // Get the IMAGE_SECTION_HEADER that contains the exports.  This is
    // usually the .edata section, but doesn't have to be.
    header = GetEnclosingSectionHeader( exportsStartRVA, pNTHeader );
    if ( !header )
        return;

    delta = (INT)(header->VirtualAddress - header->PointerToRawData);
        
    pExportDir = (PIMAGE_EXPORT_DIRECTORY)GetPtrFromRVA(exportsStartRVA, pNTHeader, pImageBase);
        
    pszFilename = (PSTR)GetPtrFromRVA( pExportDir->Name, pNTHeader, pImageBase );
        
	MPanelItem *pChild = pRoot->AddFolder(ExportsTableName);
	pChild->SetColumnsTitles(cszOrdinal,4,cszEntryPoint,8);
    pChild->printf("<Exports Table>:\n");
    pChild->printf("  Name:            %s\n", pszFilename);
    pChild->printf("  Characteristics: %08X\n", pExportDir->Characteristics);
	
	__time32_t timeStamp = pExportDir->TimeDateStamp;
    pChild->printf("  TimeDateStamp:   %08X -> %s",
    			pExportDir->TimeDateStamp, _ctime32(&timeStamp) );
    pChild->printf("  Version:         %u.%02u\n", pExportDir->MajorVersion,
            pExportDir->MinorVersion);
    pChild->printf("  Ordinal base:    %08X\n", pExportDir->Base);
    pChild->printf("  # of functions:  %08X\n", pExportDir->NumberOfFunctions);
    pChild->printf("  # of Names:      %08X\n", pExportDir->NumberOfNames);
    
    pdwFunctions =	(PDWORD)GetPtrFromRVA( pExportDir->AddressOfFunctions, pNTHeader, pImageBase );
    pwOrdinals =	(PWORD)	GetPtrFromRVA( pExportDir->AddressOfNameOrdinals, pNTHeader, pImageBase );
    pszFuncNames =	(DWORD *)GetPtrFromRVA( pExportDir->AddressOfNames, pNTHeader, pImageBase );

	LPCSTR pszFuncName = NULL, pszDecorateBuffer = NULL;
	char szNameBuffer[MAX_PATH*2], szEntryPoint[32], szOrdinal[16];
	bool lbFar1 = false, lbFar2 = false, lbFar3 = false, lbOpenPluginW = false;

    pChild->printf("\n  Entry Pt  Ordn  Name\n");
	if (pdwFunctions)
    for (	i=0;
			i < pExportDir->NumberOfFunctions;
			i++, pdwFunctions++ )
    {
        DWORD entryPointRVA = *pdwFunctions;

        if ( entryPointRVA == 0 )   // Skip over gaps in exported function
            continue;               // ordinals (the entrypoint is 0 for
                                    // these functions).

        //pRoot->printf("  %08X  %4u", entryPointRVA, i + pExportDir->Base );
		sprintf(szEntryPoint, "%08X", entryPointRVA);
		sprintf(szOrdinal, "%4u", i + pExportDir->Base);

        // See if this function has an associated name exported for it.
		pszFuncName = NULL; pszDecorateBuffer = NULL;
		if (pwOrdinals && pszFuncNames)
		{
			for ( unsigned j=0; j < pExportDir->NumberOfNames; j++ )
			{
				if ( pwOrdinals[j] == i )
				{
					pszFuncName = (LPCSTR)GetPtrFromRVA(pszFuncNames[j], pNTHeader, pImageBase);
					//pRoot->printf("  %s", GetPtrFromRVA(pszFuncNames[j], pNTHeader, pImageBase) );
					if (pszFuncName)
					{
						if (*pszFuncName == 'S')
						{
							if (!strcmp(pszFuncName, "SetStartupInfo"))
							{
								//pRoot->AddFlags(_T("FAR1"));
								lbFar1 = true;
							}
							else if(!strcmp(pszFuncName, "SetStartupInfoW"))
							{
								//pRoot->AddFlags(_T("FAR2"));
								lbFar2 = true;
							}
						}
						else if (*pszFuncName == 'G')
						{
							if(!strcmp(pszFuncName, "GetGlobalInfoW"))
							{
								//pRoot->AddFlags(_T("FAR3"));
								lbFar3 = true;
							}
						}
						else if (*pszFuncName == 'O')
						{
							if(!strcmp(pszFuncName, "OpenPluginW") || !strcmp(pszFuncName, "OpenFilePluginW"))
							{
								lbOpenPluginW = true;
							}
						}
						else if (*pszFuncName == 'D')
						{
							if (!strcmp(pszFuncName, "DllRegisterServer"))
							{
								pRoot->AddFlags(_T("COM"));
							}
						}
						else if (*pszFuncName == 'a')
						{
							if (!strcmp(pszFuncName, "acrxEntryPoint"))
							{
								pRoot->AddFlags(_T("ACAD"));
							}
						}
					}

					// Demangle
					if (gbUseUndecorate && UnDecorate_Dbghelp)
					{
						_ASSERTE(sizeof(szNameBuffer[0])==1);
						szNameBuffer[0] = 0;
						if (UnDecorate_Dbghelp(pszFuncName, szNameBuffer, sizeof(szNameBuffer), UNDNAME_COMPLETE)
							&& szNameBuffer[0] && lstrcmpA(pszFuncName, szNameBuffer))
						{
							pszDecorateBuffer = pszFuncName;
							pszFuncName = szNameBuffer;
						}
					}
				}
			}
		}
		if (!pszFuncName) {
			sprintf(szNameBuffer, "(Ordinal@%u)", i + pExportDir->Base);
			pszFuncName = szNameBuffer;
		}

		MPanelItem* pFunc = pChild->AddFile(pszFuncName);
		if (pszDecorateBuffer == NULL)
			pFunc->printf("  %s  %s  %s", szEntryPoint, szOrdinal, pszFuncName);
		else
			pFunc->printf("  %s  %s  %s\n                  %s", szEntryPoint, szOrdinal, pszFuncName, pszDecorateBuffer);
		pFunc->SetColumns(szOrdinal, szEntryPoint);

        // Is it a forwarder?  If so, the entry point RVA is inside the
        // .edata section, and is an RVA to the DllName.EntryPointName
        if ( (entryPointRVA >= exportsStartRVA)
             && (entryPointRVA <= exportsEndRVA) )
        {
            pFunc->printf(" (forwarder -> %s)", GetPtrFromRVA(entryPointRVA, pNTHeader, pImageBase) );
        }
		else
		{
			pFunc->SetData(pImageBase+entryPointRVA, 256);
		}
        
        pFunc->AddText(_T("\n"));
    }

    if (lbFar1)
    	pRoot->AddFlags(_T("FAR1"));
    if (lbFar2 && ((!lbFar3) || (lbFar3 && lbOpenPluginW)))
    	pRoot->AddFlags(_T("FAR2"));
    if (lbFar3)
    	pRoot->AddFlags(_T("FAR3"));

	pChild->printf( "\n" );
}

template <class T> void DumpRuntimeFunctions( MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader )
{
	DWORD rtFnRVA;

	rtFnRVA = GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_EXCEPTION );
	if ( !rtFnRVA )
		return;

	DWORD cEntries =
		GetImgDirEntrySize( pNTHeader, IMAGE_DIRECTORY_ENTRY_EXCEPTION )
		/ sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );
	if ( 0 == cEntries )
		return;

	PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY pRTFn = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)
							GetPtrFromRVA( rtFnRVA, pNTHeader, pImageBase );

	if ( !pRTFn )
		return;

	pRoot->printf( "Runtime Function Table (Exception handling)\n" );
	pRoot->printf( "  Begin     End       Unwind  \n" );
	pRoot->printf( "  --------  --------  --------\n" );

	for ( unsigned i = 0; i < cEntries; i++, pRTFn++ )
	{
		pRoot->printf(	"  %08X  %08X  %08X",
			pRTFn->BeginAddress, pRTFn->EndAddress, pRTFn->UnwindInfoAddress );

		if ( g_pCOFFSymbolTable )
		{
			PCOFFSymbol pSymbol
				= g_pCOFFSymbolTable->GetNearestSymbolFromRVA( pRTFn->BeginAddress, TRUE );
			if ( pSymbol )
				pRoot->printf( "  %s", pSymbol->GetName() );

			delete pSymbol;
		}

		pRoot->printf( "\n" );
	}
}

// The names of the available base relocations
TCHAR *SzRelocTypes[] = {
{_T("ABSOLUTE")},			   // 0
{_T("HIGH")},                 // 1
{_T("LOW")},                  // 2
{_T("HIGHLOW")},              // 3
{_T("HIGHADJ")},              // 4
{_T("MIPS_JMPADDR")},         // 5
{_T("???6")},
{_T("???7")},
{_T("???8")},
{_T("IA64_IMM64")},           // 9
{_T("DIR64")},                // 10
};

//
// Dump the base relocation table of a PE file
//
template <class T> void DumpBaseRelocationsSection(MPanelItem *pRoot, PBYTE pImageBase, T * pNTHeader)
{
	DWORD dwBaseRelocRVA;
    PIMAGE_BASE_RELOCATION baseReloc;

	dwBaseRelocRVA =
		GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_BASERELOC );
    if ( !dwBaseRelocRVA )
        return;

    baseReloc = (PIMAGE_BASE_RELOCATION)
    				GetPtrFromRVA( dwBaseRelocRVA, pNTHeader, pImageBase );
	if ( !baseReloc )
		return;

	MPanelItem* pChild = pRoot->AddFolder(_T("Base Relocations"));
	pChild->printf( "<Base Relocations>:\n" );

	if (!ValidateMemory(baseReloc,sizeof(IMAGE_BASE_RELOCATION)))
	{
		pChild->SetErrorPtr(baseReloc,sizeof(IMAGE_BASE_RELOCATION)); return;
	}

    //pRoot->printf("base relocations:\n\n");
    unsigned __int64 cAllEntries = 0;
    unsigned int cIDX = 0;
    TCHAR szName[64], szAddr[12];
    
    pChild->SetColumnsTitles(cszAddress, 10, cszCount, 5, 0);

    while ( baseReloc->SizeOfBlock != 0 )
    {
        unsigned cEntries;
        PWORD pEntry;
        //TCHAR *szRelocType;
        //WORD relocType;

		// Sanity check to make sure the data looks OK.
		if ( 0 == baseReloc->VirtualAddress )
			break;
		if ( baseReloc->SizeOfBlock < sizeof(*baseReloc) )
			break;
		
        cEntries = (baseReloc->SizeOfBlock-sizeof(*baseReloc))/sizeof(WORD);
        pEntry = MakePtr( PWORD, baseReloc, sizeof(*baseReloc) );
        cAllEntries += cEntries;
        
        cIDX++;
        wsprintf(szName, _T("#%u.reloc"), cIDX);
        wsprintf(szAddr, _T("0x%08X"), baseReloc->VirtualAddress);
        MPanelItem* pReloc = pChild->AddFile(szName, cEntries);
        pReloc->printf(_T("Virtual Address: %s  size: 0x%08X  entries: %u\n"),
                szAddr, baseReloc->SizeOfBlock, cEntries);

		// ’орошо бы проверить и валидность этого блока пам€ти
		if (!ValidateMemory(baseReloc,baseReloc->SizeOfBlock))
		{
			pReloc->SetErrorPtr(baseReloc,baseReloc->SizeOfBlock); break;
		}

        pReloc->SetData((LPBYTE)baseReloc, baseReloc->SizeOfBlock);
        
        // Approximate. cEntries may be changed on IMAGE_REL_BASED_HIGHADJ
        wsprintf(szName, _T("%4u"), cEntries);
        pReloc->SetColumns(szAddr, szName);
            
        //for ( i=0; i < cEntries; i++ )
        //{
        //    // Extract the top 4 bits of the relocation entry.  Turn those 4
        //    // bits into an appropriate descriptive string (szRelocType)
        //    relocType = (*pEntry & 0xF000) >> 12;
        //    szRelocType = relocType <= IMAGE_REL_BASED_DIR64 ? SzRelocTypes[relocType] : _T("unknown");
        //
		//	DWORD dwAddr = (*pEntry & 0x0FFF) + baseReloc->VirtualAddress;
        //    
        //    
        //
		//	if ( IMAGE_REL_BASED_HIGHADJ == relocType )
		//	{
		//		pEntry++;
		//		cEntries--;
		//		//pChild->printf( " (%X)", *pEntry );
		//		pChild->printf(_T("  %08X %s (%X)\n"), dwAddr, szRelocType, *pEntry);
		//	} else {
		//		pChild->printf(_T("  %08X %s\n"), dwAddr, szRelocType);
		//	}
		//
		//	//pChild->AddText( _T("\n") );
        //    pEntry++;   // Advance to next relocation entry
        //}
        
        baseReloc = MakePtr( PIMAGE_BASE_RELOCATION, baseReloc,
                             baseReloc->SizeOfBlock);
		if (!ValidateMemory(baseReloc,sizeof(IMAGE_BASE_RELOCATION)))
		{
			pChild->SetErrorPtr(baseReloc,sizeof(IMAGE_BASE_RELOCATION)); return;
		}
    }

    pChild->printf("=================================\nTotal relocations count: %I64u\n", cAllEntries);
            
	pRoot->AddText( _T("\n") );
}

//
// Dump out the new IMAGE_BOUND_IMPORT_DESCRIPTOR that NT 3.51 added
//
template <class T> void DumpBoundImportDescriptors( MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader )	// 'T' = PIMATE_NT_HEADERS
{
    DWORD bidRVA;   // Bound import descriptors RVA
    PIMAGE_BOUND_IMPORT_DESCRIPTOR pibid;

    bidRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
    if ( !bidRVA )
        return;
    
    pibid = MakePtr( PIMAGE_BOUND_IMPORT_DESCRIPTOR, pImageBase, bidRVA );

	MPanelItem* pChild = pRoot->AddFolder(_T("Bound Imports"));
    pChild->printf( "Bound import descriptors:\n\n" );
    pChild->printf( "  Module        TimeDate\n" );
    pChild->printf( "  ------------  --------\n" );
    
    while ( pibid->TimeDateStamp )
    {
        unsigned i;
        PIMAGE_BOUND_FORWARDER_REF pibfr;

        __time32_t timeStamp = pibid->TimeDateStamp;

		char *pszTime = _ctime32(&timeStamp); if (!pszTime) pszTime = "(null)\n";
		char *pszModule = (char*)(pImageBase + bidRVA + pibid->OffsetModuleName);
		//if (IsBadReadPtr(pszModule,12)) pszModule = "";
		if (!ValidateMemory(pszModule,12)) pszModule = "";
        pChild->printf( "  %-12s  %08X -> %s",
        		pszModule,
                pibid->TimeDateStamp,
                pszTime );
                            
        pibfr = MakePtr(PIMAGE_BOUND_FORWARDER_REF, pibid,
                            sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR));

        for ( i=0; i < pibid->NumberOfModuleForwarderRefs; i++ )
        {
			timeStamp = pibfr->TimeDateStamp;

			pszTime = _ctime32(&timeStamp); if (!pszTime) pszTime = "(null)\n";
			pszModule = (char*)(pImageBase + bidRVA + pibfr->OffsetModuleName);
			if (!ValidateMemory(pszModule,12)) pszModule = "";
            pChild->printf("    forwarder:  %-12s  %08X -> %s", 
							pszModule,                            
                            pibfr->TimeDateStamp,
                            pszTime );
            pibfr++;    // advance to next forwarder ref
                
            // Keep the outer loop pointer up to date too!
            pibid = MakePtr( PIMAGE_BOUND_IMPORT_DESCRIPTOR, pibid,
                             sizeof( IMAGE_BOUND_FORWARDER_REF ) );
        }

        pibid++;    // Advance to next pibid;
    }

    pChild->AddText(_T("\n" ));
}

//
// Dump the TLS data
//
template <class T,class U> void DumpTLSDirectory(MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader, U * pTLSDir )	// 'T' = IMAGE_NT_HEADERS, U = 'IMAGE_TLS_DIRECTORY'
{
    DWORD tlsRVA;   // TLS dirs RVA

    tlsRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_TLS);
    if ( !tlsRVA )
        return;

    pTLSDir = (U*)GetPtrFromRVA( tlsRVA, pNTHeader, pImageBase );

    pRoot->printf("TLS directory:\n");
    pRoot->printf( "  StartAddressOfRawData: %08X\n", pTLSDir->StartAddressOfRawData );
    pRoot->printf( "  EndAddressOfRawData:   %08X\n", pTLSDir->EndAddressOfRawData );
    pRoot->printf( "  AddressOfIndex:        %08X\n", pTLSDir->AddressOfIndex );
    pRoot->printf( "  AddressOfCallBacks:    %08X\n", pTLSDir->AddressOfCallBacks );
    pRoot->printf( "  SizeOfZeroFill:        %08X\n", pTLSDir->SizeOfZeroFill );
    pRoot->printf( "  Characteristics:       %08X\n", pTLSDir->Characteristics );

	pRoot->printf( "\n" );
}

template <class T> void DumpCOR20Header( MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader )	// T = PIMAGE_NT_HEADERS
{
    DWORD cor20HdrRVA;   // COR20_HEADER RVA

    cor20HdrRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR );
    if ( !cor20HdrRVA )
        return;

    PIMAGE_COR20_HEADER pCor20Hdr = (PIMAGE_COR20_HEADER)GetPtrFromRVA( cor20HdrRVA, pNTHeader, pImageBase );
    
    pRoot->Root()->AddFlags(_T("NET"));
    
    MPanelItem* pChild = pRoot->AddFolder(_T(".NET"));
	
	pChild->printf( "<.NET Runtime Header>:\n" );

	pChild->printf( "  Size:       %u\n", pCor20Hdr->cb );
	pChild->printf( "  Version:    %u.%u\n", pCor20Hdr->MajorRuntimeVersion, pCor20Hdr->MinorRuntimeVersion );
	pChild->printf( "  Flags:      %X\n", pCor20Hdr->Flags );
	if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_ILONLY ) pChild->printf( "    ILONLY\n" );
	if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_32BITREQUIRED ) pChild->printf( "    32BITREQUIRED\n" );
	if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_IL_LIBRARY ) pChild->printf( "    IL_LIBRARY\n" );
	if ( pCor20Hdr->Flags & 8 ) pChild->printf( "    STRONGNAMESIGNED\n" );		// At this moment, WINNT.H and CorHdr.H are out of sync...
	if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_TRACKDEBUGDATA ) pChild->printf( "    TRACKDEBUGDATA\n" );

	DisplayDataDirectoryEntry( pChild, "MetaData", pCor20Hdr->MetaData );
    DisplayDataDirectoryEntry( pChild, "Resources", pCor20Hdr->Resources );
    DisplayDataDirectoryEntry( pChild, "StrongNameSig", pCor20Hdr->StrongNameSignature );
    DisplayDataDirectoryEntry( pChild, "CodeManagerTable", pCor20Hdr->CodeManagerTable );
    DisplayDataDirectoryEntry( pChild, "VTableFixups", pCor20Hdr->VTableFixups );
    DisplayDataDirectoryEntry( pChild, "ExprtAddrTblJmps", pCor20Hdr->ExportAddressTableJumps );
    DisplayDataDirectoryEntry( pChild, "ManagedNativeHdr", pCor20Hdr->ManagedNativeHeader );

	pRoot->printf( "\n" );
}

template <class T, class U> void DumpLoadConfigDirectory(MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader, U * pLCD )	// T = PIMAGE_NT_HEADERS, U = PIMAGE_LOAD_CONFIG_DIRECTORY 
{
    DWORD loadConfigDirRVA;

    loadConfigDirRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG );
    if ( !loadConfigDirRVA )
        return;

	pLCD = (U*)GetPtrFromRVA( loadConfigDirRVA, pNTHeader, pImageBase );

	MPanelItem* pChild = pRoot->AddFolder(_T("LoadConfig"));
	pChild->printf( "<Image Load Configuration Directory>:\n" );

	if (!pLCD) {
		pChild->printf(_T("GetPtrFromRVA(0x%08X) FAILED\n"), loadConfigDirRVA);
		pChild->SetError(_T("Invalid LoadConfig RVA"));
	} else {
		pChild->printf( "  Size:    %X\n", pLCD->Size );
		pChild->printf( "  TimeDateStamp:    %X\n", pLCD->TimeDateStamp );
		pChild->printf( "  Version:    %u.%u\n", pLCD->MajorVersion, pLCD->MinorVersion );
		pChild->printf( "  GlobalFlagsClear:    %X\n", pLCD->GlobalFlagsClear );
		pChild->printf( "  GlobalFlagsSet:    %X\n", pLCD->GlobalFlagsSet );
		pChild->printf( "  CriticalSectionDefaultTimeout:    %X\n", pLCD->CriticalSectionDefaultTimeout );
		pChild->printf( "  DeCommitFreeBlockThreshold:    %X\n", pLCD->DeCommitFreeBlockThreshold );
		pChild->printf( "  DeCommitTotalFreeThreshold:    %X\n", pLCD->DeCommitTotalFreeThreshold );
		pChild->printf( "  LockPrefixTable:    %X\n", pLCD->LockPrefixTable );
		pChild->printf( "  MaximumAllocationSize:    %X\n", pLCD->MaximumAllocationSize );
		pChild->printf( "  VirtualMemoryThreshold:    %X\n", pLCD->VirtualMemoryThreshold );
		pChild->printf( "  ProcessHeapFlags:    %X\n", pLCD->ProcessHeapFlags );
		pChild->printf( "  ProcessAffinityMask:    %X\n", pLCD->ProcessAffinityMask );
		pChild->printf( "  CSDVersion:    %u\n", pLCD->CSDVersion );
		pChild->printf( "  Reserved1:    %X\n", pLCD->Reserved1 );
		pChild->printf( "  EditList:    %X\n", pLCD->EditList );
	}

	pChild->printf( "\n" );
}

template <class T> void DumpCertificates(MPanelItem *pRoot, PBYTE pImageBase, T* pNTHeader)
{
	// Note that the this DataDirectory entry gives a >>> FILE OFFSET <<< rather than
	// an RVA.
    DWORD certOffset;

    certOffset = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_SECURITY );
    if ( !certOffset )
        return;
	
	__int64 dwTotalSize = GetImgDirEntrySize( pNTHeader, IMAGE_DIRECTORY_ENTRY_SECURITY );

	TCHAR szCertName[MAX_PATH], szCertType[64], szRev[8];
	int nCertNo = 0;
	MPanelItem* pChild = pRoot->AddFolder(_T("Certificates"));
	pChild->printf( "<Certificates>:\n" );

	pChild->SetColumnsTitles(cszCertRevTitle, 6, cszCertTypeTitle, 16);

	//LPWIN_CERTIFICATE pCert = (LPWIN_CERTIFICATE)GetPtrFromRVA( certOffset, pNTHeader, pImageBase );
	LPWIN_CERTIFICATE pCert = MakePtr( LPWIN_CERTIFICATE, pImageBase, certOffset );

	while ( dwTotalSize > 0 )	// As long as there is unprocessed certificate data...
	{
		//LPWIN_CERTIFICATE pCert = MakePtr( LPWIN_CERTIFICATE, pImageBase, certOffset );

		if (!pCert || !ValidateMemory(pCert, sizeof(*pCert))) {
			pChild->printf(_T("\n!!! Failed to read LPWIN_CERTIFICATE at offset: 0x%08X !!!\n"), certOffset);
			break;
		}

		if (!pCert->dwLength) {
			break; // кончились
		}

		size_t nAllLen = pCert->dwLength;

		switch( pCert->wCertificateType )
		{
		case WIN_CERT_TYPE_X509: lstrcpy(szCertType, _T("X509")); break;
		case WIN_CERT_TYPE_PKCS_SIGNED_DATA: lstrcpy(szCertType, _T("PKCS_SIGNED_DATA")); break;
		case WIN_CERT_TYPE_TS_STACK_SIGNED: lstrcpy(szCertType, _T("TS_STACK_SIGNED")); break;
		default: wsprintf(szCertType, _T("0x%04X"), pCert->wCertificateType);
		}
		
		nCertNo++;

		if (!ValidateMemory(pCert, nAllLen)) {
			wsprintf(szCertName, _T("#%i.INVALID_CERTIFICATE"), nCertNo);
			MPanelItem* pCertFile = pChild->AddFile(szCertName, pCert->dwLength);
			pCertFile->SetData((LPBYTE)pCert, pCert->dwLength);
			wsprintf(szRev, _T("0x%04X"), pCert->wRevision);
			pCertFile->SetColumns(szRev, szCertType);

			pChild->printf(_T("\n!!! Can't access %u bytes of LPWIN_CERTIFICATE at offset: 0x%08X !!!\n"), (DWORD)nAllLen, certOffset);
			break;
		}

		pChild->printf( "  Certificate #%i\n", nCertNo );

		pChild->printf( "    Length:   %i bytes\n", pCert->dwLength );
		pChild->printf( "    Revision: 0x%04X\n", pCert->wRevision );
		pChild->printf( "    Type:     0x%04X", pCert->wCertificateType );
		
		if (szCertType[0] != _T('0'))
			pChild->printf(_T(" (%s)"), szCertType);

		wsprintf(szCertName, _T("#%i.%s"), nCertNo, szCertType);
		//int nLen = pCert->dwLength - sizeof(WIN_CERTIFICATE) + 1;
		MPanelItem* pCertFile = pChild->AddFile(szCertName, pCert->dwLength);
		pCertFile->SetData((LPBYTE)pCert, pCert->dwLength);
		wsprintf(szRev, _T("0x%04X"), pCert->wRevision);
		pCertFile->SetColumns(szRev, szCertType);

		pChild->printf( "\n" );

		dwTotalSize -= nAllLen; //pCert->dwLength;
		//certOffset += pCert->dwLength;		// Get offset to next certificate
		
		pCert = (LPWIN_CERTIFICATE)(((LPBYTE)pCert) + nAllLen);
	}

	pChild->printf( "\n" );
}

bool DumpExeFilePE( MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader, PIMAGE_NT_HEADERS32 pNTHeader );
bool DumpExeFileVX( MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader, PIMAGE_VXD_HEADER pVXDHeader );

//
// top level routine called from PEDUMP.CPP to dump the components of a PE file
//
bool DumpExeFile( MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader )
{
    PIMAGE_NT_HEADERS32 pNTHeader;
    PBYTE pImageBase = (PBYTE)dosHeader;
    
	// Make pointers to 32 and 64 bit versions of the header.
    pNTHeader = MakePtr( PIMAGE_NT_HEADERS32, dosHeader,
                                dosHeader->e_lfanew );

	DWORD nSignature = 0;
    // First, verify that the e_lfanew field gave us a reasonable
    // pointer, then verify the PE signature.
	if ( ValidateMemory( pNTHeader, sizeof(pNTHeader->Signature) ) )
	{
		nSignature = pNTHeader->Signature;
		if ( nSignature == IMAGE_NT_SIGNATURE )
		{
			return DumpExeFilePE( pRoot, dosHeader, pNTHeader );
		}
		else if ( (nSignature & 0xFFFF) == IMAGE_OS2_SIGNATURE )
		{
			return DumpExeFileNE( pRoot, dosHeader, (IMAGE_OS2_HEADER*)pNTHeader );
		}
		else if ( (nSignature & 0xFFFF) == IMAGE_OS2_SIGNATURE_LE )
		{
			return DumpExeFileNE( pRoot, dosHeader, (IMAGE_OS2_HEADER*)pNTHeader );
		}
		else if ( (nSignature & 0xFFFF) == IMAGE_VXD_SIGNATURE )
		{
			return DumpExeFileVX( pRoot, dosHeader, (IMAGE_VXD_HEADER*)pNTHeader );
		}
		else
		{
			//pRoot->Root()->AddFlags(_T("DOS")); - в корне и так будут только DOS_Header
			DumpHeader(pRoot, dosHeader);
			return true;
		}
	}

	merror(_T("Not a Portable Executable (PE) EXE\nUnknown signature: 0x%08X"), nSignature);
    return false;
}

bool ValidateMemory(LPCVOID ptr, size_t nSize)
{
	if (!ptr || (LPBYTE)ptr < (LPBYTE)g_pMappedFileBase)
		return false;
	ULONGLONG nPos = ((LPBYTE)ptr - (LPBYTE)g_pMappedFileBase);
	if ((nPos+nSize) > g_FileSize.QuadPart)
		return false;
	return true;
}

bool DumpExeFileVX( MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader, PIMAGE_VXD_HEADER pVXDHeader )
{
	PBYTE pImageBase = (PBYTE)dosHeader;

	MPanelItem* pChild = pRoot->AddFolder(_T("VxD Header"));
	pChild->AddText(_T("<VxD Header>\n"));

	pChild->AddText(_T("  Signature:         IMAGE_VXD_SIGNATURE\n"));

	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));
	DumpHeader(pRoot, dosHeader);

	MPanelItem* pVXD = pRoot->AddFile(_T("VxD_Header"), sizeof(*pVXDHeader));
	pVXD->SetData((const BYTE*)pVXDHeader, sizeof(*pVXDHeader));

	pRoot->printf("\n");

	return true;
}

bool DumpExeFilePE( MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader, PIMAGE_NT_HEADERS32 pNTHeader )
{
	PBYTE pImageBase = (PBYTE)dosHeader;
	PIMAGE_NT_HEADERS64 pNTHeader64;

	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));
	DumpHeader(pRoot, dosHeader);
	pRoot->printf("\n");

	pNTHeader64 = (PIMAGE_NT_HEADERS64)pNTHeader;

    DumpHeader(pRoot, (PIMAGE_FILE_HEADER)&pNTHeader->FileHeader);
    pRoot->printf("\n");

	bool bIs64Bit = ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );
	g_bIs64Bit = bIs64Bit;

	if ( bIs64Bit )
	{
		gpNTHeader64 = pNTHeader64;
		DumpOptionalHeader(pRoot, &pNTHeader64->OptionalHeader);

		MPanelItem* pPE = pRoot->AddFile(_T("PE64_Header"), sizeof(*gpNTHeader64));
		pPE->SetData((const BYTE*)gpNTHeader64, sizeof(*gpNTHeader64));
	}
	else
	{
		gpNTHeader32 = (PIMAGE_NT_HEADERS32)pNTHeader;
		DumpOptionalHeader(pRoot, &pNTHeader->OptionalHeader);

		MPanelItem* pPE = pRoot->AddFile(_T("PE32_Header"), sizeof(*gpNTHeader32));
		pPE->SetData((const BYTE*)gpNTHeader32, sizeof(*gpNTHeader32));
	}

    pRoot->printf("\n");

	// IsExe = TRUE, means "NOT *.obj file"
    DumpSectionTable( pRoot, IMAGE_FIRST_SECTION(pNTHeader), 
                        pNTHeader->FileHeader.NumberOfSections, TRUE);
    pRoot->printf("\n");

	if ( bIs64Bit )
		DumpExeDebugDirectory( pRoot, pImageBase, pNTHeader64 );
	else
		DumpExeDebugDirectory( pRoot, pImageBase, pNTHeader );

    if ( pNTHeader->FileHeader.PointerToSymbolTable == 0 )
        g_pCOFFHeader = 0; // Doesn't really exist!
    pRoot->printf("\n");

	DumpResources(pRoot, pImageBase, pNTHeader);

	bIs64Bit
		? DumpTLSDirectory( pRoot, pImageBase, pNTHeader64, (PIMAGE_TLS_DIRECTORY64)0 )	// Passing NULL ptr is a clever hack
		: DumpTLSDirectory( pRoot, pImageBase, pNTHeader, (PIMAGE_TLS_DIRECTORY32)0 );		// See if you can figure it out! :-)

	bIs64Bit
	    ? DumpImportsSection(pRoot, pImageBase, pNTHeader64 )
		: DumpImportsSection(pRoot, pImageBase, pNTHeader);
    
	bIs64Bit
		? DumpDelayedImportsSection(pRoot, pImageBase, pNTHeader64, bIs64Bit )
		: DumpDelayedImportsSection(pRoot, pImageBase, pNTHeader, bIs64Bit );

	bIs64Bit 
		? DumpBoundImportDescriptors( pRoot, pImageBase, pNTHeader64 )
		: DumpBoundImportDescriptors( pRoot, pImageBase, pNTHeader );
    
	bIs64Bit
	    ? DumpExportsSection( pRoot, pImageBase, pNTHeader64 )
		: DumpExportsSection( pRoot, pImageBase, pNTHeader );

	bIs64Bit
		? DumpCOR20Header( pRoot, pImageBase, pNTHeader64 )
		: DumpCOR20Header( pRoot, pImageBase, pNTHeader );

	bIs64Bit
		? DumpLoadConfigDirectory( pRoot, pImageBase, pNTHeader64, (PIMAGE_LOAD_CONFIG_DIRECTORY64)0 )	// Passing NULL ptr is a clever hack
		: DumpLoadConfigDirectory( pRoot, pImageBase, pNTHeader, (PIMAGE_LOAD_CONFIG_DIRECTORY32)0 );	// See if you can figure it out! :-)

	bIs64Bit
		? DumpCertificates( pRoot, pImageBase, pNTHeader64 )
		: DumpCertificates( pRoot, pImageBase, pNTHeader );

	//=========================================================================
	//
	// If we have COFF symbols, create a symbol table now
	//
	//=========================================================================

	if ( g_pCOFFHeader )	// Did we see a COFF symbols header while looking
	{						// through the debug directory?
		g_pCOFFSymbolTable = new COFFSymbolTable(
				pImageBase+ pNTHeader->FileHeader.PointerToSymbolTable,
				pNTHeader->FileHeader.NumberOfSymbols );
	}

	if ( fShowPDATA )
	{
		bIs64Bit
			? DumpRuntimeFunctions( pRoot, pImageBase, pNTHeader64 )
			: DumpRuntimeFunctions( pRoot, pImageBase, pNTHeader ); 
				
		pRoot->printf( "\n" );
	}

    if ( fShowRelocations )
    {
        bIs64Bit
			? DumpBaseRelocationsSection( pRoot, pImageBase, pNTHeader64 )
			: DumpBaseRelocationsSection( pRoot, pImageBase, pNTHeader );
        pRoot->printf("\n");
    } 

	if ( fShowSymbolTable && g_pMiscDebugInfo )
	{
		DumpMiscDebugInfo( pRoot, g_pMiscDebugInfo );
		pRoot->printf( "\n" );
	}

	if ( fShowSymbolTable && g_pCVHeader )
	{
		DumpCVDebugInfoRecord( pRoot, g_pCVHeader );
		pRoot->printf( "\n" );
	}

    if ( fShowSymbolTable && g_pCOFFHeader )
    {
        DumpCOFFHeader( pRoot, g_pCOFFHeader );
        pRoot->printf("\n");
    }
    
    if ( fShowLineNumbers && g_pCOFFHeader )
    {
        DumpLineNumbers( pRoot, MakePtr(PIMAGE_LINENUMBER, g_pCOFFHeader,
                            g_pCOFFHeader->LvaToFirstLinenumber),
                            g_pCOFFHeader->NumberOfLinenumbers);
        pRoot->printf("\n");
    }

    if ( fShowSymbolTable )
    {
        if ( pNTHeader->FileHeader.NumberOfSymbols 
            && pNTHeader->FileHeader.PointerToSymbolTable
			&& g_pCOFFSymbolTable )
        {
            DumpCOFFSymbolTable( pRoot, g_pCOFFSymbolTable );
            pRoot->printf("\n");
        }
    }
    
	// 04.03.2010 Maks - ¬ Exe не инетересно видеть HexDump, да это еще и долго
	//if ( fShowRawSectionData )
	//{
	//	PIMAGE_SECTION_HEADER pSectionHdr;
	//
	//	pSectionHdr = bIs64Bit ? (PIMAGE_SECTION_HEADER)(pNTHeader64+1) : (PIMAGE_SECTION_HEADER)(pNTHeader+1);
	//
	//    DumpRawSectionData( pRoot, pSectionHdr, dosHeader, pNTHeader->FileHeader.NumberOfSections);
	//}

	if ( g_pCOFFSymbolTable )
		delete g_pCOFFSymbolTable;

	return true;
}
