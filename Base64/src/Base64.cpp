
/*
Copyright (c) 2010 Maximus5
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
*/

/*
Этот простенький плагин предназначен для входа в файлы, закодированные по алгоритму base64.
Реагирует плагин на "вход в архив" (Enter & CtrlPgDn), показывает "<HostFile>.bin".
Его можно просмотреть, или скопировать на соседнюю панель (F5), подтверждений - нет.
*/


#include "header.h"
#include <ctype.h>
#include "version.h"

// {E9931181-10CB-44F3-9D51-5019F2F373E1}
static GUID guid_PluginGuid = 
{ 0xe9931181, 0x10cb, 0x44f3, { 0x9d, 0x51, 0x50, 0x19, 0xf2, 0xf3, 0x73, 0xe1 } };

// {128BFCCC-D64B-4232-B6EA-B4AF6E57C91E}
static GUID guid_PluginMenu = 
{ 0x128bfccc, 0xd64b, 0x4232, { 0xb6, 0xea, 0xb4, 0xaf, 0x6e, 0x57, 0xc9, 0x1e } };


// далее идет кусок BASE64 кодирования (MIME)

#define Assert64(Cond) if (!(Cond)) return -1

static const char Base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

/* (From RFC1521 and draft-ietf-dnssec-secext-03.txt)
   The following encoding technique is taken from RFC 1521 by Borenstein
   and Freed.  It is reproduced here in a slightly edited form for
   convenience.

   A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string.

                         Table 1: The Base64 Alphabet

      Value Encoding  Value Encoding  Value Encoding  Value Encoding
          0 A            17 R            34 i            51 z
          1 B            18 S            35 j            52 0
          2 C            19 T            36 k            53 1
          3 D            20 U            37 l            54 2
          4 E            21 V            38 m            55 3
          5 F            22 W            39 n            56 4
          6 G            23 X            40 o            57 5
          7 H            24 Y            41 p            58 6
          8 I            25 Z            42 q            59 7
          9 J            26 a            43 r            60 8
         10 K            27 b            44 s            61 9
         11 L            28 c            45 t            62 +
         12 M            29 d            46 u            63 /
         13 N            30 e            47 v
         14 O            31 f            48 w         (pad) =
         15 P            32 g            49 x
         16 Q            33 h            50 y

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a quantity.  When fewer than 24 input
   bits are available in an input group, zero bits are added (on the
   right) to form an integral number of 6-bit groups.  Padding at the
   end of the data is performed using the '=' character.

   Since all base64 input is an integral number of octets, only the
         -------------------------------------------------                       
   following cases can arise:
   
       (1) the final quantum of encoding input is an integral
           multiple of 24 bits; here, the final unit of encoded
       output will be an integral multiple of 4 characters
       with no "=" padding,
       (2) the final quantum of encoding input is exactly 8 bits;
           here, the final unit of encoded output will be two
       characters followed by two "=" padding characters, or
       (3) the final quantum of encoding input is exactly 16 bits;
           here, the final unit of encoded output will be three
       characters followed by one "=" padding character.
   */

int b64_encode(const unsigned char *src, int srclength, char *target, int targsize)
{
    int datalength = 0;
    unsigned char input[3];
    unsigned char output[4];
    int i;

    while (2 < srclength) {
        input[0] = *src++;
        input[1] = *src++;
        input[2] = *src++;
        srclength -= 3;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
        output[3] = input[2] & 0x3f;
        Assert64(output[0] < 64);
        Assert64(output[1] < 64);
        Assert64(output[2] < 64);
        Assert64(output[3] < 64);

        if (datalength + 4 > targsize)
            return (-1);
        target[datalength++] = Base64[output[0]];
        target[datalength++] = Base64[output[1]];
        target[datalength++] = Base64[output[2]];
        target[datalength++] = Base64[output[3]];
    }
    
    /* Now we worry about padding. */
    if (0 != srclength) {
        /* Get what's left. */
        input[0] = input[1] = input[2] = '\0';
        for (i = 0; i < srclength; i++)
            input[i] = *src++;
    
        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
        Assert64(output[0] < 64);
        Assert64(output[1] < 64);
        Assert64(output[2] < 64);

        if (datalength + 4 > targsize)
            return (-1);
        target[datalength++] = Base64[output[0]];
        target[datalength++] = Base64[output[1]];
        if (srclength == 1)
            target[datalength++] = Pad64;
        else
            target[datalength++] = Base64[output[2]];
        target[datalength++] = Pad64;
    }
    if (datalength >= targsize)
        return (-1);
    target[datalength] = '\0';  /* Returned value doesn't count \0. */
    return (datalength);
}

/* skips all whitespace anywhere.
   converts characters, four at a time, starting at (or after)
   src from base - 64 numbers into three 8 bit bytes in the target area.
   it returns the number of data bytes stored at the target, or -1 on error.
 */

