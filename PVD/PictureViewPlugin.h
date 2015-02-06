#ifndef PICTUREVIEWPLUGIN_H
#define PICTUREVIEWPLUGIN_H

#include <PshPack4.h>

// ��������� PVD �����������-���������, v2.0

/**************************************************************************
Copyright (c) 2009 Skakov Pavel
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

EXCEPTION:
PictureView plugins that use this header file can be distributed under any
other possible license with no implications from the above license on them.
**************************************************************************/

// ��������� ���� ����� char*: UTF-8.
// ��������� ����� wchar_t*: UTF-16

// ��� ������������� 2-� ������ ���������� ��������� ������ ��������
// ����������� � ��������� � ��������� 2.
// 
// ��������! 2-� ������ ���������� ������������ � PictureView 1.41!
// 
// ��� �������� ��������� ���������� ������ 1 ����������
// �������� � ��� ������ ���� PVD1Helper.cpp � �������������� � 
// ����� pvd ������������ � PVD1Helper.cpp �������:
//	pvdInit
//	pvdExit
//	pvdPluginInfo
//	pvdFileOpen
//	pvdPageInfo
//	pvdPageDecode
//	pvdPageFree
//	pvdFileClose



// ������ ���������� PVD, ����������� ���� ������������ ������.
// ������ ��� �������� ������������� ���������� � pvdInit ��� �������� �������������.
#define PVD_CURRENT_INTERFACE_VERSION 1
// ������ ��� �������� ������������� ���������� � pvdInit2 ��� �������� �������������.
#define PVD_UNICODE_INTERFACE_VERSION 2

// ����� ������������� ���������� (������ ������ 2 ����������)
#define PVD_IPF_COMPAT_MODE     64  // ������ ������ ������ ������ � ������ ������������� � ������ (����� PVD1Helper.cpp)

// ����� ������ ���������� (������ ������ 2 ����������)
#define PVD_IP_DECODE        1      // ������� (��� ������ 1)
#define PVD_IP_TRANSFORM     2      // �������� ������� ��� Lossless transform (��������� � ����������)
#define PVD_IP_DISPLAY       4      // ����� ���� ����������� ������ ���������� � PicView ��������� DX (��������� � ����������)
#define PVD_IP_PROCESSING    8      // PostProcessing operations (��������� � ����������)
#define PVD_IP_MULTITHREAD   0x100  // ������� ����� ���������� ������������ � ������ �����
#define PVD_IP_ALLOWCACHE    0x200  // PicView ����� ���������� ����� �� �������� pvdFileClose2 ��� ����������� �������������� �����������
//#define PVD_IP_CANREFINE     0x400  // ������� ������������ ���������� ��������� (��������)
#define PVD_IP_CANREFINERECT 0x800  // ������� ������������ ���������� ��������� (��������) ��������� �������
#define PVD_IP_CANREDUCE     0x1000 // ������� ����� ��������� ������� ����������� � ����������� ���������
#define PVD_IP_NOTERMINAL    0x2000 // ���� ������ ������� ������ ������������ � ������������ �������� (��������� ������)
#define PVD_IP_PRIVATE       0x4000 // ����� ����� ������ � ��������� (PVD_IP_DECODE|PVD_IP_DISPLAY). 
                                    // ���� ��������� �� ����� ���� ����������� ��� ������������� ������ ������
                                    // �� ����� ���������� ������ ��, ��� ����������� ���
#define PVD_IP_DIRECT        0x8000 // "�������" ������ ������. ��������, ����� ����� DirectX.
#define PVD_IP_FOLDER       0x10000 // ������ ����� ���������� Thumbnail ��� ����� (a'la ��������� Windows � ������ �������)
#define PVD_IP_CANDESCALE   0x20000 // �������������� ��������� � ������ ����������
#define PVD_IP_CANUPSCALE   0x40000 // �������������� ��������� � ������ ����������

// ���������������� ���� ������
#define PVD_ERROR_BASE                     (0xC9870000)
#define PVD_ERROR_NOTENOUGHDISPLAYMEM      (PVD_ERROR_BASE+0)   // 0xC9870000

// ������������� ��������������� �����������
#define PVD_IIF_ANIMATED      1
// ���� ������� ������������ ���������� ��������� (��������) ��������� �������
#define PVD_IIF_CAN_REFINE    2
// ��������������� ���������, ���� ��������� ������� ����������� ����� (� ������� ������� �������� �� �����)
#define PVD_IIF_FILE_REQUIRED 4
// ��������������� ����������� ������������ ����� ����� ��� �������
// ������ ����� ������ �������� �������� �������, � ����� ������� ��������� (����� � ������ ��������)
#define PVD_IIF_MAGAZINE  0x100


// ������ ��������������� ����������� �������� ������ ��� ������
#define PVD_IDF_READONLY          1
// **** ��������� ����� ������������ ������ �� 2-� ������ ����������
// pImage �������� 32���� �� ������� � ������� ���� �������� ����� �������
#define PVD_IDF_ALPHA             2  // ��� ������� � �������� - ������� ���� ������� �� �������
// ���� �� ������ (��������� ���������� ��� � pvdInfoDecode2.TransparentColor) ������� ���������� (������ ������ 2 ����������)
#define PVD_IDF_TRANSPARENT       4  // pvdInfoDecode2.TransparentColor �������� COLORREF ����������� �����
#define PVD_IDF_TRANSPARENT_INDEX 8  // pvdInfoDecode2.TransparentColor �������� ������ ����������� �����
#define PVD_IDF_ASDISPLAY        16  // ��������� �������� �������� ������� ������ (����� �� ���������� ������ �����������)
#define PVD_IDF_PRIVATE_DISPLAY  32  // "����������" �������������, ������� ����� ���� ������������ ��� ������
	                                 // ������ ���� �� ����������� (� ������� ������ ���� ���� PVD_IP_DISPLAY)
