
/*

This code derived from Microsoft SDK samples and FilePermsBox111

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#ifndef UNICODE
#define MYLPCTSTR LPCSTR
#define _MYT(x)      x
#define UNICODE
#else
#define MYLPCTSTR LPCWSTR
#define _MYT(x)      L ## x
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <aclui.h>
#include <Aclapi.h>
#include <sddl.h>
#include <tchar.h>
#include "Privobji.h"
#include "Header.h"
//#include "resource.h"
//#include "Main.h"

#undef APPLY_SECURITY_TO_REG

#define wcsnlen(pString,maxlen) lstrlenW(pString)
#define wcscpy_s(pResult,cch,pString) lstrcpynW(pResult,pString,cch)

//LONG SetKeySecurityWindows(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD);
DWORD CreateNewDescriptor(PSECURITY_DESCRIPTOR pOldSD, // Old
						  PSECURITY_DESCRIPTOR pParentSD, // Using for inheritance (when pOldSD is `clear`)
						  SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD, // Changed
						  PSECURITY_DESCRIPTOR* ppNewSD, SECURITY_INFORMATION* pSI); // Result (concatenate)
void DumpSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION si, MYLPCTSTR asLabel);

//
// from Main.cpp
//
extern HINSTANCE g_hInstance;

//HANDLE GetClientToken( WORD wIndex );


#define INHERIT_FULL            (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

typedef enum {
    ACCESS_READ = 100,
    ACCESS_WRITE = 200
} ACCESSTYPE;

// SI_ACCESS flags
#define SI_ACCESS_SPECIFIC  0x00010000L
#define SI_ACCESS_GENERAL   0x00020000L
#define SI_ACCESS_CONTAINER 0x00040000L // general access, container-only
#define SI_ACCESS_PROPERTY  0x00080000L

// Access rights for our private objects
#define ACCESS_READ     1
#define ACCESS_MODIFY   2
#define ACCESS_DELETE   4

#define ACCESS_ALL  ACCESS_READ | ACCESS_MODIFY | ACCESS_DELETE

// define our generic mapping structure for our private objects
//
GENERIC_MAPPING ObjMap =
{
    KEY_READ,
    KEY_WRITE,
    0,
    KEY_ALL_ACCESS
};




#if defined(__GNUC__)
//#if defined(MLIBCRT)
//	#if defined(__GNUC__)
		#define PURE_CALL_NAME __cxa_pure_virtual
		#define PURE_CALL_RTYPE void
		#define PURE_CALL_RET 
//	#else
//		#define PURE_CALL_NAME _purecall
//		#define PURE_CALL_RTYPE int
//		#define PURE_CALL_RET return 0
//	#endif
//	
	#ifdef __cplusplus
	extern "C"{
	#endif
	// Pure virtual functions.
	PURE_CALL_RTYPE _cdecl PURE_CALL_NAME(void)
	{
		PURE_CALL_RET;
	}
	#ifdef __cplusplus
	};
	#endif
//#endif
#endif



//
// DESCRIPTION OF ACCESS FLAG AFFECTS
//
// SI_ACCESS_GENERAL shows up on general properties page
// SI_ACCESS_SPECIFIC shows up on advanced page
// SI_ACCESS_CONTAINER shows on general page IF object is a container
//
// The following array defines the permission names for my objects.
//
SI_ACCESS g_siObjAccesses[] =
{
	{ &GUID_NULL, 
		KEY_QUERY_VALUE|KEY_SET_VALUE|KEY_CREATE_SUB_KEY|KEY_ENUMERATE_SUB_KEYS|KEY_NOTIFY|KEY_CREATE_LINK|
		DELETE|WRITE_DAC|WRITE_OWNER|READ_CONTROL,
		L"Full Control",      SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
	{ &GUID_NULL,
		KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS|KEY_NOTIFY|READ_CONTROL,
		L"Read",      SI_ACCESS_GENERAL },
	{ &GUID_NULL, KEY_QUERY_VALUE,      L"Query Value",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, KEY_SET_VALUE,      L"Set Value",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, KEY_CREATE_SUB_KEY,      L"Create Subkey",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, KEY_ENUMERATE_SUB_KEYS,      L"Enumerate Subkeys",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, KEY_NOTIFY,      L"Notify",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, KEY_CREATE_LINK,      L"Create Link",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, DELETE,      L"Delete",      SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,      L"Write DAC",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, WRITE_OWNER,      L"Write Owner",      SI_ACCESS_SPECIFIC },
	{ &GUID_NULL, READ_CONTROL,      L"Read Control",      SI_ACCESS_SPECIFIC },
};

#define g_iObjDefAccess    1   // ACCESS_READ

//GUID gRootInheritance = { /* 06646f05-a911-4965-84b3-fc5a42644f99 */
//    0x06646f05,
//    0xa911,
//    0x4965,
//    {0x84, 0xb3, 0xfc, 0x5a, 0x42, 0x64, 0x4f, 0x99}
//  };

