package sgdk.aplib;

import java.io.ByteArrayOutputStream;

public class DynamicByteArray extends ByteArrayOutputStream
{
    public DynamicByteArray()
    {
        super();
    }

    public DynamicByteArray(int size)
    {
        super(size);
    }

    public byte read(int off)
    {
        if ((off < 0) || (off >= count))
            throw new IndexOutOfBoundsException();

        return buf[off];
    }

    public void write(int off, byte value)
    {
        if ((off < 0) || (off >= count))
            throw new IndexOutOfBoundsException();

        buf[off] = value;
    }
}
