#ifndef PICTUREVIEWPLUGIN_H
#define PICTUREVIEWPLUGIN_H

#include <PshPack4.h>

// Интерфейс PVD субплагинов-декодеров, v2.0

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

// Кодировка всех строк char*: UTF-8.
// Кодировка строк wchar_t*: UTF-16

// При использовании 2-й версии интерфейса субплагин должен работать
// структурами и функциями с суффиксом 2.
// 
// Внимание! 2-я версия интерфейса несовместима с PictureView 1.41!
// 
// Для внесения поддержки интерфейса версии 1 достаточно
// включить в ваш проект файл PVD1Helper.cpp и экспортировать в 
// вашем pvd содержащиеся в PVD1Helper.cpp функции:
//	pvdInit
//	pvdExit
//	pvdPluginInfo
//	pvdFileOpen
//	pvdPageInfo
//	pvdPageDecode
//	pvdPageFree
//	pvdFileClose



// Версия интерфейса PVD, описываемая этим заголовочным файлом.
// Именно это значение рекомендуется возвращать в pvdInit при успешной инициализации.
#define PVD_CURRENT_INTERFACE_VERSION 1
// Именно это значение рекомендуется возвращать в pvdInit2 при успешной инициализации.
#define PVD_UNICODE_INTERFACE_VERSION 2

// Флаги инициализации субплагина (только версия 2 интерфейса)
#define PVD_IPF_COMPAT_MODE     64  // Плагин второй версии вызван в режиме совместимости с первой (через PVD1Helper.cpp)

// Режим работы субплагина (только версия 2 интерфейса)
#define PVD_IP_DECODE        1      // Декодер (как версия 1)
#define PVD_IP_TRANSFORM     2      // Содержит функции для Lossless transform (интерфейс в разработке)
#define PVD_IP_DISPLAY       4      // Может быть использован вместо встроенной в PicView поддержки DX (интерфейс в разработке)
#define PVD_IP_PROCESSING    8      // PostProcessing operations (интерфейс в разработке)
#define PVD_IP_MULTITHREAD   0x100  // Декодер может вызываться одновременно в разных нитях
#define PVD_IP_ALLOWCACHE    0x200  // PicView может длительное время НЕ вызывать pvdFileClose2 для кэширования декодированных изображений
//#define PVD_IP_CANREFINE     0x400  // Декодер поддерживает улучшенный рендеринг (алиасинг)
#define PVD_IP_CANREFINERECT 0x800  // Декодер поддерживает улучшенный рендеринг (алиасинг) заданного региона
#define PVD_IP_CANREDUCE     0x1000 // Декодер может загрузить большое изображение с уменьшенным масштабом
#define PVD_IP_NOTERMINAL    0x2000 // Этот модуль дисплея нельзя использовать в терминальных консолях (удаленный доступ)
#define PVD_IP_PRIVATE       0x4000 // Имеет смысл только в сочетании (PVD_IP_DECODE|PVD_IP_DISPLAY). 
                                    // Этот субплагин НЕ может быть использован как универсальный модуль вывода
                                    // он умеет отображать только то, что декодировал сам
#define PVD_IP_DIRECT        0x8000 // "Быстрый" модуль вывода. Например, вывод через DirectX.
#define PVD_IP_FOLDER       0x10000 // Модуль может показывать Thumbnail для папок (a'la проводник Windows в режиме эскизов)
#define PVD_IP_CANDESCALE   0x20000 // Поддерживается рендеринг в режиме уменьшения
#define PVD_IP_CANUPSCALE   0x40000 // Поддерживается рендеринг в режиме увеличения

// Предопределенные коды ошибок
#define PVD_ERROR_BASE                     (0xC9870000)
#define PVD_ERROR_NOTENOUGHDISPLAYMEM      (PVD_ERROR_BASE+0)   // 0xC9870000

// Анимированное многостраничное изображение
#define PVD_IIF_ANIMATED      1
// Если декодер поддерживает улучшенный рендеринг (алиасинг) заданного региона
#define PVD_IIF_CAN_REFINE    2
// Устанавливается декодером, если требуется наличие физического файла (с буфером декодер работать не умеет)
#define PVD_IIF_FILE_REQUIRED 4
// Многостраничное изображение соответсвует скану книги или журнала
// Скорее всего первая страница является лицевой, а далее следуют развороты (левая и правая страница)
#define PVD_IIF_MAGAZINE  0x100


