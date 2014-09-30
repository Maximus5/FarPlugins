
#include "header.h"

#ifdef _DEBUG
//	#define FORCE_UPDATEPANEL_ON_IDLE 1
#endif

// инициализация в NULL
void RegFolder::Init(RegPath* apKey)
{
	#ifdef _DEBUG
	_ASSERTE(nDummy == 0);
	_ASSERTE(nRefCount == 1);
	#endif
	_ASSERTE(mp_Items==NULL);
	_ASSERTE(apKey->eType != RE_UNDEFINED);

	RequireItemsLock();
	
	key.Init(apKey);
	// Что
	mn_ItemCount = mn_MaxItemCount = 0;
	mp_Items = NULL;
	mp_PluginItems = NULL;
	mn_HolderLen = mn_MaxHolderSize = 0;
	mpsz_NamesHolder = NULL;
	mn_DataHolderLen = mn_MaxDataHolderSize = 0;
	mptr_DataHolder = NULL;
}

// убедиться, что выделено достаточно памяти
void RegFolder::AllocateItems(
			/*RegPath* apKey,*/ int anSubkeys, int anValues, int anMaxKeyLen, int anMaxValueNameLen)
{
	//// Сначала подготовить информацию о ключе
	//key.Release();
	//key.Init(apKey->eType, apKey->mh_Root, apKey->mpsz_Key, apKey->ftModified);
	
	RequireItemsLock();
	
	// теперь проверить, чтобы было выделено достаточно памяти под все структуры
	if (!mp_Items || !mp_PluginItems
		|| (mn_MaxItemCount < ((DWORD)anSubkeys + (DWORD)anValues)))
	{
		mn_MaxItemCount = 16 + (anSubkeys + anValues);
		SafeFree(mp_Items);
		mp_Items = (RegItem*)malloc(mn_MaxItemCount*sizeof(RegItem));
		SafeFree(mp_PluginItems);
		mp_PluginItems = (PluginPanelItem*)malloc(mn_MaxItemCount*sizeof(PluginPanelItem));
	}
	
	// и строки
	if (!mpsz_NamesHolder
		|| (mn_MaxHolderSize < (size_t)(anSubkeys*(anMaxKeyLen+1) + anValues*(anMaxValueNameLen+1)))
		)
	{
		SafeFree(mpsz_NamesHolder);
		mn_MaxHolderSize = MAX_PATH + (anSubkeys*(anMaxKeyLen+1) + anValues*(anMaxValueNameLen+1));
		mpsz_NamesHolder = (wchar_t*)malloc(mn_MaxHolderSize*sizeof(*mpsz_NamesHolder));
		mpsz_NamesHolder[0] = 0;
	}
	mn_HolderLen = 0;
}

