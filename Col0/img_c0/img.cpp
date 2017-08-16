
/*
Copyright (c) 2010-2016 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define imgc0_EXPORTS

#include <windows.h>
#include <GdiPlus.h>

#ifdef _DEBUG
	#if !defined(__GNUC__)
		#include <crtdbg.h>
	#endif
#endif

#ifndef _ASSERTE
	#define _ASSERT(x)
	#define _ASSERTE(x)
#endif

#define _wsprintf wsprintfW
#define SKIPLEN(x)

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#include "../../common/MStream.h"
// для унификации col0host & PanelView
#include "../../common/plugin.h"

#ifdef _DEBUG
	#define Msg(f,s) //MessageBoxA(NULL,s,f,MB_OK|MB_SETFOREGROUND)
#else
	#define Msg(f,s)
#endif

typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef DWORD u32;

#define _T(x)      L ## x
#define _tcscpy lstrcpyW

HMODULE ghModule;


#define PGE_INVALID_FRAME        0x1001
#define PGE_ERROR_BASE           0x80000000
#define PGE_DLL_NOT_FOUND        0x80001001
#define PGE_FILE_NOT_FOUND       0x80001003
#define PGE_NOT_ENOUGH_MEMORY    0x80001004
#define PGE_INVALID_CONTEXT      0x80001005
#define PGE_FUNCTION_NOT_FOUND   0x80001006
#define PGE_WIN32_ERROR          0x80001007
#define PGE_UNKNOWN_COLORSPACE   0x80001008
#define PGE_UNSUPPORTED_PITCH    0x80001009
#define PGE_INVALID_PAGEDATA     0x8000100A
#define PGE_OLD_PLUGIN           0x8000100B
#define PGE_BITBLT_FAILED        0x8000100C
#define PGE_INVALID_VERSION      0x8000100D
#define PGE_INVALID_IMGSIZE      0x8000100E
#define PGE_UNSUPPORTEDFORMAT    0x8000100F


DWORD gnLastWin32Error = 0;




#define DllGetFunction(hModule, FunctionName) FunctionName = (FunctionName##_t)GetProcAddress(hModule, #FunctionName)
//#define MALLOC(n) HeapAlloc(GetProcessHeap(), 0, n)
#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "

enum tag_GdiStrMagics
{
	eGdiStr_Decoder = 0x1002,
	eGdiStr_Image = 0x1003,
	eGdiStr_Bits = 0x1004,
};

#define cfTIFF 0xb96b3cb1
#define cfJPEG 0xb96b3cae
#define cfPNG  0xb96b3caf
#define cfEXIF 0xb96b3cb2



struct GDIPlusDecoder
{
	DWORD   nMagic;
	HMODULE hGDIPlus;
	ULONG_PTR gdiplusToken; bool bTokenInitialized;
	HRESULT nErrNumber, nLastError;
	//bool bUseICM, bForceSelfDisplay, bCMYK2RGB;
	BOOL bCoInitialized;
	//const wchar_t* pszPluginKey;
	//BOOL bAsDisplay;
	BOOL bCancelled;


	/* ++ */ typedef Gdiplus::Status(WINAPI *GdiplusStartup_t)(OUT ULONG_PTR *token, const Gdiplus::GdiplusStartupInput *input, OUT Gdiplus::GdiplusStartupOutput *output);
	/* ++ */ typedef VOID (WINAPI *GdiplusShutdown_t)(ULONG_PTR token);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipCreateBitmapFromFile_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageThumbnail_t)(Gdiplus::GpImage *image, UINT thumbWidth, UINT thumbHeight, Gdiplus::GpImage **thumbImage, Gdiplus::GetThumbnailImageAbort callback, VOID * callbackData);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStream_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromFileICM_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStreamICM_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageWidth_t)(Gdiplus::GpImage *image, UINT *width);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageHeight_t)(Gdiplus::GpImage *image, UINT *height);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImagePixelFormat_t)(Gdiplus::GpImage *image, Gdiplus::PixelFormat *format);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapLockBits_t)(Gdiplus::GpBitmap* bitmap, GDIPCONST Gdiplus::GpRect* rect, UINT flags, Gdiplus::PixelFormat format, Gdiplus::BitmapData* lockedBitmapData);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapUnlockBits_t)(Gdiplus::GpBitmap* bitmap, Gdiplus::BitmapData* lockedBitmapData);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipDisposeImage_t)(Gdiplus::GpImage *image);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipImageGetFrameCount_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT* count);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipImageSelectActiveFrame_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT frameIndex);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetPropertyItemSize_t)(Gdiplus::GpImage *image, PROPID propId, UINT* size);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetPropertyItem_t)(Gdiplus::GpImage *image, PROPID propId, UINT propSize, Gdiplus::PropertyItem* buffer);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipImageRotateFlip_t)(Gdiplus::GpImage *image, Gdiplus::RotateFlipType rfType);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageRawFormat_t)(Gdiplus::GpImage *image, OUT GUID* format);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageFlags_t)(Gdiplus::GpImage *image, UINT *flags);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePalette_t)(Gdiplus::GpImage *image, Gdiplus::ColorPalette *palette, INT size);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePaletteSize_t)(Gdiplus::GpImage *image, INT *size);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipCreateFromHDC_t)(HDC hdc, Gdiplus::GpGraphics **graphics);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipDeleteGraphics_t)(Gdiplus::GpGraphics *graphics);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipDrawImageRectRectI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpImage *image, INT dstx, INT dsty, INT dstwidth, INT dstheight, INT srcx, INT srcy, INT srcwidth, INT srcheight, Gdiplus::GpUnit srcUnit, const Gdiplus::GpImageAttributes* imageAttributes, Gdiplus::DrawImageAbort callback, VOID * callbackData);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromScan0_t)(INT width, INT height, INT stride, Gdiplus::PixelFormat format, BYTE* scan0, Gdiplus::GpBitmap** bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipFillRectangleI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpBrush *brush, INT x, INT y, INT width, INT height);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateSolidFill_t)(Gdiplus::ARGB color, Gdiplus::GpSolidFill **brush);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDeleteBrush_t)(Gdiplus::GpBrush *brush);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneImage_t)(Gdiplus::GpImage *image, Gdiplus::GpImage **cloneImage);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneBitmapAreaI_t)(INT x, INT y, INT width, INT height, Gdiplus::PixelFormat format, Gdiplus::GpBitmap *srcBitmap, Gdiplus::GpBitmap **dstBitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipSetImagePalette_t)(Gdiplus::GpImage *image, GDIPCONST Gdiplus::ColorPalette *palette);



	GdiplusStartup_t GdiplusStartup;
	GdiplusShutdown_t GdiplusShutdown;
	GdipCreateBitmapFromFile_t GdipCreateBitmapFromFile;
	GdipGetImageThumbnail_t GdipGetImageThumbnail;
	//GdipCreateBitmapFromStream_t GdipCreateBitmapFromStream;
	//GdipCreateBitmapFromFileICM_t GdipCreateBitmapFromFileICM;
	//GdipCreateBitmapFromStreamICM_t GdipCreateBitmapFromStreamICM;
	GdipGetImageWidth_t GdipGetImageWidth;
	GdipGetImageHeight_t GdipGetImageHeight;
	GdipGetImagePixelFormat_t GdipGetImagePixelFormat;
	//GdipBitmapLockBits_t GdipBitmapLockBits;
	//GdipBitmapUnlockBits_t GdipBitmapUnlockBits;
	GdipDisposeImage_t GdipDisposeImage;
	GdipImageGetFrameCount_t GdipImageGetFrameCount;
	GdipImageSelectActiveFrame_t GdipImageSelectActiveFrame;
	GdipGetPropertyItemSize_t GdipGetPropertyItemSize;
	GdipGetPropertyItem_t GdipGetPropertyItem;
	GdipImageRotateFlip_t GdipImageRotateFlip;
	GdipGetImageRawFormat_t GdipGetImageRawFormat;
	//GdipGetImageFlags_t GdipGetImageFlags;
	//GdipGetImagePalette_t GdipGetImagePalette;
	//GdipGetImagePaletteSize_t GdipGetImagePaletteSize;
	GdipCreateFromHDC_t GdipCreateFromHDC;
	GdipDeleteGraphics_t GdipDeleteGraphics;
	GdipDrawImageRectRectI_t GdipDrawImageRectRectI;
	//GdipCreateBitmapFromScan0_t GdipCreateBitmapFromScan0;
	//GdipFillRectangleI_t GdipFillRectangleI;
	//GdipCreateSolidFill_t GdipCreateSolidFill;
	//GdipDeleteBrush_t GdipDeleteBrush;
	//GdipCloneImage_t GdipCloneImage;
	//GdipCloneBitmapAreaI_t GdipCloneBitmapAreaI;
	//GdipSetImagePalette_t GdipSetImagePalette;

	GDIPlusDecoder()
	{
		nMagic = eGdiStr_Decoder;
		hGDIPlus = NULL; gdiplusToken = NULL; bTokenInitialized = false;
		nErrNumber = 0; nLastError = 0; //bUseICM = false; bCoInitialized = FALSE; bCMYK2RGB = false;
		//pszPluginKey = NULL;
		bCancelled = FALSE;
		//bForceSelfDisplay = false;
	}


	bool Init()
	{
		bool result = false;
		nErrNumber = 0;
		//pszPluginKey = pInit->pRegKey;
		//ReloadConfig();
		HRESULT hrCoInitialized = CoInitialize(NULL);
		bCoInitialized = SUCCEEDED(hrCoInitialized);
		wchar_t FullPath[MAX_PATH*2+15]; FullPath[0] = 0;
		//if (ghModule)
		//{
		//	PVDSettings::FindFile(L"GdiPlus.dll", FullPath, sizeofarray(FullPath));
		//	hGDIPlus = LoadLibraryW(FullPath);
		//}
		//if (!hGDIPlus)
		hGDIPlus = LoadLibraryW(L"GdiPlus.dll");

		if (!hGDIPlus)
		{
			nErrNumber = PGE_DLL_NOT_FOUND;
		}
		else
		{
			DllGetFunction(hGDIPlus, GdiplusStartup);
			DllGetFunction(hGDIPlus, GdiplusShutdown);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromFile);
			DllGetFunction(hGDIPlus, GdipGetImageThumbnail);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromFileICM);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStream);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStreamICM);
			DllGetFunction(hGDIPlus, GdipGetImageWidth);
			DllGetFunction(hGDIPlus, GdipGetImageHeight);
			DllGetFunction(hGDIPlus, GdipGetImagePixelFormat);
			//DllGetFunction(hGDIPlus, GdipBitmapLockBits);
			//DllGetFunction(hGDIPlus, GdipBitmapUnlockBits);
			DllGetFunction(hGDIPlus, GdipDisposeImage);
			DllGetFunction(hGDIPlus, GdipImageGetFrameCount);
			DllGetFunction(hGDIPlus, GdipImageSelectActiveFrame);
			DllGetFunction(hGDIPlus, GdipGetPropertyItemSize);
			DllGetFunction(hGDIPlus, GdipGetPropertyItem);
			DllGetFunction(hGDIPlus, GdipImageRotateFlip);
			DllGetFunction(hGDIPlus, GdipGetImageRawFormat);
			//DllGetFunction(hGDIPlus, GdipGetImageFlags);
			//DllGetFunction(hGDIPlus, GdipGetImagePalette);
			//DllGetFunction(hGDIPlus, GdipGetImagePaletteSize);
			DllGetFunction(hGDIPlus, GdipCreateFromHDC);
			DllGetFunction(hGDIPlus, GdipDeleteGraphics);
			DllGetFunction(hGDIPlus, GdipDrawImageRectRectI);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromScan0);
			//DllGetFunction(hGDIPlus, GdipFillRectangleI);
			//DllGetFunction(hGDIPlus, GdipCreateSolidFill);
			//DllGetFunction(hGDIPlus, GdipDeleteBrush);
			//DllGetFunction(hGDIPlus, GdipCloneBitmapAreaI);
			//DllGetFunction(hGDIPlus, GdipSetImagePalette);

			if (GdiplusStartup && GdiplusShutdown && GdipCreateBitmapFromFile && GdipGetImageThumbnail
			        && GdipGetImageWidth && GdipGetImageHeight && GdipGetImagePixelFormat && GdipGetImageRawFormat
			        //&& GdipBitmapLockBits && GdipBitmapUnlockBits
			        && GdipDisposeImage && GdipImageGetFrameCount && GdipImageSelectActiveFrame
			        && GdipGetPropertyItemSize && GdipGetPropertyItem && GdipImageRotateFlip
			        //&& GdipGetImagePalette && GdipGetImagePaletteSize && GdipCloneBitmapAreaI && GdipGetImageFlags
			        && GdipCreateFromHDC && GdipDeleteGraphics && GdipDrawImageRectRectI
			        //&& GdipCreateBitmapFromScan0 && GdipFillRectangleI && GdipCreateSolidFill && GdipDeleteBrush
			        //&& GdipSetImagePalette
			  )
			{
				Gdiplus::GdiplusStartupInput gdiplusStartupInput;
				Gdiplus::Status lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
				result = (lRc == Gdiplus::Ok);

				if (!result)
				{
					nLastError = GetLastError();
					GdiplusShutdown(gdiplusToken); bTokenInitialized = false;
					nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
				}
				else
				{
					bTokenInitialized = true;
				}
			}
			else
			{
				nErrNumber = PGE_FUNCTION_NOT_FOUND;
			}

			if (!result)
				FreeLibrary(hGDIPlus);
		}

		return result;
	};

	// При использовании в фаре GdiPlus иногда может зависать на FreeLibrary.
	// Причины пока не выяснены
	static DWORD WINAPI FreeThreadProc(LPVOID lpParameter)
	{
		struct GDIPlusDecoder* p = (struct GDIPlusDecoder*)lpParameter;

		if (p && p->hGDIPlus)
		{
			FreeLibrary(p->hGDIPlus);
		}

		return 0;
	}

	void Close()
	{
		if (bTokenInitialized)
		{
			GdiplusShutdown(gdiplusToken);
			bTokenInitialized = false;
		}

		if (hGDIPlus)
		{
			//FreeLibrary(hGDIPlus);
			DWORD nFreeTID = 0, nWait = 0, nErr = 0;
			HANDLE hFree = CreateThread(NULL, 0, FreeThreadProc, this, 0, &nFreeTID);

			if (!hFree)
			{
				nErr = GetLastError();
				FreeLibrary(hGDIPlus);
			}
			else
			{
				nWait = WaitForSingleObject(hFree, 5000);

				if (nWait != WAIT_OBJECT_0)
					TerminateThread(hFree, 100);

				CloseHandle(hFree);
			}

			hGDIPlus = NULL;
		}

		if (bCoInitialized)
		{
			bCoInitialized = FALSE;
			CoUninitialize();
		}

		FREE(this);
	};
};