// The following array defines the inheritance types for my containers.
SI_INHERIT_TYPE g_siObjInheritTypes[] =
{
	{&GUID_NULL, 0,                                         L"This key only"},
	{&GUID_NULL, CONTAINER_INHERIT_ACE, 					L"This key and subkeys"},
	{&GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,	L"Subkeys only"},

	// А вот эти флажки приходят унаследованными от CURRENT_USER, только вот отдавать их нельзя :(
	//{&GUID_NULL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, L"This object, inherited object and containers"},
	//{&GUID_NULL, OBJECT_INHERIT_ACE, L"This object and inherited objects"},

	////{&GUID_NULL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERITED_ACE, L"This key and subkeys (2)"},
	////{&GUID_NULL, CONTAINER_INHERIT_ACE | INHERITED_ACE,     L"This key and subkeys (1)"},
	//{&GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE |	OBJECT_INHERIT_ACE, L"Inherited keys and subkeys"},
	//{&GUID_NULL, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE, L"Inherited subkeys"},
};

const OBJECT_TYPE_LIST g_DefaultOTL[] =
{/** This empty object list is required by GetEffectivePermission(). This structure is required by ACLUI
*	to be a static constant.
**/
	{ 0, 0, const_cast<LPGUID>(&GUID_NULL) },
};


HRESULT
LocalAllocString(LPWSTR* ppResult, LPCWSTR pString)
{
	SIZE_T cch;
	//SIZE_T maxlen = 1024;

    if (!ppResult || !pString)
        return E_INVALIDARG;

	cch = wcsnlen(pString, 1024) + 1;
    *ppResult = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));

    if (!*ppResult)
        return E_OUTOFMEMORY;

	wcscpy_s(*ppResult, (int)cch, pString);

    return S_OK;
}

void
LocalFreeString(LPWSTR* ppString)
{
    if (ppString)
    {
        if (*ppString)
            LocalFree(*ppString);
        *ppString = NULL;
    }
}


//class CObjSecurity : public ISecurityInformation
//{
//protected:
//    ULONG                   m_cRef;
//    DWORD                   m_dwSIFlags;
//    PSECURITY_DESCRIPTOR    *m_ppSD;
//    //WORD                    m_wClient;
//    HANDLE 					mh_Token;
//    LPWSTR                  m_pszServerName;
//    LPWSTR                  m_pszObjectName;
//	HKEY mhk_Root;
//	LPCWSTR mpsz_KeyPath;
//	DWORD mdw_RegFlags;
//
//public:
//    CObjSecurity() : m_cRef(1) {}
//    virtual ~CObjSecurity();
//
//    STDMETHOD(Initialize)(DWORD dwFlags,
//                          PSECURITY_DESCRIPTOR *ppSD,
//                          //WORD wClient,
//                          HANDLE hToken,
//                          LPCWSTR pszServer,
//                          LPCWSTR pszKeyName,
//						  HKEY hkRoot, LPCWSTR pszKeyPath, DWORD dwRegFlags);
//
//    // IUnknown methods
//    STDMETHOD(QueryInterface)(REFIID, LPVOID *);
//    STDMETHOD_(ULONG, AddRef)();
//    STDMETHOD_(ULONG, Release)();
//
//    // ISecurityInformation methods
//    STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo);
//    STDMETHOD(GetSecurity)(SECURITY_INFORMATION si,
//                           PSECURITY_DESCRIPTOR *ppSD,
//                           BOOL fDefault);
//    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,
//                           PSECURITY_DESCRIPTOR pSD);
//    STDMETHOD(GetAccessRights)(const GUID* pguidObjectType,
//                               DWORD dwFlags,
//                               PSI_ACCESS *ppAccess,
//                               ULONG *pcAccesses,
//                               ULONG *piDefaultAccess);
//    STDMETHOD(MapGeneric)(const GUID *pguidObjectType,
//                          UCHAR *pAceFlags,
//                          ACCESS_MASK *pmask);
//    STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
//                               ULONG *pcInheritTypes);
//    STDMETHOD(PropertySheetPageCallback)(HWND hwnd,
//                                         UINT uMsg,
//                                         SI_PAGE_TYPE uPage);
//};

///////////////////////////////////////////////////////////////////////////////
//
//  This is the entry point function called from our code that establishes
//  what the ACLUI interface is going to need to know.
//
//
///////////////////////////////////////////////////////////////////////////////

extern "C"
HRESULT
CreateObjSecurityInfo(
		DWORD dwFlags,           // e.g. SI_EDIT_ALL | SI_ADVANCED | SI_CONTAINER
		PSECURITY_DESCRIPTOR pSD,        // Pointer to security descriptor
		CObjSecurity **ppObjSI,
		LPCWSTR pszServerName,  // Name of server on which SIDs will be resolved
		DWORD sam,
		MRegistryBase* apWorker,
		HKEY hkRoot,
		LPCWSTR pszKeyName,
		HKEY hkWrite, HKEY hkParentRead)
{
    HRESULT hr;
    CObjSecurity *psi;

    *ppObjSI = NULL;

    psi = new CObjSecurity();
    if (!psi)
        return E_OUTOFMEMORY;

    hr = psi->Initialize(dwFlags, pSD, pszServerName, sam, apWorker, hkRoot, pszKeyName, hkWrite, hkParentRead);

    if (SUCCEEDED(hr))
        *ppObjSI = psi;
    else
        delete psi;

    return hr;
}


