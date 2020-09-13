package sgdk.aplib;

class Model
{

    static public boolean valid(int length, int offset)
    {
        boolean output = false;

        switch (length)
        {
            case 1:
                output = offset < Constant.threshold_bytematch_offset;
                break;
            case 2:
                output = offset < Constant.threshold_length2match_offset;
                break;
            case 3:
                output = offset < Constant.threshold_length3match_offset;
                break;
            default:
                output = offset < Constant.threshold_offset;
                break;
        }
        return output;
    }

    static public boolean valid(State state, int length, int offset) throws Exception
    {
        boolean output = false;

        if (state != null)
        {
            if (length == 1)
            {
                if (offset == 0)
                    output = true;
                else if (offset > -1 && offset < Constant.threshold_bytematch_offset)
                {
                    if (state.lwm() == 0 || (state.lwm() == 1 && offset != state.roffs))
                        output = true;
                }
            }
            else if (length > 1)
            {
                if (offset == state.roffs && state.lwm() == 0 && state.roffs != 0 && offset != state.edge[1])
                    output = true;
                else if (offset != state.edge[1])
                {
                    if (length == 2 && offset > 0 && offset < Constant.threshold_length2match_offset)
                        output = true;
                    else if (length == 3 && offset > 0 && offset < Constant.threshold_length3match_offset)
                        output = true;
                    else if (length > 3 && offset > 0 && offset < Constant.threshold_offset)
                    {
                        if (!(offset == state.roffs && state.lwm() == 1))// redundant
                            output = true;
                    }
                }
            }
        }
        else
            throw new Exception("no state");

        return output;
    }

    static public int cost(State state, int[] edge, byte[] input) throws Exception
    {
        int output = 0;

        int length = edge[0];
        int offset = edge[1];

        if (length == 1)
        {
            if (offset < 0 || offset >= Constant.threshold_bytematch_offset)
                throw new Exception("WAT!");

            if (offset > 0 && offset < Constant.threshold_bytematch_offset || offset == 0 && input[state.index] == 0)
                output += Constant.bitcost_prefixcode_bytematch + Constant.bitcost_offset_bytematch;
            else/* if (edge.offset == 0) */
                output += Constant.bitcost_prefixcode_literal + Constant.bitcost_literal;
        }
        else if (length >= 2 && offset == state.roffs && state.lwm() == 0)
        {
            output += Constant.bitcost_prefixcode_normalmatch;
            output += togammacount(2);
            output += togammacount(length);
        }
        else if (length >= 2 && length <= 3 && offset > 0 && offset < Constant.threshold_shortmatch_offset)
        {
            output += Constant.bitcost_prefixcode_shortmatch;
            output += Constant.bitcost_literal;
        }
        else if ((length == 2 && offset >= Constant.threshold_shortmatch_offset
                && offset < Constant.threshold_length2match_offset)
                || (length == 3 && offset >= Constant.threshold_shortmatch_offset
                        && offset < Constant.threshold_length3match_offset)
                || (length >= 4 && offset >= 1 && offset < Constant.threshold_offset))
        {
            output += Constant.bitcost_prefixcode_normalmatch;

            int offset_hi = (offset >> 8);
            offset_hi += 2;

            if (state.lwm() == 0)
                offset_hi += 1;

            output += togammacount(offset_hi);
            output += 8;

            int ldo = 0;
            if (offset < 128 || offset >= Constant.threshold_length3match_offset)
                ldo = 2;
            else if (offset >= Constant.threshold_length2match_offset)
                ldo = 1;

            output += togammacount(length - ldo);
        }
        else
            throw new Exception("WAT!");

        return output;
    }

    public static int togammacount(int input) throws Exception
    {
        if (input < 2 || input > 65535)
            throw new Exception("model togammacount b0rk @ " + input);

        return 2 * flog2(input);

        // return 2 * (int)System.Math.Log(input, 2);

        /*
         * int output = 1;
         * 
         * while (input >> output > 1)
         * {
         * output++;
         * }
         * output <<= 1;
         * 
         * return output;
         */
    }

    private static int flog2(int v)
    {
        // int r = (((v > 0xFFFF)) ? 0x10 : 0);
        // v >>= r;

        int r = 0;
        int shift = ((v > 0xFF) ? 0x8 : 0);
        v >>= shift;
        r |= shift;
        shift = ((v > 0xF) ? 0x4 : 0);
        v >>= shift;
        r |= shift;
        shift = ((v > 0x3) ? 0x2 : 0);
        v >>= shift;
        r |= shift;
        r |= (v >> 1);

        return r;
    }
}