#define DEFINE_GUID_(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID_(FrameDimensionTime, 0x6aedbd6d,0x3fb5,0x418a,0x83,0xa6,0x7f,0x45,0x22,0x9d,0xc8,0x72);
DEFINE_GUID_(FrameDimensionPage, 0x7462dc86,0x6180,0x4c7e,0x8e,0x3f,0xee,0x73,0x33,0xa7,0xa4,0x83);

struct GDIPlusImage
{
	DWORD nMagic;

	wchar_t szTempFile[MAX_PATH+1];
	GDIPlusDecoder *gdi;
	Gdiplus::GpImage *img;
	MStream* strm;
	HRESULT nErrNumber, nLastError;
	//
	UINT lWidth, lHeight, pf, nBPP, nPages, /*lFrameTime,*/ nActivePage, nTransparent; //, nImgFlags;
	bool Animation;
	wchar_t FormatName[5];
	DWORD nFormatID;

	GDIPlusImage(GDIPlusDecoder *gdi_)
		: gdi(gdi_)
		, img(NULL)
		, strm(NULL)
	{
		nMagic = eGdiStr_Image;
		szTempFile[0] = 0;
	};

	Gdiplus::GpImage* OpenBitmapFromFile(const wchar_t *pFileName)
	{
		Gdiplus::Status lRc = Gdiplus::Ok;
		Gdiplus::GpBitmap *bmp = NULL;
		_ASSERTE(img==NULL);
		lRc = gdi->GdipCreateBitmapFromFile(pFileName, &bmp);

		if (!bmp)
		{
			nLastError = GetLastError();
			nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
		}

		return bmp;
	}