// rnErrPos - one-based
int b64_decode(char const * src, unsigned char * target, int targsize, int* rnDstLen, int* rnErrPos)
{
    int tarindex, state, ch;
    const char *pos;
	char const * src_save = src;

    state = 0;
    tarindex = 0;
	if (rnDstLen) *rnDstLen = 0;
	if (rnErrPos) *rnErrPos = 0;

    while ((ch = *src++) != '\0')
	{
        if (isspace(ch))    /* Skip whitespace anywhere. */
            continue;

        if (ch == Pad64)
            break;

        pos = strchr(Base64, ch);
        if (pos == 0)       /* A non-base64 character. */
		{
			//if (abAllowErrors) {
			//	printf("Error: Invalid character '%c' at position %i\n", ch, src-src_save+1);
			//	return (tarindex);
			//}
			if (rnDstLen) *rnDstLen = tarindex;
			if (rnErrPos) *rnErrPos = src-src_save;
            return (-1);
		}

        switch (state) {
        case 0:
            if (target) {
                if ((int)tarindex >= targsize) {
					//if (abAllowErrors) {
					//	printf("Error: Buffer too small (<%i)\n", tarindex+1);
					//	return (tarindex);
					//}
					if (rnDstLen) *rnDstLen = tarindex;
					if (rnErrPos) *rnErrPos = src-src_save;
                    return (-1);
				}
                target[tarindex] = (pos - Base64) << 2;
            }
            state = 1;
            break;
        case 1:
            if (target) {
                if ((int)tarindex + 1 >= targsize)
				{
					//if (abAllowErrors) {
					//	printf("Error: Buffer too small (<%i)\n", tarindex+2);
					//	return (tarindex);
					//}
					if (rnDstLen) *rnDstLen = tarindex;
					if (rnErrPos) *rnErrPos = src-src_save;
                    return (-1);
				}
                target[tarindex]   |=  (pos - Base64) >> 4;
                target[tarindex+1]  = ((pos - Base64) & 0x0f)
                            << 4 ;
            }
            tarindex++;
            state = 2;
            break;
        case 2:
            if (target) {
                if ((int)tarindex + 1 >= targsize)
				{
					//if (abAllowErrors) {
					//	printf("Error: Buffer too small (<%i)\n", tarindex+2);
					//	return (tarindex);
					//}
					if (rnDstLen) *rnDstLen = tarindex;
					if (rnErrPos) *rnErrPos = src-src_save;
                    return (-1);
				}
                target[tarindex]   |=  (pos - Base64) >> 2;
                target[tarindex+1]  = ((pos - Base64) & 0x03)
                            << 6;
            }
            tarindex++;
            state = 3;
            break;
        case 3:
            if (target) {
                if ((int)tarindex >= targsize) {
					//if (abAllowErrors) {
					//	printf("Error: Buffer too small (<%i)\n", tarindex+1);
					//	return (tarindex);
					//}
					if (rnDstLen) *rnDstLen = tarindex;
					if (rnErrPos) *rnErrPos = src-src_save;
                    return (-1);
				}
                target[tarindex] |= (pos - Base64);
            }
            tarindex++;
            state = 0;
            break;
        default:
			//if (abAllowErrors) {
			//	printf("Error: Invalid state (<%i)\n", state);
			//	return (tarindex);
			//}
			if (rnDstLen) *rnDstLen = tarindex;
			if (rnErrPos) *rnErrPos = src-src_save;
            return (-1);
        }
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == Pad64) {      /* We got a pad char. */
        ch = *src++;        /* Skip it, get next. */
        switch (state) {
        case 0:     /* Invalid = in first position */
        case 1:     /* Invalid = in second position */
			//if (abAllowErrors) {
			//	printf("Error: PAD invalid - in second position\n");
			//	return (tarindex);
			//}
			if (rnDstLen) *rnDstLen = tarindex;
			if (rnErrPos) *rnErrPos = src-src_save;
            return (-1);

        case 2:     /* Valid, means one byte of info */
            /* Skip any number of spaces. */
            for ((void)NULL; ch != '\0'; ch = *src++)
                if (!isspace(ch))
                    break;
            /* Make sure there is another trailing = sign. */
			if (ch != Pad64) {
				//if (abAllowErrors) {
				//	printf("Error: Second PAD not found\n");
				//	return (tarindex);
				//}
				if (rnDstLen) *rnDstLen = tarindex;
				if (rnErrPos) *rnErrPos = src-src_save;
                return (-1);
			}
            ch = *src++;        /* Skip the = */
            /* Fall through to "single trailing =" case. */
            /* FALLTHROUGH */

        case 3:     /* Valid, means two bytes of info */
            /*
             * We know this char is an =.  Is there anything but
             * whitespace after it?
             */
            for ((void)NULL; ch != '\0'; ch = *src++)
                if (!isspace(ch)) {
					//if (abAllowErrors) {
					//	printf("Error: PAD invalid - state==3\n");
					//	return (tarindex);
					//}
					if (rnDstLen) *rnDstLen = tarindex;
					if (rnErrPos) *rnErrPos = src-src_save;
                    return (-1);
				}

            /*
             * Now make sure for cases 2 and 3 that the "extra"
             * bits that slopped past the last full byte were
             * zeros.  If we don't check them, they become a
             * subliminal channel.
             */
			if (target && target[tarindex] != 0) {
				//if (abAllowErrors) {
				//	printf("Error: PAD invalid - target && target[tarindex] != 0\n");
				//	return (tarindex);
				//}
				if (rnDstLen) *rnDstLen = tarindex;
				if (rnErrPos) *rnErrPos = src-src_save;
                return (-1);
			}
        }
    } else {
        /*
         * We ended by seeing the end of the string.  Make sure we
         * have no partial bytes lying around.
         */
        if (state != 0) {
			//if (abAllowErrors) {
			//	printf("Error: PAD not found, state!=0\n");
			//	return (tarindex);
			//}
			if (rnDstLen) *rnDstLen = tarindex;
			if (rnErrPos) *rnErrPos = src-src_save;
            return (-1);
		}
    }

	if (rnDstLen) *rnDstLen = tarindex;
    return (tarindex);
}

