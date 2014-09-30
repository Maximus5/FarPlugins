
#include "header.h"
#include "resource.h"
#include <Aclui.h>
#include <Aclapi.h>
#include <Sddl.h>
#include <shobjidl.h>
#include "Privobji.h"
#include "TokenHelper.h"

extern HINSTANCE g_hInstance;

// ViewOnly - запретить редактирование в релизе
#ifdef _DEBUG
	#define ALLOW_MODIFY_ACL
#else
	#undef ALLOW_MODIFY_ACL
#endif


void DumpSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION si, LPCTSTR asLabel)
{
#ifdef _DEBUG
	LPTSTR StringSecurityDescriptor = NULL;
	ULONG StringSecurityDescriptorLen = 0;

	if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSecurityDescriptor, SDDL_REVISION_1, si, &StringSecurityDescriptor, &StringSecurityDescriptorLen))
	{
		TCHAR szDbg[128];
		wsprintf(szDbg, _T("### ConvertSecurityDescriptorToStringSecurityDescriptor FAILED, ErrCode=0x%08X!\n"), GetLastError());
		OutputDebugString(szDbg);
	}
	else
	{
		TCHAR* pszDbg = (TCHAR*)malloc((StringSecurityDescriptorLen+256)*sizeof(TCHAR));
		wsprintf(pszDbg, _T("=== Dumping security descriptor (%s) ===\n%s\n==================================\n"),
			asLabel,
			StringSecurityDescriptor);
		OutputDebugString(pszDbg);


		PSECURITY_DESCRIPTOR pNew = NULL;
		ULONG nNewLen = 0, nCvtError = 0;
		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
				SDDL_REVISION_1, &pNew, &nNewLen))
			nCvtError = GetLastError();
		else if (pNew)
			LocalFree(pNew);

		free(pszDbg);
	}
	
	if (StringSecurityDescriptor)
		LocalFree((HLOCAL)StringSecurityDescriptor);
#endif
}

void AppendSid(LPCTSTR asStart, LPCTSTR asEnd, TCHAR* &psz, int nMaxLen, LPCTSTR asSidPrefix)
{
	TCHAR sSID[128], sDomain[MAX_PATH], sLogin[MAX_PATH]; sDomain[0] = sLogin[0] = 0;
	PSID pSID = NULL;
	DWORD cchDomain = ARRAYSIZE(sDomain), cchLogin = ARRAYSIZE(sLogin);
	LPCTSTR pszSid, pszSidEnd = NULL;
	SID_NAME_USE Use = SidTypeUser;

	if (!asEnd)
		asEnd = asStart + lstrlen(asStart);

	pszSid = asEnd-1;
	while (pszSid > asStart)
	{
		if (*pszSid == _T(')'))
		{
			if (pszSidEnd)
				return; // invalid
			pszSidEnd = pszSid;
		}
		else if (*pszSid == _T('('))
		{
			return; // invalid
		}
		else if (*pszSid == _T(';'))
		{
			pszSid++;
			break; // OK
		}
		pszSid--;
	}
	if (!pszSidEnd || (pszSid <= asStart) || ((pszSidEnd - pszSid) >= countof(sSID)))
		return; // invalid

	//go
	memmove(sSID, pszSid, (pszSidEnd-pszSid)*sizeof(TCHAR));
	sSID[pszSidEnd-pszSid] = 0;

	//ConvertStringSidToSid
	if (!ConvertStringSidToSid(sSID, &pSID))
		return; // Unknown SID
	//LookupAccountSid
	if (LookupAccountSid(NULL, pSID, sLogin, &cchLogin, sDomain, &cchDomain, &Use)
		&& *sLogin)
	{
		int nSidPrefix = asSidPrefix ? lstrlen(asSidPrefix) : 0;
		if (nSidPrefix)
		{
			memmove(psz, asSidPrefix, nSidPrefix*sizeof(TCHAR));
			psz += nSidPrefix;
		}

		int nLen, nMax = (nMaxLen - nSidPrefix) - 1;

		if (*sDomain && *sLogin)
		{
			nMax = ((nMaxLen - nSidPrefix) >> 1) - 2;
			nLen = lstrlen(sDomain);
			if (nLen > nMax) nLen = nMax;
			memmove(psz, sDomain, nLen*sizeof(TCHAR));
			psz += nLen;
			*(psz++) = _T('\\');
		}

		nLen = lstrlen(sLogin);
		if (nLen > nMax) nLen = nMax;
		memmove(psz, sLogin, nLen*sizeof(TCHAR));
		psz += nLen;
		*psz = 0;
	}
	LocalFree(pSID);
}

// Память под pStringSD выделяется нашими malloc, а не LocalAlloc
// LPTSTR asPrefix зарезервирован под выгрузку в reg - каджкую строку комментарить
// Рекомендованный префикс для выгрузки в reg-файл - ";; "
LPTSTR SD2StringSD(PSECURITY_DESCRIPTOR pSD, SECURITY_INFORMATION si, LPTSTR asPrefix = NULL, BOOL abFormatACL = TRUE, BOOL abSidNames = TRUE)
{
	BOOL lbRc = TRUE;
	DWORD dwErr = 0;
	LPTSTR pszOwner = NULL, pszDacl = NULL, pszSacl = NULL, pszOther = NULL;
	LPTSTR pSSD = NULL; // Result
	
	#define CSD2SSD ConvertSecurityDescriptorToStringSecurityDescriptor
	
	if (lbRc && (si & OWNER_SECURITY_INFORMATION))
	{
		si &= ~OWNER_SECURITY_INFORMATION;
		if (!CSD2SSD(pSD, SDDL_REVISION_1, OWNER_SECURITY_INFORMATION, &pszOwner, NULL))
		{
			lbRc = FALSE; dwErr = GetLastError();
		}
	}
	if (lbRc && (si & DACL_SECURITY_INFORMATION))
	{
		si &= ~DACL_SECURITY_INFORMATION;
		if (!CSD2SSD(pSD, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &pszDacl, NULL))
		{
			lbRc = FALSE; dwErr = GetLastError();
		}
	}
	if (lbRc && (si & SACL_SECURITY_INFORMATION))
	{
		si &= ~SACL_SECURITY_INFORMATION;
		if (!CSD2SSD(pSD, SDDL_REVISION_1, SACL_SECURITY_INFORMATION, &pszSacl, NULL))
		{
			lbRc = FALSE; dwErr = GetLastError();
		}
	}
	if (lbRc && si)
	{
		if (!CSD2SSD(pSD, SDDL_REVISION_1, si, &pszOther, NULL))
		{
			lbRc = FALSE; dwErr = GetLastError();
		}
	}
	
	#undef CSD2SSD
	
	
	// Если все успешно - слепить удобочитаемый дескриптор
	if (lbRc)
	{
		int nPrefix = asPrefix ? lstrlen(asPrefix) : 0;
		LPCTSTR pszSuffix = _T("\r\n"); int nSuffix = lstrlen(pszSuffix);
		LPCTSTR pszBrPrefix = _T("  "); int nBrPrefix = lstrlen(pszBrPrefix);
		LPCTSTR pszAccPrefix = _T(" // "); int nAccPrefix = lstrlen(pszAccPrefix); // визуализация SID
		int nAllLen = 0; // не забыть про \0
		//int nOwner = 0, nDacl = 0, nSacl = 0, nOther = 0;
		struct {
			LPTSTR psz; int nLen;
		} Strings[] = {
			{pszOwner}, {pszDacl}, {pszSacl}, {pszOther}
		};
	
		TCHAR *pszBr1, *pszBr2;
		for (int i = 0; i < ARRAYSIZE(Strings); i++)
		{
			if (Strings[i].psz)
			{
				Strings[i].nLen = lstrlen(Strings[i].psz);
				nAllLen += Strings[i].nLen+nSuffix+nPrefix;
				// Для удобочитаемости - каждый AclEntry на новую строку с отступом 2 пробела
				if (abFormatACL && (pszBr1 = _tcschr(Strings[i].psz, _T('('))) != NULL)
				{
					while (pszBr1)
					{
						// 128 - на имя аккаунта
						nAllLen += nSuffix+nPrefix+nBrPrefix+128;
						pszBr1 = _tcschr(pszBr1+1, _T('('));
					}
				}
			}
		}
		//if (pszOwner)
		//	nAllLen += (nOwner = lstrlen(pszOwner)+nSuffix+nPrefix);
		//if (pszDacl)
		//	nAllLen += (nDacl = lstrlen(pszDacl)+nSuffix+nPrefix);
		//if (pszSacl)
		//	nAllLen += (nSacl = lstrlen(pszSacl)+nSuffix+nPrefix);
		//if (pszOther)
		//	nAllLen += (nOther = lstrlen(pszOther)+nSuffix+nPrefix);
		
		// Формируем
		pSSD = (TCHAR*)malloc((nAllLen+1)*sizeof(TCHAR));
		
		if (!pSSD)
		{
			lbRc = FALSE;
			dwErr = ERROR_NOT_ENOUGH_MEMORY;
		}
		else
		{
			*pSSD = 0;
			TCHAR *psz = pSSD;
			// ConvertStringSidToSid, LookupAccountSid
			//TCHAR sSID[MAX_PATH], sDomain[MAX_PATH], sLogin[MAX_PATH], pszSid;
			//PSID pSID = NULL;
			//DWORD cchDomain, cchLogin;

			// go
			for (int i = 0; i < ARRAYSIZE(Strings); i++)
			{
				if (Strings[i].psz)
				{	
					if (nPrefix)
					{
						lstrcpy(psz, asPrefix);
						psz += nPrefix;
					}
					pszBr1 = abFormatACL ? _tcschr(Strings[i].psz, _T('(')) : NULL;
					pszBr2 = pszBr1 ? _tcschr(pszBr1+1, _T('(')) : NULL;
					// Для удобочитаемости - каждый AclEntry на новую строку с отступом 2 пробела
					if (pszBr1)
					{
						size_t nBrLen = pszBr2 ? (pszBr2-Strings[i].psz) : lstrlen(Strings[i].psz);
						memmove(psz, Strings[i].psz, nBrLen*sizeof(TCHAR));
						psz += nBrLen;
						AppendSid(Strings[i].psz, pszBr2, psz, 128, pszAccPrefix);
						lstrcpy(psz, pszSuffix);
						psz += nSuffix;
						while (pszBr2)
						{
							pszBr1 = pszBr2;
							pszBr2 = _tcschr(pszBr1+1, _T('('));

							if (nPrefix)
							{
								lstrcpy(psz, asPrefix);
								psz += nPrefix;
							}
							memmove(psz, pszBrPrefix, nBrPrefix*sizeof(TCHAR));
							psz += nBrPrefix;
							nBrLen = pszBr2 ? (pszBr2 - pszBr1) : lstrlen(pszBr1);
							memmove(psz, pszBr1, nBrLen*sizeof(TCHAR));
							psz += nBrLen;
							AppendSid(pszBr1, pszBr2, psz, 128, pszAccPrefix);
							lstrcpy(psz, pszSuffix);
							psz += nSuffix;
						}
					}
					else
					{
						lstrcpy(psz, Strings[i].psz);
						psz += Strings[i].nLen;
						lstrcpy(psz, pszSuffix);
						psz += nSuffix;
					}
				}
			}

			_ASSERTE((psz - pSSD) <= nAllLen);
			
			// Последний \r\n нам не нужен?
			//if (psz > pSSD) *(psz-nSuffix) = 0;
		}
	}
	
	// Освободить не наше
	if (pszOwner)
		LocalFree(pszOwner);
	if (pszDacl)
		LocalFree(pszDacl);
	if (pszSacl)
		LocalFree(pszSacl);
	if (pszOther)
		LocalFree(pszOther);
	
	if (!lbRc)
	{
		SetLastError(dwErr);
		_ASSERTE(pSSD == NULL);
		return NULL;
	}
	_ASSERTE(pSSD != NULL);
	return pSSD;
}

// Рекомендованный префикс для выгрузки в reg-файл - ";; "
PSECURITY_DESCRIPTOR StringSD2SD(LPCTSTR pSSD, PULONG pnSize = NULL, LPTSTR asPrefix = NULL)
{
	PSECURITY_DESCRIPTOR pSD = NULL;
	DWORD nErr = 0;
	BOOL lbRc = FALSE;
	TCHAR* pszCopy = NULL;
	
	#define CSSD2SD ConvertStringSecurityDescriptorToSecurityDescriptor
	lbRc = CSSD2SD(pSSD, SDDL_REVISION_1, &pSD, pnSize/*may be NULL*/);
	if (!lbRc)
	{
		nErr = GetLastError();
		LPCTSTR pszBrkChars = _T("\r\n\t /");
		if (_tcspbrk(pSSD, pszBrkChars))
		{
			// API обламывается на \r\n
			int nLen = lstrlen(pSSD);
			int nPrefixLen = asPrefix ? lstrlen(asPrefix) : 0;
			pszCopy = (TCHAR*)malloc((nLen+1)*sizeof(TCHAR));
			if (pszCopy)
			{
				TCHAR* pszDst = pszCopy;
				LPCTSTR pszSrc = pSSD;
				LPCTSTR pszBrk;
				if (nPrefixLen)
					if (memcmp(pszSrc, asPrefix, nPrefixLen*sizeof(TCHAR)) == 0)
						pszSrc += nPrefixLen;
				while ((pszBrk = _tcspbrk(pszSrc, pszBrkChars)) != 0)
				{
					if (pszBrk > pszSrc)
					{
						memmove(pszDst, pszSrc, (pszBrk-pszSrc)*sizeof(*pszSrc));
						pszDst += (pszBrk-pszSrc);
					}

					while (*pszBrk && _tcschr(pszBrkChars, *pszBrk))
					{
						if (*pszBrk == _T('/'))
						{
							pszBrk = _tcspbrk(pszBrk, _T("\r\n"));
							if (!pszBrk)
							{
								pszSrc = NULL;
								break;
							}
						}
						pszBrk++;
					}
					pszSrc = pszBrk;
					if (nPrefixLen)
						if (memcmp(pszSrc, asPrefix, nPrefixLen*sizeof(TCHAR)) == 0)
							pszSrc += nPrefixLen;
				}
				if (pszSrc && *pszSrc)
					lstrcpy(pszDst, pszSrc);
				else
					*pszDst = 0;
				
				
				// Пробуем еще раз
				lbRc = CSSD2SD(pszCopy, SDDL_REVISION_1, &pSD, pnSize/*may be NULL*/);

				free(pszCopy);
			}
		}
	}
	#undef CSSD2SD

	if (!pSD)
	{
		if (!nErr) nErr = E_UNEXPECTED;
		SetLastError(nErr);
	}
	return pSD;	
}

