
#include "header.h"
#include "RE_Guids.h"

extern HANDLE ghHeap;

#ifdef USE_HREGKEY
HREGKEY NULLHKEY;
#endif

LPCWSTR HKeyToStringKey(HKEY hkey)
{
	static LPCWSTR pszRootKeys[10]; // HKEY_CLASSES_ROOT..HKEY_CURRENT_USER_LOCAL_SETTINGS
	static bool bRootKeysFilled;
	if (!bRootKeysFilled)
	{
		pszRootKeys[0] = L"HKEY_CLASSES_ROOT";
		pszRootKeys[1] = L"HKEY_CURRENT_USER";
		pszRootKeys[2] = L"HKEY_LOCAL_MACHINE";
		pszRootKeys[3] = L"HKEY_USERS";
		pszRootKeys[4] = L"HKEY_PERFORMANCE_DATA";
		pszRootKeys[5] = L"HKEY_CURRENT_CONFIG";
		pszRootKeys[6] = L"HKEY_DYN_DATA";
		pszRootKeys[7] = L"HKEY_CURRENT_USER_LOCAL_SETTINGS";
		bRootKeysFilled = true;
	}
	_ASSERTE(pszRootKeys[0]!=0);
	
	if (hkey == HKEY__HIVE)
		return HKEY__HIVE_NAME;
		
	if ((((ULONG_PTR)hkey) >= HKEY__FIRST) && (((ULONG_PTR)hkey) <= HKEY__LAST))
		return pszRootKeys[((ULONG_PTR)hkey) - HKEY__FIRST];
	
	_ASSERTE(hkey == HKEY__HIVE);
	return NULL; // Invalid, unknown
}

bool HKeyToStringKey(HKEY hkey, wchar_t* pszKey, int nMaxLen)
{
	LPCWSTR psz = HKeyToStringKey(hkey);
	if (!psz)
		return false;
	lstrcpynW(pszKey, psz, nMaxLen);
	return true;
}
	
bool StringKeyToHKey(LPCWSTR asKey, HKEY* phkey)
{
	static struct {LPCWSTR pszKey; LPCWSTR pszAlias; HKEY hk;} pRootKeys[10]; // HKEY__HIVE_NAME_LWR, HKEY_CLASSES_ROOT..HKEY_CURRENT_USER_LOCAL_SETTINGS
	static int nRootKeysFilled;
	if (!nRootKeysFilled)
	{
		int k = 0;
		#define ADDKEY(key,alias,hkey) \
			pRootKeys[k].pszKey = key; pRootKeys[k].pszAlias = alias; pRootKeys[k++].hk = hkey;
		
		ADDKEY(HKEY__HIVE_NAME_LWR, NULL, HKEY__HIVE);
		ADDKEY(L"hkey_current_user",     L"hkcu", HKEY_CURRENT_USER);
		ADDKEY(L"hkey_classes_root",     L"hkcr", HKEY_CLASSES_ROOT);
		ADDKEY(L"hkey_local_machine",    L"hklm", HKEY_LOCAL_MACHINE);
		ADDKEY(L"hkey_users",            L"hku",  HKEY_USERS);
		ADDKEY(L"hkey_performance_data", NULL,    HKEY_PERFORMANCE_DATA);
		ADDKEY(L"hkey_current_config",   NULL,    HKEY_CURRENT_CONFIG);
		ADDKEY(L"hkey_dyn_data",         NULL,    HKEY_DYN_DATA);
		ADDKEY(L"hkey_current_user_local_settings", NULL, HKEY_CURRENT_USER_LOCAL_SETTINGS);
		
		nRootKeysFilled = k;
	}
	_ASSERTE(pRootKeys[0].pszKey[0]!=0);

	#define CHK(i,c) ( (asKey[i] == (WORD)c) || (asKey[i]|0x20) == (WORD)c )
	if (!CHK(0,'h') || !CHK(1,'k'))
		return false;

	for (int k = 0; k < nRootKeysFilled; k++) {
		bool bMatch = true;
		int c = 2;
		while (asKey[c]) {
			if (!CHK(c, pRootKeys[k].pszKey[c])) {
				bMatch = false; break;
			}
			c++;
		}
		if (bMatch && pRootKeys[k].pszKey[c] == 0) {
			if (phkey) *phkey = pRootKeys[k].hk;
			return true;
		}
		// Alias
		if (pRootKeys[k].pszAlias)
		{
			bMatch = true;
			c = 2;
			while (asKey[c]) {
				if (!CHK(c, pRootKeys[k].pszAlias[c])) {
					bMatch = false; break;
				}
				c++;
			}
			if (bMatch && pRootKeys[k].pszAlias[c] == 0) {
				if (phkey) *phkey = pRootKeys[k].hk;
				return true;
			}
		}
	}
	
	return false;

	//if (!lstrcmpiW(asKey, L"HKCR") || !lstrcmpiW(asKey, L"HKEY_CLASSES_ROOT")) {
	//	if (phkey) *phkey = HKEY_CLASSES_ROOT;
	//} else
	//if (!lstrcmpiW(asKey, L"HKCU") || !lstrcmpiW(asKey, L"HKEY_CURRENT_USER")) {
	//	if (phkey) *phkey = HKEY_CURRENT_USER;
	//} else
	//if (!lstrcmpiW(asKey, L"HKLM") || !lstrcmpiW(asKey, L"HKEY_LOCAL_MACHINE")) {
	//	if (phkey) *phkey = HKEY_LOCAL_MACHINE;
	//} else
	//if (!lstrcmpiW(asKey, L"HKU") || !lstrcmpiW(asKey, L"HKEY_USERS")) {
	//	if (phkey) *phkey = HKEY_USERS;
	//} else
	//if (!lstrcmpiW(asKey, L"HKCC") || !lstrcmpiW(asKey, L"HKEY_CURRENT_CONFIG")) {
	//	if (phkey) *phkey = HKEY_CURRENT_CONFIG;
	//} else
	//if (!lstrcmpiW(asKey, L"HKEY_PERFORMANCE_DATA")) {
	//	if (phkey) *phkey = HKEY_PERFORMANCE_DATA;
	//} else
	//if (!lstrcmpiW(asKey, HKEY__HIVE_NAME)) {
	//	if (phkey) *phkey = HKEY__HIVE;
	//} else {
	//	return false;
	//}
	//return true;
}