	bool Open(bool bVirtual, const wchar_t *pFileName, const u8 *pBuffer, i64 lFileSize)
	{
		_ASSERTE(img == NULL);
		_ASSERTE(gdi != NULL);
		bool result = false;
		Gdiplus::Status lRc;
		nActivePage = -1; nTransparent = -1; //nImgFlags = 0;

		if (bVirtual && pBuffer && lFileSize)
		{
			DWORD n;
			wchar_t szTempDir[MAX_PATH];
			n = GetTempPath(MAX_PATH-16, szTempDir);

			if (n && n < (MAX_PATH-16))
			{
				n = GetTempFileName(szTempDir, L"CET", 0, szTempFile);

				if (n)
				{
					HANDLE hFile = CreateFile(szTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);

					if (hFile == INVALID_HANDLE_VALUE)
					{
						szTempFile[0] = 0; // не создали, значит и удалять будет нечего
					}
					else
					{
						DWORD nSize = (DWORD)lFileSize; DWORD nWritten = 0;

						if (WriteFile(hFile, pBuffer, nSize, &nWritten, NULL) && nWritten == nSize)
						{
							bVirtual = false; pFileName = szTempFile;
						}

						CloseHandle(hFile);

						if (bVirtual)
						{
							DeleteFile(szTempFile); szTempFile[0] = 0;
						}
					}
				}
			}
		}

		if (bVirtual)
		{
			nErrNumber = PGE_FILE_NOT_FOUND;
			return false;
		}

		//if (!bVirtual)
		img = OpenBitmapFromFile(pFileName);
		//else // лучше бы его вообще не использовать, GDI+ как-то не очень с потоками работает...
		//	img = OpenBitmapFromStream(pBuffer, lFileSize);

		if (!img)
		{
			//nErrNumber = gdi->nErrNumber; -- ошибка УЖЕ в nErrNumber
		}
		else
		{
			lRc = gdi->GdipGetImageWidth(img, &lWidth);
			lRc = gdi->GdipGetImageHeight(img, &lHeight);
			lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);
			nBPP = pf >> 8 & 0xFF;
			//lRc = gdi->GdipGetImageFlags(img, &nImgFlags);
			Animation = false; nPages = 1;

			if (!(lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &nPages)))
				Animation = nPages > 1;
			else if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionPage, &nPages)))
				nPages = 1;

			FormatName[0] = 0;

			if (gdi->GdipGetImageRawFormat)
			{
				GUID gformat;

				if (!(lRc = gdi->GdipGetImageRawFormat(img, &gformat)))
				{
					// DEFINE_GUID(ImageFormatUndefined, 0xb96b3ca9,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
					// DEFINE_GUID(ImageFormatMemoryBMP, 0xb96b3caa,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
					const wchar_t Format[][5] = {L"BMP", L"EMF", L"WMF", L"JPEG", L"PNG", L"GIF", L"TIFF", L"EXIF", L"", L"", L"ICO"};

					if (gformat.Data1 >= 0xB96B3CAB && gformat.Data1 <= 0xB96B3CB5)
					{
						//FormatName = Format[gformat.Data1 - 0xB96B3CAB];
						nFormatID = gformat.Data1;
						lstrcpy(FormatName, Format[gformat.Data1 - 0xB96B3CAB]);
					}
				}
			}

			result = SelectPage(0);
		}

		return result;
	};
	bool SelectPage(UINT iPage)
	{
		bool result = false;
		Gdiplus::Status lRc;

		if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &pf)))
			lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionPage, iPage);
		else
			lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionTime, iPage);

		lRc = gdi->GdipGetImageWidth(img, &lWidth);
		lRc = gdi->GdipGetImageHeight(img, &lHeight);
		lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);
		nBPP = pf >> 8 & 0xFF;
		nActivePage = iPage;
		result = true;
		return result;
	};
	void Close()
	{
		if (!gdi) return;

		Gdiplus::Status lRc;

		if (img)
		{
			lRc = gdi->GdipDisposeImage(img);
			img = NULL;
		}

		if (strm)
		{
			delete strm;
			strm = NULL;
		}

		if (szTempFile[0])
		{
			DeleteFile(szTempFile);
		}

		FREE(this);
	};
	bool GetExifTagValueAsInt(PROPID pid, int& nValue)
	{
		bool bExists = false;
		Gdiplus::Status lRc;
		nValue = 0;
		UINT lPropSize;

		if (!(lRc = gdi->GdipGetPropertyItemSize(img, pid, &lPropSize)))
		{
			Gdiplus::PropertyItem* p = (Gdiplus::PropertyItem*)CALLOC(lPropSize);

			if (!(lRc = gdi->GdipGetPropertyItem(img, pid, lPropSize, p)))
			{
				switch(p->type)
				{
					case PropertyTagTypeByte:
						nValue = *(BYTE*)p->value; bExists = true;
						break;
					case PropertyTagTypeShort:
						nValue = *(short*)p->value; bExists = true;
						break;
					case PropertyTagTypeLong: case PropertyTagTypeSLONG:
						nValue = *(int*)p->value; bExists = true;
						break;
				}
			}

			FREE(p);
		}

		return bExists;
	};
};



