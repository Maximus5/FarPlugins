
#pragma once

/*

	Оптимизация работы с кучей

*/

class MInternalMalloc
{
protected:
	// Internal optimized memory allocation
	typedef struct tag_MInternalMemoryBlock
	{
		__int64 nBlockSize, nBlockFree;
		LPBYTE ptrBlock, ptrFirstFree;
	} MInternalMemoryBlock;
	size_t nBlockDefaultSize;
	DWORD  nBlockCount;   // Количество блоков в pMemBlocks;
	int    nBlockActive;  // Активный блок
	MInternalMemoryBlock* pMemBlocks, *pMemActiveBlock;
	MInternalMemoryBlock* InitializeMalloc(size_t anDefaultBlockSize);
public:
	MInternalMalloc();
	virtual ~MInternalMalloc();
	__inline void* InternalMalloc(size_t anSize)
	{
		_ASSERTE(nBlockDefaultSize != 0 && pMemBlocks != NULL);
		//MFileRegMemoryBlock* p = pMemActiveBlock/*pMemBlocks+nBlockActive*/;
		if ((size_t)pMemActiveBlock->nBlockFree < anSize)
			InitializeMalloc(anSize);

		_ASSERTE(pMemActiveBlock->ptrBlock != NULL);
		LPBYTE ptr = pMemActiveBlock->ptrFirstFree;
		pMemActiveBlock->nBlockFree -= anSize;
		pMemActiveBlock->ptrFirstFree += anSize;
		_ASSERTE(pMemActiveBlock->nBlockFree >= 0);
		return ptr;
	};
	void ReleaseMalloc();
};