void * __cdecl xf_malloc
(
		size_t _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
		, LPCSTR lpszFileName, int nLine
#endif
)
{
	_ASSERTE(ghHeap);
	_ASSERTE(_Size>0);
#ifdef TRACK_MEMORY_ALLOCATIONS
	xf_mem_block* p = (xf_mem_block*)HeapAlloc(ghHeap, 0, _Size+sizeof(xf_mem_block)+8);
	p->bBlockUsed = TRUE;
	p->nBlockSize = _Size;
	wsprintfA(p->sCreatedFrom, "%s:%i", PointToName(lpszFileName), nLine);
	#ifdef _DEBUG
	if (_Size > 0) memset(p+1, 0xFD, _Size);
	#endif
	memset(((LPBYTE)(p+1))+_Size, 0xCC, 8);
	return p?(p+1):p;
#else
	void* p = HeapAlloc(ghHeap, 0, _Size);
	return p;
#endif
}
void * __cdecl xf_calloc
(
		size_t _Count, size_t _Size
#ifdef TRACK_MEMORY_ALLOCATIONS
		, LPCSTR lpszFileName, int nLine
#endif
)
{
	_ASSERTE(ghHeap);
	_ASSERTE((_Count*_Size)>0);
#ifdef TRACK_MEMORY_ALLOCATIONS
	xf_mem_block* p = (xf_mem_block*)HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, _Count*_Size+sizeof(xf_mem_block)+8);
	p->bBlockUsed = TRUE;
	p->nBlockSize = _Count*_Size;
	wsprintfA(p->sCreatedFrom, "%s:%i", PointToName(lpszFileName), nLine);
	memset(((LPBYTE)(p+1))+_Count*_Size, 0xCC, 8);
	return p?(p+1):p;
#else
	void* p = HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, _Count*_Size);
	return p;
#endif
}
void __cdecl xf_free
(
		void * _Memory
#ifdef TRACK_MEMORY_ALLOCATIONS
		, LPCSTR lpszFileName, int nLine
#endif
)
{
	_ASSERTE(ghHeap && _Memory);

#ifdef TRACK_MEMORY_ALLOCATIONS
	xf_mem_block* p = ((xf_mem_block*)_Memory)-1;
	if (p->bBlockUsed == TRUE)
	{
		int nCCcmp = memcmp(((LPBYTE)_Memory)+p->nBlockSize, "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", 8);
		_ASSERTE(nCCcmp == 0);
		memset(_Memory, 0xFD, p->nBlockSize);
	} else {
		_ASSERTE(p->bBlockUsed == TRUE);
	}
	p->bBlockUsed = FALSE;
	wsprintfA(p->sCreatedFrom, "-- %s:%i", PointToName(lpszFileName), nLine);
	_Memory = (void*)p;
#endif

	#ifdef _DEBUG
	__int64 _Size1 = HeapSize(ghHeap, 0, _Memory);
	_ASSERTE(_Size1 > 0);
	#endif
	
	HeapFree(ghHeap, 0, _Memory);
	
	//#ifdef _DEBUG
	//SIZE_T _Size2 = HeapSize(ghHeap, 0, _Memory);
	//if (_Size1 == _Size2) {
	//	_ASSERTE(_Size1 != _Size2);
	//}
	//#endif
}
#ifdef TRACK_MEMORY_ALLOCATIONS
void __cdecl xf_dump()
{
	PROCESS_HEAP_ENTRY ent = {NULL};
	HeapLock(ghHeap);
	HeapCompact(ghHeap,0);
	char sBlockInfo[255];
	while (HeapWalk(ghHeap, &ent))
	{
		if (ent.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
			xf_mem_block* p = (xf_mem_block*)ent.lpData;
			if (p->bBlockUsed==TRUE && p->nBlockSize==ent.cbData)
			{
#ifndef _WIN64
				wsprintfA(sBlockInfo, "!!! Lost memory block at 0x%08X, size %u\n    Allocated from: %s\n", ent.lpData, ent.cbData,
					p->sCreatedFrom);
#else
				wsprintfA(sBlockInfo, "!!! Lost memory block at 0x%I64X, size %u\n    Allocated from: %s\n", ent.lpData, ent.cbData,
					p->sCreatedFrom);
#endif
			} else {
#ifndef _WIN64
				wsprintfA(sBlockInfo, "!!! Lost memory block at 0x%08X, size %u\n    Allocated from: %s\n", ent.lpData, ent.cbData,
					"<Header information broken!>");
#else
				wsprintfA(sBlockInfo, "!!! Lost memory block at 0x%I64X, size %u\n    Allocated from: %s\n", ent.lpData, ent.cbData,
					"<Header information broken!>");
#endif
			}
			OutputDebugStringA(sBlockInfo);
		}
	}
	HeapUnlock(ghHeap);
}
#endif
bool __cdecl xf_validate(void * _Memory /*= NULL*/)
{
	_ASSERTE(ghHeap);
#ifdef TRACK_MEMORY_ALLOCATIONS
	if (_Memory)
	{
		xf_mem_block* p = ((xf_mem_block*)_Memory)-1;
		_ASSERTE(p->bBlockUsed == TRUE);
		_Memory = (void*)p;
	}
#endif
	BOOL b = HeapValidate(ghHeap, 0, _Memory);
	return (b!=FALSE);
}
void * __cdecl operator new(size_t _Size)
{
	void * p = xf_malloc(
		_Size
		#ifdef TRACK_MEMORY_ALLOCATIONS
		,__FILE__,__LINE__
		#endif
		);
	#ifdef MVALIDATE_POINTERS
		_ASSERTE(p != NULL);
		if (p == NULL) InvalidOp();
	#endif
	return p;
}
void * __cdecl operator new[] (size_t _Size)
{
	void * p = xf_malloc(
		_Size
		#ifdef TRACK_MEMORY_ALLOCATIONS
		,__FILE__,__LINE__
		#endif
		);
	#ifdef MVALIDATE_POINTERS
		_ASSERTE(p != NULL);
		if (p == NULL) InvalidOp();
	#endif
	return p;
}
void __cdecl operator delete(void *p)
{
	return xf_free(
		p
		#ifdef TRACK_MEMORY_ALLOCATIONS
		,__FILE__,__LINE__
		#endif
		);
}
void __cdecl operator delete[] (void *p)
{
	return xf_free(
		p
		#ifdef TRACK_MEMORY_ALLOCATIONS
		,__FILE__,__LINE__
		#endif
		);
}


//LONG GetString(LPCWSTR asKey, LPCWSTR asName, wchar_t* pszValue, DWORD cCount)
//{
//	HREGKEY hk; LONG nRegRc; DWORD nSize;
//	
//	pszValue[0] = 0;
//
//	nRegRc = RegOpenKeyExW(HKEY_CLASSES_ROOT, asKey, 0, KEY_READ, &hk);
//	if (!nRegRc) {
//		nSize = (cCount-1)*sizeof(*pszValue);
//		nRegRc = RegQueryValueExW(hk, NULL, NULL, NULL, (LPBYTE)pszValue, &nSize);
//		if (nRegRc) {
//			*pszValue = 0;
//		} else {
//			pszValue[nSize/sizeof(pszValue[0])] = 0;
//		}
//		RegCloseKey(hk);
//	}
//	
//	return nRegRc;
//}

