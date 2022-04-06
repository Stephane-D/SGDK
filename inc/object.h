/**
 *  \file object.h
 *  \brief Base object management unit.
 *  \author Stephane Dallongeville
 *  \date 02/2022
 *
 * This unit provides methods to manage objects.<br>
 * It works in concert with the <i>pool.h</i> unit which provide dynamic object allocation.<br>
 * <br>
 * The idea of <i>Object</i> is that you can use it as base structure for your own object (entity, enemy, character, whatever you want..).<br>
 * To do that the idea is to declare your new object structure by embedding the Object into it can be anonymous) and always at the first position:<pre>
 * struct entity_
 * {
 *     Object;
 *     f32 posX;
 *     f32 posY;
 *     Sprite* sprite;
 *     ...
 * };</pre>
 *
 * Doing that your Entity structure can be used through OBJ_xxx methods.
 */

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "pool.h"


/**
 * \brief Allocated state for object (mainly useful for debugging)
 */
#define OBJ_ALLOCATED       0x8000



// forward
typedef struct Object_ Object;

/**
 *  \brief
 *      Object function callback
 *
 *  \param obj
 *      Object
 */
typedef void ObjectCallback(Object* obj);

/**
 *  \brief
 *      Base object structure
 *
 *  \param internalState
 *      Object internal state, you can use it but you should preserv bit 15 as it's used internally to detect invalid object
 *  \param type
 *      Object type, can be used to recognize the underlying object / structure type.
 *  \param init
 *      Initialisation function callback, should be only called once after object creation
 *  \param update
 *      Update function callback, usually called once per frame
 *  \param end
 *      Ending function callback, should be only called once before object release
 */
typedef struct Object_
{
    u16 internalState;
    u16 type;
    ObjectCallback* init;
    ObjectCallback* update;
    ObjectCallback* end;
} Object;


/**
 *  \brief
 *      Create and allocate the new object pool (this method is an alias of #POOL_create(..))
 *
 *  \param size
 *      the capacity of the pool (in number of object)
 *  \param objectSize
 *      the size of a single object (usually you should use sizeof(Struct) here)
 *
 *  \return the new created object pool or NULL if there is not enough memory available for that.
 *
 *  \see POOL_create(..)
 */
Pool* OBJ_createObjectPool(u16 size, u16 objectSize);

/**
 *  \brief
 *      Create a new objet from the given object pool (object must extend basic #Object structure)
 *
 *  \param pool
 *      Object pool to allocate from (see pool.h unit)
 *
 *  \return the created object or NULL if an error occured (no more available object in pool or invalid pool).
 *  The returned object is initialized to 0
 *
 *  \see OBJ_release(..)
 */
Object* OBJ_create(Pool* pool);
/**
 *  \brief
 *      Release an objet from the given object pool (object must extend basic #Object structure)
 *
 *  \param pool
 *      Object pool allocator to release from (see pool.h unit)
 *  \param obj
 *      Object to release (must extend basic #Object structure)
 *
 *  \see OBJ_create(..)
 */
void OBJ_release(Pool* pool, Object* obj);

/**
 *  \brief
 *      Iterate over all active object from the given object pool and call #update() method for each of them
 *
 *  \param pool
 *      Object pool to iterate active object from
 */
void OBJ_updateAll(Pool* pool);

/**
 *  \brief
 *      Set the initialization method for the given object
 *
 *  \param obj
 *      Object to set <i>init</i> method for
 *  \param initMethod
 *      the method to set for object initialization (should be called after object creation)
 *
 *  \see OBJ_setUpdateMethod(..)
 *  \see OBJ_setEndMethod(..)
 */
void OBJ_setInitMethod(Object* obj, ObjectCallback* initMethod);
/**
 *  \brief
 *      Set the update method for the given object
 *
 *  \param obj
 *      Object to set <i>update</i> method for
 *  \param initMethod
 *      the method to set for object update (should be called once per frame)
 *
 *  \see OBJ_setInitMethod(..)
 *  \see OBJ_setEndMethod(..)
 */
void OBJ_setUpdateMethod(Object* obj, ObjectCallback* updateMethod);
/**
 *  \brief
 *      Set the ending method for the given object
 *
 *  \param obj
 *      Object to set <i>end</i> method for
 *  \param initMethod
 *      the method to set for object destruction (should be called just before object is released)
 *
 *  \see OBJ_setInitMethod(..)
 *  \see OBJ_setUpdateMethod(..)
 */
void OBJ_setEndMethod(Object* obj, ObjectCallback* endMethod);


#endif // _OBJECT_H_
