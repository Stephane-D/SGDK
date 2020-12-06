package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.PackedData;
import sgdk.tool.ArrayUtil;

public class Bin extends Resource
{
    public final byte[] data;
    public final int align;
    public final Compression wantedCompression;
    public PackedData packedData;
    public Compression doneCompression;
    public boolean far;

    final int hc;

    public Bin(String id, byte[] data, int align, int sizeAlign, int fill, Compression compression, boolean far)
    {
        super(id);

        if (sizeAlign > 0)
            this.data = Util.sizeAlign(data, sizeAlign, (byte) fill);
        else
            this.data = data;
        this.align = align;
        wantedCompression = compression;
        packedData = null;
        doneCompression = Compression.NONE;
        this.far = far;

        // compute hash code
        hc = Arrays.hashCode(data) ^ (align << 16) ^ wantedCompression.hashCode();
    }

    public Bin(String id, byte[] data, int align, int sizeAlign, int fill, Compression compression)
    {
        this(id, data, align, sizeAlign, fill, compression, true);
    }

    public Bin(String id, byte[] data, int align, int sizeAlign, int fill)
    {
        this(id, data, align, sizeAlign, fill, Compression.NONE);
    }

    public Bin(String id, byte[] data, int align, Compression compression)
    {
        this(id, data, align, 0, 0, compression);
    }

    public Bin(String id, byte[] data, Compression compression)
    {
        this(id, data, 2, 0, 0, compression);
    }

    public Bin(String id, short[] data, Compression compression, boolean far)
    {
        this(id, ArrayUtil.shortToByte(data), 2, 0, 0, compression, far);
    }

    public Bin(String id, short[] data, Compression compression)
    {
        this(id, ArrayUtil.shortToByte(data), 2, 0, 0, compression, true);
    }

    public Bin(String id, int[] data, Compression compression)
    {
        this(id, ArrayUtil.intToByte(data), 2, 0, 0, compression);
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Bin)
        {
            final Bin bin = (Bin) obj;
            return (align == bin.align) && (wantedCompression == bin.wantedCompression)
                    && Arrays.equals(data, bin.data);
        }

        return false;
    }

    @Override
    public int shallowSize()
    {
        return (packedData != null) ? packedData.data.length : data.length;
    }

    @Override
    public int totalSize()
    {
        return shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // do 'outB' align *before* doing compression (as LZ4W compression can use previous data block)
        Util.align(outB, align);

        // pack data first if needed
        packedData = Util.pack(data, wantedCompression, outB);
        doneCompression = packedData.compression;

        final int baseSize = data.length;
        final int packedSize = packedData.data.length;

        // data was compressed ?
        if (packedSize < baseSize)
        {
            System.out.print("'" + id + "' ");

            switch (packedData.compression)
            {
                case APLIB:
                    System.out.print("packed with APLIB, ");
                    break;

                case LZ4W:
                    System.out.print("packed with LZ4W, ");
                    break;

                default:
                    break;
            }

            System.out.println("size = " + packedSize + " (" + Math.round((packedSize * 100f) / baseSize)
                    + "% - origin size = " + baseSize + ")");
        }
        // couldn't compress
        else if (wantedCompression != Compression.NONE)
            System.out.print("'" + id + "' couldn't be compressed");

        // output binary data (data alignment was done before)
        Util.outB(outB, packedData.data);

        // declare
        Util.declArray(outS, outH, "u8", id, packedData.data.length, align, global);
        // output data (compression information is stored in 'parent' resource)
        Util.outS(outS, packedData.data, 1);
        outS.append("\n");
    }
}