// Данные декодированного изображения доступны только для чтения
#define PVD_IDF_READONLY          1
// **** Следующие флаги используются только во 2-й версии интерфейса
// pImage содержит 32бита на пиксель и старший байт является альфа каналом
#define PVD_IDF_ALPHA             2  // для режимов с палитрой - старший байт берется из палитры
// Один из цветов (субплагин возвращает его в pvdInfoDecode2.TransparentColor) считать прозрачным (только версия 2 интерфейса)
#define PVD_IDF_TRANSPARENT       4  // pvdInfoDecode2.TransparentColor содержит COLORREF прозрачного цвета
#define PVD_IDF_TRANSPARENT_INDEX 8  // pvdInfoDecode2.TransparentColor содержит индекс прозрачного цвета
#define PVD_IDF_ASDISPLAY        16  // Субплагин является активным модулем вывода (можно не возвращать битмап изображения)
#define PVD_IDF_PRIVATE_DISPLAY  32  // "Внутреннее" представление, которое может быть использовано для вывода
	                                 // только этим же субплагином (у плагина должен быть флаг PVD_IP_DISPLAY)
#define PVD_IDF_COMPAT_MODE      64  // Плагин второй версии вызван в режиме совместимости с первой (через PVD1Helper.cpp)

//// *** Поворот по EXIF?
//#define PVD_IDF_ANG0    0x00000
//#define PVD_IDF_ANG90   0x10000
//#define PVD_IDF_ANG180  0x20000
//#define PVD_IDF_ANG270  0x40000



// pvdInfoPlugin - информация о субплагине
struct pvdInfoPlugin
{
	UINT32 Priority;          // [OUT] приоритет субплагина
	const char *pName;        // [OUT] имя субплагина
	const char *pVersion;     // [OUT] версия субплагина
	const char *pComments;    // [OUT] комментарии о субплагине: для чего предназначен, кто автор субплагина, ...
};
struct pvdInfoPlugin2
{
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	UINT32 Flags;                // [OUT] Возможные флаги PVD_IP_xxx
	const wchar_t *pName;        // [OUT] имя субплагина
	const wchar_t *pVersion;     // [OUT] версия субплагина
	const wchar_t *pComments;    // [OUT] комментарии о субплагине: для чего предназначен, кто автор субплагина, ...
	UINT32 Priority;             // [OUT] приоритет субплагина; используется только для новых субплагинов при формировании
	                             //       списка декодеров. Чем выше Priority тем выше в списке он будет размещен.
	HMODULE hModule;             // [IN]  HANDLE загруженной библиотеки
};

typedef LONG_PTR (__stdcall* pvdCallSehedProc2)(LONG_PTR Param1, LONG_PTR Param2);

// pvdInitPlugin2 - параметры инициализации субплагина
struct pvdInitPlugin2
{
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	UINT32 nMaxVersion;          // [IN]  максимальная версия интерфейса, которую может поддержать PictureView
	const wchar_t *pRegKey;      // [IN]  ключ реестра, в котором субплагин может хранить свои настройки.
	                             //       Например для субплагина pvd_bmp.pvd этот ключ будет таким
	                             //       "Software\\Far2\\Plugins\\PictureView\\pvd_bmp.pvd\\Settings"
	                             //       естественно в HKEY_CURRENT_USER.
	                             //       Субплагину рекомендуется сразу создать умолчательные значения (если
	                             //       ему есть что настраивать), чтобы сам PicView мог дать пользователю
	                             //       возможность настроить эти значения.
	DWORD  nErrNumber;           // [OUT] внутренний (для субплагина) код ошибки инициализации
	                             //       Субплагину желательно экспортировать функцию pvdTranslateError2
	void  *pContext;             // [OUT] контекст, используемый при обращении к субплагину

	// Some helper functions
	void  *pCallbackContext;     // [IN]  Это значение должно быть передано в функции, идущие ниже
	// 0-информация, 1-предупреждение, 2-ошибка
	void (__stdcall* MessageLog)(void *pCallbackContext, const wchar_t* asMessage, UINT32 anSeverity);
	// asExtList может содержать '*' (тогда всегда TRUE) или '.' (TRUE если asExt пусто). Сравнение регистронезависимое
	BOOL (__stdcall* ExtensionMatch)(wchar_t* asExtList, const wchar_t* asExt);
	//
	HMODULE hModule;             // [IN]  HANDLE загруженной библиотеки
	//
	BOOL (__stdcall* CallSehed)(pvdCallSehedProc2 CalledProc, LONG_PTR Param1, LONG_PTR Param2, LONG_PTR* Result);
	int (__stdcall* SortExtensions)(wchar_t* pszExtensions);
	int (__stdcall* MulDivI32)(int a, int b, int c);  // (__int64)a * b / c;
	UINT (__stdcall* MulDivU32)(UINT a, UINT b, UINT c);  // (uint)((unsigned long long)(a)*(b)/(c))
	UINT (__stdcall* MulDivU32R)(UINT a, UINT b, UINT c);  // (uint)(((unsigned long long)(a)*(b) + (c)/2)/(c))
	int (__stdcall* MulDivIU32R)(int a, UINT b, UINT c);  // (int)(((long long)(a)*(b) + (c)/2)/(c))
//	PRAGMA_ERROR("Добавить функцию декодирования PNG. Чтобы облегчить вызов из ICO.PVD да и не использовать gdi+ при открытии CMYK");
	UINT32 Flags;                // [IN] возможные флаги: PVD_IPF_xxx
};

