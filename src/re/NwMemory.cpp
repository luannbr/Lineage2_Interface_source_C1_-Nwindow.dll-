// Global operator new/delete of NWindowDecomp.dll go through the engine allocator, like
// the stock module (its operator new calls GMalloc->Malloc(Size, L"new")): objects are
// created in one module and freed in another.
#include "ue2/UE2Core.h"

void* operator new(size_t Size) { return appMalloc((DWORD)Size, TEXT("new")); }
void* operator new[](size_t Size) { return appMalloc((DWORD)Size, TEXT("new")); }

void operator delete(void* Ptr) noexcept
{
    if (Ptr)
        appFree(Ptr);
}
void operator delete[](void* Ptr) noexcept
{
    if (Ptr)
        appFree(Ptr);
}
void operator delete(void* Ptr, size_t) noexcept
{
    if (Ptr)
        appFree(Ptr);
}
void operator delete[](void* Ptr, size_t) noexcept
{
    if (Ptr)
        appFree(Ptr);
}
