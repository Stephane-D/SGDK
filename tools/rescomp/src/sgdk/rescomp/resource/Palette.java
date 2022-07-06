package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.TSX;
import sgdk.tool.FileUtil;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Palette extends Resource
{
    final int hc;

    public Bin bin;

    public Palette(String id, String fileIn, int startIndex, int maxSize, boolean align16) throws Exception
    {
        super(id);

        String file = fileIn;
        short[] palette;

        // PAL file ?
        if (FileUtil.getFileExtension(file, false).equalsIgnoreCase("pal"))
        {
            // get palette raw data
            palette = ImageUtil.getRGBA4444PaletteFromPALFile(file, 0x0EEE);
        }
        else
        {
            // TSX file ? --> get image file name
            if (FileUtil.getFileExtension(file, false).equalsIgnoreCase("tsx"))
                file = TSX.getTSXTilesetPath(file);

            // retrieve basic infos about the image
            final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(file);

            // true color / RGB image
            if (imgInfo.bpp > 8)
            {
                palette = ImageUtil.getRGBA4444PaletteFromTiles(file, 0x0EEE);
                // cannot found palette in RGB image
                if (palette == null)
                    throw new IllegalArgumentException(
                            "RGB image '" + file + "' does not contains palette data (see 'Important note about image format' in the rescomp.txt file");
            }
            else
                // get palette from indexed image
                palette = ImageUtil.getRGBA4444PaletteFromIndColImage(file, 0x0EEE);
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

    public Palette(String id, String file, int maxSize, boolean align16) throws Exception
    {
        this(id, file, 0, maxSize, true);
    }

    public Palette(String id, String file) throws Exception
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
    public List<Bin> getInternalBinResources()
    {
        return Arrays.asList(bin);
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