#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s)
#define WARNING(s)
#else
#define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))




// {8786D503-CAD4-4391-B6D0-43696AD8F246}
static const GUID guid_ImgC0 = 
{ 0x8786d503, 0xcad4, 0x4391, { 0xb6, 0xd0, 0x43, 0x69, 0x6a, 0xd8, 0xf2, 0x46 } };


/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     )
{
	return TRUE;
}


GDIPlusDecoder decoder;
bool decoder_initialized = false;

#if defined(__GNUC__)

extern
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     );

#ifdef __cplusplus
extern "C" {
#endif
	BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
	int WINAPI GetMinFarVersionW(void);
	void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info);
	void WINAPI GetPluginInfoW(struct PluginInfo *Info);
	intptr_t WINAPI GetContentFieldsW(const struct GetContentFieldsInfo *Info)
	intptr_t WINAPI GetContentDataW(struct GetContentDataInfo *Info);
	void WINAPI FreeContentDataW(const struct GetContentDataInfo *Info);
	void WINAPI ExitFARW(const struct ExitInfo *Info);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif

#if defined(CRTSTARTUP)
extern "C" {
	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif

#if 0
int WINAPI GetMinFarVersionW(void)
{
	return MAKEFARVERSION(3,0,0,3000,VS_RELEASE);
}
#endif

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	::psi = *Info;
	::fsf = *(Info->FSF);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = PF_PRELOAD;
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->MinFarVersion = MAKEFARVERSION(3,0,0,3000,VS_RELEASE);

	Info->Version = MAKEFARVERSION(1,0,0,0,VS_RELEASE);
	
	Info->Guid = guid_ImgC0;
	Info->Title = L"img_c0";
	Info->Description = L"Show image information in the C0 column";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

void cpytag(char* &psz, const char* src, int nMax)
{
	for (int i = 0; i < nMax && *src; i++)
		*(psz++) = *(src++);
	*psz = 0;
}

intptr_t WINAPI GetContentFieldsW(const struct GetContentFieldsInfo *Info)
{
	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"img")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
			return 1;
	}
	return 0;
}