wchar_t* lstrdup(LPCWSTR asText)
{
	int nLen = asText ? lstrlenW(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1)*2);
	if (nLen)
		lstrcpyW(psz, asText);
	else
		psz[0] = 0;
	return psz;
}

wchar_t* lstrdup_w(LPCSTR asText)
{
	int nLen = asText ? lstrlenA(asText) : 0;
	wchar_t* psz = (wchar_t*)malloc((nLen+1)*sizeof(wchar_t));
	if (nLen)
		lstrcpy_t(psz, nLen+1, asText);
	else
		psz[0] = 0;
	return psz;
}

#ifndef _UNICODE
char* lstrdup(LPCSTR asText)
{
	int nLen = asText ? lstrlenA(asText) : 0;
	char* psz = (char*)malloc(nLen+1);
	if (nLen)
		lstrcpyA(psz, asText);
	else
		psz[0] = 0;
	return psz;
}
#endif

TCHAR* lstrdup_t(LPCWSTR asText)
{
	int nLen = asText ? lstrlenW(asText) : 0;
	TCHAR* psz = (TCHAR*)malloc((nLen+1)*sizeof(TCHAR));
	if (nLen)
		lstrcpy_t(psz, nLen+1, asText);
	else
		psz[0] = 0;
	return psz;
}

void lstrcpy_t(TCHAR* pszDst, int cMaxSize, const wchar_t* pszSrc)
{
	if (cMaxSize<1) return;
#ifdef _UNICODE
	lstrcpyn(pszDst, pszSrc, cMaxSize);
#else
	WideCharToMultiByte(CP_OEMCP, 0, pszSrc, -1, pszDst, cMaxSize, 0,0);
#endif
}

void lstrcpy_t(wchar_t* pszDst, int cMaxSize, const char* pszSrc)
{
	MultiByteToWideChar(CP_OEMCP, 0, pszSrc, -1, pszDst, cMaxSize);
}

#if 0
LPCTSTR msprintf(LPTSTR lpOut, size_t cchOutMax, LPCTSTR lpFmt, ...)
{
	va_list argptr;
	va_start(argptr, lpFmt);
	
	const TCHAR* pszSrc = lpFmt;
	TCHAR* pszDst = lpOut;
	TCHAR  szValue[16];
	TCHAR* pszValue;

	while (*pszSrc)
	{
		if (*pszSrc == _T('%'))
		{
			pszSrc++;
			switch (*pszSrc)
			{
			case _T('%'):
				{
					*(pszDst++) = _T('%');
					pszSrc++;
					continue;
				}
			case _T('c'):
				{
					// GCC: 'wchar_t' is promoted to 'int' when passed through '...'
					TCHAR c = va_arg( argptr, int );
					*(pszDst++) = c;
					pszSrc++;
					continue;
				}
			case _T('s'):
				{
					pszValue = va_arg( argptr, TCHAR* );
					if (pszValue)
					{
						while (*pszValue)
						{
							*(pszDst++) = *(pszValue++);
						}
					}
					pszSrc++;
					continue;
				}
			case _T('S'):
				{
					#ifdef _UNICODE
					char* pszValueA = va_arg( argptr, char* );
					#else
					wchar_t* pszValueA = va_arg( argptr, char* );
					#endif
					if (pszValueA)
					{
						// по хорошему, тут бы MultiByteToWideChar звать, но
						// эта ветка должна по идее только для отладки использоваться
						while (*pszValueA)
						{
							*(pszDst++) = (TCHAR)*(pszValueA++);
						}
					}
					pszSrc++;
					continue;
				}
			case _T('u'):
			case _T('i'):
				{
					UINT nValue = 0;
					if (*pszSrc == _T('i'))
					{
						int n = va_arg( argptr, int );
						if (n < 0)
						{
							*(pszDst++) = L'-';
							nValue = -n;
						}
						else
						{
							nValue = n;
						}
					}
					else
					{
						nValue = va_arg( argptr, UINT );
					}
					pszSrc++;
					pszValue = szValue;
					while (nValue)
					{
						WORD n = (WORD)(nValue % 10);
						*(pszValue++) = (wchar_t)(L'0' + n);
						nValue = (nValue - n) / 10;
					}
					if (pszValue == szValue)
						*(pszValue++) = L'0';
					// Теперь перекинуть в szGuiPipeName
					while (pszValue > szValue)
					{
						*(pszDst++) = *(--pszValue);
					}
					continue;
				}
			case L'0':
			case L'X':
			case L'x':
				{
					int nLen = 0;
					DWORD nValue;
					wchar_t cBase = L'A';
					if (pszSrc[0] == L'0' && pszSrc[1] == L'8' && (pszSrc[2] == L'X' || pszSrc[2] == L'x'))
					{
						if (pszSrc[2] == L'x')
							cBase = L'a';
						memmove(szValue, L"00000000", 16);
						nLen = 8;
						pszSrc += 3;
					}
					else if (pszSrc[0] == L'X' || pszSrc[0] == L'x')
					{
						if (pszSrc[0] == L'x')
							cBase = L'a';
						pszSrc++;
					}
					else
					{
						_ASSERTE(*pszSrc == L'u' || *pszSrc == L's' || *pszSrc == L'c' || *pszSrc == L'i' || *pszSrc == L'X');
						goto wrap;
					}
					
					
					nValue = va_arg( argptr, UINT );
					pszValue = szValue;
					while (nValue)
					{
						WORD n = (WORD)(nValue & 0xF);
						if (n <= 9)
							*(pszValue++) = (wchar_t)(L'0' + n);
						else
							*(pszValue++) = (wchar_t)(cBase + n - 10);
						nValue = nValue >> 4;
					}
					if (!nLen)
					{
						nLen = (int)(pszValue - szValue);
						if (!nLen)
						{
							*pszValue = L'0';
							nLen = 1;
						}
					}
					else
					{
						pszValue = (szValue+nLen);
					}
					// Теперь перекинуть в szGuiPipeName
					while (pszValue > szValue)
					{
						*(pszDst++) = *(--pszValue);
					}
					continue;
				}
			default:
				_ASSERTE(*pszSrc == L'u' || *pszSrc == L's' || *pszSrc == L'c' || *pszSrc == L'i' || *pszSrc == L'X');
			}
		}
		else
		{
			*(pszDst++) = *(pszSrc++);
		}
	}
wrap:
	*pszDst = 0;
	_ASSERTE(lstrlen(lpOut) < (int)cchOutMax);
	return lpOut;
}
#endif

