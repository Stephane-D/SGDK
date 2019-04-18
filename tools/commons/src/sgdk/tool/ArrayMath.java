package sgdk.tool;

/**
 * Class defining basic arithmetic and statistic operations on 1D double arrays.
 */
public class ArrayMath
{
    /**
     * Computes the absolute value of each value of the given double array
     * 
     * @param overwrite
     *        true overwrites the input data, false returns the result in a new structure
     */
    public static double[] abs(double[] input, boolean overwrite)
    {
        final double[] result = overwrite ? input : new double[input.length];

        for (int i = 0; i < input.length; i++)
            result[i] = Math.abs(input[i]);

        return result;
    }

    /**
     * Computes the absolute value of each value of the given float array
     * 
     * @param overwrite
     *        true overwrites the input data, false returns the result in a new structure
     */
    public static float[] abs(float[] input, boolean overwrite)
    {
        final float[] result = overwrite ? input : new float[input.length];

        for (int i = 0; i < input.length; i++)
            result[i] = Math.abs(input[i]);

        return result;
    }

    /**
     * Computes the absolute value of each value of the given long array
     * 
     * @param overwrite
     *        true overwrites the input data, false returns the result in a new structure
     */
    public static long[] abs(long[] input, boolean overwrite)
    {
        final long[] result = overwrite ? input : new long[input.length];

        for (int i = 0; i < input.length; i++)
            result[i] = Math.abs(input[i]);

        return result;
    }

    /**
     * Computes the absolute value of each value of the given int array
     * 
     * @param overwrite
     *        true overwrites the input data, false returns the result in a new structure
     */
    public static int[] abs(int[] input, boolean overwrite)
    {
        final int[] result = overwrite ? input : new int[input.length];

        for (int i = 0; i < input.length; i++)
            result[i] = Math.abs(input[i]);

        return result;
    }

    /**
     * Computes the absolute value of each value of the given short array
     * 
     * @param overwrite
     *        true overwrites the input data, false returns the result in a new structure
     */
    public static short[] abs(short[] input, boolean overwrite)
    {
        final short[] result = overwrite ? input : new short[input.length];

        for (int i = 0; i < input.length; i++)
            result[i] = (short) Math.abs(input[i]);

        return result;
    }

    /**
     * Computes the absolute value of each value of the given byte array
     * 
     * @param overwrite
     *        true overwrites the input data, false returns the result in a new structure
     */
    public static byte[] abs(byte[] input, boolean overwrite)
    {
        final byte[] result = overwrite ? input : new byte[input.length];

        for (int i = 0; i < input.length; i++)
            result[i] = (byte) Math.abs(input[i]);

        return result;
    }

    /**
     * Find the minimum value of an array
     * 
     * @param array
     *        an array
     * @param signed
     *        signed / unsigned flag
     * @return the min value of the array
     */
    public static int min(byte[] array, boolean signed)
    {
        if (signed)
        {
            byte min = Byte.MAX_VALUE;

            for (byte v : array)
                if (v < min)
                    min = v;

            return min;
        }

        int min = Integer.MAX_VALUE;

        for (int i = 0; i < array.length; i++)
        {
            final int v = TypeUtil.unsign(array[i]);
            if (v < min)
                min = v;
        }

        return min;
    }

    /**
     * Find the minimum value of an array
     * 
     * @param array
     *        an array
     * @param signed
     *        signed / unsigned flag
     * @return the min value of the array
     */
    public static int min(short[] array, boolean signed)
    {
        if (signed)
        {
            short min = Short.MAX_VALUE;

            for (short v : array)
                if (v < min)
                    min = v;

            return min;
        }

        int min = Integer.MAX_VALUE;

        for (int i = 0; i < array.length; i++)
        {
            final int v = TypeUtil.unsign(array[i]);
            if (v < min)
                min = v;
        }

        return min;
    }

    /**
     * Find the minimum value of an array
     * 
     * @param array
     *        an array
     * @param signed
     *        signed / unsigned flag
     * @return the min value of the array
     */
    public static long min(int[] array, boolean signed)
    {
        if (signed)
        {
            int min = Integer.MAX_VALUE;

            for (int v : array)
                if (v < min)
                    min = v;

            return min;
        }

        long min = Long.MAX_VALUE;

        for (int i = 0; i < array.length; i++)
        {
            final long v = TypeUtil.unsign(array[i]);
            if (v < min)
                min = v;
        }

        return min;
    }