#define PVD_IDF_COMPAT_MODE      64  // ������ ������ ������ ������ � ������ ������������� � ������ (����� PVD1Helper.cpp)

//// *** ������� �� EXIF?
//#define PVD_IDF_ANG0    0x00000
//#define PVD_IDF_ANG90   0x10000
//#define PVD_IDF_ANG180  0x20000
//#define PVD_IDF_ANG270  0x40000



// pvdInfoPlugin - ���������� � ����������
struct pvdInfoPlugin
{
	UINT32 Priority;          // [OUT] ��������� ����������
	const char *pName;        // [OUT] ��� ����������
	const char *pVersion;     // [OUT] ������ ����������
	const char *pComments;    // [OUT] ����������� � ����������: ��� ���� ������������, ��� ����� ����������, ...
};
struct pvdInfoPlugin2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	UINT32 Flags;                // [OUT] ��������� ����� PVD_IP_xxx
	const wchar_t *pName;        // [OUT] ��� ����������
	const wchar_t *pVersion;     // [OUT] ������ ����������
	const wchar_t *pComments;    // [OUT] ����������� � ����������: ��� ���� ������������, ��� ����� ����������, ...
	UINT32 Priority;             // [OUT] ��������� ����������; ������������ ������ ��� ����� ����������� ��� ������������
	                             //       ������ ���������. ��� ���� Priority ��� ���� � ������ �� ����� ��������.
	HMODULE hModule;             // [IN]  HANDLE ����������� ����������
};

typedef LONG_PTR (__stdcall* pvdCallSehedProc2)(LONG_PTR Param1, LONG_PTR Param2);

// pvdInitPlugin2 - ��������� ������������� ����������
struct pvdInitPlugin2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	UINT32 nMaxVersion;          // [IN]  ������������ ������ ����������, ������� ����� ���������� PictureView
	const wchar_t *pRegKey;      // [IN]  ���� �������, � ������� ��������� ����� ������� ���� ���������.
	                             //       �������� ��� ���������� pvd_bmp.pvd ���� ���� ����� �����
	                             //       "Software\\Far2\\Plugins\\PictureView\\pvd_bmp.pvd\\Settings"
	                             //       ����������� � HKEY_CURRENT_USER.
	                             //       ���������� ������������� ����� ������� ������������� �������� (����
	                             //       ��� ���� ��� �����������), ����� ��� PicView ��� ���� ������������
	                             //       ����������� ��������� ��� ��������.
	DWORD  nErrNumber;           // [OUT] ���������� (��� ����������) ��� ������ �������������
	                             //       ���������� ���������� �������������� ������� pvdTranslateError2
	void  *pContext;             // [OUT] ��������, ������������ ��� ��������� � ����������

	// Some helper functions
	void  *pCallbackContext;     // [IN]  ��� �������� ������ ���� �������� � �������, ������ ����
	// 0-����������, 1-��������������, 2-������
	void (__stdcall* MessageLog)(void *pCallbackContext, const wchar_t* asMessage, UINT32 anSeverity);
	// asExtList ����� ��������� '*' (����� ������ TRUE) ��� '.' (TRUE ���� asExt �����). ��������� �������������������
	BOOL (__stdcall* ExtensionMatch)(wchar_t* asExtList, const wchar_t* asExt);
	//
	HMODULE hModule;             // [IN]  HANDLE ����������� ����������
	//
	BOOL (__stdcall* CallSehed)(pvdCallSehedProc2 CalledProc, LONG_PTR Param1, LONG_PTR Param2, LONG_PTR* Result);
	int (__stdcall* SortExtensions)(wchar_t* pszExtensions);
	int (__stdcall* MulDivI32)(int a, int b, int c);  // (__int64)a * b / c;
	UINT (__stdcall* MulDivU32)(UINT a, UINT b, UINT c);  // (uint)((unsigned long long)(a)*(b)/(c))
	UINT (__stdcall* MulDivU32R)(UINT a, UINT b, UINT c);  // (uint)(((unsigned long long)(a)*(b) + (c)/2)/(c))
	int (__stdcall* MulDivIU32R)(int a, UINT b, UINT c);  // (int)(((long long)(a)*(b) + (c)/2)/(c))
//	PRAGMA_ERROR("�������� ������� ������������� PNG. ����� ��������� ����� �� ICO.PVD �� � �� ������������ gdi+ ��� �������� CMYK");
	UINT32 Flags;                // [IN] ��������� �����: PVD_IPF_xxx
};

// pvdFormats2 - ������ �������������� ��������
struct pvdFormats2
{
	UINT32 cbSize;					// [IN]  ������ ��������� � ������
	const wchar_t *pActive;			// [OUT] ������ �������� ���������� ����� �������.
									//			��� ����������, ������� ������ ����� "������" ���������.
									//       ����� ����������� �������� "*" ����������, ���
									//       ��������� �������� �������������.
									//       ���� ��� ������������� �� ���� �� ����������� �� ������� �� ���������� -
									//       PicView ��� ����� ���������� ������� ���� �����������, ���� ����������
									//       �� ������� � ������ ��� �����������.
    const wchar_t *pForbidden;		// [OUT] ������ ������������ ���������� ����� �������.
									//       ��� ������ � ���������� ������������ ��������� ��
									//       ����� ���������� ������. ������� "." ��� �������������
									//       ������ ��� ����������.
    const wchar_t *pInactive;		// [OUT] ������ ���������� ���������� ����� �������.
									//       ����� ����������� ����������, ������� ������ ����� �������
									//       "� ��������", �� ��������, � ����������.
    // !!! ������ �������� "����������". ������������ ����� ������������� ������ ����������.
};