intptr_t WINAPI GetContentDataW(struct GetContentDataInfo *Info)
{
	if (!Info)
		return FALSE;
	LPCWSTR FilePath = Info->FilePath;
	wchar_t** CustomData = NULL;

	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"img")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
		{
			CustomData = (wchar_t**)(Info->Values + i);
		}
	}

	if (!FilePath || !CustomData)
		return FALSE;
	int nLen = lstrlenW(FilePath);

	// Путь должен быть, просто имя файла (без пути) - пропускаем
	LPCWSTR pszSlash, pszDot = NULL;
	pszSlash = wcsrchr(FilePath, L'\\');
	if (pszSlash) pszDot = wcsrchr(pszSlash, L'.');
	if (!pszDot || !pszDot[0]) return FALSE;

	//TODO: Если появится возможность просмотра нескольких байт - хорошо бы это делать один раз на пачку плагинов?
	if (lstrcmpiW(pszDot, L".bmp")
		&& lstrcmpiW(pszDot, L".jpg") && lstrcmpiW(pszDot, L".jpeg")
		&& lstrcmpiW(pszDot, L".png")
		&& lstrcmpiW(pszDot, L".gif")
		&& lstrcmpiW(pszDot, L".tif") && lstrcmpiW(pszDot, L".tiff")
		&& lstrcmpiW(pszDot, L".ico")
		)
	{
		return FALSE;
	}
	
	BOOL lbRc = FALSE;

	LPCTSTR pszUNCPath = FilePath;

	HANDLE hFile = CreateFileW(pszUNCPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		if (dwErr == ERROR_SHARING_VIOLATION)
		{
			hFile = CreateFileW(pszUNCPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0,0);
			//if (hFile == INVALID_HANDLE_VALUE)
			//	hFile = CreateFileW(FilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0,0);
		}
	}
	LARGE_INTEGER nSize = {{0}};
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (GetFileSizeEx(hFile, &nSize)
			&& (nSize.QuadPart >= 16) && (nSize.QuadPart <= 1024*1024*16))
		{
			lbRc = TRUE;
		}
		CloseHandle(hFile);
	}

	// May continue to gdi+?
	if (lbRc)
	{
		if (!decoder_initialized)
		{
			decoder_initialized = decoder.Init();
			if (!decoder_initialized)
				return FALSE;
		}
		GDIPlusImage img(&decoder);
		if (!img.Open(false, pszUNCPath, NULL, 0))
			return FALSE;
		if (!img.SelectPage(0))
			return FALSE;
		wchar_t szInfo[128];
		wsprintf(szInfo, L"%ux%u %uBPP", img.lWidth, img.lHeight, img.nBPP);
		if (img.nPages > 1)
			wsprintf(szInfo+lstrlen(szInfo), L" {%u}", img.nPages);
		int nLen = lstrlen(szInfo)+1;
		*CustomData = (wchar_t*)malloc(nLen*2);
		lstrcpy(*CustomData, szInfo);
	}

	return lbRc;
}

void WINAPI FreeContentDataW(const struct GetContentDataInfo *Info)
{
	for (size_t i = 0; i < Info->Count; i++)
	{
		if (!fsf.LStricmp(Info->Names[i], L"img")
			|| !fsf.LStricmp(Info->Names[i], L"C0"))
		{
			if (Info->Values[i])
			{
				wchar_t* ptr = (wchar_t*)Info->Values[i];
				if (ptr) free(ptr);
			}
		}
	}
}

void WINAPI ExitFARW(const struct ExitInfo *Info)
{
	decoder.Close();
}