    /**
     * Find the minimum value of an array
     * 
     * @param array
     *        an array
     * @return the min value of the array
     */
    public static float min(float[] array)
    {
        float min = Float.MAX_VALUE;

        for (float v : array)
            if (v < min)
                min = v;

        return min;
    }

    /**
     * Find the minimum value of an array
     * 
     * @param array
     *        an array
     * @return the min value of the array
     */
    public static double min(double[] array)
    {
        double min = Double.MAX_VALUE;

        for (double v : array)
            if (v < min)
                min = v;

        return min;
    }

    /**
     * Find the maximum value of an array
     * 
     * @param array
     *        an array
     * @param signed
     *        signed / unsigned flag
     * @return the max value of the array
     */
    public static int max(byte[] array, boolean signed)
    {
        if (signed)
        {
            byte max = Byte.MIN_VALUE;

            for (byte v : array)
                if (v > max)
                    max = v;

            return max;
        }

        int max = Integer.MIN_VALUE;

        for (int i = 0; i < array.length; i++)
        {
            final int v = TypeUtil.unsign(array[i]);
            if (v > max)
                max = v;
        }

        return max;
    }

    /**
     * Find the maximum value of an array
     * 
     * @param array
     *        an array
     * @param signed
     *        signed / unsigned flag
     * @return the max value of the array
     */
    public static int max(short[] array, boolean signed)
    {
        if (signed)
        {
            short max = Short.MIN_VALUE;

            for (short v : array)
                if (v > max)
                    max = v;

            return max;
        }

        int max = Integer.MIN_VALUE;

        for (int i = 0; i < array.length; i++)
        {
            final int v = TypeUtil.unsign(array[i]);
            if (v > max)
                max = v;
        }

        return max;
    }

    /**
     * Find the maximum value of an array
     * 
     * @param array
     *        an array
     * @param signed
     *        signed / unsigned flag
     * @return the max value of the array
     */
    public static long max(int[] array, boolean signed)
    {
        if (signed)
        {
            int max = Integer.MIN_VALUE;

            for (int v : array)
                if (v > max)
                    max = v;

            return max;
        }

        long max = Long.MIN_VALUE;

        for (int i = 0; i < array.length; i++)
        {
            final long v = TypeUtil.unsign(array[i]);
            if (v > max)
                max = v;
        }

        return max;
    }

    /**
     * Find the maximum value of an array
     * 
     * @param array
     *        an array
     * @return the max value of the array
     */
    public static float max(float[] array)
    {
        float max = -Float.MAX_VALUE;

        for (float v : array)
            if (v > max)
                max = v;

        return max;
    }

    /**
     * Find the maximum value of an array
     * 
     * @param array
     *        an array
     * @return the max value of the array
     */
    public static double max(double[] array)
    {
        double max = -Double.MAX_VALUE;

        for (double v : array)
            if (v > max)
                max = v;

        return max;
    }

    /**
     * Element-wise minimum of two arrays
     * 
     * @param a1
     *        =input1
     * @param a2
     *        =input2
     * @param output
     *        - the array of min values
     */
    public static void min(double[] a1, double[] a2, double[] output)
    {
        for (int i = 0; i < a1.length; i++)
            if (a1[i] <= a2[i])
                output[i] = a1[i];
            else
                output[i] = a2[i];
    }

    /**
     * Element-wise minimum of two arrays
     * 
     * @param a1
     *        =input1
     * @param a2
     *        =input2
     * @return the array of min values
     */
    public static double[] min(double[] a1, double[] a2)
    {
        double[] result = new double[a1.length];
        min(a1, a2, result);
        return result;
    }

    /**
     * Element-wise maximum of two arrays
     * 
     * @param a1
     *        =input1
     * @param a2
     *        =input2
     * @param output
     *        - the array of max values
     */
    public static void max(double[] a1, double[] a2, double[] output)
    {
        for (int i = 0; i < a1.length; i++)
            if (a1[i] >= a2[i])
                output[i] = a1[i];
            else
                output[i] = a2[i];
    }

    /**
     * Element-wise maximum of two arrays
     * 
     * @param a1
     *        =input1
     * @param a2
     *        =input2
     * @return the array of max values
     */
    public static double[] max(double[] a1, double[] a2)
    {
        double[] result = new double[a1.length];
        max(a1, a2, result);
        return result;
    }
}