// pvdInfoImage - ���������� � �����
struct pvdInfoImage
{
	UINT32 nPages;            // [OUT] ���������� ������� �����������
	UINT32 Flags;             // [OUT] ��������� �����: PVD_IIF_xxx
	const char *pFormatName;  // [OUT] �������� ������� �����
	const char *pCompression; // [OUT] �������� ������
	const char *pComments;    // [OUT] ��������� ����������� � �����
};
struct pvdInfoImage2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	void   *pImageContext;       // [IN]  ��� ������ �� pvdFileOpen2 ����� ���� �� NULL,
								 //       ���� ������ ������������ ������� pvdFileDetect2
								 // [OUT] ��������, ������������ ��� ��������� � �����
	UINT32 nPages;               // [OUT] ���������� ������� �����������
								 //       ��� ������ �� pvdFileDetect2 ���������� �������������
	UINT32 Flags;                // [OUT] ��������� �����: PVD_IIF_xxx
								 //       ��� ������ �� pvdFileDetect2 �������� ���� PVD_IIF_FILE_REQUIRED
	const wchar_t *pFormatName;  // [OUT] �������� ������� �����
								 //       ��� ������ �� pvdFileDetect2 ���������� �������������, �� ����������
	const wchar_t *pCompression; // [OUT] �������� ������
								 //       ��� ������ �� pvdFileDetect2 ���������� �������������
	const wchar_t *pComments;    // [OUT] ��������� ����������� � �����
								 //       ��� ������ �� pvdFileDetect2 ���������� �������������
	//
	DWORD  nErrNumber;           // [OUT] ���������� �� ������ �������������� ������� �����
	                             //       ���������� ���������� �������������� ������� pvdTranslateError2
	                             //       ��� �������� ���� (< 0x7FFFFFFF) PicView ������� ���
	                             //       ���������� ������ ���������� ���� ������ �����. PicView
	                             //       �� ����� ���������� ��� ������ ������������, ���� ������
	                             //       ������� ���� �����-�� ������ �����������-���������.

	DWORD nReserved, nReserver2;
};


// pvdRegionInfo2 - ����� �������������� ��� ���������� � ����������, ���� 
// ������� ������������ ��������� ���������� �������. ������������ ��� �����������
// �������� � (Zoom != 100%)
struct pvdRegionInfo2 {
	UINT32 cbSize;   // [IN]  ������ ��������� � ������
	RECT   rSource;  // ������� ��������� �����������, ������� PicView ����� ���������� � ���������� ��������
	SIZE   szTarget; // ������ ������� �� ������
};


// pvdInfoPage - ���������� � �������� �����������
struct pvdInfoPage
{
	UINT32 lWidth;            // [OUT] ������ ��������
	UINT32 lHeight;           // [OUT] ������ ��������
	UINT32 nBPP;              // [OUT] ���������� ��� �� ������� (������ �������������� ���� - � ��������� �� ������������)
	UINT32 lFrameTime;        // [OUT] ��� ������������� ����������� - ������������ ����������� �������� � �������� �������;
	                          //       ����� - �� ������������
};
struct pvdInfoPage2
{
	UINT32 cbSize;            // [IN]  ������ ��������� � ������
	UINT32 iPage;             // [IN]  ����� �������� (0-based)
	UINT32 lWidth;            // [OUT] ������ ��������
	UINT32 lHeight;           // [OUT] ������ ��������
	UINT32 nBPP;              // [OUT] ���������� ��� �� ������� (������ �������������� ���� - � �������� �� ������������)
	UINT32 lFrameTime;        // [OUT] ��� ������������� ����������� - ������������ ����������� �������� � �������� �������;
	                          //       ����� - �� ������������
	// Plugin output
	DWORD  nErrNumber;           // [OUT] ���������� �� ������
                                 //       ���������� ���������� �������������� ������� pvdTranslateError2
	UINT32 nPages;               // [OUT] 0, ��� ������ ����� ��������������� ���������� ������� �����������
	const wchar_t *pFormatName;  // [OUT] NULL ��� ������ ����� ��������������� �������� ������� �����
	const wchar_t *pCompression; // [OUT] NULL ��� ������ ����� ��������������� �������� ������
};

// ������ ��������� ������ "PVD_CM_BGR" � "PVD_CM_BGRA"
enum pvdColorModel
{
	PVD_CM_UNKNOWN =  0,  // -- ����� ����������� ������ ����� �� ����� �������� ��������
	PVD_CM_GRAY    =  1,  // "Gray scale"  -- UNSUPPORTED !!!
	PVD_CM_AG      =  2,  // "Alpha_Gray"  -- UNSUPPORTED !!!
	PVD_CM_RGB     =  3,  // "RGB"         -- UNSUPPORTED !!!
	PVD_CM_BGR     =  4,  // "BGR"
	PVD_CM_YCBCR   =  5,  // "YCbCr"       -- UNSUPPORTED !!!
	PVD_CM_CMYK    =  6,  // "CMYK"
	PVD_CM_YCCK    =  7,  // "YCCK"        -- UNSUPPORTED !!!
	PVD_CM_YUV     =  8,  // "YUV"         -- UNSUPPORTED !!!
	PVD_CM_BGRA    =  9,  // "BGRA"
	PVD_CM_RGBA    = 10,  // "RGBA"        -- UNSUPPORTED !!!
	PVD_CM_ABRG    = 11,  // "ABRG"        -- UNSUPPORTED !!!
	PVD_CM_PRIVATE = 12,  // ������ ���� �������==������� � ���� �� ������������
};

