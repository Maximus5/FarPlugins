//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: EXEDUMP.H
//==================================

bool DumpExeFile( MPanelItem* pRoot, PIMAGE_DOS_HEADER dosHeader );
template <class T> void DumpOptionalHeader(T* pImageOptionalHeader);	// 'T' is IMAGE_OPTIONAL_HEADER32/64
