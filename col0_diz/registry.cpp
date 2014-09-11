/*
registry.cpp

������ � registry
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

#include "headers.hpp"
#pragma hdrstop

#include "registry.hpp"
//#include "array.hpp"
#include "config.hpp"
#include "pathmix.hpp"

static LONG CloseRegKey(HKEY hKey);

//void DeleteFullKeyTree(const wchar_t *KeyName);
//static void DeleteKeyTreePart(const wchar_t *KeyName);
//
//static int DeleteCount;

static HKEY hRegRootKey=HKEY_CURRENT_USER;
static HKEY hRegCurrentKey=nullptr;
static int RequestSameKey=FALSE;

//void SetRegRootKey(HKEY hRootKey)
//{
//	hRegRootKey=hRootKey;
//}
//
//
//void UseSameRegKey()
//{
//	CloseSameRegKey();
//	RequestSameKey=TRUE;
//}


void CloseSameRegKey()
{
	if (hRegCurrentKey)
	{
		RegCloseKey(hRegCurrentKey);
		hRegCurrentKey=nullptr;
	}

	RequestSameKey=FALSE;
}


//LONG SetRegKey(const wchar_t *Key,const wchar_t *ValueName,const wchar_t * const ValueData, int SizeData, DWORD Type)
//{
//	HKEY hKey;
//	LONG Ret=ERROR_SUCCESS;
//
//	if ((hKey=CreateRegKey(Key)) )
//		Ret=RegSetValueEx(hKey,ValueName,0,Type,(unsigned char *)ValueData,(int)SizeData);
//
//	CloseRegKey(hKey);
//	return Ret;
//}
//
//LONG SetRegKey(const wchar_t *Key,const wchar_t *ValueName,const wchar_t * const ValueData)
//{
//	HKEY hKey;
//	LONG Ret=ERROR_SUCCESS;
//
//	if ((hKey=CreateRegKey(Key)) )
//		Ret=RegSetValueEx(hKey,ValueName,0,REG_SZ,(unsigned char *)ValueData,(int)(StrLength(ValueData)+1)*sizeof(wchar_t));
//
//	CloseRegKey(hKey);
//	return Ret;
//}
//
//LONG SetRegKey(const wchar_t *Key,const wchar_t *ValueName,DWORD ValueData)
//{
//	HKEY hKey;
//	LONG Ret=ERROR_SUCCESS;
//
//	if ((hKey=CreateRegKey(Key)) )
//		Ret=RegSetValueEx(hKey,ValueName,0,REG_DWORD,(BYTE *)&ValueData,sizeof(ValueData));
//
//	CloseRegKey(hKey);
//	return Ret;
//}
//
//
//LONG SetRegKey64(const wchar_t *Key,const wchar_t *ValueName,unsigned __int64 ValueData)
//{
//	HKEY hKey;
//	LONG Ret=ERROR_SUCCESS;
//
//	if ((hKey=CreateRegKey(Key)) )
//		Ret=RegSetValueEx(hKey,ValueName,0,REG_QWORD,(BYTE *)&ValueData,sizeof(ValueData));
//
//	CloseRegKey(hKey);
//	return Ret;
//}
//
//LONG SetRegKey(const wchar_t *Key,const wchar_t *ValueName,const BYTE *ValueData,DWORD ValueSize)
//{
//	HKEY hKey;
//	LONG Ret=ERROR_SUCCESS;
//
//	if ((hKey=CreateRegKey(Key)) )
//		Ret=RegSetValueEx(hKey,ValueName,0,REG_BINARY,ValueData,ValueSize);
//
//	CloseRegKey(hKey);
//	return Ret;
//}
//
//
//
//int GetRegKeySize(const wchar_t *Key,const wchar_t *ValueName)
//{
//	HKEY hKey=OpenRegKey(Key);
//	DWORD QueryDataSize=GetRegKeySize(hKey,ValueName);
//	CloseRegKey(hKey);
//	return QueryDataSize;
//}
//
//
//int GetRegKeySize(HKEY hKey,const wchar_t *ValueName)
//{
//	if (hKey)
//	{
//		BYTE Buffer;
//		DWORD Type,QueryDataSize=sizeof(Buffer);
//		int ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,(unsigned char *)&Buffer,&QueryDataSize);
//
//		if (ExitCode==ERROR_SUCCESS || ExitCode == ERROR_MORE_DATA)
//			return QueryDataSize;
//	}
//
//	return 0;
//}


/* $ 22.02.2001 SVS
  ��� ��������� ������ (GetRegKey) ���������� �������� � ERROR_MORE_DATA
  ���� ����� �������� ����������� - ������� ������� ���� � ����� ������
*/

