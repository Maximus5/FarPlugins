
#include "header.h"

/*

	Объект предназначен для работы с текстовыми (Ansi/Unicode) и бинарными файлами.

*/

#define MAX_FORMAT_KEY_LEN  0x8000
#define MAX_FORMAT_KEY_SIZE ((MAX_FORMAT_KEY_LEN+0x80)*2)

BOOL MFileTxt::bBadMszDoubleZero = FALSE;

MFileTxt::MFileTxt(BOOL abWow64on32, BOOL abVirtualize)
	: MRegistryBase(abWow64on32, abVirtualize)
{
	eType = RE_REGFILE;

	hFile = NULL; bLastRc = FALSE; nLastErr = 0; bOneKeyCreated = FALSE;
	bUnicode = cfg->bCreateUnicodeFiles;
	psTempDirectory = psFilePathName = NULL;
	psShowFilePathName = NULL;
	// Write buffer (cache)
	ptrWriteBuffer = NULL; nWriteBufferLen = nWriteBufferSize = 0;
	// Value export buffer
	pExportBufferData = pExportFormatted = pExportCPConvert = NULL;
	cbExportBufferData = cbExportFormatted = cbExportCPConvert = 0;
	// Exporting values
	pszExportHexValues = (wchar_t*)malloc(256*4);
	wchar_t sHex[] = L"0123456789abcdef";
	wchar_t *ph = pszExportHexValues;
	for (int i=0; i<16; i++) {
		for (int j=0; j<16; j++) {
			*(ph++) = sHex[i];
			*(ph++) = sHex[j];
		}
	}
	//memset(&TreeRoot, 0, sizeof(TreeRoot));
	
	//pszRootKeys[0] = L"HKEY_CLASSES_ROOT";
	//pszRootKeys[1] = L"HKEY_CURRENT_USER";
	//pszRootKeys[2] = L"HKEY_LOCAL_MACHINE";
	//pszRootKeys[3] = L"HKEY_USERS";
	//pszRootKeys[3] = L"HKEY_PERFORMANCE_DATA";
	//pszRootKeys[5] = L"HKEY_CURRENT_CONFIG";
	//pszRootKeys[6] = L"HKEY_DYN_DATA";
	//pszRootKeys[7] = L"HKEY_CURRENT_USER_LOCAL_SETTINGS";
}

MFileTxt::~MFileTxt()
{
	FileClose(); // Если еще не закрыт
	// Free blocks
	SafeFree(psTempDirectory);
	SafeFree(psFilePathName);
	SafeFree(psShowFilePathName);
	// Write buffer (cache)
	SafeFree(ptrWriteBuffer);
	// Value export buffer
	SafeFree(pExportBufferData);
	SafeFree(pExportFormatted);
	SafeFree(pExportCPConvert);
	SafeFree(pszExportHexValues);
}

void MFileTxt::ReleasePointers()
{
	SafeFree(psTempDirectory);
	SafeFree(psFilePathName);
	SafeFree(psShowFilePathName);
}

MRegistryBase* MFileTxt::Duplicate()
{
	_ASSERTE(FALSE);
	InvalidOp();
	return NULL;
}

#ifndef _UNICODE
/*static*/
// Должен возвращать длину строки + 4 байта (L"\0")
LONG MFileTxt::LoadText(LPCSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize)
{
	wchar_t* pszW = lstrdup_w(asFilePathName);
	LONG lRc = LoadText(pszW, abUseUnicode, pszText, pcbSize);
	SafeFree(pszW);
	return lRc;
}
/*static*/
// Должен возвращать длину строк + 4 байта (L"\0\0")
LONG MFileTxt::LoadTextMSZ(LPCSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize)
{
	wchar_t* pszW = lstrdup_w(asFilePathName);
	LONG lRc = LoadTextMSZ(pszW, abUseUnicode, pszText, pcbSize);
	SafeFree(pszW);
	return lRc;
}
/*static*/
LONG MFileTxt::LoadData(LPCSTR asFilePathName, void** pData, DWORD* pcbSize)
{
	wchar_t* pszW = lstrdup_w(asFilePathName);
	LONG lRc = LoadData(pszW, pData, pcbSize);
	SafeFree(pszW);
	return lRc;
}
#endif

/*static*/
// Должен возвращать длину строки + 4 байта (L"\0")
LONG MFileTxt::LoadText(LPCWSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize)
{
	LONG lLoadRc = -1;
	//HANDLE lhFile = NULL;
	void* pData = NULL; DWORD cbSize = 0;
	
	
	_ASSERTE(pszText && pcbSize);
	_ASSERTE(*pszText == NULL);
	
	lLoadRc = LoadData(asFilePathName, &pData, &cbSize);
	if (lLoadRc != 0)
		return lLoadRc;
		
	//// Если размер менее 2 байт или не выровнен на 2 байта - считаем что текст НЕ юникодный?
	//if (cbSize < 2 || (abUseUnicode && ((cbSize>>1)<<1) != cbSize))
	//{
	//	//TODO: хотя это потенциальная засада? может быть пытаться отдавать наибольшее получившееся
	//	//TODO: количество символов wchar_t?
	//	if (abUseUnicode)
	//	{
	//		//TODO: проверить ветку
	//		*pcbSize = (cbSize*2) + 2; // размер в wchar_t + L'\0'
	//		*pszText = (wchar_t*)malloc(*pcbSize);
	//		_ASSERTE(cfg->nAnsiCodePage!=CP_UTF8 && cfg->nAnsiCodePage!=CP_UTF7);
	//		MultiByteToWideChar(cfg->nAnsiCodePage, 0, (char*)pData, cbSize, *pszText, cbSize);
	//		(*pszText)[cbSize] = 0; // + L'\0'
	//	} else {
	//		//TODO: проверить ветку
	//		*pcbSize = cbSize + 1; // размер в char + L'\0'
	//		*pszText = (wchar_t*)malloc(*pcbSize);
	//		memmove(*pszText, pData, cbSize);
	//		(*((char**)pszText))[cbSize] = 0; // + '\0'
	//	}
	//	SafeFree(pData);
	//	return 0; // OK
	//}
	
	lLoadRc = -1;

	//if (abUseUnicode)
	{
		// Проверить, что есть BOM
		WORD nBOM = 0xFEFF;
		if (abUseUnicode && cbSize >= 2 && (*((WORD*)pData) == nBOM) && (((cbSize>>1)<<1) == cbSize))
		{
			// Считаем, что в файле лежит юникодный текст
			//TODO: хотя это потенциальная засада? может быть пытаться отдавать наибольшее получившееся
			//TODO: количество символов wchar_t?
			cbSize = ((cbSize >> 1) - 1) << 1;
			*pcbSize = cbSize + 2; // размер в BYTE + L'\0' + не отдавать, но зарезервировать еще один L'\0' для MULTI_SZ
			*pszText = (wchar_t*)malloc(cbSize + 4); // MULTI_SZ ( + L"\0\0" )
			// Copy skipping BOM
			if (cbSize > 0)
				memmove(*pszText, ((wchar_t*)pData)+1, cbSize);
			(*pszText)[(cbSize>>1)] = 0; // + L'\0'
			(*pszText)[(cbSize>>1)+1] = 0; // + L'\0'
			lLoadRc = 0;
		}
		else
		{
			//TODO: проверить ветку
			// Считаем, что в файле лежит ansi текст
			*pcbSize = (cbSize*2) + 2; // размер в BYTE + L'\0' + не отдавать, но зарезервировать еще один L'\0' для MULTI_SZ
			*pszText = (wchar_t*)malloc((*pcbSize) + 2);
			_ASSERTE(cfg->nAnsiCodePage!=CP_UTF8 && cfg->nAnsiCodePage!=CP_UTF7);
			MultiByteToWideChar(cfg->nAnsiCodePage, 0, (char*)pData, cbSize, *pszText, cbSize);
			(*pszText)[cbSize] = 0; // + L'\0'
			(*pszText)[cbSize+1] = 0; // + L'\0'
			lLoadRc = 0;
		}
	}
	//else
	//{
	//	//TODO: проверить ветку
	//	// Хотят на выходе получить ANSI, считаем, что файл ANSI?
	//	*pcbSize = cbSize + 1; // размер в wchar_t + L'\0'
	//	*pszText = (wchar_t*)malloc(*pcbSize);
	//	memmove(*pszText, ((wchar_t*)pData)+1, cbSize);
	//	(*((char**)pszText))[cbSize] = 0; // + '\0'
	//	lLoadRc = 0;
	//}
	
	SafeFree(pData);
	return lLoadRc;
}

