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

// Get tile type at given tile coordinates
FORCE_INLINE s16 CollisionMap_GetTileTypeAt(u16 x, u16 y)
{
    // Check if the given tile coordinates are out of bounds of the collision map, return -1 if they are
    if (x >= collision_map.w || y >= collision_map.h)
        return -1;
    // Get the tile type at the given tile coordinates by accessing the collision map's tilemap array
    return (s16) *(collision_map.tilemap + collision_map.w * y + x);
}

// Get the tile indexes of the rectangle area defined by left-top and right-bottom coordinates,
// as well as its size in tiles
TilesRectRange GetTilesRectRangeAtPos(s16 left, s16 top, s16 right, s16 bottom)
{
    // Clamp negative coordinates to 0 to prevent u16 wrap-around in GetTileIndexesAtPos
    if (left   < 0) left   = 0;
    if (top    < 0) top    = 0;
    if (right  < 0) right  = 0;
    if (bottom < 0) bottom = 0;

    // Get tile indexes of the left-top and right-bottom corners of the rectangle
    Vect2D_u16 entityBB_leftTopTileIndexes = GetTileIndexesAtPos((Vect2D_u16) {left, top});
    Vect2D_u16 entityBB_rightBottomTileIndex = GetTileIndexesAtPos((Vect2D_u16) {right, bottom});
    
    // Calculate the size of the rectangle in tiles by subtracting the left-top tile indexes
    // from the right-bottom tile indexes and adding 1 (to include both edges)
    return (TilesRectRange) {
        .leftTopTileIndex = entityBB_leftTopTileIndexes,
        .rightBottomTileIndex = entityBB_rightBottomTileIndex,
        .sizeInTiles = (Vect2D_u16) {
            entityBB_rightBottomTileIndex.x - entityBB_leftTopTileIndexes.x + 1,
            entityBB_rightBottomTileIndex.y - entityBB_leftTopTileIndexes.y + 1
        }
    };
}

// Restrict moving limit of the object by solid tile in horizontal direction, return true if tile is solid
// and restricts movement, false otherwise
FORCE_INLINE static bool
RestrictMovingLimitHorizontally(GameObject *object, u16 tileIndexX, u16 tileIndexY, HorDirection horDir, AABB_s16 *movingBoundLimitX)
{
    // Get the tile type at the given tile coordinates, return -1 if out of bounds
    s16 tileType = CollisionMap_GetTileTypeAt(tileIndexX, tileIndexY);

    // If TILE_EMPTY or TILE_ERROR (out-of-bounds) then never restrict movement
    if (tileType == TILE_EMPTY || tileType == TILE_ERROR)
        return false;

    // Let the object decide whether this tile type restricts its movement
    if (object->OnTileCollision && !object->OnTileCollision(object, (TileType) tileType))
        return false;
    
    // Get the world coordinates of the tile bounds to restrict movement accordingly
    Range_s16 wallTileBounds = GetTileBoundsInCoord(tileIndexX, tileIndexY);
    
    // Restrict the moving limit in horizontal direction based
    // on the tile bounds and movement direction
    if (horDir == DIR_RIGHT)
    {
        // If the right of the moving limit is to the right of the left of the solid tile,
        if (movingBoundLimitX->right >= wallTileBounds.leftTop.x)
            movingBoundLimitX->right = (s16)(wallTileBounds.leftTop.x - 1);
    }
    else
    {
        // If the left of the moving limit is to the left of the right of the solid tile,
        if (movingBoundLimitX->left <= wallTileBounds.rightBottom.x)
            movingBoundLimitX->left = (s16)(wallTileBounds.rightBottom.x + 1);
    }
    
    return true;
}

// Restrict moving limit of the object by solid tile in vertical direction, return true if tile is solid
// and restricts movement, false otherwise
FORCE_INLINE static bool
RestrictMovingLimitVertically(GameObject *object, u16 tileIndexX, u16 tileIndexY, VertDirection vertDir, AABB_s16 *movingBoundLimitY)
{
    // Get the tile type at the given tile coordinates, return -1 if out of bounds
    s16 tileType = CollisionMap_GetTileTypeAt(tileIndexX, tileIndexY);
    
    // If TILE_EMPTY or TILE_ERROR (out-of-bounds) then never restrict movement
    if (tileType == TILE_EMPTY || tileType == TILE_ERROR)
        return false;

    // Let the object decide whether this tile type restricts its movement
    if (object->OnTileCollision && !object->OnTileCollision(object, (TileType) tileType))
        return false;
    
    // Get the world coordinates of the tile bounds to restrict movement accordingly
    if (vertDir == DIR_DOWN)
    {
        s16 solidTileTopPosY = (s16) GetTilePosYTop(tileIndexY);
        
        if (movingBoundLimitY->bottom >= solidTileTopPosY)
            movingBoundLimitY->bottom = (s16)(solidTileTopPosY - 1);
    }
    else
    {
        s16 solidTileBottomPosY = (s16) GetTilePosYBottom(tileIndexY);
        
        if (movingBoundLimitY->top <= solidTileBottomPosY)
            movingBoundLimitY->top = (s16)(solidTileBottomPosY + 1);
    }
    return true;
}

