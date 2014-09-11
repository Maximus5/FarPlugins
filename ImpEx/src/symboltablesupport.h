class COFFSymbolTable;

BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length);
void DumpCOFFSymbolTable( MPanelItem *pRoot, COFFSymbolTable * pSymTab );
void DumpMiscDebugInfo( MPanelItem *pRoot, PIMAGE_DEBUG_MISC PMiscDebugInfo );
void DumpCVDebugInfoRecord( MPanelItem *pRoot, PDWORD pCVHeader );
void DumpLineNumbers(MPanelItem *pRoot, PIMAGE_LINENUMBER pln, DWORD count);