/*static*/
// Должен возвращать длину строк + 4 байта (L"\0\0")
LONG MFileTxt::LoadTextMSZ(LPCWSTR asFilePathName, BOOL abUseUnicode, wchar_t** pszText, DWORD* pcbSize)
{
	LONG lLoadRc = MFileTxt::LoadText(asFilePathName, abUseUnicode, pszText, pcbSize);
	if (lLoadRc != 0)
		return lLoadRc;

	// Преобразовать переводы строк \r\n в \0
	wchar_t* pszBase = *pszText;
	DWORD nLen = (*pcbSize) >> 1;
	wchar_t* pszEnd = pszBase + nLen;
	wchar_t* pszDst = pszBase;
	wchar_t* pszLnStart = pszBase;
	wchar_t* pszLnNext;
	wchar_t* pszLnEnd;
	while (pszLnStart < pszEnd)
	{
		if (*pszLnStart == 0 && (pszLnStart+1) >= pszEnd)
		{
			break;
		}
		//pszLnEnd = pszLnStart;
		//while (pszLnEnd < pszEnd)
		//{
		//	if (*pszLnEnd == L'\r' && *(pszLnEnd+1) == L'\n') {
		//		*pszLnEnd = 0;
		//		break;
		//	}
		//	pszLnEnd++;
		//}
		pszLnEnd = wcspbrk(pszLnStart, L"\r\n");
		if (!pszLnEnd)
		{
			pszLnEnd = pszEnd - 1;
			pszLnNext = pszEnd;
		}
		else
		{
			if (pszLnEnd[0] == L'\r' && pszLnEnd[1] == L'\n')
				pszLnNext = pszLnEnd + 2;
			else
				pszLnNext = pszLnEnd + 1;
		}

		if (pszDst != pszLnStart)
		{
			_ASSERTE(pszDst < pszLnStart);
			memmove(pszDst, pszLnStart, (pszLnEnd - pszLnStart)*2);
		}
		//*pszLnEnd = 0;

		pszDst += (pszLnEnd - pszLnStart);
		if (*pszLnNext == L'\r' || *pszLnNext == L'\n')
		{
			*(pszDst++) = L'\n';
		}
		else
		{
			*(pszDst++) = 0;
		}
		pszLnStart = pszLnNext;
	}
	_ASSERTE((pszDst) <= pszEnd);
	*(pszDst++) = 0;
	//*(pszDst+1) = 0;
	if (pszDst == pszBase)
		*pcbSize = 2; // пустой файл
	else
		*pcbSize = (pszDst - pszBase )*2;

	return lLoadRc;
}

