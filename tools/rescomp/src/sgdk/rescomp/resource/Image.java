package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

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

    public Image(String id, String imgFile, Compression compression, TileOptimization tileOpt, int mapBase) throws Exception
    {
        super(id);

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
        // width and height
        final int w = imgInfo.w;
        // we determine 'h' from data length and 'w' as we can crop image vertically to remove palette data
        final int h = image.length / w;
        // get size in tile
        final int wt = w / 8;
        final int ht = h / 8;

        // build TILESET with wanted compression
        tileset = (Tileset) addInternalResource(new Tileset(id + "_tileset", image, w, h, 0, 0, wt, ht, tileOpt, compression, false, false));
        // build TILEMAP with wanted compression
        tilemap = (Tilemap) addInternalResource(Tilemap.getTilemap(id + "_tilemap", tileset, mapBase, image, wt, ht, tileOpt, compression));
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
    public List<Bin> getInternalBinResources()
    {
        return new ArrayList<>();
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
