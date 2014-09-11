//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: EXTRNVAR.H
//==================================

// EXTRNVAR.H holds the "extern" definitions for global variables used
// through the program.

extern BOOL fShowRelocations;
extern BOOL fShowRawSectionData;
extern BOOL fShowSymbolTable;
extern BOOL fShowLineNumbers;
extern BOOL fShowIATentries;
extern BOOL fShowPDATA;
extern BOOL fShowResources;

extern PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader;
extern PIMAGE_DEBUG_MISC g_pMiscDebugInfo;
extern PDWORD g_pCVHeader;

class COFFSymbolTable;
extern COFFSymbolTable * g_pCOFFSymbolTable;

extern PBYTE g_pMappedFileBase;
extern ULARGE_INTEGER g_FileSize;
extern PIMAGE_NT_HEADERS32 gpNTHeader32;
extern PIMAGE_NT_HEADERS64 gpNTHeader64;
extern bool g_bIs64Bit;
