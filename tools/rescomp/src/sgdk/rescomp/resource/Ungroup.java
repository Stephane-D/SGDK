package sgdk.rescomp.resource;

import sgdk.rescomp.Resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class Ungroup extends Resource
{
    final int hc;

    public Ungroup(String id)
    {
        super(id);

        // compute hash code
        hc = 1;
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        return (obj instanceof Ungroup);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String physicalFileName()
    {
        return null;
    }

    @Override
    public int shallowSize()
    {
        return 0;
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // nothing here
    }
}