//==================================
// PEDUMP - Matt Pietrek 1997-2001
// FILE: COMMON.C
//==================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "symboltablesupport.h"
#include "extrnvar.h"

PIMAGE_DEBUG_MISC g_pMiscDebugInfo = 0;
PDWORD g_pCVHeader = 0;
PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader = 0;
COFFSymbolTable * g_pCOFFSymbolTable = 0;
extern bool g_bUPXed;

/*----------------------------------------------------------------------------*/
//
// Header related stuff
//
/*----------------------------------------------------------------------------*/

static const TCHAR cszCharacteristics[] = _T("Charact.");
static const TCHAR cszRawVirtSize[] = _T("Raw/Virtual size");


// Bitfield values and names for the IMAGE_FILE_HEADER flags
WORD_FLAG_DESCRIPTIONS ImageFileHeaderCharacteristics[] = 
{
{ IMAGE_FILE_RELOCS_STRIPPED, "RELOCS_STRIPPED" },
{ IMAGE_FILE_EXECUTABLE_IMAGE, "EXECUTABLE_IMAGE" },
{ IMAGE_FILE_LINE_NUMS_STRIPPED, "LINE_NUMS_STRIPPED" },
{ IMAGE_FILE_LOCAL_SYMS_STRIPPED, "LOCAL_SYMS_STRIPPED" },
{ IMAGE_FILE_AGGRESIVE_WS_TRIM, "AGGRESIVE_WS_TRIM" },
{ IMAGE_FILE_LARGE_ADDRESS_AWARE, "LARGE_ADDRESS_AWARE" },
{ IMAGE_FILE_BYTES_REVERSED_LO, "BYTES_REVERSED_LO" },
{ IMAGE_FILE_32BIT_MACHINE, "32BIT_MACHINE" },
{ IMAGE_FILE_DEBUG_STRIPPED, "DEBUG_STRIPPED" },
{ IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, "REMOVABLE_RUN_FROM_SWAP" },
{ IMAGE_FILE_NET_RUN_FROM_SWAP, "NET_RUN_FROM_SWAP" },
{ IMAGE_FILE_SYSTEM, "SYSTEM" },
{ IMAGE_FILE_DLL, "DLL" },
{ IMAGE_FILE_UP_SYSTEM_ONLY, "UP_SYSTEM_ONLY" },
{ IMAGE_FILE_BYTES_REVERSED_HI, "BYTES_REVERSED_HI" }
// { IMAGE_FILE_MINIMAL_OBJECT, "MINIMAL_OBJECT" }, // Removed in NT 3.5
// { IMAGE_FILE_UPDATE_OBJECT, "UPDATE_OBJECT" },   // Removed in NT 3.5
// { IMAGE_FILE_16BIT_MACHINE, "16BIT_MACHINE" },   // Removed in NT 3.5
// { IMAGE_FILE_PATCH, "PATCH" },
};