void CopyFileName(wchar_t* pszDst, int cMaxSize, const wchar_t* pszSrc)
{
	if (!pszDst) { InvalidOp(); return; }
	if (!cMaxSize || cMaxSize > 32767) { InvalidOp(); return; }
	if (!pszSrc) { InvalidOp(); return; }
	int nCount = cMaxSize;

	#define CPYCHR(s) *(pszDst)=s[0]; if ((nCount--) > 1) { *(++pszDst)=s[1]; if ((nCount--) > 1) { *(++pszDst)=s[2]; } }

	LPCWSTR pszBreakChars = L"\\/:<>|\"?*\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f";
#ifdef _DEBUG
	const wchar_t* pszDst0 = pszDst;
	const wchar_t* pszSrc0 = pszSrc;
#endif
	LPCWSTR csSharps[] = {
		L"#00", L"#01", L"#02", L"#03", L"#04", L"#05", L"#06", L"#07",
		L"#08", L"#09", L"#0A", L"#0B", L"#0C", L"#0D", L"#0E", L"#0F",
		L"#10", L"#11", L"#12", L"#13", L"#14", L"#15", L"#16", L"#17",
		L"#18", L"#19", L"#1A", L"#1B", L"#1C", L"#1D", L"#1E", L"#1F"};
	while ((nCount--) > 1)
	{
		const wchar_t *pszBreak = wcspbrk(pszSrc, pszBreakChars);
		if (!pszBreak)
		{
			// До конца имени не валидных символов нет
			if (nCount<1) {
				_ASSERTE(nCount>=0);
				*pszDst = 0;
			} else {
				lstrcpynW(pszDst, pszSrc, (nCount+1)*2);
			}
			return;
		}
		if (pszBreak != pszSrc)
		{
			int nLen = pszBreak-pszSrc;
			if (nLen > nCount)
				nLen = nCount;
			memmove(pszDst, pszSrc, 2*nLen);
			pszDst += nLen;
			nCount -= nLen;
			pszSrc = pszBreak;
		}

		wchar_t wc = *pszBreak;
		switch (wc)
		{
		case L'\\':
			CPYCHR(L"#5C");
			break;
		case L'/':
			CPYCHR(L"#2F");
			break;
		case L':':
			CPYCHR(L"#3A");
			break;
		case L'<':
			CPYCHR(L"#3C");
			break;
		case L'>':
			CPYCHR(L"#3E");
			break;
		case L'|':
			CPYCHR(L"#7C");
			break;
		case L'"':
			CPYCHR(L"#22");
			break;
		case L'?':
			CPYCHR(L"#3F");
			break;
		case L'*':
			CPYCHR(L"#2A");
			break;
		//case L'\r':
		//	CPYCHR(L"#0D");
		//	break;
		//case L'\n':
		//	CPYCHR(L"#0A");
		//	break;
		//case L'\t':
		//	CPYCHR(L"#09");
		//	break;
		default:
			if (wc < countof(csSharps))
			{
				CPYCHR(csSharps[wc]);
			} else {
				_ASSERTE(*pszBreak == L'\\');
			}
		}
		pszDst++; pszSrc++;
	}

	*pszDst = 0;

	//while ((nCount--) > 1)
	//{
	//	wchar_t ch = *pszSrc;
	//	switch (ch)
	//	{
	//	case L'\\': case L'/': case ':':
	//		*(pszDst++) = L'$';
	//		break;
	//	case '<': case '>': case '|': case '"': case '?': case '*': case '\r': case '\n': case '\t':
	//		*(pszDst++) = L'_';
	//		break;
	//	default:
	//		*(pszDst++) = ch;
	//		break;
	//	}
	//	if (!ch)
	//		break;
	//	pszSrc++;
	//}
	//*pszDst = 0;
}

#ifndef _UNICODE
void CopyFileName(char* pszDst, int cMaxSize, const wchar_t* pszSrc)
{
	wchar_t* pwsz = (wchar_t*)calloc((cMaxSize+1),2);
	CopyFileName(pwsz, cMaxSize, pszSrc);
	lstrcpy_t(pszDst, cMaxSize, pwsz);
	SafeFree(pwsz);
}
#endif

bool IsUNCPath(const char* pszSrc)
{
	return (pszSrc[0] == '\\' && pszSrc[1] == '\\' && pszSrc[2] == '?' && pszSrc[3] == '\\');
}

bool IsUNCPath(const wchar_t* pszSrc)
{
	return (pszSrc[0] == L'\\' && pszSrc[1] == L'\\' && pszSrc[2] == L'?' && pszSrc[3] == L'\\');
}

bool IsNetworkUNCPath(const char* pszSrc)
{
	if (IsUNCPath(pszSrc))
		return (pszSrc[4] == 'U' && pszSrc[5] == 'N' && pszSrc[6] == 'C' && pszSrc[7] == '\\');
	return false;
}

bool IsNetworkUNCPath(const wchar_t* pszSrc)
{
	if (IsUNCPath(pszSrc))
		return (pszSrc[4] == L'U' && pszSrc[5] == L'N' && pszSrc[6] == L'C' && pszSrc[7] == L'\\');
	return false;
}

bool IsNetworkPath(const char* pszSrc)
{
	return (pszSrc[0] == '\\' && pszSrc[1] == '\\' && pszSrc[2] > ' '
		&& pszSrc[2] != '?' && pszSrc[2] != '.' && strchr(pszSrc+3,'\\'));
}

bool IsNetworkPath(const wchar_t* pszSrc)
{
	return (pszSrc[0] == L'\\' && pszSrc[1] == L'\\' && pszSrc[2] > L' '
		&& pszSrc[2] != L'?' && pszSrc[2] != L'.' && wcschr(pszSrc+3,L'\\'));
}

wchar_t* MakeUNCPath(const char* pszSrc)
{
	if (IsUNCPath(pszSrc))
		return lstrdup_w(pszSrc);

	bool lbIsNetwork = IsNetworkPath(pszSrc);
	
	// "\\?\" X:\...
	// "\\?\UNC\" server\share\...
	int nLen = lstrlenA(pszSrc);
	wchar_t* pszDest = (wchar_t*)malloc((nLen+10)*2);
	lstrcpyW(pszDest, L"\\\\?\\");
	if (lbIsNetwork) {
		lstrcpyW(pszDest+4, L"UNC\\");
		lstrcpy_t(pszDest+8, nLen+1, pszSrc+2);
	} else {
		lstrcpy_t(pszDest+4, nLen+1, pszSrc);
	}

	MCHKHEAP;
	return pszDest;
}