//static const char UUE[] =
//    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

int uue_decode(char const * src, unsigned char * target, int targsize)
{
    int tarindex, state, ch, index;
    const char *pos, *ln_st, *ln_en;
	int nLineLen;
	unsigned char temp[3];

    state = 0;
    tarindex = 0;
	if (target)
		*target = 0;

	// Сначала должен быть заголовок "begin 644 <name>"
	if (memcmp(src, "begin ", 6) != 0)
	{
		// Ошибка, нет заголовка!
		return (-1);
	}
	// Пропустить строку с заголовком
	pos = strpbrk(src, "\r\n");
	if (!pos)
	{
		// Ошибка, строка с заголовком ЕДИНСТВЕННАЯ в переданных данных
		return (-1);
	}
	// Пропустить перевод строки
	src = pos+1;
	while (*src == '\r' || *src == '\n')
		src++;

	// Теперь можно разбирать данные
    while ((ch = (*src)) != 0)
    {
		ln_st = src;
		ln_en = strpbrk(ln_st, "\r\n");
		if (!ln_en)
		{
			// Ошибка в формате!
			return (-1);
		}

		// Это может быть последняя строка
		if (ch == '`' || ch == ' ')
		{
			if ((ln_en - ln_st) == 1)
			{
				break; // Конец файла
			}
			else
			{
				// Ошибка в формате!
				return (-1);
			}
		}
		nLineLen = (int)(ch & 0x7F) - 32;
		if (nLineLen < 1 || nLineLen > 45)
		{
			// Ошибка в формате!
			return (-1);
		}


		// Поехали декодировать поток (текущую строку)
		while (nLineLen > 0 && *src != '\0')
		{
			if ((src + 3) >= ln_en)
			{
				// Ошибка в формате!
				return (-1);
			}

			for (state = 0; state < 4; state++)
			{
				ch = *(++src);
				if ((UINT)ch < 32)
				{
					// Non UUE character
					return (-1);
				}

				index = (int)(0x3F & ((ch & 0x7F) - 32));
				//if (index < 0 || index >= 64)
				//{
				//	// Non UUE character
				//	return (-1);
				//}

				switch (state) {
				case 0:
					temp[0] = ((BYTE)index) << 2;
					break;
				case 1:
					temp[0]   |=  ((BYTE)index) >> 4;
					temp[1]  = (((BYTE)index) & 0x0f) << 4 ;
					break;
				case 2:
					temp[1]   |=  ((BYTE)index) >> 2;
					temp[2]  = (((BYTE)index) & 0x03) << 6;
					break;
				case 3:
					temp[2] |= ((BYTE)index);
					break;
				default:
					return (-1);
				}
			}

			// target[tarindex]
			// Скопировать 3 байта в результат
			if (target)
			{
				if (((int)tarindex + min(3,nLineLen)) >= targsize)
                    return (-1); // превышение размера буфера
			}

			for (state = 0; (nLineLen > 0) && (state < 3); state++, nLineLen--)
			{
				if (target) target[tarindex] = temp[state];
				tarindex++;
			}
		}

		src = ln_en+1;
		if (*src == '\r' || *src == '\n')
			src++;
    }

	// Padding в UUE отсутствует

    return (tarindex);
}

