
#include "PVD2Helper.h"

const wchar_t* ms_RegKey;
HKEY mh_Key, mh_Desc;

const wchar_t* GetVersion(HMODULE hMod)
{
	static wchar_t szVersion[32];

	if (szVersion[0])
		return szVersion;

    WCHAR ModulePath[MAX_PATH*2];
    if (GetModuleFileName(hMod,ModulePath,sizeof(ModulePath)/sizeof(ModulePath[0]))) {
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(ModulePath, &dwRsrvd);
		if (dwSize>0) {
			void *pVerData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
			if (pVerData) {
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = sizeof(lvs);
				if (GetFileVersionInfo(ModulePath, 0, dwSize, pVerData)) {
					TCHAR szSlash[3]; lstrcpyW(szSlash, L"\\");
					if (VerQueryValue ((void*)pVerData, szSlash, (void**)&lvs, &nLen)) {
						wsprintfW(szVersion, L"%i.%i",
							HIWORD(lvs->dwFileVersionMS), LOWORD(lvs->dwFileVersionMS));
						if (lvs->dwFileVersionLS)
							wsprintfW(szVersion+lstrlen(szVersion), L".%i.%i",
								HIWORD(lvs->dwFileVersionLS), LOWORD(lvs->dwFileVersionLS));
					}
				}
				HeapFree(GetProcessHeap(), 0, pVerData);
			}
		}
	}

	if (!szVersion[0])
		lstrcpy(szVersion, L"Unknown");

	return szVersion;
}

PVDSettings::PVDSettings(const wchar_t* asRegKey)
{
	ms_RegKey = asRegKey; mh_Key = NULL; mh_Desc = NULL; DWORD dwDisp = 0;
	if (!RegCreateKeyExW(HKEY_CURRENT_USER, ms_RegKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &mh_Key, &dwDisp))
		RegCreateKeyExW(mh_Key, L"Description", 0, 0, 0, KEY_ALL_ACCESS, 0, &mh_Desc, &dwDisp);
}

void PVDSettings::Close()
{
	if (mh_Desc) { RegCloseKey(mh_Desc); mh_Desc = NULL; }
	if (mh_Key) { RegCloseKey(mh_Key); mh_Key = NULL; }
}

PVDSettings::~PVDSettings()
{
	Close();
}

bool PVDSettings::FindFile(const wchar_t* asFileOrPath, wchar_t* rsPath, int nMaxLen) //nMaxLen включа€ '\0'
{
	if (!asFileOrPath || !*asFileOrPath || !rsPath || nMaxLen<=1)
		return false;
	// ≈сли asFileOrPath уже содержит путь - возвращаем его
	bool lbFullPath = false;
	if (asFileOrPath[0]==L'\\' && asFileOrPath[1]==L'\\') lbFullPath = true;
	else if (asFileOrPath[1]==L':' && asFileOrPath[2]==L'\\') lbFullPath = true;
	//
	rsPath[0] = 0;
	if (lbFullPath) {
		if (lstrlen(asFileOrPath) <= nMaxLen)
			return false; // не хватило места
		lstrcpyn(rsPath, asFileOrPath, nMaxLen);
	} else {
		if (!GetPluginFolder(rsPath, nMaxLen-lstrlen(asFileOrPath)-1))
			return false;
		lstrcat(rsPath, asFileOrPath);
	}
	return true;
}

bool PVDSettings::GetPluginFolder(wchar_t* rsPath, int nMaxLen) //nMaxLen включа€ '\\\0'
{
	if (!rsPath || nMaxLen<=1)
		return false;
	DWORD nLen = GetModuleFileNameW(ghModule, rsPath, nMaxLen);
	if (nLen && nLen < (DWORD)nMaxLen) {
		wchar_t* pszSlash = wcsrchr(rsPath, L'\\');
		if (pszSlash) pszSlash[1] = 0; else rsPath[0] = 0;
	} else {
		return false;
	}
	return true;
}

