#ifndef HEADER_TILE
#define HEADER_TILE

#define COORD_SHIFTER      3
#define MAP_TILE_SIZE      8

// Get left edge of tile in pixels
FORCE_INLINE u16 GetTilePosXLeft(u16 x)
{
    return (x << COORD_SHIFTER);
}

// Get right edge of tile in pixels
FORCE_INLINE u16 GetTilePosXRight(u16 x)
{
    return (x << COORD_SHIFTER) + MAP_TILE_SIZE;
}

// Get top edge of tile in pixels
FORCE_INLINE u16 GetTilePosYTop(u16 y)
{
    return (y << COORD_SHIFTER);
}

// Get bottom edge of tile in pixels
FORCE_INLINE u16 GetTilePosYBottom(u16 y)
{
    return (y << COORD_SHIFTER) + MAP_TILE_SIZE;
}

// Get tile indexes at pixels position
FORCE_INLINE Vect2D_u16 GetTileIndexesAtPos(Vect2D_s16 position)
{
    return (Vect2D_u16) {(position.x >> COORD_SHIFTER), (position.y >> COORD_SHIFTER)};
}

// Get tile indexes at pixels position
FORCE_INLINE Range_s16 GetTileBoundsInCoord(u16 indexX, u16 indexY)
{
    s16 x = (s16) (indexX << COORD_SHIFTER);
    s16 y = (s16) (indexY << COORD_SHIFTER);
    return (Range_s16) {{x, y}, {(s16) (x + MAP_TILE_SIZE), (s16) (y + MAP_TILE_SIZE)}};
}

#endif //HEADER_TILE