// pvdFormats2 - список поддерживаемых форматов
struct pvdFormats2
{
	UINT32 cbSize;					// [IN]  размер структуры в байтах
	const wchar_t *pActive;			// [OUT] Список активных расширений через запятую.
									//			Это расширения, которые модуль умеет "хорошо" открывать.
									//       Здесь допускается указание "*" означающее, что
									//       субплагин является универсальным.
									//       Если при распознавании ни один из субплагинов не подошел по расширению -
									//       PicView все равно попытается открыть файл субплагином, если расширение
									//       не указано в списке его запрещенных.
    const wchar_t *pForbidden;		// [OUT] Список игнорируемых расширений через запятую.
									//       Для файлов с указанными расширениями субплагин не
									//       будет вызываться вообще. Укажите "." для игнорирования
									//       файлов без расширений.
    const wchar_t *pInactive;		// [OUT] Список неактивных расширений через запятую.
									//       Здесь указываются расширения, которые модуль может открыть
									//       "в принципе", но возможно, с проблемами.
    // !!! Списки являются "умолчанием". Пользователь может перенастроить список расширений.
};

// pvdInfoImage - информация о файле
struct pvdInfoImage
{
	UINT32 nPages;            // [OUT] количество страниц изображения
	UINT32 Flags;             // [OUT] возможные флаги: PVD_IIF_xxx
	const char *pFormatName;  // [OUT] название формата файла
	const char *pCompression; // [OUT] алгоритм сжатия
	const char *pComments;    // [OUT] различные комментарии о файле
};
struct pvdInfoImage2
{
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	void   *pImageContext;       // [IN]  При вызове из pvdFileOpen2 может быть НЕ NULL,
								 //       если плагин экспортирует функцию pvdFileDetect2
								 // [OUT] Контекст, используемый при обращении к файлу
	UINT32 nPages;               // [OUT] количество страниц изображения
								 //       При вызове из pvdFileDetect2 заполнение необязательно
	UINT32 Flags;                // [OUT] возможные флаги: PVD_IIF_xxx
								 //       При выозве из pvdFileDetect2 критичен флаг PVD_IIF_FILE_REQUIRED
	const wchar_t *pFormatName;  // [OUT] название формата файла
								 //       При вызове из pvdFileDetect2 заполнение необязательно, но желательно
	const wchar_t *pCompression; // [OUT] алгоритм сжатия
								 //       При вызове из pvdFileDetect2 заполнение необязательно
	const wchar_t *pComments;    // [OUT] различные комментарии о файле
								 //       При вызове из pvdFileDetect2 заполнение необязательно
	//
	DWORD  nErrNumber;           // [OUT] Информация об ошибке детектирования формата файла
	                             //       Субплагину желательно экспортировать функцию pvdTranslateError2
	                             //       При возврате кода (< 0x7FFFFFFF) PicView считает что
	                             //       субплагину просто неизвестен этот формат файла. PicView
	                             //       не будет отображать эту ошибку пользователю, если сможет
	                             //       открыть файл каким-то другим субплагином-декодером.

	DWORD nReserved, nReserver2;
};


// pvdRegionInfo2 - может использоваться для рендеринга с алиасингом, если 
// декодер поддерживает рендеринг требуемого региона. Используется при отображении
// картинок с (Zoom != 100%)
struct pvdRegionInfo2 {
	UINT32 cbSize;   // [IN]  размер структуры в байтах
	RECT   rSource;  // Область исходного изображения, которую PicView хочет отобразить в улучшенном качестве
	SIZE   szTarget; // Размер области на экране
};


// pvdInfoPage - информация о странице изображения
struct pvdInfoPage
{
	UINT32 lWidth;            // [OUT] ширина страницы
	UINT32 lHeight;           // [OUT] высота страницы
	UINT32 nBPP;              // [OUT] количество бит на пиксель (только информационное поле - в рассчётах не используется)
	UINT32 lFrameTime;        // [OUT] для анимированных изображений - длительность отображения страницы в тысячных секунды;
	                          //       иначе - не используется
};
struct pvdInfoPage2
{
	UINT32 cbSize;            // [IN]  размер структуры в байтах
	UINT32 iPage;             // [IN]  номер страницы (0-based)
	UINT32 lWidth;            // [OUT] ширина страницы
	UINT32 lHeight;           // [OUT] высота страницы
	UINT32 nBPP;              // [OUT] количество бит на пиксель (только информационное поле - в расчётах не используется)
	UINT32 lFrameTime;        // [OUT] для анимированных изображений - длительность отображения страницы в тысячных секунды;
	                          //       иначе - не используется
	// Plugin output
	DWORD  nErrNumber;           // [OUT] Информация об ошибке
                                 //       Субплагину желательно экспортировать функцию pvdTranslateError2
	UINT32 nPages;               // [OUT] 0, или плагин может скорректировать количество страниц изображения
	const wchar_t *pFormatName;  // [OUT] NULL или плагин может скорректировать название формата файла
	const wchar_t *pCompression; // [OUT] NULL или плагин может скорректировать алгоритм сжатия
};

