
#include "header.h"
#include "RE_Guids.h"

//#define SHOW_DURATION
#define USE_REAL_CRC32
#define STORE_FULLPATH_ROOTONLY

#ifdef _DEBUG
//	#define FORCE_BACKUP_CRC 2919101836
#endif

#ifdef KEEP_LOWERS
	#define pszNameCmp pszNameLwr
	#define CompareName(p,apszNameLwr,cchCmpCount) wmemcmp(p->pszNameLwr,apszNameLwr,cchCmpCount)
#else
	#define pszNameCmp pszName
	#define CompareName(p,apszNameLwr,cchCmpCount) lstrcmpiW(p->pszName,apszNameLwr)
#endif

// Для удобства надо бы в каждом ключе хранить его ПОЛНЫЙ путь, вместе с HKEY_xxx
// Это для того, чтобы в любом случае поддержать целостность при FindExistKey

/*

	Объект предназначен для загрузки текста в формате
	REGEDIT4/REGEDIT5 в "деревянную" структуру.
	С физическими файлами - сам не работает, вызывает MFileTxt.

*/



#ifdef USE_CRC_CACHE
FileRegCrcCache* FileRegCrcCache::InitializeFrom(FileRegItem *pFirstItem)
{
	_ASSERTE(IsUsed==0 && pCRC32==NULL && ppItems==NULL && nItemsCount==0 && pNextBlock==NULL && pActiveBlock==NULL);
	pCRC32 = (DWORD*)malloc(sizeof(DWORD)*CRC_CACHE_COUNT);
	ppItems = (FileRegItem**)malloc(sizeof(FileRegItem*)*CRC_CACHE_COUNT);
	_ASSERTE(pNextBlock==NULL);
	_ASSERTE(nItemsCount==0);
	pActiveBlock = this;
	// Поехали заполнять
	FileRegItem *p = pFirstItem;
	while (p)
	{
		if (nItemsCount == CRC_CACHE_COUNT) {
			// По идее, функция должна быть вызвана, когда в папке будет 128 элементов,
			// чтобы не перенапрягать инициализацию/копирование.
			_ASSERTE(nItemsCount < CRC_CACHE_COUNT);
			_ASSERTE(pActiveBlock==this);
			pNextBlock = (FileRegCrcCache*)calloc(1,sizeof(FileRegCrcCache));
			pActiveBlock = pNextBlock->InitializeFrom(p);
			break;
		}
		pCRC32[nItemsCount] = p->NameCRC32;
		ppItems[nItemsCount++] = p;
		p = p->pNextSibling;
	}
	IsUsed = 1;
	return pActiveBlock;
}
void FileRegCrcCache::AddNewItem(FileRegItem *pItem, DWORD nCRC32)
{
#ifdef FORCE_BACKUP_CRC
	if (nCRC32 == FORCE_BACKUP_CRC) {
		nCRC32 = FORCE_BACKUP_CRC;
	}
#endif
	_ASSERTE(pItem->pNextSibling == NULL);
	_ASSERTE(pActiveBlock!=NULL);
	_ASSERTE(pItem->bDeletion==FALSE);
	if (pActiveBlock->nItemsCount == CRC_CACHE_COUNT) {
		pActiveBlock->pNextBlock = (FileRegCrcCache*)calloc(1,sizeof(FileRegCrcCache));
		pActiveBlock = pActiveBlock->pNextBlock->InitializeFrom(pItem);
	} else {
		pActiveBlock->pCRC32[pActiveBlock->nItemsCount] = pItem->NameCRC32;
		pActiveBlock->ppItems[pActiveBlock->nItemsCount++] = pItem;
	}
}
FileRegItem* FileRegCrcCache::FindItem(const wchar_t *apszName, DWORD anNameCRC32)
{
	_ASSERTE(ppItems!=NULL && pCRC32!=NULL && apszName!=NULL);
	_ASSERTE(anNameCRC32 != 0);
	FileRegCrcCache *p = this;
	while (p)
	{
		int j = p->nItemsCount;
		_ASSERTE(j <= CRC_CACHE_COUNT);
		LPDWORD lpCRC = p->pCRC32;
		FileRegItem **lpItems = p->ppItems;
		for (int i = 0; i < j; i++)
		{
			if (lpCRC[i] == anNameCRC32)
			{
				FileRegItem* pFind = lpItems[i];
				_ASSERTE(pFind->bDeletion==FALSE);
				if (pFind && pFind->pszName && lstrcmpiW(apszName, pFind->pszName) == 0)
					return pFind;
			}
			#ifdef FORCE_BACKUP_CRC
			else if (anNameCRC32 == FORCE_BACKUP_CRC)
			{
				FileRegItem* pFindB = lpItems[i];
				if (pFindB->pszName && lstrcmpiW(apszName, pFindB->pszName) == 0)
				{
					_ASSERTE(pFindB->pszName);
					return pFindB;
				}
			}
			#endif
		}
		// Следующий блок кешированных CRC
		p = p->pNextBlock;
	}
	return NULL;
}
void FileRegCrcCache::DeleteItem(FileRegItem *pItem)
{
	_ASSERTE(ppItems!=NULL && pCRC32!=NULL && pItem);
	FileRegCrcCache *p = this;
	while (p)
	{
		int j = p->nItemsCount;
		_ASSERTE(j <= CRC_CACHE_COUNT);
		LPDWORD lpCRC = p->pCRC32;
		FileRegItem **lpItems = p->ppItems;
		for (int i = 0; i < j; i++)
		{
			if (lpItems[i] == pItem)
			{
				lpItems[i] = NULL;
				lpCRC[i] = 0;
				return;
			}
		}
		// Следующий блок кешированных CRC
		p = p->pNextBlock;
	}
}
#endif




#define RETURN(n) { /*nLastErr = (n);*/ return (n); }

MFileReg::MFileReg(BOOL abWow64on32, BOOL abVirtualize)
	: MRegistryBase(abWow64on32, abVirtualize)
{
	eType = RE_REGFILE;

	//nLastErr = 0;
	bUnicode = bFirstFileFormatUnicode = FALSE; // определяется в момент импорта!
	bSilent = FALSE;
	//bInternalRegParsing = FALSE;
	
	// Value import buffer
	pImportBufferData = NULL;
	cbImportBufferData = 0;
	psRegFilePathName = NULL;
	psShowFilePathName = NULL;
	mps_KeyPathLowerBuff = NULL; mn_KeyPathLowerBuff = 0; // InternalMalloc

	// HEX
	memset(nValueFromHex, 0xFF, sizeof(nValueFromHex));
	BYTE c;
	for (c = (BYTE)'0'; c <= (BYTE)'9'; c++)
		nValueFromHex[c] = c - '0';
	for (c = (BYTE)'a'; c <= (BYTE)'f'; c++)
		nValueFromHex[c] = c - (BYTE)'a' + 10;
	for (c = (BYTE)'A'; c <= (BYTE)'F'; c++)
		nValueFromHex[c] = c - (BYTE)'A' + 10;
	
	// Tree
	memset(&TreeRoot, 0, sizeof(TreeRoot));
	TreeRoot.nMagic = FILEREGITEM_MAGIC;

	//// InternalMalloc
	//nBlockDefaultSize = nBlockCount = 0;
	//nBlockActive = -1;
	//pMemBlocks = pMemActiveBlock = NULL;
}

MFileReg::~MFileReg()
{
	// Free blocks
	SafeFree(psRegFilePathName);
	SafeFree(psShowFilePathName);
	SafeFree(pImportBufferData);
	
	// Free Tree, Освобождаем нашу память //TreeDeleteChild(NULL, &TreeRoot);
	ReleaseMalloc(); // хоть и должен выполниться деструктором MInternalMalloc, но на всякий случай позовем
	memset(&TreeRoot, 0, sizeof(TreeRoot));
}

void MFileReg::MarkDirty(FileRegItem* pItem)
{
	pItem->bIndirect = TRUE;
	bDirty = true;
}

BOOL MFileReg::IsInitiallyUnicode()
{
	return bFirstFileFormatUnicode;
}

wchar_t* MFileReg::GetFirstKeyFilled()
{
	if ((TreeRoot.nValCount + TreeRoot.nKeyCount) != 1)
		return NULL;

	wchar_t* pszFirstFilled = (wchar_t*)malloc(16384*2); pszFirstFilled[0] = 0;
	wchar_t* psz = pszFirstFilled;
	FileRegItem *p = TreeRoot.pFirstKey;
	while (p)
	{
		if (!p->pszName || p->bDeletion)
			break;
		int nLeft = 16384 - (psz - pszFirstFilled) - 1;
		if (nLeft < MAX_PATH)
			break;
		if (*pszFirstFilled) {
			*(psz++) = L'\\'; *(psz) = 0;
		}
		lstrcpynW(psz, p->pszName, nLeft);
		psz += lstrlenW(psz);
		if ((p->nValCount + p->nKeyCount) != 1)
			break;
		p = p->pFirstKey;
	}
	return pszFirstFilled;
}

//MFileRegMemoryBlock* MFileReg::InitializeMalloc(size_t anDefaultBlockSize)
//{
//	if (anDefaultBlockSize < 0x10000)
//		anDefaultBlockSize = 0x10000;
//	if (anDefaultBlockSize > nBlockDefaultSize)
//		nBlockDefaultSize = anDefaultBlockSize;
//
//	// nBlockActive в конструкторе устанавливается в -1
//	nBlockActive++;
//	_ASSERTE(nBlockActive >= 0);
//
//	if (pMemBlocks && (DWORD)nBlockActive >= nBlockCount) {
//		DWORD nNewCount = nBlockActive + 32;
//		MFileRegMemoryBlock* pNew = (MFileRegMemoryBlock*)calloc(nNewCount, sizeof(MFileRegMemoryBlock));
//		memmove(pNew, pMemBlocks, sizeof(MFileRegMemoryBlock)*nBlockCount);
//		SafeFree(pMemBlocks);
//		pMemBlocks = pNew;
//		nBlockCount = nNewCount;
//	} else if (!pMemBlocks) {
//		nBlockCount = 32;
//		pMemBlocks = (MFileRegMemoryBlock*)calloc(nBlockCount, sizeof(MFileRegMemoryBlock));
//	}
//	_ASSERTE(nBlockCount && pMemBlocks);
//
//	pMemActiveBlock = (pMemBlocks+nBlockActive);
//	pMemActiveBlock->nBlockSize = nBlockDefaultSize;
//	pMemActiveBlock->nBlockFree = nBlockDefaultSize;
//	pMemActiveBlock->ptrBlock = (LPBYTE)malloc(nBlockDefaultSize);
//	pMemActiveBlock->ptrFirstFree = pMemActiveBlock->ptrBlock;
//	
//	return pMemActiveBlock;
//}

//__inline void* MFileReg::InternalMalloc(size_t anSize)
//{
//	_ASSERTE(nBlockDefaultSize != 0 && pMemBlocks != NULL);
//	//MFileRegMemoryBlock* p = pMemActiveBlock/*pMemBlocks+nBlockActive*/;
//	if ((size_t)pMemActiveBlock->nBlockFree < anSize)
//		InitializeMalloc(anSize);
//
//	_ASSERTE(pMemActiveBlock->ptrBlock != NULL);
//	LPBYTE ptr = pMemActiveBlock->ptrFirstFree;
//	pMemActiveBlock->nBlockFree -= anSize;
//	pMemActiveBlock->ptrFirstFree += anSize;
//	_ASSERTE(pMemActiveBlock->nBlockFree >= 0);
//	return ptr;
//}

//void MFileReg::ReleaseMalloc()
//{
//	for (DWORD n = 0; n < nBlockCount; n++)
//	{
//		SafeFree(pMemBlocks[n].ptrBlock);
//	}
//	SafeFree(pMemBlocks);
//}

// Возвращает имя загруженного файла (LoadRegFile)
LPCWSTR MFileReg::GetFilePathName()
{
	_ASSERTE(psRegFilePathName != NULL);
	return psRegFilePathName;
}

LPCTSTR MFileReg::GetShowFilePathName()
{
	_ASSERTE(psRegFilePathName != NULL);
	if (!psShowFilePathName)
		psShowFilePathName = UnmakeUNCPath_t(psRegFilePathName);
	return psShowFilePathName;
}

LPBYTE MFileReg::GetImportBuffer(size_t cbSize) // --> pExportBufferData
{
	if (!pImportBufferData || cbSize > cbImportBufferData) {
		SafeFree(pImportBufferData);
		cbImportBufferData = cbSize;
		pImportBufferData = (LPBYTE)malloc(cbImportBufferData);
	}
	return pImportBufferData;
}

// ***
MRegistryBase* MFileReg::Duplicate()
{
	_ASSERTE(FALSE);
	InvalidOp();
	return NULL;
}

