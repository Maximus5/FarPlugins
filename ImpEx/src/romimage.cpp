//==================================
// PEDUMP - Matt Pietrek 1997-2001
// FILE: EXEDUMP.CPP
//==================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#pragma hdrstop
#include "common.h"
#include "exedump.h"
#include "COFFSymbolTable.h"
#include "extrnvar.h"

void DumpROMOptionalHeader( MPanelItem *pRoot, PIMAGE_ROM_OPTIONAL_HEADER pROMOptHdr )
{
    UINT width = 30;

    pRoot->printf("Optional Header\n");
    
    pRoot->printf("  %-*s%04X\n", width, "Magic", pROMOptHdr->Magic);
    pRoot->printf("  %-*s%u.%02u\n", width, "linker version",
        pROMOptHdr->MajorLinkerVersion,
        pROMOptHdr->MinorLinkerVersion);
    pRoot->printf("  %-*s%X\n", width, "size of code", pROMOptHdr->SizeOfCode);
    pRoot->printf("  %-*s%X\n", width, "size of initialized data",
        pROMOptHdr->SizeOfInitializedData);
    pRoot->printf("  %-*s%X\n", width, "size of uninitialized data",
        pROMOptHdr->SizeOfUninitializedData);
    pRoot->printf("  %-*s%X\n", width, "entrypoint RVA",
        pROMOptHdr->AddressOfEntryPoint);
    pRoot->printf("  %-*s%X\n", width, "base of code", pROMOptHdr->BaseOfCode);
    pRoot->printf("  %-*s%X\n", width, "base of Bss", pROMOptHdr->BaseOfBss);
    pRoot->printf("  %-*s%X\n", width, "GprMask", pROMOptHdr->GprMask);

	pRoot->printf("  %-*s%X\n", width, "CprMask[0]", pROMOptHdr->CprMask[0] );
	pRoot->printf("  %-*s%X\n", width, "CprMask[1]", pROMOptHdr->CprMask[1] );
	pRoot->printf("  %-*s%X\n", width, "CprMask[2]", pROMOptHdr->CprMask[2] );
	pRoot->printf("  %-*s%X\n", width, "CprMask[3]", pROMOptHdr->CprMask[3] );

    pRoot->printf("  %-*s%X\n", width, "GpValue", pROMOptHdr->GpValue);
}

// VARIATION on the IMAGE_FIRST_SECTION macro from WINNT.H

#define IMAGE_FIRST_ROM_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((DWORD_PTR)ntheader +                                                  \
     FIELD_OFFSET( IMAGE_ROM_HEADERS, OptionalHeader ) +                 \
     ((PIMAGE_ROM_HEADERS)(ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))

bool DumpROMImage( MPanelItem *pRoot, PIMAGE_ROM_HEADERS pROMHeader )
{
    DumpHeader(pRoot, &pROMHeader->FileHeader);
    pRoot->printf("\n");

    DumpROMOptionalHeader(pRoot, &pROMHeader->OptionalHeader);
    pRoot->printf("\n");

    DumpSectionTable( pRoot, IMAGE_FIRST_ROM_SECTION(pROMHeader), 
                        pROMHeader->FileHeader.NumberOfSections, TRUE);
    pRoot->printf("\n");

	// Dump COFF symbols out here.  Get offsets from the header
	return true;
}

