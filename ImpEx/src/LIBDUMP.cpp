//==================================
// PEDUMP - Matt Pietrek 1997-2001
// FILE: LIBDUMP.CPP
//==================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "common.h"
#include "objdump.h"
#include "libdump.h"
#include "extrnvar.h"

PSTR PszLongnames = 0;

DWORD ConvertBigEndian(DWORD bigEndian);

void DisplayArchiveMemberHeader(
    MPanelItem *pRoot, PIMAGE_ARCHIVE_MEMBER_HEADER pArchHeader,
    DWORD fileOffset )
{
	pRoot->printf( "---------------------------------\n" );
    pRoot->printf( "Archive Member Header (0x%08X):\n", fileOffset );

    pRoot->printf("  Name:     %.16s", pArchHeader->Name);
    if ( pArchHeader->Name[0] == '/' && isdigit(pArchHeader->Name[1]) )
        pRoot->printf( "  (%s)\n", PszLongnames + atoi((char *)pArchHeader->Name+1) );
    pRoot->printf("\n");

	char szDateAsLong[64];
	sprintf( szDateAsLong, "%.12s", pArchHeader->Date );
	LONG dateAsLong = atol(szDateAsLong);
	__time32_t timeStamp = dateAsLong;

    pRoot->printf("  Date:     %.12s %s", pArchHeader->Date, _ctime32(&timeStamp) );
    pRoot->printf("  UserID:   %.6s\n", pArchHeader->UserID);
    pRoot->printf("  GroupID:  %.6s\n", pArchHeader->GroupID);
    pRoot->printf("  Mode:     %.8s\n", pArchHeader->Mode);
    pRoot->printf("  Size:     %.10s\n", pArchHeader->Size);
}

void DumpFirstLinkerMember(MPanelItem *pRoot, PVOID p)
{
    DWORD cSymbols = *(PDWORD)p;
    PDWORD pMemberOffsets = MakePtr( PDWORD, p, 4 );
    PSTR pSymbolName;
    unsigned i;

    cSymbols = ConvertBigEndian(cSymbols);
    pSymbolName = MakePtr( PSTR, pMemberOffsets, 4 * cSymbols );
    
    pRoot->printf("First Linker Member:\n");
    pRoot->printf( "  Symbols:         %08X\n", cSymbols );
    pRoot->printf( "  MbrOffs   Name\n  --------  ----\n" );
        
    for ( i = 0; i < cSymbols; i++ )
    {
        DWORD offset;
        
        offset = ConvertBigEndian( *pMemberOffsets );
        
        pRoot->printf("  %08X  %s\n", offset, pSymbolName);
        
        pMemberOffsets++;
        pSymbolName += strlen(pSymbolName) + 1;
    }
}

void DumpSecondLinkerMember(MPanelItem *pRoot, PVOID p)
{
    DWORD cArchiveMembers = *(PDWORD)p;
    PDWORD pMemberOffsets = MakePtr( PDWORD, p, 4 );
    DWORD cSymbols;
    PSTR pSymbolName;
    PWORD pIndices;
    unsigned i;

    cArchiveMembers = cArchiveMembers;

    // The number of symbols is in the DWORD right past the end of the
    // member offset array.
    cSymbols = pMemberOffsets[cArchiveMembers];

    pIndices = MakePtr( PWORD, p, 4 + cArchiveMembers * sizeof(DWORD) + 4 );

    pSymbolName = MakePtr( PSTR, pIndices, cSymbols * sizeof(WORD) );
    
    pRoot->printf("Second Linker Member:\n");
    
    pRoot->printf( "  Archive Members: %08X\n", cArchiveMembers );
    pRoot->printf( "  Symbols:         %08X\n", cSymbols );
    pRoot->printf( "  MbrOffs   Name\n  --------  ----\n" );

    for ( i = 0; i < cSymbols; i++ )
    {
        pRoot->printf("  %08X  %s\n", pMemberOffsets[pIndices[i] - 1], pSymbolName);
        pSymbolName += strlen(pSymbolName) + 1;
    }
}

void DumpLongnamesMember(MPanelItem *pRoot, PVOID p, DWORD len)
{
    PSTR pszName = (PSTR)p;
    DWORD offset = 0;

    PszLongnames = (PSTR)p;     // Save off pointer for use when dumping
                                // out OBJ member names

    pRoot->printf("Longnames:\n");
    
    // The longnames member is a series of null-terminated string.  Print
    // out the offset of each string (in decimal), followed by the string.
    while ( offset < len )
    {
        unsigned cbString = lstrlenA( pszName )+1;

        pRoot->printf("  %05u: %s\n", offset, pszName);
        offset += cbString;
        pszName += cbString;
    }
}

