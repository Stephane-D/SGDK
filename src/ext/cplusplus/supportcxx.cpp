/*
 *  c++ support functionality
 */

#include <new>
#include <cstddef>
#include <genesis.h>

void *operator new(std::size_t size) noexcept
{
    return MEM_alloc(size);
}

void *operator new[](std::size_t size) noexcept
{
    return operator new(size); // Same as regular new
}

void *operator new(std::size_t size, std::nothrow_t) noexcept
{
    return operator new(size); // Same as regular new
}

void *operator new[](std::size_t size, std::nothrow_t) noexcept
{
    return operator new(size); // Same as regular new
}

void operator delete(void *p) noexcept
{
    MEM_free(p);
}

void operator delete[](void *p) noexcept
{
    operator delete(p); // Same as regular delete
}

void operator delete(void *p, std::nothrow_t) noexcept
{
    operator delete(p); // Same as regular delete
}

void operator delete(void *p, std::size_t size) noexcept
{
    operator delete(p); // Same as regular delete
}

void operator delete[](void *p, std::size_t size) noexcept
{
    operator delete(p); // Same as regular delete
}

void operator delete[](void *p, std::nothrow_t) noexcept
{
    operator delete(p); // Same as regular delete
}



void __cxa_pure_virtual() {}
void __cxa_atexit() {}
void __cxa_throw(void *thrown_exception, void *pvtinfo, void (*dest)(void *)) {}
