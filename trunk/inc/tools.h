/**
 *  \file tools.h
 *  \brief Misc tools methods
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides some misc tools methods as getFPS(), unpack()...
 */

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "bmp.h"
#include "vdp_tile.h"
#include "vdp_bg.h"


/**
 *  \def COMPRESSION_NONE
 *      No compression.
 */
#define COMPRESSION_NONE        0
/**
 *  \def COMPRESSION_APLIB
 *      Use aplib (appack or sixpack) compression scheme.
 */
#define COMPRESSION_APLIB       1
///**
// *  \def COMPRESSION_LZKN
// *      Use Konami (lzkn1_pack) compression scheme.
// */
//#define COMPRESSION_LZKN        2
/**
 *  \def COMPRESSION_RLE
 *      Use RLE compression scheme.
 */
#define COMPRESSION_RLE         3
/**
 *  \def COMPRESSION_MAP_RLE
 *      Use RLE compression scheme adapted for Map data.
 */
#define COMPRESSION_MAP_RLE     4


/**
 *  \brief
 *      Returns number of Frame Per Second.
 *
 * This function actually returns the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
u32 getFPS();
/**
 *  \brief
 *      Returns number of Frame Per Second (fix32 form).
 *
 * This function actually returns the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
fix32 getFPS_f();

/**
 *  \brief
 *      Allocate a new Bitmap structure which can receive unpacked bitmap data of the specified Bitmap.<br/>
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param bitmap
 *      Source Bitmap we want to allocate the unpacked Bitmap object.
 *  \return
 *      The new allocated Bitmap object which can receive the unpacked Bitmap, note that returned bitmap
 *      is allocated in a single bloc and can be released with Mem_Free(bitmap).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked bitmap.
 */
Bitmap *allocateBitmap(const Bitmap *bitmap);
/**
 *  \brief
 *      Allocate TileSet structure which can receive unpacked tiles data of the specified TileSet.<br/>
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param tileset
 *      Source TileSet we want to allocate the unpacked TileSet object.
 *  \return
 *      The new allocated TileSet object which can receive the unpacked TileSet, note that returned tile set
 *      is allocated in a single bloc and can be released with Mem_Free(tb).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked tiles.
 *      If the source TileSet is not packed then returned TileSet allocate only memory to do <i>NULL</i> is returned if there is not enough memory to store the unpacked tiles.
 */
TileSet *allocateTileSet(const TileSet *tileset);
/**
 *  \brief
 *      Allocate Map structure which can receive unpacked map data of the specified Map.<br/>
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param map
 *      Source Map we want to allocate the unpacked Map object.
 *  \return
 *      The new allocated Map object which can receive the unpacked Map, note that returned map
 *      is allocated in a single bloc and can be released with Mem_Free(map).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked map.
 */
Map *allocateMap(const Map *map);
/**
 *  \brief
 *      Allocate Image structure which can receive unpacked image data of the specified Image.
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param image
 *      Source Image we want to allocate the unpacked Image object.
 *  \return
 *      The new allocated Image object which can receive the unpacked Image, note that returned image
 *      is allocated in a single bloc and can be released with Mem_Free(image).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked image.
 */
Image *allocateImage(const Image *image);

/**
 *  \brief
 *      Unpack the specified source Bitmap.
 *
 *  \param src
 *       bitmap to unpack.
 *  \param dest
 *      Destination bitmap where to store unpacked data, be sure to allocate enough space in image buffer.<br/>
 *      If set to NULL then a dynamic allocated Bitmap is returned.
 *  \return
 *      The unpacked Bitmap.<br/>
 *      If <i>dest</i> was set to NULL then the returned bitmap is allocated in a single bloc and can be released with Mem_Free(bitmap).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked bitmap.
 */
Bitmap *unpackBitmap(const Bitmap *src, Bitmap *dest);
/**
 *  \brief
 *      Unpack the specified TileSet structure.
 *
 *  \param src
 *       tiles to unpack.
 *  \param dest
 *      Destination TileSet structure where to store unpacked data, be sure to allocate enough space in tiles and tilemap buffer.<br/>
 *      If set to NULL then a dynamic allocated TileSet is returned.
 *  \return
 *      The unpacked TileSet.<br/>
 *      If <i>dest</i> was set to NULL then the returned tiles base is allocated in a single bloc and can be released with Mem_Free(tb).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked tiles.
 */
