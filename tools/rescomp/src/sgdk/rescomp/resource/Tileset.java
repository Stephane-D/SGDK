package sgdk.rescomp.resource;

import java.awt.Rectangle;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Tile;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Tileset extends Resource
{
    public static Tileset getTileset(String id, String imgFile, Compression compression, TileOptimization tileOpt, boolean addBlank, boolean temp)
            throws Exception
    {
        // get 8bpp pixels and also check image dimension is aligned to tile
        final byte[] image = ImageUtil.getImageAs8bpp(imgFile, true, true);

        // happen when we couldn't retrieve palette data from RGB image
        if (image == null)
            throw new IllegalArgumentException(
                    "RGB image '" + imgFile + "' does not contains palette data (see 'Important note about image format' in the rescomp.txt file");

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);
        final int w = imgInfo.w;
        // we determine 'h' from data length and 'w' as we can crop image vertically to remove palette data
        final int h = image.length / w;

        return new Tileset(id, image, w, h, 0, 0, w / 8, h / 8, tileOpt, compression, addBlank, temp);
    }

    // tiles
    final private List<Tile> tiles;
    final int hc;

    // binary data block (tiles)
    public final Bin bin;

    // internals
    final boolean isDuplicate;
    final private java.util.Map<Tile, Integer> tileIndexesMap;
    final private java.util.Map<Integer, List<Tile>> tileByHashcodeMap;

    // special constructor for TSX (can have several tilesets for a single map)
    public Tileset(List<Tileset> tilesets)
    {
        super("tilesets");

        tiles = new ArrayList<>();
        tileIndexesMap = new HashMap<>();
        tileByHashcodeMap = new HashMap<>();
        isDuplicate = false;

        // !! don't optimize tilesets (important to preserve tile indexes here) !!
        for (Tileset tileset : tilesets)
            for (Tile tile : tileset.tiles)
                add(tile);

        // build the binary bloc
        final int[] data = new int[tiles.size() * 8];

        int offset = 0;
        for (Tile t : tiles)
        {
            System.arraycopy(t.data, 0, data, offset, 8);
            offset += 8;
        }

        // build BIN (tiles data) - no stored as this is a temporary tileset
        bin = new Bin(id + "_data", data, Compression.NONE);

        // compute hash code
        hc = bin.hashCode();
    }

    // special constructor for empty tileset
    public Tileset()
    {
        super("empty_tileset");

        tiles = new ArrayList<>();
        tileIndexesMap = new HashMap<>();
        tileByHashcodeMap = new HashMap<>();
        isDuplicate = false;

        // dummy bin
        bin = new Bin("empty_bin", new byte[0], Compression.NONE);
        // hash code
        hc = bin.hashCode();
    }

    // special constructor for TSX (single blank tile tileset)
    public Tileset(String id)
    {
        super(id);

        tiles = new ArrayList<>();
        tileIndexesMap = new HashMap<>();
        tileByHashcodeMap = new HashMap<>();
        isDuplicate = false;

        // just add a blank tile
        add(new Tile(new int[8], 8, 0, false, 0));

        // build the binary bloc
        final int[] data = new int[tiles.size() * 8];

        int offset = 0;
        for (Tile t : tiles)
        {
            System.arraycopy(t.data, 0, data, offset, 8);
            offset += 8;
        }

        // build BIN (tiles data) resource (temporary tileset so don't add as internal resource)
        bin = new Bin(id + "_data", data, Compression.NONE);

        // compute hash code
        hc = bin.hashCode();
    }

    public Tileset(String id, byte[] image8bpp, int imageWidth, int imageHeight, int startTileX, int startTileY, int widthTile, int heightTile,
            TileOptimization opt, Compression compression, boolean addBlank, boolean temp)
    {
        super(id);

        boolean hasBlank = false;

        tiles = new ArrayList<>();
        tileIndexesMap = new HashMap<>();
        tileByHashcodeMap = new HashMap<>();

        // important to always use the same loop order when building Tileset and Tilemap/Map object
        for (int j = 0; j < heightTile; j++)
        {
            for (int i = 0; i < widthTile; i++)
            {
                // get tile
                final Tile tile = Tile.getTile(image8bpp, imageWidth, imageHeight, (i + startTileX) * 8, (j + startTileY) * 8, 8);
                // find if tile already exist
                final int index = getTileIndex(tile, opt);

                // blank tile
                hasBlank |= tile.isBlank();

                // not found --> add it
                if (index == -1)
                    add(tile);
            }
        }

        // add a blank tile if not already present
        if (!hasBlank && addBlank)
            add(new Tile(new int[8], 8, 0, false, 0));

        // build the binary bloc
        final int[] data = new int[tiles.size() * 8];

        int offset = 0;
        for (Tile t : tiles)
        {
            System.arraycopy(t.data, 0, data, offset, 8);
            offset += 8;
        }

        // build BIN (tiles data) with wanted compression
        final Bin binResource = new Bin(id + "_data", data, compression);
        // internal
        binResource.global = false;

        // temporary tileset --> don't store the bin data
        if (temp)
        {
            isDuplicate = false;
            bin = binResource;
        }
        else
        {
            // keep track of duplicate bin resource here
            isDuplicate = findResource(binResource) != null;
            // add as resource (avoid duplicate)
            bin = (Bin) addInternalResource(binResource);
        }

        // compute hash code
        hc = bin.hashCode();
    }

    public Tileset(String id, byte[] image8bpp, int imageWidth, int imageHeight, List<? extends Rectangle> sprites, Compression compression, boolean temp)
    {
        super(id);

        tiles = new ArrayList<>();
        tileIndexesMap = new HashMap<>();
        tileByHashcodeMap = new HashMap<>();

        for (Rectangle rect : sprites)
        {
            // get width and height
            final int widthTile = rect.width / 8;
            final int heightTile = rect.height / 8;

            // important to respect sprite tile ordering (vertical)
            for (int i = 0; i < widthTile; i++)
                for (int j = 0; j < heightTile; j++)
                    add(Tile.getTile(image8bpp, imageWidth, imageHeight, rect.x + (i * 8), rect.y + (j * 8), 8));
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
        final Bin binResource = new Bin(id + "_data", data, compression);
        // internal
        binResource.global = false;

        // temporary tileset --> don't store the bin data
        if (temp)
        {
            isDuplicate = false;
            bin = binResource;
        }
        else
        {
            // keep track of duplicate bin resource here
            isDuplicate = findResource(binResource) != null;
            // add as resource (avoid duplicate)
            bin = (Bin) addInternalResource(binResource);
        }

        // compute hash code
        hc = bin.hashCode();
    }

    public int getNumTile()
    {
        return tiles.size();
    }

    public boolean isEmpty()
    {
        return getNumTile() == 0;
    }

    public Tile get(int index)
    {
        return tiles.get(index);
    }

    public void add(Tile tile)
    {
        // need to be called first
        addInternal(tile);
        tiles.add(tile);
    }

    private void addInternal(Tile tile)
    {
        // better to keep first index if duplicated (should not be really useful)..
        // if (!tileIndexesMap.containsKey(tile))
        tileIndexesMap.put(tile, Integer.valueOf(tiles.size()));

        final Integer hashKey = Integer.valueOf(tile.hashCode());
        List<Tile> hashTiles = tileByHashcodeMap.get(hashKey);

        if (hashTiles == null)
        {
            hashTiles = new ArrayList<>();
            tileByHashcodeMap.put(hashKey, hashTiles);
        }

        hashTiles.add(tile);
    }

    public int getTileIndex(Tile tile, TileOptimization opt)
    {
        // no optimization allowed --> need to duplicate tile
        if (opt == TileOptimization.NONE)
            return -1;

        // fast perfect match test (preferred choice if possible)
        final Integer key = tileIndexesMap.get(tile);
        // found ? --> return index
        if (key != null)
            return key.intValue();

        // allow flip ?
        if (opt == TileOptimization.ALL)
        {
            // get all tiles with same hash code
            final List<Tile> hashTiles = tileByHashcodeMap.get(Integer.valueOf(tile.hashCode()));

            // have some ?
            if (hashTiles != null)
            {
                for (Tile t : hashTiles)
                {
                    // flipped version ?
                    if (t.getFlipEquality(tile) != TileEquality.NONE)
                        // return index of the original tile
                        return tileIndexesMap.get(t).intValue();
                }
            }
        }

        // // always do a first pass for direct matching (preferred choice if possible)
        // for (int ind = 0; ind < tiles.size(); ind++)
        // if (tiles.get(ind).equals(tile))
        // return ind;
        //
        // // allow flip ?
        // if (opt == TileOptimization.ALL)
        // {
        // for (int ind = 0; ind < tiles.size(); ind++)
        // // found a flip equality ?
        // if (tiles.get(ind).getFlipEquality(tile) != TileEquality.NONE)
        // return ind;
        // }

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
    public List<Bin> getInternalBinResources()
    {
        return Arrays.asList(bin);
    }

    @Override
    public int shallowSize()
    {
        return 2 + 2 + 4;
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

        // output TileSet structure
        Util.decl(outS, outH, "TileSet", id, 2, global);
        // set compression info (very important that binary data had already been exported at this point)
        outS.append("    dc.w    " + (bin.doneCompression.ordinal() - 1) + "\n");
        // set number of tile
        outS.append("    dc.w    " + getNumTile() + "\n");
        // set data pointer
        outS.append("    dc.l    " + bin.id + "\n");
        outS.append("\n");
    }
}