// Освобождать память - не нужно
DWORD GetSecurityDescriptorTrustee(PSECURITY_DESCRIPTOR pSD, SECURITY_INFORMATION si, TRUSTEE* pTrustee, TRUSTEE** ppTrustee)
{
	DWORD dwErr = 0;

	//::BuildTrusteeWithSid(pTrustee, NULL);
	//memset(pTrustee, 0, sizeof(*pTrustee));
	//pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	//pTrustee->TrusteeForm = TRUSTEE_IS_SID;
	//pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;

	PSID pNewSID = NULL; BOOL lbDefaulted = FALSE, lbRc = FALSE;
	
	if (si & OWNER_SECURITY_INFORMATION)
		lbRc = ::GetSecurityDescriptorOwner(pSD, &pNewSID, &lbDefaulted);
	else if (si & GROUP_SECURITY_INFORMATION)
		lbRc = ::GetSecurityDescriptorGroup(pSD, &pNewSID, &lbDefaulted);

	if (lbRc)
	{
		//pTrustee->ptstrName = (LPTSTR)pNewSID;
		::BuildTrusteeWithSid(pTrustee, pNewSID);
		if (ppTrustee)
			*ppTrustee = pTrustee;
	}
	else
	{
		dwErr = GetLastError();
		::BuildTrusteeWithSid(pTrustee, NULL);
	}

	return dwErr;
}

// Это размеры не ACE, а необходимой памяти под EXPLICIT_ACCESS entries
DWORD BuildAce(
		SECURITY_INFORMATION si,
		PACL pAcl,
		ACL_SIZE_INFORMATION& size_info,
		BOOL abInheritedOnly,
		DWORD& nAllAceSize,
		DWORD& nAllAceCount,
		PEXPLICIT_ACCESS* ppAclEntries,
		LPBYTE* ppAclSids)
{
	DWORD dwErr = 0; BOOL lbOk = TRUE;
	ACCESS_ALLOWED_ACE *pAce = NULL;
	
	for (UINT i = 0; i < size_info.AceCount; i++)
	{
		if (!::GetAce(pAcl, i, (void**)&pAce))
		{
			dwErr = GetLastError(); lbOk = FALSE; break;
		}
		// Проверка типа
		BYTE t = pAce->Header.AceType;
		if (si & DACL_SECURITY_INFORMATION)
		{
			if (t != ACCESS_ALLOWED_ACE_TYPE && t != ACCESS_DENIED_ACE_TYPE)
			{
				InvalidOp();
				dwErr = E_INVALIDARG; lbOk = FALSE; break;
			}
		}
		else
		{
			if (t != SYSTEM_AUDIT_ACE_TYPE)
			{
				InvalidOp();
				dwErr = E_INVALIDARG; lbOk = FALSE; break;
			}
		}
		// Проверить, наследуется ли?
		if (abInheritedOnly)
		{
			if (!(pAce->Header.AceFlags & INHERITED_ACE))
				continue; // нет
		}
		// SID?
		if (!::IsValidSid((PSID)&pAce->SidStart))
		{
			InvalidOp();
			dwErr = GetLastError(); lbOk = FALSE; break;
		}
		// Длина SID
		DWORD nSidLen = ::GetLengthSid((PSID)&pAce->SidStart);
		if (!nSidLen)
		{
			InvalidOp();
			dwErr = E_UNEXPECTED; lbOk = FALSE; break;
		}
		if (ppAclEntries == NULL)
		{
			nAllAceSize += sizeof(EXPLICIT_ACCESS) + nSidLen;
			nAllAceCount++;
		}
		else
		{
			_ASSERTE(ppAclEntries && ppAclSids);
			
			//PEXPLICIT_ACCESS
			//	DWORD       grfAccessPermissions; //  ACCESS_MASK
			//	ACCESS_MODE grfAccessMode; // GRANT_ACCESS/DENY_ACCESS/SET_AUDIT_SUCCESS/SET_AUDIT_FAILURE
			//	DWORD       grfInheritance;
			//	TRUSTEE     Trustee;
			(*ppAclEntries)->grfAccessPermissions = pAce->Mask;
			if (si & DACL_SECURITY_INFORMATION)
			{
				if (t == ACCESS_ALLOWED_ACE_TYPE)
					(*ppAclEntries)->grfAccessMode = GRANT_ACCESS;
				else
					(*ppAclEntries)->grfAccessMode = DENY_ACCESS;
			}
			else
			{
				if (pAce->Header.AceFlags & FAILED_ACCESS_ACE_FLAG)
					(*ppAclEntries)->grfAccessMode = SET_AUDIT_FAILURE;
				else
					(*ppAclEntries)->grfAccessMode = SET_AUDIT_SUCCESS;
			}
			(*ppAclEntries)->grfInheritance = (pAce->Header.AceFlags & (INHERIT_ONLY_ACE|/*INHERITED_ACE|*/CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE));
			// SID
			if (!::CopySid(nSidLen, (PSID)(*ppAclSids), (PSID)&pAce->SidStart))
			{
				dwErr = GetLastError(); lbOk = FALSE; break;
			}
			::BuildTrusteeWithSid(&((*ppAclEntries)->Trustee), (PSID)(*ppAclSids));
			_ASSERTE(sizeof(**ppAclSids) == 1);
			(*ppAclSids) += nSidLen;
			(*ppAclEntries)++;
		}
	} // for (UINT i = 0; i < size_info.AceCount; i++)
	
	return lbOk ? 0 : (dwErr ? dwErr : E_UNEXPECTED);
}

// Нужно звать LocalFree для ppEntries
DWORD CombineAclEntries(PSECURITY_DESCRIPTOR pOldSD,
						SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD,
						ULONG* pnAclCount, PEXPLICIT_ACCESS* ppAclEntries)
{
	DWORD dwErr = 0; BOOL lbOk = TRUE;
	
	_ASSERTE(ppAclEntries && pnAclCount);
	*pnAclCount = 0;
	*ppAclEntries = NULL;

	PACL pOldAcl = NULL; BOOL lbOldAcl = FALSE, lbOldDefaulted = FALSE;
	PACL pNewAcl = NULL; BOOL lbNewAcl = FALSE, lbNewDefaulted = FALSE;
	PEXPLICIT_ACCESS pAclEntries = NULL;
	LPBYTE pAclSids = NULL;
	
	SECURITY_DESCRIPTOR_CONTROL ctrl = {0};
	DWORD rev = 0;
	if (!::GetSecurityDescriptorControl(pSD, &ctrl, &rev))
	{
		dwErr = GetLastError(); lbOk = FALSE;
	}
	
	if (lbOk)
	{
		if (si & DACL_SECURITY_INFORMATION)
		{
			// Current DACL
			if (ctrl & SE_DACL_PROTECTED)
			{
				// The DACL's must not inherit. Switch psi to PROTECTED_DACL_INFORMATION
			}
			//else if (ctrl & SE_DACL_AUTO_INHERIT_REQ)
			//{
			//	// We must toggle the DACL's inheritance.
			//	//TODO: Generate default DACL
			//}
			else if (!::GetSecurityDescriptorDacl(pOldSD, &lbOldAcl, &pOldAcl, &lbOldDefaulted))
			{
				dwErr = GetLastError(); lbOk = FALSE;
			}
			// New (edited) DACL
			if (lbOk && !::GetSecurityDescriptorDacl(pSD, &lbNewAcl, &pNewAcl, &lbNewDefaulted))
			{
				dwErr = GetLastError(); lbOk = FALSE;
			}
		}
		else if (si & SACL_SECURITY_INFORMATION)
		{
			// Current SACL
			if (ctrl & SE_SACL_PROTECTED)
			{
				// The SACL's must not inherit. Switch psi to PROTECTED_SACL_INFORMATION
			}
			//else if (ctrl & SE_SACL_AUTO_INHERIT_REQ)
			//{
			//	// We must toggle the SACL's inheritance.
			//	//TODO: Generate default SACL
			//}
			else if (!::GetSecurityDescriptorSacl(pOldSD, &lbOldAcl, &pOldAcl, &lbOldDefaulted))
			{
				dwErr = GetLastError(); lbOk = FALSE;
			}
			// New (edited) SACL
			if (lbOk && !::GetSecurityDescriptorSacl(pSD, &lbNewAcl, &pNewAcl, &lbNewDefaulted))
			{
				dwErr = GetLastError(); lbOk = FALSE;
			}
		}
		else
		{
			dwErr = E_INVALIDARG; lbOk = FALSE;
		}
	}

	//if (!dwErr 	&& pNewAcl)
	//{
	//	dwErr = ::GetExplicitEntriesFromAcl(pNewAcl, pnAclCount, ppAclEntries);
	//}

	if (lbOk)
	{	
		ACL_SIZE_INFORMATION size_old = {0}, size_new = {0};
		// ACCESS_ALLOWED_ACE, ACCESS_DENIED_ACE и SYSTEM_AUDIT_ACE совпадают
		// остальное нас не интересует
		ACCESS_ALLOWED_ACE *pAce = NULL;
		DWORD nAllAceSize = 0, nAllAceCount = 0;
		BOOL lbAce = FALSE;
		
		if (lbOk && pOldAcl)
		{
			if (!::GetAclInformation(pOldAcl, &size_old, sizeof(size_old), AclSizeInformation))
			{
				dwErr = GetLastError(); lbOk = FALSE;
			}
		}
		if (lbOk && pNewAcl)
		{
			if (!::GetAclInformation(pNewAcl, &size_new, sizeof(size_new), AclSizeInformation))
			{
				dwErr = GetLastError(); lbOk = FALSE;
			}
		}
		
		//if (pNewDacl)
		//{
		//	dwErr = ::GetAclInformation(pNewDacl, &size_info, sizeof(size_info), AclSizeInformation);
		//	for (UINT i = 0; i < size_info.AceCount; i++)
		//	{
		//		lbAce = ::GetAce(pNewDacl, i, (void**)&Ace);
		//	}
		//}

		if (lbOk && (size_old.AceCount || size_new.AceCount))
		{
			/*
				typedef struct _EXPLICIT_ACCESS {
				  DWORD       grfAccessPermissions;
				  ACCESS_MODE grfAccessMode;
				  DWORD       grfInheritance;
				  TRUSTEE     Trustee;
				}EXPLICIT_ACCESS, *PEXPLICIT_ACCESS;
			*/
			// Сначала нужно определить объем памяти, необходимый под PEXPLICIT_ACCESS
			nAllAceSize = 0; nAllAceCount = 0;
			dwErr = BuildAce(si, pOldAcl, size_old, TRUE, nAllAceSize, nAllAceCount, NULL, NULL);
			if (!dwErr)
				dwErr = BuildAce(si, pNewAcl, size_new, FALSE, nAllAceSize, nAllAceCount, NULL, NULL);
			if (dwErr)
				lbOk = FALSE;
			// Теперь можно выделить память и сформировать EXPLICIT_ACCESS entries
			if (lbOk && nAllAceSize && nAllAceCount)
			{
				pAclEntries = (PEXPLICIT_ACCESS)LocalAlloc(LPTR, nAllAceSize);
				if (!pAclEntries)
				{
					dwErr = ERROR_NOT_ENOUGH_MEMORY; lbOk = FALSE;
				}
				else
				{
					pAclSids = ((LPBYTE)pAclEntries) + (nAllAceCount * sizeof(EXPLICIT_ACCESS));
					*pnAclCount = nAllAceCount;
					*ppAclEntries = pAclEntries;
					// Поехали
					dwErr = BuildAce(si, pOldAcl, size_old, TRUE, nAllAceSize, nAllAceCount, &pAclEntries, &pAclSids);
					if (!dwErr)
						dwErr = BuildAce(si, pNewAcl, size_new, FALSE, nAllAceSize, nAllAceCount, &pAclEntries, &pAclSids);
					if (dwErr)
						lbOk = FALSE;
				}
			}
		}
	}
	
	return lbOk ? 0 : (dwErr ? dwErr : E_UNEXPECTED);
}

DWORD CreateNewDescriptor(PSECURITY_DESCRIPTOR pOldSD, // Old
						  PSECURITY_DESCRIPTOR pParentSD, // Using for inheritance (when pOldSD is `clear`)
						  SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD, // Changed
						  PSECURITY_DESCRIPTOR* ppNewSD, SECURITY_INFORMATION* pSI) // Result (concatenate)
{
	DWORD dwErr = 0;
	ULONG nNewSize = 0;
	
	*pSI = 0;

	SECURITY_DESCRIPTOR_CONTROL ctrl = {0}; DWORD rev = 0;
	if (!::GetSecurityDescriptorControl(pSD, &ctrl, &rev))
	{
		dwErr = GetLastError();
		return dwErr ? dwErr : E_UNEXPECTED;
	}

	// *** Owner ***
	PSID pNewOwner = NULL; BOOL lbOwnerDefaulted = FALSE;
	TRUSTEE *ptNewOwner = NULL, NewOwner = {NULL};
	if (!dwErr)
	{
		if (si & OWNER_SECURITY_INFORMATION)
			dwErr = GetSecurityDescriptorTrustee(pSD, OWNER_SECURITY_INFORMATION, &NewOwner, &ptNewOwner);
		else // ошибки извлечения из старого дескриптора - здесь не важны
			GetSecurityDescriptorTrustee(pOldSD, OWNER_SECURITY_INFORMATION, &NewOwner, &ptNewOwner);
		if (ptNewOwner)
			*pSI |= OWNER_SECURITY_INFORMATION;
	}

	// *** Group *** в Windows не используется
	PSID pNewGroup = NULL; BOOL lbGroupDefaulted = FALSE;
	TRUSTEE *ptNewGroup = NULL, NewGroup = {NULL};
	if (!dwErr)
	{
		if (si & GROUP_SECURITY_INFORMATION)
			dwErr = GetSecurityDescriptorTrustee(pSD, GROUP_SECURITY_INFORMATION, &NewGroup, &ptNewGroup);
		else // ошибки извлечения из старого дескриптора - здесь не важны
			GetSecurityDescriptorTrustee(pOldSD, GROUP_SECURITY_INFORMATION, &NewGroup, &ptNewGroup);
		if (ptNewOwner)
			*pSI |= GROUP_SECURITY_INFORMATION;
	}

	// *** Dacl *** собственно права
	ULONG nDaclCount = 0; PEXPLICIT_ACCESS pDaclEntries = NULL;
	if (!dwErr && (si & DACL_SECURITY_INFORMATION))
	{
		dwErr = CombineAclEntries(pOldSD, DACL_SECURITY_INFORMATION, pSD,
					&nDaclCount, &pDaclEntries);
		if (!dwErr)
			*pSI |= DACL_SECURITY_INFORMATION;
	}

	// *** Sacl *** аудит
	ULONG nSaclCount = 0; PEXPLICIT_ACCESS pSaclEntries = NULL;
	if (!dwErr && (si & SACL_SECURITY_INFORMATION))
	{
		dwErr = CombineAclEntries(pOldSD, DACL_SECURITY_INFORMATION, pSD,
					&nSaclCount, &pSaclEntries);
		if (!dwErr)
			*pSI |= SACL_SECURITY_INFORMATION;
	}

	// *** Final *** если все успешно - можно собирать
	if (dwErr == 0)
	{
		dwErr = BuildSecurityDescriptor(ptNewOwner, ptNewGroup, nDaclCount, pDaclEntries, nSaclCount, pSaclEntries,
			NULL, &nNewSize, ppNewSD);
		if (!dwErr)
			DumpSecurityDescriptor(*ppNewSD, *pSI, _T("Compiled new descriptor"));
	}

#if 0
	// *** Inheritance ***
	if (!dwErr && ((*pSI) & (DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION)))
	{
		SECURITY_DESCRIPTOR_CONTROL ctrl = {0};
		DWORD rev = 0;
		if (!::GetSecurityDescriptorControl(pSD, &ctrl, &rev))
			dwErr = GetLastError();
		if (!dwErr && ((*pSI) & DACL_SECURITY_INFORMATION))
		{
			if (ctrl & SE_DACL_PROTECTED)
			{	/* The DACL's must not inherit. Switch psi to PROTECTED_DACL_INFORMATION */
				*pSI |= PROTECTED_DACL_SECURITY_INFORMATION;
				if (!::SetSecurityDescriptorControl(*ppNewSD, SE_DACL_PROTECTED, SE_DACL_PROTECTED))
					dwErr = GetLastError();
			}
			if (ctrl & SE_DACL_AUTO_INHERIT_REQ)
			{	/* We must toggle the DACL's inheritance. */
				*pSI |= UNPROTECTED_DACL_SECURITY_INFORMATION;
				if (!::SetSecurityDescriptorControl(*ppNewSD, SE_DACL_AUTO_INHERIT_REQ, SE_DACL_AUTO_INHERIT_REQ))
					dwErr = GetLastError();
			}
		}
		if (!dwErr && ((*pSI) & SACL_SECURITY_INFORMATION))
		{
			if (ctrl & SE_SACL_PROTECTED)
			{	/* The SACL's must not inherit. Switch psi to PROTECTED_SACL_INFORMATION */
				*pSI |= PROTECTED_SACL_SECURITY_INFORMATION;
				if (!::SetSecurityDescriptorControl(*ppNewSD, SE_SACL_PROTECTED, SE_SACL_PROTECTED))
					dwErr = GetLastError();
			}
			if (ctrl & SE_SACL_AUTO_INHERIT_REQ)
			{	/* We must toggle the SACL's inheritance. */
				*pSI |= UNPROTECTED_SACL_SECURITY_INFORMATION;
				if (!::SetSecurityDescriptorControl(*ppNewSD, SE_SACL_AUTO_INHERIT_REQ, SE_SACL_AUTO_INHERIT_REQ))
					dwErr = GetLastError();
			}
		}
	}
#endif

	// *** Free *** освободить выделенную память - это ACL
	if (pDaclEntries)
		LocalFree(pDaclEntries);
	if (pSaclEntries)
		LocalFree(pSaclEntries);

	return dwErr;
}