enum pvdOrientation
{
	PVD_Ornt_Default         = 0,
	PVD_Ornt_TopLeft         = 1, // The 0th row is at the visual top of the image, and the 0th column is the visual left-hand side.
	PVD_Ornt_TopRight        = 2, // The 0th row is at the visual top of the image, and the 0th column is the visual right-hand side.
	PVD_Ornt_BottomRight     = 3, // The 0th row is at the visual bottom of the image, and the 0th column is the visual right-hand side.
	PVD_Ornt_BottomLeft      = 4, // The 0th row is at the visual bottom of the image, and the 0th column is the visual left-hand side.
	PVD_Ornt_LeftTop         = 5, // The 0th row is the visual left-hand side of the image, and the 0th column is the visual top.
	PVD_Ornt_RightTop        = 6, // The 0th row is the visual right-hand side of the image, and the 0th column is the visual top.
	PVD_Ornt_RightBottom     = 7, // The 0th row is the visual right-hand side of the image, and the 0th column is the visual bottom.
	PVD_Ornt_LeftBottom      = 8  // The 0th row is the visual left-hand side of the image, and the 0th column is the visual bottom.
};


// pvdInfoDecode - ���������� � �������������� �����������
struct pvdInfoDecode
{
	BYTE   *pImage;            // [OUT] ��������� �� ������ �����������
	UINT32 *pPalette;          // [OUT] ��������� �� ������� �����������, ������������ � �������� 8 � ������ ��� �� �������
	UINT32 Flags;              // [OUT] ��������� �����: PVD_IDF_READONLY
	UINT32 nBPP;               // [OUT] ���������� ��� �� ������� � �������������� �����������
                               //       PicView �� ���������� ��� �������� ������������ - � ��������� 
	                           //       ��������� pvdInfoPage.nBPP, ��� ��� ����� �������� ������ ��������������
	UINT32 nColorsUsed;        // [OUT] ���������� ������������ ������ � �������; ���� 0, �� ������������ ��� ��������� �����
	INT32  lImagePitch;        // [OUT] ������ - ����� ������ ��������������� ����������� � ������;
	                           //       ������������� �������� - ������ ���� ������ ����, ������������� - ����� �����
};


struct pvdInfoDecode2
{
	UINT32 cbSize;             // [IN]  ������ ��������� � ������
	UINT32 iPage;              // [IN]  ����� ������������ �������� (0-based)
	UINT32 lWidth, lHeight;    // [IN]  ������������� ������ ��������������� ����������� (���� ������� ������������ ������������)
	                           // [OUT] ������ �������������� ������� (pImage)
	UINT32 nBPP;               // [IN]  PicView ����� ��������� ���������������� ������ (���� �� ������������)
	                           // [OUT] ���������� ��� �� ������� � �������������� �����������
	                           //       ��� ������������� 32 ��� ����� ���� ������ ���� PVD_IDF_ALPHA
                               //       PicView �� ���������� ��� �������� ������������ - � ��������� 
	                           //       ��������� pvdInfoPage2.nBPP, ��� ��� ����� �������� ������ ��������������
	INT32  lImagePitch;        // [OUT] ������ - ����� ������ ��������������� ����������� � ������;
	                           //       ������������� �������� - ������ ���� ������ ����, ������������� - ����� �����
	UINT32 Flags;              // [IN]  PVD_IDF_ASDISPLAY | PVD_IDF_COMPAT_MODE
	                           // [OUT] ��������� �����: PVD_IDF_*
	union {
	RGBQUAD TransparentColor;  // [OUT] if (Flags&PVD_IDF_TRANSPARENT) - �������� ����, ������� ��������� ����������
	DWORD  nTransparentColor;  //       if (Flags&PVD_IDF_TRANSPARENT_INDEX) - �������� ������ ����������� �����
	};                         // ��������! ��� �������� ����� PVD_IDF_ALPHA - Transparent ������������

	BYTE   *pImage;            // [OUT] ��������� �� ������ ����������� � ���������� �������
	                           //       ������ ������� �� nBPP
	                           //       1,4,8 ��� - ������ � ��������
	                           //       16 ��� - ������ ��������� ����� ������� �� 5 ��� (BGR)
	                           //       24 ��� - 8 ��� �� ��������� (BGR)
	                           //       32 ��� - 8 ��� �� ��������� (BGR ��� BGRA ��� �������� PVD_IDF_ALPHA)
	UINT32 *pPalette;          // [OUT] ��������� �� ������� �����������, ������������ � �������� 8 � ������ ��� �� �������
	UINT32 nColorsUsed;        // [OUT] ���������� ������������ ������ � �������; ���� 0, �� ������������ ��� ��������� �����
	                           //       (���� �� ������������, ������� ������ ��������� [1<<nBPP] ������)

	DWORD  nErrNumber;         // [OUT] ���������� �� ������ �������������
	                           //       ���������� ���������� �������������� ������� pvdTranslateError2

	LPARAM lParam;             // [OUT] ��������� ����� ������������ ��� ���� �� ���� ����������

	pvdColorModel  ColorModel; // [OUT] ������ �������������� ������ PVD_CM_BGR & PVD_CM_BGRA
	DWORD          Precision;  // [RESERVED] bits per channel (8,12,16bit) 
	POINT          Origin;     // [RESERVED] m_x & m_y; Interface apl returns m_x=0; m_y=Ymax;
	float          PAR;        // [RESERVED] Pixel aspect ratio definition 
	pvdOrientation Orientation;// [RESERVED]
	UINT32 nPages;             // [OUT] 0, ��� ������ ����� ��������������� ���������� ������� �����������
	const wchar_t *pFormatName;  // [OUT] NULL ��� ������ ����� ��������������� �������� ������� �����
	const wchar_t *pCompression; // [OUT] NULL ��� ������ ����� ��������������� �������� ������
	union {
	  RGBQUAD BackgroundColor; // [IN] ������� ����� ������������ ��� ���� ��� ����������
	  DWORD  nBackgroundColor; //      ���������� �����������
	};
	UINT32 lSrcWidth,          // [OUT] ������� ����� �������� ������ ��������� �����������. ������ ���� ������
	       lSrcHeight;         // [OUT] ����� ������� � ��������� ���� (����� TitleTemplate). ���� ��������� ��
	                           //       �� ��������� - ����������� {0,0}.
};
struct pvdInfoDecodeStep2
{
	UINT32 cbSize;             // [IN]  ������ ��������� � ������
	RECT   rc;                 // [OUT] ��������� ��������������� �������������� (pImage)
	UINT32 nBPP;               // [IN]  PicView ����� ��������� ���������������� ������ (���� �� ������������)
	                           // [OUT] ���������� ��� �� ������� � �������������� �����������
	                           //       ��� ������������� 32 ��� ����� ���� ������ ���� PVD_IDF_ALPHA
	INT32  lImagePitch;        // [OUT] ������ - ����� ������ ��������������� ����������� � ������;
	                           //       ������������� �������� - ������ ���� ������ ����, ������������� - ����� �����
	UINT32 Flags;              // [OUT] ��������� �����: PVD_IDF_xxx
	union {
	RGBQUAD TransparentColor;  // [OUT] if (Flags&PVD_IDF_TRANSPARENT) - �������� ����, ������� ��������� ����������
	DWORD  nTransparentColor;  //       if (Flags&PVD_IDF_TRANSPARENT_INDEX) - �������� ������ ����������� �����
	};                         // ��������! ��� �������� ����� PVD_IDF_ALPHA - Transparent ������������

	BYTE   *pImage;            // [OUT] ��������� �� ������ ����������� � ���������� �������
	                           //       ������ ������� �� nBPP
	                           //       1,4,8 ��� - ������ � ��������
	                           //       16 ��� - ������ ��������� ����� ������� �� 5 ��� (BGR)
	                           //       24 ��� - 8 ��� �� ��������� (BGR)
	                           //       32 ��� - 8 ��� �� ��������� (BGR ��� BGRA ��� �������� PVD_IDF_ALPHA)
	UINT32 *pPalette;          // [OUT] ��������� �� ������� �����������, ������������ � �������� 8 � ������ ��� �� �������
	UINT32 nColorsUsed;        // [OUT] ���������� ������������ ������ � �������; ���� 0, �� ������������ ��� ��������� �����
	                           //       (���� �� ������������, ������� ������ ��������� [1<<nBPP] ������)

	DWORD  nErrNumber;         // [OUT] ���������� �� ������ �������������
	                           //       ���������� ���������� �������������� ������� pvdTranslateError2

	LPARAM lParam;             // [OUT] ��������� ����� ������������ ��� ���� �� ���� ����������

	pvdColorModel  ColorModel; // [OUT] ������ �������������� ������ PVD_CM_BGR & PVD_CM_BGRA
	DWORD          Precision;  // [RESERVED] bits per channel (8,12,16bit) 
	POINT          Origin;     // [RESERVED] m_x & m_y; Interface apl returns m_x=0; m_y=Ymax;
	float          PAR;        // [RESERVED] Pixel aspect ratio definition 
	pvdOrientation Orientation;// [RESERVED]
};


// pvdImageTransform2 - lossless image transformations
typedef UINT32 pvdImageTransformOp;
static const pvdImageTransformOp
	pvdRotate90       = 0x0001,
	pvdRotate180      = 0x0002,
	pvdRotate270      = 0x0003,
	pvdRotateMask     = 0x0003,

	pvdFlipHorizontal = 0x0100,
	pvdFlipVertical   = 0x0200,
	pvdFlipMask       = 0x0300,

	pvdNoTransform    = 0x0000
;
struct pvdImageTransform2
{
	UINT32 cbSize;              // [IN]  ������ ��������� � ������
	pvdImageTransformOp  Op;    // [IN]  ��� ������
	const wchar_t* pFileName;   // [IN]  ���� ������
	const wchar_t* pTargetName; // [IN]  ���� NULL - ��������� ����� �������� ������� � ����
	                            //       ����� - ��� ��������������� �����
	DWORD  nErrNumber;          // [OUT] ���������� �� ������ ��������������
	                            //       ���������� ���������� �������������� ������� pvdTranslateError2
};








