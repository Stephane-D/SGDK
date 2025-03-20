
//------------------------------ Background palette ------------------------------------------------
PALETTE bg_pal "bga.png"


//------------------------------ Tileset for background --------------------------------------------
// This tileset contains placeholders for animated tiles and map tiles.
// The placeholders can be any tiled blocks (I used hint labels),
// but is important that the placeholders consist of unique non-repeating (including mirrored) tiles,
// as this will allow using tileset optimization without worrying that the placeholders will be altered by the optimizer.
// It is also important that the placeholder tiles are not on the same horizontal line as the empty tiles and the map tiles
// (except for the last line of placeholder tiles), otherwise, when tile optimization is enabled,
// the optimizer will remove duplicated tiles, and shift the placeholder tiles will shift.
//---------------------------------------------------------------------------------------------------
TILESET bg_tileset "bga_tileset.png" BEST ALL


//------------------------------ Background map -----------------------------------------------------
MAP map_def "bga.png" bg_tileset


//------------------------------ Animated Tilesets --------------------------------------------------
// We do not use compression here otherwise there will be a frame drops in the game during unpacking.
// We also do not use tile optimization here, as we need the tiles to be in their positions,
// but during animation, empty or duplicate tiles may appear, and optimization would remove them,
// shifting other tiles from their positions.
//---------------------------------------------------------------------------------------------------
TILESET animated_tileset_frame_0 "animated_tileset_frame_0.png" NONE NONE
TILESET animated_tileset_frame_1 "animated_tileset_frame_1.png" NONE NONE