#define NUMBER_IMAGE_HEADER_FLAGS \
    (sizeof(ImageFileHeaderCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

void DumpHeader(MPanelItem *pRoot, PIMAGE_DOS_HEADER pDos)
{
    UINT headerFieldWidth = 35;
    //UINT i;

	MPanelItem *pChild = pRoot->AddFile(_T("DOS_Header"), sizeof(*pDos));
	pChild->SetData((const BYTE*)pDos, sizeof(*pDos));
    
	//MPanelItem *pChild = pRoot->AddFolder(_T("File Header"));
	pChild = pRoot->AddFile(_T("DOS_Header.txt"));
    
    pChild->AddText(_T("<DOS Header>\n"));
    
    if (!ValidateMemory(pDos, sizeof(*pDos)))
    {
    	pChild->SetErrorPtr(pDos, sizeof(*pDos)); return;
    }

    //pChild->printf("  %-*s%04X (%s)\n", headerFieldWidth, "Machine:", 
    //            pImageFileHeader->Machine,
    //            GetMachineTypeName(pImageFileHeader->Machine) );
    pChild->printf(_T("  %-*s%04X (%c%c)\n"), headerFieldWidth,
    		_T("Magic number:"), pDos->e_magic, (TCHAR)(pDos->e_magic & 0xFF), (TCHAR)((pDos->e_magic & 0xFF00)>>8));
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Bytes on last page of file:"), pDos->e_cblp, pDos->e_cblp);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Pages in file:"), pDos->e_cp, pDos->e_cp);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Relocations:"), pDos->e_crlc, pDos->e_crlc);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Size of header in paragraphs:"), pDos->e_cparhdr, pDos->e_cparhdr);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Minimum extra paragraphs needed:"), pDos->e_minalloc, pDos->e_minalloc);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Maximum extra paragraphs needed:"), pDos->e_maxalloc, pDos->e_maxalloc);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Initial (relative) SS value:"), pDos->e_ss, pDos->e_ss);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Initial SP value:"), pDos->e_sp, pDos->e_sp);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Checksum:"), pDos->e_csum, pDos->e_csum);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Initial IP value:"), pDos->e_ip, pDos->e_ip);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Initial (relative) CS value:"), pDos->e_cs, pDos->e_cs);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("File address of relocation table:"), pDos->e_lfarlc, pDos->e_lfarlc);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("Overlay number:"), pDos->e_ovno, pDos->e_ovno);
    pChild->printf(_T("  %-*sx%04hX,x%04hX,x%04hX,x%04hX\n"), headerFieldWidth,
    		_T("Reserved words (4):"), pDos->e_res[0], pDos->e_res[1], pDos->e_res[2], pDos->e_res[3]);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("OEM identifier:"), pDos->e_oemid, pDos->e_oemid);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("OEM information:"), pDos->e_oeminfo, pDos->e_oeminfo);
    pChild->printf(_T("  %-*s%04X (%hu)\n"), headerFieldWidth,
    		_T("File address of new exe header:"), pDos->e_lfanew, pDos->e_lfanew);
    pChild->printf(_T("  %-*sx%04hX,x%04hX,x%04hX,x%04hX,x%04hX,x%04hX,x%04hX,x%04hX,x%04hX,x%04hX\n"), headerFieldWidth,
    		_T("Reserved words (10):"), pDos->e_res2[0], pDos->e_res2[1], pDos->e_res2[2], pDos->e_res2[3],
    		pDos->e_res2[4], pDos->e_res2[5], pDos->e_res2[6], pDos->e_res2[7],
    		pDos->e_res2[8], pDos->e_res2[9]);
}
    
//
// Dump the IMAGE_FILE_HEADER for a PE file or an OBJ
//
void DumpHeader(MPanelItem *pRoot, PIMAGE_FILE_HEADER pImageFileHeader)
{
    UINT headerFieldWidth = 30;
    UINT i;

	//MPanelItem *pChild = pRoot->AddFolder(_T("File Header"));
	MPanelItem *pChild = pRoot->AddFile(_T("FILE_Header"), sizeof(*pImageFileHeader));
    if (!ValidateMemory(pImageFileHeader, sizeof(*pImageFileHeader)))
    {
    	pChild->SetErrorPtr(pImageFileHeader, sizeof(*pImageFileHeader)); return;
    }
	pChild->SetData((const BYTE*)pImageFileHeader, sizeof(*pImageFileHeader));	
	
	
	pChild = pRoot->AddFile(_T("FILE_Header.txt"));
    
    pChild->AddText(_T("<File Header>\n"));
    
    pChild->printf("  %-*s%04X (%s)\n", headerFieldWidth, "Machine:", 
                pImageFileHeader->Machine,
                GetMachineTypeName(pImageFileHeader->Machine) );
    pChild->printf("  %-*s%04X\n", headerFieldWidth, "Number of Sections:",
                pImageFileHeader->NumberOfSections);
	__time32_t timeStamp = pImageFileHeader->TimeDateStamp;
    pChild->printf("  %-*s%08X -> %s", headerFieldWidth, "TimeDateStamp:",
                pImageFileHeader->TimeDateStamp, _ctime32( &timeStamp) );
    pChild->printf("  %-*s%08X\n", headerFieldWidth, "PointerToSymbolTable:",
                pImageFileHeader->PointerToSymbolTable);
    pChild->printf("  %-*s%08X\n", headerFieldWidth, "NumberOfSymbols:",
                pImageFileHeader->NumberOfSymbols);
    pChild->printf("  %-*s%04X\n", headerFieldWidth, "SizeOfOptionalHeader:",
                pImageFileHeader->SizeOfOptionalHeader);
    pChild->printf("  %-*s%04X\n", headerFieldWidth, "Characteristics:",
                pImageFileHeader->Characteristics);
    for ( i=0; i < NUMBER_IMAGE_HEADER_FLAGS; i++ )
    {
        if ( pImageFileHeader->Characteristics & 
             ImageFileHeaderCharacteristics[i].flag )
            pChild->printf( "    %s\n", ImageFileHeaderCharacteristics[i].name );
    }

	//// Чтобы при входе в корень сразу была видна "битность"
	//pRoot->Root()->AddChild(
	//	(pImageFileHeader->Characteristics & IMAGE_FILE_32BIT_MACHINE)
	//	? _T("32BIT_MACHINE") : _T("64BIT_MACHINE"),
	//	FILE_ATTRIBUTE_NORMAL, 0);
}