#if defined(__GNUC__)
extern "C"{
#endif

// pvdPluginInfo, pvdPluginInfo2 - ����� ���������� � ����������
//  ����������: !!! ������� ����� ���� ������� � ����� ������ (� ��� ����� �� ������ pvdInit)
//  ���������:
//   pPluginInfo - ��������� �� ��������� � ����������� � ���������� ��� ���������� �����������
void __stdcall pvdPluginInfo(pvdInfoPlugin *pPluginInfo);
void __stdcall pvdPluginInfo2(pvdInfoPlugin2 *pPluginInfo);

// pvdTranslateError2 - ������������ ���������� ��� ������
//  ����������: ������� ����� ���� ������� ����� �������, ��������� ������. ���� - �������� ������������ ������ �����
//   ����� ����� ��������. �������� "ERR_ERR_HEADER" ��� "Memory allocation failed (60Mb)".
//  ���������:
//   nErrNumber - ��� ������ ������������ ����������� � ���� nErrNumber ����� �� ��������
//   pszErrInfo - �����, � ������� ��������� ������ ����������� �������� ������
//   nBufLen    - ������ ������ � wchar_t
//  ������������ ��������: ������ ���������� TRUE. ����� ��������� ��� � ����� ������ �� ����������
BOOL __stdcall pvdTranslateError2(DWORD nErrNumber, wchar_t *pszErrInfo, int nBufLen);



// pvdInit - ������������� �������
//  ����������: ���� ��� - ����� ����� �������� �������, ���� ��������� �� ������������ pvdInit2,
//   (!) ��� ���� ������������ ������ ������ PictureView.
//  ������������ ��������: ������ ���������� �������
//   ���� ��� ����� �� ���������� ���������� ���������, �� ��������� pvdExit � ��������� ����� ��������.
//   ������, ��� ��������� ����� ���� ����������� � � ������ ������� PictureView, ������� �� ������
//   ������ ���������� ���� 1.
//   0 - ������ ��������/������������� �������.
UINT32 __stdcall pvdInit(void);

// pvdInit2 - ������������� �������
//  ����������: ���� ��� - ����� ����� �������� �������. ������������� ��������� nMaxVersion
//   � �� ����������� (���� ��������� ������������) �� ���������� ������ ���� nMaxVersion.
//  ���������:
//   pInit - ��������� �������������
//  ������������ ��������: ������ ���������� �������
//   ���� ��� 2. ������������� ������������ ���������������� PVD_UNICODE_INTERFACE_VERSION
//   0 - ������ ��������/������������� �������.
//   ���� ��� ����� �� ���������� ���������� ���������, �� ��������� pvdExit2 � ���� ���������
//   �� ������������ ������� pvdInit - ����� ��������. ����� - ����� ������� �������������
//   ������� ��� ������ 1.
UINT32 __stdcall pvdInit2(pvdInitPlugin2* pInit);


// pvdGetFormats2
//  ����������: ������ ����� ��������� pvdInit2, ���� ���.
//   ����� ������� ����������� ������� � PicView. ��������� 
//   ����������, � ������ ������ ���������� ����� ���� ��������� �������������.
//   ��� ������� ������� ���� PicView ������ ���������:
//   �� ���������� ����������� ��������� ������������ �� ���� ���������� �����������
//   (pvdFormats2.pSupported) ��� ��������� ����������� (pvdFormats2.pSupported == "*").
//   ���� ������������ - ������� ������� ��������.
//   ��� ��������� �������� ��� ���� ���������� ����������� ����� ���������� -
//   PicView ��������� � ���������� �������.
//   ���� �� ���� ��������� �� ���� ������� ���� - PicView ���� �� ������ ����� �
//   �������� ������� ���� ����� ��������� (����� ���, ������� ���� ��� "������")
//   ���������, � ������ pvdFormats2.pIgnored �� ������� �������������� ����������.
//  ���������:
//   pFormats
//  ������������ ��������: ���
void __stdcall pvdGetFormats2(void *pContext, pvdFormats2* pFormats);


// pvdExit - ���������� ������ � �����������
//  ����������: ���� ��� - ��������������� ����� ��������� �������
void __stdcall pvdExit(void);
//   pContext    - ��������, ������������ ����������� � pvdInit2
void __stdcall pvdExit2(void *pContext);

// pvdFileOpen - �������� �����: ��������� ������, ����� �� �� ������������ ����, � ��������� ����� ���������� � �����
//  ����������: ��� �������� �����
//  ���������:
//   pFileName   - ��� ������������ �����
//   lFileSize   - ����� ������������ ����� � ������. ���� 0, �� ���� �����������, � ���������� ���������� pBuf �����
//                 �������� ��� ��������� ������ � ����� �������� ������ �� ������ pvdFileClose.
//   pBuf        - �����, ���������� ������ ������������ �����
//   lBuf        - ����� ������ pBuf � ������. ������������� ������������� �� ����� 16 ��.
//   pImageInfo  - ��������� �� ��������� � ����������� � ����� ��� ���������� �����������, ���� �� ����� ������������ ����
//   ppImageContext - ��������� �� ��������. ����� ���� �������� ��������� ����� ������� �������� - ������������ ��������,
//                 ������� ����� ������������ ��� ��� ������ ������ ������� ������ � ������ ������. ������� ����� �
//                 ����, ��� ����� ����������� ������� � ���� ������ ������� ����� �������������� ��������� ������,
//                 ������� ������������� ������������ ��������, � �� ���������� ���������� ���������� �������.
//  ������������ ��������: TRUE - ���� ��������� ����� ������������ ��������� ����; ����� - FALSE
BOOL __stdcall pvdFileOpen(const char *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage *pImageInfo, void **ppImageContext);
BOOL __stdcall pvdFileOpen2(void *pContext, const wchar_t *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage2 *pImageInfo);

// ��������������, �� �����������. ����� ��������� ������ pvdFileDetect2 ����� ������ pvdFileOpen2 � ��� ������������� pImageInfo->pImageContext.
// � ������� �� pvdFileOpen2 ������:
// 1) ��������� ������� ��������.
// 2) ���������� ����� �� ���������� ���� ��� ������������� (� ���� ������ �����������
//    ���������� ���� PVD_IIF_FILE_REQUIRED, ����� pvdFileOpen2 ����� ���� ������ ��� �����).
// 3) ���������� ���������� (pImageInfo->pImageContext) ������� ����� ������� � pvdFileOpen2. 
//    �������� ������� (pImageInfo->pImageContext==NULL), ���� ���������� �� ����������
// ������! ��� ��� ���������� PVD_IIF_FILE_REQUIRED, ��� ������ pvdFileOpen2 (pBuf & lBuf)
//         ������ ����� ���� ��� �������! (������ ����� ��������)
BOOL __stdcall pvdFileDetect2(void *pContext, const wchar_t *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage2 *pImageInfo);

// pvdPageInfo - ���������� � �������� �����������
//  ����������: ����� ������� pvdFileOpen � pvdFileClose
//  ���������:
//   pImageContext - ��������, ������������ ����������� � pvdFileOpen
//   iPage         - ����� �������� ����������� (��������� ���������� � 0)
//   pPageInfo     - ��������� �� ��������� � ����������� � �������� ����������� ��� ���������� �����������
//  ������������ ��������: TRUE - ��� �������� ����������; ����� - FALSE
BOOL __stdcall pvdPageInfo(void *pImageContext, UINT32 iPage, pvdInfoPage *pPageInfo);
//   pContext      - ��������, ������������ ����������� � pvdFileOpen
//   pImageContext - ��������, ������������ � ���� pvdInfoImage2.pImageContext
BOOL __stdcall pvdPageInfo2(void *pContext, void *pImageContext, pvdInfoPage2 *pPageInfo);


// pvdDecodeCallback - �������, ��������� �� ������� ��������� � pvdPageDecode
//  ����������: ����������� �� pvdPageDecode
//   �� �����������, �� ������������� ������������ ��������, ���� ������������� ����� ������ ���������� �����.
//  ���������:
//   pDecodeCallbackContext - ��������, ���������� ��������������� ���������� pvdPageDecode/pvdPageDecode2
//   iStep  - ����� �������� ���� ������������� (��������� �� 0 �� nSteps - 1)
//   nSteps - ����� ���������� ����� �������������
//  ������������ ��������: TRUE - ����������� �������������; FALSE - ������������� ������� ��������
typedef BOOL (__stdcall *pvdDecodeCallback)(void *pDecodeCallbackContext, UINT32 iStep, UINT32 nSteps);
//  �������������� ��������� ������ 2:
//   pImagePart - ���� �� NULL - �������� ����� ��������������� �����������, ������� PicView �����
//                ����� ���������� �� ������
typedef BOOL (__stdcall *pvdDecodeCallback2)(void *pDecodeCallbackContext2, UINT32 iStep, UINT32 nSteps,
											 pvdInfoDecodeStep2* pImagePart);

// pvdPageDecode - ������������� �������� �����������
//  ����������: ����� ������� pvdFileOpen � pvdFileClose
//  ���������:
//   pImageContext  - ��������, ������������ ����������� � pvdFileOpen
//   iPage          - ����� �������� ����������� (��������� ���������� � 0)
//   pDecodeInfo    - ��������� �� ��������� � ����������� � �������������� ����������� ��� ���������� �����������
//   DecodeCallback - ��������� �� �������, ����� ������� ��������� ����� ������������� ���������� ��������� � ����
//                    �������������; NULL, ���� ����� ������� �� ���������������
//   pDecodeCallbackContext - ��������, ������������ � DecodeCallback
//  ������������ ��������: TRUE - ��� �������� ����������; ����� - FALSE
BOOL __stdcall pvdPageDecode(void *pImageContext, UINT32 iPage, pvdInfoDecode *pDecodeInfo, 
							 pvdDecodeCallback DecodeCallback, void *pDecodeCallbackContext);
//  �������������� ��������� ������ 2:
//   pContext      - ��������, ������������ ����������� � pvdInit2
//   pImageContext - ��������, ������������ ����������� � pvdFileOpen2
BOOL __stdcall pvdPageDecode2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo, 
							  pvdDecodeCallback2 DecodeCallback, void *pDecodeCallbackContext);

