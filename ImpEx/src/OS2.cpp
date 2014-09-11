
#include "stdafx.h"
#include <stdio.h>
#include "COMMON.H"

// This is 16bit PE browser

void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes);

// Get an ASCII string representing a resource type
void GetOS2ResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
	if ((type & 0x8000) == 0x8000) {
		GetResourceTypeName(type & 0x7FFF, buffer, cBytes-10);
		strcat(buffer, " [16bit]");
	} else {
		GetResourceTypeName(type, buffer, cBytes);
	}
}

typedef struct tag_OS2RC_TNAMEINFO {
	USHORT rnOffset;
	USHORT rnLength;
	UINT   rnID;
	USHORT rnHandle;
	USHORT rnUsage;
} OS2RC_TNAMEINFO, *POS2RC_TNAMEINFO;

typedef struct tag_OS2RC_TYPEINFO {
	USHORT rtTypeID;
	USHORT rtResourceCount;
	UINT   rtReserved;
	OS2RC_TNAMEINFO rtNameInfo[1];
} OS2RC_TYPEINFO, *POS2RC_TYPEINFO;

MPanelItem* CreateResource(MPanelItem *pRoot, 
						   DWORD rootType, LPVOID ptrRes, DWORD resSize,
						   LPCSTR asID, LPCSTR langID, DWORD stringIdBase, DWORD anLangId);


void DumpNEResourceTable(MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader, LPBYTE pResourceTable)
{
	PBYTE pImageBase = (PBYTE)dosHeader;

	MPanelItem* pChild = pRoot->AddFolder(_T("Resource Table"));
	pChild->AddText(_T("<Resource Table>\n"));

	// минимальный размер
	size_t nReqSize = sizeof(OS2RC_TYPEINFO)+12;
	if (!ValidateMemory(pResourceTable, nReqSize)) {
		pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
			(DWORD)(pResourceTable - pImageBase));
		return;
	}

	//
	USHORT rscAlignShift = *(USHORT*)pResourceTable;
	OS2RC_TYPEINFO* pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	char szTypeName[128], szResName[256];
	UINT nResLength = 0, nResOffset = 0;
	LPBYTE pNames;

	// —начала нужно найти начало имен
	pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	while (pTypeInfo->rtTypeID) {
		OS2RC_TNAMEINFO* pResName = pTypeInfo->rtNameInfo;

		// Next resource type
		pTypeInfo = (OS2RC_TYPEINFO*)(pResName+pTypeInfo->rtResourceCount);
		if (!ValidateMemory(pTypeInfo, 2)) {
			pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
				(DWORD)(((LPBYTE)pTypeInfo) - pImageBase));
			return;
		}
	}
	pNames = ((LPBYTE)pTypeInfo)+2;

	// “еперь, собственно ресурсы
	pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	while (pTypeInfo->rtTypeID) {
		szTypeName[0] = 0;
		GetOS2ResourceTypeName(pTypeInfo->rtTypeID, szTypeName, sizeof(szTypeName));

		MPanelItem* pType = pChild->AddFolder(szTypeName);
		pType->printf("  <%s>:\n", szTypeName);

		pType->printf(_T("    Resource count:   %i\n"), pTypeInfo->rtResourceCount);

		OS2RC_TNAMEINFO* pResName = pTypeInfo->rtNameInfo;
		for (USHORT i = pTypeInfo->rtResourceCount; i--; pResName++) {
			nResLength = pResName->rnLength * (1 << rscAlignShift);
			nResOffset = pResName->rnOffset * (1 << rscAlignShift);

			szResName[0] = 0;
			if (pNames) {
				if (!ValidateMemory(pNames, 1)) {
					pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
						(DWORD)(pNames - pImageBase));
					pNames = NULL;
				} else if (!ValidateMemory(pNames, 1+(*pNames))) {
					pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
						(DWORD)(pNames - pImageBase));
					pNames = NULL;
				} else if (*pNames) {
					memmove(szResName, pNames+1, *pNames);
					szResName[*pNames] = 0;
					pNames += (*pNames)+1;
				} else {
					pNames++;
				}
			}
			if (szResName[0]) {
				sprintf(szResName+strlen(szResName), ".0x%08X", pResName->rnID);
			} else {
				sprintf(szResName, "ResID=0x%08X", pResName->rnID);
			}

			MPanelItem* pRes = NULL;
			if (nResLength && nResOffset) {
				pRes = CreateResource(pType, pTypeInfo->rtTypeID, 
					pImageBase+nResOffset, nResLength,
					szResName, NULL, 0, 0);
			} else {
				pRes = pType->AddFile(szResName, nResOffset ? nResLength : 0);
			}
			pType->printf("    <%s>\n", szResName);
			pType->printf("      Resource Name:    %s\n", szResName);
			pType->printf("      Resource ID:      0x%08X\n", pResName->rnID);
			pType->printf("      Resource length:  %u bytes\n", nResLength);
			pType->printf("      Resource offset:  0x%08X\n", nResOffset);
			pType->printf("      Handle(reserved): 0x%04X\n", (DWORD)pResName->rnHandle);
			pType->printf("      Usage(reserved):  0x%04X\n", (DWORD)pResName->rnUsage);
			//if (nResLength && nResOffset) {
			//	pRes->SetData(pImageBase+nResOffset, nResLength);
			//}
		}


		// Next resource type
		pTypeInfo = (OS2RC_TYPEINFO*)pResName;
	}
}