LONG SetKeySecurityWindows(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
	// Поднимаем (пытаемся) привилегии
	CAdjustProcessToken tRestore, tSecurity, tOwner;
	BOOL lbRestoreOk = FALSE, lbSecurityOk = FALSE, lbOwnerOk = FALSE;
	tRestore.Enable(2, SE_BACKUP_NAME, SE_RESTORE_NAME);
	tSecurity.Enable(1, SE_SECURITY_NAME);
	tOwner.Enable(1, SE_TAKE_OWNERSHIP_NAME);

	LONG hRc = ::RegSetKeySecurity(hKey, si, pSD);

	tRestore.Release();
	tSecurity.Release();
	tOwner.Release();
	
	//if (hRc != 0)
	//{
	//	REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, hRc);
	//}
	
	return hRc;
}

INT_PTR CALLBACK SecurityDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//LPARAM pParam=(LPARAM)::GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
			DWORD_PTR nExStyle = GetWindowLongPtr(hwndDlg, GWL_EXSTYLE);
			SetWindowLongPtr(hwndDlg, GWL_EXSTYLE, nExStyle|WS_EX_LAYERED);
			SetLayeredWindowAttributes(hwndDlg, 0, 0, LWA_ALPHA);
		}
		return TRUE;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* pmmi = (MINMAXINFO*)lParam;
			pmmi->ptMinTrackSize.x = pmmi->ptMinTrackSize.y = 1;
		}
		return TRUE;
	}
	return FALSE;
}

DWORD WINAPI EditSecurityThread(LPVOID lpParameter)
{
	DWORD nRc = 0;
	//lbRc = EditSecurity(NULL,pSI);
	//nRc = (DWORD)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_NULL), NULL, SecurityDialogProc, (LPARAM)lpParameter);
	
	CObjSecurity* pSecInfo = (CObjSecurity*)lpParameter;
	if (!pSecInfo)
		return -1;
	
	ITaskbarList* pTaskBar = NULL;
	HRESULT hr, hrCo = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (SUCCEEDED(hrCo))
		hr = CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_ITaskbarList,(void**)&pTaskBar);
	
	HWND hFore = GetForegroundWindow();
	HWND hParent = GetConsoleWindow();
	if (!hParent || !IsWindowVisible(hParent))
		hParent = hFore;
	
	HWND hWnd = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_NULL), NULL, (DLGPROC)SecurityDialogProc, (LPARAM)NULL);
	
	if (!hWnd)
	{
		// Т.к. основной поток заблокирован нашим плагином - можно звать FarApi
		InvalidOp();
		return 0;
	}
	
	SI_OBJECT_INFO info = {0};
	if (pSecInfo->GetObjectInformation(&info) == S_OK)
	{
		// MainThread заблокирован, можно дергать API
		LPCTSTR pszFormat = GetMsg(REGuiPermissionTitle);
		if (pszFormat && info.pszObjectName)
		{
			TCHAR* pszTitle = (TCHAR*)malloc((lstrlen(pszFormat)+lstrlenW(info.pszObjectName)+1)*sizeof(TCHAR));
			if (pszTitle)
			{
				#ifdef _UNICODE
				wsprintf(pszTitle, pszFormat, info.pszObjectName);
				#else
				TCHAR* pszAnsi = lstrdup_t(info.pszObjectName);
				wsprintf(pszTitle, pszFormat, pszAnsi);
				#endif
				SetWindowText(hWnd, pszTitle);
				free(pszTitle);
			}
			else
			{
				InvalidOp();
			}
		}
	}
	
	//TODO: Перебрать все мониторы, определить минимальные координаты
	RECT rcWork = {0};
	//RECT rcDlg = {0};
	if (hParent && IsWindowVisible(hParent))
		GetWindowRect(hParent, &rcWork);
	else
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
	//GetWindowRect(hWnd, &rcDlg);
	//SetWindowPos(hWnd, HWND_TOP,
	//		rcWork.left, rcWork.top - 2*(rcDlg.bottom-rcDlg.top), 0,0, 
	//		SWP_NOSIZE|SWP_SHOWWINDOW);
	HWND hAfter = HWND_TOP;
	if (hParent && (GetWindowLongPtr(hParent, GWL_EXSTYLE) & WS_EX_TOPMOST))
		hAfter = HWND_TOPMOST;
	SetWindowPos(hWnd, hAfter,
			rcWork.left+(rcWork.right-rcWork.left)/3, rcWork.top+(rcWork.bottom-rcWork.top)/3, 1, 1, 
			SWP_SHOWWINDOW);
	
	if (pTaskBar)
	{
		pTaskBar->HrInit();
		pTaskBar->AddTab(hWnd);
		pTaskBar->SetActiveAlt(hWnd);
	}
	
	nRc = EditSecurity(hWnd, pSecInfo) ? 1 : 0;
	
	if (pTaskBar)
	{
		pTaskBar->DeleteTab(hWnd);
		pTaskBar->Release();
		pTaskBar = NULL;
	}
	if (SUCCEEDED(hrCo))
		CoUninitialize();
	
	if (!DestroyWindow(hWnd))
	{
		_ASSERTE(FALSE);
	}

	if (hFore && IsWindow(hFore))
	{
		HWND hNewFore = GetForegroundWindow();
		if (hNewFore != hFore)
			SetForegroundWindow(hFore);
	}
	
	return nRc;
}

