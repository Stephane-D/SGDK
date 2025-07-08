
//------------------------------ Background palette ------------------------------------------------
// Defines the color palette for the background using "bga.png"
PALETTE bg_pal "bga.png"


//------------------------------ Background Tileset -----------------------------------------------
// This tileset contains placeholders for animated tiles and map tiles.
// Placeholders can be any tiled blocks (hint labels were used in this case).
// Important considerations:
// 1. All placeholder tiles must be unique and non-repeating (including mirrored versions)
//    This ensures safe tileset optimization without unwanted placeholder modifications
// 2. Placeholder tiles must not share horizontal lines with empty tiles or map tiles
//    Otherwise, tile optimization might remove duplicates and cause placeholder positions to shift
//-------------------------------------------------------------------------------------------------
TILESET bg_tileset "bga_tileset.png" BEST ALL


//------------------------------ Background map -----------------------------------------------------
// Creates background map using "bga.png" with the specified tileset
MAP map_def "bga.png" bg_tileset


//------------------------------ Animated Tilesets -------------------------------------------------
// Notes on compression and optimization:
// 1. Compression is disabled to prevent game frame drops during unpacking
// 2. Tile optimization is disabled because:
//    - We need to maintain exact tile positions
//    - Animation may create empty/duplicate tiles that optimization would remove
//    - This would cause other tiles to shift from their intended positions
//--------------------------------------------------------------------------------------------------
TILESET animated_tileset_frame_0 "animated_tileset_frame_0.png" NONE NONE
TILESET animated_tileset_frame_1 "animated_tileset_frame_1.png" NONE NONE