/*static*/
LONG MFileTxt::LoadData(LPCWSTR asFilePathName, void** pData, DWORD* pcbSize, size_t ncbMaxSize /*= -1*/)
{
	LONG lLoadRc = -1;
	HANDLE lhFile = NULL;
	
	_ASSERTE(pData && pcbSize);
	_ASSERTE(*pData == NULL);

	if (!asFilePathName || !*asFilePathName || !pData || !pcbSize)
	{
		InvalidOp();
		return E_INVALIDARG;
	}

	//#ifdef _UNICODE
	if (asFilePathName[0]==L'\\' && asFilePathName[1]==L'\\' && asFilePathName[2]==L'?' && asFilePathName[3]==L'\\')
	{
		lhFile = CreateFileW( asFilePathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		lLoadRc = GetLastError();
	}
	else
	{
		#ifdef _DEBUG
		int nLen = lstrlenW(asFilePathName);
		#endif
		//wchar_t* pszUNC = (wchar_t*)malloc((nLen+32)*2);
		//CopyPath(pszUNC, asFilePathName, nLen+32);
		wchar_t* pszUNC = MakeUNCPath(asFilePathName);
		lhFile = CreateFileW( pszUNC, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		lLoadRc = GetLastError();
		SafeFree(pszUNC);
	}
	//#else
	//	lhFile = CreateFileA( asFilePathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	//	lLoadRc = GetLastError();
	//#endif
	if (lhFile == INVALID_HANDLE_VALUE)
	{
		//lLoadRc = GetLastError();
		REPlugin::MessageFmt(REM_CantOpenFileReading, asFilePathName, lLoadRc);
		return (lLoadRc == 0) ? -1 : lLoadRc;
	}
	
	LARGE_INTEGER lSize;
	if (!GetFileSizeEx(lhFile, &lSize))
	{
		lLoadRc = GetLastError();
		CloseHandle(lhFile);
		return (lLoadRc == 0) ? -1 : lLoadRc;
	}
	// Таких больших файлов в случае реестра быть не может
	if (lSize.HighPart)
	{
		CloseHandle(lhFile);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	if (ncbMaxSize != (size_t)-1 && lSize.LowPart > ncbMaxSize)
		lSize.LowPart = ncbMaxSize;
	
	*pcbSize = lSize.LowPart;
	*pData = (void*)malloc(lSize.LowPart+3); // Страховка для строковых функций в MFileReg
	if (*pData == NULL)
	{
		CloseHandle(lhFile);
		REPlugin::MemoryAllocFailed(lSize.LowPart);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	
	DWORD nRead = 0;
	// Если файл пустой - то читать нет смысла
	if (lSize.LowPart && !ReadFile(lhFile, *pData, lSize.LowPart, &nRead, NULL))
	{
		lLoadRc = GetLastError();
		if (lLoadRc == 0) lLoadRc = -1;
		SafeFree(*pData);
		REPlugin::MessageFmt(REM_CantReadFile, asFilePathName, lLoadRc);
	}
	else
	{
		lLoadRc = 0;
		// Страховка для строковых функций в MFileReg
		((LPBYTE)*pData)[lSize.LowPart] = 0;
		((LPBYTE)*pData)[lSize.LowPart+1] = 0;
		((LPBYTE)*pData)[lSize.LowPart+2] = 0;
	}
	
	CloseHandle(lhFile);
	return lLoadRc;
}


// Text file operations
// asExtension = L".reg", L".txt"
BOOL MFileTxt::FileCreateTemp(LPCWSTR asDefaultName, LPCWSTR asExtension, BOOL abUnicode)
{
	//HANDLE hFile = NULL;
	int nLen;
	TCHAR szTempDir[MAX_PATH*2+1] = _T("");
	wchar_t szOutFileName[MAX_PATH*3+20] = L"";
	
	// На всякий случай, подчистим переменные
	_ASSERTE(psTempDirectory == NULL);
	SafeFree(psTempDirectory);
	_ASSERTE(psFilePathName == NULL);
	SafeFree(psFilePathName);
	_ASSERTE(psShowFilePathName == NULL);
	SafeFree(psShowFilePathName);
	_ASSERTE(hFile == NULL);
	// И закроем файл
	FileClose();
	
	//_ASSERTE(asExtension && asExtension[0] == L'.' && asExtension[1] != 0);
	if (asDefaultName == NULL)
		asDefaultName = REGEDIT_DEFAULTNAME;
	if (!asExtension)
		asExtension = L""; //L".txt"; -- без расширения, чтобы колорер нормально раскраску подхватывал
	
	// Создать временный каталог
	FSF.MkTemp(szTempDir,
		#ifdef _UNICODE
			MAX_PATH*2,
		#endif
		_T("FREG"));
	_ASSERTE(szTempDir[0] != 0);

	// Сразу запомнить (psTempDirectory), что создали временную папку
	psTempDirectory = MakeUNCPath(szTempDir);
	bLastRc = CreateDirectoryW(psTempDirectory, NULL);
	if (!bLastRc)
	{
		nLastErr = GetLastError();
		REPlugin::MessageFmt(REM_CantCreateTempFolder, psTempDirectory, nLastErr);
		SafeFree(psTempDirectory);
		return (bLastRc = FALSE);
	}

	lstrcpynW(szOutFileName, psTempDirectory, MAX_PATH*2+20);
	nLen = lstrlenW(szOutFileName);
	_ASSERTE(szOutFileName[nLen-1] != _T('\\'));
	szOutFileName[nLen++] = _T('\\'); szOutFileName[nLen] = 0;
	CopyFileName(szOutFileName+nLen, MAX_FILE_NAME-5, asDefaultName);
	lstrcpynW(szOutFileName+lstrlenW(szOutFileName), asExtension, 5);
		
	// Создаем файл (имя файла скопирует он сам)
	bLastRc = FileCreateApi(szOutFileName, abUnicode, FALSE/*abAppendExisting*/);
	
	return bLastRc;
}

BOOL MFileTxt::FileCreate(LPCWSTR asPath/*only directory!*/, LPCWSTR asDefaultName, LPCWSTR asExtension, BOOL abUnicode, BOOL abConfirmOverwrite, BOOL abNoBOM /*= FALSE*/)
{
	int nLen, nFullLen;

	// На всякий случай, подчистим переменные
	_ASSERTE(psTempDirectory == NULL);
	SafeFree(psTempDirectory);
	_ASSERTE(psFilePathName == NULL);
	SafeFree(psFilePathName);
	_ASSERTE(psShowFilePathName == NULL);
	SafeFree(psShowFilePathName);
	_ASSERTE(hFile == NULL);
	// И закроем файл
	FileClose();

	//_ASSERTE(asExtension && asExtension[0] == L'.' && asExtension[1] != 0);
	if (asDefaultName == NULL)
		asDefaultName = REGEDIT_DEFAULTNAME;
	if (!asExtension)
		asExtension = L""; //L".txt"; -- без расширения, чтобы колорер нормально раскраску подхватывал

	if (!asPath || !*asPath)
	{
		//TODO: Использовать текущую папку фара (получать через API)
		_ASSERTE(asPath && *asPath);
		return FALSE;
	}
	else
	{
		//TODO: Развернуть возможные ".\", "..\" и т.п.
		nLen = lstrlenW(asPath);
		//if (asPath[nLen-1] == _T('\\')) nLen--;
	}
	// CopyFileName может корректировать asDefaultName (заменять некорректные символы на #xx)
	nFullLen = nLen + MAX_PATH/*lstrlenW(asDefaultName)*/ + lstrlenW(asExtension) + 20;
	psFilePathName = (wchar_t*)malloc(nFullLen*sizeof(wchar_t));

	nLen = CopyUNCPath(psFilePathName, nFullLen, asPath);
	if (psFilePathName[nLen-1] == L'\\')
	{
		psFilePathName[nLen--] = 0;
	}

	//TODO: Проверить, может директория уже создана
	bLastRc = CreateDirectoryW(psFilePathName, NULL);
	//if (!bLastRc) {
	//	nLastErr = GetLastError();
	//	return (bLastRc = FALSE);
	//}

	//lstrcpy_t(szOutName+nLen, (MAX_FILE_NAME-5), asDefaultName);
	psFilePathName[nLen++] = L'\\';
	CopyFileName(psFilePathName+nLen, MAX_FILE_NAME-((*asExtension) ? 5 : 1), asDefaultName);
	if (*asExtension)
		lstrcpynW(psFilePathName+lstrlenW(psFilePathName), asExtension, 5);

	// Если abConfirmOverwrite и файл существует - подтверждение на перезапись!
	BOOL lbAppendExisting = FALSE;
	if (abConfirmOverwrite)
	{
		if (!REPlugin::ConfirmOverwriteFile(psFilePathName, &lbAppendExisting, &abUnicode))
		{
			bLastRc = TRUE; nLastErr = 0;
			return FALSE;
		}
	}

	// Создаем файл (имя файла скопирует он сам)
	bLastRc = FileCreateApi(psFilePathName, abUnicode, lbAppendExisting, abNoBOM);

	return bLastRc;
}

LPCWSTR MFileTxt::GetFilePathName()
{
	_ASSERTE(psFilePathName != NULL);
	return psFilePathName;
}

LPCTSTR MFileTxt::GetShowFilePathName()
{
	_ASSERTE(psFilePathName != NULL || psShowFilePathName != NULL);
	if (!psShowFilePathName && psFilePathName) {
		psShowFilePathName = UnmakeUNCPath_t(psFilePathName);
	}
	return psShowFilePathName;
}

BOOL MFileTxt::FileCreateApi(LPCWSTR asFilePathName, BOOL abUnicode, BOOL abAppendExisting, BOOL abNoBOM)
{
	FileClose(); // На всякий случай
	
	MFileTxt::bBadMszDoubleZero = FALSE;
	
	// Сразу!
	bUnicode = abUnicode;
	bOneKeyCreated = FALSE;
	if (asFilePathName != psFilePathName)
	{
		SafeFree(psFilePathName);
		psFilePathName = lstrdup(asFilePathName);
	}
	nAnsiCP = cfg->nAnsiCodePage;

	int nNameLen = lstrlenW(asFilePathName)+1;
	psShowFilePathName = (TCHAR*)malloc(nNameLen*sizeof(TCHAR));

	//if (wcsncmp(asFilePathName, _T("\\\\?\\"), 4))
	if (asFilePathName[0 ]== _T('\\')
		&& asFilePathName[1] == _T('\\')
		&& asFilePathName[2] == _T('?') 
		&& asFilePathName[3] == _T('\\'))
	{
		LPCWSTR pFileName = asFilePathName+4;
		if (pFileName[0] == L'U' && pFileName[1] == L'N' && pFileName[2] == L'C' && pFileName[3] == L'\\')
		{
			*psShowFilePathName = '\\';
			lstrcpy_t(psShowFilePathName+1, nNameLen-1, pFileName+3);
		}
		else
		{
			lstrcpy_t(psShowFilePathName, nNameLen, pFileName);
		}
	}
	else
	{
		lstrcpy_t(psShowFilePathName, nNameLen, asFilePathName);
	}
	MCHKHEAP;
	
	if (!ptrWriteBuffer || nWriteBufferSize < 1048576 /* 1Mb */)
	{
		SafeFree(ptrWriteBuffer);
		nWriteBufferSize = 1048576;
		nWriteBufferLen = 0;
		ptrWriteBuffer = (LPBYTE)malloc(nWriteBufferSize);
	}

	// здесь уже должен быть путь в UNC формате!
	_ASSERTE(asFilePathName[0]==L'\\' && asFilePathName[1]==L'\\' && asFilePathName[2]==L'?' && asFilePathName[3]==L'\\');
	// Создаем файл
	hFile = CreateFileW( asFilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
						(abAppendExisting ? OPEN_ALWAYS : CREATE_ALWAYS),
						FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		nLastErr = GetLastError();
		if (!(gnOpMode & (OPM_FIND|OPM_SILENT)))
			REPlugin::MessageFmt(REM_CantCreateTempFile, asFilePathName, nLastErr);
		hFile = NULL;
		return (bLastRc = FALSE);
	}
	
	// Если это НЕ дописывание в конец
	if (abUnicode && !abAppendExisting && !abNoBOM)
	{
		WORD nBOM = 0xFEFF;
		if (!FileWriteBuffered(&nBOM, sizeof(nBOM)))
			return FALSE;
	}

	// Это дописывание в конец
	if (abAppendExisting)
	{
		// Переместить указатель в конец файла
		if (SetFilePointer(hFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
			return FALSE;
		// И на всякий случай, вставить перевод строки
		if (abUnicode)
		{
			if (!FileWriteBuffered(L"\r\n", 4))
				return FALSE;
		}
		else
		{
			if (!FileWriteBuffered("\r\n", 2))
				return FALSE;
		}
	}
		
	return (bLastRc = TRUE);
}

void MFileTxt::FileClose()
{
	if (hFile && hFile != INVALID_HANDLE_VALUE)
	{
		// Если что-то осталось в кеше записи
		if (ptrWriteBuffer && nWriteBufferLen)
		{
			//FileWriteBuffered(NULL, -1); // сбросит кеш на диск
			Flush(); // сбросит кеш на диск
		}
		// И закрыть дескриптор
		CloseHandle(hFile); hFile = NULL;
	}
	SafeFree(ptrWriteBuffer);
	SafeFree(pExportFormatted);
}

void MFileTxt::FileDelete()
{
	// Если еще не закрыт
	FileClose();

	// Удалить файл
	if (psFilePathName)
	{
		if (*psFilePathName)
		{
			bLastRc = DeleteFileW(psFilePathName);
			if (!bLastRc) nLastErr = GetLastError();
		}
		else
		{
			bLastRc = FALSE; nLastErr = -1;
		}
		SafeFree(psFilePathName);
	}
	
	// Удалить созданную нами временную папку
	if (psTempDirectory)
	{
		if (*psTempDirectory)
		{
			int nLen = lstrlenW(psTempDirectory);
			if (psTempDirectory[nLen-1] == _T('\\'))
			{
				psTempDirectory[nLen-1] = 0; nLen--;
			}
			if (psTempDirectory[nLen-1] == _T(':'))
			{
				bLastRc = TRUE;
			}
			else
			{
				bLastRc = RemoveDirectoryW(psTempDirectory);
				if (!bLastRc) nLastErr = GetLastError();
			}
		}
		else
		{
			bLastRc = FALSE; nLastErr = -1;
		}
		SafeFree(psTempDirectory);
	}
}

BOOL MFileTxt::Flush()
{
	_ASSERTE(ptrWriteBuffer != NULL);
	if (nWriteBufferLen > 0 && ptrWriteBuffer)
	{
		DWORD nWritten = 0;
		bLastRc = WriteFile(hFile, ptrWriteBuffer, nWriteBufferLen, &nWritten, NULL);
		if (!bLastRc) {
			nLastErr = GetLastError();
			return FALSE;
		}
		nWriteBufferLen = 0;
	}
	return bLastRc;
}

// Запись в файл REG_MULTI_SZ значений, с преобразованием в человеческий вид (разбиение на строки через \r\n)
BOOL MFileTxt::FileWriteMSZ(LPCWSTR aszText, DWORD anLen)
{
	const wchar_t* psz = aszText;
	const wchar_t* pszEnd = aszText + anLen;
	#ifdef _DEBUG
	if (pszEnd > psz)
	{
		_ASSERTE(*(pszEnd-1) == 0);
	}
	#endif
	BOOL lbExportRc = TRUE; // Может быть пустой!
	int nDoubleZero = 0;
	while (psz < pszEnd)
	{
		if (*psz)
		{
			int nLen = lstrlenW(psz);
			if (!(lbExportRc = FileWrite(psz, nLen)))
				break; // ошибка записи
			psz += nLen+1;
			if (nDoubleZero>0) MFileTxt::bBadMszDoubleZero = TRUE;
			nDoubleZero = 0;
		}
		else
		{
			if ((psz+1) >= pszEnd)
				break;
			psz++; // просто записать перевод строки
			nDoubleZero++;
		}
		// Просто "\n" пишется для унификации пустых строк - они в реестр заносятся как "\n"
		if (!(lbExportRc = FileWrite(L"\n", 1)))
			break; // ошибка записи
	}
	if (nDoubleZero>1) MFileTxt::bBadMszDoubleZero = TRUE;
	return lbExportRc;
}

BOOL MFileTxt::FileWriteBuffered(LPCVOID apData, DWORD nDataSize)
{
	_ASSERTE(ptrWriteBuffer != NULL);
	DWORD nWritten = 0;
	
	if (!apData)
	{
		// Сбросить на диск
		_ASSERTE(apData == NULL && nDataSize == (DWORD)-1);
		Flush();
		return (bLastRc = TRUE);
	}
	// Если ничего не нужно
	if (nDataSize == 0)
	{
		return (bLastRc = TRUE);
	}
	
	if (ptrWriteBuffer)
	{
		int nLeft = nWriteBufferSize - nWriteBufferLen;
		_ASSERTE(nLeft >= 0);
		
		// Если буфер заполнился (почему? он уже должен был быть сброшен)
		// или размер данных превышает оставшийся свободный блок
		if (nLeft <= 0 || nDataSize >= (DWORD)nLeft)
		{
			// Сбросить текущий буфер на диск
			//if (!FileWriteBuffered(NULL, -1))
			if (!Flush())
				return FALSE;
			_ASSERTE(nWriteBufferLen==0);
			nWriteBufferLen = 0;
			nLeft = nWriteBufferSize;
		}
		
		// Если размер данных превышает (или равен) размеру выделенного буфера - сразу пишем на диск
		if (nDataSize >= nWriteBufferSize)
		{
			_ASSERTE(nWriteBufferLen == 0);
			bLastRc = WriteFile(hFile, apData, nDataSize, &nWritten, NULL);
			if (!bLastRc) nLastErr = GetLastError();
			return bLastRc; // Сразу выходим, уже записали или ошибка
		}

		// Скопировать в буфер и передвинуть указатель
		_ASSERTE(nDataSize <= (DWORD)nLeft && nLeft > 0);
		memmove(ptrWriteBuffer+nWriteBufferLen, apData, nDataSize);
		nWriteBufferLen += nDataSize;
		
		// Если буфер заполнился - сбросить на диск
		if (nWriteBufferLen == nWriteBufferSize)
		{
			//if (!FileWriteBuffered(NULL, -1))
			if (!Flush())
				return FALSE;
			_ASSERTE(nWriteBufferLen==0);
			nWriteBufferLen = 0;
		}
		bLastRc = TRUE;
		
	}
	else
	{
		// Буфер не был создан?
		bLastRc = WriteFile(hFile, apData, nDataSize, &nWritten, NULL);
		if (!bLastRc) nLastErr = GetLastError();
	}
	
	return bLastRc;
}

BOOL MFileTxt::FileWrite(LPCWSTR aszText, int anLen/*=-1*/)
{
	if (aszText == NULL)
	{
		_ASSERTE(aszText != NULL);
		nLastErr = -1;
		return (bLastRc = FALSE);
	}
	
	if (anLen == -1)
	{
		anLen = lstrlenW(aszText);
	}
	_ASSERTE(anLen >= 0);
	
	// Если писать нечего
	if (anLen == 0)
	{
		nLastErr = 0;
		return (bLastRc = TRUE);
	}

	if (bUnicode)
	{
		bLastRc = FileWriteBuffered(aszText, anLen*2);
		if (!bLastRc) nLastErr = GetLastError();
	}
	else
	{
		_ASSERTE(anLen < (anLen*3));
		char* pszAscii = (char*)GetConvertBuffer(anLen*3); // Может кто-то UTF-8 захочет?
		int nCvtLen = WideCharToMultiByte(nAnsiCP/*CP_ACP*/, 0, aszText, anLen, pszAscii, anLen*3, 0,0);
		bLastRc = FileWriteBuffered(pszAscii, nCvtLen);
		if (!bLastRc) nLastErr = GetLastError();
	}
	
	return bLastRc;
}

// --> pExportBufferData
LPBYTE MFileTxt::GetExportBuffer(DWORD cbSize)
{
	if (!pExportBufferData || cbSize > cbExportBufferData)
	{
		SafeFree(pExportBufferData);
		cbExportBufferData = cbSize;
		pExportBufferData = (LPBYTE)malloc(cbExportBufferData);
	}
	return pExportBufferData;
}

// --> pExportFormatted
LPBYTE MFileTxt::GetFormatBuffer(DWORD cbSize)
{
	// буфер должен быть достаточно большим для принятия 
	// отформатированных hex данных и имени значения реестра
	unsigned __int64 tSize = (cbSize*16)+MAX_FORMAT_KEY_SIZE;
	if (tSize != (tSize & 0xFFFFFFFF))
	{
		// В принципе, размер значений ограничен только объемом памяти
		// но должны же быть какие-то пределы!
		_ASSERTE(tSize == (tSize & 0xFFFFFFFF));
		return NULL;
	}
	else
	{
		cbSize = (DWORD)tSize;
	}
	
	if (!pExportFormatted || cbSize > cbExportFormatted)
	{
		SafeFree(pExportFormatted);
		cbExportFormatted = cbSize;
		pExportFormatted = (LPBYTE)malloc(cbExportFormatted);
	}
	return pExportFormatted;
}

LPBYTE MFileTxt::GetConvertBuffer(DWORD cbSize) // --> pExportCPConvert
{
	_ASSERTE(cbSize!=0);
	
	if (!pExportCPConvert || cbSize > cbExportCPConvert)
	{
		SafeFree(pExportCPConvert);
		cbExportCPConvert = cbSize;
		pExportCPConvert = (LPBYTE)malloc(cbExportCPConvert);
	}
	return pExportCPConvert;
}


//LONG MFileTxt::NotifyChangeKeyValue(RegFolder *pFolder, HKEY hKey)
//{
//	return 0;
//}

// Wrappers
LONG MFileTxt::SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment /*= NULL*/)
{
	BOOL lbRc = FileWriteValue(lpValueName, nDataType, lpData, cbData, pszComment);
	return (lbRc ? 0 : -1);
}

//LONG MFileTxt::OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/)
//{
//	return -1;
//}
//LONG MFileTxt::CloseKey(HKEY* phKey)
//{
//	return 0;
//}
//LONG MFileTxt::QueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime)
//{
//	return -1;
//}
//LONG MFileTxt::EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment /*= NULL*/)
//{
//	// pParent->dwValIndex = -1; pParent->pValIndex = -1;
//	return -1;
//}
//LONG MFileTxt::EnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime, DWORD* pnKeyFlags /*= NULL*/, TCHAR* lpDefValue /*= NULL*/, DWORD cchDefValueMax /*= 0*/, LPCWSTR* ppszKeyComment /*= NULL*/)
//{
//	// pParent->dwKeyIndex = -1; pParent->pKeyIndex = -1;
//	return -1;
//}
//LONG MFileTxt::QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment /*= NULL*/)
//{
//	return -1;
//}
//LONG MFileTxt::RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged)
//{
//	return -1;
//}

//int MFileTxt::CopyPath(wchar_t* pszDest, const char* pszSrc, int nMaxChars)
//{
//	bool lbIsFull = false, lbIsNetwork = false;
//	if (pszSrc[0] == '\\' && pszSrc[1] == '\\') {
//		if ((pszSrc[2] == '?' || pszSrc[2] == '.') && pszSrc[3] == '\\') {
//			lbIsFull = true;
//		} else {
//			lbIsNetwork = true;
//		}
//	}
//	// "\\?\" X:\...
//	// "\\?\UNC\" server\share\...
//	if (lbIsFull) {
//		lstrcpy_t(pszDest, nMaxChars, pszSrc);
//	} else {
//		lstrcpyW(pszDest, L"\\\\?\\");
//		if (lbIsNetwork) {
//			lstrcpyW(pszDest+4, L"UNC\\");
//			lstrcpy_t(pszDest+8, nMaxChars-8, pszSrc+2);
//		} else {
//			lstrcpy_t(pszDest+4, nMaxChars-4, pszSrc);
//		}
//	}
//
//	int nLen = lstrlenW(pszDest);
//	return nLen;
//}
//
//int MFileTxt::CopyPath(wchar_t* pszDest, const wchar_t* pszSrc, int nMaxChars)
//{
//	bool lbIsFull = false, lbIsNetwork = false;
//	if (pszSrc[0] == L'\\' && pszSrc[1] == L'\\') {
//		if ((pszSrc[2] == L'?' || pszSrc[2] == L'.') && pszSrc[3] == L'\\') {
//			lbIsFull = true;
//		} else {
//			lbIsNetwork = true;
//		}
//	}
//	// "\\?\" X:\...
//	// "\\?\UNC\" server\share\...
//	if (lbIsFull) {
//		lstrcpyW(pszDest, pszSrc);
//	} else {
//		lstrcpyW(pszDest, L"\\\\?\\");
//		if (lbIsNetwork) {
//			lstrcpyW(pszDest+4, L"UNC\\");
//			lstrcpyW(pszDest+8, pszSrc+2);
//		} else {
//			lstrcpyW(pszDest+4, pszSrc);
//		}
//	}
//
//	int nLen = lstrlenW(pszDest);
//	return nLen;
//}



MFileTxtReg::MFileTxtReg(BOOL abWow64on32, BOOL abVirtualize)
	: MFileTxt(abWow64on32, abVirtualize)
{
}

MFileTxtReg::~MFileTxtReg()
{
}

BOOL MFileTxtReg::FileWriteHeader(MRegistryBase* pWorker)
{
	BOOL lbExportRc = TRUE;

	if (bUnicode)
	{
		// (BOM уже записан в file.FileCreateTemp)
		lbExportRc = FileWrite(L"Windows Registry Editor Version 5.00\r\n");
	}
	else
	{
		LPCSTR pszHeader = "REGEDIT4\r\n";
		int nSize = lstrlenA(pszHeader);
		lbExportRc = FileWriteBuffered(pszHeader, nSize);
	}

	if (lbExportRc && pWorker && pWorker->eType == RE_HIVE)
	{
		//lbExportRc = file.FileWriteBuffered(pszHeader, nSize);
		//lbExportRc = file.FileWriteBuffered(pszHeader, nSize);
		wchar_t* pwszHost = UnmakeUNCPath_w(((MFileHive*)pWorker)->GetFilePathName());
		lbExportRc = FileWrite(L"\r\n;RootFile=") &&
			FileWrite(pwszHost) &&
			FileWrite(L"\r\n");
		SafeFree(pwszHost);
	}

	return lbExportRc;
}

BOOL MFileTxtReg::FileWriteValue(LPCWSTR pszValueName, REGTYPE nDataType, const BYTE* pData, DWORD nDataSize, LPCWSTR pszComment)
{
	// nDataSize - в байтах
	wchar_t* psz = (wchar_t*)GetFormatBuffer(max(0xFFFF,nDataSize));
	
	if (nDataType == REG__KEY)
	{
		_ASSERTE(nDataType!=REG__KEY);
		return FALSE;
	}

	if (nDataType == REG__COMMENT)
	{
		if (!bOneKeyCreated)
		{
			bOneKeyCreated = TRUE;
			if (!FileWrite(L"\r\n", 2))
				return FALSE;
		}

		if (!FileWrite(pszValueName, -1))
			return FALSE;
		//if (pszComment && !FileWrite(pszComment, -1))
		//	return FALSE;
		if (!FileWrite(L"\r\n", 2))
			return FALSE;
		return TRUE;
	}

	// Для REG_SZ это не нужно, конвертацию сделает FileWrite
	// А вот REG_EXPAND_SZ/REG_MULTI_SZ выкидываются как hex, поэтому нужно конвертить в ANSI
	if (!bUnicode && nDataSize && (nDataType == REG_EXPAND_SZ || nDataType == REG_MULTI_SZ))
	{
		size_t nLen = (nDataSize>>1);
		_ASSERTE(nLen < (nLen*3));
		char* pszAscii = (char*)GetConvertBuffer(nLen*3+1); // Может кто-то UTF-8 захочет?
		int nCvtLen = WideCharToMultiByte(nAnsiCP/*CP_ACP*/, 0, (LPCWSTR)pData, nLen, pszAscii, nLen*3+1, 0,0);
		_ASSERTE(nCvtLen > 0);
		pData = (LPBYTE)pszAscii; nDataSize = nCvtLen;
	}

	if (pszValueName && *pszValueName)
	{
		*(psz++) = L'\"';
		//lstrcpyW(psz, pszValueName); psz += lstrlenW(pszValueName);
		// escaped
		BOOL bCvtRN = cfg->bEscapeRNonExporting;
		const wchar_t* pszSrc = (wchar_t*)pszValueName;
		wchar_t ch;
		while ((ch = *(pszSrc++)) != 0)
		{
			switch (ch)
			{
			case L'\r':
				if (bCvtRN)
				{
					*(psz++) = L'\\'; *(psz++) = L'r';
				}
				else
				{
					*(psz++) = ch;
				}
				break;
			case L'\n':
				if (bCvtRN)
				{
					*(psz++) = L'\\'; *(psz++) = L'n';
				}
				else
				{
					*(psz++) = ch;
				}
				break;
			//TODO: заменять символ табуляции?
			//case L'\t':
			//	*(psz++) = L'\\'; *(psz++) = L't'; break;
			case L'\"':
				*(psz++) = L'\\'; *(psz++) = L'"'; break;
			case L'\\':
				*(psz++) = L'\\'; *(psz++) = L'\\'; break;
			default:
				*(psz++) = ch;
			}
		}
		*(psz++) = L'\"';
	}
	else
	{
		*(psz++) = L'@';
	}
	*(psz++) = L'=';

	//wchar_t* pszValueStart = psz;

	if (nDataType == REG__DELETE)
	{
		*(psz++) = L'-';
	}
	else if (nDataType == REG_SZ)
	{
		//TODO: Если встречается перевод каретки - можно опционально переформатировать в режиме "hex(1):"
		const wchar_t* pszSrc = (wchar_t*)pData;
		wchar_t ch;
		DWORD nLen = nDataSize>>1;
		if (nLen>0 && pszSrc[nLen-1] == 0) nLen--;
		*(psz++) = L'\"';
		BOOL bEscRN = cfg->bEscapeRNonExporting;
		while (nLen--)
		{
			switch ((ch = *(pszSrc++)))
			{
			case 0:
				*(psz++) = L'\\'; *(psz++) = L'0';
				break;
			case L'\r':
				if (bEscRN)
				{
					*(psz++) = L'\\'; *(psz++) = L'r';
				}
				else
				{
					*(psz++) = ch;
				}
				break;
			case L'\n':
				if (bEscRN)
				{
					*(psz++) = L'\\'; *(psz++) = L'n';
				}
				else
				{
					*(psz++) = ch;
				}
				break;
			//TODO: заменять символ табуляции?
			//case L'\t':
			//	*(psz++) = L'\\'; *(psz++) = L't'; break;
			case L'\"':
				*(psz++) = L'\\'; *(psz++) = L'"'; break;
			case L'\\':
				*(psz++) = L'\\'; *(psz++) = L'\\'; break;
			default:
				*(psz++) = ch;
			}
		}
		*(psz++) = L'\"';

	}
	else if (nDataType == REG_DWORD && nDataSize == 4)
	{
		//TODO: ручками
		wsprintfW(psz, L"dword:%08x", *((DWORD*)pData));
		psz += lstrlenW(psz);
		
	}
	else
	{
		if (nDataType == REG_BINARY)
		{
			*(psz++) = L'h'; *(psz++) = L'e'; *(psz++) = L'x'; *(psz++) = L':';
		}
		else
		{
			wsprintfW(psz, L"hex(%x):", nDataType);
			psz += lstrlenW(psz);
		}
		if (nDataSize)
		{
			wchar_t* ph = pszExportHexValues+(*(pData++))*2;
			*(psz++) = *(ph++);
			*(psz++) = *(ph++);
			wchar_t* pszLineStart = (wchar_t*)pExportFormatted;
			for (UINT n = 1; n < nDataSize; n++)
			{
				ph = pszExportHexValues+(*(pData++))*2;
				*(psz++) = L',';
				if ((psz - pszLineStart) >= 77)
				{
					*(psz++) = L'\\'; *(psz++) = L'\r'; *(psz++) = L'\n'; *(psz++) = L' '; *(psz++) = L' ';
					pszLineStart = psz - 2;
				}
				*(psz++) = *(ph++);
				*(psz++) = *(ph++);
			}
		}
	}
	//*(psz++) = L'\r'; *(psz++) = L'\n';
	
	int nLen = (int)(psz - ((wchar_t*)pExportFormatted));

	if (!FileWrite((LPCWSTR)pExportFormatted, nLen))
		return FALSE;

	if (pszComment && !FileWrite(pszComment, -1))
		return FALSE;

	if (!FileWrite(L"\r\n", 2))
		return FALSE;

	return TRUE;
}

LONG MFileTxtReg::CreateKeyEx(
		HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags,
		RegKeyOpenRights *apRights /*= NULL*/, LPCWSTR pszComment /*= NULL*/)
{
	if (!FileWrite(L"\r\n", 2))
		return -1;

	BOOL bKeyDeletion = (dwOptions & REG__OPTION_CREATE_DELETED) == REG__OPTION_CREATE_DELETED;

	bOneKeyCreated = TRUE;

	wchar_t sTemp[64];
	wchar_t* pszTemp = sTemp;
	*(pszTemp++) = L'[';
	if (bKeyDeletion)
		*(pszTemp++) = L'-';
	if (hKey == NULL)
	{
		if (!lpSubKey || !*lpSubKey)
		{
			_ASSERTE(lpSubKey && *lpSubKey);
			return -1;
		}

		*pszTemp = 0;
		if (!FileWrite(sTemp) ||
			!FileWrite(lpSubKey))
			return -1;
	}
	else
	{
		HKeyToStringKey(hKey, pszTemp, 40);

		// Заголовок ключа
		if (!FileWrite(sTemp))
			return -1;

		if (lpSubKey && lpSubKey[0])
		{
			if (!FileWrite(L"\\", 1) ||
				!FileWrite(lpSubKey))
				return -1;
		}
	}
	if (pszComment)
	{
		if (!FileWrite(L"]", 1) ||
			!FileWrite(pszComment) ||
			!FileWrite(L"\r\n", 2))
			return -1;
	}
	else
	{
		if (!FileWrite(L"]\r\n", 3))
			return -1;
	}

	// Что-то отличное от NULL нужно вернуть!
	*phkResult = HKEY__HIVE;

	//bKeyWasCreated = TRUE;
	return 0;
}





MFileTxtCmd::MFileTxtCmd(BOOL abWow64on32, BOOL abVirtualize)
	: MFileTxt(abWow64on32, abVirtualize)
{
	nKeyBufferSize = 16384;
	psKeyBuffer = (char*)malloc(nKeyBufferSize);
	if (psKeyBuffer)
		psKeyBuffer[0] = 0;
	bRequestUnicode = FALSE;
	
	mb_KeyWriteWaiting = FALSE;
	mb_LastKeyHasValues = FALSE;
	mn_LastSubKeyLen = 0;
	mn_LastSubKeyCch = nKeyBufferSize;
	mh_LastRoot = NULL;
	ms_LastSubKey = (wchar_t*)malloc(mn_LastSubKeyCch*sizeof(wchar_t));
	if (ms_LastSubKey)
		ms_LastSubKey[0] = 0;

	#ifndef _UNICODE
	ms_RECmdHeader = GetMsg(RECmdHeader);   // "@echo off\r\nrem \r\nrem Reg.exe has several restrictions\r\nrem * REG_QWORD (and other 'specials') is not supported\r\nrem * Strings, value names and keys can't contains \\r\\n\r\nrem * REG_MULTI_SZ may fails, if value contains \\\\0\r\nrem * Empty keys can't be created w/o default value\r\nrem \r\n"
	ms_RECmdVarReset = GetMsg(RECmdVarReset); // "set FarRegEditRc=""""\r\n"
	ms_RECmdUTF8Warn = GetMsg(RECmdUTF8Warn); // "\r\nrem Required for Unicode string support"
	ms_RECmdErrCheck = GetMsg(RECmdErrCheck); // "if errorlevel 1 if %FarRegEditRc%=="""" set FarRegEditRc="
	ms_RECmdErrFin = GetMsg(RECmdErrFin);   // "if NOT %FarRegEditRc%=="""" echo At least on value/key failed: %FarRegEditRc%\r\n"
	#else
	wchar_t* pwsz;
	int nWLen;
	struct { int nMsgId; char** ppsz; int* pnlen; } Vars[] = {
		{ RECmdHeader, &ms_RECmdHeader, &mn_RECmdHeader },
		{ RECmdVarReset, &ms_RECmdVarReset, &mn_RECmdVarReset },
		{ RECmdUTF8Warn, &ms_RECmdUTF8Warn, &mn_RECmdUTF8Warn },
		{ RECmdErrCheck, &ms_RECmdErrCheck, &mn_RECmdErrCheck },
		{ RECmdErrFin, &ms_RECmdErrFin, &mn_RECmdErrFin}
	};
	for (UINT i = 0; i < countof(Vars); i++)
		*Vars[i].ppsz = NULL;
	for (UINT i = 0; i < countof(Vars); i++)
	{
		pwsz = GetMsg(Vars[i].nMsgId);
		nWLen = lstrlenW(pwsz);
		*Vars[i].ppsz = (char*)calloc((nWLen+1),3);
		if (*Vars[i].ppsz == NULL)
		{
			InvalidOp();
			break;
		}
		WideCharToMultiByte(CP_UTF8, 0, pwsz, nWLen+1, *Vars[i].ppsz, (nWLen+1)*3, 0,0);
		*Vars[i].pnlen = lstrlenA(*Vars[i].ppsz);
	}
	#endif
	mn_RECmdHeader = ms_RECmdHeader ? lstrlenA(ms_RECmdHeader) : 0;
	mn_RECmdVarReset = ms_RECmdVarReset ? lstrlenA(ms_RECmdVarReset) : 0;
	mn_RECmdUTF8Warn = ms_RECmdUTF8Warn ? lstrlenA(ms_RECmdUTF8Warn) : 0;
	mn_RECmdErrCheck = ms_RECmdErrCheck ? lstrlenA(ms_RECmdErrCheck) : 0;
	mn_RECmdErrFin = ms_RECmdErrFin ? lstrlenA(ms_RECmdErrFin) : 0;
}

MFileTxtCmd::~MFileTxtCmd()
{
	SafeFree(psKeyBuffer);
	SafeFree(ms_LastSubKey);
	
	#ifdef _UNICODE
	SafeFree(ms_RECmdHeader);
	SafeFree(ms_RECmdVarReset);
	SafeFree(ms_RECmdUTF8Warn);
	SafeFree(ms_RECmdErrCheck);
	SafeFree(ms_RECmdErrFin);
	#endif
}

BOOL MFileTxtCmd::FileWriteHeader(MRegistryBase* pWorker)
{
	BOOL lbExportRc = TRUE;
	//LPCSTR pszHeader;
	//int nSize;
	
	bRequestUnicode = bUnicode;
	bUnicode = FALSE;
	nAnsiCP = bRequestUnicode ? CP_UTF8 : GetOEMCP();
	
	//pszHeader = 
	//	"@echo off" "\r\n"
	//	"rem " "\r\n"
	//	"rem Reg.exe has several restrictions" "\r\n"
	//	"rem * REG_QWORD (and other 'specials') is not supported" "\r\n"
	//	"rem * Strings, value names and keys can't contains \\r\\n" "\r\n"
	//	"rem * REG_MULTI_SZ may fails, if value contains \\\\0" "\r\n"
	//	"rem * Empty keys can't be created w/o default value" "\r\n"
	//	"rem " "\r\n"
	//	;
	//nSize = lstrlenA(pszHeader);
	//lbExportRc = FileWriteBuffered(pszHeader, nSize);
	if (lbExportRc && ms_RECmdHeader && *ms_RECmdHeader)
	{
		lbExportRc = FileWriteBuffered(ms_RECmdHeader, mn_RECmdHeader);
	}

	// "set FarRegEditRc=""""\r\n"
	if (lbExportRc && ms_RECmdVarReset && *ms_RECmdVarReset)
	{
		lbExportRc = FileWriteBuffered(ms_RECmdVarReset, mn_RECmdVarReset);
	}

	// "\r\nrem Required for Unicode string support"
	if (lbExportRc)
	{
		if (bRequestUnicode && ms_RECmdUTF8Warn && *ms_RECmdUTF8Warn)
		{
			lbExportRc = FileWriteBuffered(ms_RECmdUTF8Warn, mn_RECmdUTF8Warn);
		}
	}

	if (lbExportRc)
	{
		char szCHCP[64];
		wsprintfA(szCHCP, "\r\nchcp %u\r\n\r\n", nAnsiCP);
		lbExportRc = FileWriteBuffered(szCHCP, lstrlenA(szCHCP));
	}

	return lbExportRc;
}

BOOL MFileTxtCmd::FileWriteValue(LPCWSTR pszValueName, REGTYPE nDataType, const BYTE* pData, DWORD nDataSize, LPCWSTR pszComment)
{
	if (nDataType == REG__KEY)
	{
		_ASSERTE(nDataType!=REG__KEY);
		InvalidOp();
		return FALSE;
	}
	if (!psKeyBuffer || !*psKeyBuffer)
	{
		InvalidOp();
		return FALSE;
	}

	if (mb_KeyWriteWaiting) // поскольку создается значение, то САМ ключ явно создавать не нужно
		mb_KeyWriteWaiting = FALSE;
	if (!mb_LastKeyHasValues)
		mb_LastKeyHasValues = TRUE;
	
	char* pszCmd = (char*)GetFormatBuffer(max(0xFFFF,nDataSize));
	if (!pszCmd)
	{
		InvalidOp();
		return FALSE;
	}
	*pszCmd = 0;
	
	if (nDataType == REG__COMMENT)
	{
		//TODO: Выкинуть в cmd комментарии как "rem ...", учесть, что они могут содержать \r\n
#if 0
		if (!bOneKeyCreated)
		{
			bOneKeyCreated = TRUE;
			if (!FileWrite(L"\r\n", 2))
				return FALSE;
		}

		if (!FileWrite(pszValueName, -1))
			return FALSE;
		//if (pszComment && !FileWrite(pszComment, -1))
		//	return FALSE;
		if (!FileWrite(L"\r\n", 2))
			return FALSE;
#endif
		return TRUE;
	}
	
	char* psz = pszCmd;
	if (!PrepareCmd(psz, (nDataType == REG__DELETE) ? "DELETE" : "ADD", psKeyBuffer))
		return -1; // ошибка уже показана

	char* pszValueNameA = NULL; size_t nValueNameA = 0;
		
	if (!pszValueName || !*pszValueName)
	{
		// "/ve" - значение по умолчанию
		*(psz++) = '/'; *(psz++) = 'v'; *(psz++) = 'e'; *(psz++) = ' '; *psz = 0;
	}
	else
	{
		if (wcspbrk(pszValueName, L"\r\n\""))
		{
			REPlugin::MessageFmt(REM_CmdBadValueName, pszValueName);
			return -1;
		}
	
		*(psz++) = '/'; *(psz++) = 'v'; *(psz++) = ' '; *(psz++) = '"'; *psz = 0;
		int nCchLeft = cbExportFormatted - (int)(psz - pszCmd);
		pszValueNameA = psz;
		if (!PrepareString(psz, pszValueName, lstrlenW(pszValueName), nCchLeft))
			return -1; // ошибка уже показана
		nValueNameA = psz - pszValueNameA;
		*(psz++) = '"'; *(psz++) = ' '; *psz = 0; // закрыть кавыку
	}
	
	if (nDataType == REG__DELETE)
	{
		// больше ничего писать не нужно
	}
	else if (nDataType == REG_SZ || nDataType == REG_EXPAND_SZ)
	{
		lstrcpyA(psz, (nDataType == REG_SZ) ? "/t REG_SZ " : "/t REG_EXPAND_SZ ");
		psz += lstrlenA(psz);
		
		if ((nDataSize >=2 ) && pData && *((wchar_t*)pData))
		{
			*(psz++) = '/'; *(psz++) = 'd'; *(psz++) = ' '; *(psz++) = '"'; *psz = 0;
			int nCchLeft = cbExportFormatted - (int)(psz - pszCmd);
			if (!PrepareString(psz, (wchar_t*)pData, nDataSize>>1, nCchLeft))
				return -1; // ошибка уже показана
			*(psz++) = '"'; *psz = 0; // закрыть кавыку
		}
		
	}
	else if (nDataType == REG_MULTI_SZ)
	{
		lstrcpyA(psz, "/t REG_MULTI_SZ ");
		psz += lstrlenA(psz);

		if ((nDataSize >=2 ) && pData && *((wchar_t*)pData))
		{
			//*(psz++) = '/'; *(psz++) = 'd'; *(psz++) = ' '; *(psz++) = '"'; *psz = 0;
			int nCchLeft = cbExportFormatted - (int)(psz - pszCmd);
			if (!PrepareMSZ(psz, (wchar_t*)pData, nDataSize>>1, nCchLeft, pszValueName))
				return -1; // ошибка уже показана
			//*(psz++) = '"'; *psz = 0; // закрыть кавыку
		}
		//return FALSE;
	}
	else if ((nDataType == REG_DWORD) && (nDataSize == sizeof(DWORD)))
	{
		wsprintfA(psz, "/t REG_DWORD /d %u", *((DWORD*)pData));
		psz += lstrlenA(psz);
	}
	else
	{
		// Все остальное - как Binary
		lstrcpyA(psz, "/t REG_BINARY ");
		psz += lstrlenA(psz);
		
		if (nDataSize)
		{
			*(psz++) = '/'; *(psz++) = 'd'; *(psz++) = ' '; *psz = 0;
			for (UINT i = 0; i < nDataSize; i++)
			{
				//TODO: //OPTIMIZE: !!!
				wsprintfA(psz, "%02X", (DWORD)(pData[i]));
				psz += 2;
			}
			*psz = 0;
		}
	}
	
	if (!FileWriteBuffered(pszCmd, (int)(psz - pszCmd)))
		return FALSE;

	if (!FileWriteBuffered("\r\n", 2))
		return FALSE;
		
	if (mn_RECmdErrCheck)
	{
		if (pszValueNameA && nValueNameA)
			pszValueNameA[nValueNameA] = 0;
		if (!WriteRegCheck((nDataType == REG__DELETE), (pszValueNameA && *pszValueNameA) ? pszValueNameA : "(Default)"))
			return FALSE;
	}

	return TRUE;
}

LONG MFileTxtCmd::CreateKeyEx(
		HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags,
		RegKeyOpenRights *apRights /*= NULL*/, LPCWSTR pszComment /*= NULL*/)
{
	BOOL bKeyDeletion = (dwOptions & REG__OPTION_CREATE_DELETED) == REG__OPTION_CREATE_DELETED;
	
	LPCSTR pszFullKey = PrepareKey(hKey, lpSubKey);
	if (!pszFullKey)
		return -1; // ошибка уже показана
	_ASSERTE(pszFullKey == psKeyBuffer);
	
	if (bKeyDeletion)
	{
		if (!WriteCurrentKey(bKeyDeletion, TRUE))
			return -1;
	}
	else if (mb_LastKeyHasValues)
	{
		mb_LastKeyHasValues = FALSE;
		// cmd-файлы строго в однобайтовых кодировках (с UTF-8)
		if (!FileWriteBuffered("\r\n", 2))
			return FALSE;
	}
		
	// Если в ключе будут создаваться подключи или значения,
	// то не нужно создавать сам ключ,
	// а то "REG" создаст в нем пустое и не нужное значение по умолчанию
	if (!mb_KeyWriteWaiting) // если еще не...
	{
		if (!IsPredefined(hKey) || (lpSubKey && *lpSubKey))
			mb_KeyWriteWaiting = TRUE;
	}
	
	// Что-то отличное от NULL нужно вернуть!
	*phkResult = HKEY__HIVE;

	return 0;
}

BOOL MFileTxtCmd::WriteCurrentKey(BOOL bKeyDeletion, BOOL bAddRN)
{
	if (!mb_KeyWriteWaiting)
		return TRUE; // ничего не ожидается

	if (bAddRN)
	{
		// cmd-файлы строго в однобайтовых кодировках (с UTF-8)
		if (!FileWriteBuffered("\r\n", 2))
			return FALSE;
	}

	// Если в ключе будут создаваться подключи или значения,
	// то не нужно создавать сам ключ,
	// а то "REG" создаст в нем пустое и не нужное значение по умолчанию
	mb_KeyWriteWaiting = FALSE; // чтобы повторно не писать
	
	char* pszCmd = (char*)GetFormatBuffer(255); // без разницы, здесь можно и 0 передать
	if (!pszCmd)
	{
		InvalidOp();
		return FALSE;
	}
	
	char* psz = pszCmd;
	*pszCmd = 0;
	if (!PrepareCmd(psz, bKeyDeletion ? "DELETE" : "ADD", psKeyBuffer))
		return FALSE; // ошибка уже показана

	if (!FileWriteBuffered(pszCmd, lstrlenA(pszCmd)) ||
		!FileWriteBuffered("\r\n", 2))
	{
		return FALSE;
	}

	if (mn_RECmdErrCheck)
	{
		if (!WriteRegCheck(bKeyDeletion, NULL))
			return FALSE;
	}
	
	//bKeyWasCreated = TRUE;
	return TRUE;
}

// Сконвертить psKey в UTF8
// Может вернуть NULL, если ключ содержит недопустимые для cmd символы
// в этом случае ошибка уже показана
LPCSTR MFileTxtCmd::PrepareKey(HKEY hKey, LPCWSTR lpSubKey)
{
	DWORD nSubKeyLen = lpSubKey ? lstrlenW(lpSubKey) : 0;
	
	if (!ms_LastSubKey)
	{
		InvalidOp();
		return NULL;
	}
	if (nSubKeyLen >= mn_LastSubKeyCch)
	{
		InvalidOp();
		return NULL;
	}
	
	if (mb_KeyWriteWaiting)
	{
		BOOL lbParentChanged = TRUE;
		if ((hKey == mh_LastRoot) && nSubKeyLen)
		{
			if ((nSubKeyLen > mn_LastSubKeyLen)
				&& (lpSubKey[mn_LastSubKeyLen] == L'\\'))
			{
				if (memcmp(ms_LastSubKey, lpSubKey, mn_LastSubKeyLen*sizeof(wchar_t)) == 0)
					lbParentChanged = FALSE;
			}
		}
		if (lbParentChanged)
		{
			// Сменился родительский ключ, нужно таки создать пустой текущий ключ
			BOOL lbAddRN = mb_LastKeyHasValues; mb_LastKeyHasValues = FALSE;
			if (!WriteCurrentKey(FALSE, lbAddRN)
				|| !FileWriteBuffered("\r\n", 2))
			{
				if (psKeyBuffer)
					*psKeyBuffer = 0; // чтобы в cmd-случайно не попали значения от "не созданного" ключа
				return NULL;
			}
		}
	}
	// сразу сохранить копию последнего ключа
	mh_LastRoot = hKey; mn_LastSubKeyLen = nSubKeyLen;
	lstrcpyW(ms_LastSubKey, lpSubKey ? lpSubKey : L"");
	

	// поехали
	LPCSTR pszRoot = NULL;
	if (hKey == HKEY_CURRENT_USER)
		pszRoot = "HKCU";
	else if (hKey == HKEY_LOCAL_MACHINE)
		pszRoot = "HKLM";
	else if (hKey == HKEY_USERS)
		pszRoot = "HKU";
	else if (hKey == HKEY_CLASSES_ROOT)
		pszRoot = "HKCR";
	else if (hKey == HKEY_CURRENT_CONFIG)
		pszRoot = "HKCC";
	else
	{
		InvalidOp();
		if (psKeyBuffer)
			*psKeyBuffer = 0; // чтобы в cmd-случайно не попали значения от "не созданного" ключа
		return NULL;
	}
	
	if (lpSubKey && *lpSubKey)
	{
		if (wcspbrk(lpSubKey, L"\r\n\""))
		{
			REPlugin::MessageFmt(REM_CmdBadKeyName, lpSubKey);
			if (psKeyBuffer)
				*psKeyBuffer = 0; // чтобы в cmd-случайно не попали значения от "не созданного" ключа
			return NULL;
		}
	}

	DWORD nLen = lstrlenA(pszRoot) + (lpSubKey ? lstrlenW(lpSubKey) : 0) + 2;
	if (!psKeyBuffer || ((nLen * 6) > nKeyBufferSize))
	{
		SafeFree(psKeyBuffer);
		nKeyBufferSize = (nLen * 6);
		psKeyBuffer = (char*)malloc(nKeyBufferSize);
		if (!psKeyBuffer)
		{
			InvalidOp();
			return NULL;
		}
	}

	lstrcpyA(psKeyBuffer, pszRoot);
	if (lpSubKey && *lpSubKey)
	{
		nLen = lstrlenA(psKeyBuffer);
		psKeyBuffer[nLen++] = '\\'; psKeyBuffer[nLen] = 0;
		WideCharToMultiByte(nAnsiCP/*CP_UTF8*/, 0, lpSubKey, -1, psKeyBuffer+nLen, nKeyBufferSize-nLen, 0,0);
	}
	
	// В готовом UTF-8 нужно заменить " на ""
	char* pszQ = strchr(psKeyBuffer, '"');
	if (pszQ)
	{
		nLen = lstrlenA(psKeyBuffer);
		while (pszQ)
		{
			memmove(pszQ+1, pszQ, nLen - (pszQ-psKeyBuffer) + 1); // + '\0'
			nLen++;
			pszQ = strchr(pszQ+2, '"');
		}
		if ((nLen >= nKeyBufferSize) || (nLen >= MAX_FORMAT_KEY_LEN))
		{
			InvalidOp();
			*psKeyBuffer = 0; // чтобы в cmd-случайно не попали значения от "не созданного" ключа
			return NULL;
		}
	}
	pszQ = strchr(psKeyBuffer, '%');
	if (pszQ)
	{
		nLen = lstrlenA(psKeyBuffer);
		while (pszQ)
		{
			memmove(pszQ+1, pszQ, nLen - (pszQ-psKeyBuffer) + 1); // + '\0'
			nLen++;
			pszQ = strchr(pszQ+2, '%');
		}
		if ((nLen >= nKeyBufferSize) || (nLen >= MAX_FORMAT_KEY_LEN))
		{
			InvalidOp();
			*psKeyBuffer = 0; // чтобы в cmd-случайно не попали значения от "не созданного" ключа
			return NULL;
		}
	}
	
	return psKeyBuffer;
}

// psz == GetFormatBuffer()
// psVerb == "add" или "delete"
// psKeyUTF8 обычно просто == psKeyBuffer
BOOL MFileTxtCmd::PrepareCmd(char*& psz, LPCSTR psVerb, LPCSTR psKeyUTF8)
{
	if (!psKeyUTF8 || !*psKeyUTF8)
	{
		InvalidOp();
		return FALSE;
	}
	_ASSERTE(psVerb!=NULL);
	
	lstrcpyA(psz, "reg "); psz += 4;
	lstrcpyA(psz, psVerb); psz += lstrlenA(psVerb);
	*(psz++) = ' '; *psz = 0;
	
	// Теперь - ключ
	*(psz++) = '"';
	lstrcpyA(psz, psKeyUTF8); psz += lstrlenA(psKeyUTF8);
	*(psz++) = '"'; *(psz++) = ' '; // закрыть кавыку
	*(psz++) = '/'; *(psz++) = 'f'; *(psz++) = ' '; // добавить "/f" чтобы вопросов не задавала
	*psz = 0; // fin
	
	return TRUE;
}

BOOL MFileTxtCmd::PrepareString(char*& psz, LPCWSTR psText, int nLen, int nCchLeft)
{
	//if (((LPBYTE)psz < pExportFormatted) || !pExportFormatted || !psz || !psText)
	//{
	//	_ASSERTE(psz);
	//	_ASSERTE(pExportFormatted);
	//	_ASSERTE(psText);
	//	_ASSERTE((LPBYTE)psz >= pExportFormatted);
	//	InvalidOp();
	//	return FALSE;
	//}
	
	if (nLen == 0)
		return TRUE; // пустая строка, ничего не делать
	if (!psText)
	{
		InvalidOp();
		return FALSE;
	}
	if (nLen < 0)
	{
		InvalidOp();
		return FALSE;
	}
	
	while ((nLen > 0) && (psText[nLen-1] == 0))
		nLen--; // В cmd не нужно выкидывать \0

	//size_t nCurLen = ((LPBYTE)psz) - pExportFormatted;
	//if (nCurLen > cbExportFormatted || ((int)nCurLen) < 0)
	//{
	//	InvalidOp();
	//	return FALSE;
	//}
	//int nLeft = cbExportFormatted - (int)nCurLen;
	if ((nLen*4) >= nCchLeft)
	{
		InvalidOp();
		return FALSE;
	}

	//nLen = lstrlenA(psText); -- нельзя, может быть НЕ \0-terminated
	int nCvtLen = WideCharToMultiByte(nAnsiCP/*CP_UTF8*/, 0, psText, nLen, psz, nCchLeft, 0,0);
	psz[nCvtLen] = 0;
	
	// В готовом UTF-8 нужно заменить " на ""
	char* pszQ = strchr(psz, '"');
	if (pszQ)
	{
		while (pszQ)
		{
			memmove(pszQ+1, pszQ, nCvtLen - (pszQ-psz) + 1); // + '\0'
			nCvtLen++;
			pszQ = strchr(pszQ+2, '"');
		}
		if (nCvtLen >= nCchLeft)
		{
			InvalidOp();
			return FALSE;
		}
	}
	pszQ = strchr(psz, '%');
	if (pszQ)
	{
		while (pszQ)
		{
			memmove(pszQ+1, pszQ, nCvtLen - (pszQ-psz) + 1); // + '\0'
			nCvtLen++;
			pszQ = strchr(pszQ+2, '%');
		}
		if (nCvtLen >= nCchLeft)
		{
			InvalidOp();
			return FALSE;
		}
	}
	
	psz += nCvtLen;
	_ASSERTE(*psz == 0);
	*psz = 0;
	return TRUE;
}

BOOL MFileTxtCmd::PrepareMSZ(char*& psz, LPCWSTR psText, int nLen, int nCchLeft, LPCWSTR pszValueName)
{
	if (nLen == 0)
		return TRUE; // пустая строка, ничего не делать
	if (!psText)
	{
		InvalidOp();
		return FALSE;
	}
	if (nLen < 0)
	{
		InvalidOp();
		return FALSE;
	}
	
	while ((nLen > 0) && (psText[nLen-1] == 0))
		nLen--; // В cmd не нужно выкидывать завешающий \0
	if (nLen == 0)
		return TRUE; // пустая строка, ничего не делать
		
	// Для тестов нужно сначала сконвертить строку в кодировку cmd файла
	if ((nLen*3) >= nCchLeft)
	{
		InvalidOp();
		return FALSE; // буфер должен был быть выделен достаточно размера для преобразования в UTF8!
	}
	int nCvtLen = WideCharToMultiByte(nAnsiCP/*CP_UTF8*/, 0, psText, nLen, psz, nCchLeft, 0,0);
	psz[nCvtLen] = 0;

	int nLineLen;
	// Теперь можно определить разделитель, который можно использовать в cmd
	char szDelim[3] = "\\0"; // по умолчанию
	BOOL lbFailed = FALSE;
	LPCSTR pszLine = psz;
	// Поехали (шаг1)
	while (*pszLine)
	{
		// Нужно пробежаться по всем строкам, чтобы проверить валидность MSZ (в середине не должно быть \0\0)
		if (!lbFailed && strstr(pszLine, szDelim))
		{
			lbFailed = TRUE; // Разделитель "\\0" использовать нельзя - он встречается в тексте
		}
		nLineLen = lstrlenA(pszLine);
		pszLine += nLineLen+1;
	}
	// Сравниваем именно с nCvtLen, т.к. UTF8 может быть длиннее nLen
	if ((pszLine - psz) < nCvtLen)
	{
		REPlugin::MessageFmt(REM_CmdErrMsz0, pszValueName);
		return FALSE;
	}
	// Если "\\0" использовать нельзя - попытаемся подобрать разделитель из нижней части ASCII
	if (lbFailed)
	{
		for (int c = 1; c <= 127; c++)
		{
			if (c == 8 || c == '\r' || c == '\n' || c == '/' || c == '\\' || c == '"')
				continue; // этими символами разделять не будем

			lbFailed = FALSE;
			szDelim[0] = c; szDelim[1] = 0; // односимвольный
			
			pszLine = psz;
			// Поехали (шаг2)
			while (*pszLine)
			{
				// Нужно пробежаться по всем строкам, чтобы проверить валидность MSZ (в середине не должно быть \0\0)
				if (strchr(pszLine, c))
				{
					lbFailed = TRUE; // Разделитель «c» использовать нельзя - он встречается в тексте
					break;
				}
				nLineLen = lstrlenA(pszLine);
				pszLine += nLineLen+1;
			}
			if (!lbFailed)
				break; // подобрали
		} // end for
		
		// Получилось?
		if (lbFailed)
		{
			REPlugin::MessageFmt(REM_CmdBadValueMSZ, pszValueName);
			return FALSE;
		}
	}
	
	// Раз все хорошо - можно выгружать
	pszLine = psz; // чтобы nCchLeft поправить
	
	// Разделитель
	int nDelim = lstrlenA(szDelim);
	_ASSERTE((nDelim==1 && szDelim[0]!=L'\\') || (nDelim==2 && szDelim[0]==L'\\'));
	if (szDelim[0] != '\\')
	{
		*(psz++) = '/'; *(psz++) = 's'; *(psz++) = ' '; *(psz++) = szDelim[0]; *(psz++) = ' ';
	}
	*(psz++) = '/'; *(psz++) = 'd'; *(psz++) = ' '; *(psz++) = '"';
	
	// Сколько в буфере осталось?
	nCchLeft -= (int)(psz - pszLine) + 2;
	if (nCchLeft <= 0)
	{
		InvalidOp();
		return FALSE;
	}
	
	// Поехали
	*psz = 0;
	LPCWSTR pwszSrc = psText;
	while (*pwszSrc)
	{
		nLineLen = lstrlenW(pwszSrc);
		pszLine = psz; // чтобы nCchLeft поправить
		if (!PrepareString(psz, pwszSrc, nLineLen, nCchLeft))
			return -1; // ошибка уже показана
		pwszSrc += nLineLen+1;
		nCchLeft -= (int)(psz - pszLine) + nDelim;
		if (nCchLeft <= 0)
		{
			InvalidOp();
			return FALSE;
		}
		if (*pwszSrc)
		{
			*(psz++) = szDelim[0];
			if (szDelim[1])
				*(psz++) = szDelim[1];
		}
	}
	*(psz++) = '"'; *psz = 0; // закрыть кавыку
	
	return TRUE; // OK
}

void MFileTxtCmd::FileClose()
{
	if (hFile && (hFile != INVALID_HANDLE_VALUE))
	{
		if (mb_KeyWriteWaiting)
			WriteCurrentKey(FALSE, mb_LastKeyHasValues);
		if (ms_RECmdErrFin && mn_RECmdErrFin) // // "if NOT %FarRegEditRc%=="""" echo At least on value/key failed: %FarRegEditRc%\r\n"
			FileWriteBuffered(ms_RECmdErrFin, mn_RECmdErrFin);
	}
	MFileTxt::FileClose();
}

BOOL MFileTxtCmd::WriteRegCheck(BOOL bDeletion, LPCSTR psValueName)
{
	if (!FileWriteBuffered(ms_RECmdErrCheck, mn_RECmdErrCheck))
		return FALSE;
	// Если & попадается МЕЖДУ кавычками - то cmd ест нормально.
	LPCSTR psz = bDeletion ? "\"DELETE " : "\"ADD ";
	if (!FileWriteBuffered(psz, lstrlenA(psz)))
		return FALSE;
	if (psKeyBuffer)
	{
		if (!FileWriteBuffered(psKeyBuffer, lstrlenA(psKeyBuffer)))
			return FALSE;
	}
	if (psValueName)
	{
		if (!FileWriteBuffered(" :: ", 4) ||
			!FileWriteBuffered(psValueName, lstrlenA(psValueName)))
			return FALSE;
	}
	if (!FileWriteBuffered("\"\r\n", 3))
		return FALSE;
	return TRUE;
}