__inline DWORD MFileReg::CalculateNameCRC32(const wchar_t *apszName, int anNameLen)
{
#ifdef USE_REAL_CRC32
	DWORD nCRC32 = 0xFFFFFFFF;

	static DWORD CRCtable[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
	0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
	0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
	0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
	0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
	0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
	0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
	0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
	0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
	0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
	0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
	0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
	0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
	0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
	0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
	0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
	0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
	0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
	0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
	0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
	0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
	0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

	DWORD dwRead = anNameLen << 1;
	for (LPBYTE p = (LPBYTE)apszName; (dwRead--)!=0;)
	{
		nCRC32 = ( nCRC32 >> 8 ) ^ CRCtable[(unsigned char) ((unsigned char) nCRC32 ^ *p++ )];
	}

	// т.к. нас интересует только сравнение - последний XOR необязателен!
	//nCRC32 = ( nCRC32 ^ 0xFFFFFFFF );
	
#else
	// Сделать расчет CRC имени ключа. Так как граница DWORD - то дополнительную
	// память выделять не нужно. Последний DWORD либо захватит \0, либо нет.
	DWORD nDwordCount = (anNameLen+1) >> 1;
	
	// Это конечно не алгоритм CRC, но для базового сравнения содержимого потянет
	DWORD nCRC32 = 0x7A3B91F4;
	for (DWORD i = 0; i < nDwordCount; i++)
		nCRC32 ^= ((LPDWORD)apszName)[i];

#endif

	return nCRC32;
}

BOOL MFileReg::GetSubkeyToken(LPCWSTR& lpSubKey,
		wchar_t* pszSubkeyToken/*[MAX_PATH+1]*/,
		wchar_t* pszSubkeyTokenLwr/*[MAX_PATH+1]*/,
		int& rnTokenLen, DWORD& rnCRC32)
{
	_ASSERTE(MAX_REGKEY_NAME <= MAX_PATH);
	
	if (!lpSubKey || !*lpSubKey)
	{
		_ASSERTE(lpSubKey && *lpSubKey);
		return FALSE;
	}
		
	LPCWSTR pszSlash = wcschr(lpSubKey, L'\\');
	if (!pszSlash) pszSlash = lpSubKey + lstrlenW(lpSubKey); // последний токен
	
	size_t nTokenLen = pszSlash - lpSubKey;
	if (nTokenLen > MAX_REGKEY_NAME) { // длина имени ключа не может быть более MAX_REGKEY_NAME
		_ASSERTE(nTokenLen <= MAX_REGKEY_NAME);
		//TODO: Отображение ошибки, с номером строки в *.reg
		return FALSE;
	}
	rnTokenLen = (DWORD)nTokenLen;
	MCHKHEAP;

	wmemmove(pszSubkeyToken, lpSubKey, (int)nTokenLen);
	pszSubkeyToken[nTokenLen] = 0;
	wmemmove(pszSubkeyTokenLwr, lpSubKey, (int)nTokenLen);
	pszSubkeyTokenLwr[nTokenLen] = 0;
	CharLowerBuffW(pszSubkeyTokenLwr, nTokenLen);
	rnCRC32 = CalculateNameCRC32(pszSubkeyTokenLwr, nTokenLen);

	// Next token
	if (*pszSlash == L'\\') {
		lpSubKey = pszSlash+1;
		if (*lpSubKey == 0) {
			_ASSERTE(*lpSubKey != 0);
			lpSubKey = NULL;
		}
	} else {
		_ASSERTE(*pszSlash == 0);
		lpSubKey = NULL;
	}
	
	return TRUE;
}

// Wrappers
LONG MFileReg::RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged)
{
	FileRegItem *pKey = NULL, *pParent = NULL; //ItemFromHkey(hKey);

	if (!lpOldSubKey || !lpNewSubKey || !apParent || !*lpOldSubKey || !*lpNewSubKey || !pbRegChanged)
	{
		InvalidOp();
		return E_INVALIDARG;
	}

	HRESULT hr = OpenKeyEx(apParent->mh_Root, apParent->mpsz_Key, 0, KEY_ALL_ACCESS, (HKEY*)&pParent, &apParent->nKeyFlags);
	if (hr != 0 || pParent == NULL)
	{
		//TODO: Показать ошибку?
		return (hr ? hr : ERROR_FILE_NOT_FOUND);
	}

	hr = OpenKeyEx((HKEY)pParent, lpOldSubKey, 0, KEY_ALL_ACCESS, (HKEY*)&pKey, NULL);
	if (hr != 0)
		RETURN(hr);

	if (abCopyOnly)
	{
		//TODO: Сделать копирование
		InvalidOp();
	}
	else
	{
		// Если имя другое - то нужно предварительно удалить ключ с этим новым именем
		if (lstrcmpiW(lpOldSubKey, lpNewSubKey) != 0)
		{
			FileRegItem *pNewName = NULL;
			hr = OpenKeyEx((HKEY)pParent, lpNewSubKey, REG__OPTION_CREATE_DELETED, KEY_ALL_ACCESS, (HKEY*)&pNewName, NULL);
			if (hr == S_OK && pNewName)
			{
				_ASSERTE(pParent == pNewName->pParent);
				TreeDeleteChild(pParent, pNewName);
				*pbRegChanged = TRUE;
			}
			hr = OpenKeyEx((HKEY)pParent, lpNewSubKey, 0, KEY_ALL_ACCESS, (HKEY*)&pNewName, NULL);
			if (hr == S_OK && pNewName)
			{
				_ASSERTE(pParent == pNewName->pParent);
				TreeDeleteChild(pParent, pNewName);
				*pbRegChanged = TRUE;
			}
		}
		// Теперь - можно просто переименовать
		int nOldLen = lstrlenW(lpOldSubKey);
		int nNewLen = lstrlenW(lpNewSubKey);
		if (nNewLen > nOldLen)
		{
			if (pKey->nMagic != FILEREGITEM_MAGIC)
			{
				_ASSERTE(pKey->nMagic == FILEREGITEM_MAGIC);
				InvalidOp();
				return E_UNEXPECTED;
			}
			pKey->pszName = (LPCWSTR)InternalMalloc((nNewLen+1)<<1);
		}
		wmemmove((wchar_t*)pKey->pszName, lpNewSubKey, (nNewLen+1));
		pKey->NameLen = nNewLen;
		// Теперь пересчитать CRC32 для LowerCase!
		wchar_t* pszSubkeyLwr = lstrdup(lpNewSubKey);
		CharLowerBuffW(pszSubkeyLwr, nNewLen);
		pKey->NameCRC32 = CalculateNameCRC32(pszSubkeyLwr, nNewLen);
		#ifdef KEEP_LOWERS
		pKey->pszNameLwr = (LPCWSTR)InternalMalloc((nNewLen+1)<<1);
		wmemmove((wchar_t*)pKey->pszNameLwr, pszSubkeyLwr, (nNewLen+1));
		#endif
		SafeFree(pszSubkeyLwr); // lstrdup
		if (pParent->pCrcKey)
		{
			pParent->pCrcKey->DeleteItem(pKey);
			pParent->pCrcKey->AddNewItem(pKey, pKey->NameCRC32);
		}
		*pbRegChanged = TRUE;
	}

	return S_OK;
}

LONG MFileReg::CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/, LPCWSTR pszComment /*= NULL*/)
{
	_ASSERTE(phkResult != NULL);
	*phkResult = NULL;
	if (apRights) *apRights = eRightsSimple;
	
	MCHKHEAP;
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	MCHKHEAP;
	// Этот флаг НУЖЕН для того, чтобы можно было экспортировать "аннулированные" ключи
	BOOL bKeyDeletion = (dwOptions & REG__OPTION_CREATE_DELETED) == REG__OPTION_CREATE_DELETED;
	
	if (!lpSubKey) lpSubKey = L"";
	
	LONG hRc = TreeCreateKey(pParent,
		lpSubKey, lstrlenW(lpSubKey), TRUE/*abAllowCreate*/, bKeyDeletion, FALSE/*abInternal*/, (FileRegItem**)phkResult, lpdwDisposition);
	
	//2010-08-03 - так низя - pParent содержит указатель на HKEY_xxx, а 
	//   lpSubKey может содержать путь вида "Software\\Far\\KeyMacros"
	//MarkDirty(pParent);

	if (!hRc && *phkResult && pnKeyFlags)
	{
		// Пометить, что ключ реально "используется"
		MarkDirty((FileRegItem*)*phkResult);

		if (pnKeyFlags)
		{
			FileRegItem* p = (FileRegItem*)*phkResult;
			*pnKeyFlags = (p->bDeletion ? REGF_DELETED : 0) | (p->bIndirect ? REGF_INDIRECT : 0);
		}
	}

	RETURN(hRc);
}
LONG MFileReg::OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/)
{
	_ASSERTE(phkResult != NULL);
	*phkResult = NULL;
	if (apRights) *apRights = eRightsSimple;
	
	MCHKHEAP;
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	MCHKHEAP;
	
	if (!lpSubKey) lpSubKey = L"";

	// Этот флаг НУЖЕН для того, чтобы можно было экспортировать "аннулированные" ключи
	BOOL bKeyDeletion = (ulOptions & REG__OPTION_CREATE_DELETED) == REG__OPTION_CREATE_DELETED;
	
	LONG hRc = TreeCreateKey(pParent,
		lpSubKey, lstrlenW(lpSubKey), FALSE/*abAllowCreate*/, bKeyDeletion, FALSE/*abInternal*/, (FileRegItem**)phkResult);
	if (!hRc && *phkResult && pnKeyFlags)
	{
		FileRegItem* p = (FileRegItem*)*phkResult;
		*pnKeyFlags = (p->bDeletion ? REGF_DELETED : 0) | (p->bIndirect ? REGF_INDIRECT : 0);
	}
	RETURN(hRc);
}
LONG MFileReg::CloseKey(HKEY* phKey)
{
	// hKey это просто указатели на наше дерево, ничего освобождать не нужно
	*phKey = NULL;
	RETURN(0);
}
LONG MFileReg::QueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	if (lpClass && lpcClass && *lpcClass)
		*lpClass = 0;
	if (lpcClass)
		*lpcClass = 0;
	if (lpcSubKeys)
		*lpcSubKeys = pParent->nKeyCount;
	if (lpcMaxSubKeyLen)
		*lpcMaxSubKeyLen = pParent->nMaxKeyNameLen;
	if (lpcMaxClassLen)
		*lpcMaxClassLen = 0;
	if (lpcValues)
	{
		//TODO: nValCommentCount ?
		*lpcValues = pParent->nValCount;
	}
	if (lpcMaxValueNameLen)
		*lpcMaxValueNameLen = pParent->nMaxValNameLen;
	if (lpcMaxValueLen)
		*lpcMaxValueLen = pParent->nMaxValDataSize;
	if (lpcbSecurityDescriptor)
		*lpcbSecurityDescriptor = 0;
	if (lpftLastWriteTime)
		*lpftLastWriteTime = pParent->ftCreationTime;

	RETURN(0);
}

//TODO: Обработать abEnumComments
LONG MFileReg::EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment /*= NULL*/)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	// nValCommentCount - общее количество комментариев и значений
	// nValCount - "выжимка" только по значениям
	UINT lnCount = (abEnumComments ? pParent->nValCommentCount : pParent->nValCount);
	if (dwIndex >= lnCount)
		RETURN(ERROR_NO_MORE_ITEMS);

	FileRegItem* p = NULL;
	if (pParent->bValIndexComment == abEnumComments && pParent->dwValIndex == dwIndex && pParent->pValIndex)
	{
		p = pParent->pValIndex;
	}
	else if (pParent->pFirstVal && dwIndex < lnCount)
	{
		DWORD nSkipItems = 0;

		//// Защита от сбоев в кешировании?
		//for (int iStep = 2; iStep; iStep--)
		//{

		if (dwIndex < pParent->dwValIndex || !pParent->pValIndex || pParent->bValIndexComment != abEnumComments)
		{
			nSkipItems = dwIndex;
			p = pParent->pFirstVal;
		}
		else
		{
			nSkipItems = dwIndex - pParent->dwValIndex;
			p = pParent->pValIndex;
		}
		
		// Пропустить nSkipItems и остановиться на СЛЕДУЮЩЕМ значении
		if (abEnumComments)
		{
			while (p)
			{
				_ASSERTE(p->nValueType != REG__KEY);
				
				if (!nSkipItems)
					break; // нашли
				nSkipItems --;
				p = p->pNextSibling;
			}
		} else {
			// Комментарии не нужны
			while (p)
			{
				_ASSERTE(p->nValueType != REG__KEY);
				if (p->nValueType != REG__COMMENT)
				{
					if (!nSkipItems)
						break; // нашли
					nSkipItems --;
				}
				p = p->pNextSibling;
			}
		}
		
		// Проверяем, должен быть, если количество правильно посчитано
		//if (!p)
		//{
			_ASSERTE(p!=NULL);
		//}
		//else
		//{
		//	break;
		//}
		//}
	}
	
	if (!p)
		RETURN(ERROR_NO_MORE_ITEMS);

	pParent->dwValIndex = dwIndex;
	pParent->bValIndexComment = abEnumComments;
	pParent->pValIndex = p;
	
	// Name and Data
	if (lpValueName && lpcchValueName)
	{
		if (p->pszName)
		{
			if (*lpcchValueName > 0)
			{
				//int nLen = lstrlenW(p->pszName);
				int nLen = p->NameLen;
				int nMax = *lpcchValueName;
				if (nLen >= nMax)
				{
					_ASSERTE(nLen < nMax);
					nLen = nMax - 1;
				}
				wmemmove(lpValueName, p->pszName, nLen);
				lpValueName[nLen] = 0;
				*lpcchValueName = nLen;
			}
		} else {
			*lpValueName = 0; *lpcchValueName = 0;
		}
	}
	if (lpDataType)
		*lpDataType = p->nValueType;
	if (lpData && lpcbData)
	{
		_ASSERTE(p->cbDataSize == 0 || p->pData);
		if (p->cbDataSize)
		{
			if (p->cbDataSize > *lpcbData)
			{
				*lpcbData = p->cbDataSize;
				RETURN(ERROR_MORE_DATA);
			}
			memmove(lpData, p->pData, p->cbDataSize);
		}		
		// done
		*lpcbData = p->cbDataSize;
	} else if (lpcbData && !lpData) {
		*lpcbData = p->cbDataSize;
	}

	if (ppszValueComment)
		*ppszValueComment = p->pszComment;

#ifdef _DEBUG
	if (p->nValueType == REG_DWORD) {
		//[HKEY_CURRENT_USER\Software\Far\Users\Zeroes_And_Ones\Plugins\DialogM\PluginHotkeys]
		//"zg_case"=hex(4):31
		_ASSERTE(p->cbDataSize == 4 || p->cbDataSize == 1);
	}