// Restrict moving limit of the object by solid tile in diagonal direction, return true if tile is solid
// and restricts movement, false otherwise
FORCE_INLINE static void
RestrictMovingLimitDiagonally(GameObject *object, u16 tileIndexX, u16 tileIndexY, DiagDirection diagDirection, AABB_s16 *movingBoundLimitY)
{
    // Get the tile type at the given tile coordinates, return -1 if out of bounds
    s16 tileType = CollisionMap_GetTileTypeAt(tileIndexX, tileIndexY);
    
    // If TILE_EMPTY or TILE_ERROR (out-of-bounds) then never restrict movement
    if (tileType == TILE_EMPTY || tileType == TILE_ERROR)
        return;

    // Let the object decide whether this tile type restricts its movement
    if (object->OnTileCollision && !object->OnTileCollision(object, (TileType) tileType))
        return;
    
    // Get the world coordinates of the tile bounds to restrict movement accordingly
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
    // Get the tile indexes of the rectangle area defined by the cast box and its size in tiles
    TilesRectRange castedBoxTilesRectRange = GetTilesRectRangeAtPos(castBox.left, castBox.top,
                                                                    castBox.right, castBox.bottom);
    bool result = false;
    
    // Loop through the tiles in the casted box area in horizontal direction
    // and check for solid tiles to restrict movement accordingly
    for (u16 i = 0; i < castedBoxTilesRectRange.sizeInTiles.y; i++)
    {
        u16 nextInColumnTileIndexY = castedBoxTilesRectRange.leftTopTileIndex.y + i;
        
        if (object->velocity.x > 0)
            result |= RestrictMovingLimitHorizontally(object, castedBoxTilesRectRange.rightBottomTileIndex.x,
                                                      nextInColumnTileIndexY, DIR_RIGHT, movingBoundLimitX);
        else if (object->velocity.x < 0)
            result |= RestrictMovingLimitHorizontally(object, castedBoxTilesRectRange.leftTopTileIndex.x,
                                                      nextInColumnTileIndexY, DIR_LEFT, movingBoundLimitX);
    }
    return result;
}

// Update moving limit of the object by vertical box casting, return true if at least one solid tile is detected
// and restricts movement, false otherwise
static bool GameObject_UpdateMovingLimitByVerticalCast(GameObject *object, AABB_s16 *movingBoundLimitY, AABB_s16 castBox)
{
    // Get the tile indexes of the rectangle area defined by the cast box and its size in tiles
    TilesRectRange castedBoxTilesRectRange = GetTilesRectRangeAtPos(castBox.left, castBox.top,
                                                                    castBox.right, castBox.bottom);
    bool result = false;
    
    // Loop through the tiles in the casted box area in vertical direction
    // and check for solid tiles to restrict movement accordingly
    for (u16 i = 0; i < castedBoxTilesRectRange.sizeInTiles.x; i++)
    {
        u16 nextInRowTileIndexX = castedBoxTilesRectRange.leftTopTileIndex.x + i;
        
        if (object->velocity.y > 0)
            result |= RestrictMovingLimitVertically(object, nextInRowTileIndexX, castedBoxTilesRectRange.rightBottomTileIndex.y,
                                                    DIR_DOWN, movingBoundLimitY);
        else if (object->velocity.y < 0)
            result |= RestrictMovingLimitVertically(object, nextInRowTileIndexX, castedBoxTilesRectRange.leftTopTileIndex.y,
                                                    DIR_UP, movingBoundLimitY);
    }
    return result;
}

