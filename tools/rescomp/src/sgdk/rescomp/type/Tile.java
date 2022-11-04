package sgdk.rescomp.type;

import java.util.Arrays;

import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.tool.ArrayUtil;
import sgdk.tool.ImageUtil;
import sgdk.tool.TypeUtil;

public class Tile implements Comparable<Tile>
{
    public static final int TILE_MAX_NUM = 1 << 11;
    public static final int TILE_INDEX_MASK = TILE_MAX_NUM - 1;

    public static final int TILE_HFLIP_SFT = 11;
    public static final int TILE_VFLIP_SFT = 12;
    public static final int TILE_PALETTE_SFT = 13;
    public static final int TILE_PRIORITY_SFT = 15;

    public static final int TILE_HFLIP_FLAG = 1 << TILE_HFLIP_SFT;
    public static final int TILE_VFLIP_FLAG = 1 << TILE_VFLIP_SFT;
    public static final int TILE_PRIORITY_FLAG = 1 << TILE_PRIORITY_SFT;

    public static final int TILE_HFLIP_MASK = TILE_HFLIP_FLAG;
    public static final int TILE_VFLIP_MASK = TILE_VFLIP_FLAG;
    public static final int TILE_PALETTE_MASK = 3 << TILE_PALETTE_SFT;
    public static final int TILE_PRIORITY_MASK = TILE_PRIORITY_FLAG;

    public static final int TILE_ATTR_MASK = TILE_PRIORITY_MASK | TILE_PALETTE_MASK | TILE_VFLIP_MASK | TILE_HFLIP_MASK;

    public static int TILE_ATTR(int pal, int prio, int flipV, int flipH)
    {
        return (flipH << TILE_HFLIP_SFT) + (flipV << TILE_VFLIP_SFT) + (pal << TILE_PALETTE_SFT) + (prio << TILE_PRIORITY_SFT);
    }

    public static int TILE_ATTR_FULL(int pal, int prio, int flipV, int flipH, int index)
    {
        return (flipH << TILE_HFLIP_SFT) + (flipV << TILE_VFLIP_SFT) + (pal << TILE_PALETTE_SFT) + (prio << TILE_PRIORITY_SFT) + index;
    }

    public static int TILE_ATTR(int pal, boolean prio, boolean flipV, boolean flipH)
    {
        return TILE_ATTR(pal, prio ? 1 : 0, flipV ? 1 : 0, flipH ? 1 : 0);
    }

