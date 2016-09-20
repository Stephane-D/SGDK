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
 *  \brief
 *      No compression.
 */
#define COMPRESSION_NONE        0
/**
 *  \brief
 *      Use aplib (appack or sixpack) compression scheme.
 */
#define COMPRESSION_APLIB       1
/**
 *  \brief
 *      Use LZ4W compression scheme.
 */
#define COMPRESSION_LZ4W        2


/**
 *  \brief
 *      Callback for QSort comparaison
 *
 * This callback is used to compare 2 objects.<br>
 * Return value should be:<br>
 * negatif if o1 is below o2<br>
 * 0 if o1 is equal to o2<br>
 * positif if o1 is above o2
 */
typedef s16 _comparatorCallback(void* o1, void* o2);


/**
 *  \brief
 *      Set the randomizer seed (to allow reproductible value if we are lucky with HV counter :p)
 */
void setRandomSeed(u16 seed);
/**
 *  \brief
 *      Return a random u16 integer.
 */
u16 random();

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
 *      KDebug log helper methods
 */
void KLog(char* text);
void KLog_U1(char* t1, u32 v1);
void KLog_U2(char* t1, u32 v1, char* t2, u32 v2);
void KLog_U3(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3);
void KLog_U4(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4);
void KLog_U1_(char* t1, u32 v1, char* t2);
void KLog_U2_(char* t1, u32 v1, char* t2, u32 v2, char* t3);
void KLog_U3_(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4);
void KLog_U4_(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4, char* t5);
void KLog_U1x(u16 minSize, char* t1, u32 v1);
void KLog_U2x(u16 minSize, char* t1, u32 v1, char* t2, u32 v2);
void KLog_U3x(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3);
void KLog_U4x(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4);
void KLog_U1x_(u16 minSize, char* t1, u32 v1, char* t2);
void KLog_U2x_(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3);
void KLog_U3x_(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4);
void KLog_U4x_(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4, char* t5);
void KLog_S1(char* t1, s32 v1);
void KLog_S2(char* t1, s32 v1, char* t2, s32 v2);
void KLog_S3(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3);
void KLog_S4(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4, s32 v4);
void KLog_S1_(char* t1, s32 v1, char* t2);
void KLog_S2_(char* t1, s32 v1, char* t2, s32 v2, char* t3);
void KLog_S3_(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4);
void KLog_S4_(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4, s32 v4, char* t5);
void KLog_S1x(u16 minSize, char* t1, s32 v1);
void KLog_S2x(u16 minSize, char* t1, s32 v1, char* t2, s32 v2);
void KLog_S3x(u16 minSize, char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3);
void KLog_S4x(u16 minSize, char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4, s32 v4);
void KLog_f1(char* t1, fix16 v1);
void KLog_f2(char* t1, fix16 v1, char* t2, fix16 v2);
void KLog_f3(char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3);
void KLog_f4(char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3, char* t4, fix16 v4);
void KLog_f1x(s16 numDec, char* t1, fix16 v1);
void KLog_f2x(s16 numDec, char* t1, fix16 v1, char* t2, fix16 v2);
void KLog_f3x(s16 numDec, char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3);
void KLog_f4x(s16 numDec, char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3, char* t4, fix16 v4);
void KLog_F1(char* t1, fix32 v1);
void KLog_F2(char* t1, fix32 v1, char* t2, fix32 v2);
void KLog_F3(char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3);
void KLog_F4(char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3, char* t4, fix32 v4);
void KLog_F1x(s16 numDec, char* t1, fix32 v1);
void KLog_F2x(s16 numDec, char* t1, fix32 v1, char* t2, fix32 v2);
void KLog_F3x(s16 numDec, char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3);
void KLog_F4x(s16 numDec, char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3, char* t4, fix32 v4);


/**
 *  \brief
 *      Allocate a new Bitmap structure which can receive unpacked bitmap data of the specified Bitmap.<br>
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param bitmap
 *      Source Bitmap we want to allocate the unpacked Bitmap object.
 *  \return
 *      The new allocated Bitmap object which can receive the unpacked Bitmap, note that returned bitmap
 *      is allocated in a single bloc and can be released with Mem_Free(bitmap).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked bitmap.
 */