// Для (abQ==TRUE) символ '_' заменяется на пробел!
int qp_decode(char const * src, unsigned char * target, DWORD* Flags, BOOL abQ)
{
	int nLen = 0;
	char ch, ch1, ch2;
	unsigned char uch;
	BOOL lbZeroFound = FALSE;
	BOOL lbFailFound = FALSE;
	unsigned char *dst = target;

	while ((ch = *src) != 0)
	{
		if (ch != '=')
		{
			if (abQ && ch == '_')
				*(dst++) = ' ';
			else
				*(dst++) = ch;
			src++;
			continue;
		}

		uch = 0; // сброс
		// Декодируем. Ожидается два HEX символа, или \r\n
		ch1 = src[1];
		switch (ch1)
		{
		case '\r': case '\n':
			// перевод строки после '=' - игнорируется
			ch2 = src[2];
			if (ch2 == '\r' || ch2 == '\n')
				src += 3;
			else
				src += 2;
			continue; // игнорируем
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			uch = ((BYTE)(ch1 - '0')) << 4;
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
			uch = ((BYTE)(ch1 - 'a' + 10)) << 4;
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
			uch = ((BYTE)(ch1 - 'A' + 10)) << 4;
			break;
		default:
			// Ошибка!
			lbFailFound = TRUE;
			*(dst++) = ch;
			src++;
			continue;
		}
		// А теперь младшие 4 бита
		ch2 = src[2];
		switch (ch2)
		{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			uch |= ((BYTE)(ch2 - '0'));
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
			uch |= ((BYTE)(ch2 - 'a' + 10));
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
			uch |= ((BYTE)(ch2 - 'A' + 10));
			break;
		default:
			// Ошибка!
			lbFailFound = TRUE;
			*(dst++) = ch;
			src++;
			continue;
		}
		*(dst++) = uch;
		src += 3;
	}

	// Done
	*dst = 0;

	if (Flags)
		*Flags = (lbZeroFound ? 1 : 0) | (lbFailFound ? 2 : 0);

	nLen = (dst - target);
	return nLen;
}





// Теперь - собственно плагин

struct SourceBinary
{
	char* pData;
	DWORD  nDataSize;
	TCHAR* pszFilePathName;
	LPCTSTR pszFile;
	PluginPanelItem Item;
	LPBYTE pDecode;
	DWORD  nDecodeSize;
	TCHAR szBinFileName[MAX_PATH];
};


PluginStartupInfo psi;
FarStandardFunctions fsf;



BOOL IsArchive(const unsigned char *Data, int DataSize)
{
  static char chAllowed[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\r\n=";
  if (!Data || !*Data || !DataSize)
  	return FALSE;
  for (int i = DataSize; --i; Data++)
  {
  	if (!strchr(chAllowed, *Data))
  	  return FALSE;
  }
  return TRUE;
}

#if FAR_UNICODE>=2462

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->MinFarVersion = /*FARMANAGERVERSION*/
		MAKEFARVERSION(
			FARMANAGERVERSION_MAJOR,
			FARMANAGERVERSION_MINOR,
			FARMANAGERVERSION_REVISION,
			FARMANAGERVERSION_BUILD,
			FARMANAGERVERSION_STAGE);
	
	// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);
	
	Info->Guid = guid_PluginGuid;
	Info->Title = L"Base64 decoder";
	Info->Description = L"Base64 decoder for panels and editor";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

#else

int WINAPI GetMinFarVersionW(void)
{
	return FARMANAGERVERSION;
}

#endif

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *xInfo)
{
	psi = *xInfo;
	fsf = *xInfo->FSF;
	psi.FSF = &fsf;
}

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
	#if FAR_UNICODE>=1906
	//_ASSERTE(pi->StructSize >= sizeof(struct PluginInfo));
	#else
	pi->StructSize = sizeof(struct PluginInfo);
	pi->Reserved = 0;
	#endif

	pi->Flags = PF_DISABLEPANELS|PF_EDITOR/*|PF_VIEWER*/;

	static TCHAR* szMenu[1] = {_T("base64 decoder")};

	pi->CommandPrefix = NULL; //_T("base64");

	#if FAR_UNICODE>=1906
		pi->DiskMenu.Guids = NULL;
		pi->DiskMenu.Strings = NULL;
		pi->DiskMenu.Count = 0;
	#else
		pi->DiskMenuStrings = NULL;
		pi->DiskMenuNumbers = NULL;
		pi->DiskMenuStringsNumber = 0;
	#endif

	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_PluginMenu;
		pi->PluginMenu.Strings = szMenu;
		pi->PluginMenu.Count = 1;
	#else
		pi->PluginMenuStrings = gbUseMenu ? szMenu : NULL;
		pi->PluginMenuStringsNumber = gbUseMenu ? 1 : 0;
	#endif
	
	#if FAR_UNICODE>=1906
		pi->PluginConfig.Guids = NULL;
		pi->PluginConfig.Strings = NULL;
		pi->PluginConfig.Count = 0;
	#else
		pi->PluginConfigStrings = NULL; //szMenu;
		pi->PluginConfigStringsNumber = 0; //1;
	#endif
}