wchar_t* MakeUNCPath(const wchar_t* pszSrc)
{
	if (IsUNCPath(pszSrc))
		return lstrdup(pszSrc);

	bool lbIsNetwork = IsNetworkPath(pszSrc);

	// "\\?\" X:\...
	// "\\?\UNC\" server\share\...
	int nLen = lstrlenW(pszSrc);
	wchar_t* pszDest = (wchar_t*)malloc((nLen+10)*2);
	lstrcpyW(pszDest, L"\\\\?\\");
	if (lbIsNetwork) {
		lstrcpyW(pszDest+4, L"UNC\\");
		lstrcpynW(pszDest+8, pszSrc+2, nLen+1);
	} else {
		lstrcpynW(pszDest+4, pszSrc, nLen+1);
	}

	MCHKHEAP;
	return pszDest;
}

int CopyUNCPath(wchar_t* pszDest, int cchDestMax, const wchar_t* pszSrc)
{
	*pszDest = 0;
	_ASSERTE(cchDestMax > 16);
	if (IsUNCPath(pszSrc))
	{
		lstrcpynW(pszDest, pszSrc, cchDestMax);
	} else {
		bool lbIsNetwork = IsNetworkPath(pszSrc);

		// "\\?\" X:\...
		// "\\?\UNC\" server\share\...
		lstrcpyW(pszDest, L"\\\\?\\");
		if (lbIsNetwork) {
			lstrcpyW(pszDest+4, L"UNC\\");
			lstrcpynW(pszDest+8, pszSrc+2, cchDestMax-8);
		} else {
			lstrcpynW(pszDest+4, pszSrc, cchDestMax-4);
		}
	}
	MCHKHEAP;
	int nLen = lstrlenW(pszDest);
	return nLen;
}

wchar_t* UnmakeUNCPath_w(const wchar_t* pszSrc)
{
	if (!IsUNCPath(pszSrc))
		return lstrdup(pszSrc);

	int nLen = lstrlenW(pszSrc);
	wchar_t* pszDest = (wchar_t*)malloc((nLen+1)*sizeof(wchar_t));
	// \\?\UNC\ ...
	if (pszSrc[4] == L'U' && pszSrc[5] == L'N' && pszSrc[6] == L'C' && pszSrc[7] == L'\\') {
		pszDest[0] = L'\\';
		lstrcpynW(pszDest+1, pszSrc+7, nLen-1);
	} else {
		lstrcpynW(pszDest, pszSrc+4, nLen);
	}
	MCHKHEAP;
	return pszDest;
}

#ifndef _UNICODE
char* UnmakeUNCPath_t(const wchar_t* pszSrc)
{
	if (!IsUNCPath(pszSrc))
		return lstrdup_t(pszSrc);

	int nLen = lstrlenW(pszSrc);
	char* pszDest = (char*)malloc((nLen+1)*sizeof(char));
	// \\?\UNC\ ...
	if (pszSrc[4] == L'U' && pszSrc[5] == L'N' && pszSrc[6] == L'C' && pszSrc[7] == L'\\') {
		pszDest[0] = L'\\';
		lstrcpy_t(pszDest+1, nLen-1, pszSrc+7);
	} else {
		lstrcpy_t(pszDest, nLen, pszSrc+4);
	}
	MCHKHEAP;
	return pszDest;
}
#endif


const char* PointToName(const char* asFileOrPath)
{
	if (!asFileOrPath) {
		InvalidOp();
		return NULL;
	}
	const char* pszSlash = strrchr(asFileOrPath, L'\\');
	if (pszSlash)
		return pszSlash+1;
	return asFileOrPath;
}

const wchar_t* PointToName(const wchar_t* asFileOrPath)
{
	if (!asFileOrPath) {
		InvalidOp();
		return NULL;
	}
	const wchar_t* pszSlash = wcsrchr(asFileOrPath, L'\\');
	if (pszSlash)
		return pszSlash+1;
	return asFileOrPath;
}

const char* PointToExt(const char* asFileOrPath)
{
	const char* pszFile = PointToName(asFileOrPath);
	if (!pszFile) {
		InvalidOp();
		return NULL;
	}
	const char* pszDot = strrchr(pszFile, '.');
	if (pszDot)
	{
		// расширение "." не отбрасывать - используется в REPlugin::ConfirmExport!
		//const char* pszSkip = pszDot;
		//while (*pszSkip == '.') pszSkip++;
		//if (*pszSkip)
		return pszDot;
	}
	return NULL;
}

const wchar_t* PointToExt(const wchar_t* asFileOrPath)
{
	const wchar_t* pszFile = PointToName(asFileOrPath);
	if (!pszFile) {
		InvalidOp();
		return NULL;
	}
	const wchar_t* pszDot = wcsrchr(pszFile, L'.');
	if (pszDot)
	{
		// расширение "." не отбрасывать - используется в REPlugin::ConfirmExport!
		//const wchar_t* pszSkip = pszDot;
		//while (*pszSkip == L'.') pszSkip++;
		//if (*pszSkip)
		return pszDot;
	}
	return NULL;
}

wchar_t* ExpandPath(LPCWSTR asPathStart)
{
	// преобразовать в полный путь
	wchar_t* pszDst = NULL;
	MCHKHEAP;
#ifdef _UNICODE
	int nLen = FSF.ConvertPath(CPM_FULL, asPathStart, NULL, 0);
	if (nLen > 0) {
		pszDst = (wchar_t*)calloc(nLen+1,2);
		MCHKHEAP;
		FSF.ConvertPath(CPM_FULL, asPathStart, pszDst, nLen);
		MCHKHEAP;
	} else {
		MCHKHEAP;
		pszDst = lstrdup(asPathStart);
		MCHKHEAP;
	}
#else
	pszDst = (wchar_t*)calloc((MAX_PATH*3)+1,2);
	MCHKHEAP;
	DWORD nFRC = GetFullPathNameW(asPathStart, MAX_PATH*3, pszDst, NULL);
	MCHKHEAP;
	if (!nFRC || nFRC >= (MAX_PATH*3)) {
		MCHKHEAP;
		SafeFree(pszDst);
		pszDst = lstrdup(asPathStart);
		MCHKHEAP;
	}
#endif
	MCHKHEAP;
	return pszDst;
};

wchar_t* ExpandPath(LPCSTR asPathStart)
{
	wchar_t* pszSrc = lstrdup_w(asPathStart);
	wchar_t* pszDst = NULL;
	pszDst = (wchar_t*)calloc((MAX_PATH*3)+1,2);
	MCHKHEAP;
	DWORD nFRC = GetFullPathNameW(pszSrc, MAX_PATH*3, pszDst, NULL);
	MCHKHEAP;
	if (!nFRC || nFRC >= (MAX_PATH*3)) {
		MCHKHEAP;
		SafeFree(pszDst);
		pszDst = pszSrc; pszSrc = NULL;
	}
	MCHKHEAP;
	SafeFree(pszSrc);
	return pszDst;
}

