package sgdk.rescomp.type;

import java.util.Arrays;

import sgdk.rescomp.type.Basics.TileEquality;

/**
 * Metatile (NxN tile block)
 * 
 * @author stephane
 */
public class MetaTile
{
    // 1=1x1, 2=2x2 or 4=4x4
    final int size;
    // contains tile index and attribute for size x size block grid
    final public short[] data;

    public MetaTile mtHflip;
    public MetaTile mtVflip;
    public MetaTile mtHVflip;

    public Boolean globalPrio;
    int hc;

    public MetaTile()
    {
        this(2);
    }

    public MetaTile(int size)
    {
        super();

        this.size = size;
        data = new short[size * size];

        mtHflip = null;
        mtVflip = null;
        mtHVflip = null;

        globalPrio = null;
        hc = 0;
    }

    private void computeGlobalPrio()
    {
        // base prio
        int bp = data[0] & Tile.TILE_PRIORITY_MASK;
        // global prio
        boolean gp = true;

        // test if we can use global priority
        for (int i = 1; i < data.length; i++)
        {
            // prio changed --> can't use global prio
            if (bp != (data[i] & Tile.TILE_PRIORITY_MASK))
            {
                gp = false;
                break;
            }
        }

        // global prio supported ?
        if (gp)
        {
            globalPrio = Boolean.valueOf(bp != 0);

            // remove priority bit from attributes as we store it globally
            if (bp != 0)
            {
                for (int i = 0; i < data.length; i++)
                    data[i] &= ~Tile.TILE_PRIORITY_MASK;
            }
        }
    }

    private void computeHashCode()
    {
        hc = size ^ Arrays.hashCode(data);
    }

    public void set(int index, short tileattr)
    {
        data[index] = tileattr;
    }

    public void updateInternals()
    {
        computeGlobalPrio();
        computeHashCode();
    }

    private MetaTile buildFlipped(boolean hflip, boolean vflip)
    {
        final MetaTile result = new MetaTile(size);
        final int attrx = Tile.TILE_ATTR(0, false, vflip, hflip);

        for (int j = 0; j < size; j++)
        {
            int sj = (vflip) ? (size - 1) - j : j;

            for (int i = 0; i < size; i++)
            {
                int si = (hflip) ? (size - 1) - i : i;
                int sind = (sj * size) + si;

                result.set((j * size) + i, (short) (data[sind] ^ attrx));
            }
        }

        result.updateInternals();

        return result;
    }

    private void buildFlipped()
    {
        mtHflip = buildFlipped(true, false);
        mtVflip = buildFlipped(false, true);
        mtHVflip = buildFlipped(true, true);
    }

    public boolean hasGlobalPrio()
    {
        return globalPrio != null;
    }

    public boolean getGlobalPrio()
    {
        return hasGlobalPrio() && globalPrio.booleanValue();
    }

    public TileEquality getEquality(MetaTile metaTile)
    {
        if (mtHflip == null)
            buildFlipped();

        // perfect equality
        if (metaTile.equals(this))
            return TileEquality.EQUAL;
        // hflip
        if (metaTile.equals(mtHflip))
            return TileEquality.HFLIP;
        // vflip
        if (metaTile.equals(mtVflip))
            return TileEquality.VFLIP;
        // hvflip
        if (metaTile.equals(mtHVflip))
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
        if (obj instanceof MetaTile)
        {
            final MetaTile mt = (MetaTile) obj;

            return (mt.hashCode() == hashCode()) && (mt.size == size) && Arrays.equals(mt.data, data);
        }

        return super.equals(obj);
    }

    @Override
    public String toString()
    {
        return size + "x" + size + " = " + Arrays.toString(data);
    }
}
