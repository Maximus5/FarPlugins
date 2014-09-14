/*
CachedWrite.cpp

����������� ������ � ����
*/
/*
Copyright (c) 2009 Far Group
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

#include "headers.hpp"
#pragma hdrstop

#include "CachedWrite.hpp"

CachedWrite::CachedWrite(File& file):
	Buffer(reinterpret_cast<LPBYTE>(xf_malloc(BufferSize))),
	file(file),
	FreeSize(BufferSize),
	Flushed(false)
{
}

CachedWrite::~CachedWrite()
{
	Flush();

	if (Buffer)
	{
		xf_free(Buffer);
	}
}

bool CachedWrite::Write(LPCVOID Data,size_t DataSize)
{
	bool Result=false;
	bool SuccessFlush=true;

	if (Buffer)
	{
		if (DataSize>FreeSize)
		{
			SuccessFlush=Flush();
		}

		if(SuccessFlush)
		{
			if (DataSize>FreeSize)
			{
				DWORD WrittenSize=0;

				if (file.Write(Data,static_cast<DWORD>(DataSize),&WrittenSize,nullptr) && DataSize==WrittenSize)
				{
					Result=true;
				}
			}
			else
			{
				memcpy(&Buffer[BufferSize-FreeSize],Data,DataSize);
				FreeSize-=DataSize;
				Flushed=false;
				Result=true;
			}
		}
	}
	return Result;
}

bool CachedWrite::Flush()
{
	if (Buffer)
	{
		if (!Flushed)
		{
			DWORD WrittenSize=0;

			if (file.Write(Buffer,static_cast<DWORD>(BufferSize-FreeSize),&WrittenSize,nullptr) && BufferSize-FreeSize==WrittenSize)
			{
				Flushed=true;
				FreeSize=BufferSize;
			}
		}
	}

	return Flushed;
}
