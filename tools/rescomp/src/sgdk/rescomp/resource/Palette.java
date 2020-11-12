package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.ArrayUtil;
import sgdk.tool.FileUtil;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Palette extends Resource
{
    final int hc;

    public Bin bin;

    public Palette(String id, String file, int startIndex, int maxSize, boolean align16)
            throws IllegalArgumentException, IOException
    {
        super(id);

        short[] palette;

        // PAL file ?
        if (FileUtil.getFileExtension(file, false).equalsIgnoreCase("pal"))
        {
            // get palette raw data
            palette = ArrayUtil.byteToShort(Util.in(file));
        }
        else
        {
            // retrieve basic infos about the image
            final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(file);

            // check BPP is correct
            if (imgInfo.bpp > 8)
                throw new IllegalArgumentException("'" + file + "' is in " + imgInfo.bpp
                        + " bpp format, only indexed images (8,4,2,1 bpp) are supported.");

            // get palette
            palette = ImageUtil.getRGBA4444Palette(file, 0x0EEE);

            // special case where we use the image pixel to define the palette
            if (imgInfo.h == 1)
            {
                // get image data
                byte[] pixels = ImageUtil.getIndexedPixels(file);
                // convert to 8 bpp
                pixels = ImageUtil.convertTo8bpp(pixels, imgInfo.bpp);

                final short[] tmpPalette = new short[palette.length];
                // set temp palette from pixel value
                for (int i = 0; i < Math.min(imgInfo.w, palette.length); i++)
                    tmpPalette[i] = palette[pixels[i]];

                // then store in palette
                palette = tmpPalette;
            }
        }

        final int adjMaxSize;

        // align max size on 16 entries
        if (align16)
            adjMaxSize = (maxSize + 15) & 0xF0;
        else
            adjMaxSize = maxSize;

        // we keep maxSize colors max
        if (palette.length > adjMaxSize)
            palette = Arrays.copyOf(palette, adjMaxSize);

        // build BIN (we never compress palette)
        bin = (Bin) addInternalResource(new Bin(id + "_data", palette, Compression.NONE, false));

        // compute hash code
        hc = bin.hashCode();
    }

    public Palette(String id, String file, int maxSize, boolean align16) throws IllegalArgumentException, IOException
    {
        this(id, file, 0, maxSize, true);
    }

    public Palette(String id, String file) throws IllegalArgumentException, IOException
    {
        this(id, file, 16, true);
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Palette)
        {
            final Palette palette = (Palette) obj;
            return bin.equals(palette.bin);
        }

        return false;
    }

    @Override
    public int shallowSize()
    {
        return 2 + 4;
    }

    @Override
    public int totalSize()
    {
        return bin.totalSize() + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // declare
        Util.decl(outS, outH, "Palette", id, 2, global);
        // first palette size
        outS.append("    dc.w    " + (bin.data.length / 2) + "\n");
        // set palette data pointer
        outS.append("    dc.l    " + bin.id + "\n");
        outS.append("\n");
    }
}
