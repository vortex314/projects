#include "new.h"
#define NEW
#ifdef NEW
void * operator new(size_t size)
{
  return malloc(size);
}

void operator delete(void * ptr)
{
  free(ptr);
}

void * operator new[](size_t size)
{
  return malloc(size);
}

void operator delete[](void * ptr)
{
  free(ptr);
}
#endif
int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
void __cxa_guard_abort (__guard *) {};

void __cxa_pure_virtual(void) {};
