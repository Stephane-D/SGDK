#include "config.h"
#include "types.h"

#include "pool.h"

#include "memory.h"
#include "tools.h"


Pool* POOL_create(u16 size, u16 objectSize)
{
    // create the pool
    Pool* result = MEM_alloc(sizeof(Pool));

    // error on allocation --> return NULL
    if (result == NULL) return NULL;

    // allocate bank
    result->bank = MEM_alloc(size * objectSize);
    // can't allocate ? --> and return NULL
    if (result->bank == NULL)
    {
        // release pool
        MEM_free(result);

        // and return NULL
        return NULL;
    }

    // allocate stack
    result->allocStack = MEM_alloc(size * sizeof(void*));
    // can't allocate ?
    if (result->allocStack == NULL)
    {
        // release bank & pool
        MEM_free(result->bank);
        MEM_free(result);

        // and return NULL
        return NULL;
    }

    // set size
    result->size = size;
    result->objectSize = objectSize;

    POOL_reset(result, TRUE);

    return result;
}

void POOL_reset(Pool* pool, bool clear)
{
    u16 i;
    u16 os;
    u8* d;
    void** s;

    // clear bank
    if (clear)
        memset(pool->bank, 0, pool->size * pool->objectSize);

    // reset allocation stack
    s = pool->allocStack;
    d = pool->bank;
    os = pool->objectSize;
    i = pool->size;
    while(i--)
    {
        // set alloc stack pointer
        *s++ = d;
        // point on next object
        d += os;
    }

    // init 'first' position (point at the end of alloc stack)
    pool->free = s;
}

void POOL_destroy(Pool* pool)
{
    if (pool)
    {
        if (pool->allocStack)
        {
            // release alloc stack / bank and the pool itself
            MEM_free(pool->allocStack);
            pool->allocStack = NULL;
        }
        if (pool->bank)
        {
            MEM_free(pool->bank);
            pool->bank = NULL;
        }

        MEM_free(pool);
    }
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    else
    {
        kprintf("POOL_destroy(): error - trying to destroy a not allocated pool !");
    }
#endif
}


void* POOL_allocate(Pool* pool)
{
    // pool is empty ?
    if (pool->free == pool->allocStack)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        KLog("POOL_allocate(): failed - no more available object !");
#endif

        return NULL;
    }

    // allocate object
    return *--(pool->free);
}

void POOL_release(Pool* pool, void* obj)
{
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (obj == NULL)
    {
        KLog("POOL_release(): failed - trying to release a NULL object !");
        return;
    }

    // above
    if (pool->free >= &pool->allocStack[pool->size])
    {
        KLog("POOL_release(): failed - pool doesn't contain any object !");
        return;
    }
#endif

    // release object
    *(pool->free)++ = obj;
}


u16 POOL_getFree(Pool* pool)
{
    return pool->free - pool->allocStack;
}

u16 POOL_getNumAllocated(Pool* pool)
{
    return pool->size - POOL_getFree(pool);
}

void** POOL_getStackEnd(Pool* pool)
{
    return pool->allocStack + pool->size;
}

