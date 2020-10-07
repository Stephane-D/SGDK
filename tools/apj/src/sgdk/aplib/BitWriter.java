package sgdk.aplib;

public class BitWriter extends DynamicByteArray
{
    int offset;
    int tag;
    int nbit;

    public BitWriter()
    {
        super();

        tag = 0;
        nbit = 0;
        offset = -1;
    }

    public BitWriter(int size)
    {
        super(size);

        tag = 0;
        nbit = 0;
        offset = -1;
    }

    private void saveAndAllocateTag()
    {
        // need to save full tag
        if (offset != -1)
            buf[offset] = (byte) tag;

        offset = count;
        // allocate byte for tag
        write(0);
        // reset tag
        tag = 0;
    }

    public void writeBit(int bit)
    {
        // check if tag is full
        if (nbit-- == 0)
        {
            // save and allocate tag
            saveAndAllocateTag();
            nbit = 7;
        }

        tag |= (bit & 1) << nbit;
    }

    public void writeUByte(int value)
    {
        write(value);
    }

    @Override
    public synchronized byte[] toByteArray()
    {
        // need to save full tag
        if (offset != -1)
            buf[offset] = (byte) tag;

        return super.toByteArray();
    }
}