//void EditKeyPermissions(LPCWSTR pszComputer, HKEY hkRoot, LPCWSTR pszKeyPath, BOOL abWow64on32, BOOL abVisual)
//{
//	LONG hRc = 0;
//	DWORD cbSecurityDescriptor = 4096;
//	PSECURITY_DESCRIPTOR pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSecurityDescriptor);
//	CObjSecurity* pSI = NULL;
//	HANDLE hToken = NULL;
//	HREGKEY hk = NULL;
//	wchar_t szKeyName[512];
//	//BOOL bTokenAquired = FALSE;
//	SECURITY_INFORMATION si =
//		OWNER_SECURITY_INFORMATION|
//		GROUP_SECURITY_INFORMATION| // смысла в нем мало, винда не показывает и не обрабатывает
//		DACL_SECURITY_INFORMATION|
//		//SACL_SECURITY_INFORMATION| // Audit. даже для чтения нужна привилегия SE_SECURITY_NAME
//		0;
//	//#ifdef _DEBUG
//	//	si = 0x04000007; //PROTECTED_SACL_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION
//	//#endif
//	LPTSTR StringSecurityDescriptor = NULL;
//	ULONG StringSecurityDescriptorLen = 0, StringSecurityDescriptorLen2 = 0;
//	BOOL lbDataChanged = FALSE, lbExportRc = FALSE;
//	HANDLE hSecurityThread = NULL; DWORD nSecurityThreadID = 0, nSecurityThreadRc = 0;
//	PSECURITY_DESCRIPTOR pNewSD = NULL, ptrOwnerSD = NULL, ptrDaclSD = NULL; // освобождать только pNewSD
//	
//	BOOL lbDaclRO = FALSE, lbOwnerRO = FALSE, lbSaclRO = FALSE;
//	
//	// Поднимаем (пытаемся) привилегии
//	CAdjustProcessToken tBackup, tRestore, tSecurity;
//	BOOL lbBackupOk = FALSE, lbRestoreOk = FALSE, lbSecurityOk = FALSE;
//	if (tRestore.Enable(2, SE_BACKUP_NAME, SE_RESTORE_NAME) == 1)
//	{
//		lbBackupOk = lbRestoreOk = TRUE;
//	}
//	else
//	{
//		tRestore.Release(); // раз не удалось - не нужен
//	}
//	if (tBackup.Enable(1, SE_BACKUP_NAME) == 1)
//		lbBackupOk = TRUE;
//	else
//		tBackup.Release(); // раз не удалось - не нужен
//	if (tSecurity.Enable(1, SE_SECURITY_NAME) == 1)
//	{
//		lbSecurityOk = TRUE;
//		si |= SACL_SECURITY_INFORMATION;
//	}
//	else
//	{
//		tSecurity.Release(); // раз не удалось - не нужен
//		lbSaclRO = TRUE;
//	}
//
//	DWORD sam = SamDesired(abWow64on32);
//	// Если повысили привилегии - пробуем как BackupRestore
//	if (lbRestoreOk)
//		hRc = ::RegCreateKeyExW(hkRoot, pszKeyPath, 0, NULL, REG_OPTION_BACKUP_RESTORE, sam, NULL, &hk, NULL);
//	// Если привилегий нет, или не получилось открыть(?) то обычным образом
//	if (!lbRestoreOk || (hRc != 0))
//		hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, KEY_READ|WRITE_DAC|WRITE_OWNER|sam, &hk);
//	if (hRc != 0) // Если не удалось - попробуем просто READ_CONTROL
//		hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, READ_CONTROL|WRITE_DAC|WRITE_OWNER|sam, &hk);
//	if (hRc != 0) // Если опять не удалось - возможно нет прав на WRITE_DAC|WRITE_OWNER?
//	{
//		if (!(hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, READ_CONTROL|WRITE_OWNER|sam, &hk)))
//			lbDaclRO = TRUE;
//		else if (!(hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, READ_CONTROL|WRITE_DAC|sam, &hk)))
//			lbOwnerRO = TRUE;
//		else if (!(hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, READ_CONTROL|sam, &hk)))
//			lbDaclRO = lbOwnerRO = TRUE;
//	}
//	// Привилегии больше не нужны - дескриптор ключа открыт
//	tBackup.Release(); tRestore.Release(); tSecurity.Release();
//	
//	//if (hRc == ERROR_ACCESS_DENIED && cfg->bUseBackupRestore)
//	//{
//	//	// Попытаться повысить привилегии
//	//	bTokenAquired = cfg->BackupPrivilegesAcuire(TRUE);
//	//	if (bTokenAquired)
//	//	{
//	//		hRc = ::RegCreateKeyExW(hkRoot, pszKeyPath, 0, NULL, REG_OPTION_BACKUP_RESTORE, SamDesired(abWow64on32), NULL, &hk, NULL);
//	//		if (hRc != 0)
//	//		{
//	//			cfg->BackupPrivilegesRelease();
//	//			bTokenAquired = FALSE;
//	//		}
//	//	}
//	//}
//	if (hRc == 0)
//	{
//		if (pszKeyPath && *pszKeyPath)
//		{
//			lstrcpynW(szKeyName, PointToName(pszKeyPath), countof(szKeyName));
//		}
//		else
//		{
//			//hk = hkRoot; -- открыта копия?
//			HKeyToStringKey(hkRoot, szKeyName, countof(szKeyName));
//		}
//	}
//	
//	if (hRc == 0)
//	{
//		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
//		{
//			hRc = GetLastError();
//			if (!hRc) hRc = E_UNEXPECTED;
//			REPlugin::Message(_T("OpenProcessToken failed"), FMSG_WARNING|FMSG_MB_OK|FMSG_ERRORTYPE);
//		}
//	}
//	
//	if (hRc == 0)
//	{
//		hRc = RegGetKeySecurity(hk, si, pSecurityDescriptor, &cbSecurityDescriptor);
//		if (hRc == ERROR_INSUFFICIENT_BUFFER)
//		{
//			LocalFree((HLOCAL)pSecurityDescriptor);
//			pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,cbSecurityDescriptor);
//			hRc = RegGetKeySecurity(hk, si, pSecurityDescriptor, &cbSecurityDescriptor);
//		}
//		
//		if (hRc != 0)
//		{
//			REPlugin::MessageFmt(REM_GetKeySecurityFail, szKeyName, hRc);
//		}
//		else
//		{
//			DumpSecurityDescriptor(pSecurityDescriptor, si, _T("Current key descriptor"));
//		}
//
//
//		// -- уже не как KEY_READ
//		//// Ключ был открыт как KEY_READ, так что он в любом случае не понадобится	
//		//if (hk && hk != hkRoot)
//		//	RegCloseKey(hk);
//		//hk = NULL;
//	}
//	
//	// Показать строковое представление
//	if (hRc == 0)
//	{
//		if (!abVisual)
//		{
//			StringSecurityDescriptor = SD2StringSD(pSecurityDescriptor, si);
//			//if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSecurityDescriptor, SDDL_REVISION_1, si, &StringSecurityDescriptor, &StringSecurityDescriptorLen))
//			if (!StringSecurityDescriptor)
//			{
//				REPlugin::MessageFmt(REM_DescrToStringFail, szKeyName, GetLastError());
//			}
//			else
//			{
//				StringSecurityDescriptorLen = lstrlen(StringSecurityDescriptor);
//				MFileTxt file(0/*не важно*/);
//				if (file.FileCreateTemp(szKeyName, L"", sizeof(TCHAR)==2))
//				{
//					lbExportRc = file.FileWriteBuffered(StringSecurityDescriptor, StringSecurityDescriptorLen*sizeof(TCHAR));
//					file.FileClose();
//					if (!lbExportRc)
//						file.FileDelete();
//				}	
//				MCHKHEAP;
//
//				if (lbExportRc)
//				{
//					int nEdtRc = psi.Editor(file.GetShowFilePathName(), szKeyName, 0,0,-1,-1,
//						EF_DISABLEHISTORY/*EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONCLOSE|EF_ENABLE_F6*/,
//						#ifdef _UNICODE
//						1, 1, 1200
//						#else
//						0, 1
//						#endif
//						);
//					MCHKHEAP;
//					// Применить изменения в StringSecurityDescriptor/StringSecurityDescriptorLen
//					if (nEdtRc == EEC_MODIFIED)
//					{
//						LONG     lLoadRc = -1;
//						LPBYTE   ptrNewText = NULL;
//						LPBYTE   pszNewText = NULL;
//						DWORD    cbNewSize = 0;
//						lLoadRc = MFileTxt::LoadData(file.GetFilePathName(), (void**)&ptrNewText, &cbNewSize);
//						if (lLoadRc != 0)
//						{
//							//Ошибка загрузка файла, она уже показана
//						}
//						else
//						{
//							// Skip BOM
//							if (cbNewSize > 2 && ptrNewText[0] == 0xFF && ptrNewText[1] == 0xFE)
//							{
//								pszNewText = ptrNewText+2;
//								cbNewSize -= 2;
//							}
//							else if (ptrNewText[0] != 'O' && ptrNewText[0] != 'D')
//							{
//								//TODO: Показать ошибку (ожидается StringSecurityDescriptor)
//								InvalidOp();
//								lLoadRc = E_INVALIDARG;
//							}
//						}
//
//						if (lLoadRc == 0)
//						{
//							if (cbNewSize > (StringSecurityDescriptorLen*sizeof(TCHAR)))
//							{
//								lbDataChanged = TRUE;
//								SafeFree(StringSecurityDescriptor);
//								StringSecurityDescriptor = (LPTSTR)malloc(cbNewSize+sizeof(TCHAR));
//								memmove(StringSecurityDescriptor, pszNewText, cbNewSize+sizeof(TCHAR));
//							}
//							else
//							{
//								// Сравнить ASCIIZ строки
//								if ((cbNewSize != (StringSecurityDescriptorLen*sizeof(TCHAR))) ||
//									(memcmp(StringSecurityDescriptor, pszNewText, cbNewSize+sizeof(TCHAR)) != 0))
//								{
//									lbDataChanged = TRUE;
//									memset(StringSecurityDescriptor, 0, StringSecurityDescriptorLen);
//									memmove(StringSecurityDescriptor, pszNewText, cbNewSize+sizeof(TCHAR));
//									StringSecurityDescriptorLen = cbNewSize/sizeof(TCHAR);
//								}
//							}
//						}
//						SafeFree(ptrNewText);
//
//						#ifdef _DEBUG
//						lbDataChanged = TRUE;
//						#endif
//					}
//					MCHKHEAP;
//
//					// И удалить файл и папку
//					file.FileDelete();
//				}
//
//				if (lbDataChanged)
//				{
//					ULONG nNewLen = 0;
//					pNewSD = StringSD2SD(StringSecurityDescriptor, &nNewLen);
//					if (!pNewSD)
//					{
//						REPlugin::MessageFmt(REM_StringToDescrFail, szKeyName, GetLastError());
//					}
//					else
//					{
//						//HREGKEY hkWrite = NULL;
//						//HRESULT hr1 = S_OK, hr2 = S_OK;
//						PSID pNewOwner = NULL, pOwner = NULL;
//						BOOL lbNewDefOwner = FALSE, lbDefOwner = FALSE, lbOwnerChanged = FALSE;
//						// Если есть владелец
//						if (GetSecurityDescriptorOwner(pNewSD, &pNewOwner, &lbNewDefOwner) && pNewOwner)
//						{
//							if (GetSecurityDescriptorOwner(pSecurityDescriptor, &pOwner, &lbDefOwner) && pOwner)
//								lbOwnerChanged = !EqualSid(pOwner, pNewOwner);
//							else
//								lbOwnerChanged = TRUE;
//
//							if (lbOwnerChanged)
//							{
//								ptrOwnerSD = pNewSD;
//								lbDataChanged = TRUE;
//								//hr1 = RegOpenKeyExW(hkRoot, pszKeyPath, 0, WRITE_OWNER|SamDesired(abWow64on32), &hkWrite);
//								//if (hr1 == 0)
//								//{
//								//	hr1 = RegSetKeySecurity(hkWrite, OWNER_SECURITY_INFORMATION, pNewSD);
//								//	RegCloseKey(hkWrite);
//								//}
//							}
//						}
//
//						// Если есть DACL (разрешения)
//						PACL pNewDACL = NULL, pDACL = NULL;
//						BOOL lbNewDaclExist = FALSE, lbNewDefDacl = FALSE, lbDefDacl = FALSE, lbDaclExist = FALSE;
//						BOOL lbDaclChanged = FALSE;
//						if (GetSecurityDescriptorDacl(pNewSD, &lbNewDaclExist, &pNewDACL, &lbNewDefDacl) && lbNewDaclExist)
//						{
//							if (GetSecurityDescriptorDacl(pNewSD, &lbDaclExist, &pDACL, &lbDefDacl) && lbDaclExist)
//							{
//								//TODO: Сравнивать будем?
//								lbDaclChanged = TRUE;
//							}
//							else
//								lbDaclChanged = TRUE;
//
//							if (lbDaclChanged)
//							{
//								ptrDaclSD = pNewSD;
//								lbDataChanged = TRUE;
//							}
//						}
//
//						//hr2 = RegOpenKeyExW(hkRoot, pszKeyPath, 0, WRITE_DAC|SamDesired(abWow64on32), &hkWrite);
//						//if (hr2 == 0)
//						//{
//						//	hr2 = RegSetKeySecurity(hkWrite, DACL_SECURITY_INFORMATION, pNewSD);
//						//	RegCloseKey(hkWrite);
//						//}
//						//hRc = hr1 ? hr1 : hr2;
//						//if (hRc != 0)
//						//{
//						//	REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, hRc);
//						//}
//						//LocalFree(pNew);
//					}
//				}
//			}
//		}
//		else
//		{
//			// If an administrator needs access to the key, the solution is to enable 
//			// the SE_TAKE_OWNERSHIP_NAME privilege and open the registry key with WRITE_OWNER access.
//			// For more information, see Enabling and Disabling Privileges.
//			hRc = CreateObjSecurityInfo(
//						//#ifndef ALLOW_MODIFY_ACL
//						// ViewOnly - запретить редактирование в релизе?
//						((lbDaclRO && lbOwnerRO && lbSaclRO) ? SI_READONLY : 0) |
//						//#endif
//						//SI_EDIT_ALL | 
//						SI_EDIT_PERMS | //(lbDaclRO ? 0 : SI_EDIT_PERMS) |
//						SI_EDIT_OWNER | (lbOwnerRO ? SI_OWNER_READONLY : 0) |
//						(lbSaclRO ? 0 : SI_EDIT_AUDITS) |
//						SI_ADVANCED | /*SI_NO_ACL_PROTECT |*/ SI_CONTAINER | SI_EDIT_OWNER
//						//| SI_EDIT_EFFECTIVE // Must implement IEffectivePermission interface
//						//| SI_RESET | SI_RESET_DACL | SI_RESET_OWNER
//						,
//						pSecurityDescriptor, &pSI, hToken,
//						L""/* Computer Name */, szKeyName/*,
//						hkRoot, pszKeyPath,
//						READ_CONTROL|WRITE_DAC|WRITE_OWNER|SamDesired(abWow64on32)*/);
//			
//			if(hRc == 0)
//			{
//				CScreenRestore popDlg(GetMsg(REGuiPermissionEdit), GetMsg(REPluginName));
//				
//				// запустить отдельную нить, а в этой мониторить созданный диалог, чтоб его наверх поднять, если он под фаром окажется
//				//EditSecurity(NULL,pSI);
//				if (!g_hInstance)
//				{
//					InvalidOp();
//				}
//				else
//				{
//					hSecurityThread = CreateThread(NULL, 0, EditSecurityThread, pSI, 0, &nSecurityThreadID);
//					if (!hSecurityThread)
//					{
//						InvalidOp();
//					}
//					else
//					{
//						WaitForSingleObject(hSecurityThread, INFINITE);
//						GetExitCodeThread(hSecurityThread, &nSecurityThreadRc);
//						lbDataChanged = (nSecurityThreadRc);
//					}
//				}
//
//				//TODO: Если диалог вернул OK
//				ptrOwnerSD = pSI->GetNewOwnerSD();
//				ptrDaclSD = pSI->GetNewDaclSD();
//				lbDataChanged = (ptrOwnerSD || ptrDaclSD);
//
//				//PSECURITY_DESCRIPTOR pNew;
//				//if ((pNew = pSI->GetNewOwnerSD()) != NULL)
//				//{
//				//	DumpSecurityDescriptor(pNew, OWNER_SECURITY_INFORMATION, _T("pSI->GetNewOwnerSD()"));
//				//	HREGKEY hkWrite = NULL;
//				//	hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, WRITE_OWNER|SamDesired(abWow64on32), &hkWrite);
//				//	if (hRc == 0)
//				//	{
//				//		hRc = RegSetKeySecurity(hkWrite, OWNER_SECURITY_INFORMATION, pNew);
//				//		RegCloseKey(hkWrite);
//				//	}
//				//	if (hRc != 0)
//				//	{
//				//		REPlugin::MessageFmt(REM_SetKeyOwnerFail, szKeyName, hRc);
//				//	}
//				//}
//				//if ((pNew = pSI->GetNewDaclSD()) != NULL)
//				//{
//				//	DumpSecurityDescriptor(pNew, OWNER_SECURITY_INFORMATION, _T("pSI->GetNewDaclSD()"));
//				//	HREGKEY hkWrite = NULL;
//				//	hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, WRITE_DAC|SamDesired(abWow64on32), &hkWrite);
//				//	if (hRc == 0)
//				//	{
//				//		hRc = RegSetKeySecurity(hkWrite, DACL_SECURITY_INFORMATION, pNew);
//				//		RegCloseKey(hkWrite);
//				//	}
//				//	if (hRc != 0)
//				//	{
//				//		REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, hRc);
//				//	}
//				//}
//				
//				//DumpSecurityDescriptor(pSecurityDescriptor, si, _T("Editor thread finished"));
//				//pSI->Release(); pSI = NULL;
//				//TODO: lbDataChanged?
//			}
//		}
//	}
//	
//	// Применить изменения в реестре
//	if (lbDataChanged)
//	{
//		//// Если привилегию еще не схватили - пробуем
//		//if (!bTokenAquired)
//		//	bTokenAquired = cfg->BackupPrivilegesAcuire(TRUE);
//
//		if (ptrOwnerSD) // Проверить lbOwnerRO?
//		{
//			DumpSecurityDescriptor(ptrOwnerSD, OWNER_SECURITY_INFORMATION, _T("New OWNER SD"));
//			
//			//HREGKEY hkWrite = NULL;
//			//if (tRestore.Enable(2, SE_BACKUP_NAME, SE_RESTORE_NAME) == 1)
//			//	hRc = ::RegCreateKeyExW(hkRoot, pszKeyPath, 0, NULL, REG_OPTION_BACKUP_RESTORE, SamDesired(abWow64on32), NULL, &hkWrite, NULL);
//			// hk - уже имеет все возможные права
//			tRestore.Enable(2, SE_BACKUP_NAME, SE_RESTORE_NAME);
//			hRc = RegSetKeySecurity(/*hkWrite ? hkWrite :*/ hk, OWNER_SECURITY_INFORMATION, ptrOwnerSD);
//			tRestore.Release();
//			//if (hkWrite)
//			//	RegCloseKey(hkWrite);
//			
//			
//			//HREGKEY hkWrite = NULL;
//			//if (bTokenAquired)
//			//	hRc = ::RegCreateKeyExW(hkRoot, pszKeyPath, 0, NULL, REG_OPTION_BACKUP_RESTORE, SamDesired(abWow64on32), NULL, &hkWrite, NULL);
//			//if (!hkWrite)
//			//	hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, WRITE_OWNER|SamDesired(abWow64on32), &hkWrite);
//			//if (hRc == 0)
//			//{
//			//	hRc = RegSetKeySecurity(hkWrite, OWNER_SECURITY_INFORMATION, ptrOwnerSD);
//			//	RegCloseKey(hkWrite);
//			//}
//			if (hRc != 0)
//			{
//				REPlugin::MessageFmt(REM_SetKeyOwnerFail, szKeyName, hRc);
//			}
//		}
//		if (ptrDaclSD) // Проверить lbDaclRO
//		{
//			DumpSecurityDescriptor(ptrDaclSD, DACL_SECURITY_INFORMATION, _T("New DACL SD"));
//			
//			// hk - уже имеет все возможные права
//			tRestore.Enable(2, SE_BACKUP_NAME, SE_RESTORE_NAME);
//			hRc = RegSetKeySecurity(hk, DACL_SECURITY_INFORMATION, ptrDaclSD);
//			tRestore.Release();
//			
//			//HREGKEY hkWrite = NULL;
//			//if (bTokenAquired)
//			//	hRc = ::RegCreateKeyExW(hkRoot, pszKeyPath, 0, NULL, REG_OPTION_BACKUP_RESTORE, SamDesired(abWow64on32), NULL, &hkWrite, NULL);
//			//if (!hkWrite)
//			//	hRc = RegOpenKeyExW(hkRoot, pszKeyPath, 0, WRITE_DAC|SamDesired(abWow64on32), &hkWrite);
//			//if (hRc == 0)
//			//{
//			//	hRc = RegSetKeySecurity(hkWrite, DACL_SECURITY_INFORMATION, ptrDaclSD);
//			//	RegCloseKey(hkWrite);
//			//}
//			if (hRc != 0)
//			{
//				REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, hRc);
//			}
//		}
//		
//		//TODO: Sacl?
//	}
//	
//	//if (bTokenAquired)
//	//{
//	//	cfg->BackupPrivilegesRelease();
//	//	bTokenAquired = FALSE;
//	//}
//
//	if (pSI)
//		pSI->Release(); pSI = NULL;
//	
//	if (hToken)
//		CloseHandle(hToken);
//
//	if (pSecurityDescriptor)
//		LocalFree(pSecurityDescriptor);	
//	SafeFree(StringSecurityDescriptor);
//	if (pNewSD)
//		LocalFree(pNewSD);
//
//	if (hk && hk != hkRoot)
//		RegCloseKey(hk);
//}


// Ctor
MRegistryWinApi::MRegistryWinApi(BOOL abWow64on32, BOOL abVirtualize)
	: MRegistryBase(abWow64on32, abVirtualize)
{
	eType = RE_WINAPI;
	nPredefined = 0;
	bTokenAquired = FALSE;
	hAcquiredHkey = NULL;
	//hRemoteLogon = NULL;
	
	hAdvApi = LoadLibraryW(L"Advapi32.dll");
	if (hAdvApi)
	{
		// Vista+
		_RegDeleteKeyEx = (RegDeleteKeyExW_t)GetProcAddress(hAdvApi, "RegDeleteKeyExW");
		_RegRenameKey = (RegRenameKey_t)GetProcAddress(hAdvApi, "RegRenameKey");
	}

	FillPredefined();
}

MRegistryBase* MRegistryWinApi::Duplicate()
{
	MRegistryBase* pDup = new MRegistryWinApi(mb_Wow64on32, mb_Virtualize);
	if (bRemote)
		pDup->ConnectRemote(sRemoteServer, NULL, NULL, NULL);
	//if (hRemoteLogon) {
	//	HANDLE hProc = GetCurrentProcess();
	//	if (!DuplicateHandle(hProc, hRemoteLogon, 
	//			hProc, &((MRegistryWinApi*)pDup)->hRemoteLogon,
	//			0, FALSE, DUPLICATE_SAME_ACCESS))
	//	{
	//		//TODO: Показать ошибку Win32
	//		InvalidOp();
	//		((MRegistryWinApi*)pDup)->hRemoteLogon = NULL;
	//	}
	//}
	return pDup;
}

