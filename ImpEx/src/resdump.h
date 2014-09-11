//extern PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pStrResEntries;
extern PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pDlgResEntries;
//extern DWORD g_cStrResEntries;
extern DWORD g_cDlgResEntries;
extern bool  g_bUPXed;


void DumpResources( MPanelItem *pRoot, PBYTE pImageBase, PIMAGE_NT_HEADERS32 pNTHeader );

//============================================================================
DWORD GetOffsetToDataFromResEntry( 	PBYTE pResourceBase,
									PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry );

//============================================================================
//
// Dump the information about one resource directory.
//
void DumpResourceDirectory(	MPanelItem *pRoot, PIMAGE_RESOURCE_DIRECTORY pResDir,
							PBYTE pResourceBase,
							DWORD level,
							DWORD resourceType, DWORD rootType = 0, DWORD parentType = 0 );