// This routine is for the new (NT4,SP3) import library records.  If this
// doesn't compile for you, you  can #ifdef it out.
void DumpImportLibraryRecord( MPanelItem *pRoot, IMPORT_OBJECT_HEADER * pImpObjHdr )
{
	pRoot->printf( "  Import Library Record\n" );
	pRoot->printf( "  Version: %u  Machine: %X  TimeDate: %08X  SizeOfData:%u\n",
				pImpObjHdr->Version, pImpObjHdr->Machine,
				pImpObjHdr->TimeDateStamp, pImpObjHdr->SizeOfData );
				
	if ( pImpObjHdr->NameType & IMPORT_OBJECT_ORDINAL )
		pRoot->printf( "  Ordinal: %u\n", pImpObjHdr->Ordinal );
	else
		pRoot->printf( "  Hint: %u\n", pImpObjHdr->Hint );
	
	#define CASE_STRING( x ) case x: s = #x; break;

	PSTR s = "";

	switch( pImpObjHdr->Type )
	{
		CASE_STRING( IMPORT_OBJECT_CODE )
		CASE_STRING( IMPORT_OBJECT_DATA )
		CASE_STRING( IMPORT_OBJECT_CONST )
	}	
	pRoot->printf( "  Type: %s (%u)\n", s, pImpObjHdr->Type );

	s = "";
	switch( pImpObjHdr->NameType )
	{
		CASE_STRING( IMPORT_OBJECT_ORDINAL )
		CASE_STRING( IMPORT_OBJECT_NAME )
		CASE_STRING( IMPORT_OBJECT_NAME_NO_PREFIX )
		CASE_STRING( IMPORT_OBJECT_NAME_UNDECORATE )
	}	
	pRoot->printf( "  NameType: %s (%u)\n", s, pImpObjHdr->NameType );
	
	PSTR pszSymName = (PSTR)(pImpObjHdr+1);
	PSTR pszDllName = pszSymName + strlen(pszSymName)+1;
	
	pRoot->printf( "  Symbol: %s\n", pszSymName );
	pRoot->printf( "  DLL:    %s\n", pszDllName );
					
	pRoot->printf( "\n\n" );
}

bool DumpLibFile( MPanelItem *pRoot, LPVOID lpFileBase )
{
    PIMAGE_ARCHIVE_MEMBER_HEADER pArchHeader;
    BOOL fSawFirstLinkerMember = FALSE;
    BOOL fSawSecondLinkerMember = FALSE;
    BOOL fBreak = FALSE;

    if ( strncmp((char *)lpFileBase,IMAGE_ARCHIVE_START,
                            		IMAGE_ARCHIVE_START_SIZE ) )
    {
        merror(_T("Not a valid .LIB file - signature not found\n"));
        return false;
    }
    
    pArchHeader = MakePtr(PIMAGE_ARCHIVE_MEMBER_HEADER, lpFileBase,
                            IMAGE_ARCHIVE_START_SIZE);

    while ( pArchHeader )
    {
        DWORD thisMemberSize;

		DWORD fileOffset = static_cast<DWORD>((PBYTE)pArchHeader - (PBYTE)lpFileBase);
		char szArcName[64] = {0};
		lstrcpynA(szArcName, (LPCSTR)pArchHeader->Name, 16);
		for (int i=15; i>=0 && (szArcName[i]==' ' || szArcName[i]==0); i--) szArcName[i]=0;
		sprintf(szArcName+strlen(szArcName), ".0x%08X", fileOffset);
		MPanelItem* pChild = pRoot->AddFolder(szArcName);
        
        DisplayArchiveMemberHeader( pChild, pArchHeader, fileOffset);
        pChild->printf("\n");

		// These first few lines check to see if it's one of the new (NT4,SP3)
		// import library records.  If this doesn't compile for you, you 
		// can #ifdef them out.
		IMPORT_OBJECT_HEADER * pImpObjHdr
			= (IMPORT_OBJECT_HEADER *)(pArchHeader+1);

		if (   (pImpObjHdr->Sig1 == IMAGE_FILE_MACHINE_UNKNOWN)
			&& (pImpObjHdr->Sig2 == IMPORT_OBJECT_HDR_SIG2) )
		{	
			DumpImportLibraryRecord( pChild, pImpObjHdr );		
		}			
        else if ( !strncmp( (char *)pArchHeader->Name,
        				IMAGE_ARCHIVE_LINKER_MEMBER, 16) )
        {
            if ( !fSawFirstLinkerMember )
            {
                DumpFirstLinkerMember( pChild, (PVOID)(pArchHeader + 1) );
                pChild->printf("\n");
                fSawFirstLinkerMember = TRUE;
            }
            else if ( !fSawSecondLinkerMember )
            {
                DumpSecondLinkerMember( pChild, (PVOID)(pArchHeader + 1) );
                pChild->printf("\n");
                fSawSecondLinkerMember = TRUE;
            }
        }
        else if( !strncmp(	(char *)pArchHeader->Name,
        					IMAGE_ARCHIVE_LONGNAMES_MEMBER, 16) )
        {
            DumpLongnamesMember( pChild, (PVOID)(pArchHeader + 1),
                                 atoi((char *)pArchHeader->Size) );
            pChild->printf("\n");
        }
        else    // It's an OBJ file
        {
            DumpObjFile( pChild, (PIMAGE_FILE_HEADER)(pArchHeader + 1) );
        }

        // Calculate how big this member is (it's originally stored as 
        // as ASCII string.
        thisMemberSize = atoi((char *)pArchHeader->Size)
                        + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;

        thisMemberSize = (thisMemberSize+1) & ~1;   // Round up

        // Get a pointer to the next archive member
        pArchHeader = MakePtr(PIMAGE_ARCHIVE_MEMBER_HEADER, pArchHeader,
                                thisMemberSize);

        // Bail out if we don't see the EndHeader signature in the next record
        __try
        {
            if (strncmp( (char *)pArchHeader->EndHeader, IMAGE_ARCHIVE_END, 2))
                break;
        }
        __except( TRUE )    // Should only get here if pArchHeader is bogus
        {
            fBreak = TRUE;  // Ideally, we could just put a "break;" here,
        }                   // but BC++ doesn't like it.
        
        if ( fBreak )   // work around BC++ problem.
            break;
    }

	return true;
}

// Routine to convert from big endian to little endian
DWORD ConvertBigEndian(DWORD bigEndian)
{
	DWORD temp = 0;

	// pRoot->printf( "bigEndian: %08X\n", bigEndian );

	temp |= bigEndian >> 24;
	temp |= ((bigEndian & 0x00FF0000) >> 8);
	temp |= ((bigEndian & 0x0000FF00) << 8);
	temp |= ((bigEndian & 0x000000FF) << 24);

	return temp;
}