HANDLE WINAPI OpenFilePluginW(const TCHAR *Name,const unsigned char *Data,int DataSize,int OpMode)
{
	if (!IsArchive(Data, DataSize))
		return INVALID_HANDLE_VALUE;


	
	BOOL lbRc = TRUE;
	DWORD dwRead;
	WIN32_FIND_DATA fnd = {0};
	SourceBinary* p = NULL;
	HANDLE hFile = NULL;
	DWORD nErr = 0;
	
	
	p = (SourceBinary*)calloc(sizeof(SourceBinary),1);
	
	// Развернуть имя!
	#ifdef _UNICODE
	int nLen = (int)fsf.ConvertPath(CPM_FULL, Name, NULL, 0);
	if (nLen > 0) {
		//TODO: UNC
		p->pszFilePathName = (TCHAR*)calloc((nLen+1),sizeof(TCHAR));
		fsf.ConvertPath(CPM_FULL, Name, p->pszFilePathName, nLen);
		p->pszFile = wcsrchr(p->pszFilePathName, L'\\');
		if (!p->pszFile) p->pszFile = p->pszFilePathName; else p->pszFile++;
	}
	#else
	p->pszFilePathName = (TCHAR*)calloc(MAX_PATH*3,sizeof(TCHAR));
	dwRead = GetFullPathName(Name, MAX_PATH*3, p->pszFilePathName, &p->pszFile);
	if (!dwRead || dwRead > MAX_PATH*3)
		lbRc = FALSE;
	#endif
	
	
	
	if (lbRc)
	{
		hFile = FindFirstFile(p->pszFilePathName, &fnd);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			lbRc = FALSE;
		}
		else
		{
			FindClose(hFile); hFile = NULL;
			if (fnd.nFileSizeHigh)
				lbRc = FALSE;
		}
	}
		
	if (lbRc)
	{
		hFile = CreateFile(p->pszFilePathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			hFile = CreateFile(p->pszFilePathName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				nErr = GetLastError();
				lbRc = FALSE;
			}
		}
		
		if (lbRc)
		{
			p->pData = (char*)malloc(fnd.nFileSizeLow+1);
			p->nDataSize = fnd.nFileSizeLow;

			lbRc = ReadFile(hFile, p->pData, fnd.nFileSizeLow, &dwRead, NULL);
			if (!lbRc)
				nErr = GetLastError();
			else
				p->pData[dwRead] = 0;
		}
	}
	
	if (hFile && hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = NULL;
	}
	
	if (lbRc)
	{
		p->pDecode = (LPBYTE)calloc(p->nDataSize+1,1);
		int nTargetLen = 0, nErrPos = 0, nRc = 0;
		
		nRc = b64_decode(p->pData, p->pDecode, p->nDataSize+1, &nTargetLen, &nErrPos);

		if (nRc < 0)
		{
			lbRc = FALSE;
			wchar_t szErr[512];
			wsprintf(szErr, L"Universal decoder\nBase64 encoded data contains errors!\nSource size: %i, Decoded size: %i\nFailPos(1-based): %i\nContinue\nCancel",
				lstrlenA(p->pData), nTargetLen, nErrPos);
			// Пользователь может согласиться на просмотр успешно декодированной части
			if (psi.Message(_PluginNumber(guid_PluginGuid), FMSG_WARNING|FMSG_ALLINONE,
					NULL, (const wchar_t * const *)szErr, 0, 2) == 0)
			{
				lbRc = TRUE; nRc = nTargetLen;
			}
		}

		if (nRc >= 0)
		{
			lbRc = TRUE;
			p->nDecodeSize = nTargetLen;
			free(p->pData); p->pData = NULL;
		}
	}
	
	if (!lbRc)
	{
		if (p->pszFilePathName) free(p->pszFilePathName);
		if (p->pData) free(p->pData);
		if (p->pDecode) free(p->pDecode);
		free(p);
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		PanelItemAttributes(p->Item) = fnd.dwFileAttributes;
		PanelItemCreation(p->Item) = fnd.ftCreationTime;
		PanelItemAccess(p->Item) = fnd.ftLastAccessTime;
		PanelItemWrite(p->Item) = fnd.ftLastWriteTime;
		#ifdef _UNICODE
		PanelItemFileSize(p->Item) = p->nDecodeSize;
		PanelItemPackSize(p->Item) = fnd.nFileSizeLow;
		#else
		p->Item.FindData.nFileSizeLow = p->nDecodeSize;
		p->Item.FindData.nFileSizeHigh = 0;
		p->Item.PackSize = fnd.nFileSizeLow;
		p->Item.PackSizeHigh = 0;
		#endif
		
		lstrcpyn(p->szBinFileName, p->pszFile, MAX_PATH-6);
		//TODO: Попытаться по первым байтам поймать расширение файла
		lstrcat(p->szBinFileName, _T(".bin"));
		
		#ifdef _UNICODE
		PanelItemFileNamePtr(p->Item) = p->szBinFileName;
		PanelItemAltNamePtr(p->Item) = NULL;
		#else
		lstrcpy(p->Item.FindData.cFileName, p->szBinFileName);
		p->Item.FindData.cAlternateFileName[0] = 0;
		p->Item.FindData.dwReserved0 = 0;
		p->Item.FindData.dwReserved1 = 0;
		#endif
		
		p->Item.Description = NULL;
		p->Item.Owner = NULL;
		
		// Обнулить неиспользуемые поля в pi
		p->Item.Flags = 0;
		p->Item.NumberOfLinks = 0;
		p->Item.CustomColumnData = NULL;
		p->Item.CustomColumnNumber = 0;
		p->Item.CRC32 = 0;
		p->Item.Reserved[0] = p->Item.Reserved[1] = 0;
	}
	
	return (HANDLE)p;
}

