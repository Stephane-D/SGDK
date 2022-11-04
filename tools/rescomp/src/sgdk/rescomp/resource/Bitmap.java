package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.ArrayMath;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Bitmap extends Resource
{
    public final int w;
    public final int h;
    final int hc;

    public final Bin bin;
    public final Palette palette;

    public Bitmap(String id, String imgFile, Compression compression) throws Exception
    {
        super(id);

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);

        // set width and height
        w = imgInfo.w;

        // check width is correct
        if ((w & 1) == 1)
            throw new IllegalArgumentException("'" + imgFile + "' width is '" + w + ", even width (multiple of 2) required.");

        // get 8bpp pixels
        byte[] data = ImageUtil.getImageAs8bpp(imgFile, false, true);

        // we determine 'h' from data length and 'w' as we can crop image vertically to remove palette data
        h = data.length / w;

        // find max color index
        final int maxIndex = ArrayMath.max(data, false);
        // not allowed here
        if (maxIndex >= 16)
            throw new IllegalArgumentException(
                    "'" + imgFile + "' uses color index >= 16, BITMAP resource requires image with a maximum of 16 colors (use 4bpp image instead if unsure)");

        // convert to 4 bpp
        data = ImageUtil.convertTo4bpp(data, 8);

        // build BIN (image data) with wanted compression
        bin = (Bin) addInternalResource(new Bin(id + "_data", data, compression));
        // build PALETTE
        palette = (Palette) addInternalResource(new Palette(id + "_palette", imgFile));

        // compute hash code
        hc = bin.hashCode() ^ (w << 8) ^ (h << 16) ^ palette.hashCode();
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Bitmap)
        {
            final Bitmap bitmap = (Bitmap) obj;
            return (w == bitmap.w) && (h == bitmap.h) && palette.equals(bitmap.palette) && bin.equals(bitmap.bin);
        }

        return false;
    }

    @Override
    public int shallowSize()
    {
        return 2 + 2 + 2 + 4 + 4;
    }

    @Override
    public int totalSize()
    {
        return bin.totalSize() + palette.totalSize() + shallowSize();
    }

    @Override
    public List<Bin> getInternalBinResources()
    {
        return Arrays.asList(bin);
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Bitmap structure
        Util.decl(outS, outH, "Bitmap", id, 2, global);
        // set compression info (very important that binary data had already been exported at this point)
        outS.append("    dc.w    " + (bin.doneCompression.ordinal() - 1) + "\n");
        // set size in pixel
        outS.append("    dc.w    " + w + ", " + h + "\n");
        // set palette pointer
        outS.append("    dc.l    " + palette.id + "\n");
        // set image data pointer
        outS.append("    dc.l    " + bin.id + "\n");
        outS.append("\n");
    }

}