// pvdPageFree - ������������ ��������������� �����������
//  ����������: ����� �������� pvdPageDecode, ����� �������������� ����������� ������ �� �����
//  ���������:
//   pImageContext - ��������, ������������ ����������� � pvdFileOpen
//   pDecodeInfo   - ��������� �� ��������� � ����������� � �������������� �����������, ����������� � pvdPageDecode
void __stdcall pvdPageFree(void *pImageContext, pvdInfoDecode *pDecodeInfo);
//  �������������� ��������� ������ 2:
//   pContext      - ��������, ������������ ����������� � pvdInit2
//   pImageContext - ��������, ������������ ����������� � pvdFileOpen2
void __stdcall pvdPageFree2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo);

// pvdFileClose - �������� �����
//  ����������: ����� �������� pvdFileOpen[2], ����� ���� ������ �� �����
//  ���������:
//   pImageContext - ��������, ������������ ����������� � pvdFileOpen
void __stdcall pvdFileClose(void *pImageContext);
//  �������������� ��������� ������ 2:
//   pContext      - ��������, ������������ ����������� � pvdInit2
//   pImageContext - ��������, ������������ ����������� � pvdFileOpen2
void __stdcall pvdFileClose2(void *pContext, void *pImageContext);


/* ********************** */
// pvdTransform2 - lossless image transformations
//  ���������� ������ ���� ��������� �������� ���� PVD_IP_TRANSFORM
void __stdcall pvdTransform2(void *pContext, pvdImageTransform2* pTransform);


/* ********************** */
/*     PVD_IP_DISPLAY     */
/* ********************** */
struct pvdInfoDisplayInit2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	HWND hWnd;                   // [IN]  ���� ����� ���� �������� � �������� ������
	DWORD nCMYKparts;
	DWORD *pCMYKpalette;
	DWORD nCMYKsize;
	DWORD uCMYK2RGB;
	DWORD nErrNumber;            // [OUT]
};
struct pvdInfoDisplayAttach2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	HWND hWnd;                   // [IN]  ���� ����� ���� �������� � �������� ������
	BOOL bAttach;                // [IN]  ����������� ��� ���������� �� hWnd
	DWORD nErrNumber;            // [OUT]
};
struct pvdInfoDisplayCreate2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	pvdInfoDecode2* pImage;      // [IN]
	DWORD BackColor;             // [IN]  RGB background
	void* pDisplayContext;       // [OUT]
	DWORD nErrNumber;            // [OUT] ��� ������. ����� ������� PVD_ERROR_NOTENOUGHDISPLAYMEM
	const wchar_t* pFileName;    // [IN]  Information only. Valid only in pvdDisplayCreate2
	UINT32 iPage;                // [IN]  Information only
	//
	//DWORD nChessMateColor1;
	//DWORD nChessMateColor2;
	//DWORD nChessMateSize;
	DWORD *pChessMate;
	DWORD uChessMateWidth;
	DWORD uChessMateHeight;
};
//
#define PVD_IDP_BEGIN     1   // ������ ����� ��������� (�������� ��������� ������)
#define PVD_IDP_PAINT     2   // ���������� ��������� ��������� ����� ����������� � ��������� �������������� ������
#define PVD_IDP_COLORFILL 3   // ������ ������������� ������ ��������� ������
#define PVD_IDP_COMMIT    4   // ���������� ������� �� ������ �� �����
#define PVD_IDP_CHESSFILL 5   // ���������� ����� PVD_IDP_PAINT ��� ������ "��������" �� ���������� ���������
//
typedef UINT32 PVD_IDPF_FLAGS;
static const PVD_IDPF_FLAGS
		PVD_IDPF_ZOOMING    = 0x001, // ������ ���� ������������
		PVD_IDPF_PANNING    = 0x002, // ������ ���� ���������������
		PVD_IDPF_CHESSBOARD = 0x004, // ��� ������ ���������� �������� ������� ������������ ��� ������� "���������"
		PVD_IDPF_ROTATE90   = 0x100, // 90 clockwise
		PVD_IDPF_ROTATE180  = 0x200, // 180 clockwise
		PVD_IDPF_ROTATE270  = 0x300, // 270 clockwise
		PVD_IDPF_ROTATEMASK = 0x300, // rotate mask
		PVD_IDPF_MIRRORHORZ = 0x400, // mirror left-right
		PVD_IDPF_MIRRORVERT = 0x800, // mirror up-down
		PVD_IDPF_MIRRORMASK = 0xC00, // mirror mask
		PVD_IDPF_NONT       = 0