WORD_VALUE_NAMES g_arMachines[] = 
{
{ IMAGE_FILE_MACHINE_UNKNOWN, "UNKNOWN" },
{ IMAGE_FILE_MACHINE_I386, "I386" },
{ IMAGE_FILE_MACHINE_R3000, "R3000" },
{ IMAGE_FILE_MACHINE_R4000, "R4000" },
{ IMAGE_FILE_MACHINE_R10000, "R10000" },
{ IMAGE_FILE_MACHINE_WCEMIPSV2, "WCEMIPSV2" },
{ IMAGE_FILE_MACHINE_ALPHA, "ALPHA" },
{ IMAGE_FILE_MACHINE_SH3, "SH3" },
{ IMAGE_FILE_MACHINE_SH3DSP, "SH3DSP" },
{ IMAGE_FILE_MACHINE_SH3E, "SH3E" },
{ IMAGE_FILE_MACHINE_SH4, "SH4" },
{ IMAGE_FILE_MACHINE_SH5, "SH5" },
{ IMAGE_FILE_MACHINE_ARM, "ARM" },
{ IMAGE_FILE_MACHINE_THUMB, "THUMB" },
{ IMAGE_FILE_MACHINE_AM33, "AM33" },
{ IMAGE_FILE_MACHINE_POWERPC, "POWERPC" },
{ IMAGE_FILE_MACHINE_POWERPCFP, "POWERPCFP" },
{ IMAGE_FILE_MACHINE_IA64, "IA64" },
{ IMAGE_FILE_MACHINE_MIPS16, "MIPS16" },
{ IMAGE_FILE_MACHINE_ALPHA64, "ALPHA64" },
{ IMAGE_FILE_MACHINE_MIPSFPU, "MIPSFPU" },
{ IMAGE_FILE_MACHINE_MIPSFPU16, "MIPSFPU16" },
{ IMAGE_FILE_MACHINE_TRICORE, "TRICORE" },
{ IMAGE_FILE_MACHINE_CEF, "CEF" },
{ IMAGE_FILE_MACHINE_EBC, "EBC" },
{ IMAGE_FILE_MACHINE_AMD64, "AMD64" },
{ IMAGE_FILE_MACHINE_M32R, "M32R" },
{ IMAGE_FILE_MACHINE_CEE, "CEE" },
};


PSTR GetMachineTypeName( WORD wMachineType )
{
	for ( unsigned i = 0; i < ARRAY_SIZE(g_arMachines); i++ )
		if ( wMachineType == g_arMachines[i].wValue )
			return g_arMachines[i].pszName;

	return "unknown";
}

bool IsValidMachineType( WORD wMachineType, BOOL bCommonOnly /*= FALSE*/ )
{
	if (bCommonOnly)
	{
		// Для "быстрого" определения из OpenFilePlugin
		if (wMachineType == IMAGE_FILE_MACHINE_I386
			|| wMachineType == IMAGE_FILE_MACHINE_IA64
			|| wMachineType == IMAGE_FILE_MACHINE_AMD64)
		{
			return true;
		}
		return false;
	}

	for ( unsigned i = 0; i < ARRAY_SIZE(g_arMachines); i++ )
		if ( wMachineType == g_arMachines[i].wValue )
			return true;

	return false;
}

/*----------------------------------------------------------------------------*/
//
// Section related stuff
//
/*----------------------------------------------------------------------------*/

