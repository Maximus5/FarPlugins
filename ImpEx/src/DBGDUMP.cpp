//==================================
// PEDUMP - Matt Pietrek 1997-2001
// FILE: DBGDUMP.CPP
//==================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "common.h"
#include "COFFSymbolTable.h"
#include "SymbolTableSupport.h"
#include "extrnvar.h"
#include "dbgdump.h"
#include "cvsymbols.h"

void DumpImageDbgHeader(MPanelItem *pRoot, PIMAGE_SEPARATE_DEBUG_HEADER pImageSepDbgHeader)
{
    UINT headerFieldWidth = 30;

    pRoot->printf("  %-*s%04X\n", headerFieldWidth, "Flags:",
                pImageSepDbgHeader->Flags);
    pRoot->printf("  %-*s%04X %s\n", headerFieldWidth, "Machine:",
                pImageSepDbgHeader->Machine,
                GetMachineTypeName(pImageSepDbgHeader->Machine));
    pRoot->printf("  %-*s%04X\n", headerFieldWidth, "Characteristics:",
                pImageSepDbgHeader->Characteristics);
	__time32_t timeStamp = pImageSepDbgHeader->TimeDateStamp;
    pRoot->printf("  %-*s%08X -> %s", headerFieldWidth, "TimeDateStamp:",
                pImageSepDbgHeader->TimeDateStamp, _ctime32(&timeStamp) );
    pRoot->printf("  %-*s%08X\n", headerFieldWidth, "CheckSum:",
                pImageSepDbgHeader->CheckSum);
    pRoot->printf("  %-*s%08X\n", headerFieldWidth, "ImageBase:",
                pImageSepDbgHeader->ImageBase);
    pRoot->printf("  %-*s%08X\n", headerFieldWidth, "Size of Image:",
                pImageSepDbgHeader->SizeOfImage);
    pRoot->printf("  %-*s%04X\n", headerFieldWidth, "Number of Sections:",
                pImageSepDbgHeader->NumberOfSections);
    pRoot->printf("  %-*s%04X\n", headerFieldWidth, "ExportedNamesSize:",
                pImageSepDbgHeader->ExportedNamesSize);
    pRoot->printf("  %-*s%08X\n", headerFieldWidth, "DebugDirectorySize:",
                pImageSepDbgHeader->DebugDirectorySize);
    pRoot->printf("  %-*s%08X\n", headerFieldWidth, "SectionAlignment:",
                pImageSepDbgHeader->SectionAlignment);
}

bool DumpDbgFile( MPanelItem *pRoot, PIMAGE_SEPARATE_DEBUG_HEADER pImageSepDbgHeader )
{
    DumpImageDbgHeader(pRoot, pImageSepDbgHeader);
    pRoot->printf("\n");
    
    DumpSectionTable( pRoot, (PIMAGE_SECTION_HEADER)(pImageSepDbgHeader+1),
                        pImageSepDbgHeader->NumberOfSections, TRUE);
                    
    DumpDebugDirectory(
        pRoot, MakePtr(PIMAGE_DEBUG_DIRECTORY,
        pImageSepDbgHeader, sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
        (pImageSepDbgHeader->NumberOfSections * sizeof(IMAGE_SECTION_HEADER))
        + pImageSepDbgHeader->ExportedNamesSize),
        pImageSepDbgHeader->DebugDirectorySize,
        (PBYTE)pImageSepDbgHeader);
    
    pRoot->printf("\n");
    
    if ( g_pCOFFHeader )
	{
        DumpCOFFHeader( pRoot, g_pCOFFHeader );
    
		pRoot->printf("\n");

		g_pCOFFSymbolTable = new COFFSymbolTable(
			MakePtr( PVOID, g_pCOFFHeader, g_pCOFFHeader->LvaToFirstSymbol),
			g_pCOFFHeader->NumberOfSymbols );


		DumpCOFFSymbolTable( pRoot, g_pCOFFSymbolTable );

		delete g_pCOFFSymbolTable;
	}
	
	if ( g_pCVHeader )
	{
		DumpCVSymbolTable( pRoot, (PBYTE)g_pCVHeader, g_pMappedFileBase );
	}
	return true;
}