// Сейчас допустимы только "PVD_CM_BGR" и "PVD_CM_BGRA"
enum pvdColorModel
{
	PVD_CM_UNKNOWN =  0,  // -- Такое изображение скорее всего не будет показано плагином
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
	PVD_CM_PRIVATE = 12,  // Только если Дисплей==Декодер и биты не возвращаются
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


// pvdInfoDecode - информация о декодированном изображении
struct pvdInfoDecode
{
	BYTE   *pImage;            // [OUT] указатель на данные изображения
	UINT32 *pPalette;          // [OUT] указатель на палитру изображения, используется в форматах 8 и меньше бит на пиксель
	UINT32 Flags;              // [OUT] возможные флаги: PVD_IDF_READONLY
	UINT32 nBPP;               // [OUT] количество бит на пиксель в декодированном изображении
                               //       PicView не отображает это значение пользователю - в заголовке 
	                           //       выводится pvdInfoPage.nBPP, так что можно спокойно делать преобразования
	UINT32 nColorsUsed;        // [OUT] количество используемых цветов в палитре; если 0, то используются все возможные цвета
	INT32  lImagePitch;        // [OUT] модуль - длина строки декодированного изображения в байтах;
	                           //       положительные значения - строки идут сверху вниз, отрицательные - снизу вверх
};


struct pvdInfoDecode2
{
	UINT32 cbSize;             // [IN]  размер структуры в байтах
	UINT32 iPage;              // [IN]  Номер декодируемой страницы (0-based)
	UINT32 lWidth, lHeight;    // [IN]  Рекомендуемый размер декодированного изображения (если декодер поддерживает антиалиасинг)
	                           // [OUT] Размер декодированной области (pImage)
	UINT32 nBPP;               // [IN]  PicView может запросить предпочтительный формат (пока не используется)
	                           // [OUT] количество бит на пиксель в декодированном изображении
	                           //       при использовании 32 бит может быть указан флаг PVD_IDF_ALPHA
                               //       PicView не отображает это значение пользователю - в заголовке 
	                           //       выводится pvdInfoPage2.nBPP, так что можно спокойно делать преобразования
	INT32  lImagePitch;        // [OUT] модуль - длина строки декодированного изображения в байтах;
	                           //       положительные значения - строки идут сверху вниз, отрицательные - снизу вверх
	UINT32 Flags;              // [IN]  PVD_IDF_ASDISPLAY | PVD_IDF_COMPAT_MODE
	                           // [OUT] возможные флаги: PVD_IDF_*
	union {
	RGBQUAD TransparentColor;  // [OUT] if (Flags&PVD_IDF_TRANSPARENT) - содержит цвет, который считается прозрачным
	DWORD  nTransparentColor;  //       if (Flags&PVD_IDF_TRANSPARENT_INDEX) - содержит индекс прозрачного цвета
	};                         // Внимание! При указании флага PVD_IDF_ALPHA - Transparent игнорируется

	BYTE   *pImage;            // [OUT] указатель на данные изображения в допустимом формате
	                           //       формат зависит от nBPP
	                           //       1,4,8 бит - ражимы с палитрой
	                           //       16 бит - каждый компонент цвета состоит из 5 бит (BGR)
	                           //       24 бит - 8 бит на компонент (BGR)
	                           //       32 бит - 8 бит на компонент (BGR или BGRA при указании PVD_IDF_ALPHA)
	UINT32 *pPalette;          // [OUT] указатель на палитру изображения, используется в форматах 8 и меньше бит на пиксель
	UINT32 nColorsUsed;        // [OUT] количество используемых цветов в палитре; если 0, то используются все возможные цвета
	                           //       (пока не используется, палитра должна содержать [1<<nBPP] цветов)

	DWORD  nErrNumber;         // [OUT] Информация об ошибке декодирования
	                           //       Субплагину желательно экспортировать функцию pvdTranslateError2

	LPARAM lParam;             // [OUT] Субплагин может использовать это поле на свое усмотрение