void WINAPI ClosePluginW(
	#if FAR_UNICODE>2041
	const struct ClosePanelInfo *Info
	#else
	HANDLE hPlugin
	#endif
	)
{
	#if FAR_UNICODE>2041
	HANDLE hPlugin = Info->hPanel;
	#endif

	SourceBinary* p = (SourceBinary*)hPlugin;
	if (p->pData) free(p->pData);
	if (p->pDecode) free(p->pDecode);
	if (p->pszFilePathName) free(p->pszFilePathName);
	free(p);
}

void WINAPI GetOpenPluginInfoW(
		#if FAR_UNICODE>2041
			struct OpenPanelInfo *Info
		#else
			HANDLE hPlugin, struct OpenPluginInfo *Info
		#endif
		)
{
	#if FAR_UNICODE>2041
	HANDLE hPlugin = Info->hPanel;
	#endif

	SourceBinary* p = (SourceBinary*)hPlugin;

	Info->CurDir = _T("");
	Info->PanelTitle = p->pszFile;
	Info->HostFile = p->pszFilePathName;
	Info->Flags = OPIF_ADDDOTS;
	Info->Format = _T("base64");
	Info->ShortcutData = NULL;
	//Info->StartSortMode = 0;
}

FARINT WINAPI GetFindDataW(
	#if FAR_UNICODE>2041
	struct GetFindDataInfo *Info
	#else
	HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode 
	#endif
	)
{
	#if FAR_UNICODE>2041
	SourceBinary* p = (SourceBinary*)Info->hPanel;
	Info->PanelItem = &p->Item;
	Info->ItemsNumber = 1;
	#else
	SourceBinary* p = (SourceBinary*)hPlugin;
	*pPanelItem = &p->Item;
	*pItemsNumber = 1;
	#endif
	return TRUE;
}

/*void WINAPI FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
}*/

FARINT WINAPI GetFilesW(
	#if FAR_UNICODE>=2041
		struct GetFilesInfo *Info
	#else
		HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move,
		#ifdef _UNICODE
			const wchar_t **DestPath, int OpMode
		#else
			LPSTR DestPath, int OpMode
		#endif
	#endif
	)
{
	#if FAR_UNICODE>=2041
	HANDLE hPlugin = Info->hPanel;
	PluginPanelItem* PanelItem = Info->PanelItem;
	int ItemsNumber = Info->ItemsNumber;
	int Move = Info->Move;
	const wchar_t **DestPath = &Info->DestPath;
	OPERATION_MODES OpMode = Info->OpMode;
	#endif

	int iRc = 1;
	LPCTSTR pszDir;
	#ifdef _UNICODE
	pszDir = *DestPath;
	#else
	pszDir = DestPath;
	#endif
	
	SourceBinary* p = (SourceBinary*)hPlugin;
	
	int nDirLen = lstrlen(pszDir);
	int nLen = nDirLen + lstrlen(FILENAMEPTR(*PanelItem)) + 16;
	
	TCHAR* pszTarget = (TCHAR*)calloc(nLen,sizeof(TCHAR));
	//TODO: UNC
	lstrcat(pszTarget, pszDir);
	if (pszDir[nDirLen-1] != _T('\\')) lstrcat(pszTarget, _T("\\"));
	lstrcat(pszTarget, FILENAMEPTR(*PanelItem));

	
	HANDLE hDst = CreateFile(pszTarget, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDst == INVALID_HANDLE_VALUE)
	{
		iRc = 0;
	}

	if (iRc == 1)
	{
		DWORD dwWritten = 0;
		if (!WriteFile(hDst, p->pDecode, p->nDecodeSize, &dwWritten, NULL))
		{			
			iRc = 0;
		}
	}
	
	if (hDst && hDst != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDst); hDst = NULL;
	}
	
	return iRc;
}

#if FAR_UNICODE>=2462
HANDLE WINAPI AnalyseW(const struct AnalyseInfo *Info)
{
	if (!IsArchive((const unsigned char*)Info->Buffer, Info->BufferSize))
		return FALSE;

	return (HANDLE)TRUE;
}

//void WINAPI CloseAnalyseW(const struct CloseAnalyseInfo *Info)
//{
//	CPluginAnalyse* pAnalyse = (CPluginAnalyse*)Info->Handle;
//	free(pAnalyse);
//}
#endif