Bitmap *allocateBitmap(const Bitmap *bitmap);
/**
 *  \brief
 *      Allocate a new Bitmap structure which can receive the bitmap data for the specified Bitmap dimension.
 *
 *  \param width
 *      Width in pixel of the bitmap structure we want to allocate.
 *  \param heigth
 *      heigth in pixel of the bitmap structure we want to allocate.
 *  \return
 *      The new allocated Bitmap object which can receive an unpacked Bitmap for the specified dimension.<br>
 *      Note that returned bitmap is allocated in a single bloc and can be released with Mem_Free(bitmap).<br>
 *      <i>NULL</i> is returned if there is not enough memory to allocate the bitmap.
 */
Bitmap *allocateBitmapEx(u16 width, u16 heigth);
/**
 *  \brief
 *      Allocate TileSet structure which can receive unpacked tiles data of the specified TileSet.<br>
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param tileset
 *      Source TileSet we want to allocate the unpacked TileSet object.
 *  \return
 *      The new allocated TileSet object which can receive the unpacked TileSet, note that returned tile set
 *      is allocated in a single bloc and can be released with Mem_Free(tb).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked tiles.
 *      If the source TileSet is not packed then returned TileSet allocate only memory to do <i>NULL</i> is returned if there is not enough memory to store the unpacked tiles.
 */
TileSet *allocateTileSet(const TileSet *tileset);
/**
 *  \brief
 *      Allocate a new TileSet structure which can receive the data for the specified number of tile.
 *
 *  \param numTile
 *      Number of tile this tileset can contain
 *  \return
 *      The new allocated TileSet object which can receive the specified number of tile.<br>
 *      Note that returned tileset is allocated in a single bloc and can be released with Mem_Free(tileset).<br>
 *      <i>NULL</i> is returned if there is not enough memory to allocatee the tileset.
 */
TileSet *allocateTileSetEx(u16 numTile);
/**
 *  \brief
 *      Allocate Map structure which can receive unpacked map data of the specified Map.<br>
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param map
 *      Source Map we want to allocate the unpacked Map object.
 *  \return
 *      The new allocated Map object which can receive the unpacked Map, note that returned map
 *      is allocated in a single bloc and can be released with Mem_Free(map).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked map.
 */
Map *allocateMap(const Map *map);
/**
 *  \brief
 *      Allocate a new Map structure which can receive map data for the specified Map dimension.
 *
 *  \param width
 *      Width in tile of the Map structure we want to allocate.
 *  \param heigth
 *      heigth in tile of the Map structure we want to allocate.
 *  \return
 *      The new allocated Map object which can receive data for the specified Map dimension.<br>
 *      Note that returned map is allocated in a single bloc and can be released with Mem_Free(map).<br>
 *      <i>NULL</i> is returned if there is not enough memory to allocate the map.
 */
Map *allocateMapEx(u16 width, u16 heigth);
/**
 *  \brief
 *      Allocate Image structure which can receive unpacked image data of the specified Image.
 *      If source is not packed the function only allocate space for simple shallow copy of the source.
 *
 *  \param image
 *      Source Image we want to allocate the unpacked Image object.
 *  \return
 *      The new allocated Image object which can receive the unpacked Image, note that returned image
 *      is allocated in a single bloc and can be released with Mem_Free(image).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked image.
 */
Image *allocateImage(const Image *image);

/**
 *  \brief
 *      Unpack the specified source Bitmap and return result in a new allocated Bitmap.
 *
 *  \param src
 *      bitmap to unpack.
 *  \param dest
 *      Destination bitmap where to store unpacked data, be sure to allocate enough space in image buffer.<br>
 *      If set to NULL then a dynamic allocated Bitmap is returned.
 *  \return
 *      The unpacked Bitmap.<br>
 *      If <i>dest</i> was set to NULL then the returned bitmap is allocated in a single bloc and can be released with Mem_Free(bitmap).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked bitmap.
 */
