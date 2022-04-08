/**
 *  \file pool.h
 *  \brief Pool object management unit.
 *  \author Stephane Dallongeville
 *  \date 02/2022
 *
 * This unit provides methods to manage dynamic object allocation.<br>
 * <br>
 * You can use <i>Pool</i> object to handle dynamic allocation from a fixed set of objects.<br>
 * For instance if you may need to handle dynamically bullets for your game and you want to have
 * at max 20 bullets, you can handle it that way:<pre>
 * Pool* bulletPool POOL_create(20, sizeof(Bullet));
 * ...
 * // create a new bullet
 * Bullet* bullet = POOL_allocate(bulletPool);
 * // check if bullet was correctly created and do your stuff..
 * if (bullet != NULL)
 * {
 *    ...
 * }
 * ...
 * // release your bullet
 * POOL_release(bulletPool, bullet);
 * </pre>
 * <i>Pool</i> object is also very useful for fast iteration over allocated objects:<pre>
 * Bullet** bullets = POOL_getFirst(bulletPool);
 * u16 num = POOL_getNumAllocated(bulletPool);
 * while(num--)
 * {
 *    Bullet* bullet = *bullets++;
 *    // do whatever you need on your bullet
 *    ...
 * }</pre>
 *
 */

#ifndef _POOL_H_
#define _POOL_H_


/**
 *  \brief
 *      Object pool allocator structure
 *
 *  \param bank
 *      bank data
 *  \param allocStack
 *      allocation stack used for fast allocate / release operation
 *  \param ree      point on first available object in the allocation stack
 *  \param objectSize
 *      size of a single object (in bytes)
 *  \param size
 *      size of the object pool (in number of object)
 */
typedef struct
{
    void* bank;
    void** allocStack;
    void** free;
    u16 objectSize;
    u16 size;
} Pool;


/**
 *  \brief
 *      Create and allocate a new object pool allocator
 *
 *  \param size
 *      the capacity of the pool (in number of object)
 *  \param objectSize
 *      the size of a single object (usually you should use sizeof(Struct) here)
 *
 *  \return the new created object pool or NULL if there is not enough memory available for that.
 *
 *  \see POOL_destroy(..)
 *  \see POOL_reset(..)
 */
Pool* POOL_create(u16 size, u16 objectSize);
/**
 *  \brief
 *      Release the specified object pool allocator
 *
 *  \param pool
 *      Object pool allocator to release
 *
 *  \see POOL_create(..)
 */
void POOL_destroy(Pool* pool);

/**
 *  \brief
 *      Reset the 'object' pool allocator
 *
 *  \param pool
 *      Object pool allocator to reset
 *  \param clear
 *      if set to TRUE then objects memory is cleared (initialized to 0)
 */
void POOL_reset(Pool* pool, bool clear);

/**
 *  \brief
 *      Allocate a new 'object' from the specified object pool
 *
 *  \param pool
 *      Object pool allocator
 *
 *  \return the allocated object or NULL if an error occured (no more available object in pool or invalid pool)
 *
 *  \see POOL_release(..)
 */
void* POOL_allocate(Pool* pool);
/**
 *  \brief
 *      Release an objet from the specified object pool
 *
 *  \param pool
 *      Object pool allocator
 *  \param obj
 *      Object to release
 *  \param maintainCoherency
 *      set it to <i>TRUE</i> if you want to keep coherency for stack iteration (#see POOL_getFirst()).<br>
 *      Set it to <i>FALSE</i> for faster release process if you don't require object iteration through alloc stack.
 *
 *  \see POOL_allocate(..)
 */
void POOL_release(Pool* pool, void* obj, bool maintainCoherency);

/**
 *  \return
 *      the number of free object in the pool
 *
 *  \param pool
 *      Object pool allocator
 */
u16 POOL_getFree(Pool* pool);
/**
 *  \return
 *      the number of allocated object in the pool
 *
 *  \param pool
 *      Object pool allocator
 */
u16 POOL_getNumAllocated(Pool* pool);

/**
 *  \return
 *      the start position of allocated objects in the alloc stack (useful to iterate over all allocated objects)
 *
 *  \param pool
 *      Object pool allocator
 *
 *  \see POOL_getNumAllocated(..)
 */
void** POOL_getFirst(Pool* pool);

/**
 *  \return
 *      the position of an object in the alloc stack or -1 if the object isn't found (useful for debug purpose mainly)
 *
 *  \param pool
 *      Object pool allocator
 *  \param obj
 *      Object to get slot position
 */
s16 POOL_find(Pool* pool, void* obj);


#endif // _POOL_H_
