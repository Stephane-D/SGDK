package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

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

    public Bitmap(String id, String imgFile, Compression compression) throws IOException, IllegalArgumentException
    {
        super(id);

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);

        // check BPP is correct
        if (imgInfo.bpp > 8)
            throw new IllegalArgumentException("'" + imgFile + "' is in " + imgInfo.bpp
                    + " bpp format, only indexed images (8,4,2,1 bpp) are supported.");

        // set width and height
        w = imgInfo.w;
        h = imgInfo.h;

        // check width is correct
        if ((w & 1) == 1)
            throw new IllegalArgumentException(
                    "'" + imgFile + "' width is '" + w + ", even width (multiple of 2) required.");

        // get image data
        byte[] data = ImageUtil.getIndexedPixels(imgFile);
        // convert to 4 bpp
        data = ImageUtil.convertTo4bpp(data, imgInfo.bpp);

        // find max color index
        final int maxIndex = ArrayMath.max(data, false);
        // not allowed here
        if (maxIndex >= 16)
            throw new IllegalArgumentException("'" + imgFile
                    + "' uses color index >= 16, BITMAP resource requires image with a maximum of 16 colors (use 4bpp image instead if unsure)");

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
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // output Bitmap structure
        Util.decl(outS, outH, "Bitmap", id, 2, global);
        // set compression info
        outS.println("    dc.w    " + (bin.doneCompression.ordinal() - 1));
        // set size in pixel
        outS.println("    dc.w    " + w + ", " + h);
        // set palette pointer
        outS.println("    dc.l    " + palette.id);
        // set image data pointer
        outS.println("    dc.l    " + bin.id);
        outS.println();
    }

}
