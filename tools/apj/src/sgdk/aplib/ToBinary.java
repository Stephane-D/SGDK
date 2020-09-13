package sgdk.aplib;

import java.io.IOException;

class ToBinary
{
    byte[] input;

    private int bitbuffer;
    private int bitbuffer_ptr;
    private int input_ptr;
    private DynamicByteArray result;
    private State state;

    public ToBinary(byte[] input, int[][] path) throws Exception
    {
        this.input = input;

        result = new DynamicByteArray();

        result.write(input[0]);
        input_ptr = 1;

        bitbuffer = 1;
        bitbuffer_ptr = result.size();
        result.write(0);

        state = new State();
        state = new State(state, path[0], input);

        for (int i = 1; i < path.length; i++)
            encodestep(path[i]);

        putbits(Constant.prefixcode_short_match);
        result.write(0);

        while (bitbuffer < 256)
            bitbuffer <<= 1;

        result.write(bitbuffer_ptr, (byte) bitbuffer);
    }

    public byte[] get()
    {
        return result.toByteArray();
    }

    private void encodestep(int[] edge) throws Exception
    {
        int length = edge[0];
        int offset = edge[1];

        if (!Model.valid(state, length, offset))
            throw new Exception("wat!");

        if (length == 1)
        {
            if (offset == 0 && input[state.index] != 0x00)
            {
                putbits(Constant.prefixcode_literal);
                result.write(input[input_ptr]);
            }
            else
            {
                putbits(Constant.prefixcode_byte_match);
                for (int s = 3; s >= 0; s--)
                    putbit(((offset >> s) & 1));
            }
        }
        else if (length >= 2 && offset == state.roffs && state.lwm() == 0)
        {
            putbits(Constant.prefixcode_normal_match);
            putgbits(2);
            putgbits(length);
        }
        else if (length >= 2 && length <= 3 && offset > 0 && offset < Constant.threshold_shortmatch_offset)
        {
            putbits(Constant.prefixcode_short_match);
            result.write((byte) (offset << 1 | (length & 1)));
        }
        else
        {
            putbits(Constant.prefixcode_normal_match);
            putgbits((offset >> 8) + (state.lwm() == 0 ? 3 : 2));
            result.write((byte) (offset & 0xff));
            putgbits(length
                    - (offset < Constant.threshold_shortmatch_offset || offset >= Constant.threshold_length3match_offset
                            ? 2 : offset >= Constant.threshold_length2match_offset ? 1 : 0));
        }
        
        state = new State(state, edge, input);
        input_ptr += length;
    }

    private void putbit(int bit) throws IOException
    {
        if (bitbuffer > 0xff)
        {
            bitbuffer &= 0xff;
            result.write(bitbuffer_ptr, (byte) bitbuffer);
            bitbuffer_ptr = result.size();
            result.write(0);
            bitbuffer = 1;
        }

        bitbuffer <<= 1;
        bitbuffer |= bit;
    }

    private void putbits(int[] bits) throws IOException
    {
        for (int bit : bits)
            putbit(bit);
    }

    private void putgbits(int input) throws IOException
    {
        int msb = 15;
        while (input >> msb-- == 0)
            ;

        while (msb >= 0)
        {
            int bit = (input >> msb) & 1;
            putbit(bit);
            msb--;
            putbit(msb >= 0 ? 1 : 0);
        }
    }
}