MRegistryWinApi::~MRegistryWinApi()
{
	ConnectLocal();
	if (hAdvApi)
	{
		FreeLibrary(hAdvApi);
		hAdvApi = NULL;
	}
}

void MRegistryWinApi::FillPredefined()
{
	nPredefined = 0;

	HKEY hk[] = {
		HKEY_CLASSES_ROOT,
		HKEY_CURRENT_USER,
		HKEY_LOCAL_MACHINE,
		HKEY_USERS,
		HKEY_CURRENT_CONFIG,
		HKEY_PERFORMANCE_DATA,
		NULL
	};
	
	// Для RemoteRegistry определены ТОЛЬКО HKEY_LOCAL_MACHINE, HKEY_PERFORMANCE_DATA, HKEY_USERS
	if (bRemote)
	{
		hk[0] = HKEY_CLASSES_ROOT;
		hk[1] = HKEY_CURRENT_USER;
		hk[2] = HKEY_LOCAL_MACHINE;
		hk[3] = HKEY_USERS;
		hk[4] = 0;
	}
	
	memset(hkPredefined, 0, sizeof(hkPredefined));
	
	for (UINT i = 0; i < countof(hk) && hk[i]; i++)
	{
		_ASSERTE((((ULONG_PTR)(hk[i])) & HKEY__PREDEFINED_MASK) == HKEY__PREDEFINED_TEST); //  Predefined!
		hkPredefined[i].hKey = hk[i]; hkPredefined[i].hKeyRemote = NULL;
		HKeyToStringKey(hk[i], hkPredefined[i].szKey, countof(hkPredefined[i].szKey));
		hkPredefined[i].bSlow = (hk[i] == HKEY_PERFORMANCE_DATA);
		nPredefined++;
	}
	_ASSERTE(nPredefined<countof(hkPredefined));
}

void MRegistryWinApi::ConnectLocal()
{
	if (bRemote)
	{
		for (UINT i = 0; i < nPredefined; i++)
		{
			if (hkPredefined[i].hKeyRemote)
			{
				// Отключение от реестра другой машины
				::RegCloseKey(hkPredefined[i].hKeyRemote);
				hkPredefined[i].hKeyRemote = NULL;
			}
		}
	}
	if (bTokenAquired)
	{
		bTokenAquired = FALSE;
		cfg->BackupPrivilegesRelease();
	}
	//if (hRemoteLogon)
	//{
	//	CloseHandle(hRemoteLogon);
	//	hRemoteLogon = NULL;
	//}
	
	MRegistryBase::ConnectLocal();

	FillPredefined();
}

BOOL MRegistryWinApi::ConnectRemote(LPCWSTR asServer, LPCWSTR asLogin /*= NULL*/, LPCWSTR asPassword /*= NULL*/, LPCWSTR asResource /*= NULL*/)
{
	// Отключиться, если было подключение
	ConnectLocal();
	
	if (!MRegistryBase::ConnectRemote(asServer, asLogin, asPassword, asResource))
		return FALSE;
	
	FillPredefined();

	HREGKEY hKey = NULLHKEY;
	LONG lRc = ConnectRegistry(HKEY_LOCAL_MACHINE, &hKey);
	if (lRc != 0)
	{
		if (0!=REPlugin::MessageFmt(REM_RemoteKeyFailed, L"HKEY_LOCAL_MACHINE", lRc, _T("RemoteConnect"), FMSG_WARNING, 2))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

// Перекрыто, потому что здесь могут быть hRemoteKeys
BOOL MRegistryWinApi::IsPredefined(HKEY hKey)
{
	if (!hKey || (((ULONG_PTR)hKey) & HKEY__PREDEFINED_MASK) != HKEY__PREDEFINED_TEST)
		return FALSE;

	for (UINT i = 0; i < nPredefined; i++) {
		if (hKey == hkPredefined[i].hKey)
			return TRUE;
	}

	_ASSERTE(hKey == hkPredefined[0].hKey);

	return FALSE;
}


LONG MRegistryWinApi::ConnectRegistry(HKEY hKey, HKEY* phkResult)
{
	if (!bRemote)
	{
		_ASSERTE(bRemote);
		return -1;
	}
	// Сначала все-таки проверим, может уже?
	int nIdx = -1;
	for (UINT i = 0; i < nPredefined; i++)
	{
		if (hKey == hkPredefined[i].hKey)
		{
			if (hkPredefined[i].hKeyRemote)
			{
				*phkResult = hkPredefined[i].hKeyRemote;
				return 0; // уже
			}
			nIdx = i; break;
		}
	}
	if (nIdx == -1)
	{
		_ASSERTE(nIdx>=0);
		return -1; // неизвестный ключ
	}
	
	//BOOL lbNeedReturnLogon = FALSE;
	//if (sRemoteLogin[0] || hRemoteLogon)
	//{
	//	if (!hRemoteLogon)
	//	{
	//		if (!LogonUserW(sRemoteLogin, NULL, sRemotePassword, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_DEFAULT, &hRemoteLogon))
	//		{
	//			// Показать ошибку Win32 - REM_LogonFailed
	//			DWORD nErrCode = GetLastError();
	//			REPlugin::MessageFmt(REM_LogonFailed, sRemoteLogin, nErrCode, _T("RemoteConnect"));
	//		}
	//		SecureZeroMemory(sRemotePassword, sizeof(sRemotePassword));
	//	}
	//	if (hRemoteLogon)
	//	{
	//		if (!ImpersonateLoggedOnUser(hRemoteLogon))
	//		{
	//			// Показать ошибку Win32 : REM_ImpersonateFailed
	//			DWORD nErrCode = GetLastError();
	//			REPlugin::MessageFmt(REM_ImpersonateFailed, sRemoteLogin, nErrCode, _T("RemoteConnect"));
	//		} else {
	//			lbNeedReturnLogon = TRUE;
	//		}
	//	}
	//}
	
	LONG lRc = ::RegConnectRegistryW(
			sRemoteServer[0] ? sRemoteServer : NULL, // NULL - для локальной машины
			hKey, &(hkPredefined[nIdx].hKeyRemote));
	
	//if (lbNeedReturnLogon)
	//{
	//	if (!RevertToSelf())
	//	{
	//		// Показать ошибку Win32 : REM_RevertToSelfFailed
	//		DWORD nErrCode = GetLastError();
	//		if (!nErrCode) { SetLastError(-1); }
	//		REPlugin::Message(REM_RevertToSelfFailed, FMSG_ERRORTYPE|FMSG_WARNING|FMSG_MB_OK, 0, _T("RemoteConnect"));
	//	}
	//}

	if (lRc == 0)
	{
		*phkResult = hkPredefined[nIdx].hKeyRemote;
	}
	return lRc;
}

// Wrappers
LONG MRegistryWinApi::CreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/, LPCWSTR pszComment /*= NULL*/)
{
	LONG lRc = 0;
	samDesired &= ~(KEY_NOTIFY);
	if (hKey == NULL)
	{
		_ASSERTE(hKey!=NULL);
		lRc = E_INVALIDARG; // Predefined keys не открываем?
	}
	else
	{
		if (bRemote && IsPredefined(hKey))
		{
			// Если подключение уже есть - возвращает его!
			lRc = ConnectRegistry(hKey, &hKey);
			if (lRc != 0)
				return lRc;
		}
		if ((dwOptions & REG__OPTION_CREATE_DELETED))
		{
			*phkResult = NULL;
			if (hKey && lpSubKey && *lpSubKey)
			{
				lRc = DeleteSubkeyTree(hKey, lpSubKey);
				if (lRc == ERROR_FILE_NOT_FOUND)
					lRc = 0; // удалять нечего
			}
			else
			{
				lRc = 0;
			}
		}
		else
		{
			dwOptions &= ~REG__OPTION_CREATE_MASK;
			if (mb_Virtualize)
				lRc = OpenCreateVirtualKey(hKey, lpSubKey, dwOptions, samDesired, TRUE, phkResult);
			else
				lRc = -1;

			if (lRc != 0)
			{
				lRc = ::RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired|SamDesired(this->mb_Wow64on32), lpSecurityAttributes, phkResult, lpdwDisposition);
			}
		}
		if (!bRemote && lRc == ERROR_ACCESS_DENIED && cfg->bUseBackupRestore && !(apRights && *apRights == eRightsNoAdjust))
		{
			// Попытаться повысить привилегии
			if (!bTokenAquired)
			{
				bTokenAquired = cfg->BackupPrivilegesAcuire(TRUE);
			}
			if (bTokenAquired)
			{
				if ((dwOptions & REG__OPTION_CREATE_DELETED))
				{
					lRc = DeleteSubkeyTree(hKey, lpSubKey);
					if (lRc == ERROR_FILE_NOT_FOUND)
						lRc = 0; // удалять нечего
				}
				else
				{
					lRc = ::RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions|REG_OPTION_BACKUP_RESTORE, samDesired|SamDesired(this->mb_Wow64on32), lpSecurityAttributes, phkResult, lpdwDisposition);
				}
				if (lRc == 0)
				{
					if (apRights) *apRights = eRightsBackupRestore;
					if (hAcquiredHkey == NULL)
						hAcquiredHkey = *phkResult;
				} else {
					cfg->BackupPrivilegesRelease();
					bTokenAquired = FALSE;
				}
			}
		}
	}
	if (lRc) SetLastError(lRc);
	return lRc;
}

LONG MRegistryWinApi::OpenCreateVirtualKey(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, BOOL abCreate, HKEY* phkResult)
{
	LONG lRc = -1;

	if (mb_Virtualize && !bRemote && (hKey == HKEY_LOCAL_MACHINE))
	{
		LONG l = 0;
		HKEY hkStore = NULL, hkSoft = NULL;
		l = ::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\VirtualStore\\MACHINE", 0, samDesired, &hkStore);
		if (l == 0)
		{
			if (cfg->is64bitOs)
			{
				wchar_t szName[MAX_PATH];
				LPCWSTR pszSlash = wcschr(lpSubKey, L'\\');
				if (!pszSlash) pszSlash = lpSubKey + wcslen(lpSubKey);
				lstrcpynW(szName, lpSubKey, min(MAX_PATH,(pszSlash - lpSubKey + 1)));
				// Если это "Software" - нужен финт ушами с "Wow6432Node"
				if ((lstrcmpiW(szName, L"Software") == 0) && (pszSlash[0]==L'\\' && pszSlash[1]))
				{
					if ((mb_Wow64on32 == 1) || (mb_Wow64on32 == 2 && !cfg->isWow64process))
					{
						// 64bit
						if (abCreate)
							lRc = ::RegCreateKeyExW(hkStore, lpSubKey, NULL, NULL, NULL, samDesired, NULL, phkResult, NULL);
						else
							lRc = ::RegOpenKeyExW(hkStore, lpSubKey, 0, samDesired, phkResult);
					}
					else
					{
						// 32bit
						l = ::RegOpenKeyExW(hkStore, L"SOFTWARE\\Wow6432Node", 0, samDesired, &hkSoft);
						if (l == 0)
						{
							if (abCreate)
								lRc = ::RegCreateKeyExW(hkSoft, pszSlash+1, NULL, NULL, NULL, samDesired, NULL, phkResult, NULL);
							else
								lRc = ::RegOpenKeyExW(hkSoft, pszSlash+1, 0, samDesired, phkResult);
						}
					}
				}
				else
				{
					if (abCreate)
						lRc = ::RegCreateKeyExW(hkStore, lpSubKey, NULL, NULL, NULL, samDesired, NULL, phkResult, NULL);
					else
						lRc = ::RegOpenKeyExW(hkStore, lpSubKey, 0, samDesired, phkResult);
				}
			}
			else
			{
				if (abCreate)
					lRc = ::RegCreateKeyExW(hkStore, lpSubKey, NULL, NULL, NULL, samDesired, NULL, phkResult, NULL);
				else
					lRc = ::RegOpenKeyExW(hkStore, lpSubKey, 0, samDesired, phkResult);
			}
			if (hkSoft)
				::RegCloseKey(hkSoft);
			::RegCloseKey(hkStore);
		}
	}

	return lRc;
}

LONG MRegistryWinApi::OpenKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult, DWORD *pnKeyFlags, RegKeyOpenRights *apRights /*= NULL*/)
{
	LONG lRc = 0;
	samDesired &= ~(KEY_NOTIFY);
	if (hKey == NULL)
	{
		_ASSERTE(hKey!=NULL);
		lRc = E_INVALIDARG; // Predefined keys не открываем?
	}
	else
	{
		if (bRemote && IsPredefined(hKey))
		{
			// Если подключение уже есть - возвращает его!
			lRc = ConnectRegistry(hKey, &hKey);
			if (lRc != 0)
				return lRc;
			if (!lpSubKey || !*lpSubKey)
			{
				*phkResult = hKey;
				return 0;
			}
		}

		if (mb_Virtualize)
			lRc = OpenCreateVirtualKey(hKey, lpSubKey, ulOptions, samDesired, FALSE, phkResult);
		else
			lRc = -1;

		if (lRc != 0)
		{
			lRc = ::RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired|SamDesired(this->mb_Wow64on32), phkResult);
		}

		if (!bRemote && lRc == ERROR_ACCESS_DENIED && cfg->bUseBackupRestore && !(apRights && *apRights == eRightsNoAdjust))
		{
			// Попытаться повысить привилегии
			if (!bTokenAquired)
			{
				bTokenAquired = cfg->BackupPrivilegesAcuire((samDesired & KEY_WRITE) == KEY_WRITE);
			}
			if (bTokenAquired)
			{
				lRc = ::RegCreateKeyExW(hKey, lpSubKey, 0, NULL, REG_OPTION_BACKUP_RESTORE, samDesired|SamDesired(this->mb_Wow64on32), NULL, phkResult, NULL);
				if (lRc == 0)
				{
					if (apRights) *apRights = ((samDesired & KEY_WRITE) == KEY_WRITE) ? eRightsBackupRestore : eRightsBackup;
					if (hAcquiredHkey == NULL)
						hAcquiredHkey = *phkResult;
				} else {
					cfg->BackupPrivilegesRelease();
					bTokenAquired = FALSE;
				}
			}
		}
		if (lRc != 0 && ((samDesired & (KEY_SET_VALUE|KEY_CREATE_SUB_KEY)) == 0))
		{
			// Попытаться открыть ключ со пониженными правами
			lRc = ::RegOpenKeyExW(hKey, lpSubKey, ulOptions, KEY_ENUMERATE_SUB_KEYS|SamDesired(this->mb_Wow64on32), phkResult);
		}
	}
	if (lRc) SetLastError(lRc);
	return lRc;
}

