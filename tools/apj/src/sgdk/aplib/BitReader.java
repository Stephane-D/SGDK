package sgdk.aplib;

public class BitReader
{
    final byte[] data;
    int offset;
    int tag;
    int nbit;

    public BitReader(byte[] array)
    {
        super();

        data = array;
        offset = 0;
        tag = 0;
        nbit = 0;
    }

    public int readBit()
    {
        int result;

        // check if tag is empty
        if (nbit-- == 0)
        {
            // load next tag */
            tag = readUByte();
            nbit = 7;
        }

        // get highest bit from tag
        result = (tag >> 7) & 1;
        tag <<= 1;

        return result;
    }

    public int readUByte()
    {
        return data[offset++] & 0xFF;
    }
}
