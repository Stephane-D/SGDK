
//------------------------------ Background palette -----------------------------------------------------------------------
PALETTE bg_pal "bga.png"


//------------------------------ Tileset for background -------------------------------------------------------------------
// This tileset contains map tiles and a placeholders for animated tiles.
// The placeholders can be any tiles or block of tiles (I used hint labels),
// but is important that the placeholders consist of unique non-repeating tiles and must not contain mirrored,
// flipped or empty tiles, as this will allow using tileset optimization without worrying that the
// placeholders will be altered by the optimizer.
// It is also important that the placeholder tiles are not on the same horizontal line as the empty tiles
// and the map tiles (except for the last line of placeholder tiles on the right side of which empty tiles or
// map tiles are allowed), otherwise, when tile optimization is enabled, the optimizer will remove 
// duplicated tiles, and the placeholder tiles will be shifted.
// You can create such a tileset quite easily in such editors as Aseprite, Pro Motion NG, Pyxel Edit.
//
// How to make such tileset in Aseprite:
// - Edit your tile map by replacing the tiles that should be animated with placeholder tiles (save it as a .png file)
// - Make a copy of this file, and replace the placeholders in it with empty tiles
// - Open this copied file in Aseprite, right-click on the in the layer window on Layer 1 > "Convert To" > "Tilemap"
// - Choose an 8x8 grid size
// - Enable "Advanced Option" > Enable "Allowed Flips XY"
// - In the Main Menu - "File" > "Export" > "Export Tileset"
// - Select "Sheet Type" - "By Rows", "Constraints" - "Fixed # of Columns"
// - Set the necessary number of columns (In this example, it is 7, because the total width of the placeholders is 7, and 
//   to the right of the placeholders you cannot place map tiles or empty tiles to avoid their displacement by optimization)
// - Enable "Ignore empty"
// - In the "Output" tab, set a file name and click "Export" button
// - Now open this tileset image in Aseprite, resize the canvas by adding the necessary empty space you need at the top,
//   and copying your placeholders tiles into it (1 copy of each type),
//   placing them so that there is no empty space between them and around the edges
//--------------------------------------------------------------------------------------------------------------------------
TILESET bg_tileset "bga_tileset.png" BEST ALL


//------------------------------ Background map ----------------------------------------------------------------------------
MAP map_def "bga.png" bg_tileset


//------------------------------ Animated Tilesets -------------------------------------------------------------------------
// We do not use compression here otherwise there will be a frame drops in the game during unpacking.
// We also do not use tile optimization here, as we need the tiles to be in their positions,
// but during animation, empty or duplicate tiles may appear, and optimization would remove them,
// shifting other tiles from their positions.
//--------------------------------------------------------------------------------------------------------------------------
TILESET animated_tileset_frame_0 "animated_tileset_frame_0.png" NONE NONE
TILESET animated_tileset_frame_1 "animated_tileset_frame_1.png" NONE NONE
