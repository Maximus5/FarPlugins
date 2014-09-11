/*
pathmix.cpp

Misc functions for processing of path names
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

#include "pathmix.hpp"
#include "strmix.hpp"
//#include "imports.hpp"

const wchar_t *ReservedFilenameSymbols = L"<>|";

NTPath::NTPath(LPCWSTR Src)
{
	if (Src&&*Src)
	{
		Str=Src;
		ReplaceSlashToBSlash(Str);

		//if (!HasPathPrefix(Src)) -- �� ��������� � diz
		//{
		//	ConvertNameToFull(Str,Str);

		//	if (!HasPathPrefix(Str))
		//	{
		//		if (IsLocalPath(Str))
		//			Str=string(L"\\\\?\\")+Str;
		//		else
		//			Str=string(L"\\\\?\\UNC\\")+&Str[2];
		//	}
		//}
	}
}

//bool IsAbsolutePath(const wchar_t *Path)
//{
//	return Path && (HasPathPrefix(Path) || IsLocalPath(Path) || IsNetworkPath(Path));
//}

bool IsNetworkPath(const wchar_t *Path)
{
	return Path && ((Path[0] == L'\\' && Path[1] == L'\\' && !HasPathPrefix(Path))||(HasPathPrefix(Path) && !StrCmpNI(Path+4,L"UNC\\",4)));
}

//bool IsNetworkServerPath(const wchar_t *Path)
//{
///*
//	"\\server\share\" is valid windows path.
//	"\\server\" is not.
//*/
//	bool Result=false;
//	if(IsNetworkPath(Path))
//	{
//		LPCWSTR SharePtr=wcspbrk(HasPathPrefix(Path)?Path+8:Path+2,L"\\/");
//		if(!SharePtr || !SharePtr[1] || IsSlash(SharePtr[1]))
//		{
//			Result=true;
//		}
//	}
//	return Result;
//}

bool IsLocalPath(const wchar_t *Path)
{
	return (Path && *Path && Path[1]==L':');
}

//bool IsLocalRootPath(const wchar_t *Path)
//{
//	return (Path && *Path && Path[1]==L':' && IsSlash(Path[2]) && !Path[3]);
//}

bool HasPathPrefix(const wchar_t *Path)
{
	/*
		\\?\
		\\.\
		\??\
	*/
	return Path && Path[0] == L'\\' && (Path[1] == L'\\' || Path[1] == L'?') && (Path[2] == L'?' || Path[2] == L'.') && Path[3] == L'\\';
}

//bool IsLocalPrefixPath(const wchar_t *Path)
//{
//	return HasPathPrefix(Path) && Path[4] && Path[5] == L':' && Path[6] == L'\\';
//}
//
//bool IsLocalPrefixRootPath(const wchar_t *Path)
//{
//	return IsLocalPrefixPath(Path) && !Path[7];
//}
//
//bool IsLocalVolumePath(const wchar_t *Path)
//{
//	return HasPathPrefix(Path) && !_wcsnicmp(&Path[4],L"Volume{",7) && Path[47] == L'}';
//}
//
//bool IsLocalVolumeRootPath(const wchar_t *Path)
//{
//	return IsLocalVolumePath(Path) && (!Path[48] || (IsSlash(Path[48]) && !Path[49]));
//}

bool PathCanHoldRegularFile(const wchar_t *Path)
{
	if (!IsNetworkPath(Path))
		return true;

	/* \\ */
	unsigned offset = 2;

	/* \\?\UNC\ */
	if (Path[2] == L'?')
		offset = 8;

	const wchar_t *p = FirstSlash(Path + offset);

	/* server || server\ */
	if (!p || !*(p+1))
		return false;

	return true;
}