// Bitfield values and names for the IMAGE_SECTION_HEADER flags
DWORD_FLAG_DESCRIPTIONS SectionCharacteristics[] = 
{

// { IMAGE_SCN_TYPE_DSECT, "DSECT" },
// { IMAGE_SCN_TYPE_NOLOAD, "NOLOAD" },
// { IMAGE_SCN_TYPE_GROUP, "GROUP" },
{ IMAGE_SCN_TYPE_NO_PAD, "NO_PAD" },
// { IMAGE_SCN_TYPE_COPY, "COPY" },
{ IMAGE_SCN_CNT_CODE, "CODE" },
{ IMAGE_SCN_CNT_INITIALIZED_DATA, "INITIALIZED_DATA", _T('I') },
{ IMAGE_SCN_CNT_UNINITIALIZED_DATA, "UNINITIALIZED_DATA", _T('U') },
{ IMAGE_SCN_LNK_OTHER, "OTHER" },
{ IMAGE_SCN_LNK_INFO, "INFO" },
// { IMAGE_SCN_TYPE_OVER, "OVER" },
{ IMAGE_SCN_LNK_REMOVE, "REMOVE" },
{ IMAGE_SCN_LNK_COMDAT, "COMDAT" },
// { IMAGE_SCN_MEM_PROTECTED, "PROTECTED" },
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
{ IMAGE_SCN_MEM_FARDATA, "FARDATA" },
// { IMAGE_SCN_MEM_SYSHEAP, "SYSHEAP" },
{ IMAGE_SCN_MEM_PURGEABLE, "PURGEABLE" },
{ IMAGE_SCN_MEM_LOCKED, "LOCKED" },
{ IMAGE_SCN_MEM_PRELOAD, "PRELOAD", _T('P') },
{ IMAGE_SCN_LNK_NRELOC_OVFL, "NRELOC_OVFL" },
{ IMAGE_SCN_MEM_DISCARDABLE, "DISCARDABLE" },
{ IMAGE_SCN_MEM_NOT_CACHED, "NOT_CACHED" },
{ IMAGE_SCN_MEM_NOT_PAGED, "NOT_PAGED" },
{ IMAGE_SCN_MEM_SHARED, "SHARED", _T('S') },
{ IMAGE_SCN_MEM_EXECUTE, "EXECUTE", _T('E') },
{ IMAGE_SCN_MEM_READ, "READ", _T('R') },
{ IMAGE_SCN_MEM_WRITE, "WRITE", _T('W') },
};


#define NUMBER_SECTION_CHARACTERISTICS \
    (sizeof(SectionCharacteristics) / sizeof(DWORD_FLAG_DESCRIPTIONS))

