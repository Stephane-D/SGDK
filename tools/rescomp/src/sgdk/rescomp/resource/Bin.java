package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

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
    public final boolean far;
    public final boolean embedded;

    final int hc;

    public Bin(String id, byte[] data, int align, int sizeAlign, int fill, Compression compression, boolean far, boolean embedded)
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
        this.embedded = embedded;

        // compute hash code
        hc = Arrays.hashCode(data) ^ (align << 16) ^ wantedCompression.hashCode();
    }

    public Bin(String id, byte[] data, int align, int sizeAlign, int fill, Compression compression, boolean far)
    {
        // consider embedded by default
        this(id, data, align, sizeAlign, fill, compression, far, true);
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
            return (align == bin.align) && (wantedCompression == bin.wantedCompression) && Arrays.equals(data, bin.data);
        }

        return false;
    }

    @Override
    public List<Bin> getInternalBinResources()
    {
        return new ArrayList<>();
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

        // pack data first if needed (force selected compression when not embedded resource)
        packedData = Util.pack(data, wantedCompression, outB, !embedded);
        doneCompression = packedData.compression;

        final int baseSize = data.length;
        final int packedSize = packedData.data.length;

        // data was compressed ?
        if (wantedCompression != Compression.NONE)
        {
            System.out.print("'" + id + "' ");

            switch (doneCompression)
            {
                case NONE:
                    System.out.println("not packed (size = " + baseSize + ")");
                    break;

                case APLIB:
                    System.out.print("packed with APLIB, ");
                    break;

                case LZ4W:
                    System.out.print("packed with LZ4W, ");
                    break;

                default:
                    System.out.print("packed with UNKNOW, ");
                    break;
            }

            if (doneCompression != Compression.NONE)
                System.out.println("size = " + packedSize + " (" + Math.round((packedSize * 100f) / baseSize) + "% - origin size = " + baseSize + ")");
        }

        // output binary data (data alignment was done before)
        Util.outB(outB, packedData.data);

        // declare
        Util.declArray(outS, outH, "u8", id, packedData.data.length, align, global);
        // output data (compression information is stored in 'parent' resource when embedded)
        Util.outS(outS, packedData.data, 1);
        outS.append("\n");
    }
}