//bool IsPluginPrefixPath(const wchar_t *Path) //Max:
//{
//	if (Path[0] == L'\\')
//		return false;
//	const wchar_t* pC = wcschr(Path, L':');
//	if (!pC)
//		return false;
//	if ((pC - Path) == 1) {
//		wchar_t d = (Path[0] & ~0x20);
//		if (d >= L'C' && d <= 'Z')
//			return false; // ����� �����, � �� �������
//	}
//	const wchar_t* pS = FirstSlash(Path);
//	if (pS && pS < pC)
//		return false;
//	return true;
//}
//
////bool TestParentFolderName(const wchar_t *Name)
////{
////	return Name[0] == L'.' && Name[1] == L'.' && (!Name[2] || (IsSlash(Name[2]) && !Name[3]));
////}
////
////bool TestCurrentFolderName(const wchar_t *Name)
////{
////	return Name[0] == L'.' && (!Name[1] || (IsSlash(Name[1]) && !Name[2]));
////}
////
////bool TestCurrentDirectory(const wchar_t *TestDir)
////{
////	string strCurDir;
////
////	if (apiGetCurrentDirectory(strCurDir) && !StrCmpI(strCurDir,TestDir))
////		return true;
////
////	return false;
////}
//
//const wchar_t* __stdcall PointToName(const wchar_t *lpwszPath)
//{
//	return PointToName(lpwszPath,nullptr);
//}
//
//const wchar_t* PointToName(string& strPath)
//{
//	const wchar_t *lpwszPath=strPath.CPtr();
//	const wchar_t *lpwszEndPtr=lpwszPath+strPath.GetLength();
//	return PointToName(lpwszPath,lpwszEndPtr);
//}
//
//const wchar_t* PointToName(const wchar_t *lpwszPath,const wchar_t *lpwszEndPtr)
//{
//	if (!lpwszPath)
//		return nullptr;
//
//	if (*lpwszPath && *(lpwszPath+1)==L':') lpwszPath+=2;
//
//	const wchar_t *lpwszNamePtr = lpwszEndPtr;
//
//	if (!lpwszNamePtr)
//	{
//		lpwszNamePtr=lpwszPath;
//
//		while (*lpwszNamePtr) lpwszNamePtr++;
//	}
//
//	while (lpwszNamePtr != lpwszPath)
//	{
//		if (IsSlash(*lpwszNamePtr))
//			return lpwszNamePtr+1;
//
//		lpwszNamePtr--;
//	}
//
//	if (IsSlash(*lpwszPath))
//		return lpwszPath+1;
//	else
//		return lpwszPath;
//}
//
////   ������ PointToName, ������ ��� ����� ����
////   "name\" (������������ �� ����) ���������� ��������� �� name, � �� �� ������
////   ������
//const wchar_t* __stdcall PointToFolderNameIfFolder(const wchar_t *Path)
//{
//	if (!Path)
//		return nullptr;
//
//	const wchar_t *NamePtr=Path, *prevNamePtr=Path;
//
//	while (*Path)
//	{
//		if (IsSlash(*Path) ||
//		        (*Path==L':' && Path==NamePtr+1))
//		{
//			prevNamePtr=NamePtr;
//			NamePtr=Path+1;
//		}
//
//		++Path;
//	}
//
//	return ((*NamePtr)?NamePtr:prevNamePtr);
//}
//
//const wchar_t* PointToExt(const wchar_t *lpwszPath)
//{
//	if (!lpwszPath)
//		return nullptr;
//
//	const wchar_t *lpwszEndPtr = lpwszPath;
//
//	while (*lpwszEndPtr) lpwszEndPtr++;
//
//	return PointToExt(lpwszPath,lpwszEndPtr);
//}
//
//const wchar_t* PointToExt(string& strPath)
//{
//	const wchar_t *lpwszPath=strPath.CPtr();
//	const wchar_t *lpwszEndPtr=lpwszPath+strPath.GetLength();
//	return PointToExt(lpwszPath,lpwszEndPtr);
//}
//
//const wchar_t* PointToExt(const wchar_t *lpwszPath,const wchar_t *lpwszEndPtr)
//{
//	if (!lpwszPath || !lpwszEndPtr)
//		return nullptr;
//
//	const wchar_t *lpwszExtPtr = lpwszEndPtr;
//
//	while (lpwszExtPtr != lpwszPath)
//	{
//		if (*lpwszExtPtr==L'.')
//		{
//			if (IsSlash(*(lpwszExtPtr-1)) || *(lpwszExtPtr-1)==L':')
//				return lpwszEndPtr;
//			else
//				return lpwszExtPtr;
//		}
//
//		if (IsSlash(*lpwszExtPtr) || *lpwszExtPtr==L':')
//			return lpwszEndPtr;
//
//		lpwszExtPtr--;
//	}
//
//	return lpwszEndPtr;
//}