#endif
	
	RETURN(0);
}
LONG MFileReg::EnumKeyEx(
		HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName,
		LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime,
		DWORD* pnKeyFlags /*= NULL*/, TCHAR* lpDefValue /*= NULL*/, DWORD cchDefValueMax /*= 0*/,
		LPCWSTR* ppszKeyComment /*= NULL*/)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	if (dwIndex >= pParent->nKeyCount)
		RETURN(ERROR_NO_MORE_ITEMS);

	FileRegItem* p = NULL;
	if (pParent->dwKeyIndex == dwIndex && pParent->pKeyIndex)
	{
		p = pParent->pKeyIndex;
	} else if (pParent->pFirstKey && dwIndex < pParent->nKeyCount) {
		DWORD nSkipItems = 0;
		if (dwIndex < pParent->dwKeyIndex || !pParent->pKeyIndex)
		{
			nSkipItems = dwIndex;
			p = pParent->pFirstKey;
		} else {
			nSkipItems = dwIndex - pParent->dwKeyIndex;
			p = pParent->pKeyIndex;
		}
		
		// Пропустить nSkipItems и остановиться на СЛЕДУЮЩЕМ значении
		while (p)
		{
			_ASSERTE(p->nValueType == REG__KEY);
			
			if (!nSkipItems)
				break; // нашли
			nSkipItems --;
			p = p->pNextSibling;
		}
		
		// Проверяем, должен быть, если количество правильно посчитано
		_ASSERTE(p!=NULL);
	}
	
	if (!p)
		RETURN(ERROR_NO_MORE_ITEMS);

	pParent->dwKeyIndex = dwIndex;
	pParent->pKeyIndex = p;

	// LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime	
	if (lpName && lpcName)
	{
		if (p->pszName)
		{
			if (*lpcName > 0)
			{
				//int nLen = lstrlenW(p->pszName);
				int nLen = p->NameLen;
				int nMax = *lpcName;
				if (nLen >= nMax)
				{
					_ASSERTE(nLen < nMax);
					nLen = nMax - 1;
				}
				wmemmove(lpName, p->pszName, nLen);
				lpName[nLen] = 0;
				*lpcName = nLen;
			}
		} else {
			*lpName = 0; *lpcName = 0;
		}
	}
	if (lpClass && lpcClass && *lpcClass)
		*lpClass = 0;
	if (lpcClass)
		*lpcClass = 0;
	if (lpftLastWriteTime)
		*lpftLastWriteTime = p->ftCreationTime;
	if (pnKeyFlags)
	{
		if (p->nValCommentCount && !p->bIndirect)
		{
			_ASSERTE(p->bIndirect);
			MarkDirty(p);
		}
		*pnKeyFlags = (p->bDeletion ? REGF_DELETED : 0) | (p->bIndirect ? REGF_INDIRECT : 0);
	}
	// Вернуть значение по умолчанию для ключа - это для показа в дереве
	if (lpDefValue && cchDefValueMax)
	{
		if (p->pDefaultVal && p->pDefaultVal->cbDataSize >= 4)
		{
			lstrcpy_t(lpDefValue, cchDefValueMax, (wchar_t*)p->pDefaultVal->pData);
		}
		else
		{
			// Значения по умолчанию нет, но поскольку это используется только для
			// отображения в колонке C0/DIZ то вернем пустую строку, чтобы плагин
			// не пытался открыть ключ и считать default value самостоятельно
			lpDefValue[0] = _T(' '); lpDefValue[1] = 0;
		}	
	}

	if (ppszKeyComment)
		*ppszKeyComment = p->pszComment;

	RETURN(0);
}
LONG MFileReg::QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment /*= NULL*/)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);
	if (lpValueName && !*lpValueName)
	{
		if (pParent->bDeletion)
			RETURN(ERROR_FILE_NOT_FOUND);
		lpValueName = NULL;
	} else if (!lpValueName) {
		if (pParent->bDeletion)
			RETURN(ERROR_FILE_NOT_FOUND);
	}
	FileRegItem* p = TreeFindValue(pParent, lpValueName, NULL, -1, 0);
	if (!p)
		RETURN(ERROR_FILE_NOT_FOUND);

	_ASSERTE(p->nValueType != REG__KEY);
	// Return information
	if (lpDataType)
	{
		*lpDataType = p->nValueType;
	}
	if (lpcbData && lpData)
	{
		if (*lpcbData < p->cbDataSize)
		{
			*lpcbData = p->cbDataSize;
			RETURN(ERROR_MORE_DATA);
		}
		if (p->cbDataSize)
		{
			_ASSERTE(p->pData);
			memmove(lpData, p->pData, p->cbDataSize);
		}
		*lpcbData = p->cbDataSize;
	}
	else if (lpcbData)
	{
		*lpcbData = p->cbDataSize;
	}
	if (ppszValueComment)
		*ppszValueComment = p->pszComment;
	RETURN(0);
}
LONG MFileReg::SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment /*= NULL*/)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	//wchar_t sLower[32768];
	//DWORD cbCmpSize = 0;
	int nNameLen = 0;
	DWORD nNameCRC32 = 0;
	if (lpValueName && *lpValueName)
	{
		nNameLen = lstrlenW(lpValueName);
		if (nNameLen >= 32767)
		{
			_ASSERTE(nNameLen < 32767);
			nNameLen = 32767;
		}
		wmemmove(ms_LowerBuff, lpValueName, nNameLen); ms_LowerBuff[nNameLen] = 0;
		CharLowerBuffW(ms_LowerBuff, nNameLen);
		nNameCRC32 = CalculateNameCRC32(ms_LowerBuff, nNameLen);
	}

	int nCommLen = 0;
	if (pszComment) nCommLen = lstrlenW(pszComment);

	#ifdef _DEBUG
	FileRegItem* p =
	#endif
	TreeSetValue(pParent,
		lpValueName, ms_LowerBuff, nNameLen, nNameCRC32,
		nDataType, lpData, cbData, pszComment, nCommLen);
	// Пометить, что ключ реально "используется"
	MarkDirty(pParent);

	RETURN(0);

	//FileRegItem* p = TreeFindValue(pParent, lpValueName);
	//if (!p)
	//{
	//	if (lpValueName && !*lpValueName) lpValueName = NULL;
	//	p = TreeAddChild(pParent, dwType, lpValueName, lpData, cbData);
	//} else {
	//	_ASSERTE(cbData==0 || lpData!=NULL);
	//	if (cbData && lpData)
	//	{
	//		if (p->pData && p->nDataBufferSize >= cbData)
	//		{
	//			memmove((void*)(p->pData), lpData, cbData);
	//		} else {
	//			if (!p->pTempBuffer || p->nDataBufferSize < cbData) {
	//				//SafeFree(p->pTempBuffer); -- НЕ ТРЕБУЕТСЯ, это InternalMalloc
	//				p->nDataBufferSize = cbData;
	//				p->pTempBuffer = (LPBYTE)InternalMalloc(cbData);
	//			}
	//			memmove(p->pTempBuffer, lpData, cbData);
	//			p->pData = p->pTempBuffer;
	//		}
	//	}
	//	// Запомнить новый размер данных
	//	p->cbDataSize = cbData;
	//	
	//	// Обновить CD
	//	SYSTEMTIME st; GetSystemTime(&st);
	//	SystemTimeToFileTime(&st, (PFILETIME)&(p->ftCreationTime));
	//	pParent->ftCreationTime = p->ftCreationTime;
	//}
	//
	//RETURN(0);
}
LONG MFileReg::DeleteValue(HKEY hKey, LPCWSTR lpValueName)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	// Удаляем
	FileRegItem* p = TreeFindValue(pParent, lpValueName, NULL, -1, 0);
	if (p)
	{
		TreeDeleteChild(pParent, p);

		// Обновить CD родителя
		SYSTEMTIME st; GetSystemTime(&st);
		SystemTimeToFileTime(&st, (PFILETIME)&(pParent->ftCreationTime));

		// Пометить, что ключ реально "используется"
		MarkDirty(pParent);
	}
	RETURN(0);
}
LONG MFileReg::DeleteSubkeyTree(HKEY hKey, LPCWSTR lpSubKey)
{
	FileRegItem* pParent = ItemFromHkey(hKey);
	if (!pParent)
		RETURN(E_INVALIDARG);

	// Удаляем. lpSubKey может содержать ПУТЬ, тогда родитель - изменится
	FileRegItem* p = TreeFindKey(pParent, lpSubKey, NULL, -1, 0, FALSE);
	_ASSERTE(pParent != p);
	if (p)
	{
		// Пометить, что ключ реально "используется" (подвергся изменениям)
		MarkDirty(p->pParent);

		// Обновить CD родителя
		SYSTEMTIME st; GetSystemTime(&st);
		SystemTimeToFileTime(&st, (PFILETIME)&(p->pParent->ftCreationTime));

		TreeDeleteChild(p->pParent, p);

		//2010-08-03 - так низя - pParent содержит указатель на HKEY_xxx, а 
		//   lpSubKey может содержать путь вида "Software\\Far\\KeyMacros"
		//// Пометить, что ключ реально "используется"
		//MarkDirty(pParent);
	}
	RETURN(0);
}
LONG MFileReg::ExistValue(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpValueName, REGTYPE* pnDataType /*= NULL*/, DWORD* pnDataSize /*= NULL*/)
{
	FileRegItem* pParent = NULL; //ItemFromHkey(hKey);

	HRESULT hr = OpenKeyEx(hKey, lpszKey, 0, KEY_READ, (HKEY*)&pParent, NULL);
	if (hr != 0)
		RETURN(hr);

	if (!pParent)
		RETURN(ERROR_FILE_NOT_FOUND);

	FileRegItem* p = TreeFindValue(pParent, lpValueName, NULL, -1, 0);
	if (p)
	{
		if (pnDataType)
			*pnDataType = p->nValueType;
		if (pnDataSize)
			*pnDataSize = p->cbDataSize;
	}

	RETURN((p ? 0 : ERROR_FILE_NOT_FOUND));
}
LONG MFileReg::ExistKey(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpSubKey)
{
	FileRegItem* pParent = NULL; //ItemFromHkey(hKey);
	
	if (lpszKey == NULL && lpSubKey == NULL)
	{
		pParent = TreeFindPredefinedKey(&TreeRoot, hKey, FALSE);
		if (!pParent)
			return ERROR_FILE_NOT_FOUND;
		//TODO: Проверить Indirect?
		return 0;
	}

	HRESULT hr = OpenKeyEx(hKey, lpszKey, 0, KEY_READ, (HKEY*)&pParent, NULL);
	if (hr != 0)
		RETURN(hr);

	FileRegItem* p = TreeFindKey(pParent, lpSubKey, NULL, -1, 0, FALSE);
	_ASSERTE(pParent != p);

	RETURN((p ? 0 : ERROR_FILE_NOT_FOUND));
}
LONG MFileReg::SaveKey(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes /*= NULL*/)
{
	RETURN(E_NOTIMPL);
}
LONG MFileReg::RestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags /*= 0*/)
{
	RETURN(E_NOTIMPL);
}
LONG MFileReg::GetSubkeyInfo(HKEY hKey, LPCWSTR lpszSubkey, LPTSTR pszDesc, DWORD cchMaxDesc, LPTSTR pszOwner, DWORD cchMaxOwner)
{
	LONG hRc = 0;
	FileRegItem* p = NULL;

	DWORD nKeyFlagsRet = 0;
	hRc = OpenKeyEx(hKey, lpszSubkey, 0, KEY_READ, (HKEY*)p, &nKeyFlagsRet);
	if (hRc != 0)
	{
		// Значения по умолчанию нет, но поскольку это используется только для
		// отображения в колонке C0/DIZ то вернем пустую строку, чтобы плагин
		// не пытался открыть ключ и считать default value самостоятельно
		if (pszDesc) { pszDesc[0] = _T(' '); pszDesc[1] = 0; }
		if (pszOwner) { pszOwner[0] = _T(' '); pszOwner[1] = 0; }
		return hRc;
	}

	if (pszDesc && cchMaxDesc)
	{
		if (p->pDefaultVal && p->pDefaultVal->cbDataSize >= 4)
		{
			lstrcpy_t(pszDesc, cchMaxDesc, (wchar_t*)p->pDefaultVal->pData);
		}
		else
		{
			// Значения по умолчанию нет, но поскольку это используется только для
			// отображения в колонке C0/DIZ то вернем пустую строку, чтобы плагин
			// не пытался открыть ключ и считать default value самостоятельно
			pszDesc[0] = _T(' '); pszDesc[1] = 0;
		}	
	}
	if (pszOwner) { pszOwner[0] = _T(' '); pszOwner[1] = 0; }
	
	return 0;
}

void MFileReg::TreeSetComment(FileRegItem* pItem, LPCWSTR lpComment /*= NULL*/, int anCommentLen /*= -1*/)
{
	if (lpComment)
	{
		if (anCommentLen == -1) anCommentLen = lstrlenW(lpComment);
		if (anCommentLen || pItem->pszComment)
		{
			DWORD cbCommentSize = (anCommentLen<<1) + 2;
			wchar_t* psz = (wchar_t*)pItem->pszComment;
			if (!psz || cbCommentSize > pItem->cbCommSize)
			{
				cbCommentSize += 16;
				pItem->cbCommSize = cbCommentSize;
				pItem->pszComment = psz = (wchar_t*)InternalMalloc(cbCommentSize);
			}
			if (anCommentLen) wmemmove(psz, lpComment, anCommentLen);
			psz[anCommentLen] = 0;
		}
	}
	else if (pItem->pszComment)
	{
		((wchar_t*)pItem->pszComment)[0] = 0;
	}
}

FileRegItem* MFileReg::TreeSetValue(FileRegItem* pParent,
		LPCWSTR apszName, LPCWSTR apszNameLwr, int anNameLen, DWORD anNameCRC32,
		REGTYPE nDataType, const BYTE *lpData, DWORD cbData,
		LPCWSTR lpComment /*= NULL*/, int anCommentLen /*= -1*/, BOOL abDeletion /*= FALSE*/)
{
	FileRegItem* p = TreeFindValue(pParent, apszName, apszNameLwr, anNameLen, anNameCRC32);
	if (!p)
	{
		if (apszName && !*apszName) {
			apszName = apszNameLwr = NULL; anNameLen = 0; anNameCRC32 = 0;
		}
		p = TreeAddChild(pParent, nDataType, NULL, apszName, apszNameLwr, anNameLen, anNameCRC32, 
				lpData, cbData, lpComment, anCommentLen, abDeletion);
	} else {
		if (abDeletion)
		{
			p->bDeletion = TRUE;
		}
		else
		{
			_ASSERTE(cbData==0 || lpData!=NULL);
			if (cbData && lpData)
			{
				if (p->pData && p->nDataBufferSize >= cbData)
				{
					memmove((void*)(p->pData), lpData, cbData);
				} else {
					if (!p->pTempBuffer || p->nDataBufferSize < cbData) {
						//SafeFree(p->pTempBuffer); -- НЕ ТРЕБУЕТСЯ, это InternalMalloc
						p->nDataBufferSize = cbData;
						p->pTempBuffer = (LPBYTE)InternalMalloc(cbData);
					}
					memmove(p->pTempBuffer, lpData, cbData);
					p->pData = p->pTempBuffer;
				}
			}
			// Запомнить новый размер данных
			p->cbDataSize = cbData;
			// И новый тип значения
			p->nValueType = nDataType;
		}

		// Обновить комментарий в строке со значением
		TreeSetComment(p, lpComment, anCommentLen);

		// Обновить CD
		SYSTEMTIME st; GetSystemTime(&st);
		SystemTimeToFileTime(&st, (PFILETIME)&(p->ftCreationTime));
		pParent->ftCreationTime = p->ftCreationTime;
	}
	
	return p;
}

LONG MFileReg::TreeCreateKey(FileRegItem* pParent, const wchar_t *apszKeyPath, DWORD anPathLen,
		BOOL abAllowCreate, BOOL abDeletion, BOOL abInternal, FileRegItem** ppKey,
		LPDWORD lpdwDisposition /*= NULL*/, LPCWSTR apszComment /*= NULL*/, int anCommentLen /*= -1*/)
{
	//if (!abAllowCreate && abDeletion)
	//	return ERROR_FILE_NOT_FOUND;
	if (lpdwDisposition) *lpdwDisposition = REG_OPENED_EXISTING_KEY;

	//TODO: Заменить на более интеллектуальную функцию
	// Нужно сразу проверить (наверное можно даже регистронезависимо?)
	// может такой ключ уже был создан?
	LONG hRc = 0;
	//if (abAllowCreate)
	//	hRc = CreateKeyEx(ahPredefinedKey, apszKeyPath, 0, 0, abDeletion ? REG__OPTION_CREATE_DELETED : 0, 0, 0, (HREGKEY*)ppKey, NULL);
	//else if (!abDeletion)
	//	hRc = OpenKeyEx(ahPredefinedKey, apszKeyPath, 0, 0, (HREGKEY*)ppKey);
	//else
	//	hRc = ERROR_FILE_NOT_FOUND;
	
	
	// Обработать множественные ключи (путь)
	wchar_t szSubKey[MAX_PATH+1], szSubKeyLwr[MAX_PATH+1];
	int nSubKeyLen;
	DWORD nSubKeyCRC32;
	//HREGKEY hPredefined = NULL;
	//bool bFirstToken;
	FileRegItem* pKey = pParent;
	FileRegItem* p = NULL;

	// apszKeyPath - не трогаем, потом его нужно будет скопировать в pParent->pszFullPath
	LPCWSTR lpSubKey = apszKeyPath;
	BOOL bDeletion = FALSE, lbCreated = FALSE;
	LPCWSTR lpComment = NULL;

	if (lpdwDisposition)
		*lpdwDisposition = REG_OPENED_EXISTING_KEY;

	if (!apszKeyPath || !*apszKeyPath) {
		pKey = pParent;
		bDeletion = abDeletion;
		lpComment = apszComment;
		hRc = 0;
		goto wrap;
	}

	MCHKHEAP;

	_ASSERTE(anPathLen >= 0);
	// Пытаемся сравнить с последним открытым(созданным)
	if (!abDeletion && anPathLen > 0 && lpSubKey)
	{
		// Попробуем регистро-зависимо, это быстрее, и большинство скриптов регистр ключей сохраняют.
		//if (!mps_KeyPathLowerBuff || (DWORD)anPathLen >= mn_KeyPathLowerBuff) {
		//	mn_KeyPathLowerBuff = 16384 + anPathLen;
		//	mps_KeyPathLowerBuff = (wchar_t*)InternalMalloc(mn_KeyPathLowerBuff*2);
		//}
		
		LPCWSTR pszSubKeyPart = NULL;
		if ((p = FindExistKey(pKey, lpSubKey, anPathLen, &pszSubKeyPart)) != NULL)
		{
			pKey = p;
			lpSubKey = pszSubKeyPart;
		}
	}

	while (lpSubKey && *lpSubKey)
	{
		if (!GetSubkeyToken(lpSubKey, szSubKey, szSubKeyLwr, nSubKeyLen, nSubKeyCRC32))
		{
			_ASSERTE(FALSE);
			return ERROR_INVALID_NAME;
		}
		if (!lpSubKey)
		{
			// Если это последний токен
			bDeletion = abDeletion;
			lpComment = apszComment;
			//if (bInternalRegParsing)
			//{
			//	// Вообще-то это некорретно
			//	p = NULL; // При парсинге *.reg файла - строго создание
			//} else {
			//	p = TreeFindKey(pKey, szSubKey, szSubKeyLwr, nSubKeyLen, nSubKeyCRC32, bDeletion);
			//}

			if (bDeletion && abInternal)
			{
				p = TreeFindKey(pKey, szSubKey, szSubKeyLwr, nSubKeyLen, nSubKeyCRC32, FALSE);
				if (p)
					TreeDeleteChild(pKey, p);
			}
		}

		// Это может быть, если попадется двойной слеш в пути ключа. Это некорректно
		if (!*szSubKey) {
			_ASSERTE(*lpSubKey != 0);
			return ERROR_INVALID_NAME;
		}

		p = TreeFindKey(pKey, szSubKey, szSubKeyLwr, nSubKeyLen, nSubKeyCRC32, bDeletion);
		_ASSERTE(pKey != p);

		if (!p)
		{
			if (!abAllowCreate)
			{
				hRc = ERROR_FILE_NOT_FOUND;
				pKey = NULL;
				break;
			} else {
				p = TreeAddChild(pKey, REG__KEY, NULL/*ahPredefinedKey*/,
					szSubKey, szSubKeyLwr, nSubKeyLen, nSubKeyCRC32, NULL, 0,
					lpComment, anCommentLen, bDeletion);
				lbCreated = TRUE;
				if (lpdwDisposition)
					*lpdwDisposition = REG_CREATED_NEW_KEY;
				// Обновить CD родителя
				SYSTEMTIME st; GetSystemTime(&st);
				SystemTimeToFileTime(&st, (PFILETIME)&(pKey->ftCreationTime));
			}
		}
		pKey = p; //next token
		hRc = 0;
	}

	// Fin
wrap:
	if (!hRc && !pKey)
	{
		hRc = ERROR_FILE_NOT_FOUND;
	}
	else if (pKey)
	{
		if (abAllowCreate || abInternal)
		{
			MarkDirty(pKey);
			// Если ключ уже существующий - обновить комментарий (если есть) в строке с ключом
			if (!lbCreated) TreeSetComment(pKey, lpComment, anCommentLen);
		}
		if (!abDeletion)
		{
			#ifdef STORE_FULLPATH_ROOTONLY
			if (pParent->pParent == &TreeRoot && anPathLen)
			{
			#endif
				if (!pParent->pszFullPath || pParent->nFullPathSize <= anPathLen)
				{
					pParent->nFullPathSize = 16384 + anPathLen;
					pParent->pszFullPath = (wchar_t*)InternalMalloc(pParent->nFullPathSize*2);
				}
				wmemmove(pParent->pszFullPath, apszKeyPath, anPathLen);
				pParent->pszFullPath[anPathLen] = 0;
				pParent->nFullPathLen = anPathLen;
				pParent->pLastCreatedKey = pKey;
			#ifdef STORE_FULLPATH_ROOTONLY
			}
			#endif
		}
	}
	
	*ppKey = pKey;
	//nLastErr = hRc;
	return hRc;
}

