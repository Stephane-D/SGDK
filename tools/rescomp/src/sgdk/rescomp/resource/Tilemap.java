package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Tile;

public class Tilemap extends Resource
{
    public static Tilemap getTilemap(String id, Tileset tileset, int mapBase, byte[] image8bpp, int imageWidth,
            int imageHeight, int startTileX, int startTileY, int widthTile, int heigthTile, TileOptimization opt,
            Compression compression)
    {
        final int w = widthTile;
        final int h = heigthTile;

        final int mapBaseAttr = mapBase & (Tile.TILE_PRIORITY_MASK | Tile.TILE_VFLIP_MASK | Tile.TILE_HFLIP_MASK);
        final int mapBasePal = (mapBase & Tile.TILE_PALETTE_MASK) >> Tile.TILE_PALETTE_SFT;
        final int mapBaseTileInd = mapBase & Tile.TILE_INDEX_MASK;
        // we have a base offset --> we can use system plain tiles
        final boolean useSystemTiles = mapBaseTileInd != 0;

        final short[] data = new short[w * h];

        int offset = 0;
        // important to always use the same loop order when building Tileset and Tilemap object
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < w; i++)
            {
                // tile position
                final int ti = i + startTileX;
                final int tj = j + startTileY;

                // get tile
                final Tile tile = Tile.getTile(image8bpp, imageWidth, imageHeight, ti * 8, tj * 8);
                int index;
                TileEquality equality = TileEquality.NONE;

                // if no optimization, just use current offset as index
                if (opt == TileOptimization.NONE)
                    index = offset + mapBaseTileInd;
                else
                {
                    // use system tiles for plain tiles if possible
                    if (useSystemTiles && tile.isPlain())
                        index = tile.getPlainValue();
                    else
                    {
                        // otherwise we try to get tile index in the tileset
                        index = tileset.getTileIndex(tile, opt);
                        // not found ? (should never happen)
                        if (index == -1)
                            throw new RuntimeException(
                                    "Can't find tile [" + ti + "," + tj + "] in tileset, something wrong happened...");

                        // get equality info
                        equality = tile.getEquality(tileset.tiles.get(index));
                        // can add base index now
                        index += mapBaseTileInd;
                    }
                }

                // set tilemap
                data[offset++] = (short) (mapBaseAttr
                        | Tile.TILE_ATTR_FULL(mapBasePal + tile.pal, tile.prio, equality.vflip, equality.hflip, index));
            }
        }

        return new Tilemap(id, data, w, h, compression);
    }

    public static Tilemap getTilemap(String id, Tileset tileset, int mapBase, byte[] image8bpp, int widthTile,
            int heigthTile, TileOptimization opt, Compression compression)
    {
        return getTilemap(id, tileset, mapBase, image8bpp, widthTile * 8, heigthTile * 8, 0, 0, widthTile, heigthTile,
                opt, compression);
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

    @Override
    public int shallowSize()
    {
        return 2 + 2 + 2 + 4;
    }

    @Override
    public int totalSize()
    {
        return bin.totalSize() + shallowSize();
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