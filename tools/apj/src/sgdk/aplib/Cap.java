package sgdk.aplib;

public class Cap
{
    public static byte[] encode(byte[] data, boolean silent) throws Exception
    {
        byte[] output = new ToBinary(data, new Path(data).output()).get();

        if (!verify(data, output))
            throw new Exception("verify fail");

        if (!silent)
        {
            System.out.println("Initial size " + data.length + " --> packed to " + output.length + " ("
                    + ((100 * output.length) / data.length) + "%)");
        }

        return output;
    }

    public static byte[] decode(byte[] input, boolean silent) throws Exception
    {
        return new Decap(input).depack();
    }

    private static boolean verify(byte[] data, byte[] cdata) throws Exception
    {
        byte[] ddata = new Decap(cdata).depack();

        if (data.length != ddata.length)
            return false;

        for (int i = 0; i < data.length; i++)
            if (data[i] != ddata[i])
                return false;

        return true;
    }
}