	pvdColorModel  ColorModel; // [OUT] Сейчас поддерживаются только PVD_CM_BGR & PVD_CM_BGRA
	DWORD          Precision;  // [RESERVED] bits per channel (8,12,16bit) 
	POINT          Origin;     // [RESERVED] m_x & m_y; Interface apl returns m_x=0; m_y=Ymax;
	float          PAR;        // [RESERVED] Pixel aspect ratio definition 
	pvdOrientation Orientation;// [RESERVED]
	UINT32 nPages;             // [OUT] 0, или плагин может скорректировать количество страниц изображения
	const wchar_t *pFormatName;  // [OUT] NULL или плагин может скорректировать название формата файла
	const wchar_t *pCompression; // [OUT] NULL или плагин может скорректировать алгоритм сжатия
	union {
	  RGBQUAD BackgroundColor; // [IN] Декодер может использовать это поле при рендеринге
	  DWORD  nBackgroundColor; //      прозрачных изображений
	};
	UINT32 lSrcWidth,          // [OUT] Декодер может уточнить размер исходного изображения. Именно этот размер
	       lSrcHeight;         // [OUT] будет показан в заголовке окна (через TitleTemplate). Если уточнение не
	                           //       не требуется - возвращайте {0,0}.
};
struct pvdInfoDecodeStep2
{
	UINT32 cbSize;             // [IN]  размер структуры в байтах
	RECT   rc;                 // [OUT] Положение декодированного прямоугольника (pImage)
	UINT32 nBPP;               // [IN]  PicView может запросить предпочтительный формат (пока не используется)
	                           // [OUT] количество бит на пиксель в декодированном изображении
	                           //       при использовании 32 бит может быть указан флаг PVD_IDF_ALPHA
	INT32  lImagePitch;        // [OUT] модуль - длина строки декодированного изображения в байтах;
	                           //       положительные значения - строки идут сверху вниз, отрицательные - снизу вверх
	UINT32 Flags;              // [OUT] возможные флаги: PVD_IDF_xxx
	union {
	RGBQUAD TransparentColor;  // [OUT] if (Flags&PVD_IDF_TRANSPARENT) - содержит цвет, который считается прозрачным
	DWORD  nTransparentColor;  //       if (Flags&PVD_IDF_TRANSPARENT_INDEX) - содержит индекс прозрачного цвета
	};                         // Внимание! При указании флага PVD_IDF_ALPHA - Transparent игнорируется

	BYTE   *pImage;            // [OUT] указатель на данные изображения в допустимом формате
	                           //       формат зависит от nBPP
	                           //       1,4,8 бит - ражимы с палитрой
	                           //       16 бит - каждый компонент цвета состоит из 5 бит (BGR)
	                           //       24 бит - 8 бит на компонент (BGR)
	                           //       32 бит - 8 бит на компонент (BGR или BGRA при указании PVD_IDF_ALPHA)
	UINT32 *pPalette;          // [OUT] указатель на палитру изображения, используется в форматах 8 и меньше бит на пиксель
	UINT32 nColorsUsed;        // [OUT] количество используемых цветов в палитре; если 0, то используются все возможные цвета
	                           //       (пока не используесят, палитра должна содержать [1<<nBPP] цветов)

	DWORD  nErrNumber;         // [OUT] Информация об ошибке декодирования
	                           //       Субплагину желательно экспортировать функцию pvdTranslateError2

	LPARAM lParam;             // [OUT] Субплагин может использовать это поле на свое усмотрение

