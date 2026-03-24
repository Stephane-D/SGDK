#include <genesis.h>
#include "maps.h"
#include "typedefs.h"
#include "tile.h"

// Tiles range helper struct, used to store tile indexes of the rectangle area and its size in tiles
typedef struct {
    Vect2D_u16 leftTopTileIndex;
    Vect2D_u16 rightBottomTileIndex;
    Vect2D_u16 sizeInTiles;
} TilesRectRange;

// Get tile type at given tile coordinates, return -1 if out of bounds
FORCE_INLINE s16 CollisionMap_GetTileTypeAt(u16 x, u16 y)
{
    if (x >= collision_map.w || y >= collision_map.h)
        return -1;
    return (s16) *(collision_map.tilemap + collision_map.w * y + x);
}

// Get TilesRectRange data structure at given rectangle coordinates
TilesRectRange GetTilesRectRangeAtPos(s16 left, s16 top, s16 right, s16 bottom)
{
    Vect2D_u16 entityBB_leftTopTileIndexes = GetTileIndexesAtPos((Vect2D_s16) {left, top});
    Vect2D_u16 entityBB_rightBottomTileIndex = GetTileIndexesAtPos((Vect2D_s16) {right, bottom});
    
    return (TilesRectRange) {
        .leftTopTileIndex = entityBB_leftTopTileIndexes,
        .rightBottomTileIndex = entityBB_rightBottomTileIndex,
        .sizeInTiles = (Vect2D_u16) {
            entityBB_rightBottomTileIndex.x - entityBB_leftTopTileIndexes.x,
            entityBB_rightBottomTileIndex.y - entityBB_leftTopTileIndexes.y
        }
    };
}

// Restrict moving limit of the object by solid tile in horizontal direction, return true if tile is solid
// and restricts movement, false otherwise
FORCE_INLINE static bool
RestrictMovingLimitHorizontally(GameObject *object, u16 tileIndexX, u16 tileIndexY, HorDirection horDir, AABB_s16 *movingBoundLimitX)
{
    u16 tileType = CollisionMap_GetTileTypeAt(tileIndexX, tileIndexY);
    
    // If tile is TILE_EMPTY, it doesn't restrict movement, so return immediately
    if (object->OnTileCollision)
        if (!object->OnTileCollision(object, tileType))
            return FALSE;
    
    Range_s16 wallTileBounds = GetTileBoundsInCoord(tileIndexX, tileIndexY);
    
    if (horDir == DIR_RIGHT)
    {
        if (movingBoundLimitX->right >= wallTileBounds.leftTop.x)
            movingBoundLimitX->right = wallTileBounds.leftTop.x - 1;
    }
    else
    {
        if (movingBoundLimitX->left <= wallTileBounds.rightBottom.x)
            movingBoundLimitX->left = wallTileBounds.rightBottom.x + 1;
    }
    
    return TRUE;
}

// Restrict moving limit of the object by solid tile in vertical direction, return true if tile is solid
// and restricts movement, false otherwise
FORCE_INLINE static bool
RestrictMovingLimitVertically(GameObject *object, u16 tileIndexX, u16 tileIndexY, VertDirection vertDir, AABB_s16 *movingBoundLimitY)
{
    s16 tileType = CollisionMap_GetTileTypeAt(tileIndexX, tileIndexY);
    
    // If tile is TILE_EMPTY, it doesn't restrict movement, so return immediately
    if (object->OnTileCollision)
        if (!object->OnTileCollision(object, tileType))
            return FALSE;
    
    if (vertDir == DIR_DOWN)
    {
        s16 solidTileTopPosY = (s16) GetTilePosYTop(tileIndexY);
        
        if (movingBoundLimitY->bottom >= solidTileTopPosY)
            movingBoundLimitY->bottom = solidTileTopPosY - 1;
    }
    else
    {
        s16 solidTileBottomPosY = (s16) GetTilePosYBottom(tileIndexY);
        
        if (movingBoundLimitY->top <= solidTileBottomPosY)
            movingBoundLimitY->top = solidTileBottomPosY + 1;
        
    }
    return TRUE;
}

