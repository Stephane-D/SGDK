package sgdk.tool;

/**
 * @author stephane
 */
public class TypeUtil
{

    public static String toString(boolean signed)
    {
        if (signed)
            return "signed";

        return "unsigned";
    }

    /**
     * Unsign the specified byte value and return it as int
     */
    public static int unsign(byte value)
    {
        return value & 0xFF;
    }

    /**
     * Unsign the specified short value and return it as int
     */
    public static int unsign(short value)
    {
        return value & 0xFFFF;
    }

    /**
     * Unsign the specified byte value and return it as long
     */
    public static long unsignL(byte value)
    {
        return value & 0xFFL;
    }

    /**
     * Unsign the specified short value and return it as long
     */
    public static long unsignL(short value)
    {
        return value & 0xFFFFL;
    }

    /**
     * Unsign the specified int value and return it as long
     */
    public static long unsign(int value)
    {
        return value & 0xFFFFFFFFL;
    }

    public static int toShort(byte value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static int toInt(byte value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static int toInt(short value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static int toInt(float value)
    {
        // we have to cast to long before else value is limited to
        // [Integer.MIN_VALUE..Integer.MAX_VALUE] range
        return (int) (long) value;
    }

    public static int toInt(double value)
    {
        // we have to cast to long before else value is limited to
        // [Integer.MIN_VALUE..Integer.MAX_VALUE] range
        return (int) (long) value;
    }

    public static long toLong(byte value, boolean signed)
    {
        if (signed)
            return value;

        return unsignL(value);
    }

    public static long toLong(short value, boolean signed)
    {
        if (signed)
            return value;

        return unsignL(value);
    }

    public static long toLong(int value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static float toFloat(byte value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static float toFloat(short value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static float toFloat(int value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static double toDouble(byte value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static double toDouble(short value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    public static double toDouble(int value, boolean signed)
    {
        if (signed)
            return value;

        return unsign(value);
    }

    /**
     * Safe integer evaluation from Integer object.<br>
     * Return <code>defaultValue</code> if specified object is null.
     */
    public static int getInt(Integer obj, int defaultValue)
    {
        if (obj == null)
            return defaultValue;

        return obj.intValue();
    }

    /**
     * Safe float evaluation from Float object.<br>
     * Return <code>defaultValue</code> if specified object is null.
     */
    public static float getFloat(Float obj, float defaultValue)
    {
        if (obj == null)
            return defaultValue;

        return obj.floatValue();
    }

    /**
     * Safe double evaluation from Double object.<br>
     * Return <code>defaultValue</code> if <code>obj</code> is null or equal to infinite with
     * <code>allowInfinite</code> set to false.
     */
    public static double getDouble(Double obj, double defaultValue, boolean allowInfinite)
    {
        if (obj == null)
            return defaultValue;

        final double result = obj.doubleValue();

        if ((!allowInfinite) && Double.isInfinite(result))
            return defaultValue;

        return result;
    }

    /**
     * Safe double evaluation from Double object.<br>
     * Return <code>defaultValue</code> if specified object is null.
     */
    public static double getDouble(Double obj, double defaultValue)
    {
        return getDouble(obj, defaultValue, true);
    }

    /**
     * Swap all nibbles in given 32bit int
     */
    public static int swapNibble32(int value)
    {
        return unsign(swapNibble16((short) (value >> 16))) | (unsign(swapNibble16((short) value)) << 16);
    }

    public static short swapNibble16(short value)
    {
        return (short) (unsign(swapNibble8((byte) (value >> 8))) | (unsign(swapNibble8((byte) value)) << 8));
    }

    public static byte swapNibble8(byte value)
    {
        return (byte) (((value >> 4) & 0xF) | (value << 4));
    }

    /**
     * Returns size in byte of given native type
     */
    public static int sizeOf(Class<?> dataType)
    {
        if (dataType == null)
            return 0;

        if (dataType == byte.class || dataType == Byte.class)
            return Byte.SIZE / 8;
        if (dataType == short.class || dataType == Short.class)
            return Short.SIZE / 8;
        if (dataType == char.class || dataType == Character.class)
            return Character.SIZE / 8;
        if (dataType == int.class || dataType == Integer.class)
            return Integer.SIZE / 8;
        if (dataType == long.class || dataType == Long.class)
            return Long.SIZE / 8;
        if (dataType == float.class || dataType == Float.class)
            return Float.SIZE / 8;
        if (dataType == double.class || dataType == Double.class)
            return Double.SIZE / 8;

        // default for 32-bit memory pointer
        return 4;
    }
}