bool ValidatePath(LPCWSTR asFullOrRelPath)
{
	// "Плохие" указатели отбросим сразу
	if (!asFullOrRelPath || !*asFullOrRelPath)
		return false;

	// Сразу проверим наличие недопустимых символов!
	LPCWSTR pszBadChar = wcspbrk(asFullOrRelPath, L"/<>|\"?*\r\n\t");
	if (pszBadChar)
		return false;

	if (asFullOrRelPath[0] == L'\\')
	{
		if (asFullOrRelPath[1] == L'\\')
		{
			// Это или UNC или сетевой путь, считаем, что он "хороший"
			return true;
		}
		else
		{
			// Путь от корня диска. Не должен содержать ":" и др. некорректных символов
			if (wcschr(asFullOrRelPath, L':'))
				return false;
		}
	}
	else
	{
		LPCWSTR pszColon = wcschr(asFullOrRelPath, L':');
		if (!pszColon) // Путь не в абсолютном формате.
			return true; // считаем, что он "хороший"
		// Двоеточие может быть только после буквы диска!
		if (pszColon != (asFullOrRelPath+1))
			return false;
		// Больше - двоеточий быть не должно!
		if (wcschr(pszColon+1, L':'))
			return false;
	}
	// Проверки ничего плохого не выявили
	return true;
}

TCHAR *GetMsg(int MsgId)
{
	TCHAR* psz = (TCHAR*)psi.GetMsg(PluginNumber, MsgId);
	_ASSERTE(psz);
	return psz;
}