CObjSecurity::CObjSecurity()
	: m_cRef(1), m_dwSIFlags(0), m_pOrigSD(NULL)
	  ,m_OrigControl(0), mb_OrigSdDuplicated(FALSE)
	  ,m_pszServerName(NULL), m_pszObjectName(NULL)
	  ,mhk_Root(NULL), mpsz_KeyPath(NULL), mhk_Write(NULL), mpsz_KeyForInherit(NULL)
	  ,mn_WowSamDesired(0)
	  //mp_OwnerSD(NULL), mp_DaclSD(NULL), mpsz_OwnerSD(NULL), mpsz_DaclSD(NULL)
	  ,mp_NewSD(NULL), m_SI(0)
{
}

CObjSecurity::~CObjSecurity()
{
	if (m_pszServerName)
	{
		LocalFreeString(&m_pszServerName);
		m_pszServerName = NULL;
	}
	if (m_pszObjectName)
	{
		LocalFreeString(&m_pszObjectName);
		m_pszObjectName = NULL;
	}

	if (mp_NewSD)
	{
		LocalFree(mp_NewSD);
		mp_NewSD = NULL;
	}
	if (mb_OrigSdDuplicated && m_pOrigSD)
		LocalFree(m_pOrigSD);
	m_pOrigSD = NULL;
	//if (mp_OwnerSD)
	//	LocalFree(mp_OwnerSD);
	//if (mp_DaclSD)
	//	LocalFree(mp_DaclSD);
	//if (mpsz_OwnerSD)
	//	LocalFree(mpsz_OwnerSD);
	//if (mpsz_DaclSD)
	//	LocalFree(mpsz_DaclSD);
	if (mpsz_KeyForInherit)
	{
		free(mpsz_KeyForInherit);
		mpsz_KeyForInherit = NULL;
	}
}

//PSECURITY_DESCRIPTOR CObjSecurity::GetNewOwnerSD()
//{
//	if (mp_NewSD && (m_SI & OWNER_SECURITY_INFORMATION))
//		return mp_NewSD;
//	return NULL;
//	//ULONG nLen = 0, nErr = 0;
//	//if (!mp_OwnerSD && mpsz_OwnerSD)
//	//{
//	//	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(mpsz_OwnerSD, SDDL_REVISION_1, &mp_OwnerSD, &nLen))
//	//		nErr = GetLastError();
//	//}
//	//return mp_OwnerSD;
//}

PSECURITY_DESCRIPTOR CObjSecurity::GetNewSD(SECURITY_INFORMATION *pSI)
{
	if (mp_NewSD && m_SI)
	{
		*pSI = m_SI;
		return mp_NewSD;
	}
	return NULL;

	//WARNING("Эти обе функции должны возвращать ссылку на один и тот же дескриптор");
	// Который нужно формировать через BuildSecurityDescriptor - она корректно выполнит
	// builds the new security descriptor by merging the specified owner, group, access control,
	// and audit-control information with the information in this security descriptor
	// Наверное вообще не нужно со строками заморачиваться - сразу звать Build из SetSecurity.
	// Ведь в m_pSD(новая!) уже лежит наша копия дескриптора.
	// Ест-но в SetSecurity нужно проверить, что вернулось - если Absolute - звать MakeSelfRelativeSD
	
	//ULONG nLen = 0, nErr = 0;
	//if (!mp_DaclSD && mpsz_DaclSD)
	//{
	//	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(mpsz_DaclSD, SDDL_REVISION_1, &mp_DaclSD, &nLen))
	//		nErr = GetLastError();
	//}
	//return mp_DaclSD;
}

STDMETHODIMP
CObjSecurity::Initialize(
		DWORD                  dwFlags,
		PSECURITY_DESCRIPTOR   pSD,
		LPCWSTR pszServerName,
		DWORD sam,
		MRegistryBase* apWorker,
		HKEY hkRoot,
		LPCWSTR pszKeyName,
		HKEY hkWrite, HKEY hkParentRead)
{
    HRESULT hr = S_OK;

	// Обязательно сделать копию дескриптора MakeSelfRelativeSD, т.к. функция GetSecurity тупо делает memcopy
	// Но учесть, что если он уже self-relative - то звать функцию здесь не нужно - просто сразу сделать копию памяти
	DWORD dwRevision = 0, nNewLen = 0;
	if (!GetSecurityDescriptorControl(pSD, &m_OrigControl, &dwRevision))
		return HRESULT_FROM_WIN32(GetLastError());
	if (m_OrigControl & SE_SELF_RELATIVE)
	{
		m_pOrigSD = pSD;
		mb_OrigSdDuplicated = FALSE;
	}
	else
	{
		//nNewLen = 8192;
		//m_pOrigSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, nNewLen);
		if ((!::MakeSelfRelativeSD(pSD, NULL, &nNewLen) && !nNewLen) || !nNewLen)
			return HRESULT_FROM_WIN32(GetLastError());
		m_pOrigSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, nNewLen);
		if (!::MakeSelfRelativeSD(pSD, &m_pOrigSD, &nNewLen))
			return HRESULT_FROM_WIN32(GetLastError());
		mb_OrigSdDuplicated = TRUE;
	}