	pvdColorModel  ColorModel; // [OUT] Сейчас поддерживаются только PVD_CM_BGR & PVD_CM_BGRA
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
	UINT32 cbSize;              // [IN]  размер структуры в байтах
	pvdImageTransformOp  Op;    // [IN]  что делаем
	const wchar_t* pFileName;   // [IN]  кого меняем
	const wchar_t* pTargetName; // [IN]  если NULL - результат нужно записать обратно в файл
	                            //       иначе - имя результирующего файла
	DWORD  nErrNumber;          // [OUT] Информация об ошибке преобразования
	                            //       Субплагину желательно экспортировать функцию pvdTranslateError2
};








#if defined(__GNUC__)
extern "C"{
#endif

// pvdPluginInfo, pvdPluginInfo2 - общая информация о субплагине
//  Вызывается: !!! Функция может быть вызвана в любой момент (в том числе ДО вызова pvdInit)
//  Аргументы:
//   pPluginInfo - указатель на структуру с информацией о субплагине для заполнения субплагином
void __stdcall pvdPluginInfo(pvdInfoPlugin *pPluginInfo);
void __stdcall pvdPluginInfo2(pvdInfoPlugin2 *pPluginInfo);

// pvdTranslateError2 - расшифровать внутренний код ошибки
//  Вызывается: Функция может быть вызвана после функции, вернувшей ошибку. Цель - показать пользователю вместо числа
//   нечто более понятное. Например "ERR_ERR_HEADER" или "Memory allocation failed (60Mb)".
//  Аргументы:
//   nErrNumber - код ошибки возвращенный субплагином в поле nErrNumber одной из структур
//   pszErrInfo - буфер, в который субплагин должен скопировать описание ошибки
//   nBufLen    - размер буфера в wchar_t
//  Возвращаемое значение: должен возвращать TRUE. иначе считается что в буфер ничего не помещалось
BOOL __stdcall pvdTranslateError2(DWORD nErrNumber, wchar_t *pszErrInfo, int nBufLen);



// pvdInit - инициализация плагина
//  Вызывается: один раз - сразу после загрузки плагина, если субплагин не экспортирует pvdInit2,
//   (!) или если используется старая версия PictureView.
//  Возвращаемое значение: версия интерфейса плагина
//   Если это число не понравится вызывающей программе, то вызовется pvdExit и субплагин будет выгружен.
//   Учтите, что субплагин может быть использован и в старых версиях PictureView, которые не поймут
//   версию интерфейса выше 1.
//   0 - ошибка загрузки/инициализации плагина.
UINT32 __stdcall pvdInit(void);

// pvdInit2 - инициализация плагина
//  Вызывается: один раз - сразу после загрузки плагина. Рекомендуется проверять nMaxVersion
//   и по возможности (если субплагин поддерживает) не возвращать версию выше nMaxVersion.
//  Аргументы:
//   pInit - параметры инициализации
//  Возвращаемое значение: версия интерфейса плагина
//   Пока это 2. Рекомендуется использовать макроопределение PVD_UNICODE_INTERFACE_VERSION
//   0 - ошибка загрузки/инициализации плагина.
//   Если это число не понравится вызывающей программе, то вызовется pvdExit2 и если субплагин
//   не экспортирует функцию pvdInit - будет выгружен. Иначе - будет попытка инициализации
//   плагина как версии 1.
UINT32 __stdcall pvdInit2(pvdInitPlugin2* pInit);


// pvdGetFormats2
//  Вызывается: Только после успешного pvdInit2, один раз.
//   После первого обнаружения плагина в PicView. Результат 
//   кешируется, и вообще списки расширений могут быть настроены пользователем.
//   При попытке открыть файл PicView делает следующее:
//   По нисходящей приоритетов проверяет соответсвует ли файл допустимым расширениям
//   (pvdFormats2.pSupported) или субплагин универсален (pvdFormats2.pSupported == "*").
//   Если соответсвует - следует попытка открытия.
//   При неудачном открытии или если расширение отсутствует среди допустимых -
//   PicView переходит к следующему плагину.
//   Если ни один субплагин не смог открыть файл - PicView идет на второй заход и
//   пытается открыть файл всеми плагинами (кроме тех, которые файл уже "нюхали")
//   плагинами, в списке pvdFormats2.pIgnored не указано обрабатываемое расширение.
//  Аргументы:
//   pFormats
//  Возвращаемое значение: нет
void __stdcall pvdGetFormats2(void *pContext, pvdFormats2* pFormats);


// pvdExit - завершение работы с субплагином
//  Вызывается: один раз - непосредственно перед выгрузкой плагина
void __stdcall pvdExit(void);
//   pContext    - контекст, возвращённый субплагином в pvdInit2
void __stdcall pvdExit2(void *pContext);

// pvdFileOpen - открытие файла: субплагин решает, хочет ли он декодировать файл, и заполняет общую информацию о файле
//  Вызывается: при открытии файла
//  Аргументы:
//   pFileName   - имя открываемого файла
//   lFileSize   - длина открываемого файла в байтах. Если 0, то файл отсутствует, а переданный параметром pBuf буфер
//                 содержит все возможные данные и будет доступен вплоть до вызова pvdFileClose.
//   pBuf        - буфер, содержащий начало открываемого файла
//   lBuf        - длина буфера pBuf в байтах. Рекомендуется предоставлять не менее 16 Кб.
//   pImageInfo  - указатель на структуру с информацией о файле для заполнения субплагином, если он может декодировать файл
//   ppImageContext - указатель на контекст. Через этот параметр субплагин может вернуть контекст - произвольное значение,
//                 которое будет передаваться ему при вызове других функций работы с данным файлом. Следует иметь в
//                 виду, что одним экземпляром плагина в один момент времени может декодироваться несколько файлов,
//                 поэтому рекомендуется использовать контекст, а не внутренние глобальные переменные плагина.
//  Возвращаемое значение: TRUE - если субплагин может декодировать указанный файл; иначе - FALSE
BOOL __stdcall pvdFileOpen(const char *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage *pImageInfo, void **ppImageContext);
BOOL __stdcall pvdFileOpen2(void *pContext, const wchar_t *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage2 *pImageInfo);

// Необязательная, но желательная. После успешного вызова pvdFileDetect2 будет вызван pvdFileOpen2 с уже установленным pImageInfo->pImageContext.
// В отличие от pvdFileOpen2 должна:
// 1) выполнять минимум операций.
// 2) определять нужен ли физический файл для декодирования (в этом случае обязательно
//    установить флаг PVD_IIF_FILE_REQUIRED, иначе pvdFileOpen2 может быть вызван без файла).
// 3) возвращать дескриптор (pImageInfo->pImageContext) который будет передан в pvdFileOpen2. 
//    Допустим возврат (pImageInfo->pImageContext==NULL), если дескриптор не создавался
// Учтите! Что при отсутствии PVD_IIF_FILE_REQUIRED, при вызове pvdFileOpen2 (pBuf & lBuf)
//         скорее всего буду уже другими! (данные будут дочитаны)
BOOL __stdcall pvdFileDetect2(void *pContext, const wchar_t *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage2 *pImageInfo);

// pvdPageInfo - информация о странице изображения
//  Вызывается: между удачным pvdFileOpen и pvdFileClose
//  Аргументы:
//   pImageContext - контекст, возвращённый субплагином в pvdFileOpen
//   iPage         - номер страницы изображения (нумерация начинается с 0)
//   pPageInfo     - указатель на структуру с информацией о странице изображения для заполнения субплагином
//  Возвращаемое значение: TRUE - при успешном выполнении; иначе - FALSE
BOOL __stdcall pvdPageInfo(void *pImageContext, UINT32 iPage, pvdInfoPage *pPageInfo);
//   pContext      - контекст, возвращённый субплагином в pvdFileOpen
//   pImageContext - контекст, возвращаемый в поле pvdInfoImage2.pImageContext
BOOL __stdcall pvdPageInfo2(void *pContext, void *pImageContext, pvdInfoPage2 *pPageInfo);


// pvdDecodeCallback - функция, указатель на которую передаётся в pvdPageDecode
//  Вызывается: субплагином из pvdPageDecode
//   Не обязательно, но рекомендуется периодически вызывать, если декодирование может занять длительное время.
//  Аргументы:
//   pDecodeCallbackContext - контекст, переданный соответствующим параметром pvdPageDecode/pvdPageDecode2
//   iStep  - номер текущего шага декодирования (нумерация от 0 до nSteps - 1)
//   nSteps - общее количество шагов декодирования
//  Возвращаемое значение: TRUE - продолжение декодирования; FALSE - декодирование следует прервать
typedef BOOL (__stdcall *pvdDecodeCallback)(void *pDecodeCallbackContext, UINT32 iStep, UINT32 nSteps);
//  Дополнительные аргументы версии 2:
//   pImagePart - если не NULL - содержит часть декодированного изображения, которую PicView может
//                сразу отобразить на экране
typedef BOOL (__stdcall *pvdDecodeCallback2)(void *pDecodeCallbackContext2, UINT32 iStep, UINT32 nSteps,
											 pvdInfoDecodeStep2* pImagePart);

// pvdPageDecode - декодирование страницы изображения
//  Вызывается: между удачным pvdFileOpen и pvdFileClose
//  Аргументы:
//   pImageContext  - контекст, возвращённый субплагином в pvdFileOpen
//   iPage          - номер страницы изображения (нумерация начинается с 0)
//   pDecodeInfo    - указатель на структуру с информацией о декодированном изображении для заполнения субплагином
//   DecodeCallback - указатель на функцию, через которую субплагин может информировать вызывающую программу о ходе
//                    декодирования; NULL, если такая функция не предоставляется
//   pDecodeCallbackContext - контекст, передаваемый в DecodeCallback
//  Возвращаемое значение: TRUE - при успешном выполнении; иначе - FALSE
BOOL __stdcall pvdPageDecode(void *pImageContext, UINT32 iPage, pvdInfoDecode *pDecodeInfo, 
							 pvdDecodeCallback DecodeCallback, void *pDecodeCallbackContext);
//  Дополнительные аргументы версии 2:
//   pContext      - контекст, возвращённый субплагином в pvdInit2
//   pImageContext - контекст, возвращаемый субплагином в pvdFileOpen2
BOOL __stdcall pvdPageDecode2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo, 
							  pvdDecodeCallback2 DecodeCallback, void *pDecodeCallbackContext);

// pvdPageFree - освобождение декодированного изображения
//  Вызывается: после удачного pvdPageDecode, когда декодированное изображение больше не нужно
//  Аргументы:
//   pImageContext - контекст, возвращённый субплагином в pvdFileOpen
//   pDecodeInfo   - указатель на структуру с информацией о декодированном изображении, заполненной в pvdPageDecode
void __stdcall pvdPageFree(void *pImageContext, pvdInfoDecode *pDecodeInfo);
//  Дополнительные аргументы версии 2:
//   pContext      - контекст, возвращённый субплагином в pvdInit2
//   pImageContext - контекст, возвращаемый субплагином в pvdFileOpen2
void __stdcall pvdPageFree2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo);