BOOL AddEndSlash(wchar_t *Path, wchar_t TypeSlash)
{
	BOOL Result=FALSE;

	if (Path)
	{
		/* $ 06.12.2000 IS
		  ! ������ ������� �������� � ������ ������ ������, ����� ����������
		    ��������� ��� ������������� ��������� ����� �� �����, �������
		    ����������� ����.
		*/
		wchar_t *end;
		int Slash=0, BackSlash=0;

		if (!TypeSlash)
		{
			end=Path;

			while (*end)
			{
				Slash+=(*end==L'\\');
				BackSlash+=(*end==L'/');
				end++;
			}
		}
		else
		{
			end=Path+StrLength(Path);

			if (TypeSlash == L'\\')
				Slash=1;
			else
				BackSlash=1;
		}

		int Length=(int)(end-Path);
		char c=(Slash<BackSlash)?L'/':L'\\';
		Result=TRUE;

		if (!Length)
		{
			*end=c;
			end[1]=0;
		}
		else
		{
			end--;

			if (!IsSlash(*end))
			{
				end[1]=c;
				end[2]=0;
			}
			else
			{
				*end=c;
			}
		}
	}

	return Result;
}


BOOL WINAPI AddEndSlash(wchar_t *Path)
{
	return AddEndSlash(Path, 0);
}


BOOL AddEndSlash(string &strPath)
{
	return AddEndSlash(strPath, 0);
}

BOOL AddEndSlash(string &strPath, wchar_t TypeSlash)
{
	wchar_t *lpwszPath = strPath.GetBuffer(strPath.GetLength()+2);
	BOOL Result = AddEndSlash(lpwszPath, TypeSlash);
	strPath.ReleaseBuffer();
	return Result;
}

//bool DeleteEndSlash(wchar_t *Path, bool AllEndSlash)
//{
//	bool Ret = false;
//	size_t len = StrLength(Path);
//
//	while (len && IsSlash(Path[--len]))
//	{
//		Ret = true;
//		Path[len] = L'\0';
//
//		if (!AllEndSlash)
//			break;
//	}
//
//	return Ret;
//}
//
//BOOL WINAPI DeleteEndSlash(string &strPath, bool AllEndSlash)
//{
//	BOOL Ret=FALSE;
//
//	if (!strPath.IsEmpty())
//	{
//		size_t len=strPath.GetLength();
//		wchar_t *lpwszPath = strPath.GetBuffer();
//
//		while (len && IsSlash(lpwszPath[--len]))
//		{
//			Ret=TRUE;
//			lpwszPath[len] = L'\0';
//
//			if (!AllEndSlash)
//				break;
//		}
//
//		strPath.ReleaseBuffer();
//	}
//
//	return Ret;
//}
//
//bool CutToSlash(string &strStr, bool bInclude)
//{
//	size_t pos;
//
//	if (FindLastSlash(pos,strStr))
//	{
//		if (pos==3 && HasPathPrefix(strStr))
//			return false;
//
//		if (bInclude)
//			strStr.SetLength(pos);
//		else
//			strStr.SetLength(pos+1);
//
//		return true;
//	}
//
//	return false;
//}
//
//string &CutToNameUNC(string &strPath)
//{
//	wchar_t *lpwszPath = strPath.GetBuffer();
//
//	if (IsSlash(lpwszPath[0]) && IsSlash(lpwszPath[1]))
//	{
//		lpwszPath+=2;
//
//		for (int i=0; i<2; i++)
//		{
//			while (*lpwszPath && !IsSlash(*lpwszPath))
//				lpwszPath++;
//
//			if (*lpwszPath)
//				lpwszPath++;
//		}
//	}
//
//	wchar_t *lpwszNamePtr = lpwszPath;
//
//	while (*lpwszPath)
//	{
//		if (IsSlash(*lpwszPath) || (*lpwszPath==L':' && lpwszPath == lpwszNamePtr+1))
//			lpwszNamePtr = lpwszPath+1;
//
//		lpwszPath++;
//	}
//
//	*lpwszNamePtr = 0;
//	strPath.ReleaseBuffer();
//
//	return strPath;
//}
//
//string &CutToFolderNameIfFolder(string &strPath)
//{
//	wchar_t *lpwszPath = strPath.GetBuffer();
//	wchar_t *lpwszNamePtr=lpwszPath, *lpwszprevNamePtr=lpwszPath;
//
//	while (*lpwszPath)
//	{
//		if (IsSlash(*lpwszPath) || (*lpwszPath==L':' && lpwszPath==lpwszNamePtr+1))
//		{
//			lpwszprevNamePtr=lpwszNamePtr;
//			lpwszNamePtr=lpwszPath+1;
//		}
//
//		++lpwszPath;
//	}
//
//	if (*lpwszNamePtr)
//		*lpwszNamePtr=0;
//	else
//		*lpwszprevNamePtr=0;
//
//	strPath.ReleaseBuffer();
//	return strPath;
//}
//
//const wchar_t *PointToNameUNC(const wchar_t *lpwszPath)
//{
//	if (!lpwszPath)
//		return nullptr;
//
//	if (IsSlash(lpwszPath[0]) && IsSlash(lpwszPath[1]))
//	{
//		lpwszPath+=2;
//
//		for (int i=0; i<2; i++)
//		{
//			while (*lpwszPath && !IsSlash(*lpwszPath))
//				lpwszPath++;
//
//			if (*lpwszPath)
//				lpwszPath++;
//		}
//	}
//
//	const wchar_t *lpwszNamePtr = lpwszPath;
//
//	while (*lpwszPath)
//	{
//		if (IsSlash(*lpwszPath) || (*lpwszPath==L':' && lpwszPath == lpwszNamePtr+1))
//			lpwszNamePtr = lpwszPath+1;
//
//		lpwszPath++;
//	}
//
//	return lpwszNamePtr;
//}