// Restrict moving limit of the object by solid tile in diagonal direction, return true if tile is solid
// and restricts movement, false otherwise
FORCE_INLINE static void
RestrictMovingLimitDiagonally(GameObject *object, u16 tileIndexX, u16 tileIndexY, DiagDirection diagDirection, AABB_s16 *movingBoundLimitY)
{
    s16 tileType = CollisionMap_GetTileTypeAt(tileIndexX, tileIndexY);
    
    // If tile is TILE_EMPTY, it doesn't restrict movement, so return immediately
    if (object->OnTileCollision)
        if (!object->OnTileCollision(object, tileType))
            return;
    
    if (diagDirection == DIR_RIGHT_DOWN || diagDirection == DIR_LEFT_DOWN)
    {
        s16 solidTileTopPosY = (s16) GetTilePosYTop(tileIndexY);
        
        if (movingBoundLimitY->bottom >= solidTileTopPosY)
            movingBoundLimitY->bottom = (s16) (solidTileTopPosY - 1);
    }
    else
    {
        s16 solidTileBottomPosY = (s16) GetTilePosYBottom(tileIndexY);
        
        if (movingBoundLimitY->top <= solidTileBottomPosY)
            movingBoundLimitY->top = (s16) (solidTileBottomPosY + 1);
    }
}

// Update moving limit of the object by horizontal box casting, return true if at least one solid tile is detected
// and restricts movement, false otherwise
static bool GameObject_UpdateMovingLimitByHorizontalCast(GameObject *object, AABB_s16 *movingBoundLimitX, AABB_s16 castBox)
{
    TilesRectRange castedBoxTilesRectRange = GetTilesRectRangeAtPos(castBox.left, castBox.top,
                                                                    castBox.right, castBox.bottom);
    bool result = FALSE;
    
    for (u16 i = 0; i <= castedBoxTilesRectRange.sizeInTiles.y; i++)
    {
        u16 nextInColumnTileIndexY = castedBoxTilesRectRange.leftTopTileIndex.y + i;
        
        if (object->velocity.x > 0)
            result = RestrictMovingLimitHorizontally(object, castedBoxTilesRectRange.rightBottomTileIndex.x,
                                                     nextInColumnTileIndexY, DIR_RIGHT, movingBoundLimitX);
        else if (object->velocity.x < 0)
            result = RestrictMovingLimitHorizontally(object, castedBoxTilesRectRange.leftTopTileIndex.x,
                                                     nextInColumnTileIndexY, DIR_LEFT, movingBoundLimitX);
    }
    return result;
}

// Update moving limit of the object by vertical box casting, return true if at least one solid tile is detected
// and restricts movement, false otherwise
static bool GameObject_UpdateMovingLimitByVerticalCast(GameObject *object, AABB_s16 *movingBoundLimitY, AABB_s16 castBox)
{
    TilesRectRange castedBoxTilesRectRange = GetTilesRectRangeAtPos(castBox.left, castBox.top,
                                                                    castBox.right, castBox.bottom);
    bool result = FALSE;
    for (u16 i = 0; i <= castedBoxTilesRectRange.sizeInTiles.x; i++)
    {
        u16 nextInRowTileIndexX = castedBoxTilesRectRange.leftTopTileIndex.x + i;
        
        if (object->velocity.y > 0)
            result = RestrictMovingLimitVertically(object, nextInRowTileIndexX, castedBoxTilesRectRange.rightBottomTileIndex.y,
                                                   DIR_DOWN, movingBoundLimitY);
        else if (object->velocity.y < 0)
            result = RestrictMovingLimitVertically(object, nextInRowTileIndexX, castedBoxTilesRectRange.leftTopTileIndex.y,
                                                   DIR_UP, movingBoundLimitY);
    }
    return result;
}

// Update moving limit of the object by diagonal box casting, return true if at least one solid tile is detected
// and restricts movement, false otherwise
static void GameObject_UpdateMovingLimitByDiagonalCast(GameObject *object, AABB_s16 *movingBoundLimitY, AABB_s16 castBox)
{
    TilesRectRange castedBoxTilesRectRange = GetTilesRectRangeAtPos(castBox.left, castBox.top,
                                                                    castBox.right, castBox.bottom);
    if (object->velocity.x > 0 && object->velocity.y > 0)
    {
        RestrictMovingLimitDiagonally(object, castedBoxTilesRectRange.rightBottomTileIndex.x,
                                      castedBoxTilesRectRange.rightBottomTileIndex.y,
                                      DIR_RIGHT_DOWN, movingBoundLimitY);
    }
    else if (object->velocity.x < 0 && object->velocity.y > 0)
    {
        RestrictMovingLimitDiagonally(object, castedBoxTilesRectRange.leftTopTileIndex.x, castedBoxTilesRectRange.rightBottomTileIndex.y,
                                      DIR_LEFT_DOWN, movingBoundLimitY);
    }
    else if (object->velocity.x > 0 && object->velocity.y < 0)
    {
        RestrictMovingLimitDiagonally(object, castedBoxTilesRectRange.rightBottomTileIndex.x, castedBoxTilesRectRange.leftTopTileIndex.y,
                                      DIR_RIGHT_UP, movingBoundLimitY);
    }
    else if (object->velocity.x < 0 && object->velocity.y < 0)
    {
        RestrictMovingLimitDiagonally(object, castedBoxTilesRectRange.leftTopTileIndex.x, castedBoxTilesRectRange.leftTopTileIndex.y,
                                      DIR_LEFT_UP, movingBoundLimitY);
    }
}

