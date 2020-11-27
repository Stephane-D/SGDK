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
        return (flipH << TILE_HFLIP_SFT) + (flipV << TILE_VFLIP_SFT) + (pal << TILE_PALETTE_SFT)
                + (prio << TILE_PRIORITY_SFT);
    }

    public static int TILE_ATTR_FULL(int pal, int prio, int flipV, int flipH, int index)
    {
        return (flipH << TILE_HFLIP_SFT) + (flipV << TILE_VFLIP_SFT) + (pal << TILE_PALETTE_SFT)
                + (prio << TILE_PRIORITY_SFT) + index;
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
    public static Tile getTile(byte[] image8bpp, int imgW, int imgH, int x, int y)
    {
        final byte[] data = new byte[64];

        int plainCol = -1;
        boolean plain = true;

        int pal = -1;
        int prio = -1;
        int srcOff = (y * imgW) + x;
        int dstOff = 0;
        for (int j = y; j < (y + 8); j++)
        {
            for (int i = x; i < (x + 8); i++)
            {
                final int pixel;

                // inside image ?
                if ((i >= 0) && (i < imgW) && (j >= 0) && (j < imgH))
                    pixel = TypeUtil.unsign(image8bpp[srcOff]);
                else
                    pixel = 0;
                srcOff++;

                final int color = pixel & 0xF;

                // first pixel --> affect color
                if (plainCol == -1)
                    plainCol = color;
                // not the same color --> not a plain tile
                else if (plainCol != color)
                    plain = false;

                // not a transparent pixel ?
                if (color != 0)
                {
                    final int curPal = (pixel >> 4) & 3;
                    final int curPrio = (pixel >> 7) & 1;

                    // set palette
                    if (pal == -1)
                        pal = curPal;
                    else if (pal != curPal)
                        throw new IllegalArgumentException("Error: pixel at [" + i + "," + j
                                + "] reference a different palette (" + curPal + " != " + pal + ").");

                    // set prio
                    if (prio == -1)
                        prio = curPrio;
                    else if (prio != curPrio)
                        throw new IllegalArgumentException("Error: pixel at [" + i + "," + j
                                + "] reference a different priority (" + curPrio + " != " + prio + ").");
                }

                data[dstOff++] = (byte) color;
            }

            srcOff += imgW - 8;
        }

        // default palette and priority
        if (pal == -1)
            pal = 0;
        if (prio == -1)
            prio = 0;

        return new Tile(data, pal, prio != 0, plain ? plainCol : -1);
    }

    public final int[] data;
    public final int pal;
    public final int plain;
    public final boolean prio;
    public final boolean empty;

    final int[] hFlip;
    final int[] vFlip;
    final int[] hvFlip;

    final int hc;

    public Tile(int[] data, int pal, boolean prio, int plain)
    {
        super();

        if (data.length != 8)
            throw new IllegalArgumentException("new Tile(int[]) error: array lenght is != 8");

        // 8 lines of 8 pixels (4bpp)
        this.data = data;

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

        hc = getHash(data) ^ getHash(hFlip) ^ getHash(vFlip) ^ getHash(hvFlip);
    }

    public Tile(byte[] pixel8bpp, int pal, boolean prio, int plain)
    {
        this(ArrayUtil.byteToInt(ImageUtil.convertTo4bpp(pixel8bpp, 8)), pal, prio, plain);
    }

    public int getHash(int[] array)
    {
        int result = 0;
        for (int i = 0; i < array.length; i++)
        {
            final int d = array[i];
            result ^= d;
        }

        return result;
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
        final int[] result = new int[8];

        for (int i = 0; i < 8; i++)
        {
            int line;

            if (vflip)
                line = 7 - i;
            else
                line = i;

            if (hflip)
                result[i] = TypeUtil.swapNibble32(data[line]);
            else
                result[i] = data[line];
        }

        return result;
    }

    public TileEquality getEquality(Tile tile)
    {
        // perfect equality
        if ( Arrays.equals(tile.data, data))
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