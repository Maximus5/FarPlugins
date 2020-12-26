HANDLE varHeap;

bool InitHeap() // run it in begin of program
{
  varHeap = GetProcessHeap();
  return (varHeap != NULL);
}

void * my_realloc(void * block, size_t size)
{
  if (!size) // size == 0
  {
    if (block) // block != NULL
      HeapFree(varHeap, 0, block);
    return NULL;
  }
  if (block) // block != NULL
    return HeapReAlloc(varHeap, HEAP_ZERO_MEMORY, block, size);
  else // block == NULL
    return HeapAlloc(varHeap, HEAP_ZERO_MEMORY, size);
}

void * operator new(size_t size)
{
  return HeapAlloc(varHeap, HEAP_ZERO_MEMORY, size);
}

void operator delete(void * block)
{
  HeapFree(varHeap, 0, block);
}

void * operator new[](size_t size)
{
  return HeapAlloc(varHeap, HEAP_ZERO_MEMORY, size);
}

void operator delete[](void * block)
{
  HeapFree(varHeap, 0, block);
}

void * memcpy(void *dst, const void *src, size_t count)
{
  void *ret = dst;

  while (count--)
  {
    *(char *)dst = *(char *)src;
    dst = (char *)dst + 1;
    src = (char *)src + 1;
  }
  return(ret);
}

void * memmove(void *dst, const void *src, size_t count)
{
  void *ret = dst;
  if (dst <= src || (char *)dst >= ((char *)src + count))
  {
    while (count--)
    {
      *(char *)dst = *(char *)src;
      dst = (char *)dst + 1;
      src = (char *)src + 1;
    }
  }
  else
  {
    dst = (char *)dst + count - 1;
    src = (char *)src + count - 1;

    while (count--)
    {
      *(char *)dst = *(char *)src;
      dst = (char *)dst - 1;
      src = (char *)src - 1;
    }
  }
  return(ret);
}

void * memset(void *dst, int val, size_t count)
{
  void *start = dst;

  while (count--)
  {
    *(char *)dst = (char)val;
    dst = (char *)dst + 1;
  }
  return(start);
}
