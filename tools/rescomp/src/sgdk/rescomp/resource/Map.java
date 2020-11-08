package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
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
    public final int wb;
    public final int hb;
    final int hc;

    public final List<Metatile> metatiles;
    public final List<MapBlock> mapBlocks;
    public final short mapBlockIndexes[];
    public final short mapBlockRowOffsets[];
    public final Palette palette;

    // binary data
    public final Bin metatilesBin;
    public final Bin mapBlocksBin;
    public final Bin mapBlockIndexesBin;
    public final Bin mapBlockRowOffsetsBin;

    public Map(String id, String imgFile, int mapBase, int metatileSize, Tileset tileset)
            throws IOException, IllegalArgumentException
    {
        super(id);

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);

        // check BPP is correct
        if (imgInfo.bpp > 8)
            throw new IllegalArgumentException("'" + imgFile + "' is in " + imgInfo.bpp
                    + " bpp format, only indexed images (8,4,2,1 bpp) are supported.");

        // set width and height
        final int w = imgInfo.w;
        final int h = imgInfo.h;

        // check size is correct
        if ((w & 7) != 0)
            throw new IllegalArgumentException("'" + imgFile + "' width is '" + w + ", should be a multiple of 8.");
        if ((h & 7) != 0)
            throw new IllegalArgumentException("'" + imgFile + "' height is '" + h + ", should be a multiple of 8.");

        // get size in tile
        final int wt = w / 8;
        final int ht = h / 8;

        // get image data
        byte[] image = ImageUtil.getIndexedPixels(imgFile);
        // convert to 8 bpp
        image = ImageUtil.convertTo8bpp(image, imgInfo.bpp);

        // b0-b3 = pixel data; b4-b5 = palette index; b7 = priority bit
        // check if image try to use bit 6 (probably mean that we have too much colors in our image)
        for (byte d : image)
        {
            // bit 6 used ?
            if ((d & 0x40) != 0)
                throw new IllegalArgumentException("'" + imgFile
                        + "' has color index in [64..127] range, IMAGE resource requires image with a maximum of 64 colors");
        }

        // base attributes and base tile index offset
        final int mapBaseAttr = mapBase & Tile.TILE_ATTR_MASK;
        final int mapBaseTileInd = mapBase & Tile.TILE_INDEX_MASK;
        // we have a base offset --> we can use system plain tiles
        final boolean useSystemTiles = mapBaseTileInd != 0;

        // // build TILESET with wanted compression
        // tileset = (Tileset) addInternalResource(new Tileset(id + "_tileset", image, w, h, 0, 0, wt, ht,
        // TileOptimization.ALL, useSystemTiles, Compression.AUTO));

        // get size in block
        wb = (wt + 15) / 16;
        hb = (ht + 15) / 16;

        // build METATILES
        metatiles = new ArrayList<>();
        // build MAPBLOCKS
        mapBlocks = new ArrayList<>();

        // block indexes
        mapBlockIndexes = new short[wb * hb];
        int mbii = 0;

        // important to always use the same loop order when building Tileset and Tilemap object
        for (int j = 0; j < hb; j++)
        {
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

                                // get tile
                                final Tile tile = Tile.getTile(image, wt * 8, ht * 8, ti * 8, tj * 8);
                                final TileEquality eq;
                                int index;

                                // use system tiles for plain tiles if possible
                                if (useSystemTiles && tile.isPlain())
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
                                        throw new RuntimeException(
                                                "Can't find tile in tileset, something wrong happened...");

                                    // get equality info
                                    eq = tile.getEquality(tileset.get(index));
                                    // can add base index now
                                    index += mapBaseTileInd;
                                }

                                // set metatile attributes
                                mt.set(mtsi++, (short) (mapBaseAttr
                                        | Tile.TILE_ATTR_FULL(tile.pal, tile.prio, eq.vflip, eq.hflip, index)));
                            }
                        }

                        // update prio and hash code
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

                        // get equality info
                        final TileEquality mtEq = mt.getEquality(metatiles.get(mtIndex));

                        // set block attributes
                        mb.set(mbi++,
                                (short) Tile.TILE_ATTR_FULL(0, mt.getGlobalPrio(), mtEq.vflip, mtEq.hflip, mtIndex));
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
                mapBlockIndexes[mbii++] = (short) mbIndex;
            }
        }

        // define block row offsets
        mapBlockRowOffsets = new short[hb];
        for (int j = 0; j < hb; j++)
            mapBlockRowOffsets[j] = (short) (j * wb);

        // need convert to array data
        short[] data;
        int offset;

        // convert metatiles to array
        data = new short[metatiles.size() * (metatileSize * metatileSize)];
        offset = 0;
        for (Metatile mt : metatiles)
        {
            for (short attr : mt.data)
                data[offset++] = attr;
        }

        // build BIN (metatiles data)
        metatilesBin = (Bin) addInternalResource(new Bin(id + "_metatiles", data, Compression.NONE));

        // convert mapBlocks to array
        data = new short[mapBlocks.size() * (8 * 8)];
        offset = 0;
        for (MapBlock mb : mapBlocks)
        {
            for (short attr : mb.data)
                data[offset++] = attr;
        }

        // build BIN (mapBlocks data)
        mapBlocksBin = (Bin) addInternalResource(new Bin(id + "_mapblocks", data, Compression.NONE));
        // build BIN (mapBlockIndexes data)
        mapBlockIndexesBin = (Bin) addInternalResource(
                new Bin(id + "_mapblockindexes", mapBlockIndexes, Compression.NONE));
        // build BIN (mapBlockRowOffsets data)
        mapBlockRowOffsetsBin = (Bin) addInternalResource(
                new Bin(id + "_mapblockrowoffsets", mapBlockRowOffsets, Compression.NONE));

        // build PALETTE
        palette = (Palette) addInternalResource(new Palette(id + "_palette", imgFile, 64, true));

        // compute hash code
        hc = tileset.hashCode() ^ palette.hashCode() ^ metatilesBin.hashCode() ^ mapBlocksBin.hashCode()
                ^ mapBlockIndexesBin.hashCode() ^ mapBlockRowOffsetsBin.hashCode();
    }

    public int getMetaTileIndex(Metatile metatile)
    {
        for (int ind = 0; ind < metatiles.size(); ind++)
        {
            final Metatile mt = metatiles.get(ind);

            // we found a matching metatile --> return its index
            if (mt.getEquality(metatile) != TileEquality.NONE)
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
            return palette.equals(map.palette) && metatiles.equals(map.metatiles) && mapBlocks.equals(map.mapBlocks)
                    && Arrays.equals(mapBlockIndexes, map.mapBlockIndexes)
                    && Arrays.equals(mapBlockRowOffsets, map.mapBlockRowOffsets);
        }

        return false;
    }

    @Override
    public int shallowSize()
    {
        return 4 + 2 + 2 + 2 + 4 + 2 + 4 + 4 + 4;
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Image structure
        Util.decl(outS, outH, "Map", id, 2, global);
        // Palette pointer
        outS.append("    dc.l    " + palette.id + "\n");
        // set size in block
        outS.append("    dc.w    " + wb + ", " + hb + "\n");
        // set num metatile
        outS.append("    dc.w    " + metatiles.size() + "\n");
        // set metatile data pointer
        outS.append("    dc.l    " + metatilesBin.id + "\n");
        // set num mapblock
        outS.append("    dc.w    " + mapBlocks.size() + "\n");
        // set mapblock data pointer
        outS.append("    dc.l    " + mapBlocksBin.id + "\n");
        // set mapBlockIndexes data pointer
        outS.append("    dc.l    " + mapBlockIndexesBin.id + "\n");
        // set mapBlockRowOffsets data pointer
        outS.append("    dc.l    " + mapBlockRowOffsetsBin.id + "\n");
        outS.append("\n");
    }
}