//
// Dump the section table from a PE file or an OBJ
//
void DumpSectionTable(MPanelItem *pRoot, PIMAGE_SECTION_HEADER section,
                      unsigned cSections,
                      BOOL IsEXE)
{
	MPanelItem *pChild = pRoot->AddFolder(_T("Section Table"));

    pChild->AddText(_T("<Section Table>\n"));

	pChild->SetColumnsTitles(cszCharacteristics, 7, cszRawVirtSize, 21);
    
    for ( unsigned i=1; i <= cSections; i++, section++ )
    {
		//MAX section name length is 8, but may be not zero terminated
		char cSectName[9];
		lstrcpynA(cSectName, (char*)section->Name, 9);
		MPanelItem *pSect = pChild->AddFile(cSectName, section->Misc.VirtualSize);

		if (section->Name[0] == 'U' && section->Name[1] == 'P' && section->Name[2] == 'X') {
			g_bUPXed = true;
			pRoot->Root()->AddFlags(_T("UPX"));
		}

		// Как то странно смотрится: IsEXE ? "VirtSize" : "PhysAddr",
		// но так было в оригинале PEDUMP
        pSect->printf( "  %02X %-8.8s  %s: %08X  VirtAddr:  %08X\n",
                i, section->Name,
                IsEXE ? "VirtSize" : "PhysAddr",
                section->Misc.VirtualSize, section->VirtualAddress);
        pSect->printf( "    raw data offs:   %08X  raw data size: %08X\n",
                section->PointerToRawData, section->SizeOfRawData );
        pSect->printf( "    relocation offs: %08X  relocations:   %08X\n",
                section->PointerToRelocations, section->NumberOfRelocations );
        pSect->printf( "    line # offs:     %08X  line #'s:      %08X\n",
                section->PointerToLinenumbers, section->NumberOfLinenumbers );
        pSect->printf( "    characteristics: %08X\n", section->Characteristics);

        pSect->printf("    ");
		TCHAR sChars[32]; TCHAR *pszChars = sChars; *pszChars = 0; TCHAR chCurAbbr = 0;
        for ( unsigned j=0; j < NUMBER_SECTION_CHARACTERISTICS; j++ )
        {
			chCurAbbr = 0;
            if ( section->Characteristics & 
                SectionCharacteristics[j].flag )
			{
                pSect->printf( "  %s", SectionCharacteristics[j].name );
				if (SectionCharacteristics[j].abbr)
					chCurAbbr = SectionCharacteristics[j].abbr;
			} else if (SectionCharacteristics[j].abbr)
				chCurAbbr = _T(' ');
			if (chCurAbbr) {
				*(pszChars++) = chCurAbbr;
				*pszChars = 0;
			}
        }
		TCHAR sRawVirtSize[64];
		wsprintf(sRawVirtSize, _T("0x%08X/0x%08X"), section->SizeOfRawData, section->Misc.VirtualSize);
		pSect->SetColumns(sChars, sRawVirtSize);

		unsigned alignment = (section->Characteristics & IMAGE_SCN_ALIGN_MASK);
		if ( alignment == 0 )
		{
			pSect->printf( "  ALIGN_DEFAULT(16)" );
		}
		else
		{
			// Yeah, it's hard to read this, but it works, and it's elegant
			alignment = alignment >>= 20;
			pSect->printf( "  ALIGN_%uBYTES", 1 << (alignment-1) );
		}
		
        pSect->printf("\n\n");
		
		if (gpNTHeader32 || gpNTHeader64) {
			LPVOID ptrSect = NULL;
			if (g_bIs64Bit)
				ptrSect = GetPtrFromRVA(section->VirtualAddress, gpNTHeader64, g_pMappedFileBase);
			else
				ptrSect = GetPtrFromRVA(section->VirtualAddress, gpNTHeader32, g_pMappedFileBase);
			// section->Misc.VirtualSize - If this value is greater than the SizeOfRawData member, the section is filled with zeroes
			if (ptrSect)
				pSect->SetData((const BYTE*)ptrSect, section->SizeOfRawData);
		} else {
			// Dumping *.obj file?
			if (section->PointerToRawData && section->SizeOfRawData) {
				LPVOID ptrSect = g_pMappedFileBase+section->PointerToRawData;
				pSect->SetData((const BYTE*)ptrSect, section->SizeOfRawData);
			}
		}
    }
}

//
// Given a section name, look it up in the section table and return a
// pointer to the start of its raw data area.
//
LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, PBYTE imageBase)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
    {
        if (strncmp((char *)section->Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0)
            return (LPVOID)(section->PointerToRawData + imageBase);
    }
    
    return 0;
}


PIMAGE_COFF_SYMBOLS_HEADER PCOFFDebugInfo = 0;

char *SzDebugFormats[] = {
"UNKNOWN/BORLAND","COFF","CODEVIEW","FPO","MISC","EXCEPTION","FIXUP",
"OMAP_TO_SRC", "OMAP_FROM_SRC"};

//
// Dump the debug directory array
//
void DumpDebugDirectory(MPanelItem *pRoot, PIMAGE_DEBUG_DIRECTORY debugDir, DWORD size, PBYTE pImageBase)
{
    DWORD cDebugFormats = size / sizeof(IMAGE_DEBUG_DIRECTORY);
    PSTR szDebugFormat;
    unsigned i;
    
    if ( cDebugFormats == 0 )
        return;
    
    pRoot->printf(
    "Debug Formats in File\n"
    "  Type            Size     Address  FilePtr  Charactr TimeDate Version\n"
    "  --------------- -------- -------- -------- -------- -------- --------\n"
    );
    
    for ( i=0; i < cDebugFormats; i++ )
    {
        szDebugFormat = (debugDir->Type <= IMAGE_DEBUG_TYPE_OMAP_FROM_SRC )
                        ? SzDebugFormats[debugDir->Type] : "???";

        pRoot->printf("  %-15s %08X %08X %08X %08X %08X %u.%02u\n",
            szDebugFormat, debugDir->SizeOfData, debugDir->AddressOfRawData,
            debugDir->PointerToRawData, debugDir->Characteristics,
            debugDir->TimeDateStamp, debugDir->MajorVersion,
            debugDir->MinorVersion);

		switch( debugDir->Type )
		{
        	case IMAGE_DEBUG_TYPE_COFF:
	            g_pCOFFHeader =
                (PIMAGE_COFF_SYMBOLS_HEADER)(pImageBase+ debugDir->PointerToRawData);
				break;

			case IMAGE_DEBUG_TYPE_MISC:
				g_pMiscDebugInfo =
				(PIMAGE_DEBUG_MISC)(pImageBase + debugDir->PointerToRawData);
				break;

			case IMAGE_DEBUG_TYPE_CODEVIEW:
				g_pCVHeader = (PDWORD)(pImageBase + debugDir->PointerToRawData);
				break;
		}

        debugDir++;
    }
}

