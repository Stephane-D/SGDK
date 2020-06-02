package sgdk.rescomp.resource;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Tile;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class Tilemap extends Resource
{
    public static final int TILE_MAX_NUM = 1 << 11;
    public static final int TILE_INDEX_MASK = TILE_MAX_NUM - 1;

    public static final int TILE_HFLIP_SFT = 11;
    public static final int TILE_VFLIP_SFT = 12;
    public static final int TILE_PALETTE_SFT = 13;
    public static final int TILE_PRIORITY_SFT = 15;

    public static final int TILE_HFLIP_FLAG = 1 << TILE_HFLIP_SFT;
    public static final int TILE_VFLIP_FLAG = 1 << TILE_VFLIP_SFT;
    public static final int TILE_PRIORITY_FLAG = 1 << TILE_PRIORITY_SFT;

    public static final int TILE_HFLIP_MASK = TILE_HFLIP_FLAG;
    public static final int TILE_VFLIP_MASK = TILE_VFLIP_FLAG;
    public static final int TILE_PALETTE_MASK = 3 << TILE_PALETTE_SFT;
    public static final int TILE_PRIORITY_MASK = TILE_PRIORITY_FLAG;

    public static final int TILE_ATTR_MASK = TILE_PRIORITY_MASK | TILE_PALETTE_MASK | TILE_VFLIP_MASK | TILE_HFLIP_MASK;

    public static int TILE_ATTR(int pal, int prio, int flipV, int flipH)
    {
        return (flipH << TILE_HFLIP_SFT) + (flipV << TILE_VFLIP_SFT) + (pal << TILE_PALETTE_SFT)
                + (prio << TILE_PRIORITY_SFT);
    }

    public static int TILE_ATTR_FULL(int pal, int prio, int flipV, int flipH, int index)
    {
        return (flipH << TILE_HFLIP_SFT) + (flipV << TILE_VFLIP_SFT) + (pal << TILE_PALETTE_SFT)
                + (prio << TILE_PRIORITY_SFT) + index;
    }

    public static int TILE_ATTR(int pal, boolean prio, boolean flipV, boolean flipH)
    {
        return TILE_ATTR(pal, prio ? 1 : 0, flipV ? 1 : 0, flipH ? 1 : 0);
    }

    public static int TILE_ATTR_FULL(int pal, boolean prio, boolean flipV, boolean flipH, int index)
    {
        return TILE_ATTR_FULL(pal, prio ? 1 : 0, flipV ? 1 : 0, flipH ? 1 : 0, index);
    }

    public static Tilemap getTilemap(String id, Tileset tileset, int mapBase, byte[] image8bpp, int imageWidth,
            int imageHeight, int startTileX, int startTileY, int widthTile, int heigthTile, TileOptimization opt,
            Compression compression)
    {
        final int w = widthTile;
        final int h = heigthTile;

        final int mapBaseAttr = mapBase & TILE_ATTR_MASK;
        final int mapBaseTileInd = mapBase & TILE_INDEX_MASK;
        // we have a base offset --> we can use system plain tiles
        final boolean useSystemTiles = mapBaseTileInd != 0;

        final short[] data = new short[w * h];

        int offset = 0;
        // important to always use the same loop order when building Tileset and Tilemap object
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < w; i++)
            {
                // get tile
                final Tile tile = Tile.getTile(image8bpp, imageWidth, imageHeight, (i + startTileX) * 8,
                        (j + startTileY) * 8);
                final int index;

                // if no optimization, just use current offset as index
                if (opt == TileOptimization.NONE)
                    index = offset + mapBaseTileInd;
                else
                {
                    // use system tiles for plain tiles if possible
                    if (useSystemTiles && tile.isPlain())
                        index = tile.getPlainValue();
                    else
                        // otherwise we try to get tile index in the tileset
                        index = tileset.getTileIndex(tile, opt) + mapBaseTileInd;
                }

                // should never happen
                if (index == -1)
                    throw new RuntimeException("Can't find tile in tileset, something wrong happened...");

                // get equality info
                final TileEquality equality = tile.getEquality(tileset.tiles.get(index));
                // set tilemap
                data[offset++] = (short) (mapBaseAttr
                        | TILE_ATTR_FULL(tile.pal, tile.prio, equality.vflip, equality.hflip, index));
            }
        }

        return new Tilemap(id, data, null, w, h, compression);
    }

    public final int w;
    public final int h;
    final int hc;

    // binary data for tilemap
    public final Bin bin;

    private final String fileName;

    public Tilemap(String id, short[] data, String imgFile, int w, int h, Compression compression)
    {
        super(id);

        fileName = imgFile;

        this.w = w;
        this.h = h;

        // build BIN (tilemap data) with wanted compression
        bin = (Bin) addInternalResource(new Bin(id + "_data", data, compression, true));

        // compute hash code
        hc = bin.hashCode() ^ (w << 8) ^ (h << 16);
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Tilemap)
        {
            final Tilemap tilemap = (Tilemap) obj;
            return (w == tilemap.w) && (h == tilemap.h) && bin.equals(tilemap.bin);
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String physicalFileName()
    {
        return fileName;
    }

    @Override
    public int shallowSize()
    {
        return 2 + 2 + 2 + 4;
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output TileMap structure
        Util.decl(outS, outH, "TileMap", id, 2, global);
        // set compression info (very important that binary data had already been exported at this point)
        outS.append("    dc.w    " + (bin.doneCompression.ordinal() - 1) + "\n");
        // set size in tile
        outS.append("    dc.w    " + w + ", " + h + "\n");
        // set data pointer
        outS.append("    dc.l    " + bin.id + "\n");
        outS.append("\n");
    }
}