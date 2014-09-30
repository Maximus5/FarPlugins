
#include "header.h"


/*

	Оптимизация работы с кучей

*/

MInternalMalloc::MInternalMalloc()
{
	nBlockDefaultSize = nBlockCount = 0;
	nBlockActive = -1;
	pMemBlocks = pMemActiveBlock = NULL;
}

MInternalMalloc::~MInternalMalloc()
{
	ReleaseMalloc();
}

MInternalMalloc::MInternalMemoryBlock* MInternalMalloc::InitializeMalloc(size_t anDefaultBlockSize)
{
	if (anDefaultBlockSize < 0x10000)
		anDefaultBlockSize = 0x10000;
	if (anDefaultBlockSize > nBlockDefaultSize)
		nBlockDefaultSize = anDefaultBlockSize;

	// nBlockActive в конструкторе устанавливается в -1
	nBlockActive++;
	_ASSERTE(nBlockActive >= 0);

	if (pMemBlocks && (DWORD)nBlockActive >= nBlockCount) {
		DWORD nNewCount = nBlockActive + 32;
		MInternalMemoryBlock* pNew = (MInternalMemoryBlock*)calloc(nNewCount, sizeof(MInternalMemoryBlock));
		memmove(pNew, pMemBlocks, sizeof(MInternalMemoryBlock)*nBlockCount);
		SafeFree(pMemBlocks);
		pMemBlocks = pNew;
		nBlockCount = nNewCount;
	} else if (!pMemBlocks) {
		nBlockCount = 32;
		pMemBlocks = (MInternalMemoryBlock*)calloc(nBlockCount, sizeof(MInternalMemoryBlock));
	}
	_ASSERTE(nBlockCount && pMemBlocks);

	pMemActiveBlock = (pMemBlocks+nBlockActive);
	pMemActiveBlock->nBlockSize = nBlockDefaultSize;
	pMemActiveBlock->nBlockFree = nBlockDefaultSize;
	pMemActiveBlock->ptrBlock = (LPBYTE)malloc(nBlockDefaultSize);
	pMemActiveBlock->ptrFirstFree = pMemActiveBlock->ptrBlock;
	
	return pMemActiveBlock;
}

//__inline void* MInternalMalloc::InternalMalloc(size_t anSize)
//{
//	_ASSERTE(nBlockDefaultSize != 0 && pMemBlocks != NULL);
//	//MFileRegMemoryBlock* p = pMemActiveBlock/*pMemBlocks+nBlockActive*/;
//	if ((size_t)pMemActiveBlock->nBlockFree < anSize)
//		InitializeMalloc(anSize);
//
//	_ASSERTE(pMemActiveBlock->ptrBlock != NULL);
//	LPBYTE ptr = pMemActiveBlock->ptrFirstFree;
//	pMemActiveBlock->nBlockFree -= anSize;
//	pMemActiveBlock->ptrFirstFree += anSize;
//	_ASSERTE(pMemActiveBlock->nBlockFree >= 0);
//	return ptr;
//}

void MInternalMalloc::ReleaseMalloc()
{
	if (pMemBlocks)
	{
		for (DWORD n = 0; n < nBlockCount; n++)
		{
			SafeFree(pMemBlocks[n].ptrBlock);
		}
		SafeFree(pMemBlocks);
	}
}