void InternalInvalidOp(LPCWSTR asFile, int nLine)
{
	const wchar_t* pszFile;
	if (asFile) {
		pszFile = PointToName(asFile);
	} else {
		pszFile = L"Unknown";
	}
	//TODO: Хотя, надо бы еще проверить, в главной ли мы нити...
	if (gpActivePlugin == NULL)
	{
		// Значит мы скорее всего НЕ в стеке вызовов FAR!
		_ASSERTE(gpActivePlugin != NULL);
		wchar_t szMessage[1024];
		wsprintfW(szMessage, L"Invalid operation\n%s:%i\ngpActivePlugin == NULL", pszFile, nLine);
		MessageBoxW(NULL, szMessage, L"RegEditor", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
	} else {
		_ASSERTE(psi.Message != NULL);
		TCHAR sFileLine[128]; lstrcpy_t(sFileLine, 100, pszFile);
		wsprintf(sFileLine+lstrlen(sFileLine), _T(":%i"), nLine);
		const TCHAR* sLines[] = 
		{
			GetMsg(REPluginName),
			_T("Invalid operation"),
			sFileLine,
			cfg->sVersionInfo
		};
		psi.Message(_PluginNumber(guid_InvalidOp), FMSG_WARNING|FMSG_MB_OK, NULL, 
			sLines, countof(sLines), 0);
	}
}

// pnFormat -> 0: REGEDIT4, 1: REGEDIT5, 2: Hive
// pbUnicode (для 0&1) - сам файл юникодный
// pbBOM (для pbUnicode==TRUE) - в начале - BOM
BOOL DetectFileFormat(const unsigned char *Data,int DataSize, int* pnFormat, BOOL* pbUnicode, BOOL* pbBOM)
{
	if (!Data || DataSize < 4)
	{
		_ASSERTE(Data && DataSize >= 4);
		return FALSE;
	}

	if (pbUnicode) *pbUnicode = FALSE;
	if (pbBOM) *pbBOM = FALSE;

	if (memcmp(Data, "regf", 4) == 0)
	{
		if (pnFormat) *pnFormat = 2;
		return TRUE;
	}

	if (!Data || DataSize < 8)
		return FALSE;

	LPCSTR  pszFormatA =  "Windows Registry Editor Version 5.00";
	LPCWSTR pszFormatW = L"Windows Registry Editor Version 5.00";
	DWORD cchFormatLen = lstrlenW(pszFormatW);

	if (memcmp(Data, "REGEDIT4", 8) == 0) 
	{
		if (pnFormat) *pnFormat = 0;
		return TRUE;
	}

	if (DataSize < 8)
		return FALSE;
	if (memcmp(Data, "REGEDIT5", 8) == 0
		|| memcmp(Data, pszFormatA, cchFormatLen) == 0)
	{
		if (pnFormat) *pnFormat = 1;
		return TRUE;
	}

	WORD nBOM = 0xFEFF;
	wchar_t* pszData = (wchar_t*)Data;
	// Если есть BOM - пропустить
	if (*pszData == nBOM)
	{
		pszData ++;
		if (pbBOM) *pbBOM = TRUE;
	}

	if (DataSize < 16)
		return FALSE;
	if (wmemcmp(pszData, L"REGEDIT5", 8) == 0 // REGEDIT5 для упрощения редактирования :)
		|| wmemcmp(pszData, pszFormatW, cchFormatLen) == 0)
	{
		if (pnFormat) *pnFormat = 1;
		if (pbUnicode) *pbUnicode = TRUE;
		return TRUE;
	}
	if (wmemcmp(pszData, L"REGEDIT4", 8) == 0)
	{
		if (pnFormat) *pnFormat = 0;
		if (pbUnicode) *pbUnicode = TRUE;
		return TRUE;
	}

	return FALSE;
}

// pnFormat -> 0: REGEDIT4, 1: REGEDIT5, 2: Hive
// pbUnicode (для 0&1) - сам файл юникодный
// pbBOM (для pbUnicode==TRUE) - в начале - BOM
BOOL DetectFileFormat(LPCWSTR apszFileName, int* pnFormat, BOOL* pbUnicode, BOOL* pbBOM)
{
	BOOL lbRc = FALSE;
	HANDLE hFile = CreateFileW(apszFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		if (dwErr == ERROR_SHARING_VIOLATION)
			hFile = CreateFileW(apszFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	}
	if (hFile && hFile != INVALID_HANDLE_VALUE)
	{
		BYTE Data[160]; DWORD dwRead = 0;
		if (ReadFile(hFile, Data, sizeof(Data), &dwRead, 0))
		{
			lbRc = DetectFileFormat(Data, dwRead, pnFormat, pbUnicode, pbBOM);
		}
		CloseHandle(hFile);
	}

	return lbRc;
}

DWORD SamDesired(DWORD abWow64on32)
{
	switch (abWow64on32)
	{
	case 0:
		return KEY_WOW64_32KEY;
	case 1:
		return KEY_WOW64_64KEY;
	default:
		return 0;
	}
}

const TCHAR* BitSuffix(DWORD abWow64on32, DWORD abVirtualize)
{
	switch (abWow64on32)
	{
	case 0:
		return GetMsg(abVirtualize ? REPanelx32VirtLabel : REPanelx32Label);
	case 1:
		return GetMsg(abVirtualize ? REPanelx64VirtLabel : REPanelx64Label);
	default:
		return abVirtualize ? GetMsg(REPanelVirtLabel) : _T("");
	}
}

bool IsRegPath(LPCWSTR apsz, HKEY* phRootKey /*= NULL*/, LPCWSTR* ppszSubKey /*= NULL*/, BOOL abCheckExist /*= FALSE*/)
{
	if (*apsz == L'\\' || *apsz == L'"' || *apsz == L'[') apsz++;

	HKEY hRoot = NULL;
	wchar_t sFirstToken[MAX_REGKEY_NAME+1];
	LPCWSTR pszSlash = wcschr(apsz, L'\\');
	int nLen;
	if (pszSlash) {
		nLen = pszSlash - apsz;
		if (nLen > MAX_REGKEY_NAME)
			return false;
		memmove(sFirstToken, apsz, 2*nLen);
		sFirstToken[nLen] = 0;
	} else {
		nLen = lstrlenW(apsz);
		if (nLen > MAX_REGKEY_NAME)
			return false;
		lstrcpynW(sFirstToken, apsz, MAX_REGKEY_NAME+1);
	}
	if (!StringKeyToHKey(sFirstToken, &hRoot))
		return false;
	if ((((ULONG_PTR)hRoot) < HKEY__FIRST) || (((ULONG_PTR)hRoot) > HKEY__LAST))
		return false;
	
	if (phRootKey)
		*phRootKey = hRoot;

	if (!pszSlash)
	{
		if (ppszSubKey) *ppszSubKey = apsz+lstrlenW(apsz);
		return true;
	}

	apsz = pszSlash+1;
	if (ppszSubKey) *ppszSubKey = apsz;

	if (!abCheckExist)
	{
		return true;
	}
	
	pszSlash = wcschr(apsz, L'\\');
	if (pszSlash) {
		nLen = pszSlash - apsz;
		if (nLen > MAX_REGKEY_NAME)
			return false;
		memmove(sFirstToken, apsz, 2*nLen);
		sFirstToken[nLen] = 0;
	} else {
		nLen = lstrlenW(apsz);
		if (nLen > MAX_REGKEY_NAME)
			return false;
		lstrcpynW(sFirstToken, apsz, MAX_REGKEY_NAME+1);
	}

	MRegistryWinApi reg(0,0);
	if (reg.ExistKey(hRoot, sFirstToken, NULL) == 0)
		return true;

	return false;
}


void FormatDataVisual(REGTYPE nDataType, LPBYTE pData, DWORD dwDataSize, TCHAR* szDesc/*[128]*/)
{
	if (dwDataSize == 0)
	{
		szDesc[0] = 0;
		return;
	}
	
	//TODO: Преобразовать NonPrintable (<32) в их юникодные аналоги
	
	//[HKEY_CURRENT_USER\Software\Far\Users\Zeroes_And_Ones\Plugins\DialogM\PluginHotkeys]
	//"zg_case"=hex(4):31
	if ((nDataType == REG_DWORD || nDataType == REG_DWORD_BIG_ENDIAN) && dwDataSize != 4)
	{
		nDataType = REG_BINARY;
	}
	else if (nDataType == REG_QWORD && dwDataSize != 8)
	{
		nDataType = REG_BINARY;
	}

	switch(nDataType) {
		case REG__KEY:
		case REG__COMMENT:
			szDesc[0] = 0;
			break;
		case REG_DWORD:
			_ASSERTE(dwDataSize == 4);
			wsprintf(szDesc, _T("0x%08X (%i)"), *((DWORD*)pData), (int)*((int*)pData));
			break;
		case REG_DWORD_BIG_ENDIAN:
			_ASSERTE(dwDataSize == 4);
			wsprintf(szDesc, _T("0x%08X (%i) B"), *((DWORD*)pData), (int)*((int*)pData));
			break;
		case REG_QWORD:
			{
				_ASSERTE(dwDataSize == 8);
				u64 data = *((u64*)pData);
				if (!(data >> 32))
					wsprintf(szDesc, _T("0x%08X (%u) Q"), (DWORD)data, (DWORD)data);
				else
					wsprintf(szDesc, _T("0x%08X%08X"), (DWORD)((data & 0xFFFFFFFF00000000LL)>>32), (DWORD)(data & 0xFFFFFFFFLL));
			}
			break;
		case REG_SZ:
		case REG_EXPAND_SZ:
		{
			// Assume always as Unicode
			int nLen = dwDataSize>>1;
			if (nLen > 127) nLen = 127;
			// Делаем это с учетом "кривых строк", хранящихся в реестре БЕЗ закрывающего '\0'
			lstrcpy_t(szDesc, nLen, (wchar_t*)pData);
			szDesc[nLen] = 0;
			break;
		}
		case REG_MULTI_SZ:
		{
			int nLen = dwDataSize>>1;
			if (nLen > 127) nLen = 127;
			#ifdef _UNICODE
			wmemmove(szDesc, (wchar_t*)pData, nLen);
			#else
			WideCharToMultiByte(cfg->nAnsiCodePage, 0, (wchar_t*)pData, nLen, szDesc, nLen, 0,0);
			#endif
			szDesc[nLen] = 0;
			//const wchar_t* pszSrc = (wchar_t*)pData; 
			//const wchar_t* pszSrcEnd = pszSrc + nLen;
			//wchar_t wc;
			
			TCHAR* pszDst = szDesc;
			TCHAR* pszDstEnd = szDesc+nLen;
			while (pszDst < pszDstEnd /*&& pszSrc < pszSrcEnd*/)
			{
				if (!*pszDst)
					*pszDst = _T(' ');
				pszDst++;
				//wc = *(pszSrc++);
				//switch (wc)
				//{
				//case 0:
				//	*(pszDst++) = _T(' '); break;
				//default:
				//	*(pszDst++) = wc;
				//}
			}
			*pszDst = 0;
			break;
		}
		default:
			//TODO: обработать остальные значения, а пока как binary
		{
			szDesc[0] = 0; TCHAR* psz = szDesc;
			int nMax = min(41,dwDataSize);
			for (int i = 0; i < nMax; i++)
			{
				//TODO: //OPTIMIZE: !!!
				wsprintf(psz, _T("%02X "), (DWORD)(BYTE)(pData[i]));
				psz += 3;
			}
			*psz = 0;
			if (dwDataSize == 4)
			{
				wsprintf(psz, _T("(%i)"), (int)*((int*)pData));
			}
			else if (dwDataSize == 2)
			{
				wsprintf(psz, _T("(%i)"), (DWORD)*((WORD*)pData));
			}
			else if (dwDataSize == 1)
			{
				wsprintf(psz, _T("(%u)"), (DWORD)*((BYTE*)pData));
			}
		}
	}
}



#ifdef _DEBUG
#define MAX_NAME 256

BOOL SearchTokenGroupsForSID (VOID) 
{
	DWORD i, dwSize = 0, dwResult = 0;
	HANDLE hToken;
	PTOKEN_GROUPS pGroupInfo;
	SID_NAME_USE SidType;
	TCHAR lpName[MAX_NAME];
	TCHAR lpDomain[MAX_NAME];
	BYTE sidBuffer[100];
	PSID pSID = (PSID)&sidBuffer;
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	DWORD dwErr = 0;

	// Open a handle to the access token for the calling process.

	if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken )) 
	{
		//printf( "OpenProcessToken Error %u\n", GetLastError() );
		dwErr = GetLastError();
		return FALSE;
	}

	// Call GetTokenInformation to get the buffer size.

	if(!GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize)) 
	{
		dwResult = GetLastError();
		if( dwResult != ERROR_INSUFFICIENT_BUFFER ) {
			//printf( "GetTokenInformation Error %u\n", dwResult );
			dwErr = GetLastError();
			return FALSE;
		}
	}

	// Allocate the buffer.

	pGroupInfo = (PTOKEN_GROUPS) GlobalAlloc( GPTR, dwSize );

	// Call GetTokenInformation again to get the group information.

	if(! GetTokenInformation(hToken, TokenGroups, pGroupInfo, 
		dwSize, &dwSize ) ) 
	{
		//printf( "GetTokenInformation Error %u\n", GetLastError() );
		dwErr = GetLastError();
		return FALSE;
	}

	// Create a SID for the BUILTIN\Administrators group.

	if(! AllocateAndInitializeSid( &SIDAuth, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSID) ) 
	{
		//printf( "AllocateAndInitializeSid Error %u\n", GetLastError() );
		dwErr = GetLastError();
		return FALSE;
	}

	// Loop through the group SIDs looking for the administrator SID.

	for(i=0; i<pGroupInfo->GroupCount; i++) 
	{
		if ( EqualSid(pSID, pGroupInfo->Groups[i].Sid) ) 
		{

			// Lookup the account name and print it.

			dwSize = MAX_NAME;
			if( !LookupAccountSid( NULL, pGroupInfo->Groups[i].Sid,
				lpName, &dwSize, lpDomain, 
				&dwSize, &SidType ) ) 
			{
				dwResult = GetLastError();
				if( dwResult == ERROR_NONE_MAPPED )
				{
					//strcpy_s (lpName, dwSize, "NONE_MAPPED" );
				}
				else 
				{
					//printf("LookupAccountSid Error %u\n", GetLastError());
					dwErr = GetLastError();
					return FALSE;
				}
			}
			//printf( "Current user is a member of the %s\\%s group\n", 
			//	lpDomain, lpName );

			// Find out whether the SID is enabled in the token.
			if (pGroupInfo->Groups[i].Attributes & SE_GROUP_ENABLED)
			{
				//printf("The group SID is enabled.\n");
			} else if (pGroupInfo->Groups[i].Attributes & SE_GROUP_USE_FOR_DENY_ONLY) {
				//printf("The group SID is a deny-only SID.\n");
			} else { 
				//printf("The group SID is not enabled.\n");
			}
		}
	}

	if (pSID)
		FreeSid(pSID);
	if ( pGroupInfo )
		GlobalFree( pGroupInfo );
	return TRUE;
}
#endif

