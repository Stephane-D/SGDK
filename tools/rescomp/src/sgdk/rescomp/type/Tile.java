package sgdk.rescomp.type;

import java.util.Arrays;

import sgdk.rescomp.type.Basics.TileEquality;
import sgdk.tool.ArrayUtil;
import sgdk.tool.ImageUtil;
import sgdk.tool.TypeUtil;

public class Tile implements Comparable<Tile>
{
    /**
     * @param x
     *        X position in pixel
     * @param y
     *        Y position in pixel
     */
    public static Tile getTile(byte[] image8bpp, int imgW, int imgH, int x, int y)
    {
        final byte[] data = new byte[64];

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

                final int curPal = (pixel >> 4) & 3;
                final int curPrio = (pixel >> 7) & 1;

                if (pal == -1)
                    pal = curPal;
                else if (pal != curPal)
                    throw new IllegalArgumentException(
                            "Error: pixel at [" + x + "," + y + "] reference a different palette.");

                if (prio == -1)
                    prio = curPrio;
                else if (prio != curPrio)
                    throw new IllegalArgumentException(
                            "Error: pixel at [" + x + "," + y + "] reference a different priority.");

                data[dstOff++] = (byte) (pixel & 0xF);
            }

            srcOff += imgW - 8;
        }

        return new Tile(data, pal, prio != 0);
    }

    public final int[] data;
    public final int pal;
    public final boolean prio;
    public final boolean empty;

    final int hc;

    public Tile(int[] data, int pal, boolean prio)
    {
        super();

        if (data.length != 8)
            throw new IllegalArgumentException("new Tile(int[]) error: array lenght is != 8");

        // 8 lines of 8 pixels (4bpp)
        this.data = data;

        this.pal = pal & 3;
        this.prio = prio;

        // compute hash code and empty
        int c = 0;
        boolean emp = true;
        for (int i = 0; i < data.length; i++)
        {
            final int d = data[i];
            c ^= d;
            if (d != 0)
                emp = false;
        }

        hc = c;
        empty = emp;
    }

    public Tile(int[] data)
    {
        this(data, 0, false);
    }

    public Tile(byte[] pixel8bpp, int pal, boolean prio)
    {
        this(ArrayUtil.byteToInt(ImageUtil.convert8bppTo4bpp(pixel8bpp)), pal, prio);
    }

    public Tile(byte[] pixel8bpp)
    {
        this(ArrayUtil.byteToInt(ImageUtil.convert8bppTo4bpp(pixel8bpp)));
    }

    public Tile getFlipped(boolean hflip, boolean vflip)
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

        return new Tile(result);
    }

    public TileEquality getEquality(Tile tile)
    {
        // perfect equality
        if (tile.equals(this))
            return TileEquality.EQUAL;

        // hflip
        if (Arrays.equals(data, tile.getFlipped(true, false).data))
            return TileEquality.HFLIP;
        // vflip
        if (Arrays.equals(data, tile.getFlipped(false, true).data))
            return TileEquality.VFLIP;
        // hvflip
        if (Arrays.equals(data, tile.getFlipped(true, true).data))
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
}