//nMaxLen включа€ '\0'
void PVDSettings::GetStrParam(const wchar_t* asName, const wchar_t* asFormatDescription, const wchar_t* asDefault, wchar_t* rsValue, int nMaxLen)
{
	_ASSERTE(asName && asFormatDescription && rsValue && nMaxLen>2);
	if (!asName || !rsValue || nMaxLen < 2)
		return;
		
	DWORD nSize = 0;
	if (asFormatDescription && *asFormatDescription) {
		if (RegQueryValueEx(mh_Desc, asName, 0, 0, 0, &nSize)) {
			// —охранить в реестре описание и формат данных параметра
			RegSetValueEx(mh_Desc, asName, 0, REG_SZ, (LPBYTE)asFormatDescription, (lstrlen(asFormatDescription)+1)*sizeof(wchar_t));
		}
	}
	
	// ¬ принципе, может вернуть ERROR_MORE_DATA. Ќо счита€ данные недопустимыми - перетираем умолчанием
	if (RegQueryValueEx(mh_Key, asName, 0, 0, (LPBYTE)rsValue, &(nSize=nMaxLen*sizeof(wchar_t)))) {
		// —кинуть в реестр значение по умолчанию
		if (asDefault)
			RegSetValueEx(mh_Key, asName, 0, REG_SZ, (LPBYTE)asDefault, (lstrlen(asDefault)+1)*sizeof(wchar_t));
		if (asDefault) {
			if (rsValue != asDefault)
				lstrcpyn(rsValue, asDefault, nMaxLen);
		} else {
			rsValue[0] = 0;
		}
	}
}

void PVDSettings::SetStrParam(const wchar_t* asName, const wchar_t* asFormatDescription, 
							  const wchar_t* asValue)
{
	_ASSERTE(asName && asFormatDescription && asValue);
	if (!asValue || !asName)
		return;
		
	DWORD nSize = 0;
	if (asFormatDescription && *asFormatDescription) {
		// —охранить в реестре описание и формат данных параметра
		RegSetValueEx(mh_Desc, asName, 0, REG_SZ, (LPBYTE)asFormatDescription, (lstrlen(asFormatDescription)+1)*sizeof(wchar_t));
	}
	
	// » сохранить само значение
	RegSetValueEx(mh_Key, asName, 0, REG_SZ, (LPBYTE)asValue, (lstrlen(asValue)+1)*sizeof(wchar_t));
}

void PVDSettings::GetParam(const wchar_t* asName, const wchar_t* asFormatDescription,
						   DWORD anType, LPCVOID apDefault, LPVOID rpValue, int anSize)
{
	_ASSERTE(asName);
	if (!asName)
		return;

	DWORD nSize = 0;
	if (asFormatDescription && *asFormatDescription) {
		if (RegQueryValueEx(mh_Desc, asName, 0, 0, 0, &nSize)) {
			// —охранить в реестре описание и формат данных параметра
			RegSetValueEx(mh_Desc, asName, 0, REG_SZ, (LPBYTE)asFormatDescription, (lstrlen(asFormatDescription)+1)*sizeof(wchar_t));
		}
	}

	// ¬ принципе, может вернуть ERROR_MORE_DATA. Ќо счита€ данные недопустимыми - перетираем умолчанием
	if (RegQueryValueEx(mh_Key, asName, 0, 0, (LPBYTE)rpValue, &(nSize=anSize))) {
		// —кинуть в реестр значение по умолчанию
		_ASSERTE(apDefault);
		if (apDefault)
			RegSetValueEx(mh_Key, asName, 0, anType, (LPBYTE)apDefault, anSize);
		if (apDefault) {
			if (rpValue!=apDefault)
				memmove(rpValue, apDefault, anSize);
		} else {
			memset(rpValue, 0, anSize);
		}
	}
}

void PVDSettings::SetParam(const wchar_t* asName, const wchar_t* asFormatDescription,
						   DWORD anType, LPCVOID apValue, int anSize)
{
	_ASSERTE(asName);
	if (!asName)
		return;

	DWORD nSize = 0;
	if (asFormatDescription && *asFormatDescription) {
		// —охранить в реестре описание и формат данных параметра
		RegSetValueEx(mh_Desc, asName, 0, REG_SZ, (LPBYTE)asFormatDescription, (lstrlen(asFormatDescription)+1)*sizeof(wchar_t));
	}

	// » сохранить само значение
	RegSetValueEx(mh_Key, asName, 0, anType, (LPBYTE)apValue, anSize);
}