    public static int TILE_ATTR_FULL(int pal, boolean prio, boolean flipV, boolean flipH, int index)
    {
        return TILE_ATTR_FULL(pal, prio ? 1 : 0, flipV ? 1 : 0, flipH ? 1 : 0, index);
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static byte[] transformTile(byte[] tile8bpp, int size, boolean hflip, boolean vflip, boolean prio, byte[] destTile8bpp)
    {
        int dstOffset = 0;
        int baseOffset = vflip ? (size - 1) * size : 0;

        for (int j = 0; j < size; j++)
        {
            int offset = baseOffset + (hflip ? size - 1 : 0);

            for (int i = 0; i < size; i++)
            {
                destTile8bpp[dstOffset++] = (byte) (tile8bpp[offset] | (prio ? 0x80 : 00));
                if (hflip)
                    offset--;
                else
                    offset++;
            }

            if (vflip)
                baseOffset -= size;
            else
                baseOffset += size;
        }

        return destTile8bpp;
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static byte[] flipTile(byte[] tile8bpp, int size, boolean hflip, boolean vflip, boolean prio)
    {
        return transformTile(tile8bpp, size, hflip, vflip, prio, new byte[size * size]);
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static void setPrioTile(byte[] destImage8bpp, int imgW, int x, int y, int size)
    {
        int dstOff = (y * imgW) + x;
        for (int j = 0; j < size; j++)
        {
            for (int i = 0; i < size; i++)
                destImage8bpp[dstOff++] |= 0x80;

            dstOff += imgW - size;
        }
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static void copyTile(byte[] destImage8bpp, int imgW, byte[] tile8bpp, int x, int y, int size)
    {
        int dstOff = (y * imgW) + x;
        int srcOff = 0;
        for (int j = 0; j < size; j++)
        {
            for (int i = 0; i < size; i++)
                destImage8bpp[dstOff++] = tile8bpp[srcOff++];

            dstOff += imgW - size;
        }
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static byte[] getImageTile(byte[] image8bpp, int imgW, int imgH, int x, int y, int size, byte[] destTile8bpp)
    {
        int srcOff = (y * imgW) + x;
        int dstOff = 0;
        for (int j = y; j < (y + size); j++)
        {
            for (int i = x; i < (x + size); i++)
            {
                final int pixel;

                // inside image ?
                if ((i >= 0) && (i < imgW) && (j >= 0) && (j < imgH))
                    pixel = TypeUtil.unsign(image8bpp[srcOff]);
                else
                    pixel = 0;
                srcOff++;

                destTile8bpp[dstOff++] = (byte) pixel;
            }

            srcOff += imgW - size;
        }

        return destTile8bpp;
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static byte[] getImageTile(byte[] image8bpp, int imgW, int imgH, int x, int y, int size)
    {
        return getImageTile(image8bpp, imgW, imgH, x, y, size, new byte[size * size]);
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static byte[] getImageTile(byte[] image8bpp, int imgW, int imgH, int tileInd, int size, byte[] destTile8bpp)
    {
        final int wt = imgW / size;
        return getImageTile(image8bpp, imgW, imgH, (tileInd % wt) * size, (tileInd / wt) * size, size, destTile8bpp);
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static byte[] getImageTile(byte[] image8bpp, int imgW, int imgH, int tileInd, int size)
    {
        final int wt = imgW / size;
        return getImageTile(image8bpp, imgW, imgH, (tileInd % wt) * size, (tileInd / wt) * size, size, new byte[size * size]);
    }

    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static Tile getTile(byte[] image8bpp, int imgW, int imgH, int x, int y, int size)
    {
        final byte[] imageTile = getImageTile(image8bpp, imgW, imgH, x, y, size);
        final byte[] data = new byte[size * size];

        int plainCol = -1;
        boolean plain = true;

        int pal = -1;
        int prio = -1;
        int transPal = -1;
        int transPrio = -1;

        int off = 0;
        for (int j = 0; j < size; j++)
        {
            for (int i = 0; i < size; i++)
            {
                final int pixel = imageTile[off];
                final int color = pixel & 0xF;

                // first pixel --> affect color
                if (plainCol == -1)
                    plainCol = color;
                // not the same color --> not a plain tile
                else if (plainCol != color)
                    plain = false;

                final int curPal = (pixel >> 4) & 3;
                final int curPrio = (pixel >> 7) & 1;

                // transparent pixel ?
                if (color == 0)
                {
                    // set palette
                    if (transPal == -1)
                        transPal = curPal;
                    // test for difference with previous palette from transparent pixels
                    else if (transPal != curPal)
                        throw new IllegalArgumentException("Error: transparent pixel at [" + (x + i) + "," + (y + j) + "] reference a different palette ("
                                + curPal + " != " + transPal + ").");

                    // set prio
                    if (transPrio == -1)
                        transPrio = curPrio;
                    // test for difference with previous priority from transparent pixels
                    else if (transPrio != curPrio)
                        throw new IllegalArgumentException("Error: transparent pixel at [" + (x + i) + "," + (y + j) + "] reference a different priority ("
                                + curPrio + " != " + transPrio + ").");
                }
                // opaque pixel
                else
                {
                    // set palette
                    if (pal == -1)
                        pal = curPal;
                    // test for difference with previous palette from opaque pixels
                    else if (pal != curPal)
                        throw new IllegalArgumentException(
                                "Error: pixel at [" + (x + i) + "," + (y + j) + "] reference a different palette (" + curPal + " != " + pal + ").");

                    // set prio
                    if (prio == -1)
                        prio = curPrio;
                    // test for difference with previous priority from opaque pixels
                    else if (prio != curPrio)
                        throw new IllegalArgumentException(
                                "Error: pixel at [" + (x + i) + "," + (y + j) + "] reference a different priority (" + curPrio + " != " + prio + ").");
                }

                data[off] = (byte) color;
                off++;
            }
        }

        // use transparent pixel extended attributes if no opaque pixels
        if (pal == -1)
            pal = transPal;
        if (prio == -1)
            prio = transPrio;

        return new Tile(data, size, pal, prio != 0, plain ? plainCol : -1);
    }

    public final int[] data;
    public final int size;
    public final int pal;
    public final int plain;
    public final boolean prio;
    public final boolean empty;

    final int[] hFlip;
    final int[] vFlip;
    final int[] hvFlip;

    final int hc;

    public Tile(int[] data, int size, int pal, boolean prio, int plain)
    {
        super();

        // 8 pixels of 4bpp per 'int' entry
        if (data.length != ((size * size) / 8))
            throw new IllegalArgumentException("new Tile(int[]) error: array lenght does not match tile size !");

        this.data = data;
        this.size = size;

        this.pal = pal & 3;
        this.prio = prio;
        this.plain = plain;

        // set empty
        boolean emp = true;
        for (int i = 0; i < data.length; i++)
        {
            if (data[i] != 0)
            {
                emp = false;
                break;
            }
        }
        empty = emp;

        hFlip = getFlipped(true, false);
        vFlip = getFlipped(false, true);
        hvFlip = getFlipped(true, true);

        hc = getHash(data) + getHash(hFlip) + getHash(vFlip) + getHash(hvFlip);
    }

    public Tile(byte[] pixel8bpp, int size, int pal, boolean prio, int plain)
    {
        this(ArrayUtil.byteToInt(ImageUtil.convertTo4bpp(pixel8bpp, 8)), size, pal, prio, plain);
    }

    public int getHash(int[] array)
    {
        int result = 0;
        for (int i = 0; i < array.length; i++)
            result += array[i];

        return result;
    }

    public boolean isBlank()
    {
        return empty;
    }

    public boolean isPlain()
    {
        return getPlainValue() != -1;
    }

    /**
     * Return plain pixel value for plain tile (-1 otherwise)
     */
    public int getPlainValue()
    {
        return plain;
    }

    private int[] getFlipped(boolean hflip, boolean vflip)
    {
        final int[] result = new int[data.length];
        final int rowSize = (size / 8);

        int dstOffset = 0;
        int baseOffset = vflip ? (size - 1) * rowSize : 0;

        for (int j = 0; j < size; j++)
        {
            int offset = baseOffset + (hflip ? rowSize - 1 : 0);

            for (int i = 0; i < rowSize; i++)
            {
                if (hflip)
                {
                    result[dstOffset] = TypeUtil.swapNibble32(data[offset]);
                    offset--;
                }
                else
                {
                    result[dstOffset] = data[offset];
                    offset++;
                }

                dstOffset++;
            }

            if (vflip)
                baseOffset -= rowSize;
            else
                baseOffset += rowSize;
        }

        return result;
    }

    public TileEquality getEquality(Tile tile)
    {
        // perfect equality
        if (Arrays.equals(tile.data, data))
            return TileEquality.EQUAL;

        // hflip
        if (Arrays.equals(tile.data, hFlip))
            return TileEquality.HFLIP;
        // vflip
        if (Arrays.equals(tile.data, vFlip))
            return TileEquality.VFLIP;
        // hvflip
        if (Arrays.equals(tile.data, hvFlip))
            return TileEquality.HVFLIP;

        return TileEquality.NONE;
    }

    // only test for flip version here
    public TileEquality getFlipEquality(Tile tile)
    {
        // hflip
        if (Arrays.equals(tile.data, hFlip))
            return TileEquality.HFLIP;
        // vflip
        if (Arrays.equals(tile.data, vFlip))
            return TileEquality.VFLIP;
        // hvflip
        if (Arrays.equals(tile.data, hvFlip))
            return TileEquality.HVFLIP;

        return TileEquality.NONE;
    }

    @Override
    public int hashCode()
    {
        return hc;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (obj instanceof Tile)
        {
            final Tile t = (Tile) obj;

            return (hc == t.hc) && Arrays.equals(data, t.data);
        }

        return super.equals(obj);
    }

    @Override
    public int compareTo(Tile tile)
    {
        return Integer.compare(hc, tile.hc);
    }

    @Override
    public String toString()
    {
        return Arrays.toString(data);
    }
}