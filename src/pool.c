#include "config.h"
#include "types.h"

#include "pool.h"

#include "memory.h"
#include "tools.h"


Pool* POOL_create(u16 size, u16 objectSize)
{
    // word align object size
    const u16 adjObjectSize = (objectSize + 1) & 0xFFFE;
    // create the pool
    Pool* result = MEM_alloc(sizeof(Pool));

    // error on allocation --> return NULL
    if (result == NULL)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("POOL_create(%d, %d) error: not enough memory (free = %d, largest block = %d)", size, adjObjectSize, MEM_getFree() & 0xFFFF, MEM_getLargestFreeBlock() & 0XFFFF);
#endif
        return NULL;
    }

    // allocate bank (we use objectSize + 2 as we store the object index before the object)
    result->bank = MEM_alloc(size * (adjObjectSize + 2));
    // can't allocate ? --> and return NULL
    if (result->bank == NULL)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("POOL_create(%d, %d) error: not enough memory (free = %d, largest block = %d, required = %d)", size, adjObjectSize, MEM_getFree() & 0xFFFF, MEM_getLargestFreeBlock() & 0XFFFF, size * (adjObjectSize + 2));
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
        kprintf("POOL_create(%d, %d) error: not enough memory (free = %d, largest block = %d)", size, adjObjectSize, MEM_getFree() & 0xFFFF, MEM_getLargestFreeBlock() & 0XFFFF);
#endif

        // release bank & pool
        MEM_free(result->bank);
        MEM_free(result);

        // and return NULL
        return NULL;
    }

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("Object pool succefully created - number of object = %d - object size = %d (free memory = %d)", size, adjObjectSize, MEM_getFree() & 0xFFFF);
#endif

    // set size
    result->size = size;
    result->objectSize = adjObjectSize;

    POOL_reset(result, TRUE);

    return result;
}

void POOL_reset(Pool* pool, bool clear)
{
    u16 i;
    u16 ind;
    u16 os;
    u16* d;
    void** s;

    // clear bank
    if (clear)
        memset(pool->bank, 0, pool->size * (pool->objectSize + 2));

    // reset allocation stack
    s = pool->allocStack;
    d = pool->bank;
    // as we use u16 pointer
    os = pool->objectSize >> 1;
    i = pool->size;
    ind = 0;
    while(i--)
    {
        // store the object index before the object
        *d++ = ind++;
        // set current object pointer
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
        // get object index
        u16* objectIndexP = ((u16*)object) - 1;
        u16 objectIndex = *objectIndexP;

        // replace with the overwritten one so we can use stack iteration
        pool->free[objectIndex] = prevObject;

        // get previous object index
        u16* prevObjectIndexP = ((u16*)prevObject) - 1;
        u16 prevObjectIndex = *prevObjectIndexP;

        // swap indexes
        *prevObjectIndexP = objectIndex;
        *objectIndexP = prevObjectIndex;
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