FileRegItem* MFileReg::TreeAddChild(FileRegItem* apParent, REGTYPE anValueType, HKEY ahPredefinedKey,
		const wchar_t *apszName, const wchar_t *apszNameLwr, int anNameLen, DWORD anNameCRC32,
		const BYTE *apData, DWORD acbDataSize, 
		LPCWSTR lpComment /*= NULL*/, int anCommentLen /*= -1*/, BOOL abDeletion /*= FALSE*/)
{
#ifdef FORCE_BACKUP_CRC
	if (anNameCRC32 == FORCE_BACKUP_CRC) {
		anNameCRC32 = FORCE_BACKUP_CRC;
	}
#endif
	if (!apParent) {
		_ASSERTE(apParent != NULL);
		return NULL;
	}
	if (apszName && !*apszName) {
		apszName = NULL;
		apszNameLwr = NULL;
	}


	int nNameLen = 1; // именно 1 ('\0')
	DWORD cbNameSize = 0;
	DWORD cbCommentSize = 0;
	if (lpComment)
	{
		if (anCommentLen > 0)
		{
			cbCommentSize = ((anCommentLen+1)<<1);
		} else {
			_ASSERTE(anCommentLen >= 0);
			lpComment = NULL;
		}
 	}

	if (!apszName) {
		if (anValueType == REG__KEY) {
			_ASSERTE(anValueType != REG__KEY);
			InvalidOp();
			return NULL;
		}
		cbNameSize = 0;
		anNameLen = 0;
	} else {
		_ASSERTE(*apszName);
		if (!apszNameLwr)
		{
			anNameLen = lstrlenW(apszName);
			if (anNameLen >= 32767)
			{
				_ASSERTE(anNameLen < 32767);
				anNameLen = 32767;
			}
			wmemmove(ms_LowerBuff, apszName, anNameLen); ms_LowerBuff[anNameLen] = 0;
			CharLowerBuffW(ms_LowerBuff, anNameLen);
			apszNameLwr = ms_LowerBuff;
			anNameCRC32 = CalculateNameCRC32(ms_LowerBuff, anNameLen);
		} else {
			_ASSERTE(anNameLen > 0 || (anValueType == REG__COMMENT && anNameLen == 0));
		}
	
		_ASSERTE(anNameLen>0);
		nNameLen = anNameLen+1;
		_ASSERTE(nNameLen > 0);
		cbNameSize = nNameLen*2;

		//#ifdef _DEBUG
		//int nDbgCmp = lstrcmpiW(pszName, L"Item10");
		//if (nDbgCmp == 0) {
		//	// Что-то мой парсер зациклился на UserMenu\MainMenu\Item10\Item10...
		//	_ASSERTE(nDbgCmp != 0);
		//}
		//#endif

	}

	// Выделить память, инициализировать данные
	FileRegItem *p = (FileRegItem*)InternalMalloc(
			sizeof(FileRegItem)
			#ifdef KEEP_LOWERS
			+(cbNameSize<<1)
			#else
			+cbNameSize
			#endif
			+cbCommentSize
			+acbDataSize);
	wchar_t* pszNamePtr = (wchar_t*)(((LPBYTE)p)+sizeof(FileRegItem));
	wchar_t* pszCommPtr = (wchar_t*)(((LPBYTE)pszNamePtr)+cbNameSize);
	#ifdef KEEP_LOWERS
	wchar_t* pszNameLwrPtr = (wchar_t*)(((LPBYTE)pszCommPtr)+cbCommentSize);
	LPBYTE   pDataPtr = ((LPBYTE)pszNameLwrPtr)+cbNameSize;
	#else
	LPBYTE   pDataPtr = (LPBYTE)(((LPBYTE)pszCommPtr)+cbCommentSize);
	#endif

	// Сбросим только содержимое структуры (во избежание не инициализированных указателей)
	memset(p, 0, sizeof(FileRegItem));
	p->nMagic = FILEREGITEM_MAGIC; // Сразу ставим тэг


	MCHKHEAP;
	
	if (apParent)
	{
		// Создание - это на этапе парсинга *.reg, для ускорения просто берем из родителя
		p->ftCreationTime = apParent->ftCreationTime;
	} else {
		SYSTEMTIME st; GetSystemTime(&st);
		SystemTimeToFileTime(&st, (PFILETIME)&(p->ftCreationTime));
		apParent->ftCreationTime = p->ftCreationTime;
	}
	
	p->nValueType = anValueType;
	
	// Удаление ключей [-HKEY_...] или значений "ValueName"=-
	p->bDeletion = abDeletion;
	
	// Имя (если оно не умолчание значения ключа)
	if (apszName)
	{
		_ASSERTE(anNameLen >= 0);
		if (anNameLen)
			wmemmove(pszNamePtr, apszName, anNameLen);
		pszNamePtr[anNameLen] = 0;
		p->pszName = pszNamePtr;

		#ifdef KEEP_LOWERS
		wmemmove(pszNameLwrPtr, apszNameLwr, nNameLen);
		//CharLowerBuffW(pszNameLwrPtr, nNameLen);
		p->pszNameLwr = pszNameLwrPtr;
		#endif
		
		p->NameCRC32 = anNameCRC32;
		p->NameLen = anNameLen;
	}

	if (cbCommentSize)
	{
		wmemmove(pszCommPtr, lpComment, anCommentLen);
		pszCommPtr[anCommentLen] = 0;
		p->pszComment = pszCommPtr;
		p->cbCommSize = cbCommentSize;
	}

	MCHKHEAP;
	
	// память под данные выделять строго, не пытаться использовать ссылки на pData!
	if (acbDataSize && apData)
	{
		_ASSERTE(anValueType != REG__KEY);
		memmove(pDataPtr, apData, acbDataSize);
		p->pData = pDataPtr;
		p->cbDataSize = acbDataSize;
		p->nDataBufferSize = acbDataSize;
		p->pTempBuffer = NULL;
		// Увеличить максимальную длину значения в родителе
		if (apParent->nMaxValDataSize < acbDataSize)
			apParent->nMaxValDataSize = acbDataSize;
	}

	MCHKHEAP;
	
	// Вставить в дерево
	p->pParent = apParent;
	
	MCHKHEAP;

	// Увеличить максимальную длину имени в родителе
	// Установить hPredefined для ключей
	// Запомнить последний созданный в родителе
	// Увеличить количество ключей/значений в родителе
	// Вставить в дерево
	if (anValueType == REG__KEY) {
		// Увеличить максимальную длину имени в родителе
		if (apParent->nMaxKeyNameLen < (UINT)nNameLen)
			apParent->nMaxKeyNameLen = (UINT)nNameLen;
		p->hPredefined = ahPredefinedKey;
		// Запомнить последний созданный в родителе
		apParent->pCachedFindKey = p;
		// Увеличить количество ключей в родителе
		apParent->nKeyCount++;
		// Вставить в дерево
		if (apParent->pFirstKey)
		{
			_ASSERTE(apParent->pLastKey != NULL);
			_ASSERTE(apParent->pLastKey->pNextSibling == NULL);

			apParent->pLastKey->pNextSibling = p;
			p->pPrevSibling = apParent->pLastKey;
			apParent->pLastKey = p;

		} else {
			// Детей у родителя еще нет, это первый
			_ASSERTE(apParent->pLastKey == NULL);
			apParent->pFirstKey = p;
			apParent->pLastKey = p;
		}
		// Если много подключей - создаем кеш CRC
		#ifdef USE_CRC_CACHE
		if (!abDeletion && apParent->nKeyCount >= CRC_CACHE_FRINGE)
		{
			if (!apParent->pCrcKey)
			{
				apParent->pCrcKey = (FileRegCrcCache*)calloc(1,sizeof(FileRegCrcCache));
				apParent->pCrcKey->InitializeFrom(apParent->pFirstKey);
			}
			else if (apParent->pCrcKey->IsUsed == 1)
			{
				apParent->pCrcKey->AddNewItem(p, anNameCRC32);
			}
		}
		#endif
	} else {
		// Увеличить максимальную длину имени в родителе
		if (apParent->nMaxValNameLen < (UINT)nNameLen)
			apParent->nMaxValNameLen = (UINT)nNameLen;
		// Вставить в дерево
		if (apParent->pFirstVal)
		{
			_ASSERTE(apParent->pLastVal != NULL);
			_ASSERTE(apParent->pLastVal->pNextSibling == NULL);

			apParent->pLastVal->pNextSibling = p;
			p->pPrevSibling = apParent->pLastVal;
			apParent->pLastVal = p;

		} else {
			// Детей у родителя еще нет, это первый
			_ASSERTE(apParent->pLastVal == NULL);
			apParent->pFirstVal = p;
			apParent->pLastVal = p;
		}
		// Увеличить количество значений в родителе
		apParent->nValCommentCount++;
		if (anValueType != REG__COMMENT)
		{
			apParent->nValCount++;
			// Запомнить последний созданный в родителе
			apParent->pCachedFindVal = p;
			// Запомнить значение по умолчанию
			if (!apszName)
			{
				if (!abDeletion)
					apParent->pDefaultVal = p;
			} else if (!apParent->pDefaultVal) {
				if (lstrcmpiW(apszName, L"Description") == 0)
					apParent->pDefaultVal = p;
			}
		}
		if (apszName)
		{
			// Если стало слишком много значений - создаем кеш CRC
			#ifdef USE_CRC_CACHE_VAL
			if (apParent->nValCount >= CRC_CACHE_FRINGE)
			{
				if (!apParent->pCrcVal)
				{
					apParent->pCrcVal = (FileRegCrcCache*)calloc(1,sizeof(FileRegCrcCache));
					apParent->pCrcVal->InitializeFrom(apParent->pFirstVal);
				}
				else if (apParent->pCrcVal->IsUsed == 1)
				{
					apParent->pCrcVal->AddNewItem(p, anNameCRC32);
				}
			}
			#endif
		}
	}
	
	return p;
}

FileRegItem* MFileReg::TreeFindPredefinedKey(FileRegItem* pParent, HKEY ahKey, BOOL bKeyDeleted)
{
	_ASSERTE(pParent == &TreeRoot);
	if (!ahKey) {
		_ASSERTE(ahKey != NULL);
		return NULL;
	}
	
	FileRegItem* p = pParent->pFirstKey;
	while (p)
	{
		if (p->hPredefined == ahKey)
		{
			_ASSERTE(p->nValueType == REG__KEY);
			_ASSERTE(p->pszName!=NULL);
			//pParent->pCachedFindKey = p; -- наверное не будем кеш трогать, тут и так должно быть быстро
			return p;
		}
		p = p->pNextSibling;
	}

	return NULL;
}

FileRegItem* MFileReg::TreeFindKey(FileRegItem* pParent, const wchar_t *pszName, 
		LPCWSTR apszNameLwr, int anNameLen, DWORD anNameCRC32,
		BOOL bKeyDeleted)
{
	if (!pParent || !pParent->pFirstKey || !pszName || !*pszName) {
		_ASSERTE(pParent);
		_ASSERTE(pszName && *pszName);
		return NULL;
	}

	DWORD cchCmpCount = 0;
	int nLen = 0;
	if (!apszNameLwr)
	{
		nLen = lstrlenW(pszName);
		if (nLen >= 32767) {
			_ASSERTE(nLen < 32767);
			nLen = 32767;
		}
		wmemmove(ms_LowerBuff, pszName, nLen); ms_LowerBuff[nLen] = 0;
		CharLowerBuffW(ms_LowerBuff, nLen);
		cchCmpCount = (nLen+1);
		apszNameLwr = ms_LowerBuff;
		anNameCRC32 = CalculateNameCRC32(apszNameLwr, nLen);
	} else {
		_ASSERTE(anNameLen != -1);
		nLen = anNameLen;
		cchCmpCount = (nLen+1);
	}

	FileRegItem* pc = pParent->pCachedFindKey;
	if (pc == pParent) {
		_ASSERTE(pParent->pCachedFindKey != pParent);
		pParent->pCachedFindKey = pc = NULL;
	}
	if (pc)
	{
		if (pc->NameCRC32 == anNameCRC32
			//&& pc->pszNameCmp
			&& pc->bDeletion == bKeyDeleted
			&& pc->NameLen == nLen)
		{
			if (CompareName(pc, apszNameLwr, cchCmpCount) == 0)
			{
				_ASSERTE(pParent->pCachedFindKey->nValueType == REG__KEY);
				return pc;
			}
		}
		FileRegItem* pc1 = pc->pNextSibling;
		if (pc1 == pParent) {
			_ASSERTE(pParent->pCachedFindKey != pParent);
			pc1 = NULL;
		}
		if (pc1
			&& pc1->NameCRC32 == anNameCRC32
			//&& pc1->pszNameCmp
			&& pc1->bDeletion == bKeyDeleted
			&& pc1->NameLen == nLen)
		{
			if (CompareName(pc1, apszNameLwr, cchCmpCount) == 0)
			{
				_ASSERTE(pParent->pCachedFindKey->nValueType == REG__KEY);
				pParent->pCachedFindKey = pc1;
				return pc1;
			}
		}
	}

	FileRegItem* pi = pParent->pKeyIndex;
	if (pi == pParent) {
		_ASSERTE(pParent->pKeyIndex != pParent);
		pParent->pKeyIndex = pi = NULL;
	}
	if (pi
		&& pi->NameCRC32 == anNameCRC32
		//&& pi != pParent->pCachedFindKey
		//&& pi->pszNameCmp
		&& pi->bDeletion == bKeyDeleted
		&& pi->NameLen == nLen)
	{
		if (CompareName(pi, apszNameLwr, cchCmpCount) == 0)
		{
			_ASSERTE(pParent->pKeyIndex->nValueType == REG__KEY);
			return pi;
		}
	}

	#ifdef USE_CRC_CACHE
	if (pParent->pCrcKey && pParent->pCrcKey->IsUsed == 1 && !bKeyDeleted)
	{
		FileRegItem* p = pParent->pCrcKey->FindItem(pszName, anNameCRC32);
		if (p)
			pParent->pCachedFindKey = p;
		return p;
	}
	else
	#endif
	{
		FileRegItem* p = pParent->pFirstKey;
		while (p)
		{
			_ASSERTE(p->nValueType == REG__KEY);
			// Убрал! Это дикая нагрузка. Проверять условие откидывания 2 элементов 
			// при наличии десятков тысяч ключей неразумно - очень большие затраты!
			//// Если указатель совпадает с кешировнными или индексированным - он уже проверен!
			//if (p != pc && p != pi) 
			//{
				if (p->NameCRC32 == anNameCRC32
					//&& p->pszNameCmp 
					&& p->bDeletion == bKeyDeleted
					&& p->NameLen == nLen)
				{
					if (CompareName(p, apszNameLwr, cchCmpCount) == 0)
					{
						pParent->pCachedFindKey = p;
						return p;
					}
				}
				_ASSERTE(p->pszName!=NULL);
			//}
			p = p->pNextSibling;
		}
	}

	return NULL;
}