BOOL RegFolder::AddItem(
			LPCWSTR aszName, int anNameLen, REGFILETIME aftModified, LPCTSTR asDesc, LPCTSTR asOwner,
			DWORD adwFileAttributes /*= FILE_ATTRIBUTE_DIRECTORY*/,
			REGTYPE anValueType /*= REG__KEY*/, DWORD anValueSize /*= 0*/, LPCBYTE apValueData,
			LPCWSTR apszComment /*= NULL*/)
{
	_ASSERTE(mp_Items!=NULL && mp_PluginItems!=NULL && mpsz_NamesHolder!=NULL);
	_ASSERTE(mn_MaxItemCount>0 && mn_MaxHolderSize>0);

	bool lbDefaultValue = false;
	if (!aszName || !*aszName)
	{
		//TODO: пустых ключей и значений вроде быть не может?
		aszName = REGEDIT_DEFAULTNAME;
		anNameLen = lstrlenW(aszName);
		lbDefaultValue = true;
		bHaveDefaultValue = true;
	}
	
	// По идее, такого быть не должно, 
	// но теоретически во время сканирования ключа могут быть добавлены
	// новые подключи или значения
	if (mn_ItemCount >= mn_MaxItemCount)
	{
		RequireItemsLock();
		
		_ASSERTE(mn_ItemCount == mn_MaxItemCount);
		mn_MaxItemCount += 256;
		//
		RegItem* pNewItems = (RegItem*)malloc(mn_MaxItemCount*sizeof(RegItem));
		if (mn_ItemCount > 0)
			memmove(pNewItems, mp_Items, mn_ItemCount*sizeof(RegItem));
		SafeFree(mp_Items);
		mp_Items = pNewItems;
		//
		PluginPanelItem* pNewPluginItems = (PluginPanelItem*)malloc(mn_MaxItemCount*sizeof(PluginPanelItem));
		if (mn_ItemCount > 0)
			memmove(pNewPluginItems, mp_PluginItems, mn_ItemCount*sizeof(PluginPanelItem));
		SafeFree(mp_PluginItems);
		mp_PluginItems = pNewPluginItems;
		// После копирования - изменились указатели, обновить UserData
		for (UINT i = 0; i < mn_ItemCount; i++)
		{
			SetPanelItemUserData(mp_PluginItems[i], mp_Items+i);
			//mp_Items[i].pi = (mp_PluginItems+i);
		}
	}
	
	// И этого тоже быть по идее не должно, под строки уже выделено достаточно памяти, но проверим
	if ((mn_HolderLen+anNameLen+1) > mn_MaxHolderSize)
	{
		mn_MaxHolderSize += MAX_PATH+anNameLen;
		wchar_t* pszNew = (wchar_t*)malloc(mn_MaxHolderSize*sizeof(wchar_t));
		if (mn_HolderLen > 0)
			wmemmove(pszNew, mpsz_NamesHolder, mn_HolderLen);
		SafeFree(mpsz_NamesHolder);
		// После копирования - изменились указатели, обновить UserData
		for (UINT k = 0; k < mn_ItemCount; k++)
		{
			mp_Items[k].pszName = pszNew+(mp_Items[k].pszName - mpsz_NamesHolder);
			#ifdef _UNICODE
			PanelItemFileNamePtr(mp_PluginItems[k]) = mp_Items[k].pszName;
			#endif
		}
		mpsz_NamesHolder = pszNew;
	}
	
	#ifdef _DEBUG
	if ((adwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		_ASSERTE(anValueType == REG__KEY);
	}
	#endif
	
	// Заполняем
	RegItem* pItem = (mp_Items+mn_ItemCount);
	pItem->nMagic = REGEDIT_MAGIC;
	pItem->pFolder = this;
	pItem->pszComment = apszComment;
	pItem->nItemIndex = mn_ItemCount;
	pItem->nValueType = anValueType;
	pItem->nDataSize  = anValueSize; // Для удобства
	pItem->ptrData = apValueData;
	pItem->bDefaultValue = lbDefaultValue;
	pItem->nFlags = (adwFileAttributes & REGF_ALL_MASK);
	wchar_t* pwsz = mpsz_NamesHolder + mn_HolderLen;
	_ASSERTE(anNameLen>0);
	wmemmove(pwsz, aszName, anNameLen);
	pwsz[anNameLen] = 0;
	mn_HolderLen += anNameLen + 1;
	pItem->pszName = pwsz;
	if (asDesc) {
		lstrcpyn(pItem->szDescription, asDesc, countof(pItem->szDescription));
	} else {
		pItem->szDescription[0] = 0;
	}
	pItem->szTempDescription[0] = 0;
	if (asOwner) {
		lstrcpyn(pItem->szOwner, asOwner, countof(pItem->szOwner));
	} else {
		pItem->szOwner[0] = 0;
	}
	pItem->szTempOwner[0] = 0;
	
	//pItem->pi = (mp_PluginItems+mn_ItemCount);
	SetPanelItemUserData(mp_PluginItems[mn_ItemCount], mp_Items+mn_ItemCount);

	if ((adwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !bShowKeysAsDirs)
	{
		PanelItemAttributes(mp_PluginItems[mn_ItemCount]) = (adwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY) | FILE_ATTRIBUTE_COMPRESSED;
	}
	else
	{
		PanelItemAttributes(mp_PluginItems[mn_ItemCount]) = adwFileAttributes;
	}
	PanelItemCreation(mp_PluginItems[mn_ItemCount]) = aftModified;
	PanelItemAccess(mp_PluginItems[mn_ItemCount]) = aftModified;
	PanelItemWrite(mp_PluginItems[mn_ItemCount]) = aftModified;
	#if FAR_UNICODE>=1906
	PanelItemChange(mp_PluginItems[mn_ItemCount]) = aftModified;
	#endif
	
	PanelItemFileSize(mp_PluginItems[mn_ItemCount]) = anValueSize;
	PanelItemPackSize(mp_PluginItems[mn_ItemCount]) = anValueSize;
	#ifndef _UNICODE
	mp_PluginItems[mn_ItemCount].FindData.nFileSizeHigh = 0;
	mp_PluginItems[mn_ItemCount].PackSizeHigh = 0;
	#endif
	
	#ifdef _UNICODE
	if (pwsz[0] == L'.' && pwsz[1] == L'.' && pwsz[2] == 0)
	{
		lstrcpyW(pItem->szTwoDots, REGEDIT_REPLACETWODOTS);
		PanelItemFileNamePtr(mp_PluginItems[mn_ItemCount]) = pItem->szTwoDots;
	}
	else
	{
		PanelItemFileNamePtr(mp_PluginItems[mn_ItemCount]) = pwsz;
	}
	PanelItemAltNamePtr(mp_PluginItems[mn_ItemCount]) = NULL;
	#else
	if (pwsz[0] == L'.' && pwsz[1] == L'.' && pwsz[2] == 0) {
		lstrcpyA(pItem->szTwoDots, REGEDIT_REPLACETWODOTS);
		lstrcpyA(mp_PluginItems[mn_ItemCount].FindData.cFileName, REGEDIT_REPLACETWODOTS);
	} else {
		lstrcpy_t(mp_PluginItems[mn_ItemCount].FindData.cFileName, MAX_PATH, aszName);
	}
	mp_PluginItems[mn_ItemCount].FindData.cAlternateFileName[0] = 0;
	mp_PluginItems[mn_ItemCount].FindData.dwReserved0 = 0;
	mp_PluginItems[mn_ItemCount].FindData.dwReserved1 = 0;
	#endif
	
	mp_PluginItems[mn_ItemCount].Description = pItem->szDescription;
	mp_PluginItems[mn_ItemCount].Owner = pItem->szOwner;
	
	// Обнулить неиспользуемые поля в pi
	#if FAR_UNICODE>=1906
	mp_PluginItems[mn_ItemCount].Flags = PPIF_HASNOTEXTENSION;
	#else
	mp_PluginItems[mn_ItemCount].Flags = 0;
	#endif
	mp_PluginItems[mn_ItemCount].NumberOfLinks = 0;
	mp_PluginItems[mn_ItemCount].CustomColumnData = &(mp_PluginItems[mn_ItemCount].Description);
	mp_PluginItems[mn_ItemCount].CustomColumnNumber = 1;
	mp_PluginItems[mn_ItemCount].CRC32 = 0;
	mp_PluginItems[mn_ItemCount].Reserved[0] = mp_PluginItems[mn_ItemCount].Reserved[1] = 0;

	mn_ItemCount++;
	return TRUE;
}

// сброс количества элементов, память не освобождвается
void RegFolder::Reset()
{
	RequireItemsLock();
	//key.Release(); -- низя. только в FinalRelease!
	//key.nKeyFlags &= ~REGF_NOTEMPTY;
	mn_ItemCount = 0;
	bHaveDefaultValue = false;
	mn_HolderLen = 0;
	if (mpsz_NamesHolder) mpsz_NamesHolder[0] = 0;
	mn_DataHolderLen = 0;
	mft_LastSubkey.QuadPart = 0;
	ResetRegistryChanged();
}

// Увеличить счетчик использований
int RegFolder::AddRef()
{
	nRefCount ++;
	return nRefCount;
}

// Уменьшить счетчик, если nRefCount < 1 - FinalRelease()
int RegFolder::Release()
{
	if (key.eType == RE_UNDEFINED || ((DWORD)key.eType) > ((DWORD)RE_HIVE))
	{
		_ASSERTE(key.eType != RE_UNDEFINED && ((DWORD)key.eType) <= ((DWORD)RE_HIVE));
		InvalidOp();
		return -1;
	}

	if (nRefCount > 0)
		nRefCount--;
	int i = nRefCount;
	if (nRefCount <= 0)
	{
		if (bForceRelease)
			FinalFree();
	}
	return i;
}

// освободить всю память
void RegFolder::FinalFree()
{
	if (key.eType == RE_UNDEFINED || ((DWORD)key.eType) > ((DWORD)RE_HIVE))
	{
		_ASSERTE(key.eType != RE_UNDEFINED && ((DWORD)key.eType) <= ((DWORD)RE_HIVE));
		InvalidOp();
		return;
	}
	_ASSERTE(nRefCount == 0);
	RequireItemsLock();


	if (hDescReadThread)
	{
		bRequestStopReadThread = TRUE;
		DWORD nWait = WaitForSingleObject(hDescReadThread, 500);
		if (nWait)
		{
			_ASSERTE(nWait == WAIT_OBJECT_0);
			InvalidOp();
			TerminateThread(hDescReadThread, 100);
		}
		CloseHandle(hDescReadThread); hDescReadThread = NULL;
	}	
	
	SafeDelete(args.pWorker);
	SafeFree(args.pData);
	
	key.Release();
	SafeFree(mp_Items);
	SafeFree(mp_PluginItems);
	SafeFree(mpsz_NamesHolder);
	SafeFree(mptr_DataHolder);
	mn_ItemCount = mn_MaxItemCount = 0;
	mn_HolderLen = mn_MaxHolderSize = 0;
	mn_DataHolderLen = mn_MaxDataHolderSize = 0;
	//if (mh_ChangeNotify) { CloseHandle(mh_ChangeNotify); mh_ChangeNotify = NULL; }
	
	// Сбросим, чтобы было видно, что все освободили
	key.eType = RE_UNDEFINED;
}



// Загрузить (если были изменения) список подключей и значений
BOOL RegFolder::LoadKey(REPlugin* pPlugin, MRegistryBase* pWorker, KeyFirstEnum aKeysFirst,
						BOOL abForceReload, BOOL abSilence, BOOL abLoadDesc, MRegistryBase* file /*= NULL*/,
						HKEY hKeyWrite /* = NULL*/)
{
	BOOL lbSucceeded = FALSE;
	REGFILETIME ft; SYSTEMTIME st;
	MCHKHEAP;
    HREGKEY hKey = NULLHKEY;
    WCHAR *pwszName = NULL;
	LPCWSTR pwszComment = NULL;
    DWORD dwLen = 0, dwDataSize = 0;
	REGTYPE nDataType = 0;
    TCHAR szDesc[128];
    //TCHAR szOwner[64];
    UINT i;
    TCHAR szKeyDeletedDesc[128];

#ifdef _DEBUG
	sDbgKeyPath[0] = 0;
	if (key.mpsz_Key) lstrcpynW(sDbgKeyPath, key.mpsz_Key, countof(sDbgKeyPath));
#endif
	
	// При поиске Alt-F7 - описания нафиг не нужны
	if ((abSilence || file) && abLoadDesc) abLoadDesc = FALSE;
	

	if (mn_ItemCount > 0)
	{
		// Проверить, изменился ли ключ?
		if (!abForceReload)
		{
			//#ifdef FORCE_UPDATEPANEL_ON_IDLE
			////WARNING!!! Это неправильно! только для отладки постоянного обновления панелей...
			//if (CheckRegistryChanged(pPlugin, pWorker))
			//#else
			// Если установлен флажок mb_RegistryChanged (был вызов из OnIdle) то сразу возвращает TRUE
			if (!CheckRegistryChanged(pPlugin, pWorker))
			{
			//#endif

				if (CheckLoadingFinished())
					ResetLoadingFinished();

				if (bShowKeysAsDirs != cfg->bShowKeysAsDirs)
					UpdateShowKeysAsDirs();
				lbSucceeded = TRUE;
				goto wrap;
			}
			else
			{
				abForceReload = TRUE;
			}
		}

		RequireItemsLock();

		// сброс количества элементов, память не освобождвается
		Reset();
	}
	else
	{

		RequireItemsLock();
	}
	
	lstrcpyn(szKeyDeletedDesc, GetMsg(REDeletedKey), 127);

	if (!key.mpsz_Key)
	{
		_ASSERTE(key.mpsz_Key!=NULL);
		Reset();
		lbSucceeded = FALSE;
		goto wrap;
	}
	
	// Сбросить флажок mb_RegistryChanged
	ResetRegistryChanged();

    LONG hRc = OpenKey(pWorker, &hKey);

    if (ERROR_SUCCESS != hRc)
    {
    	if (!abSilence && pPlugin)
    	{
			SetLastError(hRc);
			pPlugin->CantOpenKey(&key, FALSE);
		}
		Reset();
    }
    else
    {
		cSubKeys = 0;
		cMaxSubKeyLen = 0;
		cValues = 0;
		cMaxValueNameLen = 0;
		cMaxValueLen = 0;

		bShowKeysAsDirs = cfg->bShowKeysAsDirs;
		
        // Получить в &ft - дату изменения открытого ключа!
		// Если нет прав на ReadControl - cSubKey и cValues <- 0xcccccccc, hr = 5
        if ((hRc = pWorker->QueryInfoKey(hKey, NULL,NULL, NULL, 
        		&cSubKeys, &cMaxSubKeyLen, NULL,
        		&cValues, &cMaxValueNameLen, &cMaxValueLen,
        		NULL, &ft)) != 0)
		{
			// TODO: Показать ошибку?
        	GetSystemTime(&st); SystemTimeToFileTime(&st, (PFILETIME)&ft);
			cSubKeys = 16; cValues = 16;
        	cMaxSubKeyLen = 16383;
        	cMaxValueNameLen = 16383;
        	cMaxValueLen = 32767;
			//key.nKeyFlags &= ~REGF_NOTEMPTY;
    	}
		//else
		//{
		//	if (cValues || cSubKeys)
		//		key.nKeyFlags |= REGF_NOTEMPTY;
		//	else
		//		key.nKeyFlags &= ~REGF_NOTEMPTY;
		//}
    	cMaxSubKeyLen++; cMaxValueNameLen++;
    	if (cMaxValueLen <= 32767)
    		cMaxValueLen = 32767;
    		
    	// Обновить дату изменения в key
    	key.ftModified = ft;
    		
    	// Заранее выделить память под (cValues + cSubKeys) элементов!
		// Выделяет минимум памяти в любом случае, даже если (cSubKeys+cValues)==0
    	AllocateItems(/*&key,*/ cSubKeys, file ? 0 : cValues, cMaxSubKeyLen, file ? 0 : cMaxValueNameLen);

    	// Temp bufers
        pwszName = (WCHAR*)calloc(16384,sizeof(wchar_t));
        
        _ASSERTE(mn_ItemCount == 0);

		MCHKHEAP;
        
        for (int step = 0; step <= 1; step++)
        {
			// Загрузка списка значений
			if ((aKeysFirst != eKeysOnly) && (step == ((aKeysFirst == eValuesFirst) ? 0 : 1)))
        	{
				LPBYTE pDataHolder = NULL;
				LPBYTE pData = NULL;

				MCHKHEAP;
				//TODO: Хорошо бы определить, а нужно ли нам реально данные извлекать?
				BOOL lbStoreData = (file == NULL) && ((gnOpMode & OPM_FIND) == OPM_FIND);
				BOOL lbFastFind = FALSE;
        		BOOL lbLoadData = abLoadDesc || (file!=NULL) || lbStoreData;
				if (lbStoreData)
				{
					if (pPlugin->mb_FindInContents == 0)
					{
						// Это поиск только по имени, не по содержимому!
						lbStoreData = FALSE;
						lbFastFind = TRUE;
						lbLoadData = FALSE;
						abLoadDesc = FALSE;
					}
					else
					{
						size_t nRequestSize = max((cValues * 2),64) * cMaxValueLen;
						if (!mptr_DataHolder || mn_MaxDataHolderSize < nRequestSize)
						{
							SafeFree(mptr_DataHolder);
							mn_MaxDataHolderSize = nRequestSize;
							mn_DataHolderLen = 0;
							mptr_DataHolder = (LPBYTE)malloc(mn_MaxDataHolderSize);
							pData = mptr_DataHolder;
						}
					}
				}
				if (!pData && !lbStoreData && !lbFastFind) //TODO: Если данные не нужны - то и не выделять память
				{
					pDataHolder = (LPBYTE)calloc(cMaxValueLen ? cMaxValueLen : 1,1); // выделяем - обязательно, а там разберемся что загружать
					pData = pDataHolder;
				}

				// Если требуется хранение данных (поиск по содержимому)
		        // Сначала обрабатываем значения
		        //TODO: обработка cfg->bExportDefaultValueFirst
				bool bDefaultFirst = (cfg->bExportDefaultValueFirst && file);
				int nFrom = bDefaultFirst ? -1 : 0;
		        for (int j = nFrom;; j++)
		        {
		            dwLen = 16384;
					if (lbStoreData)
					{
						_ASSERTE(mptr_DataHolder!=NULL);
						if (mn_DataHolderLen < mn_MaxDataHolderSize)
						{
							pData = mptr_DataHolder+mn_DataHolderLen;
							// с прицелом на 64бит
							dwDataSize = min(0x7FFFFFFF, (mn_MaxDataHolderSize-mn_DataHolderLen));
						}
						else
						{
							pData = NULL;
						}
					}
					else
					{
						dwDataSize = cMaxValueLen;
						pData = pDataHolder;
					}
					szDesc[0] = 0;
					if (j == -1)
					{
						hRc = pWorker->QueryValueEx(hKey, NULL, NULL, &nDataType,
							lbLoadData ? (LPBYTE)pData : NULL, &dwDataSize, &pwszComment);
					}
					else
					{
						hRc = pWorker->EnumValue(hKey, j, pwszName, &dwLen, NULL, &nDataType,
							lbLoadData ? (LPBYTE)pData : NULL, &dwDataSize, (file!=NULL), &pwszComment);
						// Комментарии допустимы только в *.reg файлах.
						//_ASSERTE(file->eType == RE_REGFILE);
					}
					if (hRc == ERROR_ACCESS_DENIED) {
						_ASSERTE(hRc != ERROR_ACCESS_DENIED);
						break;
					}
					if (hRc == ERROR_NO_MORE_ITEMS)
					{
						_ASSERTE(hRc != ERROR_NO_MORE_ITEMS || j >= (int)cValues);
						break;
					}
		            if (hRc == ERROR_MORE_DATA)
					{
						_ASSERTE(lbStoreData == FALSE);
		            	// По идее не должно так быть, сверху мы запросили максимальный размер значения в ключе
						if (hKey != HKEY_PERFORMANCE_DATA)
						{
		            		_ASSERTE(hRc != ERROR_MORE_DATA);
						}
		            	szDesc[0] = 0;
		            	if (file != NULL && !lbStoreData)
		            	{
		            		_ASSERTE(dwDataSize > cMaxValueLen);
		            		cMaxValueLen = dwDataSize;
		            		SafeFree(pData);
		            		pData = (LPBYTE)malloc(cMaxValueLen);
							if (j == -1)
							{
								hRc = pWorker->QueryValueEx(hKey, NULL, NULL, &nDataType,
									(LPBYTE)pData, &dwDataSize);
							}
							else
							{
			            		hRc = pWorker->QueryValueEx(hKey, pwszName, NULL, &nDataType,
									(LPBYTE)pData, &dwDataSize);
							}
		            		if (hRc != 0)
		            		{
			            		_ASSERTE(hRc == 0);
			            		//TODO: показать ошибку
		            		}
		            	}
		            	else
		            	{
		            		hRc = ERROR_SUCCESS;
							pData = NULL;
	            		}
					}
					else if ((j == -1) && (hRc == ERROR_FILE_NOT_FOUND))
					{
						continue; // значения по умолчанию в этом ключе нет, переходим к обычному EnumValues
					}
					else if (hRc != ERROR_SUCCESS)
					{
						//InvalidOp();
						break;
		            }
		            else if (!file && abLoadDesc)
		            {
		            	FormatDataVisual(nDataType, pData, dwDataSize, szDesc);
		            }
		            //if (ERROR_SUCCESS != hRc)
		            //    break;

					if (bDefaultFirst && !*pwszName && j >= 0)
						continue; // 

					if (file == NULL)
					{
						// Как обычно, формируем свой каталог
		            	AddItem(pwszName, dwLen, key.ftModified, szDesc, NULL, FILE_ATTRIBUTE_NORMAL,
							nDataType, dwDataSize, lbStoreData ? pData : NULL,
							pwszComment);
						if (lbStoreData && pData)
						{
							mn_DataHolderLen += dwDataSize;
						}
	            	}
	            	else
	            	{
	            		// Выгрузка значения в *.reg
						//if (!file->FileWriteValue(pwszName, nDataType, pData, dwDataSize))
						//TODO: Обработка значение bDeletion!
						if ((nDataType & REG__INTERNAL_TYPES) && (file->eType != RE_REGFILE))
						{
							// Комментарии допустимы только в *.reg файлах.
							//--_ASSERTE(file->eType == RE_REGFILE);
							//MRegistryWinApi::SetValueEx сам об этом позаботится
						}
						if (0 != file->SetValueEx(hKeyWrite, pwszName, 0, nDataType, pData, dwDataSize, pwszComment))
						{
							//TODO: Показать ошибку
							InvalidOp();
							hRc = -1;
							break;
						}
	            	}
		        }

				SafeFree(pDataHolder);
	        }
        
			MCHKHEAP;

			// Загрузка списка подключей
	        if (step == ((aKeysFirst != eValuesFirst) ? 0 : 1))
	        {
				if ((key.nKeyFlags & REGF_DELETED))
					continue; // [-HKEY_CURRENT_USER\Software\Conemu]
				//TODO: !!! Так будут пропущены комментарии, если они есть после [-HKEY_...]?

				MCHKHEAP;
	        	_ASSERTE(!file || mn_ItemCount==0);
	        	
		        // после них - подключи
		        UINT nFirstKeyIdx = mn_ItemCount;
				DWORD nKeyFlags = 0;
		        for (i=0;; i++)
		        {
		            dwLen = 16384;
		            //OPTIMIZE: наиболее "толстый" здесь это вызов, но его не оптимизируешь
		            hRc = pWorker->EnumKeyEx(hKey, i, pwszName, &dwLen, NULL, NULL, NULL, &ft, 
		            	&nKeyFlags, szDesc, countof(szDesc), &pwszComment);
		            if (ERROR_SUCCESS != hRc)
		                break;
					// Определить самый новый ключ (для обнаружения изменений в подключах)
					if (ft.QuadPart > mft_LastSubkey.QuadPart)
						mft_LastSubkey = ft;
					AddItem(pwszName, dwLen, ft, 
						(nKeyFlags & REGF_DELETED) ? szKeyDeletedDesc : (szDesc[0] ? szDesc : NULL), NULL, 
						FILE_ATTRIBUTE_DIRECTORY|nKeyFlags,
						REG__KEY, 0, NULL, pwszComment);
		        }
		        bDescrWasUpdated = FALSE;
		        if (abLoadDesc && key.mh_Root != NULL)
		        {
		        	DWORD nStartTick = GetTickCount();
		        	DWORD nProcessed = 0; bool lbStartFailed = false;
					BOOL bAllowBackground = (key.eType == RE_WINAPI) && (cfg->nLargeKeyTimeout);
					
			        for (i = nFirstKeyIdx; i < mn_ItemCount; i++)
			        {
						if (mp_Items[i].szDescription[0] || // Уже может быть проставлено в EnumKeyEx (*.reg)
							(PanelItemAttributes(mp_PluginItems[i]) & REG_ATTRIBUTE_DELETED))
							continue; // Для "удаленных" Desc уже проставлен!
							
			    		if (0 != pWorker->GetSubkeyInfo(hKey, mp_Items[i].pszName, 
			    			mp_Items[i].szDescription, countof(mp_Items[i].szDescription),
			    			mp_Items[i].szOwner, countof(mp_Items[i].szOwner)))
			    		{
							// Значения по умолчанию нет, но поскольку это используется только для
							// отображения в колонке C0/DIZ то вернем пустую строку, чтобы плагин
							// не пытался открыть ключ и считать default value самостоятельно
							lstrcpy(mp_Items[i].szDescription, _T(" "));
							lstrcpy(mp_Items[i].szOwner, _T(" "));
			    		}
			    		
			        	//HREGKEY hSubKey = NULL;
						//// Не пытаться выполнить AjustTokenPrivileges
			        	//if (0 == pWorker->OpenKeyEx(hKey,
			        	//		mp_Items[i].pszName, 0, KEY_READ, &hSubKey, TRUE))
			        	//{
						//	if (hSubKey==NULL || hSubKey==(HREGKEY)-1)
						//	{
						//		_ASSERTE(hSubKey!=NULL && hSubKey!=(HREGKEY)-1);
						//		InvalidOp();
						//		break;
						//	}
			        	//	dwDataSize = cMaxValueLen;
			        	//	// Считать значение "по умолчанию"
			        	//	if (0 == pWorker->QueryValueEx(hSubKey, NULL, NULL, &nDataType, (LPBYTE)pData, &dwDataSize))
			        	//	{
			        	//		//#ifdef _UNICODE
			        	//		FormatDataVisual(nDataType, pData, dwDataSize, mp_Items[i].szDescription);
			        	//		//#else
			        	//		//FormatDataVisual(nDataType, pData, dwDataSize, szDesc);
			        	//		//lstrcpy_t(mp_Items[i].szDescription, countof(mp_Items[i].szDescription), szDesc);
			        	//		//#endif
			        	//	}
			        	//	pWorker->CloseKey(hSubKey);
			        	//}
			        	
			        	nProcessed++;
			        	if (bAllowBackground && nProcessed > 100)
			        	{
			        		nProcessed = 0;
			        		if (!lbStartFailed && cfg->nLargeKeyTimeout)
			        		{
				        		DWORD nDelta = GetTickCount() - nStartTick;
				        		if (cfg->nLargeKeyTimeout && nDelta > cfg->nLargeKeyTimeout)
				        		{
				        			BOOL lbStarted = CreateDescReadThread(pPlugin, pWorker);
				        			if (!lbStarted)
				        			{
				        				_ASSERTE(lbStarted);
				        				lbStartFailed = true;
			        				}
			        				else
			        				{
			        					break; // дочитывать описания будет фоновая нить
			        				}
				        		}
			        		}
			        	}
			        }
		        }
	        }
        } // for (int step = 0; step <= 1; step++)

		MCHKHEAP;
        SafeFree(pwszName);
        //SafeFree(pData);
        
		//if (cfg->bRefreshChanges && !abSilence && !file) {
		//	if (0 != pWorker->NotifyChangeKeyValue(this, hKey)) {
		//		if (mh_ChangeNotify) {
		//			CloseHandle(mh_ChangeNotify); mh_ChangeNotify = NULL;
		//		}
		//	}
		//}

		_ASSERTE(mp_Items != NULL);

		mn_LastKeyStamp = mn_LastSubkeyStamp = GetTickCount();
        
		// Закрывает только если ключ != key.mh_Root
		CloseKey(pWorker, &hKey);
    }
	
	MCHKHEAP;
	lbSucceeded = TRUE;
wrap:
	return lbSucceeded;
}

//void RegFolder::FormatDataVisual(REGTYPE nDataType, LPBYTE pData, DWORD dwDataSize, wchar_t* szDesc/*[128]*/)
//{
//	if (dwDataSize == 0) {
//		szDesc[0] = 0;
//		return;
//	}
//	
//	//TODO: Преобразовать NonPrintable (<32) в их юникодные аналоги
//	
//	//[HKEY_CURRENT_USER\Software\Far\Users\Zeroes_And_Ones\Plugins\DialogM\PluginHotkeys]
//	//"zg_case"=hex(4):31
//	if (nDataType == REG_DWORD && dwDataSize != 4)
//		nDataType = REG_BINARY;
//
//	switch(nDataType) {
//		case REG__KEY:
//		case REG__COMMENT:
//			szDesc[0] = 0;
//			break;
//		case REG_DWORD:
//			_ASSERTE(dwDataSize == 4);
//			wsprintfW(szDesc, L"0x%08X (%i)", *((DWORD*)pData), (int)*((int*)pData));
//			break;
//		case REG_SZ:
//		case REG_EXPAND_SZ:
//		{
//			// Assume always as Unicode
//			int nLen = dwDataSize>>1;
//			if (nLen > 127) nLen = 127;
//			// Делаем это с учетом "кривых строк", хранящихся в реестре БЕЗ закрывающего '\0'
//			memmove(szDesc, (wchar_t*)pData, nLen*2);
//			szDesc[nLen] = 0;
//			break;
//		}
//		case REG_MULTI_SZ:
//		{
//			int nLen = dwDataSize>>1;
//			wchar_t* pszDst = szDesc; wchar_t* pszDstEnd = szDesc+127;
//			const wchar_t* pszSrc = (wchar_t*)pData; 
//			const wchar_t* pszSrcEnd = pszSrc + nLen;
//			wchar_t wc;
//			while (pszDst < pszDstEnd && pszSrc < pszSrcEnd)
//			{
//				wc = *(pszSrc++);
//				switch (wc)
//				{
//				case 0:
//					*(pszDst++) = L' '; break;
//				default:
//					*(pszDst++) = wc;
//				}
//			}
//			*pszDst = 0;
//			break;
//		}
//		default:
//			//TODO: обработать остальные значения, а пока как binary
//		{
//			szDesc[0] = 0; wchar_t* psz = szDesc;
//			int nMax = min(41,dwDataSize);
//			for (int i = 0; i < nMax; i++) {
//				//TODO: //OPTIMIZE: !!!
//				wsprintfW(psz, L"%02X ", (DWORD)(BYTE)(pData[i]));
//				psz += 3;
//			}
//			*psz = 0;
//			if (dwDataSize == 4) {
//				wsprintfW(psz, L"(%i)", (int)*((int*)pData));
//			} else if (dwDataSize == 2) {
//				wsprintfW(psz, L"(%i)", (DWORD)*((WORD*)pData));
//			} else if (dwDataSize == 1) {
//				wsprintfW(psz, L"(%u)", (DWORD)*((BYTE*)pData));
//			}
//		}
//	}
//}

BOOL RegFolder::Transfer(REPlugin* pPlugin, MRegistryBase* pSrcWorker, RegFolder *pDstFolder, MRegistryBase* pDstWorker)
{
	BOOL lbSucceeded = TRUE;
	HREGKEY hWriteKey = NULLHKEY;
	RegPath *pDstKey = NULL;
	MCHKHEAP;

	_ASSERTE(key.mh_Root || key.eType != RE_HIVE);
	
	// Выделение памяти под чтение значений реестра
	pDstWorker->GetExportBuffer(32767);
	

	if (key.mh_Root == NULL && key.eType != RE_REGFILE && key.eType != RE_HIVE)
	{
		_ASSERTE(key.mh_Root != NULL);
		lbSucceeded = FALSE; goto wrap;
	}
	
	lbSucceeded = TRUE;
	
	// Флаги могут быть еще не обновлены и при F4 на ".." не выгрузится [ключ] в *.reg файл
	if (key.eType == RE_REGFILE && /*key.mh_Root &&*/ key.mpsz_Key && *key.mpsz_Key && !key.nKeyFlags)
	{
		HREGKEY hTest = NULLHKEY;
		if (!OpenKey(pSrcWorker, &hTest, KEY_READ))
			CloseKey(pSrcWorker, &hTest);
	}

	pDstKey = &pDstFolder->key;
	if ((key.mh_Root != NULL || (key.mpsz_Key && *key.mpsz_Key)) && (key.nKeyFlags || key.eType != RE_REGFILE))
	{
		if ((pDstKey->mh_Root != NULL) || (pDstKey->mpsz_Key && *pDstKey->mpsz_Key) || (pDstKey->eType == RE_REGFILE))
		{
			DWORD dwDeleteOption = (key.nKeyFlags & REGF_DELETED) ? REG__OPTION_CREATE_DELETED : 0;
			LONG lWriteRc = pDstWorker->CreateKeyEx(pDstKey->mh_Root, pDstKey->mpsz_Key, 0, 0,
				dwDeleteOption, KEY_WRITE, 0, &hWriteKey, 0,
				&key.nKeyFlags, FALSE, key.pszComment);
			if (0 != lWriteRc && !dwDeleteOption)
			{
				InvalidOp();
				lbSucceeded = FALSE; goto wrap;
			}
		}
		else
		{
			InvalidOp();
			lbSucceeded = FALSE; goto wrap;
		}
	}

	// В "Удаленных" ключах могут быть комментарии
	//// В "Удаленных" ключах значений быть не может!
	//if (key.bDeletion)
	//	{ lbSucceeded = FALSE; goto wrap; }

	//Reset(); -- низя. уже могли запихнуть желаемые ключи/значения!
	
	BOOL bExportSelected = (mn_ItemCount > 0);
	
	// Эта ветка активируется, если выгружается ключ целиком
	// LoadKey сразу выгрузит значения (при EnumValue)
	if (!bExportSelected)
	{
		// В принципе, хорошо бы наверное совместить считывание списка значений с их выгрузкой
		if (!LoadKey(pPlugin, pSrcWorker, eValuesFirst/*abKeysFirst*/, TRUE/*abForceReload*/, FALSE/*abSilence*/,
					 FALSE/*abLoadDesc*/, pDstWorker, hWriteKey))
		{
			if (hWriteKey)
				pDstWorker->CloseKey(&hWriteKey);
			//TODO: Показать ошибку? хотя она уже должна быть показана в функции LoadKey. Тогда надо abSilence=TRUE в нее передавать
			lbSucceeded = FALSE; goto wrap;
		}
	}

	#ifdef _DEBUG
	if (mn_ItemCount > 0)
	{
		// debug - чтобы мусора не оказалось
		_ASSERTE(mp_Items[0].pszName[0] < 0xd000);
	}
	#endif

	//int i = 0;
	if (gpProgress)
	{
		DWORD nAddKeys = 0;
		for (UINT k = 0; k < mn_ItemCount; k++)
		{
			if (mp_Items[k].nValueType == REG__KEY) nAddKeys++;
		}
		if (nAddKeys)
			gpProgress->IncAll(nAddKeys);
	}

	// Эта ветка активируется, если выгружаются избранные значения (по запросу из FAR)
	//if (mn_ItemCount > 0 && mp_Items[0].nValueType != REG__KEY) -- некорректно: mn_ItemCount меняется в LoadKey, ключи и значения могут быть вперемешку
	if (bExportSelected)
	{
	    HREGKEY hKey = NULLHKEY;
		LONG hRc = OpenKey(pSrcWorker, &hKey);
		if (hRc != 0)
		{
			//TODO: показать ошибку!
			InvalidOp();
			if (hWriteKey)
				pDstWorker->CloseKey(&hWriteKey);
			lbSucceeded = FALSE; goto wrap;
		}
		
		DWORD  cbExportBufferData = 32767;
		LPBYTE pExportBufferData = pDstWorker->GetExportBuffer(cbExportBufferData);
		if (!pExportBufferData)
		{
			_ASSERTE(pExportBufferData);
			InvalidOp();
			if (hWriteKey)
				pDstWorker->CloseKey(&hWriteKey);
			lbSucceeded = FALSE; goto wrap;
		}
		
		//WARNING!!! Тут значения и ключи могут оказаться вперемешку!
		
		// Сначала выгружаем значения
		DWORD dwLen;
		REGTYPE nDataType;
		BOOL bDefaultFirst = cfg->bExportDefaultValueFirst;
		int i = bDefaultFirst ? -1 : 0; // сначала - @
		LPCWSTR pszValueName = NULL;
		while (lbSucceeded && i < (int)mn_ItemCount)
		{
			if (i == -1)
			{
				pszValueName = NULL;
			}
			else
			{
				if (mp_Items[i].nValueType == REG__KEY)
				{
					i++; continue; // значения и ключи могут оказаться вперемешку
				}

				if (mp_Items[i].bDefaultValue)
				{
					if (bDefaultFirst) {
						i++; continue; // уже обработано первым
					}
					else
					{
						pszValueName = NULL;
					}
				}
				else
				{
					pszValueName = mp_Items[i].pszName;
				}
			}
		
			// Загрузить значение
			dwLen = cbExportBufferData;
			
			// Эта ветка активируется только в том случае, если LoadKey не вызывался (выгрузка избранных значений)
			// поэтому никакого дублирования нет
			LPCWSTR pszComment = NULL;
			hRc = pSrcWorker->QueryValueEx(hKey, pszValueName, NULL, &nDataType, (LPBYTE)pExportBufferData, &dwLen, &pszComment);
			if (hRc == ERROR_MORE_DATA)
			{
				_ASSERTE(dwLen > cbExportBufferData);
				cbExportBufferData = dwLen;
				pExportBufferData = pDstWorker->GetExportBuffer(cbExportBufferData);
				if (!pExportBufferData)
				{
					_ASSERTE(pExportBufferData);
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
				hRc = pSrcWorker->QueryValueEx(hKey, pszValueName, NULL, &nDataType, (LPBYTE)pExportBufferData, &dwLen);
			}
			if (hRc != ERROR_FILE_NOT_FOUND)
			{
				if (hRc != 0)
				{
					//TODO: Показать ошибку
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
				if (!hWriteKey)
				{
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
		
				if (0 != pDstWorker->SetValueEx(hWriteKey, pszValueName, 0, nDataType, pExportBufferData, dwLen, pszComment))
				{
					//TODO: Показать ошибку
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
			}
		
			// Следующее
			i++;
		} // while (lbSucceeded && i < (int)mn_ItemCount)
		
		// Закрывает только если ключ не совпадает с key.mh_Root
		CloseKey(pSrcWorker, &hKey);
	}


	LPCWSTR pszSubKeyName = NULL;
	if (lbSucceeded && mn_ItemCount > 0)
	{
		UINT i = 0; // Поскольку ключи и значения могут оказатьяс перемешаны - сканируем с начала
		//if (i < 0) {
		//	_ASSERTE(i >= 0);
		//} else {
		RegFolder SubKey; //memset(&SubKey, 0, sizeof(SubKey)); -- больше низя! инициализиуется конструктором!
		SubKey.Init(&key);
		SubKey.key.AllocateAddLen(MAX_PATH*2);
		wchar_t* pszSubKey = SubKey.key.mpsz_Key;
		if (*pszSubKey)
		{
			pszSubKey += lstrlenW(SubKey.key.mpsz_Key);
			*(pszSubKey++) = L'\\';
		}

		while (lbSucceeded && i < mn_ItemCount)
		{
			if (mp_Items[i].nValueType != REG__KEY)
			{
				//_ASSERTE(mp_Items[i].nValueType == REG__KEY); -- ok, ключи и значения могут оказатьяс перемешаны
				i++; continue;
			}

			pszSubKeyName = mp_Items[i].pszName;
			if (!pszSubKeyName || !*pszSubKeyName)
			{
				_ASSERTE(pszSubKeyName!=NULL && *pszSubKeyName!=NULL);
				i++; continue;
			}

			int nKeyNameLen = lstrlenW(pszSubKeyName)+1;
			memmove(pszSubKey, pszSubKeyName, nKeyNameLen*2);
			SubKey.Reset();
			SubKey.key.nKeyFlags = mp_Items[i].nFlags;
			SubKey.key.pszComment = mp_Items[i].pszComment;

			RegPath DstSubKey; memset(&DstSubKey, 0, sizeof(DstSubKey));
			DstSubKey.Init(&pDstFolder->key);
			DstSubKey.ChDir(pszSubKeyName, TRUE, NULL, TRUE);
			RegFolder DstSubFolder; // -- memset(&DstSubFolder, 0, sizeof(DstSubFolder));
			DstSubFolder.bForceRelease = TRUE;
			DstSubFolder.Init(&DstSubKey);
			DstSubKey.Release();

			if (!SubKey.Transfer(pPlugin, pSrcWorker, &DstSubFolder, pDstWorker))
				lbSucceeded = FALSE;

			DstSubFolder.Release();
			
			if (!lbSucceeded)
				break;

			i++;
		}
		
		SubKey.Release();
		//}
	}

	if (lbSucceeded && gpProgress)
		gpProgress->Step();
wrap:
	if (hWriteKey)
		pDstWorker->CloseKey(&hWriteKey);
	return lbSucceeded;
}

BOOL RegFolder::ExportToFile(REPlugin* pPlugin, MRegistryBase* pWorker, MRegistryBase* file, BOOL abUnicode)
{
	BOOL lbSucceeded = TRUE;
	HREGKEY hWriteKey = NULLHKEY;
	MCHKHEAP;

	_ASSERTE(key.mh_Root || key.eType != RE_HIVE);
	
	// Выделение памяти под чтение значений реестра
	file->GetExportBuffer(32767);
	
	//if (abUnicode)
	//	lbSucceeded = ExportToFileUnicode(pPlugin, pWorker, file);
	//else
	//	lbSucceeded = ExportToFileAnsi(pPlugin, pWorker, file);
	//
	//MCHKHEAP;
	//return lbSucceeded;

	if (key.mh_Root == NULL && key.eType != RE_REGFILE && key.eType != RE_HIVE)
	{
		_ASSERTE(key.mh_Root != NULL);
		lbSucceeded = FALSE; goto wrap;
	}
	
	lbSucceeded = TRUE;
	//wchar_t sTemp[128];
	//sTemp[0] = L'[';
	//HKeyToStringKey(key.mh_Root, sTemp+1, 40);
	//// Заголовок ключа
	//if (!file->FileWrite(sTemp))
	//	{ lbSucceeded = FALSE; goto wrap; }
	//if (key.mpsz_Key && key.mpsz_Key[0]) {
	//	if (!file->FileWrite(L"\\", 1) ||
	//		!file->FileWrite(key.mpsz_Key))
	//		{ lbSucceeded = FALSE; goto wrap; }
	//}
	//if (!file->FileWrite(L"]\r\n", 3))
	//	{ lbSucceeded = FALSE; goto wrap; }
	
	// Флаги могут быть еще не обновлены и при F4 на ".." не выгрузится [ключ] в *.reg файл
	if (key.eType == RE_REGFILE && /*key.mh_Root &&*/ key.mpsz_Key && *key.mpsz_Key && !key.nKeyFlags)
	{
		HREGKEY hTest = NULLHKEY;
		if (!OpenKey(pWorker, &hTest, KEY_READ))
			CloseKey(pWorker, &hTest);
	}

	if ((key.mh_Root != NULL || (key.mpsz_Key && *key.mpsz_Key)) && (key.nKeyFlags || key.eType != RE_REGFILE))
	{
		if (0 != file->CreateKeyEx(key.mh_Root, key.mpsz_Key, 0, 0,
			(key.nKeyFlags & REGF_DELETED) ? REG__OPTION_CREATE_DELETED : 0, 0, 0, &hWriteKey, 0,
			&key.nKeyFlags, FALSE, key.pszComment))
		{
			//InvalidOp(); -- ошибка уже должна быть показана
			lbSucceeded = FALSE; goto wrap;
		}
	}

	// В "Удаленных" ключах могут быть комментарии
	//// В "Удаленных" ключах значений быть не может!
	//if (key.bDeletion)
	//	{ lbSucceeded = FALSE; goto wrap; }

	//Reset(); -- низя. уже могли запихнуть желаемые ключи/значения!
	
	BOOL bExportSelected = (mn_ItemCount > 0);
	
	// Эта ветка активируется, если выгружается ключ целиком
	// LoadKey сразу выгрузит значения (при EnumValue)
	if (!bExportSelected)
	{
		// В принципе, хорошо бы наверное совместить считывание списка значений с их выгрузкой
		// % раньше в LoadKey не передавалось hWriteKey, хотя по идее - должно
		if (!LoadKey(pPlugin, pWorker, eValuesFirst/*abKeysFirst*/, TRUE/*abForceReload*/, FALSE/*abSilence*/, FALSE/*abLoadDesc*/, file, hWriteKey))
		{
			//TODO: Показать ошибку? хотя она уже должна быть показана в функции LoadKey. Тогда надо abSilence=TRUE в нее передавать
			lbSucceeded = FALSE; goto wrap;
		}
	}

	#ifdef _DEBUG
	if (mn_ItemCount > 0) {
		// debug - чтобы мусора не оказалось
		_ASSERTE(mp_Items[0].pszName[0] < 0xd000);
	}
	#endif

	//int i = 0;
	if (gpProgress)
	{
		DWORD nAddKeys = 0;
		for (UINT k = 0; k < mn_ItemCount; k++)
		{
			if (mp_Items[k].nValueType == REG__KEY) nAddKeys++;
		}
		if (nAddKeys)
			gpProgress->IncAll(nAddKeys);
	}

	// Эта ветка активируется, если выгружаются избранные значения (по запросу из FAR)
	//if (mn_ItemCount > 0 && mp_Items[0].nValueType != REG__KEY) -- некорректно: mn_ItemCount меняется в LoadKey, ключи и значения могут быть вперемешку
	if (bExportSelected)
	{
	    HREGKEY hKey = NULLHKEY;
		LONG hRc = OpenKey(pWorker, &hKey);
		if (hRc != 0)
		{
			//TODO: показать ошибку!
			InvalidOp();
			lbSucceeded = FALSE; goto wrap;
		}
		
		DWORD  cbExportBufferData = 32767;
		LPBYTE pExportBufferData = file->GetExportBuffer(cbExportBufferData);
		if (!pExportBufferData)
		{
			_ASSERTE(pExportBufferData);
			InvalidOp();
			lbSucceeded = FALSE; goto wrap;
		}
		
		//WARNING!!! Тут значения и ключи могут оказаться вперемешку!
		
		// Сначала выгружаем значения
		DWORD dwLen;
		REGTYPE nDataType;
		BOOL bDefaultFirst = cfg->bExportDefaultValueFirst;
		int i = bDefaultFirst ? -1 : 0; // сначала - @
		LPCWSTR pszValueName = NULL;
		while (lbSucceeded && i < (int)mn_ItemCount)
		{
			if (i == -1)
			{
				pszValueName = NULL;
			}
			else
			{
				if (mp_Items[i].nValueType == REG__KEY)
				{
					i++; continue; // значения и ключи могут оказаться вперемешку
				}

				if (mp_Items[i].bDefaultValue)
				{
					if (bDefaultFirst)
					{
						i++; continue; // уже обработано первым
					}
					else
					{
						pszValueName = NULL;
					}
				}
				else
				{
					pszValueName = mp_Items[i].pszName;
				}
			}
		
			// Загрузить значение
			dwLen = cbExportBufferData;
			
			// Эта ветка активируется только в том случае, если LoadKey не вызывался (выгрузка избранных значений)
			// поэтому никакого дублирования нет
			LPCWSTR pszComment = NULL;
			hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL, &nDataType, (LPBYTE)pExportBufferData, &dwLen, &pszComment);
			if (hRc == ERROR_MORE_DATA)
			{
				//Contents of dwLen for HKEY_PERFORMANCE_DATA is unpredictable if RegQueryValueEx fails, which is why
				//you need to increment BufferSize and use it to set dwLen.
				_ASSERTE(dwLen > cbExportBufferData || hKey == HKEY_PERFORMANCE_DATA);
				int nMaxTries = 100;
				while ((hRc == ERROR_MORE_DATA) && ((nMaxTries--) > 0))
				{
					cbExportBufferData += max(dwLen,(512<<10));
					pExportBufferData = file->GetExportBuffer(cbExportBufferData);
					if (!pExportBufferData)
					{
						_ASSERTE(pExportBufferData);
						InvalidOp();
						lbSucceeded = FALSE; break;
					}
					dwLen = cbExportBufferData;
					hRc = pWorker->QueryValueEx(hKey, pszValueName, NULL, &nDataType, (LPBYTE)pExportBufferData, &dwLen);
				}
			}
			if (hRc != ERROR_FILE_NOT_FOUND)
			{
				if (hRc != 0)
				{
					//TODO: Показать ошибку
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
				if (!hWriteKey)
				{
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
		
				//if (!file->FileWriteValue(pszValueName, nDataType, pExportBufferData, dwLen))
				if (0 != file->SetValueEx(hWriteKey, pszValueName, 0, nDataType, pExportBufferData, dwLen, pszComment))
				{
					//TODO: Показать ошибку
					InvalidOp();
					lbSucceeded = FALSE; break;
				}
			}
		
			// Следующее
			i++;
		} // while (lbSucceeded && i < (int)mn_ItemCount)
		
		// Закрывает только если ключ не совпадает с key.mh_Root
		CloseKey(pWorker, &hKey);
	}

	//if (lbSucceeded) {
	//	if (!file->FileWrite(L"\r\n", 2))
	//	{
	//		//TODO: Показать ошибку
	//		lbSucceeded = FALSE;
	//	}
	//}

	//int i = 0;
	LPCWSTR pszSubKeyName = NULL;
	//if (lbSucceeded && i < (int)mn_ItemCount)
	if (lbSucceeded && mn_ItemCount > 0)
	{
		UINT i = 0; // Поскольку ключи и значения могут оказатьяс перемешаны - сканируем с начала
		//if (i < 0) {
		//	_ASSERTE(i >= 0);
		//} else {
		RegFolder SubKey; //memset(&SubKey, 0, sizeof(SubKey)); -- больше низя! инициализиуется конструктором!
		SubKey.Init(&key);
		SubKey.key.AllocateAddLen(MAX_PATH*2);
		wchar_t* pszSubKey = SubKey.key.mpsz_Key;
		if (*pszSubKey) {
			pszSubKey += lstrlenW(SubKey.key.mpsz_Key);
			*(pszSubKey++) = L'\\';
		}

		while (lbSucceeded && i < mn_ItemCount)
		{
			if (mp_Items[i].nValueType != REG__KEY) {
				//_ASSERTE(mp_Items[i].nValueType == REG__KEY); -- ok, ключи и значения могут оказатьяс перемешаны
				i++; continue;
			}

			pszSubKeyName = mp_Items[i].pszName;
			if (!pszSubKeyName || !*pszSubKeyName) {
				_ASSERTE(pszSubKeyName!=NULL && *pszSubKeyName!=NULL);
				i++; continue;
			}

			int nKeyNameLen = lstrlenW(pszSubKeyName)+1;
			memmove(pszSubKey, pszSubKeyName, nKeyNameLen*2);
			SubKey.Reset();
			SubKey.key.nKeyFlags = mp_Items[i].nFlags;
			SubKey.key.pszComment = mp_Items[i].pszComment;

			if (!SubKey.ExportToFile(pPlugin, pWorker, file, abUnicode)) {
				lbSucceeded = FALSE; break;
			}					

			i++;
		}
		
		SubKey.Release();
		//}
	}

	if (lbSucceeded && gpProgress)
	{
		if (!gpProgress->Step())
		{
			// Значит был нажат Esc, остановить операцию
			//TODO: Спросить подтверждения у пользователя
			lbSucceeded = FALSE;
			goto wrap;
		}
	}

wrap:
	if (hWriteKey != NULL)
		file->CloseKey(&hWriteKey);
	return lbSucceeded;
}

BOOL RegFolder::CreateDescReadThread(REPlugin* pPlugin, MRegistryBase* pWorker)
{
	if (hDescReadThread != NULL) {
		_ASSERTE(hDescReadThread!=NULL);
		bRequestStopReadThread = TRUE;
		DWORD dwWait = WaitForSingleObject(hDescReadThread,500);
		if (dwWait) {
			return FALSE;
		}
		CloseHandle(hDescReadThread);
	}
	bRequestStopReadThread = FALSE; bDescrWasUpdated = FALSE;
	
	SafeDelete(args.pWorker);

	// Формируем параметры
	args.pFolder = this; args.pPlugin = pPlugin;
	args.nLastProcessedItems = 0;
	if (!args.pData || args.nMaxValueLen < cMaxValueLen) {
		SafeFree(args.pData);
		args.nMaxValueLen = cMaxValueLen ? cMaxValueLen : 1;
		args.pData = (LPBYTE)malloc(args.nMaxValueLen);
	}
	// Создаем свой Worker, чтобы не конфликтовать с плагином, который может закрыть свой экземпляр
	args.pWorker = pWorker->Duplicate();
	
	hDescReadThread = CreateThread(NULL, 0, DescReadThread, &args, 0, &nDescReadThreadId);
	
	return (hDescReadThread != NULL);
}

//HANDLE hDescReadThread; DWORD nDescReadThreadId;
DWORD RegFolder::DescReadThread(LPVOID lpParameter)
{
	MCHKHEAP;
	RegFolderThreadArg *pArgs = (RegFolderThreadArg*)lpParameter;
	RegFolder* pFolder = pArgs->pFolder;
#ifdef _UNICODE
	REPlugin* pPlugin = pArgs->pPlugin;
	_ASSERTE(pPlugin!=NULL);
#endif
	MRegistryBase* pWorker = pArgs->pWorker;
	
	// !!! Вся память и объекты освобождаются только в главной нити !!!
	
	if (pFolder->bRequestStopReadThread)
	{
		return 0; // и проверять в цикле
	}

	pFolder->bDescrWasUpdated = FALSE;

	BOOL lbSucceeded = TRUE;
	//#ifndef _UNICODE
	//wchar_t szDesc[128];
	//#endif
    DWORD lcMaxValueLen = pArgs->nMaxValueLen;
	//REGTYPE nDataType;
    LPBYTE pData = pArgs->pData; _ASSERTE(pData!=NULL && lcMaxValueLen > 0);
    LONG hRc;
    HREGKEY hKey;
    if (pFolder->key.mpsz_Key && pFolder->key.mpsz_Key[0])
	{
    	hRc = pWorker->OpenKeyEx(pFolder->key.mh_Root, pFolder->key.mpsz_Key, 0, KEY_READ, &hKey, &pFolder->key.nKeyFlags);
	}
	else
	{
		hKey = pFolder->key.mh_Root;
	}
    
	MCHKHEAP;
    for (DWORD i = 0; i < pFolder->mn_ItemCount; i++)
	{
    	if (pFolder->mp_Items[i].nValueType == REG__KEY && pFolder->mp_Items[i].szDescription[0] == 0)
    	{
    		if (0 != pWorker->GetSubkeyInfo(hKey, pFolder->mp_Items[i].pszName, 
    			pFolder->mp_Items[i].szTempDescription, countof(pFolder->mp_Items[i].szTempDescription),
    			pFolder->mp_Items[i].szTempOwner, countof(pFolder->mp_Items[i].szTempOwner)))
    		{
				// Значения по умолчанию нет, но поскольку это используется только для
				// отображения в колонке C0/DIZ то вернем пустую строку, чтобы плагин
				// не пытался открыть ключ и считать default value самостоятельно
				lstrcpy(pFolder->mp_Items[i].szTempDescription, _T(" "));
				lstrcpy(pFolder->mp_Items[i].szTempOwner, _T(" "));
    		}
    	
	    	//HREGKEY hSubKey = NULL;
			//// Не пытаться выполнить AjustTokenPrivileges
	    	//if (0 == pWorker->OpenKeyEx(hKey,
	    	//		pFolder->mp_Items[i].pszName, 0, KEY_READ, &hSubKey, TRUE))
	    	//{
	    	//	dwDataSize = lcMaxValueLen;
	    	//	// Считать значение "по умолчанию"
	    	//	if (0 == pWorker->QueryValueEx(hSubKey, NULL, NULL, &nDataType, (LPBYTE)pData, &dwDataSize))
	    	//	{
	    	//		//#ifdef _UNICODE
	    	//		FormatDataVisual(nDataType, pData, dwDataSize, pFolder->mp_Items[i].szTempDescription);
	    	//		//#else
	    	//		//FormatDataVisual(nDataType, pData, dwDataSize, szDesc);
	    	//		//lstrcpy_t(pFolder->mp_Items[i].szTempDescription, countof(pFolder->mp_Items[i].szDescription), szDesc);
	    	//		//#endif
	    	//	}
	    	//	pWorker->CloseKey(hSubKey);
	    	//}
    	}
		// Показать, сколько мы обработали
		pArgs->nLastProcessedItems = i + 1;
		// Проверка - может затребовано завершение?
		if (pFolder->bRequestStopReadThread)
		{
			lbSucceeded = FALSE;
			break;
		}
    }

	MCHKHEAP;

    if (hKey)
	{
		if (hKey != pFolder->key.mh_Root)
    		pWorker->CloseKey(&hKey);
		else
			hKey = NULL; // для четкости
	}
	pFolder->bDescrWasUpdated = lbSucceeded;
#ifdef _UNICODE
	if (lbSucceeded)
	{
		pFolder->m_SyncArg.nEvent = REGEDIT_SYNCHRO_DESC_FINISHED; pFolder->m_SyncArg.pPlugin = pPlugin;
		psi.AdvControl(PluginNumber,ACTL_SYNCHRO,FADV1988 &pFolder->m_SyncArg);
	}
#endif
	MCHKHEAP;
	return 0;
}

// Проверить, нужно ли обновить панель в фаре
BOOL RegFolder::CheckLoadingFinished(BOOL abAlwaysCopyFromTemp /*= FALSE*/)
{
	MCHKHEAP
	if (!hDescReadThread) // данные уже должны были быть скопированы из Temp в основные переменные
		return bDescrWasUpdated;
	// Если выставлена bDescrWasUpdated - значит нить уже закончила работу (а в FAR2 может выполнять Synchro)
	DWORD nWait = bDescrWasUpdated ? WAIT_OBJECT_0 : WaitForSingleObject(hDescReadThread, 0);
	if (nWait == WAIT_OBJECT_0)
	{
		CloseHandle(hDescReadThread); hDescReadThread = NULL;
		nDescReadThreadId = 0; bRequestStopReadThread = FALSE;
		SafeDelete(args.pWorker);
		SafeFree(args.pData);
	}
	else if (!abAlwaysCopyFromTemp)
	{
		return FALSE;
	}

	if (args.nLastProcessedItems == 0)
	{
		_ASSERTE(args.nLastProcessedItems != 0);
		return FALSE;
	}

	// Нить еще может трудиться - показать что она успела
	DWORD nLastProcessed = args.nLastProcessedItems;
	if (nLastProcessed > mn_ItemCount)
	{
		_ASSERTE(nLastProcessed <= mn_ItemCount);
		nLastProcessed = mn_ItemCount;
	}

	// Мы попали в основную нить, нужно применить загруженные описания в szDescription
	for (UINT i = 0; i < nLastProcessed; i++)
	{
		if (!*mp_Items[i].szDescription && *mp_Items[i].szTempDescription)
		{
			lstrcpyn(mp_Items[i].szDescription, mp_Items[i].szTempDescription, countof(mp_Items[i].szDescription));
			mp_Items[i].szTempDescription[0] = 0;
		}
		
		if (!*mp_Items[i].szOwner && *mp_Items[i].szTempOwner)
		{
			lstrcpyn(mp_Items[i].szOwner, mp_Items[i].szTempOwner, countof(mp_Items[i].szOwner));
			mp_Items[i].szTempOwner[0] = 0;
		}
	}
	
	MCHKHEAP
	return ((nWait == WAIT_OBJECT_0) && bDescrWasUpdated);
}

void RegFolder::ResetLoadingFinished()
{
	bDescrWasUpdated = FALSE;
}

// Для минимизации обращений к реестру - проверка НЕ ДОЛЖНА
// выполняться между GetFindDataW и первым вызовом OnIdle
void RegFolder::MonitorRegistryChange(BOOL abEnabled)
{
	mb_MonitorRegistryChange = abEnabled;
}

// Проверить, не изменился ли реестр
BOOL RegFolder::CheckRegistryChanged(REPlugin *pPlugin, MRegistryBase* pWorker, HKEY hKey /*= NULL*/)
{
	//if (mh_ChangeNotify) {
	//	DWORD nWait = WaitForSingleObject(mh_ChangeNotify, 0);
	//	return (nWait == WAIT_OBJECT_0);
	//} else {
	//	//TODO: Открыть ключ и проверить его ChangeTime
	//}
	MCHKHEAP;

	if (//key.eType != RE_WINAPI // Пока поддерживаем только WinApi
		key.mh_Root == NULL // Для самого корня плагина (где HKCR, HKCU, HKLM,...) ничего не делать
		|| mp_Items == NULL    // Если ключ ни разу не загружался - то проверять нечего
		)
	{
		return FALSE;
	}

	// Для минимизации обращений к реестру - проверка НЕ ДОЛЖНА
	// выполняться между GetFindDataW и первым вызовом OnIdle
	if (!mb_ForceRegistryCheck && !mb_MonitorRegistryChange)
		return FALSE;

	// Уже было обнаружено изменение в реестре, но перечитывания ключа еще не было
	if (mb_RegistryChanged)
		return TRUE;

	// Что будем проверять
	BOOL lbCheckKey = FALSE, lbCheckSubkey = FALSE;

	MCHKHEAP;

	// mb_ForceRegistryCheck устанавливается в REPlugin::LoadItems
	// после смены (перезахода) в ключ
	if (!mb_ForceRegistryCheck)
	{
		DWORD nTick = GetTickCount();
		lbCheckKey = cfg->nRefreshKeyTimeout && ((nTick - mn_LastKeyStamp) > cfg->nRefreshKeyTimeout);
		lbCheckSubkey = cfg->nRefreshSubkeyTimeout && ((nTick - mn_LastSubkeyStamp) > cfg->nRefreshSubkeyTimeout);

		if (!lbCheckKey && !lbCheckSubkey)
			return FALSE;
	} else {
		// Однократно, поэтому сразу сбросим
		mb_ForceRegistryCheck = FALSE;
		// но проверять - все
		lbCheckKey = lbCheckSubkey = TRUE;
	}

	MCHKHEAP;

	BOOL lbChanged = FALSE;
	BOOL lbKeySelfOpened = FALSE;
	REGFILETIME ft, ftKey;
	ftKey = key.ftModified;

	if (!hKey) {
		if (OpenKey(pWorker, &hKey) != 0) {
			//TODO: Показать ошибку
			return FALSE;
		}
		lbKeySelfOpened = TRUE;
	}

	
	// Сначала проверим дату изменения самого ключа (изменение его значений)
	DWORD nSubKeys = 0, nValues = 0;
	if (pWorker->QueryInfoKey(hKey, NULL, NULL, NULL, &nSubKeys, NULL, NULL, &nValues, NULL, NULL, NULL, &ft) == 0)
	{
		if (ft.QuadPart != ftKey.QuadPart)
			lbChanged = TRUE;
		mn_LastKeyStamp = GetTickCount();
		if ((nSubKeys + nValues) != mn_ItemCount)
			lbChanged = TRUE;
	}

	MCHKHEAP;

	// Проверить подключи
	if (!lbChanged && (lbCheckSubkey || mn_ItemCount < 1000))
	{
		wchar_t sKeyName[16384]; DWORD dwLen; LONG hRc;
		for (UINT i=0;; i++) {
			dwLen = 16384;
			//OPTIMIZE: это наиболее "толстый" вызов?
			hRc = pWorker->EnumKeyEx(hKey, i, sKeyName, &dwLen, NULL, NULL, NULL, &ft);
			if (ERROR_SUCCESS != hRc)
				break;
			// Если дата подключа НОВЕЕ даты ключа - считаем что есть изменение
			if (ft.QuadPart > mft_LastSubkey.QuadPart) {
				lbChanged = TRUE;
				break; // больше перебирать не нужно
			}
		}
		mn_LastSubkeyStamp = GetTickCount();
	}

	MCHKHEAP;

	// Закрыть HREGKEY, если открыли здесь
	if (lbKeySelfOpened)
		CloseKey(pWorker, &hKey);

	if (lbChanged)
		mb_RegistryChanged = TRUE;

	MCHKHEAP;
	return lbChanged;
}

void RegFolder::ResetRegistryChanged()
{
	//if (mh_ChangeNotify)
	//	ResetEvent(mh_ChangeNotify);
	mb_RegistryChanged = FALSE;
}

void RegFolder::RequireItemsLock()
{
	if (!hDescReadThread) return;
	if (nDescReadThreadId == GetCurrentThreadId()) {
		_ASSERTE(nDescReadThreadId != GetCurrentThreadId());
		return;
	}
	bRequestStopReadThread = TRUE;
	// Если выставлена bDescrWasUpdated - значит нить уже закончила работу (а в FAR2 может выполнять Synchro)
	if (!bDescrWasUpdated)
		WaitForSingleObject(hDescReadThread, INFINITE);
	CloseHandle(hDescReadThread); hDescReadThread = NULL;
	nDescReadThreadId = 0; bRequestStopReadThread = FALSE;
}

LONG RegFolder::OpenKey(MRegistryBase* pWorker, HKEY* phKey, DWORD anRights /* = KEY_READ */)
{
	if (!pWorker) {
		InvalidOp();
		return ERROR_INVALID_PARAMETER;
	}
	LONG hRc = 0;
	if ((key.mpsz_Key && key.mpsz_Key[0]) 
		|| (pWorker->bRemote && pWorker->IsPredefined(key.mh_Root))
		|| (!key.mh_Root && !(key.mpsz_Key && *key.mpsz_Key) && key.eType == RE_REGFILE))
	{
		//DWORD samDesired = anRights;
		//if (cfg->bRefreshChanges && abUseNotify)
		//	samDesired |= KEY_NOTIFY;

		hRc = -1;
		hRc = pWorker->OpenKeyEx(key.mh_Root, key.mpsz_Key, (key.nKeyFlags & REGF_DELETED) ? REG__OPTION_CREATE_DELETED : 0, anRights, phKey, &key.nKeyFlags);

		if (hRc == 0)
		{
			_ASSERTE(*phKey != NULL);
		} else {
			*phKey = NULL;
		}
	} else {
		//_ASSERTE(key.mh_Root != NULL); -- допускается. мы в корне плагина, хотим посмотреть список доступных корневых ключей реестра
		*phKey = key.mh_Root;
	}
	return hRc;
}

LONG RegFolder::CreateKey(MRegistryBase* pWorker, HKEY* phKey, DWORD anRights /*= (KEY_READ|KEY_WRITE)*/)
{
	if (!pWorker)
	{
		InvalidOp();
		return ERROR_INVALID_PARAMETER;
	}
	LONG hRc = 0;
	if (key.eType == RE_REGFILE || (key.mpsz_Key && key.mpsz_Key[0]))
	{
		hRc = pWorker->CreateKeyEx(key.mh_Root, key.mpsz_Key, 0, NULL, 0, anRights, NULL, phKey, NULL, &key.nKeyFlags);
		if (hRc == 0)
		{
			_ASSERTE(*phKey != NULL);
		}
		else
		{
			*phKey = NULL;
		}
	}
	else
	{
		_ASSERTE(key.mh_Root != NULL);
		*phKey = key.mh_Root;
	}
	return hRc;
}

void RegFolder::CloseKey(MRegistryBase* pWorker, HKEY* phKey)
{
	if (*phKey && *phKey != key.mh_Root)
		pWorker->CloseKey(phKey); 
	*phKey = NULL;
}

void RegFolder::UpdateShowKeysAsDirs()
{
	bShowKeysAsDirs = cfg->bShowKeysAsDirs;
	if (mp_Items == NULL || mp_PluginItems == NULL)
		return;

	for (DWORD i = 0; i < mn_ItemCount; i++)
	{
		if (mp_Items[i].nValueType == REG__KEY)
		{
			if (bShowKeysAsDirs)
			{
				PanelItemAttributes(mp_PluginItems[i]) |= FILE_ATTRIBUTE_DIRECTORY;
				PanelItemAttributes(mp_PluginItems[i]) &= ~FILE_ATTRIBUTE_COMPRESSED;
			}
			else
			{
				PanelItemAttributes(mp_PluginItems[i]) &= ~FILE_ATTRIBUTE_DIRECTORY;
				PanelItemAttributes(mp_PluginItems[i]) |= FILE_ATTRIBUTE_COMPRESSED;
			}
		}
	}
}