// pvdFileClose - закрытие файла
//  Вызывается: после удачного pvdFileOpen[2], когда файл больше не нужен
//  Аргументы:
//   pImageContext - контекст, возвращённый субплагином в pvdFileOpen
void __stdcall pvdFileClose(void *pImageContext);
//  Дополнительные аргументы версии 2:
//   pContext      - контекст, возвращённый субплагином в pvdInit2
//   pImageContext - контекст, возвращаемый субплагином в pvdFileOpen2
void __stdcall pvdFileClose2(void *pContext, void *pImageContext);


/* ********************** */
// pvdTransform2 - lossless image transformations
//  Вызывается только если субплагин выставил флаг PVD_IP_TRANSFORM
void __stdcall pvdTransform2(void *pContext, pvdImageTransform2* pTransform);


/* ********************** */
/*     PVD_IP_DISPLAY     */
/* ********************** */
struct pvdInfoDisplayInit2
{
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	HWND hWnd;                   // [IN]  Окно может быть изменено в процессе работы
	DWORD nCMYKparts;
	DWORD *pCMYKpalette;
	DWORD nCMYKsize;
	DWORD uCMYK2RGB;
	DWORD nErrNumber;            // [OUT]
};
struct pvdInfoDisplayAttach2
{
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	HWND hWnd;                   // [IN]  Окно может быть изменено в процессе работы
	BOOL bAttach;                // [IN]  Подцепиться или отцепиться от hWnd
	DWORD nErrNumber;            // [OUT]
};
struct pvdInfoDisplayCreate2
{
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	pvdInfoDecode2* pImage;      // [IN]
	DWORD BackColor;             // [IN]  RGB background
	void* pDisplayContext;       // [OUT]
	DWORD nErrNumber;            // [OUT] код ошибки. Может вернуть PVD_ERROR_NOTENOUGHDISPLAYMEM
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
#define PVD_IDP_BEGIN     1   // Начало цикла отрисовки (создание экранного буфера)
#define PVD_IDP_PAINT     2   // Собственно отрисовка требуемой части изображения в требуемом прямоугольнике экрана
#define PVD_IDP_COLORFILL 3   // Залить прямоугольник экрана требуемым цветом
#define PVD_IDP_COMMIT    4   // Отобразить готовое из буфера на экран
#define PVD_IDP_CHESSFILL 5   // Вызывается перед PVD_IDP_PAINT при выводе "шахматки" на прозрачных картинках
//
typedef UINT32 PVD_IDPF_FLAGS;
static const PVD_IDPF_FLAGS
		PVD_IDPF_ZOOMING    = 0x001, // Сейчас идет зуммирование
		PVD_IDPF_PANNING    = 0x002, // Сейчас идет панорамирование
		PVD_IDPF_CHESSBOARD = 0x004, // При выводе прозрачной картинки плагину предлагается фон сделать "шахматкой"
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
	UINT32 cbSize;               // [IN]  размер структуры в байтах
	DWORD Operation;  // PVD_IDP_*
	HWND hWnd;                   // [IN]  Где рисовать
	HWND hParentWnd;             // [IN]
	union {
	RGBQUAD BackColor;  //
	DWORD  nBackColor;  //
	};
	RECT ImageRect;
	RECT DisplayRect;