Bitmap *unpackBitmap(const Bitmap *src, Bitmap *dest);
/**
 *  \brief
 *      Unpack the specified TileSet structure and return result in a new allocated TileSet.
 *
 *  \param src
 *      tiles to unpack.
 *  \param dest
 *      Destination TileSet structure where to store unpacked data, be sure to allocate enough space in tiles and tilemap buffer.<br>
 *      If set to NULL then a dynamic allocated TileSet is returned.
 *  \return
 *      The unpacked TileSet.<br>
 *      If <i>dest</i> was set to NULL then the returned tiles base is allocated in a single bloc and can be released with Mem_Free(tb).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked tiles.
 */
TileSet *unpackTileSet(const TileSet *src, TileSet *dest);
/**
 *  \brief
 *      Unpack the specified Map structure and return result in a new allocated Map.
 *
 *  \param src
 *      map to unpack.
 *  \param dest
 *      Destination map where to store unpacked data, be sure to allocate enough space in tiles and tilemap buffer.<br>
 *      If set to NULL then a dynamic allocated Map is returned.
 *  \return
 *      The unpacked Map.<br>
 *      If <i>dest</i> was set to NULL then the returned map is allocated in a single bloc and can be released with Mem_Free(map).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked map.
 */
Map *unpackMap(const Map *src, Map *dest);
/**
 *  \brief
 *      Unpack the specified Image structure and return result in a new allocated Image.
 *
 *  \param src
 *       image to unpack.
 *  \param dest
 *      Destination Image where to store unpacked data.<br>
 *      If set to NULL then a dynamic allocated Image is returned.
 *  \return
 *      The unpacked Image.<br>
 *      If <i>dest</i> was set to NULL then the returned image is allocated in a single bloc and can be released with Mem_Free(image).<br>
 *      <i>NULL</i> is returned if there is not enough memory to store the unpacked image.
 */
Image *unpackImage(const Image *src, Image *dest);

/**
 *  \brief
 *      Unpack the specified source data buffer in the specified destination buffer.<br>
 *      if source is not packed then nothing is done.
 *
 *  \param compression
 *      compression type, accepted values:<br>
 *      <b>COMPRESSION_APLIB</b><br>
 *      <b>COMPRESSION_LZ4W</b><br>
 *  \param src
 *      Source data buffer containing the packed data to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.
 *  \return
 *      Unpacked size.
 */
u32 unpack(u16 compression, u8 *src, u8 *dest);

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
 *      Unpack (LZ4W) the specified source data buffer in the specified destination buffer.
 *
 *  \param src
 *      Source data buffer containing the packed data (LZ4W packed) to unpack.
 *  \param dest
 *      Destination buffer where to store unpacked data, be sure to allocate enough space.<br>
 *      The size of unpacked data is contained in the first 4 bytes of 'src'.
 *  \return
 *      Unpacked size.
 */
u32 lz4w_unpack(const u8 *src, u8 *dest);

/**
 *  \brief
 *      Decompresses data in raw deflate/zlib format.<br/>
 *
 *   zlib is a general-purpose compressor, supporting quite good
 *   compression ratios, but unpacking is relatively slow on the
 *   Genesis (around 23kb/s).
 *
 *  \param dest
 *      Destination buffer
 *  \param outLen
 *      Size of the destination buffer in bytes
 *  \param src
 *      Source data buffer containing compressed data
 *  \param srcLen
 *      Size of the source buffer in bytes
 */
int zlib_unpack(void *dest, const unsigned outLen, const void *src, const unsigned srcLen);


/**
 *  \brief
 *      Quick sort algo on u8 data array.
 *
 *  \param data
 *      u8 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void qsort_u8(u8 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on s8 data array.
 *
 *  \param data
 *      s8 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void qsort_s8(s8 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on u16 data array.
 *
 *  \param data
 *      u16 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void qsort_u16(u16 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on s16 data array.
 *
 *  \param data
 *      s16 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void qsort_s16(s16 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on u32 data array.
 *
 *  \param data
 *      u32 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void qsort_u32(u32 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on s32 data array.
 *
 *  \param data
 *      s32 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void qsort_s32(s32 *data, u16 left, u16 right);

/**
 *  \brief
 *      Quick sort algo on array of pointer (object)
 *
 *  \param data
 *      array of pointer (pointer design object to sort).
 *  \param len
 *      number of element in the data array
 *  \param cb
 *      comparator callback used to compare 2 objects.
 */
void qsort(void** data, u16 len, _comparatorCallback* cb);



#endif // _TOOLS_H_