bool DumpExeFileNE( MPanelItem *pRoot, PIMAGE_DOS_HEADER dosHeader, PIMAGE_OS2_HEADER pOS2Header )
{
	PBYTE pImageBase = (PBYTE)dosHeader;

	pRoot->Root()->AddFlags(_T("16BIT"));


	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));
	DumpHeader(pRoot, dosHeader);

	
	//MPanelItem* pChild = pRoot->AddFolder(_T("OS2 Header"));
	MPanelItem* pChild = pRoot->AddFile(_T("OS2_Header.txt"));
	pChild->AddText(_T("<OS2 Header>\n"));

	MPanelItem* pOS2 = pRoot->AddFile(_T("OS2_Header"), sizeof(*pOS2Header));
	pOS2->SetData((const BYTE*)pOS2Header, sizeof(*pOS2Header));


	if (pOS2Header->ne_magic != IMAGE_OS2_SIGNATURE) {
		pChild->AddText(_T("  IMAGE_OS2_SIGNATURE_LE signature not supported\n"));
		return true;
	}

	pChild->AddText(_T("  Signature:                          IMAGE_OS2_SIGNATURE\n"));  

	pChild->printf(_T("  Version number:                     %u\n"), (UINT)pOS2Header->ne_ver);
	pChild->printf(_T("  Revision number:                    %u\n"), (UINT)pOS2Header->ne_rev);
	pChild->printf(_T("  Offset of Entry Table:              %u\n"), (UINT)pOS2Header->ne_enttab);
	pChild->printf(_T("  Number of bytes in Entry Table:     %u\n"), (UINT)pOS2Header->ne_cbenttab);
	pChild->printf(_T("  Checksum of whole file:             0x%08X\n"), (UINT)pOS2Header->ne_crc);
	pChild->printf(_T("  Flag word:                          0x%04X\n"), (UINT)pOS2Header->ne_flags);
	pChild->printf(_T("  Automatic data segment number:      %u\n"), (UINT)pOS2Header->ne_autodata);
	pChild->printf(_T("  Initial heap allocation:            %u\n"), (UINT)pOS2Header->ne_heap);
	pChild->printf(_T("  Initial stack allocation:           %u\n"), (UINT)pOS2Header->ne_stack);
	pChild->printf(_T("  Initial CS:IP setting:              0x%08X\n"), (UINT)pOS2Header->ne_csip);
	pChild->printf(_T("  Initial SS:SP setting:              0x%08X\n"), (UINT)pOS2Header->ne_sssp);
	pChild->printf(_T("  Count of file segments:             %u\n"), (UINT)pOS2Header->ne_cseg);
	pChild->printf(_T("  Entries in Module Reference Table:  %u\n"), (UINT)pOS2Header->ne_cmod);
	pChild->printf(_T("  Size of non-resident name table:    %u\n"), (UINT)pOS2Header->ne_cbnrestab);
	pChild->printf(_T("  Offset of Segment Table:            %u\n"), (UINT)pOS2Header->ne_segtab);
	pChild->printf(_T("  Offset of Resource Table:           %u\n"), (UINT)pOS2Header->ne_rsrctab);
	pChild->printf(_T("  Offset of resident name table:      %u\n"), (UINT)pOS2Header->ne_restab);
	pChild->printf(_T("  Offset of Module Reference Table:   %u\n"), (UINT)pOS2Header->ne_modtab);
	pChild->printf(_T("  Offset of Imported Names Table:     %u\n"), (UINT)pOS2Header->ne_imptab);
	pChild->printf(_T("  Offset of Non-resident Names Table: %u\n"), (UINT)pOS2Header->ne_nrestab);
	pChild->printf(_T("  Count of movable entries:           %u\n"), (UINT)pOS2Header->ne_cmovent);
	pChild->printf(_T("  Segment alignment shift count:      %u\n"), (UINT)pOS2Header->ne_align);
	pChild->printf(_T("  Count of resource segments:         %u\n"), (UINT)pOS2Header->ne_cres);
	pChild->printf(_T("  Target Operating system:            %u\n"), (UINT)pOS2Header->ne_exetyp);
	pChild->printf(_T("  Other .EXE flags:                   0x%02X\n"), (UINT)pOS2Header->ne_flagsothers);
	pChild->printf(_T("  offset to return thunks:            %u\n"), (UINT)pOS2Header->ne_pretthunks);
	pChild->printf(_T("  offset to segment ref. bytes:       %u\n"), (UINT)pOS2Header->ne_psegrefbytes);
	pChild->printf(_T("  Minimum code swap area size:        %u\n"), (UINT)pOS2Header->ne_swaparea);
	pChild->printf(_T("  Expected Windows version number:    %u.%u\n"), (UINT)HIBYTE(pOS2Header->ne_expver), (UINT)LOBYTE(pOS2Header->ne_expver));

	pChild->AddText(_T("\n"));

	if (pOS2Header->ne_rsrctab) {
		LPBYTE pResourceTable = (((LPBYTE)pOS2Header)+pOS2Header->ne_rsrctab);
		DumpNEResourceTable(pRoot, dosHeader, pResourceTable);
		//MPanelItem* pChild = pRoot->AddFolder(_T("Resource Table"));
		//pChild->AddText(_T("<Resource Table>\n"));
	}

	return true;
}