	LPVOID pDrawContext; // Это поле может использоваться субплагином для хранения "HDC". Освобождать должен субплагин по команде PVD_IDP_COMMIT

	//RECT ParentRect;
	////DWORD BackColor;             // [IN]  RGB background
	//BOOL bFreePosition;
	//BOOL bCorrectMousePos;
	//POINT ViewCenter;
	//POINT DragBase;
	//UINT32 Zoom;
	//RECT rcGlobal;               // [IN]  в каком месте окна нужно показать изображение (остальное заливается фоном BackColor)
	//RECT rcCrop;                 // [IN]  прямоугольник отсечения (клиентская часть окна)
	DWORD nErrNumber;            // [OUT]
	
	DWORD nZoom; // [IN] передается только для информации. 0x10000 == 100%
	PVD_IDPF_FLAGS nFlags; // [IN] PVD_IDPF_*
	
	DWORD *pChessMate;
	DWORD uChessMateWidth;
	DWORD uChessMateHeight;

	// 
};
// Инициализация контекста дисплея. Используется тот pContext, который был получен в pvdInit2
BOOL __stdcall pvdDisplayInit2(void *pContext, pvdInfoDisplayInit2* pDisplayInit);
// Прицепиться или отцепиться от окна вывода
BOOL __stdcall pvdDisplayAttach2(void *pContext, pvdInfoDisplayAttach2* pDisplayAttach);
// Создать контекст для отображения картинки в pContext (перенос декодированных данных в видеопамять)
BOOL __stdcall pvdDisplayCreate2(void *pContext, pvdInfoDisplayCreate2* pDisplayCreate);
// Собственно отрисовка. Функция должна при необходимости выполнять "Stretch"
BOOL __stdcall pvdDisplayPaint2(void *pContext, void* pDisplayContext, pvdInfoDisplayPaint2* pDisplayPaint);
// Закрыть контекст для отображения картинки (освободить видеопамять)
void __stdcall pvdDisplayClose2(void *pContext, void* pDisplayContext);
// Закрыть модуль вывода (освобождение интерфейсов DX, отцепиться от окна)
void __stdcall pvdDisplayExit2(void *pContext);

//Примерная последовательность вызова функций при работе модуля Display
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
/* Необязательные функции */
/* ********************** */

// Вызывается после нажатия ОК пользователем в диалоге настроек субплагина-декодера.
void __stdcall pvdReloadConfig2(void *pContext);

// Затребована остановка всех операций контекста (декодирование, преобразование, и пр.)
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