string &ReplaceSlashToBSlash(string &strStr)
{
	wchar_t *lpwszStr = strStr.GetBuffer();

	while (*lpwszStr)
	{
		if (*lpwszStr == L'/')
			*lpwszStr = L'\\';

		lpwszStr++;
	}

	strStr.ReleaseBuffer(strStr.GetLength());
	return strStr;
}

const wchar_t *FirstSlash(const wchar_t *String)
{
	do
	{
		if (IsSlash(*String))
			return String;
	}
	while (*String++);

	return nullptr;
}

//const wchar_t *LastSlash(const wchar_t *String)
//{
//	const wchar_t *Start = String;
//
//	while (*String++)
//		;
//
//	while (--String!=Start && !IsSlash(*String))
//		;
//
//	return IsSlash(*String)?String:nullptr;
//}
//
//bool FindSlash(size_t &Pos, const string &Str, size_t StartPos)
//{
//	for (size_t p = StartPos; p < Str.GetLength(); p++)
//	{
//		if (IsSlash(Str[p]))
//		{
//			Pos = p;
//			return true;
//		}
//	}
//
//	return false;
//}
//
//bool FindLastSlash(size_t &Pos, const string &Str)
//{
//	for (size_t p = Str.GetLength(); p > 0; p--)
//	{
//		if (IsSlash(Str[p - 1]))
//		{
//			Pos = p - 1;
//			return true;
//		}
//	}
//
//	return false;
//}
//
//// find path root component (drive letter / volume name / server share) and calculate its length
//size_t GetPathRootLength(const string &Path)
//{
//	unsigned PrefixLen = 0;
//	bool IsUNC = false;
//
//	if (Path.Equal(0,8,L"\\\\?\\UNC\\",8))
//	{
//		PrefixLen = 8;
//		IsUNC = true;
//	}
//	else if (Path.Equal(0,4,L"\\\\?\\",4) || Path.Equal(0,4,L"\\??\\",4) || Path.Equal(0,4,L"\\\\.\\",4))
//	{
//		PrefixLen = 4;
//	}
//	else if (Path.Equal(0,2,L"\\\\",2))
//	{
//		PrefixLen = 2;
//		IsUNC = true;
//	}
//
//	if (!PrefixLen && !Path.Equal(1, L':'))
//		return 0;
//
//	size_t p;
//
//	if (!FindSlash(p, Path, PrefixLen))
//		p = Path.GetLength();
//
//	if (IsUNC)
//		if (!FindSlash(p, Path, p + 1))
//			p = Path.GetLength();
//
//	return p;
//}
//
//string ExtractPathRoot(const string &Path)
//{
//	size_t PathRootLen = GetPathRootLength(Path);
//
//	if (PathRootLen)
//		return string(Path.CPtr(), PathRootLen).Append(L'\\');
//	else
//		return string();
//}
//
//string ExtractFileName(const string &Path)
//{
//	size_t p;
//
//	if (FindLastSlash(p, Path))
//		p++;
//	else
//		p = 0;
//
//	size_t PathRootLen = GetPathRootLength(Path);
//
//	if (p <= PathRootLen && PathRootLen)
//		return string();
//
//	return string(Path.CPtr() + p, Path.GetLength() - p);
//}
//
//string ExtractFilePath(const string &Path)
//{
//	size_t p;
//
//	if (!FindLastSlash(p, Path))
//		p = 0;
//
//	size_t PathRootLen = GetPathRootLength(Path);
//
//	if (p <= PathRootLen && PathRootLen)
//		return string(Path.CPtr(), PathRootLen).Append(L'\\');
//
//	return string(Path.CPtr(), p);
//}
//
//bool IsRootPath(const string &Path)
//{
//	size_t PathRootLen = GetPathRootLength(Path);
//
//	if (Path.GetLength() == PathRootLen)
//		return true;
//
//	if (Path.GetLength() == PathRootLen + 1 && IsSlash(Path[Path.GetLength() - 1]))
//		return true;
//
//	return false;
//}
//
//bool PathStartsWith(const string &Path, const string &Start)
//{
//	string PathPart(Start);
//	DeleteEndSlash(PathPart, true);
//	return Path.Equal(0, PathPart) && (Path.GetLength() == PathPart.GetLength() || IsSlash(Path[PathPart.GetLength()]));
//}
//
//int MatchNtPathRoot(const string &NtPath, const wchar_t *DeviceName)
//{
//	string TargetPath;
//	if (apiQueryDosDevice(DeviceName, TargetPath))
//	{
//		TargetPath.ReleaseBuffer();
//
//		if (PathStartsWith(NtPath, TargetPath))
//			return static_cast<int>(TargetPath.GetLength());
//
//		// path could be an Object Manager symlink, try to resolve
//		UNICODE_STRING ObjName;
//		ObjName.Length = ObjName.MaximumLength = static_cast<USHORT>(TargetPath.GetLength() * sizeof(wchar_t));
//		ObjName.Buffer = const_cast<PWSTR>(TargetPath.CPtr());
//		OBJECT_ATTRIBUTES ObjAttrs;
//		InitializeObjectAttributes(&ObjAttrs, &ObjName, 0, nullptr, nullptr);
//		HANDLE hSymLink;
//		NTSTATUS Res = ifn.pfnNtOpenSymbolicLinkObject(&hSymLink, GENERIC_READ, &ObjAttrs);
//
//		if (Res == STATUS_SUCCESS)
//		{
//			ULONG BufSize = 0x7FFF;
//			string Buffer;
//			UNICODE_STRING LinkTarget;
//			LinkTarget.MaximumLength = static_cast<USHORT>(BufSize * sizeof(wchar_t));
//			LinkTarget.Buffer = Buffer.GetBuffer(BufSize);
//			Res = ifn.pfnNtQuerySymbolicLinkObject(hSymLink, &LinkTarget, nullptr);
//
//			if (Res == STATUS_SUCCESS)
//			{
//				TargetPath.Copy(LinkTarget.Buffer, LinkTarget.Length / sizeof(wchar_t));
//			}
//
//			ifn.pfnNtClose(hSymLink);
//
//			if (PathStartsWith(NtPath, TargetPath))
//				return static_cast<int>(TargetPath.GetLength());
//		}
//	}
//
//	return 0;
//}
//
//SELF_TEST(
//    assert(ExtractPathRoot(L"") == L"");
//    assert(ExtractPathRoot(L"\\") == L"");
//    assert(ExtractPathRoot(L"file") == L"");
//    assert(ExtractPathRoot(L"path\\file") == L"");
//    assert(ExtractPathRoot(L"C:") == L"C:\\");
//    assert(ExtractPathRoot(L"C:\\") == L"C:\\");
//    assert(ExtractPathRoot(L"C:\\path\\file") == L"C:\\");
//    assert(ExtractPathRoot(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\");
//    assert(ExtractPathRoot(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\");
//    assert(ExtractPathRoot(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\path\\file") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\");
//    assert(ExtractPathRoot(L"\\\\server\\share") == L"\\\\server\\share\\");
//    assert(ExtractPathRoot(L"\\\\server\\share\\") == L"\\\\server\\share\\");
//    assert(ExtractPathRoot(L"\\\\server\\share\\path\\file") == L"\\\\server\\share\\");
//    assert(ExtractPathRoot(L"\\\\?\\UNC\\server\\share") == L"\\\\?\\UNC\\server\\share\\");
//    assert(ExtractPathRoot(L"\\\\?\\UNC\\server\\share\\") == L"\\\\?\\UNC\\server\\share\\");
//    assert(ExtractPathRoot(L"\\\\?\\UNC\\server\\share\\path\\file") == L"\\\\?\\UNC\\server\\share\\");
//
//    assert(ExtractFilePath(L"") == L"");
//    assert(ExtractFilePath(L"\\") == L"");
//    assert(ExtractFilePath(L"\\file") == L"");
//    assert(ExtractFilePath(L"file") == L"");
//    assert(ExtractFilePath(L"path\\") == L"path");
//    assert(ExtractFilePath(L"path\\file") == L"path");
//    assert(ExtractFilePath(L"C:") == L"C:\\");
//    assert(ExtractFilePath(L"C:\\") == L"C:\\");
//    assert(ExtractFilePath(L"C:\\file") == L"C:\\");
//    assert(ExtractFilePath(L"C:\\path\\file") == L"C:\\path");
//    assert(ExtractFilePath(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\");
//    assert(ExtractFilePath(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\");
//    assert(ExtractFilePath(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\file") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\");
//    assert(ExtractFilePath(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\path\\file") == L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\path");
//    assert(ExtractFilePath(L"\\\\server\\share") == L"\\\\server\\share\\");
//    assert(ExtractFilePath(L"\\\\server\\share\\") == L"\\\\server\\share\\");
//    assert(ExtractFilePath(L"\\\\server\\share\\file") == L"\\\\server\\share\\");
//    assert(ExtractFilePath(L"\\\\server\\share\\path\\file") == L"\\\\server\\share\\path");
//    assert(ExtractFilePath(L"\\\\?\\UNC\\server\\share") == L"\\\\?\\UNC\\server\\share\\");
//    assert(ExtractFilePath(L"\\\\?\\UNC\\server\\share\\") == L"\\\\?\\UNC\\server\\share\\");
//    assert(ExtractFilePath(L"\\\\?\\UNC\\server\\share\\file") == L"\\\\?\\UNC\\server\\share\\");
//    assert(ExtractFilePath(L"\\\\?\\UNC\\server\\share\\path\\file") == L"\\\\?\\UNC\\server\\share\\path");
//
//    assert(ExtractFileName(L"") == L"");
//    assert(ExtractFileName(L"\\") == L"");
//    assert(ExtractFileName(L"\\file") == L"file");
//    assert(ExtractFileName(L"file") == L"file");
//    assert(ExtractFileName(L"path\\") == L"");
//    assert(ExtractFileName(L"path\\file") == L"file");
//    assert(ExtractFileName(L"C:") == L"");
//    assert(ExtractFileName(L"C:\\") == L"");
//    assert(ExtractFileName(L"C:\\file") == L"file");
//    assert(ExtractFileName(L"C:\\path\\file") == L"file");
//    assert(ExtractFileName(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}") == L"");
//    assert(ExtractFileName(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\") == L"");
//    assert(ExtractFileName(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\file") == L"file");
//    assert(ExtractFileName(L"\\\\?\\Volume{01e45c83-9ce4-11db-b27f-806d6172696f}\\path\\file") == L"file");
//    assert(ExtractFileName(L"\\\\server\\share") == L"");
//    assert(ExtractFileName(L"\\\\server\\share\\") == L"");
//    assert(ExtractFileName(L"\\\\server\\share\\file") == L"file");
//    assert(ExtractFileName(L"\\\\server\\share\\path\\file") == L"file");
//    assert(ExtractFileName(L"\\\\?\\UNC\\server\\share") == L"");
//    assert(ExtractFileName(L"\\\\?\\UNC\\server\\share\\") == L"");
//    assert(ExtractFileName(L"\\\\?\\UNC\\server\\share\\file") == L"file");
//    assert(ExtractFileName(L"\\\\?\\UNC\\server\\share\\path\\file") == L"file");
//
//    assert(IsRootPath(L"C:"));
//    assert(IsRootPath(L"C:\\"));
//    assert(IsRootPath(L"\\"));
//    assert(!IsRootPath(L"C:\\path"));
//
//    assert(PathStartsWith(L"C:\\path\\file", L"C:\\path"));
//    assert(PathStartsWith(L"C:\\path\\file", L"C:\\path\\"));
//    assert(!PathStartsWith(L"C:\\path\\file", L"C:\\pat"));
//    assert(PathStartsWith(L"\\", L""));
//    assert(!PathStartsWith(L"C:\\path\\file", L""));
//)