TileSet *unpackTileSet(const TileSet *src, TileSet *dest);
/**
 *  \brief
 *      Unpack the specified Map structure.
 *
 *  \param src
 *       map to unpack.
 *  \param dest
 *      Destination map where to store unpacked data, be sure to allocate enough space in tiles and tilemap buffer.<br/>
 *      If set to NULL then a dynamic allocated Map is returned.
 *  \return
 *      The unpacked Map.<br/>
 *      If <i>dest</i> was set to NULL then the returned map is allocated in a single bloc and can be released with Mem_Free(map).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked map.
 */
Map *unpackMap(const Map *src, Map *dest);
/**
 *  \brief
 *      Unpack the specified Image structure.
 *
 *  \param src
 *       image to unpack.
 *  \param dest
 *      Destination Image where to store unpacked data.<br/>
 *      If set to NULL then a dynamic allocated Image is returned.
 *  \return
 *      The unpacked Image.<br/>
 *      If <i>dest</i> was set to NULL then the returned image is allocated in a single bloc and can be released with Mem_Free(image).<br/>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked image.
 */
Image *unpackImage(const Image *src, Image *dest);

/**
 *  \brief
 *      Unpack the specified source data buffer in the specified destination buffer.<br/>
 *      if source is not packed then nothing is done.
 *
 *  \param compression
 *      compression type, accepted values:<br/>
 *      <b>COMPRESSION_NONE</b><br/>
 *      <b>COMPRESSION_APLIB</b><br/>
 *      <b>COMPRESSION_RLE</b><br/>
 *      <b>COMPRESSION_MAP_RLE</b><br/>
 *  \param src
 *      Source data buffer containing the packed data (aplib packer) to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.
 */
void unpack(u16 compression, u8 *src, u8 *dest);

/**
 *  \brief
 *      Unpack (aplib packer) the specified source data buffer in the specified destination buffer.
 *
 *  \param src
 *      Source data buffer containing the packed data (aplib packer) to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.
 *  \return
 *      Unpacked size.
 */
u32 aplib_unpack(u8 *src, u8 *dest);
/**
 *  \brief
 *      Unpack (Konami packer) the specified source data buffer in the specified destination buffer.
 *
 *  \param src
 *      Source data buffer containing the packed data (Konami packer) to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.
 *  \return
 *      Unpacked size.
 */
u32 lzkn_unpack(u8 *src, u8 *dest);
/**
 *  \brief
 *      Unpack (RLE 4bit packer based on Charles MacDonald code) the specified source data buffer in the specified destination buffer.
 *
 *  \param src
 *      Source data buffer containing the packed data (RLE 4bit packer) to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.
 */
void rle4b_unpack(u8 *src, u8 *dest);
/**
 *  \brief
 *      Unpack (RLE MAP packer) the specified source data buffer in the specified destination buffer.
 *
 *  \param src
 *      Source data buffer containing the packed data (RLE MAP packer) to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.
 */
void rlemap_unpack(u8 *src, u8 *dest);
/**
 *  \brief
 *      Unpack (RLE 4bit packer based on Charles MacDonald code) the specified source data buffer directly in VRAM.
 *
 *  \param src
 *      Source data buffer containing the packed data (RLE 4bit packer) to unpack.
 *  \param dest
 *      VRAM destination address where to store unpacked dat.
 */
void rle4b_unpackVRam(u8 *src, u16 dest);
/**
 *  \brief
 *      Unpack (RLE MAP packer) the specified source data buffer directly in VRAM.
 *
 *  \param src
 *      Source data buffer containing the packed data (RLE MAP packer) to unpack.
 *  \param dest
 *      VRAM destination address where to store unpacked dat.
 *  \param basetile
 *      Base tile index and flags for tile attributes in the map (see TILE_ATTR_FULL() macro).
 */
void rlemap_unpackVRam(u8 *src, u16 dest, u16 basetile);

#endif // _TOOLS_H_