LONG MRegistryWinApi::CloseKey(HKEY* phKey)
{
	if (*phKey == NULL)
	{
		_ASSERTE(*phKey!=NULL);
		return -1;
	}

	for (UINT i = 0; i < nPredefined; i++)
	{
		if (hkPredefined[i].hKey == *phKey)
		{
			*phKey = NULL;
			return 0; // Predefined key!
		}
		if (bRemote)
		{
			if (hkPredefined[i].hKeyRemote == *phKey)
			{
				*phKey = NULL;
				return 0; // Predefined Remote key! они закрываются в деструкторе!
			}
		}
	}
	
	LONG lRc = ::RegCloseKey(*phKey);

	if (hAcquiredHkey && *phKey == hAcquiredHkey)
	{
		hAcquiredHkey = NULL;
		if (bTokenAquired)
		{
			bTokenAquired = FALSE;
			cfg->BackupPrivilegesRelease();
		}
	}

	*phKey = NULL;
	return lRc;
}

LONG MRegistryWinApi::QueryInfoKey(
		HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved,
		LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen,
		LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen,
		LPDWORD lpcbSecurityDescriptor, REGFILETIME* lpftLastWriteTime)
{
	if (hKey == NULL) {
		// Собираются смотреть список ключей (HKEY_xxx)
		if (lpClass) lpClass[0] = 0;
		if (lpcSubKeys) *lpcSubKeys = nPredefined;
		if (lpcMaxSubKeyLen) *lpcMaxSubKeyLen = 32;
		if (lpcMaxClassLen) *lpcMaxClassLen = 0;
		if (lpcValues) *lpcValues = 0;
		if (lpcMaxValueNameLen) *lpcMaxValueNameLen = 0;
		if (lpcMaxValueLen) *lpcMaxValueLen = 0;
		if (lpcbSecurityDescriptor) *lpcbSecurityDescriptor = 0;
		if (lpftLastWriteTime) {
			SYSTEMTIME st; GetSystemTime(&st); SystemTimeToFileTime(&st, (PFILETIME)lpftLastWriteTime);
		}
		return 0;
	}

	LONG lRc = 0;	
	if (bRemote && IsPredefined(hKey))
	{
		// Если подключение уже есть - возвращает его!
		lRc = ConnectRegistry(hKey, &hKey);
		if (lRc != 0)
			return lRc;
	}
	
	lRc = ::RegQueryInfoKeyW(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, (PFILETIME)lpftLastWriteTime);
	return lRc;
}

LONG MRegistryWinApi::EnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, BOOL abEnumComments, LPCWSTR* ppszValueComment /*= NULL*/)
{
	if (hKey == NULL)
	{
		return ERROR_NO_MORE_ITEMS;
	}
	LONG lRc = 0;
	if (bRemote && IsPredefined(hKey))
	{
		// Если подключение уже есть - возвращает его!
		lRc = ConnectRegistry(hKey, &hKey);
		if (lRc != 0)
			return lRc;
	}
	DWORD nApiDataType = 0;
	lRc = ::RegEnumValueW(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, &nApiDataType, lpData, lpcbData);
	if (lpDataType) *lpDataType = nApiDataType;
	if ((lRc == 0 || lRc == ERROR_MORE_DATA) && lpValueName && lpcchValueName)
	{
		int nLen = lstrlenW(lpValueName);
		if (nLen != *lpcchValueName)
		{
			// Глюк винды. В HKEY_PERFORMANCE_DATA почему-то возаращает 5 символов
			_ASSERTE(nLen == *lpcchValueName || hKey == HKEY_PERFORMANCE_DATA);
			*lpcchValueName = nLen;
		}
	}
	return lRc;
}

LONG MRegistryWinApi::EnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcClass, REGFILETIME* lpftLastWriteTime, DWORD* pnKeyFlags /*= NULL*/, TCHAR* lpDefValue /*= NULL*/, DWORD cchDefValueMax /*= 0*/, LPCWSTR* ppszKeyComment /*= NULL*/)
{
	LONG lRc = 0;

	// Сразу сбросим, то что в API не используется
	if (pnKeyFlags) *pnKeyFlags = 0;
	
	// Здесь чтение описаний ключа не поддерживается
	if (lpDefValue) *lpDefValue = 0;

	if (hKey == NULL) {
		if (dwIndex >= nPredefined)
			return ERROR_NO_MORE_ITEMS;
		if (bRemote || hkPredefined[dwIndex].bSlow) {
			if (lpftLastWriteTime) {
				SYSTEMTIME st; GetSystemTime(&st); SystemTimeToFileTime(&st, (PFILETIME)lpftLastWriteTime);
			}
			if (lpClass) *lpClass = 0;
			if (lpcClass) *lpcClass = 0;
		} else
		if (lpftLastWriteTime || lpClass || lpcClass) {
			lRc = ::RegQueryInfoKeyW(
						hkPredefined[dwIndex].hKey, lpClass, lpcClass,
						NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
						(PFILETIME)lpftLastWriteTime);
		}
		if (lpName) lstrcpynW(lpName, hkPredefined[dwIndex].szKey, *lpcName);
		*lpcName = lstrlenW(hkPredefined[dwIndex].szKey);
	}
	else
	{
		if (bRemote && IsPredefined(hKey))
		{
			// Если подключение уже есть - возвращает его!
			lRc = ConnectRegistry(hKey, &hKey);
			if (lRc != 0)
				return lRc;
		}
		lRc = ::RegEnumKeyExW(hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, (PFILETIME)lpftLastWriteTime);
	}
	return lRc;
}

LONG MRegistryWinApi::QueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, REGTYPE* lpDataType, LPBYTE lpData, LPDWORD lpcbData, LPCWSTR* ppszValueComment /*= NULL*/)
{
	if (hKey == NULL)
		return -1;
	LONG lRc = 0;
	if (bRemote && IsPredefined(hKey))
	{
		// Если подключение уже есть - возвращает его!
		lRc = ConnectRegistry(hKey, &hKey);
		if (lRc != 0)
			return lRc;
	}
	DWORD nApiDataType = 0;
	lRc = ::RegQueryValueExW(hKey, lpValueName, lpReserved, &nApiDataType, lpData, lpcbData);
	if (lpDataType) *lpDataType = nApiDataType;
	if (ppszValueComment) *ppszValueComment = NULL;
	return lRc;
}

LONG MRegistryWinApi::SetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, REGTYPE nDataType, const BYTE *lpData, DWORD cbData, LPCWSTR pszComment /*= NULL*/)
{
	if (nDataType >= REG__INTERNAL_TYPES)
		return 0; // Через API - не обрабатываем
	if (hKey == NULL)
		return -1;
	LONG lRc = 0;
	if (bRemote && IsPredefined(hKey))
	{
		// Если подключение уже есть - возвращает его!
		lRc = ConnectRegistry(hKey, &hKey);
		if (lRc != 0)
			return lRc;
	}
	lRc = ::RegSetValueExW(hKey, lpValueName, Reserved, (DWORD)nDataType, lpData, cbData);
	return lRc;
}


// Service - TRUE, если права были изменены
BOOL MRegistryWinApi::EditKeyPermissions(RegPath *pKey, RegItem* pItem, BOOL abVisual)
{
	if (pKey->eType != RE_WINAPI)
		return FALSE;
	
	if (pItem == NULL && pKey->mh_Root == NULL)
		return FALSE; // самый корень плагина, выделен ".."

	HKEY hkRoot = pKey->mh_Root;
	if (pKey->mh_Root == NULL)
	{
		if (!StringKeyToHKey(pItem->pszName, &hkRoot))
		{
			_ASSERTE(FALSE);
			return FALSE;
		}
	}

	// права на удаленной машине
	if (bRemote && ((hkRoot == NULL) || !IsPredefined(hkRoot)))
	{
		_ASSERTE(hkRoot==HKEY_USERS/*...*/);
		return FALSE; // Только предопределенные корневые ключи!
	}
	

	wchar_t* pszKeyPath = NULL;
	LPCWSTR pszRoot = NULL;
	int nFullLen = 0, nKeyLen = 0, nSubkeyLen = 0;
	if (hkRoot == HKEY_CLASSES_ROOT)
		pszRoot = L"CLASSES_ROOT";
	else if (hkRoot == HKEY_CURRENT_USER)
		pszRoot = L"CURRENT_USER";
	else if (hkRoot == HKEY_LOCAL_MACHINE)
		pszRoot = L"MACHINE";
	else if (hkRoot == HKEY_USERS)
		pszRoot = L"USERS";
	else
	{
		_ASSERTE(hkRoot == HKEY_USERS/* .. */);
		// Редактирование в других ключах не допускается
		return FALSE;
	}
	// Смысла нет, [Get|Set]SecurityInfo не умеют работать с указанной веткой (x64/x86)
	//nFullLen += lstrlenW(pszRoot)+1;
	LPCWSTR pszComuter = NULL;
	if (bRemote && *sRemoteServer)
	{
		if (sRemoteServer[0] != L'\\')
		{
			InvalidOp();
			return FALSE;
		}
		//nFullLen += lstrlenW(sRemoteServer)+1;
		pszComuter = sRemoteServer;
	}
		
	if (pKey->mpsz_Key && *pKey->mpsz_Key)
		nFullLen += (nKeyLen = lstrlenW(pKey->mpsz_Key));
	if (pKey->mh_Root != NULL && pItem && pItem->nValueType == REG__KEY)
		nFullLen += (nSubkeyLen = lstrlenW(pItem->pszName));
	
	pszKeyPath = (wchar_t*)malloc((nFullLen+3)*2);
	pszKeyPath[0] = 0;
	//if (bRemote && *sRemoteServer)
	//{
	//	lstrcatW(pszKeyPath, sRemoteServer);
	//	lstrcatW(pszKeyPath, L"\\");
	//}
	//lstrcatW(pszKeyPath, pszRoot);
	if (nKeyLen > 0)
	{
		lstrcatW(pszKeyPath, pKey->mpsz_Key);
		if (nSubkeyLen > 0)
		{
			lstrcatW(pszKeyPath, L"\\");
			lstrcatW(pszKeyPath, pItem->pszName);
		}
	}
	else if (nSubkeyLen > 0 && pKey->mh_Root != NULL)
	{
		//lstrcatW(pszKeyPath, L"\\");
		lstrcatW(pszKeyPath, pItem->pszName);
	}
	
	BOOL lbChanged = FALSE;
	HREGKEY hkWrite = NULLHKEY;
	HREGKEY hkParent = NULLHKEY;
	wchar_t szKeyName[512]; // Title
	#ifdef _UNICODE
	TCHAR* szKeyNameT = szKeyName;
	#else
	char szKeyNameT[512];
	#endif
	SECURITY_INFORMATION si = 0, siWrite = 0;
	PSECURITY_DESCRIPTOR pSD = NULL;

	if (pszKeyPath && *pszKeyPath)
		lstrcpynW(szKeyName, PointToName(pszKeyPath), countof(szKeyName));
	else
		HKeyToStringKey(hkRoot, szKeyName, countof(szKeyName));
	#ifndef _UNICODE
	lstrcpy_t(szKeyNameT, countof(szKeyNameT), szKeyName);
	#endif
	
	LONG hRc = GetKeySecurity(hkRoot, pszKeyPath, &hkWrite, &hkParent, &si, &pSD, &siWrite);
	if (hRc != 0)
	{
		REPlugin::MessageFmt(REM_GetKeySecurityFail, szKeyName, hRc);
	}
	else
	{
		DumpSecurityDescriptor(pSD, si, _T("Current key descriptor"));
		
		// [Get|Set]SecurityInfo использовать нифига нельзя, потому что
		// они не умеют работать с указанной веткой (x64/x86)
		//::EditKeyPermissions(pszComputer, hkRoot, pszKeyPath, mb_Wow64on32, abVisual);
		
		// Показать строковое представление
		if (hRc == 0)
		{
			if (!abVisual)
			{
				LPTSTR StringSecurityDescriptor = SD2StringSD(pSD, si);
				//if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSecurityDescriptor, SDDL_REVISION_1, si, &StringSecurityDescriptor, &StringSecurityDescriptorLen))
				if (!StringSecurityDescriptor)
				{
					REPlugin::MessageFmt(REM_DescrToStringFail, szKeyName, GetLastError());
				}
				else
				{
					BOOL lbExportRc = FALSE, lbDataChanged = FALSE;
					ULONG StringSecurityDescriptorLen = lstrlen(StringSecurityDescriptor);
					MFileTxtReg file(0/*не важно*/, 0);
					if (file.FileCreateTemp(szKeyName, L"", sizeof(TCHAR)==2))
					{
						lbExportRc = file.FileWriteBuffered(StringSecurityDescriptor, StringSecurityDescriptorLen*sizeof(TCHAR));
						file.FileClose();
						if (!lbExportRc)
							file.FileDelete();
					}	
					MCHKHEAP;

					if (lbExportRc)
					{
						int nEdtRc = psi.Editor(file.GetShowFilePathName(), szKeyNameT, 0,0,-1,-1,
							EF_DISABLEHISTORY/*EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONCLOSE|EF_ENABLE_F6*/,
							#ifdef _UNICODE
							1, 1, 1200
							#else
							0, 1
							#endif
							);
						MCHKHEAP;
						// Применить изменения в StringSecurityDescriptor/StringSecurityDescriptorLen
						if (nEdtRc == EEC_MODIFIED)
						{
							LONG     lLoadRc = -1;
							LPBYTE   ptrNewText = NULL;
							LPBYTE   pszNewText = NULL;
							DWORD    cbNewSize = 0;
							lLoadRc = MFileTxt::LoadData(file.GetFilePathName(), (void**)&ptrNewText, &cbNewSize);
							if (lLoadRc != 0)
							{
								//Ошибка загрузка файла, она уже показана
							}
							else
							{
								// Skip BOM
								if (cbNewSize > 2 && ptrNewText[0] == 0xFF && ptrNewText[1] == 0xFE)
								{
									pszNewText = ptrNewText+2;
									cbNewSize -= 2;
								}
								else if ((ptrNewText[0] != 'O' && ptrNewText[0] != 'D'
									 	  && ptrNewText[0] != 'G' && ptrNewText[0] != 'S')
									 	 || (ptrNewText[1] != ':'))
								{
									REPlugin::MessageFmt(REM_DescrToStringFail, szKeyName);
									lLoadRc = E_INVALIDARG;
								}
							}

							if (lLoadRc == 0)
							{
								if (cbNewSize > (StringSecurityDescriptorLen*sizeof(TCHAR)))
								{
									lbDataChanged = TRUE;
									SafeFree(StringSecurityDescriptor);
									StringSecurityDescriptor = (LPTSTR)malloc(cbNewSize+sizeof(TCHAR));
									memmove(StringSecurityDescriptor, pszNewText, cbNewSize+sizeof(TCHAR));
								}
								else
								{
									// Сравнить ASCIIZ строки
									if ((cbNewSize != (StringSecurityDescriptorLen*sizeof(TCHAR))) ||
										(memcmp(StringSecurityDescriptor, pszNewText, cbNewSize+sizeof(TCHAR)) != 0))
									{
										lbDataChanged = TRUE;
										memset(StringSecurityDescriptor, 0, StringSecurityDescriptorLen);
										memmove(StringSecurityDescriptor, pszNewText, cbNewSize+sizeof(TCHAR));
										StringSecurityDescriptorLen = cbNewSize/sizeof(TCHAR);
									}
								}
							}
							SafeFree(ptrNewText);

							#ifdef _DEBUG
							lbDataChanged = TRUE;
							#endif
						}
						MCHKHEAP;

						// И удалить файл и папку
						file.FileDelete();
					}

					if (lbDataChanged)
					{
						ULONG nNewLen = 0;
						PSECURITY_DESCRIPTOR pNewSD = StringSD2SD(StringSecurityDescriptor, &nNewLen);
						if (!pNewSD)
						{
							hRc = GetLastError();
							if (!hRc) hRc = E_UNEXPECTED;
							REPlugin::MessageFmt(REM_StringToDescrFail, szKeyName, hRc);
						}
						else if (siWrite)
						{
							DumpSecurityDescriptor(pNewSD, siWrite, _T("New descriptor from TextEditor"));
							// Установить новые разрешения
							hRc = SetKeySecurity(
										hkWrite /*? hkWrite : hkRoot,
										hkWrite ? NULL : pszKeyPath*/,
										siWrite, pNewSD);
							if (hRc)
								REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, hRc);
							SafeFree(pNewSD);
							lbChanged = (hRc == 0);
						}
						else
						{
							// Вообще нет прав на изменение дескриптора
							REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, 0);
						}
					}
					SafeFree(StringSecurityDescriptor);
				}
			} // end 'if (!abVisual)'
			else
			{
				//OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
				//GetVersionEx(&osv);
				//DWORD nPermsElevation = (osv.dwMajorVersion>=6) ? 0x01000000L/*SI_PERMS_ELEVATION_REQUIRED*/ : 0;
				//DWORD nAuditElevation = (osv.dwMajorVersion>=6) ? 0x02000000L/*SI_AUDITS_ELEVATION_REQUIRED*/ : 0;
				//DWORD nOwnerElevation = (osv.dwMajorVersion>=6) ? 0x04000000L/*SI_OWNER_ELEVATION_REQUIRED*/ : 0;
				//SI_AUDITS_ELEVATION_REQUIRED 0x02000000L
				//SI_OWNER_ELEVATION_REQUIRED 0x04000000L
				//SI_PERMS_ELEVATION_REQUIRED 0x01000000L
				//SI_RESET_DACL_TREE 0x00004000L "Reset permissions on all child objects"
				
				CObjSecurity* pSecInfo = NULL;
				// If an administrator needs access to the key, the solution is to enable 
				// the SE_TAKE_OWNERSHIP_NAME privilege and open the registry key with WRITE_OWNER access.
				// For more information, see Enabling and Disabling Privileges.
				hRc = CreateObjSecurityInfo(
							SI_CONTAINER |
							//#ifndef ALLOW_MODIFY_ACL
							// ViewOnly - запретить редактирование в релизе?
							//#ifdef _DEBUG
							((siWrite == 0) ? SI_READONLY : 0) |
							//#else
							//SI_READONLY
							//#endif
							//#endif
							SI_EDIT_ALL | 
							//SI_EDIT_PERMS | //(lbDaclRO ? 0 : SI_EDIT_PERMS) |
							//SI_EDIT_OWNER | 
							((siWrite & OWNER_SECURITY_INFORMATION) ? 0 : SI_OWNER_READONLY) |
							//((siWrite & SACL_SECURITY_INFORMATION) ? SI_EDIT_AUDITS : 0) |
							//((siWrite & SACL_SECURITY_INFORMATION) ? 0 : nAuditElevation) |
							SI_ADVANCED |
							SI_EDIT_EFFECTIVE | // Must implement IEffectivePermission interface
							//| SI_RESET | SI_RESET_DACL | SI_RESET_OWNER
							0
							,
							pSD,
							&pSecInfo,
							(bRemote && *sRemoteServer) ? sRemoteServer : L"",
							SamDesired(mb_Wow64on32),
							this,
							hkRoot, pszKeyPath, hkWrite, hkParent
							);
				
				if(hRc == 0)
				{
					BOOL lbDataChanged = FALSE;
					CScreenRestore popDlg(GetMsg(REGuiPermissionEdit), GetMsg(REPluginName));
					
					// запустить отдельную нить, а в этой мониторить созданный диалог, чтоб его наверх поднять, если он под фаром окажется
					//EditSecurity(NULL,pSI);
					if (!g_hInstance)
					{
						InvalidOp();
					}
					else
					{
						DWORD nSecurityThreadID = 0, nSecurityThreadRc = 0;
						HANDLE hSecurityThread = CreateThread(NULL, 0, EditSecurityThread, pSecInfo, 0, &nSecurityThreadID);
						if (!hSecurityThread)
						{
							InvalidOp();
						}
						else
						{
							WaitForSingleObject(hSecurityThread, INFINITE);
							GetExitCodeThread(hSecurityThread, &nSecurityThreadRc);
							lbDataChanged = (nSecurityThreadRc);
							CloseHandle(hSecurityThread);
						}
					}

					////TODO: Если диалог вернул OK
					//// Если задан hkWrite - то права уже установлены в самом диалоге
					// Если mhk_Write == NULL - дергаться смысла нет
					if (lbDataChanged && !hkWrite)
					{
					//	PSECURITY_DESCRIPTOR pNewSD = pSecInfo->GetNewSD(&siWrite); // const
					//	if (pNewSD)
					//	{
					//		if (siWrite)
					//		{
					//			// ViewOnly - запретить редактирование в релизе?
					//			#ifndef _DEBUG
					//			InvalidOp();
					//			#else
					//			DumpSecurityDescriptor(pNewSD, siWrite, _T("New descriptor from UIEditor"));
					//			// Установить новые разрешения
					//			hRc = SetKeySecurity(
					//						hkRoot, pszKeyPath,
					//						siWrite, pNewSD);
					//			if (hRc)
					//				REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, hRc);
					//			lbChanged = (hRc == 0);
					//			#endif
					//		}
					//		else
					//		{
								// Вообще нет прав на изменение дескриптора
								REPlugin::MessageFmt(REM_SetKeySecurityFail, szKeyName, 0);
					//		}
					//	}
					}
				}
				if (pSecInfo)
				{
					pSecInfo->Release(); pSecInfo = NULL;
				}
			} // end 'abVisual'
		}
	}
	
	if (hkWrite)
		CloseKey(&hkWrite);
	if (hkParent)
		CloseKey(&hkParent);
	if (pSD)
		LocalFree(pSD);
	SafeFree(pszKeyPath);
	
	return lbChanged;
}

