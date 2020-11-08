package sgdk.rescomp.type;

import java.util.Arrays;

/**
 * Map block (128x128 pixels block internally encoded as 8x8 metaTile block)
 * 
 * @author steph
 */
public class MapBlock
{
    // contain metatile index + attributes for 8x8 block grid
    final public short[] data;

    int hc;

    public MapBlock()
    {
        super();

        data = new short[8 * 8];

        hc = 0;
    }

    public void set(int index, short attr)
    {
        data[index] = attr;
    }

    public void computeHashCode()
    {
        hc = Arrays.hashCode(data);
    }

    @Override
    public int hashCode()
    {
        return hc;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (obj instanceof MapBlock)
        {
            final MapBlock mb = (MapBlock) obj;

            return (mb.hashCode() == hashCode()) && Arrays.equals(mb.data, data);
        }

        return super.equals(obj);
    }

    @Override
    public String toString()
    {
        return "MapBlock[8x8] = " + Arrays.toString(data);
    }

}