// Для @ (значения по умолчанию) pszName == NULL
FileRegItem* MFileReg::TreeFindValue(FileRegItem* pParent, const wchar_t *pszName,
		LPCWSTR apszNameLwr /*= NULL*/, int anNameLen /*= -1*/, DWORD anNameCRC32 /*= 0*/)
{
	if (!pParent || !pParent->pFirstVal)
	{
		_ASSERTE(pParent);
		return NULL;
	}
	if (pszName && *pszName == 0)
	{
		//_ASSERTE(pszName == NULL); -- не ругаться, пустая строка приходит из EditDescription
		pszName = NULL;
		apszNameLwr = NULL;
	}

	//wchar_t sLower[32768];
	DWORD cchCmpCount = 0;
	int nLen = 0;
	if (pszName != NULL)
	{
		if (!apszNameLwr)
		{
			nLen = lstrlenW(pszName);
			if (nLen >= 32767) {
				_ASSERTE(nLen < 32767);
				nLen = 32767;
			}
			wmemmove(ms_LowerBuff, pszName, nLen); ms_LowerBuff[nLen] = 0;
			CharLowerBuffW(ms_LowerBuff, nLen);
			cchCmpCount = (nLen+1);
			apszNameLwr = ms_LowerBuff;
			anNameCRC32 = CalculateNameCRC32(ms_LowerBuff, nLen);
		} else {
			_ASSERTE(anNameLen != -1);
			nLen = anNameLen;
			cchCmpCount = (nLen+1);
		}
	}
	
	//TODO: Доделать сравнение по CRC

	FileRegItem* pc = pParent->pCachedFindVal;
	FileRegItem* pi = pParent->pValIndex;

	if (pszName == NULL)
	{
		if (pc == pParent) {
			_ASSERTE(pParent->pCachedFindVal != pParent);
			pParent->pCachedFindVal = pc = NULL;
		}
		if (pc
			&& !pc->pszName)
		{
			_ASSERTE(pParent->pCachedFindVal->nValueType != REG__KEY);
			return pc;
		}
		
		if (pi == pParent) {
			_ASSERTE(pParent->pValIndex != pParent);
			pParent->pValIndex = pi = NULL;
		}
		if (pi && pi != pc
			&& !pi->pszName)
		{
			_ASSERTE(pParent->pValIndex->nValueType != REG__KEY);
			return pi;
		}
	} else {
		if (pc == pParent) {
			_ASSERTE(pParent->pCachedFindVal != pParent);
			pParent->pCachedFindVal = pc = NULL;
		}
		if (pc
			&& pc->NameCRC32 == anNameCRC32
			&& pc->NameLen == nLen
			//&& pc->pszNameCmp
			)
		{
			if (CompareName(pc, apszNameLwr, cchCmpCount) == 0)
			{
				_ASSERTE(pParent->pCachedFindVal->nValueType != REG__KEY);
				return pc;
			}
		}
		
		if (pi == pParent) {
			_ASSERTE(pParent->pValIndex != pParent);
			pParent->pValIndex = pi = NULL;
		}
		if (pi /*&& pi != pc*/ // тоже лишние траты
			&& pi->NameCRC32 == anNameCRC32
			&& pi->NameLen == nLen
			//&& pi->pszNameCmp
			)
		{
			if (CompareName(pi, apszNameLwr, cchCmpCount) == 0)
			{
				_ASSERTE(pParent->pValIndex->nValueType != REG__KEY);
				return pi;
			}
		}
	}

	FileRegItem* p = pParent->pFirstVal;
	if (pszName == NULL)
	{
		while (p)
		{
			_ASSERTE(p->nValueType != REG__KEY);
			if (p->pszName == NULL)
			{
				pParent->pCachedFindVal = p;
				return p;
			}
			p = p->pNextSibling;
		}
	} else {
		_ASSERTE(cchCmpCount);
		#ifdef USE_CRC_CACHE_VAL
		if (pParent->pCrcVal && pParent->pCrcVal->IsUsed == 1)
		{
			FileRegItem* p = pParent->pCrcVal->FindItem(pszName, anNameCRC32);
			if (p)
				pParent->pCachedFindVal = p;
			return p;
		}
		else
		#endif
		{
			while (p)
			{
				_ASSERTE(p->nValueType != REG__KEY);
				// Если указатель совпадает с кешировнными или индексированным - он уже проверен!
				if (p->NameCRC32 == anNameCRC32
					//&& p->pszNameCmp
					&& p->NameLen == nLen)
					//&& p != pc
					//&& p != pi)
				{
					//if (p->NameLen == nLen
					//	&& p->NameCRC32 == anNameCRC32)
					//{
						if (CompareName(p, apszNameLwr, cchCmpCount) == 0)
						{
							pParent->pCachedFindVal = p;
							return p;
						}
					//}
				}
				p = p->pNextSibling;
			}
		}
	}

	return NULL;
}

bool MFileReg::CompareFileRegItem(FileRegItem* pItem, DWORD nValueType, const wchar_t *pszName)
{
	if (!pItem) {
		_ASSERTE(pItem!=NULL);
		return false;
	}
	if ((pItem->nValueType == REG__KEY) != (nValueType == REG__KEY))
		return false;

	if (pszName && *pszName == 0) {
		_ASSERTE(pszName == NULL);
		pszName = NULL;
	}
	
	if (!pItem->pszName && !pszName)
		return true;

	if (!pszName || !pItem->pszName)
		return false;

	if (lstrcmpiW(pszName, pItem->pszName) == 0)
		return true;
		
	return false;
}

void MFileReg::TreeDeleteChild(FileRegItem* pParent, FileRegItem* pChild)
{
	_ASSERTE(pChild);

	MCHKHEAP;

	if (pParent == NULL)
	{
		if (pChild != &TreeRoot)
		{
			_ASSERTE(pParent);
			_ASSERTE(pParent == pChild->pParent);
		}
	}
	else
	{	
		_ASSERTE(pParent);
		_ASSERTE(pParent == pChild->pParent);
		
		pParent = pChild->pParent;

		// удалить элемент из кеша CRC
		#ifdef USE_CRC_CACHE
		if (pChild->nValueType == REG__KEY)
		{
			if (pParent->pCrcKey)
				pParent->pCrcKey->DeleteItem(pChild);
				//pParent->pCrcKey->IsUsed = 2;
		}
		#ifdef USE_CRC_CACHE_VAL
		else
		{
			if (pParent->pCrcVal)
				pParent->pCrcVal->DeleteItem(pChild);
				//pParent->pCrcVal->IsUsed = 2;
		}
		#endif
		#endif
		
		// Key	
		if (pParent->pFirstKey == pChild)
		{
			_ASSERTE(pChild->pPrevSibling == NULL);
			pParent->pFirstKey = pChild->pNextSibling;
		}
		if (pParent->pLastKey == pChild)
		{
			_ASSERTE(pChild->pNextSibling == NULL);
			pParent->pLastKey = pChild->pPrevSibling;
		}
		if (pParent->pCachedFindKey == pChild)
		{
			pParent->pCachedFindKey = NULL;
		}
		// Value
		if (pParent->pFirstVal == pChild)
		{
			_ASSERTE(pChild->pPrevSibling == NULL);
			pParent->pFirstVal = pChild->pNextSibling;
		}
		if (pParent->pLastVal == pChild)
		{
			_ASSERTE(pChild->pNextSibling == NULL);
			pParent->pLastVal = pChild->pPrevSibling;
		}
		if (pParent->pDefaultVal == pChild)
		{
			pParent->pDefaultVal = NULL;
		}
		if (pParent->pCachedFindVal == pChild)
		{
			pParent->pCachedFindVal = NULL;
		}
		
		// декремент количества ключей/значений в родителе
		if (pChild->nValueType == REG__KEY)
		{
			_ASSERTE(pParent->nKeyCount > 0);
			if (pParent->nKeyCount) pParent->nKeyCount--;
		}
		else
		{	
			if (pChild->nValueType != REG__COMMENT)
			{
				_ASSERTE(pParent->nValCount > 0);
				if (pParent->nValCount) pParent->nValCount--;
			}
			_ASSERTE(pParent->nValCommentCount > 0);
			if (pParent->nValCommentCount) pParent->nValCommentCount--;
		}

		// При удалении - сбрасываем последние полученные по индексу	
		if (pChild->nValueType == REG__KEY)
		{
			pParent->dwKeyIndex = 0; pParent->pKeyIndex = NULL;
		}
		else
		{
			pParent->dwValIndex = 0; pParent->pValIndex = NULL; pParent->bValIndexComment = FALSE;
		}
		
		//// И обновляем дату
		//SYSTEMTIME st; GetSystemTime(&st);
		//SystemTimeToFileTime(&st, (PFILETIME)&(pParent->ftCreationTime));
	}
	// С родителем закончили
	
	
	// Теперь, собственно, элемент (его одноуровневые)
	if (pChild->pNextSibling) {
		pChild->pNextSibling->pPrevSibling = pChild->pPrevSibling;
	}
	if (pChild->pPrevSibling) {
		pChild->pPrevSibling->pNextSibling = pChild->pNextSibling;
	}

	MCHKHEAP;

	// И дочерние элементы (Keys)
	while (pChild->pFirstKey) {
		#ifdef _DEBUG
		FileRegItem* p = pChild->pFirstKey;
		#endif
		TreeDeleteChild(pChild, pChild->pFirstKey);
		#ifdef _DEBUG
		_ASSERTE(p!=pChild->pFirstKey);
		#endif
	}
	// Values
	while (pChild->pFirstVal) {
		#ifdef _DEBUG
		FileRegItem* p = pChild->pFirstVal;
		#endif
		TreeDeleteChild(pChild, pChild->pFirstVal);
		#ifdef _DEBUG
		_ASSERTE(p!=pChild->pFirstVal);
		#endif
	}

	MCHKHEAP;

	// Final release memory - НЕ ТРЕБУЕТСЯ, это InternalMalloc
	//SafeFree(pChild->pszTempBuffer);
	//SafeFree(pChild->pTempBuffer);

	MCHKHEAP;
	
	// Это может быть наш корень, проверим
	if (pChild != &TreeRoot)
	{
		// Сброс кеша для полных ключей
		FileRegItem* p = TreeRoot.pFirstKey;
		while (p) {
			if (p->pLastCreatedKey == pChild)
			{
				p->pLastCreatedKey = NULL;
				if (p->pszFullPath) *p->pszFullPath = 0;
				p->nFullPathLen = 0;
			}
			p = p->pNextSibling;
		}

		pChild->nMagic = 0;

		//SafeFree(pChild); -- НЕ ТРЕБУЕТСЯ, это InternalMalloc
		MCHKHEAP;
	} else {
		memset(&TreeRoot, 0, sizeof(TreeRoot));
		TreeRoot.nMagic = FILEREGITEM_MAGIC;
		MCHKHEAP;
	}
}

FileRegItem* MFileReg::ItemFromHkey(HKEY hKey)
{
	if (hKey == NULL)
		return (&TreeRoot);

	FileRegItem* pParent = NULL;

	MCHKHEAP;

	if (IsPredefined(hKey))
	{
		//LPCWSTR pszName = HKeyToStringKey(hKey);
		//if (!pszName)
		//	return NULL;
		//MCHKHEAP;
		pParent = TreeFindPredefinedKey(&TreeRoot, hKey, FALSE);
		_ASSERTE(pParent != &TreeRoot);
		MCHKHEAP;
		if (!pParent /*&& IsPredefined(hKey)*/)
		{
			MCHKHEAP;
			LPCWSTR pszName = HKeyToStringKey(hKey);
			if (!pszName)
			{
				_ASSERTE(pszName != NULL);
				return NULL;
			}
			pParent = TreeAddChild(&TreeRoot, REG__KEY, hKey, pszName, NULL, -1, 0, NULL/*apData*/, 0);
			MCHKHEAP;
		}
		
	}
	else
	{

		pParent = (FileRegItem*)hKey;
		if (pParent != &TreeRoot && pParent->nValueType!=REG__KEY)
		{
			_ASSERTE(pParent->nValueType==REG__KEY);
			InvalidOp();
			pParent = NULL;
		}
	}

	return pParent;
}

// Поддерживаются смешанные переводы "\r", "\n", "\r\n"
__inline void SkipLine(LPWSTR& pszLine, LPCWSTR pszEnd, DWORD& nLine, LPCWSTR* ppszLineEnd = NULL)
{
	if (pszLine >= pszEnd) return;
	wchar_t* pszNewLine = wcspbrk(pszLine, L"\r\n");
	if (!pszNewLine)
	{
		pszLine = (wchar_t*)pszEnd+1;
	}
	else
	{
		if (pszNewLine[0] == L'\r' && pszNewLine[1] == L'\n')
			pszLine = pszNewLine + 2;
		else
			pszLine = pszNewLine + 1;
		nLine++;
	}

	//wchar_t ch = 0;

	//if (ppszLineEnd)
	//	*ppszLineEnd = pszEnd-1;
	//
	//while (pszLine < pszEnd)
	//{
	//	ch = *(pszLine++);
	//	if (ch == L'\r' || ch == L'\n')
	//	{
	//		nLine++;
	//		if (ppszLineEnd)
	//			*ppszLineEnd = pszLine-1;
	//		break; // нашли конец строки
	//	}
	//}
	//// нужно перейти на следующую строку
	//if (pszLine < pszEnd)
	//{
	//	if (ch == L'\r' && *pszLine == L'\n')
	//	{
	//		pszLine++;
	//	}
	//}
}

__inline void SkipSpaces(LPWSTR& pszLine, LPCWSTR pszEnd)
{
	wchar_t ch;
	while (pszLine < pszEnd)
	{
		ch = *pszLine;
		if (ch != L'\t' && ch != L' ' && ch != 0x0A/*&nbsp;*/ && ch != 0)
			break; // нашли первый не пробельный символ
		pszLine++;
	}
}

__inline BOOL DeEscape(wchar_t*& pszSrc, wchar_t*& pszDst)
{
	wchar_t wc = *(++pszSrc);
	switch (wc)
	{
	case L'r':
		*pszDst = L'\r'; break;
	case L'n':
		*pszDst = L'\n'; break;
	case L't': // в принципе, табуляция не экранируется, но на всякий случай - проверяем
		*pszDst = L'\t'; break;
	case L'0':
		*pszDst = 0; break;
	case L'"':
		*pszDst = L'\"'; break;
	case L'\\':
		*pszDst = L'\\'; break;
	default:
		return FALSE;
	}
	pszDst++; pszSrc++;
	return TRUE;
}