#ifdef _DEBUG
	PACL pNewDacl = NULL; BOOL lbDacl = FALSE, lbDaclDefaulted = FALSE;
	ULONG nDaclCount = 0; PEXPLICIT_ACCESS pDaclEntries = NULL;
	DWORD dwErr = 0;
	PTRUSTEE pOwner = NULL, pGroup = NULL;
	ULONG nSaclCount = 0; PEXPLICIT_ACCESS pSaclEntries = NULL;
	dwErr = ::GetSecurityDescriptorDacl(m_pOrigSD, &lbDacl, &pNewDacl, &lbDaclDefaulted);
	ACL_SIZE_INFORMATION size_info = {0};
	ACCESS_ALLOWED_ACE *Ace = NULL;
	BOOL lbAce = FALSE;
	if (pNewDacl)
	{
		dwErr = ::GetAclInformation(pNewDacl, &size_info, sizeof(size_info), AclSizeInformation);
		for (UINT i = 0; i < size_info.AceCount; i++)
		{
			lbAce = ::GetAce(pNewDacl, i, (void**)&Ace);
		}
	}
	dwErr = ::LookupSecurityDescriptorParts(&pOwner, &pGroup, &nDaclCount, &pDaclEntries,
		&nSaclCount, &pSaclEntries, m_pOrigSD);
	DumpSecurityDescriptor(m_pOrigSD, DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION, _MYT("CObjSecurity::Initialize"));
#endif

    m_dwSIFlags = dwFlags;
	//#ifdef _DEBUG
	//	m_dwSIFlags = SI_ADVANCED | SI_EDIT_ALL;
	//#endif

    //m_wClient = wClient;
    //mh_Token = hToken;
	mp_Worker = apWorker;
	mhk_Write = hkWrite;
	mhk_ParentRead = hkParentRead;
	mhk_Root = hkRoot;
	mpsz_KeyPath = pszKeyName;
	mn_WowSamDesired = sam;
	//TODO: А нужно ли "\\" перед именем сервера?
	hr = LocalAllocString(&m_pszServerName, pszServerName);
	LPCWSTR pszName = NULL, pszRoot = NULL;
	if (hkRoot == HKEY_CLASSES_ROOT)
		pszRoot = L"CLASSES_ROOT";
	else if (hkRoot == HKEY_CURRENT_USER)
		pszRoot = L"CURRENT_USER";
	else if (hkRoot == HKEY_LOCAL_MACHINE)
		pszRoot = L"MACHINE";
	else if (hkRoot == HKEY_USERS)
		pszRoot = L"USERS";
	else
		pszRoot = L"UNKNOWN";
	if (pszKeyName && *pszKeyName)
	{
		pszName = wcsrchr(pszKeyName, L'\\');
		if (pszName) pszName++; else pszName = pszKeyName;
	}
	else
	{
		pszName = pszRoot ? pszRoot : L"<Unknown>";
	}
	hr = LocalAllocString(&m_pszObjectName, pszName);


	BOOL lbAllowInheritLookup = (sam == 0);
	if (!lbAllowInheritLookup)
	{
		#ifdef _WIN64
		lbAllowInheritLookup = (sam & KEY_WOW64_64KEY) == KEY_WOW64_64KEY;
		#else
		lbAllowInheritLookup = (sam & KEY_WOW64_32KEY) == KEY_WOW64_32KEY;
		#endif
	}
	// Если наследование проверить можно - создаем полный путь
	if (lbAllowInheritLookup)
	{
		int nFullLen = 10
			+ (pszServerName ? lstrlenW(pszServerName) : 0)
			+ (lstrlenW(pszRoot))
			+ (pszKeyName ? lstrlenW(pszKeyName) : 0);
		mpsz_KeyForInherit = (wchar_t*)calloc(nFullLen,sizeof(wchar_t));
		if (pszServerName && *pszServerName)
		{
			while (*pszServerName == L'\\' || *pszServerName == L'/') pszServerName++;
			lstrcatW(mpsz_KeyForInherit, L"\\\\");
			lstrcatW(mpsz_KeyForInherit, pszServerName);
			lstrcatW(mpsz_KeyForInherit, L"\\");
		}
		lstrcatW(mpsz_KeyForInherit, pszRoot);
		if (pszKeyName && *pszKeyName)
		{
			lstrcatW(mpsz_KeyForInherit, L"\\");
			lstrcatW(mpsz_KeyForInherit, pszKeyName);
		}
	}
	

	// Result
	//mp_OwnerSD = mp_DaclSD = NULL;
	mp_NewSD = NULL;
	m_SI = 0;


    return hr;
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CObjSecurity::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CObjSecurity::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CObjSecurity::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
	if (::IsEqualIID(riid, IID_IUnknown))
	{
		*ppv = this;
		AddRef();
		return S_OK;
	}
	if(::IsEqualIID(riid, IID_ISecurityInformation))
	{
		*ppv = dynamic_cast<ISecurityInformation *>(this);
		AddRef();
		return S_OK;
	}
	if (::IsEqualIID(riid, IID_IEffectivePermission))
	{
		*ppv = dynamic_cast<IEffectivePermission *>(this);
		AddRef();
		return S_OK;
	}
	if( ::IsEqualIID(riid, IID_ISecurityObjectTypeInfo))
	{
		*ppv = dynamic_cast<ISecurityObjectTypeInfo *>(this);
		AddRef();
		return S_OK;
	}

	//if (IsEqualIID(riid, IID_IUnknown) 
	//	|| IsEqualIID(riid, IID_ISecurityInformation)
	//	//|| (mpsz_KeyForInherit && IsEqualIID(riid, IID_ISecurityObjectTypeInfo))
	//	)
	//{
	//    *ppv = (LPSECURITYINFO)this;
	//    m_cRef++;
	//    return S_OK;
	//}

    *ppv = NULL;
    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CObjSecurity::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    pObjectInfo->dwFlags = m_dwSIFlags;
    pObjectInfo->hInstance = g_hInstance;
    pObjectInfo->pszServerName = m_pszServerName;
    pObjectInfo->pszObjectName = m_pszObjectName;

    return S_OK;
}

