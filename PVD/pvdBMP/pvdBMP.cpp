
#undef WINVER
#define WINVER 0x500

#include <windows.h>
#ifndef WIN64
#include <crtdbg.h>
#else
#define _ASSERTE(a)
#endif
#include "../PictureViewPlugin.h"

typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef DWORD u32;

// Некритичные ошибки ( < 0x7FFFFFFF )
// При подборе декодера в PicView нужно возвращать именно их ( < 0x7FFFFFFF )
// для тех файлов (форматов), которые неизвестны данному декодеру.
// PicView не будет отображать эти ошибки пользователю.
#define PBE_NOT_BMP_HEADER           0x1001
#define PBE_TOO_SMALL_FILE           0x1002

// Далее идут критичные ошибки. Информация об ошибке будет показана
// пользователю, если PicView не сможет открыть файл никаким декодером
#define PBE_OPEN_FILE_FAILED         0x80000001
#define PBE_FILE_MAPPING_FAILED      0x80000002
#define PBE_INVALID_CONTEXT          0x80000003
#define PBE_TOO_LARGE_FILE           0x80000004
#define PBE_OLD_PICVIEW              0x80000005

BOOL __stdcall pvdTranslateError2(DWORD nErrNumber, wchar_t *pszErrInfo, int nBufLen)
{
	if (!pszErrInfo || nBufLen<=0)
		return FALSE;

	switch (nErrNumber)
	{
	case PBE_OPEN_FILE_FAILED:
		lstrcpynW(pszErrInfo, L"Open file failed", nBufLen); break;
	case PBE_FILE_MAPPING_FAILED:
		lstrcpynW(pszErrInfo, L"File mapping failed (not enough memory)", nBufLen); break;
	case PBE_INVALID_CONTEXT:
		lstrcpynW(pszErrInfo, L"Invalid context", nBufLen); break;
	case PBE_NOT_BMP_HEADER:
		lstrcpynW(pszErrInfo, L"Not BMP header", nBufLen); break;
	case PBE_TOO_LARGE_FILE:
		lstrcpynW(pszErrInfo, L"File is too large", nBufLen); break;
	case PBE_TOO_SMALL_FILE:
		lstrcpynW(pszErrInfo, L"File is too smal", nBufLen); break;
	case PBE_OLD_PICVIEW:	
		lstrcpyn(pszErrInfo, L"Old PicView version, exiting", nBufLen); break;
	default:
		return FALSE;
	}
	return TRUE;
}

bool CheckBmHeader(const BYTE *pBuf, UINT32 lBuf)
{
	BITMAPFILEHEADER* pbfh = (BITMAPFILEHEADER*)pBuf;
	if ((lBuf < 0x38) || (pbfh->bfType != 'MB'))
		return false;
	if ((pbfh->bfOffBits < 0x36) || (pbfh->bfOffBits > 0x436))
		return false;
	#ifdef _DEBUG
	const BYTE* pBits = pBuf + pbfh->bfOffBits;
	#endif
	const int iSize0 = sizeof(BITMAPCOREHEADER), iSize3 = sizeof(BITMAPINFOHEADER),
		iSize4 = sizeof(BITMAPV4HEADER), iSize5 = sizeof(BITMAPV5HEADER);
	BITMAPINFO* pbmi = (BITMAPINFO*)(pBuf+14);
	BITMAPCOREINFO* pbci = (BITMAPCOREINFO*)(pBuf+14);
	if ((pbmi->bmiHeader.biSize != iSize0) && (pbmi->bmiHeader.biSize != iSize3)
		&& (pbmi->bmiHeader.biSize != iSize4) && (pbmi->bmiHeader.biSize != iSize5)
		&& (pbmi->bmiHeader.biSize != 0x38) // GIMP???
		)
		return false;
	if ((pbmi->bmiHeader.biBitCount > 0xFF)
		|| ((pbmi->bmiHeader.biCompression != BI_RGB) && ((pbmi->bmiHeader.biCompression != BI_BITFIELDS)))
		)
		return false;
	#ifdef _DEBUG
	bool bOk = (lBuf >= 0x38)
		&& (*(u16*)pBuf == 'MB')
		&& (*(u32*)(pBuf + 0x0A) >= 0x36) && (*(u32*)(pBuf + 0x0A) <= 0x436)
		&& (*(u32*)(pBuf + 0x0E) == 0x28)
		&& (!pBuf[0x1D])
		&& (!*(u32*)(pBuf + 0x1E))
		;
	#endif
	return true;
}


struct FileMap
{
	//HANDLE hFile, hMapping;
	DWORD lSize;
	u8 *pMapping;
	DWORD nErrNumber;