;
//
struct pvdInfoDisplayPaint2
{
	UINT32 cbSize;               // [IN]  ������ ��������� � ������
	DWORD Operation;  // PVD_IDP_*
	HWND hWnd;                   // [IN]  ��� ��������
	HWND hParentWnd;             // [IN]
	union {
	RGBQUAD BackColor;  //
	DWORD  nBackColor;  //
	};
	RECT ImageRect;
	RECT DisplayRect;

	LPVOID pDrawContext; // ��� ���� ����� �������������� ����������� ��� �������� "HDC". ����������� ������ ��������� �� ������� PVD_IDP_COMMIT

	//RECT ParentRect;
	////DWORD BackColor;             // [IN]  RGB background
	//BOOL bFreePosition;
	//BOOL bCorrectMousePos;
	//POINT ViewCenter;
	//POINT DragBase;
	//UINT32 Zoom;
	//RECT rcGlobal;               // [IN]  � ����� ����� ���� ����� �������� ����������� (��������� ���������� ����� BackColor)
	//RECT rcCrop;                 // [IN]  ������������� ��������� (���������� ����� ����)
	DWORD nErrNumber;            // [OUT]
	
	DWORD nZoom; // [IN] ���������� ������ ��� ����������. 0x10000 == 100%
	PVD_IDPF_FLAGS nFlags; // [IN] PVD_IDPF_*
	
	DWORD *pChessMate;
	DWORD uChessMateWidth;
	DWORD uChessMateHeight;

	// 
};
// ������������� ��������� �������. ������������ ��� pContext, ������� ��� ������� � pvdInit2
BOOL __stdcall pvdDisplayInit2(void *pContext, pvdInfoDisplayInit2* pDisplayInit);
// ����������� ��� ���������� �� ���� ������
BOOL __stdcall pvdDisplayAttach2(void *pContext, pvdInfoDisplayAttach2* pDisplayAttach);
// ������� �������� ��� ����������� �������� � pContext (������� �������������� ������ � �����������)
BOOL __stdcall pvdDisplayCreate2(void *pContext, pvdInfoDisplayCreate2* pDisplayCreate);
// ���������� ���������. ������� ������ ��� ������������� ��������� "Stretch"
BOOL __stdcall pvdDisplayPaint2(void *pContext, void* pDisplayContext, pvdInfoDisplayPaint2* pDisplayPaint);
// ������� �������� ��� ����������� �������� (���������� �����������)
void __stdcall pvdDisplayClose2(void *pContext, void* pDisplayContext);
// ������� ������ ������ (������������ ����������� DX, ���������� �� ����)
void __stdcall pvdDisplayExit2(void *pContext);

//��������� ������������������ ������ ������� ��� ������ ������ Display
//pvdInit2
//pvdDisplayInit2
//pvdDisplayAttach2(TRUE)
//pvdDisplayCreate2
//pvdDisplayPaint2
//...
//pvdDisplayPaint2
//pvdDisplayClose2
//pvdDisplayAttach2(FALSE)
//pvdDisplayExit2
//pvdExit2



/* ********************** */
/* �������������� ������� */
/* ********************** */

// ���������� ����� ������� �� ������������� � ������� �������� ����������-��������.
void __stdcall pvdReloadConfig2(void *pContext);

// ����������� ��������� ���� �������� ��������� (�������������, ��������������, � ��.)
void __stdcall pvdCancel2(void *pContext);


#if defined(__GNUC__)
};
#endif


#include <PopPack.h>


// Some conversions
#define RED_FROM_RGBA(x) ((((DWORD)x) & 0xFF))
#define GREEN_FROM_RGBA(x) ((((DWORD)x) & 0xFF00) >> 8)
#define BLUE_FROM_RGBA(x) ((((DWORD)x) & 0xFF0000) >> 16)
#define ALPHA_FROM_RGBA(x) ((((DWORD)x) & 0xFF000000) >> 24)
//
#define BLUE_FROM_BGRA(x) ((((DWORD)x) & 0xFF))
#define GREEN_FROM_BGRA(x) ((((DWORD)x) & 0xFF00) >> 8)
#define RED_FROM_BGRA(x) ((((DWORD)x) & 0xFF0000) >> 16)
#define ALPHA_FROM_BGRA(x) ((((DWORD)x) & 0xFF000000) >> 24)
//
#define BGRA_FROM_RGBA(x) ((BLUE_FROM_RGBA(x)) | (((DWORD)x) & 0xFF00) | (RED_FROM_RGBA(x) << 16) | (ALPHA_FROM_RGBA(x) << 24))

#ifndef sizeofarray
	#define sizeofarray(array) (sizeof(array)/sizeof(*array))
#endif

#define PVD_CMYK2RGB_FAST      0
#define PVD_CMYK2RGB_APPROX    1
#define PVD_CMYK2RGB_PRECISE   2

#endif