STDMETHODIMP
CObjSecurity::GetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault)
{
    DWORD dwLength = 0;
    DWORD dwErr = 0;

    *ppSD = NULL;

    if (fDefault)
        return E_NOTIMPL;

#ifdef _DEBUG
	TCHAR szDbgInfo[128];
	wsprintf(szDbgInfo, _T("CObjSecurity::GetSecurity(0x%08X,%i)\n"), si, fDefault);
	OutputDebugString(szDbgInfo);
#endif

	if (mpsz_KeyForInherit)
	{
		dwErr = GetNamedSecurityInfo(mpsz_KeyForInherit, SE_REGISTRY_KEY,
			si, NULL, NULL, NULL, NULL, ppSD);
	}
	else
	{
		// Просто отдадим копию
		dwLength = GetSecurityDescriptorLength(m_pOrigSD);
		if (dwLength)
		{
			*ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwLength);
			if (*ppSD)
				memmove(*ppSD, m_pOrigSD, dwLength);
		}
	}

    ////
    //// Assume that required privileges have already been enabled
    ////
    //GetPrivateObjectSecurity(*m_ppSD, si, NULL, 0, &dwLength);
    //if (dwLength)
    //{
    //    *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwLength);
    //    if (*ppSD &&
    //        !GetPrivateObjectSecurity(*m_ppSD, si, *ppSD, dwLength, &dwLength))
    //    {
    //        dwErr = GetLastError();
    //        LocalFree(*ppSD);
    //        *ppSD = NULL;
    //    }
    //}
    //else
    //    dwErr = GetLastError();


    return HRESULT_FROM_WIN32(dwErr);
}

