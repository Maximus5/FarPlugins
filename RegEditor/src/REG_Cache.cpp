
#include "header.h"

//RegFolderCache *gpCache = NULL;

RegFolderCache::RegFolderCache()
{
	mn_RefCount = mn_ItemCount = mn_MaxItemCount = 0;
	mp_Cache = NULL;
	//memset(Plugins, 0, sizeof(Plugins));
}

RegFolderCache::~RegFolderCache()
{
	FreeItems();
}

//void RegFolderCache::AddRef(REPlugin* pPlugin)
//{
//	mn_RefCount++;
//	for (UINT i = 0; i < countof(Plugins); i++) {
//		if (Plugins[i] == NULL) {
//			Plugins[i] = pPlugin; break;
//		}
//	}
//}
//
//void RegFolderCache::Release(REPlugin* pPlugin)
//{
//	for (UINT i = 0; i < countof(Plugins); i++) {
//		if (Plugins[i] == pPlugin) {
//			Plugins[i] = NULL; break;
//		}
//	}
//	if (mn_RefCount > 0)
//		mn_RefCount--;
//	if (mn_RefCount == 0)
//		FreeItems();
//}

RegFolder* RegFolderCache::GetFolder(RegPath* apKey, u64 OpMode)
{
	if (apKey->eType == RE_UNDEFINED)
	{
		_ASSERTE(apKey->eType != RE_UNDEFINED);
		InvalidOp();
		return NULL;
	}

	int nFreeIdx = -1;
	// Может он уже есть?
	if (mp_Cache) {
 		for (int i = 0; i < mn_ItemCount; i++) {
 			if (mp_Cache[i]->key.eType == RE_UNDEFINED)
 			{
 				if (nFreeIdx == -1)
 					nFreeIdx = i;
 				continue;
			}
 			if (mp_Cache[i]->key.IsEqual(apKey))
 			{
				mp_Cache[i]->AddRef();
 				return (mp_Cache[i]);
 			}
 		}
	}
	
	// Нужно создать!
	
	if (!mp_Cache || (nFreeIdx == -1 && mn_ItemCount >= mn_MaxItemCount)) {
		// Выделить доп. память под структуры
		_ASSERTE(mn_ItemCount <= mn_MaxItemCount);
		mn_MaxItemCount += 256;
		RegFolder **pNew = (RegFolder**)calloc(mn_MaxItemCount,sizeof(RegFolder*));
		if (mn_ItemCount > 0)
			memmove(pNew, mp_Cache, mn_ItemCount*sizeof(RegFolder*));
		SafeFree(mp_Cache);
		mp_Cache = pNew;
	}
	
	int nIdx = 0;
	if (nFreeIdx != -1)
		nIdx = nFreeIdx;
	else
		nIdx = mn_ItemCount++;
	// На всякий случай - обнуляем
	if (mp_Cache[nIdx] == NULL)
		mp_Cache[nIdx] = new RegFolder;
	else
		mp_Cache[nIdx]->AddRef();
	mp_Cache[nIdx]->Init(apKey);
	mp_Cache[nIdx]->bForceRelease = (OpMode & OPM_FIND) == OPM_FIND; // Реально освободить при запросе из Фар
	return (mp_Cache[nIdx]);
}

void RegFolderCache::FreeItems()
{
	if (mp_Cache) {
 		for (int i = 0; i < mn_ItemCount; i++) {
			if (mp_Cache[i])
			{
				if (mp_Cache[i]->key.eType != RE_UNDEFINED)
 					mp_Cache[i]->Release();
				SafeDelete(mp_Cache[i]);
			}
 		}
 		SafeFree(mp_Cache);
	}
	mn_ItemCount = mn_MaxItemCount = 0;
}

RegFolder* RegFolderCache::FindByPanelItems(struct PluginPanelItem *PanelItem)
{
	for (int i = 0; i < mn_ItemCount; i++)
	{
		if (mp_Cache[i]->key.eType == RE_UNDEFINED)
			continue; // ячейка не занята
		if (mp_Cache[i]->mp_PluginItems == PanelItem)
		{
			return (mp_Cache[i]);
		}
	}
	return NULL;
}

//void RegFolderCache::UpdateAllTitles()
//{
//	for (UINT i = 0; i < countof(Plugins); i++) {
//		if (Plugins[i]) {
//			Plugins[i]->m_Key.Update();
//		}
//	}
//}
