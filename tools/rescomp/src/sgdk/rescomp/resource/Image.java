package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileOptimization;
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

        // b0-b3 = pixel data; b4-b5 = palette index; b7 = priority bit
        // check if image try to use bit 6 (probably mean that we have too much colors in our image)
        for (byte d : data)
        {
            // bit 6 used ?
            if ((d & 0x40) != 0)
                throw new IllegalArgumentException("'" + imgFile
                        + "' uses color index >= 64, IMAGE resource requires image with a maximum of 64 colors");
        }

        // final int maxIndex = ArrayMath.max(data, false);
        // if (maxIndex >= 64)
        // throw new IllegalArgumentException("'" + imgFile
        // + "' uses color index >= 64, IMAGE resource requires image with a maximum of 64 colors");

        // build TILESET with wanted compression
        tileset = (Tileset) addInternalResource(
                new Tileset(id + "_tileset", data, w, h, 0, 0, wt, ht, tileOpt, compression));
        // build TILEMAP with wanted compression
        tilemap = (Tilemap) addInternalResource(
                Tilemap.getTilemap(id + "_map", tileset, mapBase, data, w, h, 0, 0, wt, ht, tileOpt, compression));
        // build PALETTE
        palette = (Palette) addInternalResource(new Palette(id + "_palette", imgFile, 64, true));

        // compute hash code
        hc = tileset.hashCode() ^ tilemap.hashCode() ^ palette.hashCode();
    }

    public int getWidth()
    {
        return tilemap.w * 8;
    }

    public int getHeigth()
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
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Image structure
        Util.decl(outS, outH, "Image", id, 2, global);
        // Palette pointer
        outS.println("    dc.l    " + palette.id);
        // Tileset pointer
        outS.println("    dc.l    " + tileset.id);
        // Tilemap pointer
        outS.println("    dc.l    " + tilemap.id);
        outS.println();
    }
}