// Если функция возвращает NULL в phKey (например в MFileReg) - RegSetKeySecurity не вызывается
// ppSD - LocalAlloc'ed
LONG MRegistryWinApi::GetKeySecurity(HKEY hkRoot, LPCWSTR lpszKey, HKEY* phKey, HKEY* phParentKey, SECURITY_INFORMATION *pSI, PSECURITY_DESCRIPTOR *ppSD, SECURITY_INFORMATION *pWriteSI)
{
	LONG hRc = 0;
	DWORD cbSecurityDescriptor = 4096;
	_ASSERTE(ppSD!=NULL && *ppSD==NULL);
	_ASSERTE(*phKey == NULL);
	_ASSERTE(*phParentKey == NULL);
	*ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSecurityDescriptor);
	
	*pSI =
		OWNER_SECURITY_INFORMATION|  // Owner
		DACL_SECURITY_INFORMATION|   // Permissions
		GROUP_SECURITY_INFORMATION|  // смысла в нем мало, винда не показывает и не обрабатывает
		//SACL_SECURITY_INFORMATION| // Audit. даже для чтения нужна привилегия SE_SECURITY_NAME
		0;
	*pWriteSI = 0;
	
	BOOL lbDaclRO = FALSE, lbOwnerRO = FALSE, lbSaclRO = FALSE;
	DWORD KSM = 0;
	
	// Поднимаем (пытаемся) привилегии
	CAdjustProcessToken tBackup, tRestore, tSecurity, tOwner;
	BOOL lbBackupOk = FALSE, lbRestoreOk = FALSE, lbSecurityOk = FALSE, lbOwnerOk = FALSE;
	if (tRestore.Enable(2, SE_BACKUP_NAME, SE_RESTORE_NAME) == 1)
	{
		lbBackupOk = lbRestoreOk = TRUE;
	}
	else
	{
		tRestore.Release(); // раз не удалось - не нужен
	}
	if (!lbBackupOk)
	{
		if (tBackup.Enable(1, SE_BACKUP_NAME) == 1)
			lbBackupOk = TRUE;
		else
			tBackup.Release(); // раз не удалось - не нужен
	}
	if (tSecurity.Enable(1, SE_SECURITY_NAME) == 1)
	{
		lbSecurityOk = TRUE;
		*pSI |= SACL_SECURITY_INFORMATION;
	}
	else
	{
		tSecurity.Release(); // раз не удалось - не нужен
		lbSaclRO = TRUE;
	}
	if (tOwner.Enable(1, SE_TAKE_OWNERSHIP_NAME) == 1)
		lbOwnerOk = TRUE;

	DWORD sam = SamDesired(mb_Wow64on32);
	// Если повысили привилегии - пробуем как BackupRestore
	if (lbRestoreOk)
		hRc = ::RegCreateKeyExW(hkRoot, lpszKey, 0, NULL, REG_OPTION_BACKUP_RESTORE, sam, NULL, phKey, NULL);
	// Если привилегий нет, или не получилось открыть(?) то обычным образом
	if (!lbRestoreOk || (hRc != 0))
		hRc = RegOpenKeyExW(hkRoot, lpszKey, 0, KSM=KEY_ALL_ACCESS|WRITE_DAC|WRITE_OWNER|sam, phKey);
	if (hRc != 0) // Если не удалось - попробуем просто KEY_READ
		hRc = RegOpenKeyExW(hkRoot, lpszKey, 0, KSM=KEY_READ|WRITE_DAC|WRITE_OWNER|sam, phKey);
	if (hRc != 0) // Если не удалось - попробуем просто READ_CONTROL
		hRc = RegOpenKeyExW(hkRoot, lpszKey, 0, KSM=READ_CONTROL|WRITE_DAC|WRITE_OWNER|sam, phKey);
	if (hRc != 0) // Если опять не удалось - возможно нет прав на WRITE_DAC|WRITE_OWNER?
	{
		if (!(hRc = RegOpenKeyExW(hkRoot, lpszKey, 0, KSM=READ_CONTROL|WRITE_OWNER|sam, phKey)))
			lbDaclRO = TRUE;
		else if (!(hRc = RegOpenKeyExW(hkRoot, lpszKey, 0, KSM=READ_CONTROL|WRITE_DAC|sam, phKey)))
			lbOwnerRO = TRUE;
		else if (!(hRc = RegOpenKeyExW(hkRoot, lpszKey, 0, KSM=READ_CONTROL|sam, phKey)))
			lbDaclRO = lbOwnerRO = TRUE;
		else if (lbBackupOk && !(hRc = ::RegCreateKeyExW(hkRoot, lpszKey, 0, NULL,
						REG_OPTION_BACKUP_RESTORE, sam, NULL, phKey, NULL)))
		{
			lbDaclRO = lbOwnerRO = TRUE;
			KSM = 0;
		}
	}
	
	if (phParentKey && lpszKey && *lpszKey)
	{
		LPWSTR pszParentKey = lstrdup(lpszKey);
		if (pszParentKey)
		{
			wchar_t* pszSlash = wcsrchr(pszParentKey, L'\\');
			if (pszSlash)
				*pszSlash = 0;
			else
				*pszParentKey = 0;
			if (KSM == 0)
				::RegCreateKeyExW(hkRoot, pszParentKey, 0, NULL, REG_OPTION_BACKUP_RESTORE, sam, NULL, phParentKey, NULL);
			else
				::RegOpenKeyExW(hkRoot, pszParentKey, 0, KSM, phParentKey);
		}
	}
	
	if (hRc == 0)
	{
		hRc = RegGetKeySecurity(*phKey, *pSI, *ppSD, &cbSecurityDescriptor);
		if (hRc == ERROR_INSUFFICIENT_BUFFER)
		{
			LocalFree(*ppSD);
			*ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,cbSecurityDescriptor);
			hRc = RegGetKeySecurity(*phKey, *pSI, *ppSD, &cbSecurityDescriptor);
		}
		
		//if (hRc != 0)
		//{
		//	REPlugin::MessageFmt(REM_GetKeySecurityFail, szKeyName, hRc);
		//}
		//else
		//{
		//	DumpSecurityDescriptor(pSecurityDescriptor, si, _T("Current key descriptor"));
		//}
	}
	
	// Привилегии больше не нужны - дескриптор ключа открыт
	tBackup.Release(); tRestore.Release(); tSecurity.Release(); tOwner.Release();

	if (hRc == 0)
	{
		*pWriteSI = GROUP_SECURITY_INFORMATION;  // по умолчанию?
		if (!lbDaclRO)
			*pWriteSI |= DACL_SECURITY_INFORMATION;
		if (!lbOwnerRO)
			*pWriteSI |= OWNER_SECURITY_INFORMATION;
		if (!lbSaclRO)
			*pWriteSI |= SACL_SECURITY_INFORMATION;
	}
	
	return hRc;
}

// ppSD - LocalAlloc'ed
LONG MRegistryWinApi::GetKeySecurity(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR *ppSD)
{
	LONG hRc = 0;
	DWORD cbSecurityDescriptor = 4096;
	_ASSERTE(ppSD!=NULL && *ppSD==NULL);
	*ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSecurityDescriptor);

	//*pSI =
	//	OWNER_SECURITY_INFORMATION|  // Owner
	//	DACL_SECURITY_INFORMATION|   // Permissions
	//	GROUP_SECURITY_INFORMATION|  // смысла в нем мало, винда не показывает и не обрабатывает
	//	//SACL_SECURITY_INFORMATION| // Audit. даже для чтения нужна привилегия SE_SECURITY_NAME
	//	0;

	// Поднимаем (пытаемся) привилегии
	CAdjustProcessToken tBackup, tSecurity;
	tBackup.Enable(1, SE_BACKUP_NAME); // может и не нужно, но чтобы не проверять, как именно был открыт ключ...
	if (si & SACL_SECURITY_INFORMATION)
	{
		if (tSecurity.Enable(1, SE_SECURITY_NAME) != 1)
		{
			si &= ~SACL_SECURITY_INFORMATION; // отрубаем, считать не удастся
			tSecurity.Release(); // раз не удалось - не нужен
		}
	}

	hRc = RegGetKeySecurity(hKey, si, *ppSD, &cbSecurityDescriptor);
	if (hRc == ERROR_INSUFFICIENT_BUFFER)
	{
		LocalFree(*ppSD);
		*ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,cbSecurityDescriptor);
		hRc = RegGetKeySecurity(hKey, si, *ppSD, &cbSecurityDescriptor);
	}


	// Привилегии больше не нужны - дескриптор ключа открыт
	tBackup.Release(); tSecurity.Release();

	return hRc;
}

LONG MRegistryWinApi::SetKeySecurity(HKEY hKey, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
	if (!hKey /*|| lpszSubKey || *lpszSubKey*/)
	{
		// Ключ должен быть открыт и передан в hKey!
		InvalidOp();
		return E_INVALIDARG;
	}
	LONG hRc = SetKeySecurityWindows(hKey, si, pSD);
	return hRc;
}