	const u8 *Open(const wchar_t *pName)
	{
		nErrNumber = 0;
		//hMapping = NULL;
		pMapping = NULL;
		HANDLE hFile = CreateFileW(pName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			hFile = CreateFileW(pName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if ((hFile) != INVALID_HANDLE_VALUE)
		{
			DWORD nHigh = 0;
			lSize = GetFileSize(hFile, &nHigh);
			if (nHigh || (lSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)) {
				nErrNumber = PBE_TOO_LARGE_FILE;
			} else
			if (lSize < 0x38) {
				nErrNumber = PBE_TOO_SMALL_FILE;
			} else {
				BYTE buf[0x38];
				if (ReadFile(hFile, buf, sizeof(buf), &nHigh, 0)) {
					if (!CheckBmHeader(buf, nHigh))
					{
						nErrNumber = PBE_NOT_BMP_HEADER;
					}
					else
					{
						//if (hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL))
						//pMapping = (u8*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
						pMapping = (u8*)HeapAlloc(GetProcessHeap(), 0, lSize);
						memmove(pMapping, buf, nHigh);
						DWORD nRead, nToRead;
						nToRead = lSize-nHigh;
						if (!ReadFile(hFile, pMapping+nHigh, nToRead, &nRead, 0) || nToRead != nRead)
						{
							HeapFree(GetProcessHeap(), 0, pMapping);
							pMapping = NULL;
						}
					}
				}
				if (!pMapping) nErrNumber = PBE_FILE_MAPPING_FAILED;
			}
			CloseHandle(hFile);
		} else nErrNumber = PBE_OPEN_FILE_FAILED;
		//if (!pMapping) CloseHandle(hFile);
		return pMapping;
	}
	void Close(void)
	{
		if (pMapping)
		{
			HeapFree(GetProcessHeap(), 0, pMapping);
			pMapping = NULL;
		}
		//if (hFile && hFile != INVALID_HANDLE_VALUE)
		//{
		//	if (pMapping) {
		//		UnmapViewOfFile((void*)pMapping); // BUGGY GCC(?), argument marked as void*
		//		pMapping = NULL;
		//	}
		//	if (hMapping) {
		//		CloseHandle(hMapping); hMapping = NULL;
		//	}
		//	CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
		//}
	}
};

void __stdcall pvdPluginInfo2(pvdInfoPlugin2 *pPluginInfo)
{
	_ASSERTE(pPluginInfo->cbSize >= sizeof(pvdInfoPlugin2));
	pPluginInfo->Priority = 1;
	pPluginInfo->pName = L"BMP";
	pPluginInfo->pVersion = L"2.0";
	pPluginInfo->pComments = L"PictureView demo plugin";
	pPluginInfo->Flags = PVD_IP_DECODE;
}


UINT32 __stdcall pvdInit2(pvdInitPlugin2* pInit)
{
	_ASSERTE(pInit->cbSize >= sizeof(pvdInitPlugin2));
	if (pInit->cbSize < sizeof(pvdInitPlugin2)) {
		pInit->nErrNumber = PBE_OLD_PICVIEW;
		return 0;
	}

	//FileMap *pFile = (FileMap*) HeapAlloc(GetProcessHeap(), 0, sizeof(FileMap));
	//if (!pFile)
	//	return 0;
	//pInit->pContext = pFile;
	pInit->pContext = (void*)1;
	pInit->nErrNumber = 0;
	return PVD_UNICODE_INTERFACE_VERSION;
}

void __stdcall pvdGetFormats2(void *pContext, pvdFormats2* pFormats)
{
	_ASSERTE(pFormats->cbSize >= sizeof(pvdFormats2));

	// pContext в данном случае не используется, т.к. все тривиально
	// Он может пригодиться другим декодерам, если им требуется сканирование
	// своих библиотек для определения поддерживаемых форматов

	pFormats->pActive = L"bmp,dib";
	pFormats->pInactive = pFormats->pForbidden = L"";
}

BOOL __stdcall pvdFileOpen2(void *pContext, const wchar_t *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage2 *pImageInfo)
{
	//FileMap *pFile = (FileMap*)pContext;
	//if (!pFile) {
	//	pImageInfo->nErrNumber = PBE_INVALID_CONTEXT;
	//	return FALSE;
	//}
	BOOL lbRc = FALSE;
	FileMap *pFile = (FileMap*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FileMap));

	_ASSERTE(pImageInfo->cbSize >= sizeof(pvdInfoImage2));
	//BUGBUG: вообще-то PicView 1.41 не всегда передает данные (или их часть?) через pBuf. Он может быть и пустым.
	// PicView 2 всегда передает через pBuf хотя бы часть файла


	if (!lFileSize || lFileSize == (__int64)lBuf)
	{	// Имя файла виртуальное, все требуемые данные переданы через pBuf
		if (!CheckBmHeader(pBuf, lBuf)) {
			pImageInfo->nErrNumber = PBE_NOT_BMP_HEADER;
			goto wrap;
		}
		//pFile->hFile = INVALID_HANDLE_VALUE;
		pFile->pMapping = (u8*)HeapAlloc(GetProcessHeap(), 0, lBuf);
		memmove(pFile->pMapping, pBuf, lBuf);
		pFile->lSize = lBuf;
	}
	else {
		if (pBuf && lBuf) {
			if (!CheckBmHeader(pBuf, lBuf)) {
				pImageInfo->nErrNumber = PBE_NOT_BMP_HEADER;
				goto wrap;
			}
		}
		if (!pFile->Open(pFileName) || pFile->lSize < 0x38)
		{
			pImageInfo->nErrNumber = pFile->nErrNumber;
			//pFile->Close();
			goto wrap;
		}
	}

	if (*(u16*)(pFile->pMapping+0x1C) == 32)
	{
		// May be alpha channel?
		pImageInfo->nPages = 2;
	} else {
		pImageInfo->nPages = 1;
	}
	pImageInfo->Flags = 0;
	pImageInfo->pFormatName = L"BMP";
	pImageInfo->pCompression = NULL;
	pImageInfo->pComments = L"This is sample (*.bmp) comment";
	pImageInfo->pImageContext = pFile;
	lbRc = TRUE;
wrap:
	if (!lbRc)
	{
		pFile->Close();
		HeapFree(GetProcessHeap(), 0, pFile);
	}
	return lbRc;
}

BOOL __stdcall pvdPageInfo2(void *pContext, void *pImageContext, pvdInfoPage2 *pPageInfo)
{
	_ASSERTE(pPageInfo->cbSize >= sizeof(pvdInfoPage2));

	const u8 *const p = ((FileMap*)pImageContext)->pMapping;
	pPageInfo->lWidth = *(u32*)(p + 0x12);
	pPageInfo->lHeight = abs(*(i32*)(p + 0x16));
	pPageInfo->nBPP = *(u16*)(p + 0x1C);

	if (!pPageInfo->iPage || (pPageInfo->iPage == 1 && pPageInfo->nBPP == 32))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL __stdcall pvdPageDecode2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo, pvdDecodeCallback2 DecodeCallback, void *pDecodeCallbackContext)
{
	_ASSERTE(pDecodeInfo->cbSize >= sizeof(pvdInfoDecode2));

	u8 *const p = (u8*)((FileMap*)pImageContext)->pMapping;
	pDecodeInfo->lWidth = *(u32*)(p + 0x12);
	pDecodeInfo->lHeight = abs(*(i32*)(p + 0x16));
	pDecodeInfo->nBPP = *(u16*)(p + 0x1C);

	if (!pDecodeInfo->iPage // Поддерживается только страница #0
		|| (pDecodeInfo->iPage == 1 && pDecodeInfo->nBPP == 32)) // страница #1 добавлена для прозрачности
	{
		pDecodeInfo->pImage = p + *(u32*)(p + 0x0A);
		pDecodeInfo->pPalette = (pDecodeInfo->nBPP > 8) ? NULL : (UINT32*)(p + 0x36);
		pDecodeInfo->Flags = PVD_IDF_READONLY | (pDecodeInfo->iPage ? PVD_IDF_ALPHA : 0);
		pDecodeInfo->nColorsUsed = *(u32*)(p + 0x2E);
		pDecodeInfo->lImagePitch = *(i32*)(p + 0x16) < 0 
				? (*(u32*)(p + 0x12) * *(u16*)(p + 0x1C) + 0x1F & ~0x1F) / 8 
				: -(i32)((*(u32*)(p + 0x12) * *(u16*)(p + 0x1C) + 0x1F & ~0x1F) / 8);
		pDecodeInfo->ColorModel = pDecodeInfo->iPage ? PVD_CM_BGRA : PVD_CM_BGR;
		return TRUE;
	}
	return FALSE;
}

void __stdcall pvdPageFree2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo)
{
	_ASSERTE(pDecodeInfo->cbSize >= sizeof(pvdInfoDecode2));
}

void __stdcall pvdFileClose2(void *pContext, void *pImageContext)
{
	((FileMap*)pImageContext)->Close();
	HeapFree(GetProcessHeap(), 0, pImageContext);
}

void __stdcall pvdExit2(void *pContext)
{
	//if (pContext)
	//	HeapFree(GetProcessHeap(), 0, pContext);
}