// Update moving limit of the object by diagonal box casting,
// this is necessary to prevent corner clipping when both horizontal and vertical movement are present,
static void GameObject_UpdateMovingLimitByDiagonalCast(GameObject *object, AABB_s16 *movingBoundLimitY, AABB_s16 castBox)
{
    // Get the tile indexes of the rectangle area defined by the cast box and its size in tiles
    TilesRectRange castedBoxTilesRectRange = GetTilesRectRangeAtPos(castBox.left, castBox.top,
                                                                    castBox.right, castBox.bottom);
    // Check the diagonal movement direction and restrict movement
    // by the tile at the corresponding corner of the cast box
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
        // If the object is moving right, calculate the maximum allowed velocity
        // to prevent moving beyond the right limit
        s16 maxRightVelocity = (s16)(movingBoundLimitX.right - entityAabbW.right);
        if (object->velocity.x > maxRightVelocity)
            object->velocity.x = maxRightVelocity;
    }
    else if (object->velocity.x < 0)
    {
        // If the object is moving left, calculate the maximum allowed velocity
        // to prevent moving beyond the left limit
        s16 maxLeftVelocity = (s16)(movingBoundLimitX.left - entityAabbW.left - 1);
        if (object->velocity.x < maxLeftVelocity)
            object->velocity.x = maxLeftVelocity;
    }
}

// Restrict the object's velocity in vertical direction according to the moving limit determined by tile collision checks
FORCE_INLINE static void GameObject_RestrictVelocityY(GameObject *object, AABB_s16 entityAabbW, AABB_s16 movingBoundLimitY)
{
    if (object->velocity.y > 0)
    {
        // If the object is moving down, calculate the maximum allowed velocity
        // to prevent moving beyond the bottom limit
        s16 maxDownVelocity = (s16)(movingBoundLimitY.bottom - entityAabbW.bottom);
        if (object->velocity.y > maxDownVelocity)
            object->velocity.y = maxDownVelocity;
    }
    else if (object->velocity.y < 0)
    {
        // If the object is moving up, calculate the maximum allowed velocity
        // to prevent moving beyond the top limit
        s16 maxUpVelocity = (s16)(movingBoundLimitY.top - entityAabbW.top - 1);
        if (object->velocity.y < maxUpVelocity)
            object->velocity.y = maxUpVelocity;
    }
}

// Restrict the object's velocity by solid tile collision, perform horizontal, vertical and diagonal box casts to determine the moving limits
void GameObject_RestrictVelocityByTileCollision(GameObject *object)
{
    // Calculate the world coordinates of the object's AABB
    s16 worldLeft   = (s16)(object->pos.x + object->aabb.leftTop.x);
    s16 worldTop    = (s16)(object->pos.y + object->aabb.leftTop.y);
    s16 worldRight  = (s16)(object->pos.x + object->aabb.rightBottom.x);
    s16 worldBottom = (s16)(object->pos.y + object->aabb.rightBottom.y);
    
    // Get the object's velocity
    s16 velX = object->velocity.x;
    s16 velY = object->velocity.y;
    
    // Create AABBs for the current position and the boxes casted in the direction of movement
    AABB_s16 objectAabbWorld   = {worldLeft, worldTop, worldRight, worldBottom};
    AABB_s16 castBoxHorizontal = {(s16)(worldLeft + velX), worldTop, (s16)(worldRight + velX), worldBottom};
    AABB_s16 castBoxVertical   = {worldLeft, (s16)(worldTop + velY), worldRight, (s16)(worldBottom + velY)};
    AABB_s16 castBoxDiagonal   = {(s16)(worldLeft + velX), (s16)(worldTop + velY), (s16)(worldRight + velX), (s16)(worldBottom + velY)};
    
    // Initialize moving limits to the maximum possible range within the collision map
    AABB_s16 movingBoundLimit = (AABB_s16) {
        .left = 0,
        .right =  (s16) (collision_map.w << COORD_SHIFTER),
        .top =  0,
        .bottom = (s16) (collision_map.h << COORD_SHIFTER),
    };
    
    // First, check horizontal movement and restrict velocity if necessary
    // by performing a horizontal box cast in the direction of movement
    bool limitedHorizontal = GameObject_UpdateMovingLimitByHorizontalCast(object, &movingBoundLimit, castBoxHorizontal);
    
    // Restrict velocity in horizontal direction according to the moving limit determined by the horizontal box cast
    GameObject_RestrictVelocityX(object, objectAabbWorld, movingBoundLimit);
    
    // Then, check vertical movement and restrict velocity if necessary
    // by performing a vertical box cast in the direction of movement
    bool limitedVertical = GameObject_UpdateMovingLimitByVerticalCast(object, &movingBoundLimit, castBoxVertical);
    
    // Finally, if both horizontal and vertical movement are present,
    // perform a diagonal box cast to prevent corner clipping (only if both horizontal and vertical movement are not already limited)
    if (!limitedHorizontal && !limitedVertical)
        GameObject_UpdateMovingLimitByDiagonalCast(object, &movingBoundLimit, castBoxDiagonal);
    
    // Restrict velocity in vertical direction according to the final moving limit determined by all box casts
    GameObject_RestrictVelocityY(object, objectAabbWorld, movingBoundLimit);
}
