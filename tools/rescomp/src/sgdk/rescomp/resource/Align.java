package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

import sgdk.rescomp.Resource;

public class Align extends Resource
{
    public final int align;

    final int hc;

    public Align(String id, int align)
    {
        super(id);

        this.align = align;

        // compute hash code
        hc = align;
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Align)
        {
            return (align == ((Align) obj).align);
        }

        return false;
    }

    @Override
    public int shallowSize()
    {
        return 0;
    }

    @Override
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH) throws IOException
    {
        // can't know align size so we just reset binary stream here (used for compression only)
        outB.reset();

        // simple alignment declaration
        outS.println("    .align  " + align);
        outS.println();
    }
}