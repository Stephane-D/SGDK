package sgdk.aplib;

class Decap
{
    private byte[] input;

    private int bitbuffer;
    private int bitcount;
    private int inptr;
    private int lwm;
    private int roffs;

    private int getbit() throws Exception
    {
        if (bitcount-- == 0)
        {
            bitbuffer = getbyte();
            bitcount = 7;
        }

        int bit = (bitbuffer >> 7) & 1;
        bitbuffer <<= 1;

        return bit;
    }

    private int getbyte() throws Exception
    {
        if (inptr < 0 && inptr >= input.length)
            throw new Exception("decap getbyte b0rk");

        return input[inptr++] & 0xFF;
    }

    private int getgamma() throws Exception
    {
        int output = 1;

        do
        {
            output <<= 1;
            output |= getbit();
        }
        while (getbit() == 1);

        return output;
    }

    public Decap(byte[] input)
    {
        this.input = input;
    }

    public byte[] depack() throws Exception
    {
        final DynamicByteArray output = new DynamicByteArray();

        inptr = 0;
        lwm = 0;
        roffs = 0;

//        output.write(getbyte());
//        output.write(getbyte());

        output.write(getbyte());

        int offs = 0;
        int len = 0;

        boolean done = false;

        while (!done)
        {
            int pfx = 0;
            while (pfx < 3 && getbit() == 1)
                pfx++;

            switch (pfx)
            {
                case 0:
                    output.write(getbyte());
                    lwm = 0;
                    break;

                case 1:
                    offs = getgamma();

                    if (lwm == 0 && offs == 2)
                    {
                        offs = roffs;
                        len = getgamma();

                        while (len-- > 0)
                            output.write(output.read(output.size() - offs));
                    }
                    else
                    {
                        offs -= lwm == 0 ? 3 : 2;
                        offs <<= 8;
                        offs |= getbyte();
                        len = getgamma();

                        if (offs < Constant.threshold_shortmatch_offset
                                || offs >= Constant.threshold_length3match_offset)
                            len += 2;
                        else if (offs >= Constant.threshold_length2match_offset)
                            len += 1;

                        while (len-- > 0)
                            output.write(output.read(output.size() - offs));

                        roffs = offs;
                    }
                    lwm = 1;
                    break;

                case 2:
                    offs = getbyte();
                    len = 2 + (offs & 1);
                    offs >>= 1;
                    if (offs > 0)
                    {
                        while (len-- > 0)
                            output.write(output.read(output.size() - offs));
                    }
                    else
                        done = true;

                    roffs = offs;
                    lwm = 1;
                    break;

                case 3:
                    offs = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        offs <<= 1;
                        offs |= getbit();
                    }

                    if (offs > 0)
                        output.write(output.read(output.size() - offs));
                    else
                        output.write(0);

                    lwm = 0;
                    break;

                default:
                    throw new Exception("decap depack b0rk");
            }
        }

        return output.toByteArray();
    }

}