int GetRegKey(const wchar_t *Key,const wchar_t *ValueName,string &strValueData,const wchar_t *Default,DWORD *pType)
{
	int ExitCode=!ERROR_SUCCESS;
	HKEY hKey=OpenRegKey(Key);

	if (hKey) // ������� ���������!
	{
		DWORD Type,QueryDataSize=0;

		if ((ExitCode = RegQueryValueEx(
		                    hKey,
		                    ValueName,
		                    0,
		                    &Type,
		                    nullptr,
		                    &QueryDataSize
		                )) == ERROR_SUCCESS)
		{
			wchar_t *TempBuffer = strValueData.GetBuffer(QueryDataSize/sizeof(wchar_t)+1);  // ...�� ������� ������� ����
			ExitCode = RegQueryValueEx(hKey,ValueName,0,&Type,(unsigned char *)TempBuffer,&QueryDataSize);
			strValueData.ReleaseBuffer(QueryDataSize/sizeof(wchar_t));

			if (strValueData.GetLength() > 0 && !strValueData.At(strValueData.GetLength()-1))
				strValueData.SetLength(strValueData.GetLength()-1);
		}

		if (pType)
			*pType=Type;

		CloseRegKey(hKey);
	}

	if (ExitCode!=ERROR_SUCCESS)
	{
		strValueData = Default;
		return FALSE;
	}

	return TRUE;
}

/* SVS $ */

int GetRegKey(const wchar_t *Key,const wchar_t *ValueName,int &ValueData,DWORD Default)
{
	int ExitCode=!ERROR_SUCCESS;
	HKEY hKey=OpenRegKey(Key);

	if (hKey)
	{
		DWORD Type,Size=sizeof(ValueData);
		ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,(BYTE *)&ValueData,&Size);
		CloseRegKey(hKey);
	}

	if (ExitCode!=ERROR_SUCCESS)
	{
		ValueData=Default;
		return FALSE;
	}

	return TRUE;
}

//int GetRegKey(const wchar_t *Key,const wchar_t *ValueName,DWORD Default)
//{
//	int ValueData;
//	GetRegKey(Key,ValueName,ValueData,Default);
//	return(ValueData);
//}
//
//int GetRegKey64(const wchar_t *Key,const wchar_t *ValueName,__int64 &ValueData,unsigned __int64 Default)
//{
//	int ExitCode=!ERROR_SUCCESS;
//	HKEY hKey=OpenRegKey(Key);
//
//	if (hKey)
//	{
//		DWORD Type,Size=sizeof(ValueData);
//		ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,(BYTE *)&ValueData,&Size);
//		CloseRegKey(hKey);
//	}
//
//	if (ExitCode!=ERROR_SUCCESS)
//	{
//		ValueData=Default;
//		return FALSE;
//	}
//
//	return TRUE;
//}
//
//__int64 GetRegKey64(const wchar_t *Key,const wchar_t *ValueName,unsigned __int64 Default)
//{
//	__int64 ValueData;
//	GetRegKey64(Key,ValueName,ValueData,Default);
//	return(ValueData);
//}

int GetRegKey(const wchar_t *Key,const wchar_t *ValueName,BYTE *ValueData,const BYTE *Default,DWORD DataSize,DWORD *pType)
{
	int ExitCode=!ERROR_SUCCESS;
	HKEY hKey=OpenRegKey(Key);
	DWORD Required=DataSize;

	if (hKey)
	{
		DWORD Type;
		ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,ValueData,&Required);

		if (ExitCode == ERROR_MORE_DATA) // ���� ������ �� ����������...
		{
			char *TempBuffer=new char[Required+1]; // ...�� ������� ������� ����

			if (TempBuffer) // ���� � ������� ��� ���������...
			{
				if ((ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,(unsigned char *)TempBuffer,&Required)) == ERROR_SUCCESS)
					memcpy(ValueData,TempBuffer,DataSize);  // ��������� ������� ����.

				delete[] TempBuffer;
			}
		}

		if (pType)
			*pType=Type;

		CloseRegKey(hKey);
	}

	if (ExitCode!=ERROR_SUCCESS)
	{
		if (Default)
			memcpy(ValueData,Default,DataSize);
		else
			memset(ValueData,0,DataSize);

		return 0;
	}

	return(Required);
}

