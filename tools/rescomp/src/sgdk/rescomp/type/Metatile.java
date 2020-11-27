package sgdk.rescomp.type;

import java.util.Arrays;

/**
 * Metatile (NxN tile block)
 * 
 * @author stephane
 */
public class Metatile
{
    // 1=1x1, 2=2x2 or 4=4x4
    final int size;
    // contains tile index and attribute for size x size block grid
    final public short[] data;

    int hc;

    public Metatile()
    {
        this(2);
    }

    public Metatile(int size)
    {
        super();

        this.size = size;
        data = new short[size * size];
        hc = 0;
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
        computeHashCode();
    }

    @Override
    public int hashCode()
    {
        return hc;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (obj instanceof Metatile)
        {
            final Metatile mt = (Metatile) obj;

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
