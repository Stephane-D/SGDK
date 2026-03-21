
#        name         tmx_file                   layer_id      field_defs                                                                                                                                                sortby         decl_type          type_filter
OBJECTS  playersData  "tmx_map/objects_map.tmx"  "ActorLayer"  "name:string;x:f32;y:f32;type:u32;sprDefInd:u32;pal:u8;flipH:bool;flipV:bool;priority:bool;enabled:bool;phrase:string;speed:f32;hp:s16;target:object"     "sortby:x"     "TMX_ActorData"    "TiledPlayer"
      
OBJECTS  enemiesData  "tmx_map/objects_map.tmx"  "ActorLayer"  "name:string;x:f32;y:f32;type:u32;sprDefInd:u32;pal:u8;flipH:bool;flipV:bool;priority:bool;enabled:bool;phrase:string;speed:f32;hp:s16;target:object"     "sortby:x"     "TMX_ActorData"    "TiledEnemy"
      
OBJECTS  itemsData    "tmx_map/objects_map.tmx"  "ItemLayer"   "name:string;x:f32;y:f32;type:u32;sprDefInd:u32;pal:u8;flipH:bool;flipV:bool;priority:bool;enabled:bool;hp:s16"                                           "sortby:id"    "TMX_ItemData"     "TiledItem"

#  ------------------------------------------------- Parameters description --------------------------------------------------------------------------
#
#  name        - the name of the array that will contain data from the .tmx file (elements of the "decl_type" type) and is stored in ROM memory
#
#  tmx_file    - the path to the .tmx file containing the objects to be imported
#
#  layer_id    - the name of the layer (taken from Tiled) from which the objects will be imported
#
#  field_defs  - the name of the fields and their types, which will contain the data from the .tmx file.
#                The names must match the names of the fields in the Tiled objects,
#                and the types must match the types in Tiled according to the type mapping table:
#                    Tiled:  SGDK:
#                    bool    bool
#                    color   u32
#                    float   f32, f16, s32, u32, s16, u16
#                    file    NOT SUPPORTED
#                    int     f32, f16, s32, u32, s16, u16, s8, u8
#                    Object  object (internally void*)
#                    string  string (internally char*)
#                    Enum    u8, u16, u32
#
#  sortby      - the name of the field by which the objects in the "name" array will be sorted. By default, the "id" field is used,
#                but any other field can be specified. (optional, default is "id")
#
#  decl_type   - the data type (structure or structure typedef) into which the data from the imported objects will be placed.
#                All fields (names, types, and order) must match those in "field_defs". (optional, default is "void")
#
#  type_filter - just treat it as a filter. With it, you can filter objects from the Tiled layer by class name (the class field in Tiled),
#                i.e. import only objects from the layer that have the corresponding class name in Tiled.
#                Therefore, you must first assign the class name to the desired objects in Tiled.
#                (optional, default is no filter, all objects from the layer will be imported)
#
#  -----------------------------------------------------------------------------------------------------------------------------------------------------