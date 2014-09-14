#pragma once

/*
message.hpp

����� MessageBox
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

//#define MAX_WIDTH_MESSAGE static_cast<DWORD>(ScrX-13)

#define ADDSPACEFORPSTRFORMESSAGE 16

//enum
//{
//	MSG_WARNING        =0x00000001,
//	MSG_ERRORTYPE      =0x00000002,
//	MSG_KEEPBACKGROUND =0x00000004,
//	MSG_LEFTALIGN      =0x00000010,
//	MSG_KILLSAVESCREEN =0x00000020,
//};

int Message(DWORD Flags,int Buttons,const wchar_t *Title,const wchar_t *Str1,
            const wchar_t *Str2=nullptr,const wchar_t *Str3=nullptr,const wchar_t *Str4=nullptr);
int Message(DWORD Flags,int Buttons,const wchar_t *Title,const wchar_t *Str1,
            const wchar_t *Str2,const wchar_t *Str3,const wchar_t *Str4,
            const wchar_t *Str5,const wchar_t *Str6=nullptr,const wchar_t *Str7=nullptr);
int Message(DWORD Flags,int Buttons,const wchar_t *Title,const wchar_t *Str1,
            const wchar_t *Str2,const wchar_t *Str3,const wchar_t *Str4,
            const wchar_t *Str5,const wchar_t *Str6,const wchar_t *Str7,
            const wchar_t *Str8,const wchar_t *Str9=nullptr,const wchar_t *Str10=nullptr);
int Message(DWORD Flags,int Buttons,const wchar_t *Title,const wchar_t *Str1,
            const wchar_t *Str2,const wchar_t *Str3,const wchar_t *Str4,
            const wchar_t *Str5,const wchar_t *Str6,const wchar_t *Str7,
            const wchar_t *Str8,const wchar_t *Str9,const wchar_t *Str10,
            const wchar_t *Str11,const wchar_t *Str12=nullptr,const wchar_t *Str13=nullptr,
            const wchar_t *Str14=nullptr);

//int Message(DWORD Flags,int Buttons,const wchar_t *Title,const wchar_t * const *Items,int ItemsNumber);

//void SetMessageHelp(const wchar_t *Topic);
//void GetMessagePosition(int &X1,int &Y1,int &X2,int &Y2);

///* $ 12.03.2002 VVM
//  ����� ������� - ������������ ��������� �������� ��������.
//  ������� ������.
//  ����������:
//   FALSE - ���������� ��������
//   TRUE  - �������� ��������
//*/
//int AbortMessage();
//
//bool GetErrorString(string &strErrStr);