/*----------------------------------------------------------------------------*/
//
// Other assorted stuff
//
/*----------------------------------------------------------------------------*/

//
// Do a hexadecimal dump of the raw data for all the sections.  You
// could just dump one section by adjusting the PIMAGE_SECTION_HEADER
// and cSections parameters
//
void DumpRawSectionData(MPanelItem *pRoot, PIMAGE_SECTION_HEADER section,
                        PVOID base,
                        unsigned cSections)
{
    unsigned i;
    char name[IMAGE_SIZEOF_SHORT_NAME + 1];

    pRoot->printf("Section Hex Dumps\n");
    
    for ( i=1; i <= cSections; i++, section++ )
    {
        // Make a copy of the section name so that we can ensure that
        // it's null-terminated
        memcpy(name, section->Name, IMAGE_SIZEOF_SHORT_NAME);
        name[IMAGE_SIZEOF_SHORT_NAME] = 0;

        // Don't dump sections that don't exist in the file!
        if ( section->PointerToRawData == 0 )
            continue;
        
        pRoot->printf( "section %02X (%s)  size: %08X  file offs: %08X\n",
                i, name, section->SizeOfRawData, section->PointerToRawData);

        HexDump( pRoot, MakePtr(PBYTE, base, section->PointerToRawData),
                 section->SizeOfRawData );
        pRoot->printf("\n");
    }
}

// Number of hex values displayed per line
#define HEX_DUMP_WIDTH 16

//
// Dump a region of memory in a hexadecimal format
//
void HexDump(MPanelItem *pRoot, PBYTE ptr, DWORD length)
{
    char buffer[256];
    PSTR buffPtr, buffPtr2;
    unsigned cOutput, i;
    DWORD bytesToGo=length;

    while ( bytesToGo  )
    {
        cOutput = bytesToGo >= HEX_DUMP_WIDTH ? HEX_DUMP_WIDTH : bytesToGo;

        buffPtr = buffer;
        buffPtr += sprintf(buffPtr, "%08X:  ", length-bytesToGo );
        buffPtr2 = buffPtr + (HEX_DUMP_WIDTH * 3) + 1;
        
        for ( i=0; i < HEX_DUMP_WIDTH; i++ )
        {
            BYTE value = *(ptr+i);

            if ( i >= cOutput )
            {
                // On last line.  Pad with spaces
                *buffPtr++ = ' ';
                *buffPtr++ = ' ';
                *buffPtr++ = ' ';
            }
            else
            {
                if ( value < 0x10 )
                {
                    *buffPtr++ = '0';
                    _itoa( value, buffPtr++, 16);
                }
                else
                {
                    _itoa( value, buffPtr, 16);
                    buffPtr+=2;
                }
 
                *buffPtr++ = ' ';
                *buffPtr2++ = isprint(value) ? value : '.';
            }
            
            // Put an extra space between the 1st and 2nd half of the bytes
            // on each line.
            if ( i == (HEX_DUMP_WIDTH/2)-1 )
                *buffPtr++ = ' ';
        }

        *buffPtr2 = 0;  // Null terminate it.
        pRoot->AddText(buffer); // Can't use simple printf(), since there may be a '%' in the string.
        pRoot->AddText("\n");
        bytesToGo -= cOutput;
        ptr += HEX_DUMP_WIDTH;
    }
}


