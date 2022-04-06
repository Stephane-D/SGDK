#include "config.h"
#include "types.h"

#include "pool.h"

#include "object.h"

#include "memory.h"
#include "tools.h"


static void dummyObjectMethod(Object* obj);


#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
static bool isAllocated(Object* obj)
{
    return (obj->internalState & OBJ_ALLOCATED) != 0;
}
#endif

static bool checkValid(Object* obj, char* methodName)
{
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!isAllocated(obj))
    {
        kprintf("%s: object %p is not allocated !", methodName, obj);
        return FALSE;
    }
#endif

    return TRUE;
}


Pool* OBJ_createObjectPool(u16 size, u16 objectSize)
{
    return POOL_create(size, objectSize);
}


Object* OBJ_create(Pool* pool)
{
    Object* result = POOL_allocate(pool);

    if (result == NULL) return NULL;

    // object is allocated
    result->internalState = OBJ_ALLOCATED;
    // init
    result->type = 0;
    result->init = dummyObjectMethod;
    result->update = dummyObjectMethod;
    result->end = dummyObjectMethod;

    return result;
}

void OBJ_release(Pool* pool, Object* obj)
{
    if (!checkValid(obj, "OBJ_release")) return;

    POOL_release(pool, obj);
}


void OBJ_setInitMethod(Object* obj, ObjectCallback* initMethod)
{
    if (!checkValid(obj, "OBJ_setInitMethod")) return;

    if (initMethod != NULL) obj->init = initMethod;
    else obj->init = dummyObjectMethod;
}

void OBJ_setUpdateMethod(Object* obj, ObjectCallback* updateMethod)
{
    if (!checkValid(obj, "OBJ_setUpdateMethod")) return;

    if (updateMethod != NULL) obj->update = updateMethod;
    else obj->update = dummyObjectMethod;
}

void OBJ_setEndMethod(Object* obj, ObjectCallback* endMethod)
{
    if (!checkValid(obj, "OBJ_setEndMethod")) return;

    if (endMethod != NULL) obj->end = endMethod;
    else obj->end = dummyObjectMethod;
}

void OBJ_updateAll(Pool* pool)
{
    Object** objects = (Object**) POOL_getStackEnd(pool);
    u16 num = POOL_getNumAllocated(pool);

    while(num--)
    {
        Object* object = *--objects;
        object->update(object);
    }
}


static void dummyObjectMethod(Object* obj)
{
    //
}