HANDLE WINAPI OpenPluginW(
	#if FAR_UNICODE>=2041
	const struct OpenInfo *Info
	#else
	int OpenFrom, INT_PTR Item
	#endif
	)
{
	#if FAR_UNICODE>=2041
	OPENFROM OpenFrom = Info->OpenFrom;
	INT_PTR Item = Info->Data;
	#endif

	#if defined(_UNICODE) && (FARMANAGERVERSION_BUILD>=2572)
	const HANDLE PanelStop = PANEL_STOP;
	const HANDLE PanelNone = NULL;
	#else
	const HANDLE PanelStop = (HANDLE)-2;
	const HANDLE PanelNone = INVALID_HANDLE_VALUE;
	#endif

	#if (FARMANAGERVERSION_BUILD>=2572)
	if (OpenFrom == OPEN_ANALYSE)
	{
		AnalyseInfo* pAnalyse = ((OpenAnalyseInfo*)Info->Data)->Info;
		return OpenFilePluginW(pAnalyse->FileName, (LPBYTE)pAnalyse->Buffer, pAnalyse->BufferSize, 0);
	}
	#endif

	if (OpenFrom != OPEN_EDITOR)
		return PanelNone;

	if (OpenFrom == OPEN_EDITOR)
	{
		EditorInfo ei = {};
		#if FAR_UNICODE>=3000
		ei.StructSize = sizeof(ei);
		#endif
		EditCtrl(ECTL_GETINFO, &ei);
		if (ei.BlockType != BTYPE_STREAM)
		{
			psi.Message(_PluginNumber(guid_PluginGuid), FMSG_WARNING|FMSG_ALLINONE|FMSG_MB_OK,
				NULL, (const wchar_t * const *)L"Universal decoder\nThere is no selection to decode!", 0, 0);
			return PanelNone;
		}

		//FarMenuItem pItems[] = {
		//	{_T("&1 Tabulate right"),0,0,0},
		//	{_T("&2 Tabulate left"),0,0,0},
		//	{_T("&3 Comment"),0,0,0},
		//	{_T("&4 UnComment"),0,0,0}
		//};
		//	//int nMode = lmMenu.DoModal();
		//	nMode = psi.Menu ( psi.ModuleNumber,
		//	-1, -1, 0, FMENU_WRAPMODE,
		//	_T(szMsgBlockEditorPlugin),NULL,NULL,
		//	0, NULL, pItems, 4 );

		EditorGetString egs = {};
		#if FAR_UNICODE>=3000
		egs.StructSize = sizeof(egs);
		#endif
		
		//EditorSetString ess;
		//EditorSelect es;
		
		int nStartLine = ei.BlockStartLine;
		int nEndLine = nStartLine;
		int Y1 = nStartLine, Y2 = -1;
		int X1 = 0, X2 = -1;
		bool lbCurLineShifted = false;
		int nInsertSlash = -1;
		int nTextLen = 0;

		for (egs.StringNumber=nStartLine;
			egs.StringNumber<ei.TotalLines;
			egs.StringNumber++ )
		{
			EditCtrl(ECTL_GETSTRING,&egs);

			//if (egs.StringNumber==nStartLine) {
			//	X1 = egs.SelStart;
			//}

			//if (egs.SelEnd!=-1 && X2==-1) {
			//	X2 = egs.SelEnd;
			//	Y2 = nEndLine;
			//}

			nTextLen += egs.StringLength + 1;

			if (egs.SelStart == -1)
			{
				nEndLine = egs.StringNumber-1;
				break;
			}
		}
		//if (Y2==-1)
		Y2 = nEndLine;
		if (X2==0) Y2++;

		nTextLen += 2; // чтобы точно ASCIIZ получился

		char* lsFull = (char*)malloc(nTextLen);
		*lsFull = 0;
		char* lsText = lsFull;
		
		for (egs.StringNumber=nStartLine;
			egs.StringNumber<=nEndLine;
			egs.StringNumber++ )
		{
			EditCtrl(ECTL_GETSTRING,&egs);

			int nStart = egs.SelStart;
			int nEnd = (egs.SelEnd == -1) ? egs.StringLength : egs.SelEnd;
			int nLen = (nEnd - nStart);

			if (nLen > 0)
			{
				WideCharToMultiByte(CP_ACP, 0, egs.StringText+nStart, nLen, lsText, nTextLen, 0,0);

				lsText += nLen;
				//*(lsText++) = '\n'; 
				*lsText = 0;
			}
		}

		int targsize = 0;
		lsText = NULL;
		int nDecodeLen = 0, nErrPos = 0, nRc = 0;

		FarMenuItem/*Ex*/ items[] = {
			{0,  L"Base64 decode"},
			{0,  L"Quoted-printable decode"},
			{MIF_SEPARATOR},
			{0,  L"Base64 encode"}
		};
		int nCount = sizeof(items)/sizeof(items[0]);

		int nDecoderType = -1;

		char* pszTextStart = lsFull;
		char szCodePage[32] = {0};
		char szEnc = 0;
		int nLen = lstrlenA(lsFull);
		if (nLen > 10 && lsFull[0] == '=' && lsFull[1] == '?' && lsFull[nLen-1] == '=' && lsFull[nLen-2] == '?')
		{
			char* pszEnc = strchr(lsFull+2, '?');
			int n = pszEnc - lsFull;
			if (n > 3 && n < (nLen-4) && n < 32)
			{
				pszEnc++;
				if ((*pszEnc == 'B' || *pszEnc == 'Q') && pszEnc[1] == '?')
				{
					szEnc = *pszEnc;
					pszTextStart = pszEnc + 2;
					lsFull[nLen-2] = 0;
					memmove(szCodePage, lsFull+2, n-2);
					szCodePage[n-2] = 0;
				}
			}
		}
		
		if (szEnc == 'B')
			nDecoderType = 0;
		else if (szEnc == 'Q')
			nDecoderType = 1;
		else
			nDecoderType = psi.Menu(_PluginNumber(guid_PluginGuid), -1,-1, 0, 
				/*FMENU_USEEXT|*/FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
				L"Choose decoder",
				NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);
		
		if (nDecoderType >= 0)
		{
			if (nDecoderType == 0)
			{
				targsize = nTextLen+2;
				lsText = (char*)malloc(targsize);
				nRc = b64_decode(pszTextStart, (unsigned char *)lsText, targsize, &nDecodeLen, &nErrPos);
			}
			else if (nDecoderType == 1)
			{
				targsize = nTextLen+2;
				lsText = (char*)malloc(targsize);
				DWORD Flags = 0;
				nDecodeLen = qp_decode(pszTextStart, (unsigned char *)lsText, &Flags, (szEnc=='Q'));
				if (Flags & 2)
					nRc = -1;
				else
					nRc = nDecodeLen;
			}
			else if (nDecoderType = 3)
			{
				targsize = nTextLen*2+4;
				lsText = (char*)malloc(targsize);
				nRc = b64_encode((const unsigned char*)lsFull, lstrlenA(lsFull), lsText, targsize);
				nDecodeLen = nRc;
			}
				

			if (nRc < 0)
			{
				wchar_t szErr[512];
				wsprintf(szErr, L"Universal decoder\nBase64 encoded data contains errors!\nSource size: %i, Decoded size: %i\nFailPos(1-based): %i\nContinue\nCancel",
					lstrlenA(lsFull), nDecodeLen, nErrPos);
				// Пользователь может согласиться на просмотр успешно декодированной части
				if (psi.Message(_PluginNumber(guid_PluginGuid), FMSG_WARNING|FMSG_ALLINONE,
						NULL, (const wchar_t * const *)szErr, 0, 2) == 0)
				{
					nRc = nDecodeLen;
				}
			}

			if (nRc >= 0)
			{
				if (EditCtrl(ECTL_DELETEBLOCK,NULL))
				{
					wchar_t* pwszText = (wchar_t*)malloc((nDecodeLen+1)*2);
					bool lbReady = false;
					if (szEnc)
					{
						int nCP = -1;
						if (!lstrcmpiA(szCodePage, "cp866"))
							nCP = 866;
						//else if (!lstrcmpiA(szCodePage, "utf-16"))
						//	nCP = 1200;
						//else if (!lstrcmpiA(szCodePage, "unicodefffe"))
						//	nCP = 1201;
						else if (!lstrcmpiA(szCodePage, "windows-1251"))
							nCP = 1251;
						else if (!lstrcmpiA(szCodePage, "koi8-u"))
							nCP = 21866;
						else if (!lstrcmpiA(szCodePage, "utf-7"))
							nCP = 65000;
						else if (!lstrcmpiA(szCodePage, "utf-8"))
							nCP = 65001;
						else if (!lstrcmpiA(szCodePage, "koi8-r"))
							nCP = 20866;

						if (nCP != -1)
						{
							//if (nCP != 1200 && nCP != 1201 && nCP != 65000 && nCP != 65001)
							//{
							nDecodeLen = MultiByteToWideChar(nCP, 0, lsText, nDecodeLen, pwszText, nDecodeLen);
							lbReady = true;
							//}
							//else
							//{
							//	char* pszMB = (char*)calloc(nDecodeLen+2,1);
							//	MultiByteToWideChar(nCP, 0, lsText, nDecodeLen, pwszText, nDecodeLen);
							//}
						}
					}
					if (!lbReady)
						nDecodeLen = MultiByteToWideChar(CP_ACP, 0, lsText, nDecodeLen, pwszText, nDecodeLen);
					pwszText[nDecodeLen] = 0;
					EditCtrl(ECTL_INSERTTEXT, pwszText);
					free(pwszText);
				}
			}
		}

		if (lsText) free(lsText);
		free(lsFull);
	}

	return PanelNone;
}
