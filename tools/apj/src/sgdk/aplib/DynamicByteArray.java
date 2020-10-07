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

    public int readBackUByte(int off)
    {
        return buf[count - off] & 0xFF;
    }

    public int readUByte(int off)
    {
        return buf[off] & 0xFF;
    }

    public void write(int off, int value)
    {
        buf[off] = (byte) value;
    }

    public void writeCheck(int value, byte[] verif)
    {
        if ((verif != null) && (value != (verif[count] & 0xFF)))
            System.out.println("Error at " + count);

        super.write(value);
    }
}
