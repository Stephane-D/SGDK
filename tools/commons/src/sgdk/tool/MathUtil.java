package sgdk.tool;

/**
 * Math utilities class.
 * 
 * @author stephane
 */
public class MathUtil
{
    public static final String INFINITE_STRING = "\u221E";

    public static final double POW2_8_DOUBLE = Math.pow(2, 8);
    public static final float POW2_8_FLOAT = (float) POW2_8_DOUBLE;
    public static final double POW2_16_DOUBLE = Math.pow(2, 16);
    public static final float POW2_16_FLOAT = (float) POW2_16_DOUBLE;
    public static final double POW2_32_DOUBLE = Math.pow(2, 32);
    public static final float POW2_32_FLOAT = (float) POW2_32_DOUBLE;
    public static final double POW2_64_DOUBLE = Math.pow(2, 64);
    public static final float POW2_64_FLOAT = (float) POW2_64_DOUBLE;

    public static double frac(double value)
    {
        return value - Math.floor(value);
    }

    /**
     * Round specified value to specified number of significant digit.<br>
     * If keepInteger is true then integer part of number is entirely conserved.<br>
     * If <i>numDigit</i> is &lt;= 0 then the value stay unchanged.
     */
    public static double roundSignificant(double d, int numDigit, boolean keepInteger)
    {
        if ((numDigit <= 0) || (d == 0d))
            return d;

        final double digit = Math.ceil(Math.log10(Math.abs(d)));
        if ((digit >= numDigit) && keepInteger)
            return Math.round(d);

        return round(d, numDigit - (int) digit);
    }

    /**
     * Round specified value to specified number of significant digit.
     */
    public static double roundSignificant(double d, int numDigit)
    {
        return roundSignificant(d, numDigit, false);
    }

    /**
     * Round specified value to specified number of decimal.
     */
    public static double round(double d, int numDecimal)
    {
        final double pow = Math.pow(10, numDecimal);
        return Math.round(d * pow) / pow;
    }

    /**
     * Return the previous multiple of "mul" for the specified value
     * <ul>
     * <li>prevMultiple(200, 64) = 192</li>
     * </ul>
     * 
     * @param value
     * @param mul
     */
    public static double prevMultiple(double value, double mul)
    {
        if (mul == 0)
            return 0d;

        return Math.floor(value / mul) * mul;
    }

    /**
     * Return the next multiple of "mul" for the specified value
     * <ul>
     * <li>nextMultiple(200, 64) = 256</li>
     * </ul>
     * 
     * @param value
     * @param mul
     */
    public static double nextMultiple(double value, double mul)
    {
        if (mul == 0)
            return 0d;

        return Math.ceil(value / mul) * mul;
    }

    /**
     * Return the next power of 2 for the specified value
     * <ul>
     * <li>nextPow2(17) = 32</li>
     * <li>nextPow2(16) = 32</li>
     * <li>nextPow2(-12) = -8</li>
     * <li>nextPow2(-8) = -4</li>
     * </ul>
     * 
     * @param value
     * @return next power of 2
     */
    public static long nextPow2(long value)
    {
        long result;

        if (value < 0)
        {
            result = -1;
            while (result > value)
                result <<= 1;
            result >>= 1;
        }
        else
        {
            result = 1;
            while (result <= value)
                result <<= 1;
        }

        return result;
    }

    /**
     * Return the next power of 2 mask for the specified value
     * <ul>
     * <li>nextPow2Mask(17) = 31</li>
     * <li>nextPow2Mask(16) = 31</li>
     * <li>nextPow2Mask(-12) = -8</li>
     * <li>nextPow2Mask(-8) = -4</li>
     * </ul>
     * 
     * @param value
     * @return next power of 2 mask
     */
    public static long nextPow2Mask(long value)
    {
        final long result = nextPow2(value);
        if (value > 0)
            return result - 1;

        return result;
    }

    /**
     * Return the previous power of 2 for the specified value
     * <ul>
     * <li>prevPow2(17) = 16</li>
     * <li>prevPow2(16) = 8</li>
     * <li>prevPow2(-12) = -16</li>
     * <li>prevPow2(-8) = -16</li>
     * </ul>
     * 
     * @param value
     * @return previous power of 2
     */
    public static long prevPow2(long value)
    {
        long result;

        if (value < 0)
        {
            result = -1;
            while (result >= value)
                result <<= 1;
        }
        else
        {
            result = 1;
            while (result < value)
                result <<= 1;
            result >>= 1;
        }

        return result;
    }

    /**
     * Return the next power of 10 for the specified value
     * <ul>
     * <li>nextPow10(0.0067) = 0.01</li>
     * <li>nextPow10(-28.7) = -10</li>
     * </ul>
     * 
     * @param value
     */
    public static double nextPow10(double value)
    {
        if (value == 0)
            return 0;
        else if (value < 0)
            return -Math.pow(10, Math.floor(Math.log10(-value)));
        else
            return Math.pow(10, Math.ceil(Math.log10(value)));
    }

    /**
     * Return the previous power of 10 for the specified value
     * <ul>
     * <li>prevPow10(0.0067) = 0.001</li>
     * <li>prevPow10(-28.7) = -100</li>
     * </ul>
     * 
     * @param value
     */
    public static double prevPow10(double value)
    {
        if (value == 0)
            return 0;
        else if (value < 0)
            return -Math.pow(10, Math.ceil(Math.log10(-value)));
        else
            return Math.pow(10, Math.floor(Math.log10(value)));
    }

    /**
     * Format the specified degree angle to stay in [0..360[ range
     */
    public static double formatDegreeAngle(double angle)
    {
        final double res = angle % 360d;

        if (res < 0)
            return 360d + res;

        return res;
    }

    /**
     * Format the specified degree angle to stay in [-180..180] range
     */
    public static double formatDegreeAngle2(double angle)
    {
        final double res = angle % 360d;

        if (res < -180d)
            return 360d + res;
        if (res > 180d)
            return res - 360d;

        return res;
    }

    /**
     * Format the specified degree angle to stay in [0..2PI[ range
     */
    public static double formatRadianAngle(double angle)
    {
        final double res = angle % (2 * Math.PI);

        if (res < 0)
            return (2 * Math.PI) + res;

        return res;
    }

    /**
     * Format the specified degree angle to stay in [-PI..PI] range
     */
    public static double formatRadianAngle2(double angle)
    {
        final double res = angle % (2 * Math.PI);

        if (res < -Math.PI)
            return (2 * Math.PI) + res;
        if (res > Math.PI)
            return res - (2 * Math.PI);

        return res;
    }

    /**
     * Return the value in <code>source</code> which is the closest to <code>value</code> Returns
     * <code>0</code> if source is null or empty.
     */
    public static double closest(double value, double[] source)
    {
        if ((source == null) || (source.length == 0))
            return 0d;

        double result = source[0];
        double minDelta = Math.abs(value - result);

        for (double d : source)
        {
            final double delta = Math.abs(value - d);

            if (delta < minDelta)
            {
                result = d;
                minDelta = delta;
            }
        }

        return result;
    }

    /**
     * Calculate the cubic root of the specified value.
     */
    public static double cubicRoot(double value)
    {
        return Math.pow(value, 1d / 3d);
    }
}
