
#pragma once

class MRegistryBase;

class CObjSecurity : 
	public ISecurityInformation,
	public IEffectivePermission,
	public ISecurityObjectTypeInfo
{
protected:
    ULONG                   m_cRef;
    DWORD                   m_dwSIFlags;
    PSECURITY_DESCRIPTOR    m_pOrigSD;
	SECURITY_DESCRIPTOR_CONTROL m_OrigControl;
	BOOL					mb_OrigSdDuplicated;
    LPWSTR                  m_pszServerName;
    LPWSTR                  m_pszObjectName;
	HKEY    mhk_Root;			// Для mpsz_KeyPath. May be NULL
	LPCWSTR mpsz_KeyPath;       // Путь в реестре от mhk_Root. May be NULL
	HKEY    mhk_Write;          // привилегии устанавливаются сразу в реестр
	HKEY    mhk_ParentRead;     // используется при формировании "наследуемых" прав
	LPWSTR  mpsz_KeyForInherit; // полный путь вида "CURRENT_USER\Software\Far" (возможно, с префиксом имени \\сервера)
	DWORD   mn_WowSamDesired;   // 0/KEY_WOW64_32KEY/KEY_WOW64_64KEY
	MRegistryBase* mp_Worker;

	// Changes
	//PSECURITY_DESCRIPTOR mp_OwnerSD, mp_DaclSD;
	//LPTSTR mpsz_OwnerSD, mpsz_DaclSD;
	PSECURITY_DESCRIPTOR mp_NewSD;
	SECURITY_INFORMATION m_SI;

public:
    CObjSecurity();
    virtual ~CObjSecurity();

	PSECURITY_DESCRIPTOR GetNewSD(SECURITY_INFORMATION *pSI);

    STDMETHOD(Initialize)(DWORD dwFlags,
                          PSECURITY_DESCRIPTOR pSD,
                          LPCWSTR pszServerName,  // Name of server on which SIDs will be resolved
						  DWORD sam,
						  MRegistryBase* apWorker,
	                      HKEY hkRoot,
	                      LPCWSTR pszKeyName,
	                      HKEY hkWrite, HKEY hkParentRead);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation)(
    	PSI_OBJECT_INFO pObjectInfo);
    STDMETHOD(GetSecurity)(
    	SECURITY_INFORMATION si,
        PSECURITY_DESCRIPTOR *ppSD,
        BOOL fDefault);
    STDMETHOD(SetSecurity)(
    	SECURITY_INFORMATION si,
        PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(GetAccessRights)(
    	const GUID* pguidObjectType,
        DWORD dwFlags,
        PSI_ACCESS *ppAccess,
        ULONG *pcAccesses,
        ULONG *piDefaultAccess);
    STDMETHOD(MapGeneric)(
    	const GUID *pguidObjectType,
        UCHAR *pAceFlags,
        ACCESS_MASK *pmask);
    STDMETHOD(GetInheritTypes)(
    	PSI_INHERIT_TYPE *ppInheritTypes,
		ULONG *pcInheritTypes);
    STDMETHOD(PropertySheetPageCallback)(
    	HWND hwnd,
		UINT uMsg,
		SI_PAGE_TYPE uPage);

	// *** ISecurityObjectTypeInfo methods ***
	STDMETHOD(GetInheritSource)(
		SECURITY_INFORMATION si,
		PACL pACL,
		PINHERITED_FROM *ppInheritArray);

	// *** IEffectivePermission methods ***
	STDMETHOD(GetEffectivePermission)(
		const GUID *, PSID, LPCWSTR, PSECURITY_DESCRIPTOR,
		POBJECT_TYPE_LIST *, ULONG *, PACCESS_MASK *, ULONG * );
};

extern "C"
HRESULT
CreateObjSecurityInfo(DWORD dwFlags,          // e.g. SI_EDIT_ALL | SI_ADVANCED | SI_CONTAINER
                      PSECURITY_DESCRIPTOR pSD,  // Descriptor or NULL
                      CObjSecurity **ppObjSI,
                      LPCWSTR pszServerName,  // Name of server on which SIDs will be resolved
					  DWORD sam,
					  MRegistryBase* apWorker,
                      HKEY hkRoot,
                      LPCWSTR pszKeyName,
                      HKEY hkWrite, HKEY hkParentRead);