static string &MkKeyName(const wchar_t *Key, string &strDest)
{
	int len = (int)Opt.strRegRoot.GetLength();
	int len2 = StrLength(Key);
	wchar_t *p = strDest.GetBuffer(len + len2 + 2);
	wcscpy(p, Opt.strRegRoot);
	p+=len;

	if (*Key)
	{
		if (len)
		{
			*(p++) = L'\\';
			*p = 0;
			len++;
		}

		wcscpy(p, Key);
		len+=len2;
	}

	strDest.ReleaseBuffer(len);
	return strDest;
}


//HKEY CreateRegKey(const wchar_t *Key)
//{
//	if (hRegCurrentKey)
//		return(hRegCurrentKey);
//
//	HKEY hKey;
//	DWORD Disposition;
//	static string strFullKeyName;
//	MkKeyName(Key,strFullKeyName);
//
//	if (RegCreateKeyEx(hRegRootKey,strFullKeyName,0,nullptr,0,KEY_WRITE,nullptr,
//	                   &hKey,&Disposition) != ERROR_SUCCESS)
//		hKey=nullptr;
//
//	if (RequestSameKey)
//	{
//		RequestSameKey=FALSE;
//		hRegCurrentKey=hKey;
//	}
//
//	return(hKey);
//}


HKEY OpenRegKey(const wchar_t *Key)
{
	if (hRegCurrentKey)
		return(hRegCurrentKey);

	HKEY hKey;
	static string strFullKeyName;
	MkKeyName(Key,strFullKeyName);

	if (RegOpenKeyEx(hRegRootKey,strFullKeyName,0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,&hKey)!=ERROR_SUCCESS)
	{
		CloseSameRegKey();
		return nullptr;
	}

	if (RequestSameKey)
	{
		RequestSameKey=FALSE;
		hRegCurrentKey=hKey;
	}

	return(hKey);
}


