package sgdk.rescomp.resource;

import java.awt.Rectangle;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Tile;
import sgdk.tool.ArrayMath;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Tileset extends Resource
{
    public static Tileset getTileset(String id, String imgFile, Compression compression, TileOptimization tileOpt,
            int maxPaletteSize) throws IOException, IllegalArgumentException
    {
        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);

        // check BPP is correct
        if ((imgInfo.bpp != 8) && (imgInfo.bpp != 4))
            throw new IllegalArgumentException(
                    "'" + imgFile + "' is in " + imgInfo.bpp + " bpp format, only 8bpp or 4bpp image supported.");

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
        byte[] data = ImageUtil.getIndexedPixels(imgFile);

        // 4 bpp image ? --> convert to 8bpp
        if (imgInfo.bpp == 4)
            data = ImageUtil.convert4bppTo8bpp(data);

        // find max color index
        final int maxIndex = ArrayMath.max(data, false);
        // check if we are above the maximum palette size
        if (maxIndex >= maxPaletteSize)
            throw new IllegalArgumentException("'" + imgFile + "' uses color index >= " + maxPaletteSize
                    + ", TILESET resource requires image with a maximum of " + maxPaletteSize + " colors.");

        // build TILESET
        return new Tileset(id, data, w, h, 0, 0, wt, ht, tileOpt, compression);
    }

    // tiles
    final List<Tile> tiles;
    final int hc;

    // binary data block (tiles)
    public final Bin bin;

    public Tileset(String id, byte[] image8bpp, int imageWidth, int imageHeight, int startTileX, int startTileY,
            int widthTile, int heightTile, TileOptimization opt, Compression compression)
    {
        super(id);

        tiles = new ArrayList<>();

        // important to always use the same loop order when building Tileset and Tilemap object
        for (int j = 0; j < heightTile; j++)
        {
            for (int i = 0; i < widthTile; i++)
            {
                // get tile
                final Tile tile = Tile.getTile(image8bpp, imageWidth, imageHeight, (i + startTileX) * 8,
                        (j + startTileY) * 8);
                // find if tile already exist
                final int index = getTileIndex(tile, opt);

                // not found --> add it
                if (index == -1)
                    tiles.add(tile);
            }
        }

        // build the binary bloc
        final int[] data = new int[tiles.size() * 8];

        int offset = 0;
        for (Tile t : tiles)
        {
            System.arraycopy(t.data, 0, data, offset, 8);
            offset += 8;
        }

        // build BIN (tiles data) with wanted compression
        bin = (Bin) addInternalResource(new Bin(id + "_data", data, compression));

        // compute hash code
        hc = bin.hashCode();
    }

    public Tileset(String id, byte[] image8bpp, int imageWidth, int imageHeight, List<? extends Rectangle> sprites,
            Compression compression)
    {
        super(id);

        tiles = new ArrayList<>();

        for (Rectangle rect : sprites)
        {
            // get width and height
            final int widthTile = rect.width / 8;
            final int heightTile = rect.height / 8;

            // important to respect sprite tile ordering (vertical)
            for (int i = 0; i < widthTile; i++)
                for (int j = 0; j < heightTile; j++)
                    tiles.add(Tile.getTile(image8bpp, imageWidth, imageHeight, rect.x + (i * 8), rect.y + (j * 8)));
        }

        // build the binary bloc
        final int[] data = new int[tiles.size() * 8];

        int offset = 0;
        for (Tile t : tiles)
        {
            System.arraycopy(t.data, 0, data, offset, 8);
            offset += 8;
        }

        // build BIN (tiles data) with wanted compression
        bin = (Bin) addInternalResource(new Bin(id + "_data", data, compression));

        // compute hash code
        hc = bin.hashCode();
    }

    public int getNumTile()
    {
        return tiles.size();
    }

    public int getTileIndex(Tile tile, TileOptimization opt)
    {
        // no optimization allowed --> need to duplicate tile
        if (opt == TileOptimization.NONE)
            return -1;

        int ind = 0;
        for (Tile t : tiles)
        {
            if (opt == TileOptimization.DUPLICATE_ONLY)
            {
                // look only for duplicated tiles
                if (t.equals(tile))
                    return ind;
            }
            else
            {
                // look for flipped tiles as well
                final TileEquality result = t.getEquality(tile);
                // we found a matching tile --> return its index
                if (result != TileEquality.NONE)
                    return ind;
            }

            ind++;
        }

        // not found
        return -1;
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Tileset)
        {
            final Tileset tileset = (Tileset) obj;
            return bin.equals(tileset.bin);
        }

        return false;
    }

    @Override
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output TileSet structure
        Util.decl(outS, outH, "TileSet", id, 2, global);
        // set compression info
        outS.println("    dc.w    " + (bin.doneCompression.ordinal() - 1));
        // set number of tile
        outS.println("    dc.w    " + getNumTile());
        // set data pointer
        outS.println("    dc.l    " + bin.id);
        outS.println();
    }
}