STDMETHODIMP
CObjSecurity::SetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD)
{
	if(!::IsValidSecurityDescriptor(pSD))
	{
		return E_POINTER;
	}

    DWORD dwErr = 0;
	PSECURITY_DESCRIPTOR pParentSD = NULL;

	if (mp_NewSD)
	{
		LocalFree(mp_NewSD);
		mp_NewSD = NULL;
	}

	if (mhk_ParentRead != NULL)
	{
		dwErr = mp_Worker->GetKeySecurity(mhk_ParentRead, DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION, &pParentSD);
	}
	
	dwErr = CreateNewDescriptor(m_pOrigSD, // Old
					pParentSD, // Used for inheritance calculation, may be NULL
					si, pSD, // Changed (it must be merged with Old)
					&mp_NewSD, &m_SI); // Resultant
	
	if ((dwErr == 0) && (mhk_Write != NULL))
	{
		//dwErr = SetKeySecurityWindows(mhk_Write, m_SI, mp_NewSD);
		dwErr = mp_Worker->SetKeySecurity(mhk_Write, m_SI, mp_NewSD);
		
		// сразу чистим дескриптор, независимо от того "успешно или нет"
		LocalFree(mp_NewSD);
		mp_NewSD = NULL;
	}

	if (pParentSD)
		LocalFree(pParentSD);
						  
	return HRESULT_FROM_WIN32(dwErr);
	
#if 0 
	TCHAR szDbgLabel[64];
	wsprintf(szDbgLabel, _T("CObjSecurity::SetSecurity(0x%X)"), si);
    DumpSecurityDescriptor(pSD, si, szDbgLabel);

	PSID pNewOwner = NULL; BOOL lbOwnerDefaulted = FALSE;
	TRUSTEE *ptNewOwner = NULL, NewOwner = {NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_UNKNOWN};
	if (!dwErr && (si & OWNER_SECURITY_INFORMATION))
	{
		if (::GetSecurityDescriptorOwner(pSD, &pNewOwner, &lbOwnerDefaulted))
		{
			NewOwner.ptstrName = (LPTSTR)pNewOwner;
			ptNewOwner = &NewOwner;
		}
		else
			dwErr = GetLastError();
	}
	PSID pNewGroup = NULL; BOOL lbGroupDefaulted = FALSE;
	TRUSTEE *ptNewGroup = NULL, NewGroup = {NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_UNKNOWN};
	if (!dwErr && (si & GROUP_SECURITY_INFORMATION))
	{
		if (::GetSecurityDescriptorGroup(pSD, &pNewGroup, &lbGroupDefaulted))
		{
			NewGroup.ptstrName = (LPTSTR)pNewGroup;
			ptNewGroup = &NewGroup;
		}
		else
			dwErr = GetLastError();
	}
	PACL pNewDacl = NULL; BOOL lbDacl = FALSE, lbDaclDefaulted = FALSE;
	ULONG nDaclCount = 0; PEXPLICIT_ACCESS pDaclEntries = NULL;
	if (!dwErr && (si & DACL_SECURITY_INFORMATION))
	{
		if (::GetSecurityDescriptorDacl(pSD, &lbDacl, &pNewDacl, &lbDaclDefaulted))
		{
			dwErr = ::GetExplicitEntriesFromAcl(pNewDacl, &nDaclCount, &pDaclEntries);
		}
		else
			dwErr = GetLastError();
	}
	PACL pNewSacl = NULL; BOOL lbSacl = FALSE, lbSaclDefaulted = FALSE;
	ULONG nSaclCount = 0; PEXPLICIT_ACCESS pSaclEntries = NULL;
	if (!dwErr && (si & SACL_SECURITY_INFORMATION))
	{
		if (::GetSecurityDescriptorSacl(pSD, &lbSacl, &pNewSacl, &lbSaclDefaulted))
		{
			dwErr = ::GetExplicitEntriesFromAcl(pNewSacl, &nSaclCount, &pSaclEntries);
		}
		else
			dwErr = GetLastError();
	}

	PSECURITY_DESCRIPTOR pNewSD = NULL; ULONG nNewSize = 0;
	BOOL lbRc = FALSE;
	
	if (dwErr == 0)
		dwErr = BuildSecurityDescriptor(ptNewOwner, ptNewGroup, nDaclCount, pDaclEntries, nSaclCount, pSaclEntries,
			m_pOrigSD/*self-relative!*/, &nNewSize, &pNewSD);
	if (dwErr == 0)
	{
		DumpSecurityDescriptor(pNewSD, OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION, _T("After BuildSecurityDescriptor"));
		if (pNewOwner)
		{
			if (!::SetSecurityDescriptorOwner(pNewSD, pNewOwner, lbOwnerDefaulted))
				dwErr = GetLastError();
			else
				DumpSecurityDescriptor(pNewSD, OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION, _T("After SetSecurityDescriptorOwner"));
		}
		if (dwErr != 0)
			LocalFree(pNewSD);
		else
		{
			if (mp_NewSD)
				LocalFree(mp_NewSD);
			mp_NewSD = pNewSD;
			m_SI = si;
		}
	}
	else
		dwErr = GetLastError();

	//BOOL lb1 = IsValidSecurityDescriptor((PSECURITY_DESCRIPTOR)m_ppSD);
	//BOOL lb2 = IsValidSecurityDescriptor(*m_ppSD);
	//DumpSecurityDescriptor(*m_ppSD, OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, _T("Stored descriptor pointer"));

	//WARNING("BuildSecurityDescriptor");
	//// она корректно выполнит
	//// builds the new security descriptor by merging the specified owner, group, access control,
	//// and audit-control information with the information in this security descriptor
	//// Наверное вообще не нужно со строками заморачиваться - сразу звать Build из SetSecurity.
	//// Ведь в m_pSD(новая!) уже лежит наша копия дескриптора.
	//// Ест-но в SetSecurity нужно проверить, что вернулось - если Absolute - звать MakeSelfRelativeSD
	//WARNING("Сделать OR на новую переменную масок - что юзер поменял");
	//
	//if (si & DACL_SECURITY_INFORMATION)
	//{
	//	if (mp_DaclSD)
	//	{
	//		LocalFree(mp_DaclSD);
	//		mp_DaclSD = NULL;
	//	}
	//	if (mpsz_DaclSD)
	//	{
	//		LocalFree(mpsz_DaclSD);
	//		mpsz_DaclSD = NULL;
	//	}

	//	ULONG nLen = 0;
	//	if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSD, SDDL_REVISION_1, DACL_SECURITY_INFORMATION,
	//			&mpsz_DaclSD, &nLen))
	//		dwErr = GetLastError();
	//}

	//if (si & OWNER_SECURITY_INFORMATION)
	//{
	//	if (mp_OwnerSD)
	//	{
	//		LocalFree(mp_OwnerSD);
	//		mp_OwnerSD = NULL;
	//	}
	//	if (mpsz_OwnerSD)
	//	{
	//		LocalFree(mpsz_OwnerSD);
	//		mpsz_OwnerSD = NULL;
	//	}

	//	ULONG nLen = 0;
	//	if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSD, SDDL_REVISION_1, OWNER_SECURITY_INFORMATION,
	//			&mpsz_OwnerSD, &nLen))
	//		dwErr = GetLastError();

	//	//DWORD nLen = GetSecurityDescriptorLength(pSD);
	//	//if (nLen)
	//	//{
	//	//	mp_OwnerSD = LocalAlloc(LPTR, nLen);
	//	//	if (mp_OwnerSD)
	//	//		memmove(mp_OwnerSD, pSD, nLen);
	//	//}
	//	//else
	//	//{
	//	//	mp_OwnerSD = NULL;
	//	//}
	//	//PSID pOwner = NULL; BOOL lbOwnerDefaulted = FALSE;
	//	//if (GetSecurityDescriptorOwner(pSD, &pOwner, &lbOwnerDefaulted))
	//	//{
	//	//	if (SetSecurityDescriptorOwner(*m_ppSD, pOwner, lbOwnerDefaulted))
	//	//	{
	//	//		DumpSecurityDescriptor(pSD, si, _T("CObjSecurity::SetSecurity Applied"));
	//	//	}
	//	//	else
	//	//	{
	//	//		dwErr = GetLastError();
	//	//	}
	//	//}
	//	//else
	//	//{
	//	//	dwErr = GetLastError();
	//	//}
	//}

#ifdef APPLY_SECURITY_TO_REG
    //
    // Assume that required privileges have already been enabled
    //
    //if (!SetPrivateObjectSecurity(si, pSD, m_ppSD, &ObjMap, mh_Token))
    //    dwErr = GetLastError();

	if (mpsz_KeyPath && *mpsz_KeyPath)
	{
		HKEY hk;
		dwErr = RegOpenKeyExW(mhk_Root, mpsz_KeyPath, 0, mdw_RegFlags, &hk);
		if (dwErr == 0)
		{
			dwErr = RegSetKeySecurity(hk, DACL_SECURITY_INFORMATION, pSD);
			RegCloseKey(hk);
		}
	}
	else
	{
		dwErr = RegSetKeySecurity(mhk_Root, DACL_SECURITY_INFORMATION, pSD);
	}

	if (dwErr == 0)
	{
	    DWORD dwLength = 0;
	    //
	    // Assume that required privileges have already been enabled
	    //
	    GetPrivateObjectSecurity(pSD, DACL_SECURITY_INFORMATION, NULL, 0, &dwLength);
	    if (dwLength)
	    {
	    	PSECURITY_DESCRIPTOR pNewSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwLength);
	        if (pNewSD &&
	            !GetPrivateObjectSecurity(pSD, DACL_SECURITY_INFORMATION, pNewSD, dwLength, &dwLength))
	        {
	            dwErr = GetLastError();
	            LocalFree(pNewSD);
	            pNewSD = NULL;
	        }
	        else
	        {
	        	LocalFree((HLOCAL)*m_ppSD);
	        	*m_ppSD = pNewSD;
	        }
	    }
	    else
	        dwErr = GetLastError();
	}