//void DeleteRegKey(const wchar_t *Key)
//{
//	string strFullKeyName;
//	MkKeyName(Key,strFullKeyName);
//	RegDeleteKey(hRegRootKey,strFullKeyName);
//}
//
//
//void DeleteRegValue(const wchar_t *Key,const wchar_t *Value)
//{
//	HKEY hKey;
//	string strFullKeyName;
//	MkKeyName(Key,strFullKeyName);
//
//	if (RegOpenKeyEx(hRegRootKey,strFullKeyName,0,KEY_WRITE,&hKey)==ERROR_SUCCESS)
//	{
//		RegDeleteValue(hKey,Value);
//		CloseRegKey(hKey);
//	}
//}
//
//void DeleteKeyRecord(const wchar_t *KeyMask,int Position)
//{
//	string strFullKeyName, strNextFullKeyName;
//	string strMaskKeyName;
//	MkKeyName(KeyMask, strMaskKeyName);
//
//	for (;;)
//	{
//		strFullKeyName.Format(strMaskKeyName,Position++);
//		strNextFullKeyName.Format(strMaskKeyName,Position);
//		DeleteFullKeyTree(strFullKeyName);
//		if (!CopyKeyTree(strNextFullKeyName,strFullKeyName))
//		{
//			DeleteFullKeyTree(strFullKeyName);
//			break;
//		}
//	}
//}
//
//void InsertKeyRecord(const wchar_t *KeyMask,int Position,int TotalKeys)
//{
//	string strFullKeyName, strPrevFullKeyName;
//	string strMaskKeyName;
//	MkKeyName(KeyMask,strMaskKeyName);
//
//	for (int CurPos=TotalKeys; CurPos>Position; CurPos--)
//	{
//		strFullKeyName.Format(strMaskKeyName,CurPos);
//		strPrevFullKeyName.Format(strMaskKeyName,CurPos-1);
//
//		if (!CopyKeyTree(strPrevFullKeyName,strFullKeyName))
//			break;
//	}
//
//	strFullKeyName.Format(strMaskKeyName,Position);
//	DeleteFullKeyTree(strFullKeyName);
//}
//
//
//class KeyRecordItem
//{
//	public:
//		int ItemIdx;
//		KeyRecordItem() { ItemIdx=0; }
//		bool operator==(const KeyRecordItem &rhs) const
//		{
//			return ItemIdx == rhs.ItemIdx;
//		};
//		int operator<(const KeyRecordItem &rhs) const
//		{
//			return ItemIdx < rhs.ItemIdx;
//		};
//		const KeyRecordItem& operator=(const KeyRecordItem &rhs)
//		{
//			ItemIdx = rhs.ItemIdx;
//			return *this;
//		};
//
//		~KeyRecordItem()
//		{
//		}
//};
//
//void RenumKeyRecord(const wchar_t *KeyRoot,const wchar_t *KeyMask,const wchar_t *KeyMask0)
//{
//	TArray<KeyRecordItem> KAItems;
//	KeyRecordItem KItem;
//	string strRegKey;
//	string strFullKeyName, strPrevFullKeyName;
//	string strMaskKeyName;
//	BOOL Processed=FALSE;
//
//	// ���� ������
//	for (int CurPos=0;; CurPos++)
//	{
//		if (!EnumRegKey(KeyRoot,CurPos,strRegKey))
//			break;
//
//		KItem.ItemIdx=_wtoi(strRegKey.CPtr()+StrLength(KeyMask0));
//
//		if (KItem.ItemIdx != CurPos)
//			Processed=TRUE;
//
//		KAItems.addItem(KItem);
//	}
//
//	if (Processed)
//	{
//		KAItems.Sort();
//		MkKeyName(KeyMask,strMaskKeyName);
//
//		for (int CurPos=0;; ++CurPos)
//		{
//			KeyRecordItem *Item=KAItems.getItem(CurPos);
//
//			if (!Item)
//				break;
//
//			// �������� ������������� CurPos
//			strFullKeyName.Format(KeyMask,CurPos);
//
//			if (!CheckRegKey(strFullKeyName))
//			{
//				strFullKeyName.Format(strMaskKeyName,CurPos);
//				strPrevFullKeyName.Format(strMaskKeyName,Item->ItemIdx);
//
//				if (!CopyKeyTree(strPrevFullKeyName,strFullKeyName))
//					break;
//
//				DeleteFullKeyTree(strPrevFullKeyName);
//			}
//		}
//	}
//}
//
//
//int CopyKeyTree(const wchar_t *Src,const wchar_t *Dest,const wchar_t *Skip)
//{
//	HKEY hSrcKey,hDestKey;
//
//	if (RegOpenKeyEx(hRegRootKey,Src,0,KEY_READ,&hSrcKey)!=ERROR_SUCCESS)
//		return FALSE;
//
//	DeleteFullKeyTree(Dest);
//	DWORD Disposition;
//
//	if (RegCreateKeyEx(hRegRootKey,Dest,0,nullptr,0,KEY_WRITE,nullptr,&hDestKey,&Disposition)!=ERROR_SUCCESS)
//	{
//		CloseRegKey(hSrcKey);
//		return FALSE;
//	}
//
//	wchar_t ValueDataStatic[1000];
//	DWORD AllocDataSize=sizeof(ValueDataStatic);
//	bool allocData=false;
//	wchar_t *ValueData=ValueDataStatic;
//
//	for (int i=0; ; i++)
//	{
//		wchar_t ValueName[200];
//		DWORD Type,NameSize=ARRAYSIZE(ValueName),DataSize=AllocDataSize;
//		int ExitCode=RegEnumValue(hSrcKey,i,ValueName,&NameSize,nullptr,&Type,(BYTE *)ValueData,&DataSize);
//
//		if (ExitCode != ERROR_SUCCESS)
//		{
//			if (ExitCode != ERROR_MORE_DATA)
//				break;
//
//			if (DataSize > AllocDataSize)
//			{
//				AllocDataSize=DataSize;
//				wchar_t *NewValueData=(wchar_t *)xf_malloc(AllocDataSize);
//
//				if (!NewValueData)
//					break;
//
//				allocData=true;
//				ValueData=NewValueData;
//			}
//
//			ExitCode=RegEnumValue(hSrcKey,i,ValueName,&NameSize,nullptr,&Type,(BYTE *)ValueData,&DataSize);
//
//			if (ExitCode != ERROR_SUCCESS)
//				break;
//		}
//
//		RegSetValueEx(hDestKey,ValueName,0,Type,(BYTE *)ValueData,DataSize);
//	}
//
//	if (allocData && ValueData)
//		xf_free(ValueData);
//
//	CloseRegKey(hDestKey);
//
//	for (int i=0; ; i++)
//	{
//		string strSubkeyName, strSrcKeyName, strDestKeyName;
//
//		if (apiRegEnumKeyEx(hSrcKey,i,strSubkeyName)!=ERROR_SUCCESS)
//			break;
//
//		strSrcKeyName  = Src;
//		strSrcKeyName += L"\\";
//		strSrcKeyName += strSubkeyName;
//
//		if (Skip)
//		{
//			bool Found=false;
//			const wchar_t *SkipName=Skip;
//
//			while (!Found && *SkipName)
//				if (!StrCmpI(strSrcKeyName,SkipName))
//					Found=true;
//				else
//					SkipName+=StrLength(SkipName)+1;
//
//			if (Found)
//				continue;
//		}
//
//		strDestKeyName = Dest;
//		strDestKeyName += L"\\";
//		strDestKeyName += strSubkeyName;
//
//		if (RegCreateKeyEx(hRegRootKey,strDestKeyName,0,nullptr,0,KEY_WRITE,nullptr,&hDestKey,&Disposition)!=ERROR_SUCCESS)
//			break;
//
//		CloseRegKey(hDestKey);
//		CopyKeyTree(strSrcKeyName,strDestKeyName);
//	}
//
//	CloseRegKey(hSrcKey);
//	return TRUE;
//}
//
//int CopyLocalKeyTree(const wchar_t *Src,const wchar_t *Dst)
//{
//	string strFullSrc,strFullDst;
//	MkKeyName(Src,strFullSrc);
//	MkKeyName(Dst,strFullDst);
//	return CopyKeyTree(strFullSrc,strFullDst);
//}
//
//void DeleteKeyTree(const wchar_t *KeyName)
//{
//	string strFullKeyName;
//	MkKeyName(KeyName,strFullKeyName);
//	DeleteFullKeyTree(strFullKeyName);
//}
//
//void DeleteFullKeyTree(const wchar_t *KeyName)
//{
//	do
//	{
//		DeleteCount=0;
//		DeleteKeyTreePart(KeyName);
//	}
//	while (DeleteCount);
//}
//
//void DeleteKeyTreePart(const wchar_t *KeyName)
//{
//	HKEY hKey;
//
//	if (RegOpenKeyEx(hRegRootKey,KeyName,0,KEY_READ,&hKey)!=ERROR_SUCCESS)
//		return;
//
//	for (int I=0;; I++)
//	{
//		string strSubkeyName,strFullKeyName;
//
//		if (apiRegEnumKeyEx(hKey,I,strSubkeyName)!=ERROR_SUCCESS)
//			break;
//
//		strFullKeyName = KeyName;
//		strFullKeyName += L"\\";
//		strFullKeyName += strSubkeyName;
//		DeleteKeyTreePart(strFullKeyName);
//	}
//
//	CloseRegKey(hKey);
//
//	if (RegDeleteKey(hRegRootKey,KeyName)==ERROR_SUCCESS)
//		DeleteCount++;
//}
//
//
///* 07.03.2001 IS
//   �������� ������� ����� � ��� ������, ���� �� �� �������� ������� ����������
//   � ���������. ���������� TRUE ��� ������.
//*/
//
//int DeleteEmptyKey(HKEY hRoot, const wchar_t *FullKeyName)
//{
//	HKEY hKey;
//	int Exist=RegOpenKeyEx(hRoot,FullKeyName,0,KEY_ALL_ACCESS,&hKey)==ERROR_SUCCESS;
//
//	if (Exist)
//	{
//		int RetCode=FALSE;
//
//		if (hKey)
//		{
//			string strSubName;
//			LONG ExitCode=apiRegEnumKeyEx(hKey,0,strSubName);
//
//			if (ExitCode!=ERROR_SUCCESS)
//			{
//				wchar_t SubName[1];	// no matter
//				DWORD SubSize=ARRAYSIZE(SubName);
//				ExitCode=RegEnumValue(hKey,0,SubName,&SubSize,nullptr,nullptr,nullptr, nullptr);
//			}
//
//			CloseRegKey(hKey);
//
//			if (ExitCode!=ERROR_SUCCESS)
//			{
//				string strKeyName = FullKeyName;
//				wchar_t *pKeyName = strKeyName.GetBuffer();
//				wchar_t *pSubKey = pKeyName;
//
//				if (nullptr!=(pSubKey=wcsrchr(pSubKey,L'\\')))
//				{
//					*pSubKey=0;
//					pSubKey++;
//					Exist=RegOpenKeyEx(hRoot,pKeyName,0,KEY_ALL_ACCESS,&hKey)==ERROR_SUCCESS;
//
//					if (Exist && hKey)
//					{
//						RetCode=RegDeleteKey(hKey, pSubKey)==ERROR_SUCCESS;
//						CloseRegKey(hKey);
//					}
//				}
//
//				//strKeyName.ReleaseBuffer (); �� ����, ������ ��� ����� ���������
//			}
//		}
//
//		return RetCode;
//	}
//
//	return TRUE;
//}
//
//int CheckRegKey(const wchar_t *Key)
//{
//	HKEY hKey;
//	string strFullKeyName;
//	MkKeyName(Key,strFullKeyName);
//	int Exist=RegOpenKeyEx(hRegRootKey,strFullKeyName,0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS;
//	CloseRegKey(hKey);
//	return(Exist);
//}
//
///* 15.09.2000 IS
//   ���������� FALSE, ���� ��������� ���������� �� �������� ������
//   ��� ������ ������ ����� ����.
//*/
//int CheckRegValue(const wchar_t *Key,const wchar_t *ValueName)
//{
//	int ExitCode=!ERROR_SUCCESS;
//	DWORD DataSize=0;
//	HKEY hKey=OpenRegKey(Key);
//
//	if (hKey)
//	{
//		DWORD Type;
//		ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,nullptr,&DataSize);
//		CloseRegKey(hKey);
//	}
//
//	if (ExitCode!=ERROR_SUCCESS || !DataSize)
//		return FALSE;
//
//	return TRUE;
//}
//
//int EnumRegKey(const wchar_t *Key,DWORD Index,string &strDestName)
//{
//	HKEY hKey=OpenRegKey(Key);
//
//	if (hKey)
//	{
//		int ExitCode=apiRegEnumKeyEx(hKey,Index,strDestName);
//		CloseRegKey(hKey);
//
//		if (ExitCode==ERROR_SUCCESS)
//		{
//			string strTempName = Key;
//
//			if (!strTempName.IsEmpty())
//				AddEndSlash(strTempName);
//
//			strTempName += strDestName;
//			strDestName = strTempName; //???
//			return TRUE;
//		}
//	}
//
//	return FALSE;
//}
//
//int EnumRegValue(const wchar_t *Key,DWORD Index, string &strDestName,LPBYTE SData,DWORD SDataSize,LPDWORD IData,__int64* IData64)
//{
//	string strSData;
//	int ExitCode=EnumRegValueEx(Key,Index,strDestName,strSData,IData,IData64);
//
//	if (ExitCode != REG_NONE)
//	{
//		// ??? need check ExitCode ???
//		memcpy(SData,strSData.CPtr(),Min(SDataSize,(DWORD)(strSData.GetLength()*sizeof(wchar_t))));
//	}
//
//	return ExitCode;
//}
//
//int EnumRegValueEx(const wchar_t *Key,DWORD Index, string &strDestName, string &strSData, LPDWORD IData, __int64* IData64, DWORD *lpType)
//{
//	HKEY hKey=OpenRegKey(Key);
//	int RetCode=REG_NONE;
//	DWORD Type=(DWORD)-1;
//
//	if (hKey)
//	{
//		string strValueName;
//		DWORD ValNameSize=512, ValNameSize0;
//		LONG ExitCode=ERROR_MORE_DATA;
//
//		// get size value name
//		for (; ExitCode==ERROR_MORE_DATA; ValNameSize<<=1)
//		{
//			wchar_t *Name=strValueName.GetBuffer(ValNameSize);
//			ValNameSize0=ValNameSize;
//			ExitCode=RegEnumValue(hKey,Index,Name,&ValNameSize0,nullptr,nullptr,nullptr,nullptr);
//			strValueName.ReleaseBuffer();
//		}
//
//		if (ExitCode != ERROR_NO_MORE_ITEMS)
//		{
//			DWORD Size = 0, Size0;
//			ValNameSize0=ValNameSize;
//			// Get DataSize
//			/*ExitCode = */RegEnumValue(hKey,Index,(LPWSTR)strValueName.CPtr(),&ValNameSize0, nullptr, &Type, nullptr, &Size);
//			// ����� ExitCode == ERROR_SUCCESS
//
//			// ������������� �������
//			if (Type == REG_DWORD)
//			{
//				if (Size < sizeof(DWORD))
//					Size = sizeof(DWORD);
//			}
//			else if (Type == REG_QWORD)
//			{
//				if (Size < sizeof(__int64))
//					Size = sizeof(__int64);
//			}
//
//			wchar_t *Data = strSData.GetBuffer(Size/sizeof(wchar_t)+1);
//			wmemset(Data,0,Size/sizeof(wchar_t)+1);
//			ValNameSize0=ValNameSize;
//			Size0=Size;
//			ExitCode=RegEnumValue(hKey,Index,(LPWSTR)strValueName.CPtr(),&ValNameSize0,nullptr,&Type,(LPBYTE)Data,&Size0);
//
//			if (ExitCode == ERROR_SUCCESS)
//			{
//				if (Type == REG_DWORD)
//				{
//					if (IData)
//						*IData=*(DWORD*)Data;
//				}
//				else if (Type == REG_QWORD)
//				{
//					if (IData64)
//						*IData64=*(__int64*)Data;
//				}
//
//				RetCode=Type;
//				strDestName = strValueName;
//			}
//
//			strSData.ReleaseBuffer(Size/sizeof(wchar_t));
//		}
//
//		CloseRegKey(hKey);
//	}
//
//	if (lpType)
//		*lpType=Type;
//
//	return RetCode;
//}


