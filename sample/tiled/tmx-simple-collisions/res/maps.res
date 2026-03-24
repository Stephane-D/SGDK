
# ---------------------------- Foreground & Background------------------------------------------
# -------------------------------- tileset loading ---------------------------------------------
#          name          file                          compression  opt

TILESET    fg_tileset    "levels\tileset_fg.png"       FAST		    ALL
TILESET    bg_tileset    "levels\tileset_bg.png"       FAST		    ALL
# note: we can also use .tsx file instead of the .png file

# ---------------------------------- map loading -----------------------------------------------
#          name          tmx_file                      layer_id     ts_compression  map_compression
  
MAP        fg_map_def    "levels\map.tmx"              layer_a	    FAST            FAST
MAP        bg_map_def    "levels\map.tmx"              layer_b	    FAST 	        FAST
# note: layer_id - layer name from Tiled

# -------------------------------- palette loading ---------------------------------------------
#          name          file
PALETTE    fg_palette    "levels\tileset_fg.png"
PALETTE    bg_palette    "levels\tileset_bg.png"
  
# --------------------------- collision map loading ---------------------------------------------
#          name               tmx_file                 layer_id     
TILEMAP    collision_map      "levels\map.tmx"         collisions

# note: see bin/rescomp.txt for a detailed description of commands, arguments,
# and available additional optional arguments