#endif
	
    return HRESULT_FROM_WIN32(dwErr);
#endif
}

STDMETHODIMP
CObjSecurity::GetAccessRights(const GUID* /*pguidObjectType*/,
                               DWORD /*dwFlags*/,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    *ppAccesses = g_siObjAccesses;
    *pcAccesses = sizeof(g_siObjAccesses)/sizeof(g_siObjAccesses[0]);
    *piDefaultAccess = g_iObjDefAccess;

    return S_OK;
}

STDMETHODIMP
CObjSecurity::MapGeneric(const GUID* /*pguidObjectType*/,
                         UCHAR * /*pAceFlags*/,
                         ACCESS_MASK *pmask)
{
    MapGenericMask(pmask, &ObjMap);

    return S_OK;
}

STDMETHODIMP
CObjSecurity::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
    *ppInheritTypes = g_siObjInheritTypes;
    *pcInheritTypes = sizeof(g_siObjInheritTypes)/sizeof(g_siObjInheritTypes[0]);

    return S_OK;
}

STDMETHODIMP
CObjSecurity::PropertySheetPageCallback(HWND /*hwnd*/,
                                         UINT /*uMsg*/,
                                         SI_PAGE_TYPE /*uPage*/)
{
    return S_OK;
}

STDMETHODIMP
CObjSecurity::GetInheritSource(SECURITY_INFORMATION psi, PACL pAcl, PINHERITED_FROM *ppInheritArray)
{
	DWORD dwErr = 0;
	size_t i = 0, dwSize = 0;
	PINHERITED_FROM InheritResult = NULL;

	if (mpsz_KeyForInherit == NULL)
	{
		LPCTSTR pszUnknown = _T("<unknown>");
		dwSize = lstrlen(pszUnknown) + 1;
		InheritResult = (PINHERITED_FROM)LocalAlloc(LPTR, 
			(1 + pAcl->AceCount) * sizeof(INHERITED_FROM) + dwSize * sizeof(TCHAR));
		TCHAR* DataPtr = (TCHAR*)(((LPBYTE)InheritResult) + (pAcl->AceCount * sizeof(INHERITED_FROM)));
		lstrcpy(DataPtr, pszUnknown);
		for(i = 0 ; i < pAcl->AceCount ; i++)
		{
			InheritResult[i].GenerationGap = 1;
			InheritResult[i].AncestorName = DataPtr;
		}
	}
	else
	{
		PINHERITED_FROM InheritTmp = (PINHERITED_FROM)LocalAlloc(LPTR, (1 + pAcl->AceCount) * sizeof(INHERITED_FROM));

		BOOL Container = TRUE;

		dwErr = GetInheritanceSource(mpsz_KeyForInherit, SE_REGISTRY_KEY, psi,
			Container, NULL, 0, pAcl, NULL, &ObjMap, InheritTmp);

		if (dwErr == 0)
		{
			for(i = 0 ; i < pAcl->AceCount ; i++)
			{
				LPCTSTR pszAncName = InheritTmp[i].AncestorName ? InheritTmp[i].AncestorName : _T("<not inherited>");
				dwSize += 1 + lstrlen(pszAncName);
			}

			InheritResult = (PINHERITED_FROM)LocalAlloc(LPTR, 
				(1 + pAcl->AceCount) * sizeof(INHERITED_FROM) + dwSize * sizeof(TCHAR));

			TCHAR* DataPtr = (TCHAR*)(((LPBYTE)InheritResult) + (pAcl->AceCount * sizeof(INHERITED_FROM)));

			for(i = 0 ; i < pAcl->AceCount ; i++)
			{
				LPCTSTR pszAncName = InheritTmp[i].AncestorName ? InheritTmp[i].AncestorName : _T("<not inherited>");
				lstrcpy(DataPtr, pszAncName);
				
				InheritResult[i].GenerationGap = InheritTmp[i].GenerationGap;
				InheritResult[i].AncestorName = DataPtr;

				DataPtr += lstrlen(DataPtr)+1;
			}

			FreeInheritedFromArray(InheritTmp, pAcl->AceCount, NULL);
		}
		LocalFree(InheritTmp); InheritTmp = NULL;
	}

	*ppInheritArray = InheritResult;
	return HRESULT_FROM_WIN32(dwErr);
}