BOOL MFileReg::GetKeyFromLine(
		LPWSTR& pszLine, LPWSTR& pszCol, LPWSTR pszEnd, HKEY& hKeyRoot, LPWSTR& pszKeyName, LPWSTR& pszKeyEnd,
		BOOL& bDeletion, DWORD& nLine, LPWSTR& pszComment, int& nCommentLen)
{
	_ASSERTE(*pszCol == L'[');
	pszCol++;
	pszKeyEnd = NULL;
	pszComment = NULL;

	LPWSTR pszSlash = NULL, pszFind = NULL;
	if (*pszCol == L'-') {
		bDeletion = TRUE; pszCol++;
	} else {
		bDeletion = FALSE;
	}
	pszKeyName = pszCol;

	// Кроме всего прочего, нужно учесть, что ключик может быть последним
	// то есть после него перевода строки вообще нет (последняя строка файла)
	pszFind = wcspbrk(pszCol, L"\\\r\n");
	if (!pszFind)
	{
		pszFind = pszCol+lstrlenW(pszCol);
	}
	else if (*pszFind == L'\\')
	{
		pszSlash = pszFind;
		pszFind = wcspbrk(pszFind+1, L"\r\n");
		if (!pszFind) // может после ключа вообще нет перевода строки (последняя строка файла)
			pszFind = pszCol+lstrlenW(pszCol);
	}

	//! Засада
	//[HREGKEY\SYSTEM\ControlSet001\Control\MediaProperties\PrivateProperties\Midi\Ports\Creative [Emulated]] ; 5555
	//Закрывающая скобка может быть НЕ последней,
	//и даже после последней скобки - может идти мусор.
	//Или еще хуже
	//[HREGKEY\SYSTEM\ControlSet001\Control\MediaProperties\PrivateProperties\Midi\Ports\Creative [Emulated]] ; 5555] ; и только теперь - комментарий

	pszKeyEnd = pszFind - 1;
	while (*pszKeyEnd != L']' && pszKeyEnd > pszKeyName)
		pszKeyEnd--;

	// Закончилась строка БЕЗ закрывающей ']'
	if (*pszKeyEnd != L']')
	{
		pszKeyName = NULL;
		return FALSE;
	}


	size_t nKeyPathLen = pszKeyEnd - pszKeyName;
	if (nKeyPathLen != (DWORD)nKeyPathLen)
	{
		pszKeyName = NULL;
		return FALSE;
	}

	// Обрубить ASCIIZ
	_ASSERTE(*pszKeyEnd == L']');
	*pszKeyEnd = 0;
	if ((pszKeyEnd+1) < pszFind)
	{
		pszComment = pszKeyEnd + 1;
		nCommentLen = pszFind - pszComment;
	}

	if (pszSlash) {
		*pszSlash = 0;
	} else {
		pszSlash = pszKeyEnd-1;
	}
	if (!StringKeyToHKey(pszKeyName, &hKeyRoot))
	{
		return FALSE;
		//// 1 - continue, 0 - break to editor, -1 - stop
		//int nCont = -1;
		//if (!bSilent)
		//	nCont = RegFormatFailed(GetShowFilePathName(), nLine, 0);
		////TODO: Editor
		//if (nCont != 1)
		//	return FALSE;
	}
	
	pszKeyName = pszSlash+1;
	pszLine = pszKeyEnd+1;
	while (*(pszKeyEnd-1) == L'\\') {
		*(--pszKeyEnd) = 0;
	}

	MCHKHEAP;
	
	//SkipLine(pszLine, pszEnd, nLine);
	if (pszFind[0] == L'\r' && pszFind[1] == L'\n')
		pszLine = pszFind + 2;
	else
		pszLine = pszFind + 1;
	nLine++;
	return TRUE;
}

__inline BOOL GetEscapedString(wchar_t*& pszCol, wchar_t*& pszValBegin, wchar_t*& pszValEnd, DWORD& nLine)
{
	if (*pszCol != L'"') {
		_ASSERTE(*pszCol == L'"');
		return FALSE;
	}
	pszValBegin = ++pszCol;

	// Одна строка (в regedit.exe переводы строк не экранируются, а реально попадают в файл)
	wchar_t* pszSrc = pszValBegin;
	wchar_t* pszDst = pszValBegin;
	LPCWSTR pszBreakChars = L"\"\\\n\r";
	while (true) // (pszSrc < pszEnd) -- т.к. строка (считанные данные целиком) у нас однозначно завершается (wchar_t=0) то это безопасно
	{
		//WARNING! Похоже, что wcspbrk начинает искать с (pszSrc+1)!
		wchar_t *pszBreak = wcspbrk(pszSrc, pszBreakChars);
		if (!pszBreak)
			return FALSE;
		if (pszBreak != pszSrc)
		{
			if (pszSrc != pszDst)
				wmemmove(pszDst, pszSrc, (pszBreak-pszSrc));
			pszDst += (pszBreak - pszSrc);
			pszSrc = pszBreak;
		}

		wchar_t wc = *pszBreak;
		switch (wc)
		{
		case L'\"':
			goto end_name_loop1;
		case L'\\':
			// Убираем экранирование
			if (!DeEscape(pszSrc, pszDst))
				return FALSE;
			continue;
		case L'\n':
			nLine++;
			*pszDst = L'\n';
			break;
		case L'\r':
			if (pszSrc[1] == L'\n')
			{
				*pszDst = L'\r';
				//wc = L'\n';
				pszDst++; pszSrc++;
				*pszDst = L'\n';
			} else {
				*pszDst = L'\r';
				nLine++;
			}
			break;
		case 0:
			return FALSE;
		}
		pszDst++; pszSrc++;
	}

	end_name_loop1:

	//TODO: Нужно проверить, что после '"' нет мусора (допустимы только пробелы и перевод строки!)
	//if (pszSrc < pszEnd) { }

	*pszDst = 0; // Завершаем строку
	pszValEnd = pszDst;
	pszCol = pszSrc;
	return TRUE;
}

BOOL MFileReg::GetValueFromLine(
		LPWSTR& pszLine, LPWSTR& pszCol, LPWSTR pszEnd,
		LPWSTR& pszValName, LPWSTR pszValNameLwr/*[32768]*/, int& nNameLen, DWORD& nNameCRC32,
		BOOL& bDeletion, DWORD& nLine,
		REGTYPE& nValueType, LPBYTE& ptrValueData, DWORD& nValueDataLen,
		LPWSTR& pszComment, int& nCommentLen)
{
	_ASSERTE(*pszCol == L'\"' || *pszCol == L'@');
	wchar_t *pszNameEnd = NULL, *pszValBegin = NULL;
	//LPCWSTR pszLineEnd = NULL;
	pszComment = NULL;
	
	// Получить имя (до следующей "), в имени возможно экранирование
	if (*pszCol == L'@')
	{
		// Значение по умолчанию
		*pszCol = 0;
		pszValName = NULL;
		pszValNameLwr[0] = 0; nNameLen = 0; nNameCRC32 = 0;
		pszNameEnd = pszCol;
		pszCol++;
	} else {
		if (!GetEscapedString(pszCol, pszValName, pszNameEnd, nLine))
			return FALSE;
		size_t nCchCount = pszNameEnd - pszValName;
		if (nCchCount >= 32767) {
			return FALSE;
		}
		nNameLen = (int)nCchCount;
		wmemmove(pszValNameLwr, pszValName, nNameLen);
		pszValNameLwr[nNameLen] = 0;
		CharLowerBuffW(pszValNameLwr, nNameLen);
		nNameCRC32 = CalculateNameCRC32(pszValNameLwr, nNameLen);
		pszCol++;
	}
	if (!pszNameEnd) {
		InvalidOp();
		return FALSE;
	}

	ptrValueData = NULL;
	nValueDataLen = 0;
	
	bDeletion = FALSE;

	*pszNameEnd = 0; //pszCol = pszNameEnd+1;
	SkipSpaces(pszCol, pszEnd);
	if (*pszCol != L'=')
	{
		return FALSE;
	}
	else
	{
		pszCol++; SkipSpaces(pszCol, pszEnd);
		if (*pszCol == L'-')
		{
			nValueType = REG__DELETE;
			pszComment = pszCol+1;
			bDeletion = TRUE;
		} else {
			//TODO: пропустить spaces до '=', пропустить spaces. Если '=' нет - ошибка
			pszValBegin = pszCol;

			// Получаем значение REG_SZ
			if (*pszCol == L'"')
			{
				nValueType = REG_SZ;
				wchar_t* pszValEnd = NULL;
				if (!GetEscapedString(pszCol, pszValBegin, pszValEnd, nLine))
					return FALSE;

				////TODO: Нужно проверить, что после '"' нет мусора (допустимы только пробелы и перевод строки!)
				////if (pszSrc < pszEnd) { }

				//*pszDst = 0; // Завершаем строку
				size_t nSize = (pszValEnd - pszValBegin + 1) * 2;
				if (nSize != (DWORD)nSize) {
					return FALSE;
				}
				ptrValueData = (LPBYTE)pszValBegin;
				nValueDataLen = (DWORD)nSize;
				pszLine = pszComment = pszCol+1;
				MCHKHEAP;
				//SkipLine(pszLine, pszEnd, nLine);
				goto success;
			}
			else if (pszCol[0]==L'd' && pszCol[1]==L'w' && pszCol[2]==L'o' && pszCol[3]==L'r' && pszCol[4]==L'd')
			{
				// hex-число (4 байт)
				// += "dword:"
				ptrValueData = (LPBYTE)pszCol;
				pszCol += 6;
				DWORD nValue = 0; bool bValue = false;
				WORD wc;
				//while ((wc = *(pszCol)) != L'\r' && wc != L'\n')
				while (pszCol < pszEnd)
				{
					wc = *pszCol;
					switch (wc)
					{
					case L'0': case L'1': case L'2': case L'3': case L'4':
					case L'5': case L'6': case L'7': case L'8': case L'9':
						nValue = (nValue << 4) | (wc - 0x30/*(WORD)L'0'*/);
						pszCol++; bValue = true;
						continue;
					case L'A': case L'B': case L'C': case L'D': case L'E': case L'F': 
						nValue = (nValue << 4) | (wc - 0x37 /*((WORD)L'A'+10)*/);
						pszCol++; bValue = true;
						continue;
					case L'a': case L'b': case L'c': case L'd': case L'e': case L'f': 
						nValue = (nValue << 4) | (wc - 0x57 /*((WORD)L'a'+10)*/);
						pszCol++; bValue = true;
						continue;
					}
					SkipSpaces(pszCol, pszEnd);
					wc = *pszCol;
					if (wc == L'\r' || wc == L'\n' || !wc || wc == L';')
						break; // OK
					return FALSE;
				}
				if (!bValue)
					return FALSE;
				// 4 байта. даже размер префикса не перекроют.
				*((LPDWORD)ptrValueData) = nValue;
				nValueType = REG_DWORD;
				nValueDataLen = 4;
				pszLine = pszComment = pszCol;
				MCHKHEAP;
				//SkipLine(pszLine, pszEnd, nLine);
				goto success;
			}
			//else if (pszCol[0]==L'q' && pszCol[1]==L'w' && pszCol[2]==L'o' && pszCol[3]==L'r' && pszCol[4]==L'd')
			//{
			//	// hex-число (8 байт)
			//	// += "qword:"
			//	ptrValueData = (LPBYTE)(pszCol-1); // чтобы памяти гарантированно хватило - перекроем знак '='
			//	pszCol += 6;
			//	unsigned __int64 nValue = 0; bool bValue = false;
			//	WORD wc;
			//	//while ((wc = *(pszCol)) != L'\r' && wc != L'\n')
			//	while (pszCol < pszEnd)
			//	{
			//		wc = *pszCol;
			//		switch (wc)
			//		{
			//		case L'0': case L'1': case L'2': case L'3': case L'4':
			//		case L'5': case L'6': case L'7': case L'8': case L'9':
			//			nValue = (nValue << 4) | (wc - 0x30/*(WORD)L'0'*/);
			//			pszCol++; bValue = true;
			//			continue;
			//		case L'A': case L'B': case L'C': case L'D': case L'E': case L'F': 
			//			nValue = (nValue << 4) | (wc - 0x37 /*((WORD)L'A'+10)*/);
			//			pszCol++; bValue = true;
			//			continue;
			//		case L'a': case L'b': case L'c': case L'd': case L'e': case L'f': 
			//			nValue = (nValue << 4) | (wc - 0x57 /*((WORD)L'a'+10)*/);
			//			pszCol++; bValue = true;
			//			continue;
			//		}
			//		SkipSpaces(pszCol, pszEnd);
			//		wc = *pszCol;
			//		if (wc == L'\r' || wc == L'\n' || !wc || wc == L';')
			//			break; // OK
			//		return FALSE;
			//	}
			//	if (!bValue)
			//		return FALSE;
			//	// 8 байт. по размеру это (префикс=6)+(перед ним знак '=')+(как минимум один символ bValue==true).
			//	*((unsigned __int64*)ptrValueData) = nValue;
			//	nValueType = REG_QWORD;
			//	nValueDataLen = 8;
			//	pszLine = pszComment = pszCol;
			//	MCHKHEAP;
			//	//SkipLine(pszLine, pszEnd, nLine);
			//	goto success;
			//}
			else if (pszCol[0]==L'h' && pszCol[1]==L'e' && pszCol[2]==L'x')
			{
				ptrValueData = (LPBYTE)pszCol;
				pszCol += 3;
				if (*pszCol == L':') {
					nValueType = REG_BINARY;
					pszCol++;
				} else if (*pszCol == L'(') {
					pszCol++;
					wchar_t* pszScanEnd = NULL;
					DWORD nUnHex = wcstoul(pszCol, &pszScanEnd, 16);
					if (!pszScanEnd)
						return FALSE;
					if (nUnHex == REG__KEY)
						return FALSE;
					pszCol = pszScanEnd;
					if (pszCol[0] != L')' || pszCol[1] != L':')
						return FALSE;
					nValueType = nUnHex;
					pszCol += 2;
				} else {
					return FALSE;
				}
				// Поехали конвертить простыню hex'ов
				_ASSERTE(nValueType != REG__KEY); // Epson создал значение типа 0x334
				LPBYTE ptrDst = ptrValueData;
				WORD nValue, wc; size_t nSize;
				while (pszCol < pszEnd)
				{
					wc = *(pszCol++);
					switch (wc)
					{
					case L'0': case L'1': case L'2': case L'3': case L'4':
					case L'5': case L'6': case L'7': case L'8': case L'9':
						nValue = (wc - 0x30/*(WORD)L'0'*/);
						wc = *(pszCol++);
						switch (wc)
						{
						case L'0': case L'1': case L'2': case L'3': case L'4':
						case L'5': case L'6': case L'7': case L'8': case L'9':
							nValue = (nValue << 4) | (wc - 0x30/*(WORD)L'0'*/);
							break;
						case L'A': case L'B': case L'C': case L'D': case L'E': case L'F': 
							nValue = (nValue << 4) | (wc - 0x37 /*((WORD)L'A'+10)*/);
							break;
						case L'a': case L'b': case L'c': case L'd': case L'e': case L'f': 
							nValue = (nValue << 4) | (wc - 0x57 /*((WORD)L'a'+10)*/);
							break;
						default:
							return FALSE;
						}
						*(ptrDst++) = (BYTE)nValue;
						continue;
					case L'A': case L'B': case L'C': case L'D': case L'E': case L'F': 
						nValue = (wc - 0x37 /*((WORD)L'A'+10)*/);
						wc = *(pszCol++);
						switch (wc)
						{
						case L'0': case L'1': case L'2': case L'3': case L'4':
						case L'5': case L'6': case L'7': case L'8': case L'9':
							nValue = (nValue << 4) | (wc - 0x30/*(WORD)L'0'*/);
							break;
						case L'A': case L'B': case L'C': case L'D': case L'E': case L'F': 
							nValue = (nValue << 4) | (wc - 0x37 /*((WORD)L'A'+10)*/);
							break;
						case L'a': case L'b': case L'c': case L'd': case L'e': case L'f': 
							nValue = (nValue << 4) | (wc - 0x57 /*((WORD)L'a'+10)*/);
							break;
						default:
							return FALSE;
						}
						*(ptrDst++) = (BYTE)nValue;
						continue;
					case L'a': case L'b': case L'c': case L'd': case L'e': case L'f': 
						nValue = (wc - 0x57 /*((WORD)L'a'+10)*/);
						wc = *(pszCol++);
						switch (wc)
						{
						case L'0': case L'1': case L'2': case L'3': case L'4':
						case L'5': case L'6': case L'7': case L'8': case L'9':
							nValue = (nValue << 4) | (wc - 0x30/*(WORD)L'0'*/);
							break;
						case L'A': case L'B': case L'C': case L'D': case L'E': case L'F': 
							nValue = (nValue << 4) | (wc - 0x37 /*((WORD)L'A'+10)*/);
							break;
						case L'a': case L'b': case L'c': case L'd': case L'e': case L'f': 
							nValue = (nValue << 4) | (wc - 0x57 /*((WORD)L'a'+10)*/);
							break;
						default:
							return FALSE;
						}
						*(ptrDst++) = (BYTE)nValue;
						continue;
					case L',':
						// Следующий байт
						continue;
					case L'\\':
						// Перевод строки
						pszLine = pszCol;
						SkipLine(pszLine, pszEnd, nLine);
						pszCol = pszLine;
						SkipSpaces(pszCol, pszEnd);
						continue;
					case L'\r': case L'\n': case 0:
						// Конец данных
						goto hex_end;
					case L' ': case 0xA0:
						continue;
					case L';':
						// начало комментария в этой строке, конец данных!
						pszComment = pszCol-1;
						goto hex_end;
					default:
						// Недопустимый символ!
						return FALSE;
					}
				}
				// Конец файла или конец данных
				hex_end:
				nSize = (ptrDst - ptrValueData);
				if (nSize != (DWORD)nSize) {
					return FALSE;
				}
				nValueDataLen = (DWORD)nSize;
				// Если это строки, и файл типа REGEDIT4 - строки нужно сконвертировать в юникод
				if ((nValueType == REG_SZ || nValueType == REG_MULTI_SZ || nValueType == REG_EXPAND_SZ)
					&& !bUnicode)
				{
					// (используя наш временный буфер GetImportBuffer(nSize*2);
					wchar_t* pszCvt = (wchar_t*)GetImportBuffer(nSize*2);
					MultiByteToWideChar(cfg->nAnsiCodePage, 0, (LPCSTR)ptrValueData, nValueDataLen, pszCvt, nValueDataLen);
					ptrValueData = (LPBYTE)pszCvt;
					nValueDataLen = nValueDataLen * 2;
				}
				pszLine = pszCol - 1;
				if (!pszComment)
					pszComment = pszLine;
				//if (pszLine < pszEnd)
				//	SkipLine(pszLine, pszEnd, nLine);
				MCHKHEAP;
				goto success;
			}
			else
			{
				return FALSE;
			}
		}
	}
	
	MCHKHEAP;
	pszLine = pszCol;
success:
	//SkipLine(pszLine, pszEnd, nLine);
	if (pszComment)
	{
		while (*(pszComment-1) == ' ') pszComment--;
	}
	wchar_t* pszNewLine = wcspbrk(pszLine, L"\r\n");
	if (!pszNewLine)
	{
		_ASSERTE(pszNewLine || (pszEnd - pszLine) < 80);
		pszLine = pszEnd;
		if (pszComment) // ASCIIZ, блок данных гарантированно завершается '\0'
			nCommentLen = lstrlenW(pszComment);
	}
	else
	{
		if (pszComment) {
			nCommentLen = pszNewLine - pszComment;
			if (nCommentLen < 1)
				pszComment = NULL;
		}
		if (pszNewLine[0] == L'\r' && pszNewLine[1] == L'\n')
			pszLine = pszNewLine + 2;
		else
			pszLine = pszNewLine + 1;
		nLine++;
	}

	return TRUE;
}

