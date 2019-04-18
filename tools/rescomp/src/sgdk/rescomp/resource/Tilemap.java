package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Tile;

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
                    index = offset;
                // otherwise we try to get tile index in the tileset
                else
                    index = tileset.getTileIndex(tile, opt);

                // should never happen
                if (index == -1)
                    throw new RuntimeException("Can't find tile in tileset, something wrong happened...");

                // get equality info
                final TileEquality equality = tile.getEquality(tileset.tiles.get(index));
                // set tilemap
                data[offset++] = (short) (mapBase
                        | TILE_ATTR_FULL(tile.pal, tile.prio, equality.vflip, equality.hflip, index));
            }
        }

        return new Tilemap(id, data, w, h, compression);
    }

    public final int w;
    public final int h;
    final int hc;

    // binary data for tilemap
    public final Bin bin;

    public Tilemap(String id, short[] data, int w, int h, Compression compression)
    {
        super(id);

        this.w = w;
        this.h = h;

        // build BIN (tilemap data) with wanted compression
        bin = (Bin) addInternalResource(new Bin(id + "_data", data, compression));

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

    @Override
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Map structure
        Util.decl(outS, outH, "Map", id, 2, global);
        // set compression info
        outS.println("    dc.w    " + (bin.doneCompression.ordinal() - 1));
        // set size in tile
        outS.println("    dc.w    " + w + ", " + h);
        // set data pointer
        outS.println("    dc.l    " + bin.id);
        outS.println();
    }
}