STDMETHODIMP
CObjSecurity::GetEffectivePermission(const GUID * /*pguidObjectType*/, PSID pUserSid, LPCWSTR /*pszServerName*/,
	PSECURITY_DESCRIPTOR pSD, POBJECT_TYPE_LIST* ppObjectTypeList, ULONG* pcObjectTypeListLength,
	PACCESS_MASK* ppGrantedAccessList, ULONG* pcGrantedAccessListLength)
{/** This method is called by GetEffectivePermission when the user uses ACLUI's frontend for GetEffectiveRightsFromAcl().
*	We are supplied the User SID, the server for that SID, and the SD. We must fill the object list and the
*	grantedAccessList (and their sizes).
**/
	DWORD dwErr = 0;
	BOOL AclPresent = FALSE, AclDefaulted = FALSE;
	PACCESS_MASK AccessRights = NULL;
	PACL Dacl = NULL;

	*ppObjectTypeList = const_cast<OBJECT_TYPE_LIST *>(g_DefaultOTL);
	*pcObjectTypeListLength = 1;
	::SetLastError(ERROR_SUCCESS);

	/* Get the Acl from the SD given */
	dwErr = ::GetSecurityDescriptorDacl(pSD, &AclPresent, &Dacl, &AclDefaulted);
	if(Dacl != NULL && dwErr == TRUE)
	{/* Make a trustee from the SID given */
		TRUSTEE Trustee = {0};
		::BuildTrusteeWithSid(&Trustee, pUserSid);

		/* Copy AccessRights to a LocalAlloc ptr. ACLUI will free it. */
		AccessRights = reinterpret_cast<PACCESS_MASK>(::LocalAlloc(LPTR, sizeof(PACCESS_MASK) + sizeof(ACCESS_MASK)));
		if(AccessRights == NULL)
		{
			return E_OUTOFMEMORY;
		}
		dwErr = ::GetEffectiveRightsFromAcl(Dacl, &Trustee, AccessRights);
		if(dwErr != ERROR_SUCCESS)
		{/* And call the required API. */
			::LocalFree(AccessRights); AccessRights = NULL;
			return HRESULT_FROM_WIN32(dwErr);
		}
		*ppGrantedAccessList = AccessRights;
		*pcGrantedAccessListLength = 1;
		return S_OK;
	}
	return HRESULT_FROM_WIN32(dwErr);
}
