package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Tile;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Image extends Resource
{
    final int hc;

    public final Tileset tileset;
    public final Tilemap tilemap;
    public final Palette palette;

    public Image(String id, String imgFile, Compression compression, TileOptimization tileOpt, int mapBase)
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
        byte[] data = ImageUtil.getIndexedPixels(imgFile);
        // convert to 8 bpp
        data = ImageUtil.convertTo8bpp(data, imgInfo.bpp);

        // b0-b3 = pixel data; b4-b5 = palette index; b7 = priority bit
        // check if image try to use bit 6 (probably mean that we have too much colors in our image)
        for (byte d : data)
        {
            // bit 6 used ?
            if ((d & 0x40) != 0)
                throw new IllegalArgumentException("'" + imgFile
                        + "' has color index in [64..127] range, IMAGE resource requires image with a maximum of 64 colors");
        }

        // build TILESET with wanted compression
        tileset = (Tileset) addInternalResource(new Tileset(id + "_tileset", data, w, h, 0, 0, wt, ht, tileOpt,
                (mapBase & Tile.TILE_INDEX_MASK) != 0, compression));
        // build TILEMAP with wanted compression
        tilemap = (Tilemap) addInternalResource(
                Tilemap.getTilemap(id + "_tilemap", tileset, mapBase, data, wt, ht, tileOpt, compression));
        // build PALETTE
        palette = (Palette) addInternalResource(new Palette(id + "_palette", imgFile, 64, true));

        // compute hash code
        hc = tileset.hashCode() ^ tilemap.hashCode() ^ palette.hashCode();
    }

    public int getWidth()
    {
        return tilemap.w * 8;
    }

    public int getHeight()
    {
        return tilemap.h * 8;
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Image)
        {
            final Image image = (Image) obj;
            return palette.equals(image.palette) && tilemap.equals(image.tilemap) && tileset.equals(image.tileset);
        }

        return false;
    }

    @Override
    public int shallowSize()
    {
        return 4 + 4 + 4;
    }

    @Override
    public int totalSize()
    {
        return palette.totalSize() + tileset.totalSize() + tilemap.totalSize() + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Image structure
        Util.decl(outS, outH, "Image", id, 2, global);
        // Palette pointer
        outS.append("    dc.l    " + palette.id + "\n");
        // Tileset pointer
        outS.append("    dc.l    " + tileset.id + "\n");
        // Tilemap pointer
        outS.append("    dc.l    " + tilemap.id + "\n");
        outS.append("\n");
    }
}
