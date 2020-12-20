TCHAR PluginRootKey[80];

#define HKCU HKEY_CURRENT_USER
#define HKCR HKEY_CLASSES_ROOT
#define HKCC HKEY_CURRENT_CONFIG
#define HKLM HKEY_LOCAL_MACHINE
#define HKU  HKEY_USERS

/*
  �㭪樨 ࠡ��� � ॥��஬
  �ॡ���� ������ ��६����� PluginRootKey
*/

#ifdef __cplusplus

HKEY CreateRegKey(HKEY hRoot,TCHAR *Key);
HKEY OpenRegKey(HKEY hRoot,TCHAR *Key);

// ��� ���祭�� � ������ ValueName ��⠭����� ���祭�� ⨯� char*
void SetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,TCHAR *ValueData)
{
	HKEY hKey=CreateRegKey(hRoot,Key);
	RegSetValueEx(hKey,ValueName,0,REG_SZ,(const BYTE*)ValueData,(lstrlen(ValueData)+1)*sizeof(TCHAR));
	RegCloseKey(hKey);
}


// ��⠭����� ���祭�� ⨯� DWORD
void SetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,DWORD ValueData)
{
	HKEY hKey=CreateRegKey(hRoot,Key);
	RegSetValueEx(hKey,ValueName,0,REG_DWORD,(BYTE *)&ValueData,sizeof(ValueData));
	RegCloseKey(hKey);
}


// ��⠭����� ���祭�� ⨯� Binary
void SetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,BYTE *ValueData,DWORD ValueSize)
{
	HKEY hKey=CreateRegKey(hRoot,Key);
	RegSetValueEx(hKey,ValueName,0,REG_BINARY,ValueData,ValueSize);
	RegCloseKey(hKey);
}


// ������� ����� ⨯� char*
int GetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,TCHAR *ValueData,TCHAR *Default,DWORD DataSize)
{
	HKEY hKey=OpenRegKey(hRoot,Key);
	DWORD Type;
	DWORD Size = DataSize * sizeof(TCHAR);
	int ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,(LPBYTE)ValueData,&Size);
	ValueData[DataSize - 1] = 0;
	RegCloseKey(hKey);

	if(hKey==NULL || ExitCode!=ERROR_SUCCESS)
	{
		StringCchCopy(ValueData,DataSize,Default);
		return(FALSE);
	}

	return(TRUE);
}


// ������� ����� ⨯� DWORD
int GetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,int &ValueData,DWORD Default)
{
	HKEY hKey=OpenRegKey(hRoot,Key);
	DWORD Type,Size=sizeof(ValueData);
	int ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,(BYTE *)&ValueData,&Size);
	RegCloseKey(hKey);

	if(hKey==NULL || ExitCode!=ERROR_SUCCESS)
	{
		ValueData=Default;
		return(FALSE);
	}

	return(TRUE);
}


// ������� ����� ⨯� DWORD
int GetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,DWORD Default)
{
	int ValueData;
	GetRegKey(hRoot,Key,ValueName,ValueData,Default);
	return(ValueData);
}


// ������� ����� ⨯� Binary
int GetRegKey(HKEY hRoot,TCHAR *Key,TCHAR *ValueName,BYTE *ValueData,BYTE *Default,DWORD DataSize)
{
	HKEY hKey=OpenRegKey(hRoot,Key);
	DWORD Type;
	int ExitCode=RegQueryValueEx(hKey,ValueName,0,&Type,ValueData,&DataSize);
	RegCloseKey(hKey);

	if(hKey==NULL || ExitCode!=ERROR_SUCCESS)
	{
		if(Default!=NULL)
			memcpy(ValueData,Default,DataSize);
		else
			memset(ValueData,0,DataSize);

		return(FALSE);
	}

	return(TRUE);
}


// 㤠��� ����
void DeleteRegKey(HKEY hRoot,TCHAR *Key)
{
	TCHAR FullKeyName[512];
	StringCchCopy(FullKeyName,ARRAYSIZE(FullKeyName),PluginRootKey);
	StringCchCat(FullKeyName,ARRAYSIZE(FullKeyName),(Key && *Key ? _T("\\"):_T("")));
	StringCchCat(FullKeyName,ARRAYSIZE(FullKeyName),Key);
	RegDeleteKey(hRoot,FullKeyName);
}


// ᮧ���� ����
HKEY CreateRegKey(HKEY hRoot,TCHAR *Key)
{
	HKEY hKey;
	DWORD Disposition;
	TCHAR FullKeyName[512];
	StringCchCopy(FullKeyName,ARRAYSIZE(FullKeyName),PluginRootKey);
	StringCchCat(FullKeyName,ARRAYSIZE(FullKeyName),(Key && *Key ? _T("\\"):_T("")));
	StringCchCat(FullKeyName,ARRAYSIZE(FullKeyName),Key);

	if(RegCreateKeyEx(hRoot,FullKeyName,0,NULL,0,KEY_WRITE,NULL,
	                  &hKey,&Disposition) != ERROR_SUCCESS)
		hKey=NULL;

	return(hKey);
}


// ������ �������� ���� ॥���
HKEY OpenRegKey(HKEY hRoot,TCHAR *Key)
{
	HKEY hKey;
	TCHAR FullKeyName[512];
	StringCchCopy(FullKeyName,ARRAYSIZE(FullKeyName),PluginRootKey);
	StringCchCat(FullKeyName,ARRAYSIZE(FullKeyName),(Key && *Key ? _T("\\"):_T("")));
	StringCchCat(FullKeyName,ARRAYSIZE(FullKeyName),Key);

	if(RegOpenKeyEx(hRoot,FullKeyName,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
		return(NULL);

	return(hKey);
}

// �஢���� ����⢮����� ����
BOOL CheckRegKey(HKEY hRoot,TCHAR *Key)
{
	HKEY hKey=OpenRegKey(hRoot,Key);

	if(hKey!=NULL)
		RegCloseKey(hKey);

	return(hKey!=NULL);
}

#endif
