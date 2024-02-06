package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.MapBlock;
import sgdk.rescomp.type.Metatile;
import sgdk.rescomp.type.Tile;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Map extends Resource
{
    public static Map getMap(String id, String imgFile, int mapBase, int metatileSize, List<Tileset> tilesets, Compression compression, boolean addTileset) throws Exception
    {
        // get 8bpp pixels and also check image dimension is aligned to tile
        final byte[] image = ImageUtil.getImageAs8bpp(imgFile, true, true);

        // happen when we couldn't retrieve palette data from RGB image
        if (image == null)
            throw new IllegalArgumentException(
                    "RGB image '" + imgFile + "' does not contains palette data (see 'Important note about image format' in the rescomp.txt file");

        // b0-b3 = pixel data; b4-b5 = palette index; b7 = priority bit
        // check if image try to use bit 6 (probably mean that we have too much colors in our image)
        for (byte d : image)
        {
            // bit 6 used ?
            if ((d & 0x40) != 0)
                throw new IllegalArgumentException(
                        "'" + imgFile + "' has color index in [64..127] range, IMAGE resource requires image with a maximum of 64 colors");
        }

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);
        final int w = imgInfo.w;
        // we determine 'h' from data length and 'w' as we can crop image vertically to remove palette data
        final int h = image.length / w;

        return new Map(id, image, w, h, mapBase, metatileSize, tilesets, compression, addTileset);
    }

    public final int wb;
    public final int hb;
    public final Compression compression;
    final int hc;

    public final List<Metatile> metatiles;
    public final List<MapBlock> mapBlocks;
    public final List<short[]> mapBlockIndexes;
    public final short[] mapBlockRowOffsets;
    public final List<Tileset> tilesets;

    // binary data
    public final Bin metatilesBin;
    public final Bin mapBlocksBin;
    public final Bin mapBlockIndexesBin;
    public final Bin mapBlockRowOffsetsBin;

    public Map(String id, byte[] image8bpp, int imageWidth, int imageHeight, int mapBase, int metatileSize, List<Tileset> tilesets, Compression compression,
            boolean addTileset) throws IllegalArgumentException
    {
        super(id);

        // get size in tile
        final int wt = imageWidth / 8;
        final int ht = imageHeight / 8;

        // base prio, pal attributes and base tile index offset
        final boolean mapBasePrio = (mapBase & Tile.TILE_PRIORITY_MASK) != 0;
        final int mapBasePal = (mapBase & Tile.TILE_PALETTE_MASK) >> Tile.TILE_PALETTE_SFT;
        final int mapBaseTileInd = mapBase & Tile.TILE_INDEX_MASK;
        // store base tile index usage
        final boolean hasBaseTileIndex = mapBaseTileInd != 0;

        // store tileset
        this.tilesets = tilesets;

        // add tileset resources (and replace by duplicate if found)
        if (addTileset)
        {
            for (int t = 0; t < tilesets.size(); t++)
                this.tilesets.set(t, (Tileset) addResource(tilesets.get(t)));
        }

        // store compression
        this.compression = compression;

        // get size in block
        wb = (wt + 15) / 16;
        hb = (ht + 15) / 16;

        // build global TILESET
        final Tileset tileset = new Tileset(tilesets);
        // build METATILES
        metatiles = new ArrayList<>();
        // build MAPBLOCKS
        mapBlocks = new ArrayList<>();
        // build block indexes
        mapBlockIndexes = new ArrayList<>();
        // build block row offsets
        mapBlockRowOffsets = new short[hb];
        // set to -1 to mark that it's not yet set (we shouldn't never meet an offset of 65535 realistically)
        Arrays.fill(mapBlockRowOffsets, (short) -1);

        // important to always use the same loop order when building Tileset and Tilemap object
        for (int j = 0; j < hb; j++)
        {
            int mbrii = 0;
            final short[] mbRowIndexes = new short[wb];

            for (int i = 0; i < wb; i++)
            {
                final MapBlock mb = new MapBlock();
                int mbi = 0;

                for (int bj = 0; bj < 8; bj++)
                {
                    for (int bi = 0; bi < 8; bi++)
                    {
                        final Metatile mt = new Metatile();
                        int mtsi = 0;

                        for (int mj = 0; mj < metatileSize; mj++)
                        {
                            for (int mi = 0; mi < metatileSize; mi++)
                            {
                                // tile position
                                final int ti = ((i * 16) + (bi * 2) + (mi * 1));
                                final int tj = ((j * 16) + (bj * 2) + (mj * 1));
                                final Tile tile;
                                final TileEquality eq;
                                int index;

                                // outside image ?
                                if ((ti >= wt) || (tj >= ht))
                                {
                                    // use dummy tile
                                    tile = new Tile(new byte[64], 8, 0, false, 0);
                                    eq = TileEquality.NONE;
                                    index = 0;
                                }
                                else
                                {
                                    tile = Tile.getTile(image8bpp, wt * 8, ht * 8, ti * 8, tj * 8, 8);

                                    // we can use system tiles when we have a base tile offset
                                    if (hasBaseTileIndex && tile.isPlain())
                                    {
                                        index = tile.getPlainValue();
                                        eq = TileEquality.NONE;
                                    }
                                    else
                                    {
                                        // otherwise we try to get tile index in the tileset
                                        index = tileset.getTileIndex(tile, TileOptimization.ALL);
                                        // not found ? (should never happen)
                                        if (index == -1)
                                            throw new RuntimeException("Can't find tile [" + ti + "," + tj + "] in tileset, something wrong happened...");
                                        // index > 2047 ? --> not allowed
                                        if (index > 2047)
                                            throw new RuntimeException("Can't have more than 2048 different tiles, try to reduce number of unique tile...");

                                        // get equality info
                                        eq = tile.getEquality(tileset.get(index));
                                        // can add base index now
                                        index += mapBaseTileInd;
                                    }
                                }

                                // set metatile attributes
                                mt.set(mtsi++, (short) Tile.TILE_ATTR_FULL(mapBasePal + tile.pal, mapBasePrio | tile.prio, eq.vflip, eq.hflip, index));
                            }
                        }

                        // update internals (hash code)
                        mt.updateInternals();

                        // get index of metatile
                        int mtIndex = getMetaTileIndex(mt);
                        // not yet present ?
                        if (mtIndex == -1)
                        {
                            // get index
                            mtIndex = metatiles.size();
                            // add to MetaTiles list
                            metatiles.add(mt);
                        }

                        // set block attributes (metatile index only here)
                        mb.set(mbi++, (short) mtIndex);
                    }
                }

                // update hash code
                mb.computeHashCode();

                // get index of block
                int mbIndex = getBlockIndex(mb);
                // not yet present ?
                if (mbIndex == -1)
                {
                    // get index
                    mbIndex = mapBlocks.size();
                    // add to MapBlock list
                    mapBlocks.add(mb);
                }

                // store MapBlock index (we can't have more than 65536 blocks)
                mbRowIndexes[mbrii++] = (short) mbIndex;
            }

            // check if we have a duplicated map block row
            for (int i = 0; i < mapBlockIndexes.size(); i++)
            {
                // duplicated ?
                if (Arrays.equals(mbRowIndexes, mapBlockIndexes.get(i)))
                {
                    // store offset for this map block row
                    mapBlockRowOffsets[j] = (short) (i * wb);
                    break;
                }
            }

            // not yet set ?
            if (mapBlockRowOffsets[j] == -1)
            {
                // set offset to current row
                mapBlockRowOffsets[j] = (short) (mapBlockIndexes.size() * wb);
                // and add map block row indexes to list
                mapBlockIndexes.add(mbRowIndexes);
            }
        }

        // convert metatiles to array
        short[] mtData = new short[metatiles.size() * (metatileSize * metatileSize)];
        int offset = 0;
        for (Metatile mt : metatiles)
        {
            // we can't use packed metatile representation when we have a base tile index
            for (short attr : mt.data)
                mtData[offset++] = attr;
        }

        // build BIN (metatiles data)
        metatilesBin = (Bin) addInternalResource(new Bin(id + "_metatiles", mtData, compression));

        // convert mapBlocks to array
        if (metatiles.size() > 256)
        {
            // require 16 bit index
            final short[] mbData = new short[mapBlocks.size() * (8 * 8)];
            offset = 0;
            for (MapBlock mb : mapBlocks)
            {
                for (short ind : mb.data)
                    mbData[offset++] = ind;
            }

            // build BIN (mapBlocks data)
            mapBlocksBin = (Bin) addInternalResource(new Bin(id + "_mapBlocks", mbData, compression));
        }
        else
        {
            // 8 bit index
            final byte[] mbData = new byte[mapBlocks.size() * (8 * 8)];
            offset = 0;
            for (MapBlock mb : mapBlocks)
            {
                for (short ind : mb.data)
                    mbData[offset++] = (byte) ind;
            }

            // build BIN (mapBlocks data)
            mapBlocksBin = (Bin) addInternalResource(new Bin(id + "_mapBlocks", mbData, compression));
        }

        // require 16 bit index ? --> directly use mapBlockIndexes map
        if (mapBlocks.size() > 256)
        {
            // 16 bit index
            final short[] mbiData = new short[mapBlockIndexes.size() * wb];
            offset = 0;
            for (short[] rowIndexes : mapBlockIndexes)
                for (short ind : rowIndexes)
                    mbiData[offset++] = ind;

            // build BIN (mapBlockIndexes data)
            mapBlockIndexesBin = (Bin) addInternalResource(new Bin(id + "_mapBlockIndexes", mbiData, compression));
        }
        else
        {
            // 8 bit index
            final byte[] mbiData = new byte[mapBlockIndexes.size() * wb];
            offset = 0;
            for (short[] rowIndexes : mapBlockIndexes)
                for (short ind : rowIndexes)
                    mbiData[offset++] = (byte) ind;

            // build BIN (mapBlockIndexes data)
            mapBlockIndexesBin = (Bin) addInternalResource(new Bin(id + "_mapBlockIndexes", mbiData, compression));
        }

        // build BIN (mapBlockRowOffsets data) - never compressed (not worthing it)
        mapBlockRowOffsetsBin = (Bin) addInternalResource(new Bin(id + "_mapBlockRowOffsets", mapBlockRowOffsets, Compression.NONE));

        // check if we can unpack the MAP
        if (compression != Compression.NONE)
        {
            // get unpacked size
            int ts = totalSize();

            // fix for condensed encoded metatiles
            if (!hasBaseTileIndex)
            {
                // remove metatiles binary blob size
                ts -= metatilesBin.totalSize();
                // add metatiles definition RAW size
                ts += metatiles.size() * 4 * 2;
            }

            // above 48 KB ? --> error
            if (ts > (48 * 1024))
                throw new RuntimeException("Error: MAP '" + id + "' unpacked size = " + ts
                        + " bytes, you won't have enough memory to unpack it.\nRemove compression from MAP resource definition");
            // above 34 KB ? --> warning
            if (ts > (34 * 1024))
                System.err.println("Warning: MAP '" + id + "' unpacked size = " + ts
                        + " bytes, you may not be able to unpack it.\nYou may remove compression from MAP resource definition");
        }

        // compute hash code
        hc = tileset.hashCode() ^ metatilesBin.hashCode() ^ mapBlocksBin.hashCode() ^ mapBlockIndexesBin.hashCode() ^ mapBlockRowOffsetsBin.hashCode();
    }

    public int getMetaTileIndex(Metatile metatile)
    {
        for (int ind = 0; ind < metatiles.size(); ind++)
        {
            final Metatile mt = metatiles.get(ind);

            // we found a matching metatile --> return its index
            if (mt.equals(metatile))
                return ind;
        }

        // not found
        return -1;
    }

    private int getBlockIndex(MapBlock mapBlock)
    {
        return mapBlocks.indexOf(mapBlock);
    }

    /**
     * Returns width in block
     */
    public int getWidth()
    {
        return wb;
    }

    /**
     * Returns height in block
     */
    public int getHeight()
    {
        return hb;
    }

    private int getCompression()
    {
        int result = 0;

        result += (mapBlockIndexesBin.packedData.compression.ordinal() - 1);
        result <<= 4;
        result += (mapBlocksBin.packedData.compression.ordinal() - 1);
        result <<= 4;
        result += (metatilesBin.packedData.compression.ordinal() - 1);

        return result;
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Map)
        {
            final Map map = (Map) obj;
            return metatiles.equals(map.metatiles) && mapBlocks.equals(map.mapBlocks) && mapBlockIndexesBin.equals(map.mapBlockIndexesBin)
                    && Arrays.equals(mapBlockRowOffsets, map.mapBlockRowOffsets);
        }

        return false;
    }

    @Override
    public List<Bin> getInternalBinResources()
    {
        return Arrays.asList(metatilesBin, mapBlocksBin, mapBlockIndexesBin, mapBlockRowOffsetsBin);
    }

    @Override
    public int shallowSize()
    {
        return 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4 + 4 + 4;
    }

    @Override
    public int totalSize()
    {
        int result = metatilesBin.totalSize() + mapBlocksBin.totalSize() + mapBlockIndexesBin.totalSize() + mapBlockRowOffsetsBin.totalSize() + shallowSize();

        return result;
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Image structure
        Util.decl(outS, outH, "MapDefinition", id, 2, global);
        // set size in block
        outS.append("    dc.w    " + wb + ", " + hb + "\n");
        // set real height of mapBlockIndexes (can have duplicated row which aren't stored)
        outS.append("    dc.w    " + mapBlockIndexes.size() + "\n");
        // set compression
        outS.append("    dc.w    " + getCompression() + "\n");
        // set num metatile
        outS.append("    dc.w    " + metatiles.size() + "\n");
        // set num mapblock
        outS.append("    dc.w    " + mapBlocks.size() + "\n");
        // set metatile data pointer
        outS.append("    dc.l    " + metatilesBin.id + "\n");
        // set mapblock data pointer
        outS.append("    dc.l    " + mapBlocksBin.id + "\n");
        // set mapBlockIndexes data pointer
        outS.append("    dc.l    " + mapBlockIndexesBin.id + "\n");
        // set mapBlockRowOffsets data pointer
        outS.append("    dc.l    " + mapBlockRowOffsetsBin.id + "\n");
        outS.append("\n");
    }

    @Override
    public String toString()
    {
        // display info about map encoding
        return "MAP '" + id + "' details: " + tilesets.size() + " tilesets, " + metatiles.size() + " metatiles, " + mapBlocks.size()
                + " blocks, block grid size = " + wb + " x " + hb + " - optimized = " + wb + " x " + mapBlockIndexes.size();
    }
}