// Restrict the object's velocity in horizontal direction according to the moving limit determined by tile collision checks
FORCE_INLINE static void GameObject_RestrictVelocityX(GameObject *object, AABB_s16 entityAabbW, AABB_s16 movingBoundLimitX)
{
    if (object->velocity.x > 0)
    {
        s16 maxRightVelocity = movingBoundLimitX.right - entityAabbW.right;
        if (object->velocity.x > maxRightVelocity)
            object->velocity.x = maxRightVelocity;
    }
    else if (object->velocity.x < 0)
    {
        s16 maxLeftVelocity = movingBoundLimitX.left - entityAabbW.left - 1;
        if (object->velocity.x < maxLeftVelocity)
            object->velocity.x = maxLeftVelocity;
    }
}

// Restrict the object's velocity in vertical direction according to the moving limit determined by tile collision checks
FORCE_INLINE static void GameObject_RestrictVelocityY(GameObject *object, AABB_s16 entityAabbW, AABB_s16 movingBoundLimitY)
{
    if (object->velocity.y > 0)
    {
        s16 maxDownVelocity = movingBoundLimitY.bottom - entityAabbW.bottom;
        if (object->velocity.y > maxDownVelocity)
            object->velocity.y = maxDownVelocity;
    }
    else if (object->velocity.y < 0)
    {
        s16 maxUpVelocity = movingBoundLimitY.top - entityAabbW.top - 1;
        if (object->velocity.y < maxUpVelocity)
            object->velocity.y = maxUpVelocity;
    }
}

// Restrict the object's velocity by solid tile collision, perform horizontal, vertical and diagonal box casts to determine the moving limits
void GameObject_RestrictVelocityByTileCollision(GameObject *object)
{
    // Calculate the world coordinates of the object's AABB
    s16 worldLeft   = object->pos.x + object->aabb.leftTop.x;
    s16 worldTop    = object->pos.y + object->aabb.leftTop.y;
    s16 worldRight  = object->pos.x + object->aabb.rightBottom.x;
    s16 worldBottom = object->pos.y + object->aabb.rightBottom.y;
    
    // Get the object's velocity
    s16 velX = object->velocity.x;
    s16 velY = object->velocity.y;
    
    // Create AABBs for the current position and the boxes casted in the direction of movement
    AABB_s16 objectAabbWorld   = {worldLeft, worldTop, worldRight, worldBottom};
    AABB_s16 castBoxHorizontal = {worldLeft + velX, worldTop, worldRight + velX, worldBottom};
    AABB_s16 castBoxVertical   = {worldLeft, worldTop + velY, worldRight, worldBottom + velY};
    AABB_s16 castedBoxDiagonal = {worldLeft + velX, worldTop + velY, worldRight + velX, worldBottom + velY};
    
    // Initialize moving limits to the maximum possible range within the collision map
    AABB_s16 movingBoundLimit = (AABB_s16) {
        .left = 0,
        .right = (s16) (collision_map.w << COORD_SHIFTER),
        .top = 0,
        .bottom= (s16) (collision_map.h << COORD_SHIFTER),
    };
    
    // First, check horizontal movement and restrict velocity if necessary
    bool limitedHorizontal = GameObject_UpdateMovingLimitByHorizontalCast(object, &movingBoundLimit, castBoxHorizontal);
    GameObject_RestrictVelocityX(object, objectAabbWorld, movingBoundLimit);
    
    // Then, check vertical movement and restrict velocity if necessary
    bool limitedVertical = GameObject_UpdateMovingLimitByVerticalCast(object, &movingBoundLimit, castBoxVertical);
    
    // Finally, perform a diagonal cast to restrict velocity if necessary (only if both horizontal and vertical movement are not already limited)
    if (!limitedHorizontal && !limitedVertical)
        GameObject_UpdateMovingLimitByDiagonalCast(object, &movingBoundLimit, castedBoxDiagonal);
    
    // After determining the final moving limits, restrict the object's velocity accordingly
    GameObject_RestrictVelocityY(object, objectAabbWorld, movingBoundLimit);
}