LONG CloseRegKey(HKEY hKey)
{
	if (hRegCurrentKey || !hKey)
		return ERROR_SUCCESS;

	return(RegCloseKey(hKey));
}


//int RegQueryStringValueEx(
//    HKEY hKey,
//    const wchar_t *lpwszValueName,
//    string &strData,
//    const wchar_t *lpwszDefault
//)
//{
//	DWORD cbSize = 0;
//	int nResult = RegQueryValueEx(
//	                  hKey,
//	                  lpwszValueName,
//	                  nullptr,
//	                  nullptr,
//	                  nullptr,
//	                  &cbSize
//	              );
//
//	if (nResult == ERROR_SUCCESS)
//	{
//		wchar_t *lpwszData = strData.GetBuffer(cbSize/sizeof(wchar_t)+1);
//		nResult = RegQueryValueEx(
//		              hKey,
//		              lpwszValueName,
//		              nullptr,
//		              nullptr,
//		              (LPBYTE)lpwszData,
//		              &cbSize
//		          );
//		lpwszData[cbSize/sizeof(wchar_t)] = 0;
//		strData.ReleaseBuffer();
//	}
//
//	if (nResult != ERROR_SUCCESS)
//		strData = lpwszDefault;
//
//	return nResult;
//}
//
//int RegQueryStringValue(
//    HKEY hKey,
//    const wchar_t *lpwszSubKey,
//    string &strData,
//    const wchar_t *lpwszDefault
//)
//{
//	DWORD cbSize = 0;
//	int nResult = RegQueryValueEx(
//	                  hKey,
//	                  lpwszSubKey,
//	                  nullptr,
//	                  nullptr,
//	                  nullptr,
//	                  &cbSize
//	              );
//
//	if (nResult == ERROR_SUCCESS)
//	{
//		wchar_t *lpwszData = strData.GetBuffer(cbSize/sizeof(wchar_t)+1);
//		DWORD Type=REG_SZ;
//		nResult = RegQueryValueEx(
//		              hKey,
//		              lpwszSubKey,
//		              nullptr,
//		              &Type,
//		              (LPBYTE)lpwszData,
//		              &cbSize
//		          );
//		int Size=cbSize/sizeof(wchar_t);
//
//		if (Type==REG_SZ||Type==REG_EXPAND_SZ||Type==REG_MULTI_SZ)
//		{
//			if (!lpwszData[Size-1])
//				Size--;
//
//			strData.ReleaseBuffer(Size);
//		}
//		else
//		{
//			lpwszData[Size] = 0;
//			strData.ReleaseBuffer();
//		}
//	}
//
//	if (nResult != ERROR_SUCCESS)
//		strData = lpwszDefault;
//
//	return nResult;
//}