// Регистронезависимое сравнения латинских букв
// c - lower case char!
bool CHREQ(const wchar_t* s, int i, wchar_t c)
{
	//bool bRc = ( ((((DWORD)(s)[i])|0x20) == (((DWORD)c)|0x20)) || ((((DWORD)(s)[i])&~0x20) == (((DWORD)c)&~0x20)) );
	bool bRc = (s[i] == c) || (s[i] == (c&~0x20));
	return bRc;
}

void DebracketRegPath(wchar_t* asRegPath)
{
	if (!asRegPath || !*asRegPath)
		return;

	int nDirLen = lstrlenW(asRegPath);
	if (asRegPath[0] == L'[' && asRegPath[nDirLen-1] == L']')
	{
		// Это может быть строка, скопированная из *.reg
		// "[HKEY_CURRENT_USER\Software\Far\Users]"
		if (CHREQ(asRegPath, 1, L'h') && CHREQ(asRegPath, 2, L'k') 
			&& CHREQ(asRegPath, 3, L'e') && CHREQ(asRegPath, 4, L'y'))
		{
			asRegPath[0] = L'\\'; asRegPath[nDirLen-1] = 0;
		}
	}
}



CPluginActivator::CPluginActivator(HANDLE hPlugin, u64 anOpMode)
{
	_ASSERTE((gpActivePlugin == NULL || (gpPluginList && gpPluginList->IsValid(gpActivePlugin))) || gpActivePlugin == (REPlugin*)hPlugin);
	pBefore = gpActivePlugin;
	pWasSet = (REPlugin*)hPlugin;
	_ASSERTE(pWasSet->mn_ActivatedCount>=0);
	pWasSet->mn_ActivatedCount++;
	gpActivePlugin = pWasSet;
	nBeforeOpMode = gnOpMode;
	gnOpMode = anOpMode;
}
CPluginActivator::~CPluginActivator()
{
	if (!pWasSet || !gpActivePlugin)
	{
		// gpActivePlugin не трогать!
	}
	else if (!gpPluginList)
	{
		gpActivePlugin = NULL;
	}
	else if (!gpPluginList->IsValid(pWasSet))
	{
		gpActivePlugin = NULL;
	}
	else
	{
		_ASSERTE(pWasSet->mn_ActivatedCount > 0);
		pWasSet->mn_ActivatedCount--;
		if (pWasSet->mn_ActivatedCount<=0)
			pBefore = NULL;

		if (gpActivePlugin == pWasSet)
		{				
			gpActivePlugin = pBefore;
			gnOpMode = nBeforeOpMode;
		}
		else if (gpActivePlugin == NULL && (!pBefore || pWasSet == pBefore))
		{
			// уже сбросили из другой нити?
		}
		else
		{
			_ASSERTE(gpActivePlugin == pWasSet);
		}
	}
};
