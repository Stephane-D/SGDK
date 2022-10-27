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
    if (result == NULL)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("POOL_create(%d, %d) error: not enough memory (free = %d, largest block = %d)", size, objectSize, MEM_getFree() & 0xFFFF, MEM_getLargestFreeBlock() & 0XFFFF);
#endif
        return NULL;
    }

    // allocate bank
    result->bank = MEM_alloc(size * objectSize);
    // can't allocate ? --> and return NULL
    if (result->bank == NULL)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("POOL_create(%d, %d) error: not enough memory (free = %d, largest block = %d)", size, objectSize, MEM_getFree() & 0xFFFF, MEM_getLargestFreeBlock() & 0XFFFF);
#endif

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
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("POOL_create(%d, %d) error: not enough memory (free = %d, largest block = %d)", size, objectSize, MEM_getFree() & 0xFFFF, MEM_getLargestFreeBlock() & 0XFFFF);
#endif

        // release bank & pool
        MEM_free(result->bank);
        MEM_free(result);

        // and return NULL
        return NULL;
    }

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("Object pool succefully created - number of object = %d - object size = %d (free memory = %d)", size, objectSize, MEM_getFree() & 0xFFFF);
#endif

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

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        kprintf("Object pool succefully destroyed - number of object = %d - object size = %d (free memory = %d)", pool->size, pool->objectSize, MEM_getFree() & 0xFFFF);
#endif
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
    --pool->free;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("POOL_allocate: object succefully created at position %d - remaining object = %d", POOL_find(pool, *pool->free), POOL_getFree(pool));
#endif

    // return it
    return *pool->free;
}

void POOL_release(Pool* pool, void* object, bool maintainCoherency)
{
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (object == NULL)
        KLog("POOL_release(): failed - trying to release a NULL object !");

    // empty pool ?
    if (pool->free >= &pool->allocStack[pool->size])
        KLog("POOL_release(): failed - pool doesn't contain any object !");
#endif

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("POOL_release: object at position %d released - remaining object = %d", POOL_find(pool, object), POOL_getFree(pool) + 1);
#endif

    // get previous
    void* prevObject = *pool->free;

    // release object
    *pool->free = object;
    pool->free++;

    // different from the one in place ?
    if (maintainCoherency && (prevObject != object))
    {
        void** s = pool->free;
        u16 i = POOL_getNumAllocated(pool);

        while(i--)
        {
            // found the original object in alloc stack ?
            if (*s == object)
            {
                // replace with the overwritten one so we can use stack iteration
                *s = prevObject;
                return;
            }

            // next
            s++;
        }
    }
}


u16 POOL_getFree(Pool* pool)
{
    return pool->free - pool->allocStack;
}

u16 POOL_getNumAllocated(Pool* pool)
{
    return pool->size - POOL_getFree(pool);
}

void** POOL_getFirst(Pool* pool)
{
    // free point on last allocated object
    return pool->free;
}

s16 POOL_find(Pool* pool, void* object)
{
    void** s = pool->free;
    u16 i = POOL_getNumAllocated(pool);

    while(i--)
    {
        // we found the object in alloc stack ? --> return its position
        if (*s == object)
            return (pool->size - 1) - (s - pool->allocStack);
        // next
        s++;
    }

    // not found
    return -1;
}