//LONG MRegistryWinApi::NotifyChangeKeyValue(RegFolder *pFolder, HKEY hKey)
//{
//	if (bRemote)
//		return -1; // Для удаленной машины - не поддерживается
//		
//	if (pFolder->mh_ChangeNotify == NULL) {
//		pFolder->mh_ChangeNotify = CreateEvent(NULL, TRUE, FALSE, NULL);
//	} else {
//		ResetEvent(pFolder->mh_ChangeNotify);
//	}
//
//	#ifdef _DEBUG
//	DWORD nWait = WaitForSingleObject(pFolder->mh_ChangeNotify, 0);
//	#endif
//
//	// смотрим Subtree в том случае, если загружаются описания
//	// получается некоторая избыточность (мониторятся все подуровни ключа)
//	// но это лучше, чем мониторить все дочерние ключи одноуровнево
//	LONG lRc = RegNotifyChangeKeyValue(
//					hKey, FALSE/*cfg->bLoadDescriptions*/,  //TRUE - какая-то пурга. ключ постоянно перечитывается?
//					REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET,
//					pFolder->mh_ChangeNotify, TRUE);
//	
//
//	#ifdef _DEBUG
//	nWait = WaitForSingleObject(pFolder->mh_ChangeNotify, 0);
//	#endif
//
//	return lRc;
//}

LONG MRegistryWinApi::DeleteValue(HKEY hKey, LPCWSTR lpValueName)
{
	LONG hRc = RegDeleteValueW(hKey, lpValueName);
	return hRc;
}

LONG MRegistryWinApi::DeleteSubkeyTree(HKEY hKey, LPCWSTR lpSubKey)
{
	if (!lpSubKey || !*lpSubKey)
		return -1;

	LONG hRc = 0;
	HREGKEY hSubKey = NULLHKEY;

	hRc = OpenKeyEx(hKey, lpSubKey, 0, 
		DELETE|KEY_ENUMERATE_SUB_KEYS |SamDesired(this->mb_Wow64on32), &hSubKey, NULL);
	if (hRc == 0)
	{
		RegFolder subkeys; //memset(&subkeys,0,sizeof(subkeys)); -- больше низя! инициализиуется конструктором!
		//REGFILETIME ft = {{0}};
		RegPath subkey = {RE_UNDEFINED}; subkey.Init(RE_WINAPI, mb_Wow64on32, mb_Virtualize, hSubKey); // , L"", ft);
		subkeys.Init(&subkey);
		if (subkeys.LoadKey(NULL, this, eKeysOnly, TRUE, TRUE, FALSE, NULL))
		{
			// Удалить все подключи
			for (UINT i = 0; i < subkeys.mn_ItemCount; i++)
			{
				if (subkeys.mp_Items[i].nValueType == REG__KEY)
				{
					hRc = DeleteSubkeyTree(hSubKey, subkeys.mp_Items[i].pszName);
				}
			}
		}
		subkeys.Release();
		CloseKey(&hSubKey);
	}

	hRc = ::RegDeleteKeyW(hKey, lpSubKey);
	return hRc;
}

LONG MRegistryWinApi::ExistValue(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpValueName, REGTYPE* pnDataType /*= NULL*/, DWORD* pnDataSize /*= NULL*/)
{
	LONG hRc = 0;
	HREGKEY hSubKey = NULLHKEY;
	REGTYPE nDataType = 0;
	DWORD nDataSize = 0;

	//TODO: Заменить KEY_READ на что-то менее требовательное к правам?
	
	// Проверить наличие ключа
	if (lpszKey && *lpszKey)
	{
		hRc = OpenKeyEx(hKey, lpszKey, 0, KEY_READ|SamDesired(this->mb_Wow64on32), &hSubKey, NULL);
	} else {
		hSubKey = hKey; hRc = 0;
	}
	
	if (hRc == 0) {
		// Проверяем наличие значение, и получаем заодно его тип данных
		hRc = QueryValueEx(hSubKey, lpValueName, NULL, &nDataType, NULL, &nDataSize);
		// Может он ERROR_MORE_DATA может вернуть?
		if (hRc == ERROR_MORE_DATA) hRc = 0;
		if (hRc == 0) {
			if (pnDataType) *pnDataType = nDataType;
			if (pnDataSize) *pnDataSize = nDataSize;
		}

		// Ключ на чтение больше не требуется
		CloseKey(&hSubKey);
	}
	
	return hRc;
}

LONG MRegistryWinApi::ExistKey(HKEY hKey, LPCWSTR lpszKey, LPCWSTR lpSubKey)
{
	LONG hRc = -1;
	HREGKEY hSubKey = NULLHKEY;
	HREGKEY hSubKey2 = NULLHKEY;
	//DWORD nDataType = 0;

	//TODO: Заменить KEY_READ на что-то менее требовательное к правам?
	
	// Проверить наличие ключа
	if (lpszKey && *lpszKey)
	{
		hRc = OpenKeyEx(hKey, lpszKey, 0, KEY_READ|SamDesired(this->mb_Wow64on32), &hSubKey, NULL);
	}
	else
	{
		hSubKey = hKey; hRc = 0;
	}
	
	if (hRc == 0)
	{
		// Проверяем подключ
		if (lpSubKey && *lpSubKey)
		{
			hRc = OpenKeyEx(hSubKey, lpSubKey, 0, KEY_READ|SamDesired(this->mb_Wow64on32), &hSubKey2, NULL);
			// Подключ на чтение больше не требуется
			if (hRc == 0)
				CloseKey(&hSubKey2);
		}

		// Ключ на чтение больше не требуется
		if (hSubKey != hKey)
			CloseKey(&hSubKey);
		else
			hSubKey = NULL; // для четкости
	}
	
	return hRc;
}

LONG MRegistryWinApi::SaveKey(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes /*= NULL*/)
{
	LONG hRc = 0;
	
	// The calling process must have the SE_BACKUP_NAME privilege enabled
	hRc = ::RegSaveKeyW(hKey, lpFile, lpSecurityAttributes);
	if (hRc != 0) SetLastError(hRc);
	
	return hRc;
}

LONG MRegistryWinApi::RestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags /*= 0*/)
{
	// The calling process must have the SE_RESTORE_NAME and SE_BACKUP_NAME privileges 
	LONG hRc = 0;
	
	hRc = ::RegRestoreKeyW(hKey, lpFile, dwFlags);
	if (hRc != 0) SetLastError(hRc);
	
	return hRc;
}

LONG MRegistryWinApi::GetSubkeyInfo(HKEY hKey, LPCWSTR lpszSubkey, LPTSTR pszDesc, DWORD cchMaxDesc, LPTSTR pszOwner, DWORD cchMaxOwner)
{
	LONG hRc = 0;
	HREGKEY hSubKey = NULLHKEY;
	_ASSERTE(cchMaxDesc == 128);
	
	if (pszDesc)  *pszDesc  = 0;
	if (pszOwner) *pszOwner = 0;
	
	// Не пытаться выполнить AjustTokenPrivileges
	if (0 == (hRc = OpenKeyEx(hKey, lpszSubkey, 0, KEY_READ, &hSubKey, FALSE)))
	{
		wchar_t wsData[4096];
		DWORD   dwDataSize = sizeof(wsData);
		REGTYPE dwValueType = 0;		
		LPBYTE  ptrData = (LPBYTE)wsData;
		DWORD   dwAllocDataSize = sizeof(wsData);
		PSECURITY_DESCRIPTOR pSD = NULL;
		SECURITY_INFORMATION si = OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION;
		
		// Считать значение "по умолчанию"
		hRc = QueryValueEx(hSubKey, NULL, NULL, &dwValueType, (LPBYTE)ptrData, &dwDataSize);
		if (hRc == ERROR_MORE_DATA)
		{
			_ASSERTE(dwDataSize > dwAllocDataSize);
			ptrData = (LPBYTE)malloc(dwDataSize);
			dwAllocDataSize = dwDataSize;
			hRc = QueryValueEx(hSubKey, NULL, NULL, &dwValueType, (LPBYTE)ptrData, &dwDataSize);
		}
		else if (hRc == ERROR_FILE_NOT_FOUND)
		{
			hRc = QueryValueEx(hSubKey, L"Description", NULL, &dwValueType, (LPBYTE)ptrData, &dwDataSize);
			if (hRc == ERROR_MORE_DATA)
			{
				_ASSERTE(dwDataSize > dwAllocDataSize);
				ptrData = (LPBYTE)malloc(dwDataSize);
				dwAllocDataSize = dwDataSize;
				hRc = QueryValueEx(hSubKey, L"Description", NULL, &dwValueType, (LPBYTE)ptrData, &dwDataSize);
			}
		}

		if (0 == hRc)
		{
			FormatDataVisual(dwValueType, ptrData, dwDataSize, pszDesc);
		}
		

		pSD = (PSECURITY_DESCRIPTOR)ptrData;
		hRc = RegGetKeySecurity(hSubKey, si, pSD, &dwAllocDataSize);
		if (hRc == ERROR_INSUFFICIENT_BUFFER) {
			if (ptrData != (LPBYTE)wsData)
				SafeFree(ptrData);
			ptrData = (LPBYTE)malloc(dwAllocDataSize);
			pSD = (PSECURITY_DESCRIPTOR)ptrData;
			hRc = RegGetKeySecurity(hSubKey, si, pSD, &dwAllocDataSize);
		}
		if (hRc == 0)
		{
			PSID pOwner = NULL;
			BOOL bOwnerDefaulted = FALSE;
			if (GetSecurityDescriptorOwner(pSD, &pOwner, &bOwnerDefaulted))
			{
				if (IsValidSid(pOwner))
				{
					gpPluginList->GetSIDName(bRemote ? (sRemoteServer+2) : L"", pOwner, pszOwner, cchMaxOwner);
				}
			}
		}

		if (ptrData != (LPBYTE)wsData)
			SafeFree(ptrData);
		
		CloseKey(&hSubKey);
	}
	else
	{
		// Значения по умолчанию нет, но поскольку это используется только для
		// отображения в колонке C0/DIZ то вернем пустую строку, чтобы плагин
		// не пытался открыть ключ и считать default value самостоятельно
		if (pszDesc) { pszDesc[0] = _T(' '); pszDesc[1] = 0; }
		if (pszOwner) { pszOwner[0] = _T(' '); pszOwner[1] = 0; }
	}
	
	return hRc;
}

LONG MRegistryWinApi::RenameKey(RegPath* apParent, BOOL abCopyOnly, LPCWSTR lpOldSubKey, LPCWSTR lpNewSubKey, BOOL* pbRegChanged)
{
	if (!lpOldSubKey || !lpNewSubKey || !apParent || !*lpOldSubKey || !*lpNewSubKey || !pbRegChanged)
	{
		InvalidOp();
		return E_INVALIDARG;
	}

	LONG hRc = 0;

	// Начиная с Vista есть хорошая функция, пробуем
	if (!abCopyOnly && _RegRenameKey)
	{
		// выполнить RegSaveKey
		HREGKEY hParent = NULLHKEY;
		if ((hRc = OpenKeyEx(apParent->mh_Root, apParent->mpsz_Key, 0, KEY_ALL_ACCESS, &hParent, NULL)) != 0)
			REPlugin::CantOpenKey(apParent, TRUE);
		if (hRc == 0 && (hRc = _RegRenameKey(hParent, lpOldSubKey, lpNewSubKey)) != 0)
			REPlugin::CantLoadSaveKey(lpOldSubKey, lpNewSubKey, 2/*Rename*/);
		if (hRc == 0)
		{
			*pbRegChanged = TRUE;
		}
		// Закрыть подключ
		if (hParent != NULL)
			CloseKey(&hParent);
	}
	else if (!cfg->BackupPrivilegesAcuire(TRUE))
	{
		// Ошибка уже может быть показана в RegConfig::BackupPrivilegesAcuire
		if (GetLastError())
			REPlugin::Message(REM_NotEnoughRighsForCopy, FMSG_WARNING|FMSG_ERRORTYPE|FMSG_MB_OK);
		cfg->BackupPrivilegesRelease();
		hRc = E_ACCESSDENIED;
	}
	else
	{
		TCHAR szTemp[MAX_PATH*2+1] = _T("");
		// Сформировать имя временного файла
		FSF.MkTemp(szTemp,
			#ifdef _UNICODE
			MAX_PATH*2,
			#endif
			_T("FREG"));
		_ASSERTE(szTemp[0] != 0);
		wchar_t szTempFile[MAX_PATH*2+1]; lstrcpy_t(szTempFile, countof(szTempFile), szTemp);

		// выполнить RegSaveKey
		HREGKEY hKey = NULLHKEY, hParent = NULLHKEY;
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
		if ((hRc = OpenKeyEx(apParent->mh_Root, apParent->mpsz_Key, 0, KEY_ALL_ACCESS, &hParent, NULL)) != 0)
			REPlugin::CantOpenKey(apParent, TRUE);
		if (hRc == 0 && (hRc = OpenKeyEx(hParent, lpOldSubKey, 0, KEY_ALL_ACCESS, &hKey, NULL)) != 0)
			REPlugin::CantOpenKey(apParent, lpOldSubKey, TRUE);
		if (hRc == 0 && (hRc = SaveKey(hKey, szTempFile)) != 0)
			REPlugin::CantLoadSaveKey(lpOldSubKey, szTempFile, TRUE/*abSave*/);
		//TODO: Сохранить текущий lpSecurityAttributes
		// Закрыть подключ
		if (hKey != NULL)
			CloseKey(&hKey);
		// придется сразу удалить старый ключ, если различие только в регистре
		if (hRc == 0 && lstrcmpiW(lpNewSubKey, lpOldSubKey) == 0 && lstrcmpW(lpNewSubKey, lpOldSubKey) != 0)
		{
			if ((hRc = DeleteSubkeyTree(hParent, lpOldSubKey)) != 0)
				REPlugin::ValueOperationFailed(apParent, lpOldSubKey, TRUE/*abModify*/);
			else
				*pbRegChanged = TRUE;
		}
		// Удалять ключ с новым именем, если он уже есть, не нужно?
		// WinApi сама чистит его от было мусора (левых ключей/значений)?
		// Создаем ключ с новым именем
		if (hRc == 0 && (hRc = CreateKeyEx(hParent, lpNewSubKey, 0, NULL, 0, KEY_ALL_ACCESS, lpSecurityAttributes, &hKey, NULL, NULL)) != 0)
			REPlugin::CantOpenKey(apParent, lpNewSubKey, TRUE);
		// Загрузить содержимое из файла
		if (hRc == 0)
		{
			if ((hRc = RestoreKey(hKey, szTempFile)) != 0)
				REPlugin::CantLoadSaveKey(lpOldSubKey, szTemp, FALSE/*abSave*/);
			else
				*pbRegChanged = TRUE;
		}
		// Закрыть подключ
		if (hKey != NULL)
			CloseKey(&hKey);
		// Удалить Удалить старое дерево ключа, если оно различается регистроНЕзависимо
		if (!abCopyOnly)
		{
			if (hRc == 0 && lstrcmpiW(lpNewSubKey, lpOldSubKey) != 0) {
				if ((hRc = DeleteSubkeyTree(hParent, lpOldSubKey)) != 0)
					REPlugin::ValueOperationFailed(apParent, lpOldSubKey, TRUE/*abModify*/);
				else
					*pbRegChanged = TRUE;
			}
		}
		// Удалить временный файл
		DeleteFileW(szTempFile);
		//TODO: Восстановить права на сам ключ?
		// Закрыть текущий ключ
		if (hParent)
			CloseKey(&hParent);

		// Отпустить привилегию
		cfg->BackupPrivilegesRelease();
	}

	return hRc;
}