// 1 - continue, 0 - break to editor, -1 - stop
int MFileReg::RegFormatFailed(LPCTSTR asFile, DWORD nLine, DWORD nCol)
{
	TCHAR  sLineInfo[128];
	wsprintf(sLineInfo, GetMsg(REFormatFailLineInfo), (nLine+1), (nCol+1));

	const TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg(REFormatFailLabel),
		asFile ? asFile : _T(""),
		sLineInfo,
		GetMsg(REBtnContinue),
		GetMsg(REBtnEditor),
		GetMsg(REBtnCancel),
	};
	int nBtn = psi.Message(_PluginNumber(guid_RegFormatFailed), FMSG_WARNING, NULL, 
		sLines, countof(sLines), 3);
 	if (nBtn == 0)
		return (nContinueRc = 1);
	else if (nBtn == 1)
		return (nContinueRc = 0);
	return (nContinueRc = -1);
}

void MFileReg::RegLoadingFailed(int anMsgId, LPCTSTR asFile, DWORD anErrCode)
{
	const TCHAR* sLines[] = 
	{
		GetMsg(REPluginName),
		GetMsg(anMsgId),
		asFile ? asFile : _T(""),
	};
	if (anErrCode) SetLastError(anErrCode);
	psi.Message(_PluginNumber(guid_RegLoadFailed), FMSG_WARNING|(anErrCode ? FMSG_ERRORTYPE : 0)|FMSG_MB_OK, NULL, 
		sLines, countof(sLines), 0);
}

// Для ускорения загрузки reg-файлов, в которых ключи чаще всего создаются последовательно
// будем проверять и сравнивать ПОЛНЫЙ путь последнего созданного ключа
FileRegItem* MFileReg::FindExistKey(FileRegItem* pParent, LPCWSTR apszSubKey, DWORD anSubkeyLen, LPCWSTR* ppszSubKeyPart)
{
	_ASSERTE(pParent);
	FileRegItem* p = pParent->pLastCreatedKey;
	if (!p)
		return NULL;
	LPCWSTR pszLast = pParent->pszFullPath;
	DWORD nFullPathLen = pParent->nFullPathLen;
	if (!pszLast || !*pszLast || !apszSubKey || !*apszSubKey || anSubkeyLen < 1 || !nFullPathLen)
		return NULL;
	if (p->nMagic != FILEREGITEM_MAGIC) {
		_ASSERTE(p->nMagic == FILEREGITEM_MAGIC);
		pParent->pLastCreatedKey = NULL;
		if (pParent->pszFullPath) *pParent->pszFullPath = 0;
		pParent->nFullPathLen = 0;
		return NULL;
	}
	_ASSERTE(nFullPathLen && nFullPathLen < pParent->nFullPathSize);

	_ASSERTE(pParent->pszFullPath[pParent->nFullPathLen] == 0);
	
	int nCmpRc;
	
	if (nFullPathLen < anSubkeyLen)
	{
		// Идеальный вариант - подключ предыдущего
		// [HREGKEY\SYSTEM\ControlSet001\Control\GraphicsDrivers]
		// [HREGKEY\SYSTEM\ControlSet001\Control\GraphicsDrivers\DCI]
		// Сразу возвращаем предыдущий ключ
		nCmpRc = wmemcmp(apszSubKey, pszLast, nFullPathLen);
		if (nCmpRc == 0 && apszSubKey[nFullPathLen] == L'\\') {
			*ppszSubKeyPart = (apszSubKey+nFullPathLen+1);
			return p;
		}
	}
	
	// Это может быть переход на уровень вверх (только скрипты, исправленные вручную)
	// [HREGKEY\SYSTEM\ControlSet001\Control\GraphicsDrivers\DCI]
	// [HREGKEY\SYSTEM\ControlSet001\Control\GraphicsDrivers]
	// Нужно вернуть общий ключ [HREGKEY\SYSTEM\ControlSet001\Control\GraphicsDrivers]
	//
		// Или, возможно, что это переключение вида
	// [HREGKEY\SYSTEM\ControlSet001\Control\DeviceClasses\{ffbb6e3f-ccfe-4d84-90d9-421418b03a8e}]
	// [HREGKEY\SYSTEM\ControlSet001\Control\GraphicsDrivers\DCI]
	// Тоже нужно вернуть общий ключ [HREGKEY\SYSTEM\ControlSet001\Control]

	for (int i = (nFullPathLen-1); i > 0; i--)
	{
		if (pszLast[i] == L'\\')
		{
			p = p->pParent;
			if (!p || p == &TreeRoot)
				return NULL; // не нашли
			if ((DWORD)i < anSubkeyLen && apszSubKey[i] == L'\\')
			{
				nCmpRc = wmemcmp(apszSubKey, pszLast, i);
				if (nCmpRc == 0) {
					*ppszSubKeyPart = apszSubKey+i+1;
					return p;
				}
			}
		}
	}
	

	return NULL;
}

// 
LONG MFileReg::LoadRegFile(LPCWSTR asRegFilePathName, BOOL abSilent, MRegistryBase* apTargetWorker /*= NULL*/,
						   BOOL abDelayLoading /*= FALSE*/, MFileReg* apFileReg /*= NULL*/)
{
	MFileReg* lpFileReg = NULL;
	if (apFileReg == NULL)
	{
		InvalidOp();
		return -1;
	}

	BOOL bInternalRegParsing = FALSE;
	if (apTargetWorker && apTargetWorker->eType == RE_REGFILE)
	{
		if (apFileReg == (MFileReg*)apTargetWorker)
		{
			_ASSERTE(apTargetWorker == NULL);
			lpFileReg = apFileReg;
			bInternalRegParsing = TRUE;
		}
		else
		{
			lpFileReg = (MFileReg*)apTargetWorker;
		}
		apTargetWorker = NULL; // работаем с чистым *.reg
	}
	else if (apTargetWorker == NULL)
	{
		bInternalRegParsing = TRUE;
		lpFileReg = apFileReg;
	}
	else
	{
		// А тут придется вызывать стандартные методы apTargetWorker (это может быть WinApi, Hive, ...)
		lpFileReg = apFileReg;
		bInternalRegParsing = FALSE;
	}

	_ASSERTE(asRegFilePathName && *asRegFilePathName);
	_ASSERTE(apFileReg->psRegFilePathName == NULL);

	_ASSERTE(apFileReg->TreeRoot.pFirstKey == NULL && apFileReg->TreeRoot.pFirstVal == NULL);

	SafeFree(apFileReg->psRegFilePathName);
	SafeFree(apFileReg->psShowFilePathName);
	apFileReg->psRegFilePathName = MakeUNCPath(asRegFilePathName);
	apFileReg->bSilent = abSilent;
	apFileReg->nContinueRc = 1;
	//if (apTargetWorker == (MRegistryBase*)apFileReg)
	//{
	//	//apFileReg->bInternalRegParsing = FALSE;
	//} else {
	//	//apFileReg->bInternalRegParsing = FALSE;
	//}

	//TreeDeleteChild(NULL, &TreeRoot); -- !!! Низя !!! Эта функция вызывается и при обновлении (AltShiftF4 в загруженном .reg)

	LONG hRc = 0;	
	DWORD nDataLen = 0;
	LPBYTE ptrData = NULL;
	wchar_t* pszData = NULL; BOOL bSzAllocated = FALSE;
	LPWSTR pszLine = NULL, pszEnd = NULL, pszCol = NULL;
	LPWSTR pszName = NULL; BOOL bKeyDeletion = FALSE, bValDeletion = FALSE;
	LPWSTR pszComment = NULL; int nCommentLen = 0;
	wchar_t sDummyKeyName[2] = {L'.',0};
	LPWSTR pszKeyName = sDummyKeyName /*чтобы сразу строчка для пути ключа добавилась в окно прогресса*/, pszKeyNameEnd = NULL;
	LPBYTE  ptrValueData = NULL; DWORD nValueDataLen = 0; REGTYPE nValueType = 0;
	DWORD nLine = 0;
	int nCont;
	FileRegItem* pKey = NULL;
	FileRegItem* pCommentKey = NULL;
	FileRegItem* pVal = NULL;
	HKEY hKeyRoot = NULL; // Это один из HKEY_xxx
	HREGKEY hWorkKey = NULLHKEY; // А вот этот - нужно закрывать
	wchar_t sLower[32768];
	int nNameLen = 0;
	DWORD nNameCRC32 = 0;
	LPCWSTR pszFormat5W = L"Windows Registry Editor Version 5.00";
	LPCSTR pszFormat5A = "Windows Registry Editor Version 5.00";
	DWORD cbFormat5Len = lstrlenW(pszFormat5W);
	int nSteps = 0;
	bool bFirstKey = false;
	bool bWasEmptyLine = false, bFileIsDirty = false;
	BOOL lbFileUnicode = FALSE, lbBOM = FALSE;
	int nFileFormat = 0;

	
	// apTargetWorker может быть указан при импорте *.reg файла в системный реестр Windows
	// apTargetWorker==NULL при загрузке дерева для показа его на панели плагина

	// .reg назначения
	pKey = pCommentKey = &(lpFileReg->TreeRoot);
	

	// Загрузить RAW данные файла
	hRc = MFileTxt::LoadData(asRegFilePathName, (void**)&ptrData, &nDataLen, abDelayLoading ? 16384 : -1);
	if (hRc != 0) {
		apFileReg->RegLoadingFailed(REM_RegFileLoadFailed, apFileReg->GetShowFilePathName(), hRc);
		goto wrap;
	}

	if (gpProgress)
	{
		wchar_t szMsg[128];
		lstrcpy_t(szMsg, countof(szMsg), GetMsg(RELoadingRegFileTitle));
		if (!gpProgress->SetStep(0, TRUE, szMsg/*L" "*/))
			goto wrap; // Esc pressed
	}

	if (!DetectFileFormat(ptrData, nDataLen, &nFileFormat, &lbFileUnicode, &lbBOM))
	{
		hRc = ERROR_BAD_FORMAT;
		REPlugin::MessageFmt(REM_UnknownFileFormat, asRegFilePathName);
		goto wrap;
	}

	// если он ANSI - преобразовать в Unicode, для унификации функции разбора
	//if (memcmp(ptrData, "REGEDIT4", 8) == 0
	//	|| memcmp(ptrData, "REGEDIT5", 8) == 0
	//	|| memcmp(ptrData, pszFormat5A, cbFormat5Len) == 0)
	if (!lbFileUnicode)
	{
		_ASSERTE(nDataLen < nDataLen*2);
		pszData = (wchar_t*)malloc(nDataLen*2+2);
		if (!pszData) {
			InvalidOp();
			hRc = ERROR_NOT_ENOUGH_MEMORY;
			goto wrap;
		}
		bSzAllocated = TRUE;
		if (cfg->nAnsiCodePage == CP_UTF8 || cfg->nAnsiCodePage == CP_UTF7) {
			_ASSERTE(cfg->nAnsiCodePage!=CP_UTF8 && cfg->nAnsiCodePage!=CP_UTF7);
			InvalidOp();
			hRc = -1;
			goto wrap;
		}
		// Нужен Unicode
		//TODO: Оптимизировать? Сделать раздельный разбор файлов?
		int nCvtLen = MultiByteToWideChar(cfg->nAnsiCodePage, 0, (LPCSTR)ptrData, nDataLen, pszData, nDataLen);
		if (nCvtLen <= 0) {
			hRc = GetLastError(); if (!hRc) hRc = -1;
			InvalidOp();
			goto wrap;
		}
		_ASSERTE(nDataLen == (DWORD)nCvtLen);
		pszData[nCvtLen] = 0; // страховка для строковых функций!
		nDataLen = nCvtLen;
		//bUnicode = (memcmp(ptrData, "REGEDIT4", 8) != 0); // REGEDIT4?
		apFileReg->bUnicode = (nFileFormat == 1);
		SafeFree(ptrData);
		
	} else {
		pszData = ((wchar_t*)ptrData) + (lbBOM ? 1 : 0);
		// Размер был в байтах, нам нужно в символах
		nDataLen = nDataLen >> 1;
		apFileReg->bUnicode = (nFileFormat == 1);

		//WORD nBOM = 0xFEFF;
		//// Если есть BOM - пропустить
		//if (*pszData == nBOM) { pszData ++; nDataLen-=2; }
		//if (memcmp(pszData, pszFormat5W, cbFormat5Len*2) == 0
		//	|| memcmp(pszData, L"REGEDIT5", 16) == 0 // REGEDIT5 для упрощения редактирования :)
		//	|| memcmp(pszData, L"REGEDIT4", 16) == 0) // Допускается формат REGEDIT4 в юникодном файле
		//{
		//	// Размер был в байтах, нам нужно в символах
		//	nDataLen = nDataLen >> 1;
		//	bUnicode = (memcmp(pszData, L"REGEDIT4", 16) != 0); // REGEDIT5?
		//	
		//} else {
		//	hRc = ERROR_BAD_FORMAT;
		//	InvalidOp();
		//	goto wrap;
		//}
	}

	// Запомнить в переменной формат из заголовка
	apFileReg->bFirstFileFormatUnicode = apFileReg->bUnicode;
	
	pszLine = pszData;
	pszEnd = pszData + nDataLen - 1;
	SkipLine(pszLine, pszEnd, nLine);

#ifdef _DEBUG
	lpFileReg->InitializeMalloc(nDataLen*3);
#else
	lpFileReg->InitializeMalloc(nDataLen >> 2);
#endif

	if (gpProgress)
		gpProgress->SetAll((pszEnd - pszData));

	MCHKHEAP;
	
	//TODO: Бежать по файлу, и звать для apTargetWorker CreateKey, DeleteKey, SetValue, DeleteValue
	while (pszLine < pszEnd)
	{
		// 1 - continue, 0 - break to editor, -1 - stop
		// RegFormatFailed(LPCTSTR asFile, DWORD nLine, DWORD nCol)

		nSteps++; // Чтобы SetStep слишком часто GetTickCount() не дергала
		if (gpProgress && nSteps >= 10)
		{
			nSteps = 0;
			if (!gpProgress->SetStep((pszLine - pszData), FALSE, pszKeyName))
				break; // Esc pressed
		}

#ifdef _DEBUG
		if (nLine >= 5009 && nLine <= 5015)
			pszLine = pszLine;
#endif

		pszCol = pszLine;
		SkipSpaces(pszCol, pszEnd);
		MCHKHEAP;
		switch (*pszCol)
		{
		case L'[':
			// Обработка нового ключа
			bWasEmptyLine = false; bFileIsDirty = true;
			if (!apFileReg->GetKeyFromLine(pszLine, pszCol, pszEnd, hKeyRoot, pszKeyName, pszKeyNameEnd, bKeyDeletion, nLine, pszComment, nCommentLen))
			{
				if (!apFileReg->bSilent)
					nCont = apFileReg->RegFormatFailed(apFileReg->GetShowFilePathName(), nLine, (DWORD)(pszCol-pszLine));
				else
					nCont = -1;
				if (nCont == 1) {
					pszKeyName = NULL; bKeyDeletion = TRUE; // Разрешено продолжить
				} else {
					hRc = ERROR_BAD_FORMAT;
					goto wrap;
				}
			}
			if (pszKeyName == NULL)
			{
				pKey = NULL; bKeyDeletion = TRUE; // пропуск некорректных элементов
				continue; // строка уже накручена!
			} else {
				if (bFirstKey) {
					bFirstKey = false;
					gpProgress->SetStep((pszLine - pszData), TRUE, pszKeyName);
				}
				//#ifdef _DEBUG
				//int nDbgCmp = lstrcmpiW(pszKeyName, L"Software\\Far\\Users\\Zeroes_And_Ones\\UserMenu\\MainMenu\\Item10\\Item10");
				//if (nDbgCmp == 0) {
				//	// Что-то мой парсер зациклился на UserMenu\MainMenu\Item10\Item10...
				//	_ASSERTE(nDbgCmp != 0);
				//}
				//#endif

				if (abDelayLoading)
					break; // OK, считаем файл валидным
				if (!apTargetWorker)
				{
					MCHKHEAP;
					//hRc = CreateKeyEx(hKeyRoot, pszKeyName, 0, 0, bKeyDeletion ? REG__OPTION_CREATE_DELETED : 0, 0, 0, (HREGKEY*)&pKey, NULL);
					
					FileRegItem* pRoot  = lpFileReg->ItemFromHkey(hKeyRoot);
					if (!pRoot)
					{
						InvalidOp();
						hRc = E_UNEXPECTED;
						goto wrap;
					}
					
					hRc = lpFileReg->TreeCreateKey(pRoot,
						pszKeyName, (int)(pszKeyNameEnd-pszKeyName), TRUE/*abAllowCreate*/, bKeyDeletion,
						TRUE/*abInternal*/, &pKey, NULL, pszComment, nCommentLen);
					//if (hRc == ERROR_INVALID_NAME)
					if (hRc != 0)
					{
						if (!apFileReg->bSilent) //TODO: Можно бы показать, на каком подключе затык?
							nCont = apFileReg->RegFormatFailed(apFileReg->GetShowFilePathName(), nLine-1, 0);
						else
							nCont = -1;
						if (nCont == 1) {
							pszKeyName = NULL; bKeyDeletion = TRUE; // Разрешено продолжить
							continue; // строка уже накручена!
						} else {
							nLine--;
							hRc = ERROR_BAD_FORMAT;
							goto wrap;
						}
					} else {
						pCommentKey = pKey;
						//if (pszComment)
						//{
						//	_ASSERTE(nCommentLen > 0);
						//	TreeAddChild(pCommentKey, REG__COMMENT, NULL, pszComment, pszComment, nCommentLen, 0, NULL, 0, FALSE);
						//}
					}
					//else if (hRc != 0)
					//{
					//	InvalidOp();
					//}
				} else {
					if (hWorkKey)
						apTargetWorker->CloseKey(&hWorkKey);
					MCHKHEAP;
					if (bKeyDeletion)
					{
						hRc = apTargetWorker->DeleteSubkeyTree(hKeyRoot, pszKeyName);
						if (hRc == ERROR_FILE_NOT_FOUND)
							hRc = 0;
					} else {
						hRc = apTargetWorker->CreateKeyEx(hKeyRoot, pszKeyName, 0, 0, 0, KEY_WRITE, NULL, &hWorkKey, NULL, NULL);
					}
					if (hRc != 0)
					{
						wchar_t szFullKeyName[MAX_PATH*3+42];
						HKeyToStringKey(hKeyRoot, szFullKeyName, 40); lstrcatW(szFullKeyName, L"\\");
						if (pszKeyName) lstrcpynW(szFullKeyName+lstrlenW(szFullKeyName), pszKeyName, MAX_PATH*3);
						int nContRc = REPlugin::MessageFmt(
							bKeyDeletion ? REM_ImportDeleteKeyFailed : REM_ImportCreateKeyFailed, szFullKeyName,
							hRc, 0, FMSG_WARNING|FMSG_ERRORTYPE, 2);
						if (nContRc == 0)
						{
							// Пропустить все значения этого ключа, раз создать не получилось
							bKeyDeletion = TRUE;
							hRc = 0;
							break;
						}
					}
				}
				if (hRc != 0)
					goto wrap; // Ошибка уже показана
				if (!apTargetWorker)
				{
					_ASSERTE(pKey);
					pKey->bDeletion = bKeyDeletion;
				}
			}
			continue; // строка уже накручена!
		case L'\"':
		case L'@':
			// Обработка нового значения
			if (!apFileReg->GetValueFromLine(pszLine, pszCol, pszEnd, pszName, sLower, nNameLen, nNameCRC32, bValDeletion,
					nLine, nValueType, ptrValueData, nValueDataLen, pszComment, nCommentLen))
			{
				if (!apFileReg->bSilent)
					nCont = apFileReg->RegFormatFailed(apFileReg->GetShowFilePathName(), nLine, (DWORD)(pszCol-pszLine));
				else
					nCont = -1;
				if (nCont == 1) {
					break;
				} else {
					hRc = ERROR_BAD_FORMAT;
					goto wrap;
				}
			}
			bFileIsDirty = true;
			// Тут именно KEY. Если ранее был удаляемый ключ [-HREGKEY...], то все что до следующего ключа [HREGKEY] - пропускается!
			if (bKeyDeletion)
			{
				if (!apFileReg->bSilent)
					nCont = apFileReg->RegFormatFailed(apFileReg->GetShowFilePathName(), nLine-1, 0); //"-1" т.к. строка уже накручена
				else
					nCont = 1;
				switch (nCont)
				{
				case 1:
					pszLine = pszCol;
					continue; // следующая строка
				case 0:
					//в редактор
					nLine--;
					hRc = ERROR_BAD_FORMAT;
					goto wrap;
				default:
					hRc = ERROR_BAD_FORMAT;
					goto wrap;
				} // switch (nCont)
			}
			else if (apTargetWorker)
			{
				// !!! Запись в реальный реестр
				_ASSERTE(hWorkKey!=NULL);
				if (bValDeletion) {
					hRc = apTargetWorker->DeleteValue(hWorkKey, pszName);
					if (hRc == ERROR_FILE_NOT_FOUND)
						hRc = 0;
				} else {
					_ASSERTE(nValueType != REG_DWORD || nValueDataLen == 4);
					hRc = apTargetWorker->SetValueEx(hWorkKey, pszName, 0, nValueType, ptrValueData, nValueDataLen);
				}
			}
			else if (pKey)
			{
				lpFileReg->MarkDirty(pKey);
				// Сохранение пустых строк
				if (bWasEmptyLine)
				{
					bWasEmptyLine = false;
					if (bInternalRegParsing)
						lpFileReg->TreeAddChild(pKey, REG__COMMENT, NULL, L"", L"", 0, 0, NULL, 0, FALSE);
				}
				// Добавить значение в родителя
				//2010-07-29 заменил TreeAddChild на TreeSetValue, ибо дублировались значения
				pVal = lpFileReg->TreeSetValue(pKey, pszName, sLower, nNameLen, nNameCRC32,
						nValueType, ptrValueData, nValueDataLen, pszComment, nCommentLen, bValDeletion);
				//if (pszComment)
				//	TreeAddChild(pKey, REG__COMMENT, NULL, pszComment, pszComment, nCommentLen, 0, NULL, 0);
			}
			continue; // строка уже накручена!
		case L';':
			// Однострочный комментарий
			pszLine = pszCol; bFileIsDirty = true;
			if (pCommentKey)
			{
				lpFileReg->MarkDirty(pCommentKey);
				if (bWasEmptyLine)
				{
					bWasEmptyLine = false;
					if (bInternalRegParsing)
						lpFileReg->TreeAddChild(pCommentKey, REG__COMMENT, NULL, L"", L"", 0, 0, NULL, 0, FALSE);
				}
				LPWSTR pszLineEnd = wcspbrk(pszCol, L"\r\n");
				if (!pszLineEnd)
					pszLineEnd = pszEnd;

				int nCommLen = (int)(pszLineEnd - pszLine);
				if (nCommLen > 0)
				{
					wchar_t wcCur = *pszLineEnd; *pszLineEnd = 0; // Иначе сравнивалка обломается
					DWORD nCommCRC32 = apFileReg->CalculateNameCRC32(pszLine, nCommLen);
					if (bInternalRegParsing)
					{
						// Если это первичная загрузка - сразу добавляем
						lpFileReg->TreeAddChild(pCommentKey, REG__COMMENT, NULL, pszLine, pszLine, nCommLen, nCommCRC32, NULL, 0, FALSE);
					}
					else
					{
						// Иначе - его нужно попытаться найти, может такой комментарий уже есть, тогда задваивать не нужно!
						FileRegItem* lpComm = lpFileReg->TreeFindValue(pCommentKey, pszLine, pszLine, nCommLen, nCommCRC32);
						// И если комментарий новый - тогда добавить (но он пойдет в текущий конец ключа)
						if (!lpComm)
							lpFileReg->TreeAddChild(pCommentKey, REG__COMMENT, NULL, pszLine, pszLine, nCommLen, nCommCRC32, NULL, 0, FALSE);
					}
					*pszLineEnd = wcCur;
				}
				else
				{
					_ASSERTE(nCommLen > 0);
				}
				pszLine = pszLineEnd;
			}
			SkipLine(pszLine, pszEnd, nLine);
			continue; // следующая строка
		case L'\r':
		case L'\n':
			// Пустая строка
			if (bFileIsDirty)
			{
				if (bWasEmptyLine && pCommentKey && bInternalRegParsing)
				{
					// Сохранить в структуре
					lpFileReg->TreeAddChild(pCommentKey, REG__COMMENT, NULL, L"", L"", 0, 0, NULL, 0, FALSE);
				}
				bWasEmptyLine = true;
			}
			break;
		default:
			{
				// тут могут быть куски заголовков, полученные от склейки файлов
				WORD nBOM = 0xFEFF;
				if (*pszCol == nBOM && (pszCol[1] == L'R' || pszCol[1] == L'W'))
					pszCol++;
				if (*pszCol == L'R')
				{
					if (memcmp(pszCol, L"REGEDIT4", 16) == 0)
					{
						apFileReg->bUnicode = FALSE; // Обновить режим конвертации для REG_EXPAND_SZ & REG_MULTI_SZ
						break; // Перейти на следующую строку
					}
					else if (memcmp(pszCol, L"REGEDIT5", 16) == 0)
					{
						apFileReg->bUnicode = TRUE; // Обновить режим конвертации для REG_EXPAND_SZ & REG_MULTI_SZ
						break; // Перейти на следующую строку
					}
				}
				else
				{
					LPCWSTR pszFormat = L"Windows Registry Editor Version 5.00";
					DWORD cbFormatLen = lstrlenW(pszFormat)*2;
					if (memcmp(pszCol, pszFormat, cbFormatLen) == 0)
					{
						apFileReg->bUnicode = TRUE; // Обновить режим конвертации для REG_EXPAND_SZ & REG_MULTI_SZ
						break; // Перейти на следующую строку
					}
				}
				
				// В противном случае - вываливаем ошибку
				if (!apFileReg->bSilent)
					nCont = apFileReg->RegFormatFailed(apFileReg->GetShowFilePathName(), nLine, (DWORD)(pszCol-pszLine));
				else
					nCont = -1;
				switch (nCont)
				{
				case 1:
					pszLine = pszCol;
					SkipLine(pszLine, pszEnd, nLine);
					continue; // следующая строка
				case 0:
					//в редактор
					hRc = ERROR_BAD_FORMAT;
					goto wrap;
				case -1:
					//Cancel
					hRc = ERROR_BAD_FORMAT;
					goto wrap;
				default:
					hRc = ERROR_BAD_FORMAT;
					InvalidOp();
					goto wrap;
				} // switch (nCont)
			} // default:
		} // switch (*pszCol)
		
		// следующая строка
		pszLine = pszCol;
		SkipLine(pszLine, pszEnd, nLine);
	}

	if (gpProgress)
	{
		if (!gpProgress->SetStep((pszEnd - pszData), TRUE, L""))
		{
			hRc = ERROR_CANCELLED;
		}
		else
		{
			#ifdef SHOW_DURATION
			gpProgress->ShowDuration();
			#endif
		}
	}

	// Done
wrap:
	//apFileReg->bInternalRegParsing = FALSE;
	// Release work key
	if (apTargetWorker && hWorkKey)
		apTargetWorker->CloseKey(&hWorkKey);
	// Release pointers
	if (pszData && bSzAllocated)
		SafeFree(pszData);
	pszData = NULL;
	SafeFree(ptrData);
	// Если хотели открыть редактор
	if (apFileReg->nContinueRc == 0)
	{
		TCHAR* pszShowName = UnmakeUNCPath_t(asRegFilePathName);
		// Запустить редактор с установкой на строке
		int nCol = (pszCol > pszLine && pszCol < pszEnd) ? (pszCol - pszLine + 1) : 1;
		//TODO: Когда редакторы реестра станут немодальными - просто установить курсор на нужную строку/колонку
		#ifdef _DEBUG
		int nEdtRc =
		#endif
		psi.Editor(pszShowName, NULL, 0,0,-1,-1,
			EF_DISABLEHISTORY
			| ((gnOpMode & OPM_DISABLE_NONMODAL_EDIT) ? 0 : (EF_NONMODAL|EF_IMMEDIATERETURN))
			| EF_ENABLE_F6,
			#ifdef _UNICODE
			nLine+1, nCol, /*cfg->EditorCodePage()*/ lbFileUnicode ? 1200 : FARCP_AUTODETECT
			#else
			nLine, nCol
			#endif
		);
		SafeFree(pszShowName);
	}
	RETURN(hRc);
}
