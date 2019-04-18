package sgdk.tool;

import java.lang.reflect.Array;
import java.util.Arrays;

/**
 * @author Stephane
 */
public class ArrayUtil
{
    /**
     * Return the type (java type only) of the specified array.
     */
    public static Class<?> getComponentType(Object array)
    {
        Class<? extends Object> arrayClass = array.getClass();
        while (arrayClass.isArray())
            arrayClass = arrayClass.getComponentType();

        return arrayClass;
    }

    /**
     * Return the type (java type only) of the specified array.
     */
    public static int getComponentSize(Object array)
    {
        return TypeUtil.sizeOf(getComponentType(array));
    }

    /**
     * Return the number of element of the specified array
     */
    public static int getLength(Object array)
    {
        if (array != null)
            return Array.getLength(array);

        // null array
        return 0;
    }

    /**
     * Return the total number of element of the specified array
     */
    public static int getTotalLength(byte[] array)
    {
        if (array != null)
            return array.length;

        return 0;
    }

    /**
     * Return the total number of element of the specified array
     */
    public static int getTotalLength(short[] array)
    {
        if (array != null)
            return array.length;

        return 0;
    }

    /**
     * Return the total number of element of the specified array
     */
    public static int getTotalLength(int[] array)
    {
        if (array != null)
            return array.length;

        return 0;
    }

    /**
     * Return the total number of element of the specified array
     */
    public static int getTotalLength(long[] array)
    {
        if (array != null)
            return array.length;

        return 0;
    }

    /**
     * Return the total number of element of the specified array
     */
    public static int getTotalLength(float[] array)
    {
        if (array != null)
            return array.length;

        return 0;
    }

    /**
     * Return the total number of element of the specified array
     */
    public static int getTotalLength(double[] array)
    {
        if (array != null)
            return array.length;

        return 0;
    }

    /**
     * Get maximum length for a copy from in to out with specified offset.<br>
     * If specified length != -1 then the value is directly returned.
     */
    static int getCopyLength(Object in, int inOffset, Object out, int outOffset, int length)
    {
        if (length == -1)
            return getCopyLength(in, inOffset, out, outOffset);

        return length;
    }

    /**
     * Get maximum length for a copy from in to out with specified offset.
     */
    public static int getCopyLength(Object in, int inOffset, Object out, int outOffset)
    {
        // 'in' object can't be null !
        final int len = getCopyLength(in, inOffset);

        if (out == null)
            return len;

        return Math.min(len, getCopyLength(out, outOffset));
    }

    /**
     * Get length for a copy from or to the specified array with specified offset
     */
    public static int getCopyLength(Object array, int offset)
    {
        return getLength(array) - offset;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified len
     */
    public static boolean[] allocIfNull(boolean[] out, int len)
    {
        if (out == null)
            return new boolean[len];

        return out;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified len
     */
    public static byte[] allocIfNull(byte[] out, int len)
    {
        if (out == null)
            return new byte[len];

        return out;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified len
     */
    public static short[] allocIfNull(short[] out, int len)
    {
        if (out == null)
            return new short[len];

        return out;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified len
     */
    public static int[] allocIfNull(int[] out, int len)
    {
        if (out == null)
            return new int[len];

        return out;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified len
     */
    public static long[] allocIfNull(long[] out, int len)
    {
        if (out == null)
            return new long[len];

        return out;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified len
     */
    public static float[] allocIfNull(float[] out, int len)
    {
        if (out == null)
            return new float[len];

        return out;
    }

    /**
     * Allocate the specified array if it's defined to null with the specified lenght
     */
    public static double[] allocIfNull(double[] out, int len)
    {
        if (out == null)
            return new double[len];

        return out;
    }

    /**
     * Get value as double from specified byte array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static double getValue(byte[] array, int offset, boolean signed)
    {
        return TypeUtil.toDouble(array[offset], signed);
    }

    /**
     * Get value as double from specified short array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static double getValue(short[] array, int offset, boolean signed)
    {
        return TypeUtil.toDouble(array[offset], signed);
    }

    /**
     * Get value as double from specified int array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static double getValue(int[] array, int offset, boolean signed)
    {
        return TypeUtil.toDouble(array[offset], signed);
    }

    /**
     * Get value as double from specified float array and offset.
     */
    public static double getValue(float[] array, int offset)
    {
        return array[offset];
    }

    /**
     * Get value as double from specified double array and offset.
     */
    public static double getValue(double[] array, int offset)
    {
        return array[offset];
    }

    //

    /**
     * Get value as float from specified byte array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static float getValueAsFloat(byte[] array, int offset, boolean signed)
    {
        return TypeUtil.toFloat(array[offset], signed);
    }

    /**
     * Get value as float from specified short array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static float getValueAsFloat(short[] array, int offset, boolean signed)
    {
        return TypeUtil.toFloat(array[offset], signed);
    }

    /**
     * Get value as float from specified int array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static float getValueAsFloat(int[] array, int offset, boolean signed)
    {
        return TypeUtil.toFloat(array[offset], signed);
    }

    /**
     * Get value as float from specified float array and offset.
     */
    public static float getValueAsFloat(float[] array, int offset)
    {
        return array[offset];
    }

    /**
     * Get value as float from specified double array and offset.
     */
    public static float getValueAsFloat(double[] array, int offset)
    {
        return (float) array[offset];
    }

    //

    /**
     * Get value as int from specified byte array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static int getValueAsInt(byte[] array, int offset, boolean signed)
    {
        return TypeUtil.toInt(array[offset], signed);
    }

    /**
     * Get value as int from specified short array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static int getValueAsInt(short[] array, int offset, boolean signed)
    {
        return TypeUtil.toInt(array[offset], signed);
    }

    /**
     * Get value as int from specified int array and offset.<br>
     */
    public static int getValueAsInt(int[] array, int offset)
    {
        // can't unsign here
        return array[offset];
    }

    /**
     * Get value as int from specified long array and offset.<br>
     */
    public static int getValueAsInt(long[] array, int offset)
    {
        return (int) array[offset];
    }

    /**
     * Get value as int from specified float array and offset.
     */
    public static int getValueAsInt(float[] array, int offset)
    {
        return (int) array[offset];
    }

    /**
     * Get value as int from specified double array and offset.
     */
    public static int getValueAsInt(double[] array, int offset)
    {
        return (int) array[offset];
    }

    //

    /**
     * Get value as int from specified byte array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static long getValueAsLong(byte[] array, int offset, boolean signed)
    {
        return TypeUtil.toLong(array[offset], signed);
    }

    /**
     * Get value as int from specified short array and offset.<br>
     * If signed is true then we consider data as signed
     */
    public static long getValueAsLong(short[] array, int offset, boolean signed)
    {
        return TypeUtil.toLong(array[offset], signed);

    }

    /**
     * Get value as int from specified int array and offset.<br>
     */
    public static long getValueAsLong(int[] array, int offset, boolean signed)
    {
        return TypeUtil.toLong(array[offset], signed);

    }

    /**
     * Get value as int from specified long array and offset.<br>
     */
    public static long getValueAsLong(long[] array, int offset)
    {
        // can't unsign here
        return array[offset];
    }

    /**
     * Get value as int from specified float array and offset.
     */
    public static long getValueAsLong(float[] array, int offset)
    {
        return (long) array[offset];
    }

    /**
     * Get value as int from specified double array and offset.
     */
    public static long getValueAsLong(double[] array, int offset)
    {
        return (long) array[offset];
    }

    /**
     * Set value at specified offset as double value.
     */
    public static void setValue(byte[] array, int offset, double value)
    {
        array[offset] = (byte) value;
    }

    /**
     * Set value at specified offset as double value.
     */
    public static void setValue(short[] array, int offset, double value)
    {
        array[offset] = (short) value;
    }

    /**
     * Set value at specified offset as double value.
     */
    public static void setValue(int[] array, int offset, double value)
    {
        array[offset] = (int) value;
    }

    /**
     * Set value at specified offset as double value.
     */
    public static void setValue(long[] array, int offset, double value)
    {
        array[offset] = (long) value;
    }

    /**
     * Set value at specified offset as double value.
     */
    public static void setValue(float[] array, int offset, double value)
    {
        array[offset] = (float) value;
    }

    /**
     * Set value at specified offset as double value.
     */
    public static void setValue(double[] array, int offset, double value)
    {
        array[offset] = value;
    }

    /**
     * Return true is the specified arrays are equals
     */
    public static boolean arrayByteCompare(byte[] array1, byte[] array2)
    {
        return Arrays.equals(array1, array2);
    }

    /**
     * Return true is the specified arrays are equals
     */
    public static boolean arrayShortCompare(short[] array1, short[] array2)
    {
        return Arrays.equals(array1, array2);
    }

    /**
     * Return true is the specified arrays are equals
     */
    public static boolean arrayIntCompare(int[] array1, int[] array2)
    {
        return Arrays.equals(array1, array2);
    }

    /**
     * Return true is the specified arrays are equals
     */
    public static boolean arrayLongCompare(long[] array1, long[] array2)
    {
        return Arrays.equals(array1, array2);
    }

    /**
     * Return true is the specified arrays are equals
     */
    public static boolean arrayFloatCompare(float[] array1, float[] array2)
    {
        return Arrays.equals(array1, array2);
    }

    /**
     * Return true is the specified arrays are equals
     */
    public static boolean arrayDoubleCompare(double[] array1, double[] array2)
    {
        return Arrays.equals(array1, array2);
    }

    /**
     * Same as {@link Arrays#fill(byte[], int, int, byte)}
     */
    public static void fill(byte[] array, int from, int to, byte value)
    {
        for (int i = from; i < to; i++)
            array[i] = value;
    }

    /**
     * Same as {@link Arrays#fill(short[], int, int, short)}
     */
    public static void fill(short[] array, int from, int to, short value)
    {
        for (int i = from; i < to; i++)
            array[i] = value;
    }

    /**
     * Same as {@link Arrays#fill(int[], int, int, int)}
     */
    public static void fill(int[] array, int from, int to, int value)
    {
        for (int i = from; i < to; i++)
            array[i] = value;
    }

    /**
     * Same as {@link Arrays#fill(long[], int, int, long)}
     */
    public static void fill(long[] array, int from, int to, long value)
    {
        for (int i = from; i < to; i++)
            array[i] = value;
    }

    /**
     * Same as {@link Arrays#fill(float[], int, int, float)}
     */
    public static void fill(float[] array, int from, int to, float value)
    {
        for (int i = from; i < to; i++)
            array[i] = value;
    }

    /**
     * Same as {@link Arrays#fill(double[], int, int, double)}
     */
    public static void fill(double[] array, int from, int to, double value)
    {
        for (int i = from; i < to; i++)
            array[i] = value;
    }

    /**
     * Copy 'cnt' elements from 'from' index to 'to' index in a safe manner (no overlap)
     */
    public static void innerCopy(byte[] array, int from, int to, int cnt)
    {
        final int delta = to - from;

        if ((array == null) || (delta == 0))
            return;

        final int length = array.length;

        if ((from < 0) || (to < 0) || (from >= length) || (to >= length))
            return;

        final int adjCnt;

        // forward copy
        if (delta < 0)
        {
            // adjust copy size
            if ((from + cnt) >= length)
                adjCnt = length - from;
            else
                adjCnt = cnt;

            int to_ = to;
            int from_ = from;
            for (int i = 0; i < adjCnt; i++)
                array[to_++] = array[from_++];
        }
        else
        // backward copy
        {
            // adjust copy size
            if ((to + cnt) >= length)
                adjCnt = length - to;
            else
                adjCnt = cnt;

            int to_ = to + cnt;
            int from_ = from + cnt;
            for (int i = 0; i < adjCnt; i++)
                array[--to_] = array[--from_];
        }
    }

    /**
     * Copy 'cnt' elements from 'from' index to 'to' index in a safe manner (no overlap)
     */
    public static void innerCopy(short[] array, int from, int to, int cnt)
    {
        final int delta = to - from;

        if ((array == null) || (delta == 0))
            return;

        final int length = array.length;

        if ((from < 0) || (to < 0) || (from >= length) || (to >= length))
            return;

        final int adjCnt;

        // forward copy
        if (delta < 0)
        {
            // adjust copy size
            if ((from + cnt) >= length)
                adjCnt = length - from;
            else
                adjCnt = cnt;

            int to_ = to;
            int from_ = from;
            for (int i = 0; i < adjCnt; i++)
                array[to_++] = array[from_++];
        }
        else
        // backward copy
        {
            // adjust copy size
            if ((to + cnt) >= length)
                adjCnt = length - to;
            else
                adjCnt = cnt;

            int to_ = to + cnt;
            int from_ = from + cnt;
            for (int i = 0; i < adjCnt; i++)
                array[--to_] = array[--from_];
        }
    }

    /**
     * Copy 'cnt' elements from 'from' index to 'to' index in a safe manner (no overlap)
     */
    public static void innerCopy(int[] array, int from, int to, int cnt)
    {
        final int delta = to - from;

        if ((array == null) || (delta == 0))
            return;

        final int length = array.length;

        if ((from < 0) || (to < 0) || (from >= length) || (to >= length))
            return;

        final int adjCnt;

        // forward copy
        if (delta < 0)
        {
            // adjust copy size
            if ((from + cnt) >= length)
                adjCnt = length - from;
            else
                adjCnt = cnt;

            int to_ = to;
            int from_ = from;
            for (int i = 0; i < adjCnt; i++)
                array[to_++] = array[from_++];
        }
        else
        // backward copy
        {
            // adjust copy size
            if ((to + cnt) >= length)
                adjCnt = length - to;
            else
                adjCnt = cnt;

            int to_ = to + cnt;
            int from_ = from + cnt;
            for (int i = 0; i < adjCnt; i++)
                array[--to_] = array[--from_];
        }
    }

    /**
     * Copy 'cnt' elements from 'from' index to 'to' index in a safe manner (no overlap)
     */
    public static void innerCopy(long[] array, int from, int to, int cnt)
    {
        final int delta = to - from;

        if ((array == null) || (delta == 0))
            return;

        final int length = array.length;

        if ((from < 0) || (to < 0) || (from >= length) || (to >= length))
            return;

        final int adjCnt;

        // forward copy
        if (delta < 0)
        {
            // adjust copy size
            if ((from + cnt) >= length)
                adjCnt = length - from;
            else
                adjCnt = cnt;

            int to_ = to;
            int from_ = from;
            for (int i = 0; i < adjCnt; i++)
                array[to_++] = array[from_++];
        }
        else
        // backward copy
        {
            // adjust copy size
            if ((to + cnt) >= length)
                adjCnt = length - to;
            else
                adjCnt = cnt;

            int to_ = to + cnt;
            int from_ = from + cnt;
            for (int i = 0; i < adjCnt; i++)
                array[--to_] = array[--from_];
        }
    }

    /**
     * Copy 'cnt' elements from 'from' index to 'to' index in a safe manner (no overlap)
     */
    public static void innerCopy(float[] array, int from, int to, int cnt)
    {
        final int delta = to - from;

        if ((array == null) || (delta == 0))
            return;

        final int length = array.length;

        if ((from < 0) || (to < 0) || (from >= length) || (to >= length))
            return;

        final int adjCnt;

        // forward copy
        if (delta < 0)
        {
            // adjust copy size
            if ((from + cnt) >= length)
                adjCnt = length - from;
            else
                adjCnt = cnt;

            int to_ = to;
            int from_ = from;
            for (int i = 0; i < adjCnt; i++)
                array[to_++] = array[from_++];
        }
        else
        // backward copy
        {
            // adjust copy size
            if ((to + cnt) >= length)
                adjCnt = length - to;
            else
                adjCnt = cnt;

            int to_ = to + cnt;
            int from_ = from + cnt;
            for (int i = 0; i < adjCnt; i++)
                array[--to_] = array[--from_];
        }
    }

    /**
     * Copy 'cnt' elements from 'from' index to 'to' index in a safe manner (no overlap)
     */
    public static void innerCopy(double[] array, int from, int to, int cnt)
    {
        final int delta = to - from;

        if ((array == null) || (delta == 0))
            return;

        final int length = array.length;

        if ((from < 0) || (to < 0) || (from >= length) || (to >= length))
            return;

        final int adjCnt;

        // forward copy
        if (delta < 0)
        {
            // adjust copy size
            if ((from + cnt) >= length)
                adjCnt = length - from;
            else
                adjCnt = cnt;

            int to_ = to;
            int from_ = from;
            for (int i = 0; i < adjCnt; i++)
                array[to_++] = array[from_++];
        }
        else
        // backward copy
        {
            // adjust copy size
            if ((to + cnt) >= length)
                adjCnt = length - to;
            else
                adjCnt = cnt;

            int to_ = to + cnt;
            int from_ = from + cnt;
            for (int i = 0; i < adjCnt; i++)
                array[--to_] = array[--from_];
        }
    }

    /**
     * Return the 'in' array as a single dimension array.<br>
     * The resulting array is returned in 'out' at specified offset.<br>
     * If (out == null) a new array is allocated.
     */
    public static byte[] toByteArray(byte[] in, byte[] out, int offset)
    {
        final int len = getTotalLength(in);
        final byte[] result = allocIfNull(out, offset + len);

        if (in != null)
            System.arraycopy(in, 0, result, offset, len);

        return result;
    }

    /**
     * Return the 'in' array as a single dimension array.<br>
     * The resulting array is returned in 'out' at specified offset.<br>
     * If (out == null) a new array is allocated.
     */
    public static short[] toShortArray(short[] in, short[] out, int offset)
    {
        final int len = getTotalLength(in);
        final short[] result = allocIfNull(out, offset + len);

        if (in != null)
            System.arraycopy(in, 0, result, offset, len);

        return result;
    }

    /**
     * Return the 'in' array as a single dimension array.<br>
     * The resulting array is returned in 'out' at specified offset.<br>
     * If (out == null) a new array is allocated.
     */
    public static int[] toIntArray(int[] in, int[] out, int offset)
    {
        final int len = getTotalLength(in);
        final int[] result = allocIfNull(out, offset + len);

        if (in != null)
            System.arraycopy(in, 0, result, offset, len);

        return result;
    }

    /**
     * Return the 'in' array as a single dimension array.<br>
     * The resulting array is returned in 'out' at specified offset.<br>
     * If (out == null) a new array is allocated.
     */
    public static long[] toLongArray(long[] in, long[] out, int offset)
    {
        final int len = getTotalLength(in);
        final long[] result = allocIfNull(out, offset + len);

        if (in != null)
            System.arraycopy(in, 0, result, offset, len);

        return result;
    }

    /**
     * Return the 'in' array as a single dimension array.<br>
     * The resulting array is returned in 'out' at specified offset.<br>
     * If (out == null) a new array is allocated.
     */
    public static float[] toFloatArray(float[] in, float[] out, int offset)
    {
        final int len = getTotalLength(in);
        final float[] result = allocIfNull(out, offset + len);

        if (in != null)
            System.arraycopy(in, 0, result, offset, len);

        return result;
    }

    /**
     * Return the 'in' array as a single dimension array.<br>
     * The resulting array is returned in 'out' at specified offset.<br>
     * If (out == null) a new array is allocated.
     */
    public static double[] toDoubleArray(double[] in, double[] out, int offset)
    {
        final int len = getTotalLength(in);
        final double[] result = allocIfNull(out, offset + len);

        if (in != null)
            System.arraycopy(in, 0, result, offset, len);

        return result;
    }

    //
    //
    //
    //

    /**
     * Convert a boolean array to a byte array (unpacked form : 1 boolean --> 1 byte)
     */
    public static byte[] toByteArray(boolean[] array)
    {
        return toByteArray(array, null, 0);
    }

    /**
     * Convert a boolean array to a byte array (unpacked form : 1 boolean --> 1 byte)
     * The resulting array is returned in 'out' and from the specified if any.<br>
     * If (out == null) a new array is allocated.
     */
    public static byte[] toByteArray(boolean[] in, byte[] out, int offset)
    {
        if (in == null)
            return new byte[0];

        final int len = in.length;
        final byte[] result = allocIfNull(out, offset + len);

        for (int i = 0; i < len; i++)
            result[i] = (byte) ((in[i]) ? 1 : 0);

        return result;
    }

    /**
     * Convert a byte array (unpacked form : 1 byte --> 1 boolean) to a boolean array
     */
    public static boolean[] toBooleanArray(byte[] array)
    {
        return toBooleanArray(array, null, 0);
    }

    /**
     * Convert a boolean array to a byte array (unpacked form : 1 boolean --> 1 byte)
     * The resulting array is returned in 'out' and from the specified if any.<br>
     * If (out == null) a new array is allocated.
     */
    public static boolean[] toBooleanArray(byte[] in, boolean[] out, int offset)
    {
        if (in == null)
            return new boolean[0];

        final int len = in.length;
        final boolean[] result = allocIfNull(out, offset + len);

        for (int i = 0; i < len; i++)
            result[i] = (in[i] != 0) ? true : false;

        return result;
    }

    /**
     * Retrieve interleaved byte data from 'in' array and return the result in the 'out' array.
     * 
     * @param in
     *        input byte array containing interleaved data
     * @param inOffset
     *        input array offset
     * @param step
     *        interleave step
     * @param out
     *        output result array. If set to <code>null</code> then a new array is allocated.
     * @param outOffset
     *        output array offset
     * @param size
     *        number of byte to retrieve
     * @return byte array containing de-interleaved data.
     */
    public static byte[] getInterleavedData(byte[] in, int inOffset, int step, byte[] out, int outOffset, int size)
    {
        final byte[] result = allocIfNull(out, outOffset + size);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < size; i++)
        {
            result[outOff] = in[inOff];
            inOff += step;
            outOff++;
        }

        return result;
    }

    /**
     * De interleave data from 'in' array and return the result in the 'out' array.
     * 
     * @param in
     *        input byte array containing interleaved data
     * @param inOffset
     *        input array offset
     * @param step
     *        interleave step
     * @param out
     *        output result array. If set to <code>null</code> then a new array is allocated
     * @param outOffset
     *        output array offset
     * @param size
     *        number of element to de-interleave
     * @return byte array containing de-interleaved data.
     */
    public static byte[] deInterleave(byte[] in, int inOffset, int step, byte[] out, int outOffset, int size)
    {
        final byte[] result = allocIfNull(out, outOffset + (size * step));

        int inOff1 = inOffset;
        int outOff1 = outOffset;

        for (int j = 0; j < step; j++)
        {
            int inOff2 = inOff1;
            int outOff2 = outOff1;

            for (int i = 0; i < size; i++)
            {
                result[outOff2] = in[inOff2];
                inOff2 += step;
                outOff2++;
            }

            inOff1++;
            outOff1 += size;
        }

        return result;
    }

    /**
     * Get maximum length in bytes for a copy from in to out with specified offset.<br>
     * If specified length != -1 then the value is directly returned (assumed to be in bytes).
     */
    static int getCopyLengthInBytes(Object in, int inOffset, Object out, int outOffset, int length)
    {
        if (length == -1)
            return getCopyLengthInBytes(in, inOffset, out, outOffset);

        return length;
    }

    /**
     * Get maximum length in bytes for a copy from in to out with specified offset.
     */
    public static int getCopyLengthInBytes(Object in, int inOffset, Object out, int outOffset)
    {
        // 'in' object can't be null !
        final int len = getCopyLengthInBytes(in, inOffset);

        if (out == null)
            return len;

        return Math.min(len, getCopyLengthInBytes(out, outOffset));
    }

    /**
     * Get length in bytes for a copy from or to array with specified offset.
     */
    public static int getCopyLengthInBytes(Object array, int offset)
    {
        return ArrayUtil.getCopyLength(array, offset) * ArrayUtil.getComponentSize(array);
    }

    /**
     * Read a byte from the input byte array at specified position.
     */
    public static byte readByte(byte[] array, int offset)
    {
        return array[offset];
    }

    /**
     * Read a short value from the input byte array at specified position.
     */
    public static short readShort(byte[] array, int offset, boolean littleEndian)
    {
        if (littleEndian)
            return (short) (((array[offset + 0] & 0xFF) << 0) + ((array[offset + 1] & 0xFF) << 8));

        return (short) (((array[offset + 0] & 0xFF) << 8) + ((array[offset + 1] & 0xFF) << 0));
    }

    /**
     * Read a int value from the input byte array at specified position.
     */
    public static int readInt(byte[] array, int offset, boolean littleEndian)
    {
        if (littleEndian)
            return ((array[offset + 0] & 0xFF) << 0) + ((array[offset + 1] & 0xFF) << 8)
                    + ((array[offset + 2] & 0xFF) << 16) + ((array[offset + 3] & 0xFF) << 24);

        return ((array[offset + 0] & 0xFF) << 24) + ((array[offset + 1] & 0xFF) << 16)
                + ((array[offset + 2] & 0xFF) << 8) + ((array[offset + 3] & 0xFF) << 0);
    }

    /**
     * Read a long value from the input byte array at specified position.
     */
    public static long readLong(byte[] array, int offset, boolean littleEndian)
    {
        if (littleEndian)
        {
            final int v1 = ((array[offset + 0] & 0xFF) << 0) + ((array[offset + 1] & 0xFF) << 8)
                    + ((array[offset + 2] & 0xFF) << 16) + ((array[offset + 3] & 0xFF) << 24);
            final int v2 = ((array[offset + 4] & 0xFF) << 0) + ((array[offset + 5] & 0xFF) << 8)
                    + ((array[offset + 6] & 0xFF) << 16) + ((array[offset + 7] & 0xFF) << 24);
            return ((v1 & 0xFFFFFFFFL) << 0) + ((v2 & 0xFFFFFFFFL) << 32);
        }

        final int v1 = ((array[offset + 0] & 0xFF) << 24) + ((array[offset + 1] & 0xFF) << 16)
                + ((array[offset + 2] & 0xFF) << 8) + ((array[offset + 3] & 0xFF) << 0);
        final int v2 = ((array[offset + 4] & 0xFF) << 24) + ((array[offset + 5] & 0xFF) << 16)
                + ((array[offset + 6] & 0xFF) << 8) + ((array[offset + 7] & 0xFF) << 0);
        return ((v1 & 0xFFFFFFFFL) << 32) + ((v2 & 0xFFFFFFFFL) << 0);
    }

    /**
     * Read a long value from the input byte array at specified position.
     */
    public static float readFloat(byte[] array, int offset, boolean littleEndian)
    {
        return Float.intBitsToFloat(readInt(array, offset, littleEndian));
    }

    /**
     * Read a long value from the input byte array at specified position.
     */
    public static double readDouble(byte[] array, int offset, boolean littleEndian)
    {
        return Double.longBitsToDouble(readLong(array, offset, littleEndian));
    }

    /**
     * Write a byte to the output byte array at specified position.
     */
    public static void writeByte(byte[] array, int offset, byte value)
    {
        array[offset] = value;
    }

    /**
     * Write a short to the output byte array at specified position.
     */
    public static void writeShort(byte[] array, int offset, short value, boolean littleEndian)
    {
        if (littleEndian)
        {
            array[offset + 0] = (byte) (value >> 0);
            array[offset + 1] = (byte) (value >> 8);
        }
        else
        {
            array[offset + 0] = (byte) (value >> 8);
            array[offset + 1] = (byte) (value >> 0);
        }
    }

    /**
     * Write a int to the output byte array at specified position.
     */
    public static void writeInt(byte[] array, int offset, int value, boolean littleEndian)
    {
        if (littleEndian)
        {
            array[offset + 0] = (byte) (value >> 0);
            array[offset + 1] = (byte) (value >> 8);
            array[offset + 2] = (byte) (value >> 16);
            array[offset + 3] = (byte) (value >> 24);
        }
        else
        {
            array[offset + 0] = (byte) (value >> 24);
            array[offset + 1] = (byte) (value >> 16);
            array[offset + 2] = (byte) (value >> 8);
            array[offset + 3] = (byte) (value >> 0);
        }
    }

    /**
     * Write a long to the output byte array at specified position.
     */
    public static void writeLong(byte[] array, int offset, long value, boolean littleEndian)
    {
        int v;

        if (littleEndian)
        {
            v = (int) (value >> 0);
            array[offset + 0] = (byte) (v >> 0);
            array[offset + 1] = (byte) (v >> 8);
            array[offset + 2] = (byte) (v >> 16);
            array[offset + 3] = (byte) (v >> 24);
            v = (int) (value >> 32);
            array[offset + 4] = (byte) (v >> 0);
            array[offset + 5] = (byte) (v >> 8);
            array[offset + 6] = (byte) (v >> 16);
            array[offset + 7] = (byte) (v >> 24);
        }
        else
        {
            v = (int) (value >> 32);
            array[offset + 0] = (byte) (v >> 24);
            array[offset + 1] = (byte) (v >> 16);
            array[offset + 2] = (byte) (v >> 8);
            array[offset + 3] = (byte) (v >> 0);
            v = (int) (value >> 0);
            array[offset + 4] = (byte) (v >> 24);
            array[offset + 5] = (byte) (v >> 16);
            array[offset + 6] = (byte) (v >> 8);
            array[offset + 7] = (byte) (v >> 0);
        }
    }

    /**
     * Write a float to the output byte array at specified position.
     */
    public static void writeFloat(byte[] array, int offset, float value, boolean littleEndian)
    {
        writeInt(array, offset, Float.floatToRawIntBits(value), littleEndian);
    }

    /**
     * Write a double to the output byte array at specified position.
     */
    public static void writeDouble(byte[] array, int offset, double value, boolean littleEndian)
    {
        writeLong(array, offset, Double.doubleToRawLongBits(value), littleEndian);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of bytes to compute (-1 means we will use the maximum possible)
     */
    public static byte[] byteToByte(byte[] in, int inOffset, byte[] out, int outOffset, int length)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length);
        final byte[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        // simple copy
        System.arraycopy(in, inOffset, result, outOffset, len);

        return result;
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     */
    public static byte[] byteToByte(byte[] in, byte[] out)
    {
        return byteToByte(in, 0, out, 0, -1);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     */
    public static byte[] byteToByte(byte[] in)
    {
        return byteToByte(in, 0, null, 0, -1);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of bytes to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static short[] byteToShort(byte[] in, int inOffset, short[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length) / 2;
        final short[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len; i++)
        {
            result[outOff++] = readShort(in, inOff, little);
            inOff += 2;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param little
     *        little endian order
     */
    public static short[] byteToShort(byte[] in, short[] out, boolean little)
    {
        return byteToShort(in, 0, out, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static short[] byteToShort(byte[] in, boolean little)
    {
        return byteToShort(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     */
    public static short[] byteToShort(byte[] in)
    {
        return byteToShort(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of bytes to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static int[] byteToInt(byte[] in, int inOffset, int[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length) / 4;
        final int[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len; i++)
        {
            result[outOff++] = readInt(in, inOff, little);
            inOff += 4;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param little
     *        little endian order
     */
    public static int[] byteToInt(byte[] in, int[] out, boolean little)
    {
        return byteToInt(in, 0, out, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static int[] byteToInt(byte[] in, boolean little)
    {
        return byteToInt(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     */
    public static int[] byteToInt(byte[] in)
    {
        return byteToInt(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of bytes to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static long[] byteToLong(byte[] in, int inOffset, long[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length) / 8;
        final long[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len; i++)
        {
            result[outOff++] = readLong(in, inOff, little);
            inOff += 8;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param little
     *        little endian order
     */
    public static long[] byteToLong(byte[] in, long[] out, boolean little)
    {
        return byteToLong(in, 0, out, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static long[] byteToLong(byte[] in, boolean little)
    {
        return byteToLong(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     */
    public static long[] byteToLong(byte[] in)
    {
        return byteToLong(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' byte array in 'out' float array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of bytes to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static float[] byteToFloat(byte[] in, int inOffset, float[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length) / 4;
        final float[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len; i++)
        {
            result[outOff++] = readFloat(in, inOff, little);
            inOff += 4;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param little
     *        little endian order
     */
    public static float[] byteToFloat(byte[] in, float[] out, boolean little)
    {
        return byteToFloat(in, 0, out, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static float[] byteToFloat(byte[] in, boolean little)
    {
        return byteToFloat(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     */
    public static float[] byteToFloat(byte[] in)
    {
        return byteToFloat(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of bytes to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static double[] byteToDouble(byte[] in, int inOffset, double[] out, int outOffset, int length,
            boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length) / 8;
        final double[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len; i++)
        {
            result[outOff++] = readDouble(in, inOff, little);
            inOff += 8;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param out
     *        output array which is used to receive result (and so define wanted type)
     * @param little
     *        little endian order
     */
    public static double[] byteToDouble(byte[] in, double[] out, boolean little)
    {
        return byteToDouble(in, 0, out, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static double[] byteToDouble(byte[] in, boolean little)
    {
        return byteToDouble(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' byte array in the specified data type array
     * 
     * @param in
     *        input array
     */
    public static double[] byteToDouble(byte[] in)
    {
        return byteToDouble(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' short array as byte array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output byte array which is used to receive result
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of <b>bytes</b> to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static byte[] shortToByte(short[] in, int inOffset, byte[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length);
        final byte[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len / 2; i++)
        {
            writeShort(result, outOff, in[inOff++], little);
            outOff += 2;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' short array as byte array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static byte[] shortToByte(short[] in, boolean little)
    {
        return shortToByte(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' short array as byte array
     * 
     * @param in
     *        input array
     */
    public static byte[] shortToByte(short[] in)
    {
        return shortToByte(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' int array as byte array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output byte array which is used to receive result
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of <b>bytes</b> to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static byte[] intToByte(int[] in, int inOffset, byte[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length);
        final byte[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len / 4; i++)
        {
            writeInt(result, outOff, in[inOff++], little);
            outOff += 4;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' int array as byte array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static byte[] intToByte(int[] in, boolean little)
    {
        return intToByte(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' int array as byte array
     * 
     * @param in
     *        input array
     */
    public static byte[] intToByte(int[] in)
    {
        return intToByte(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' long array as byte array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output byte array which is used to receive result
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of <b>bytes</b> to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static byte[] longToByte(long[] in, int inOffset, byte[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length);
        final byte[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len / 8; i++)
        {
            writeLong(result, outOff, in[inOff++], little);
            outOff += 8;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' long array as byte array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static byte[] longToByte(long[] in, boolean little)
    {
        return longToByte(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' long array as byte array
     * 
     * @param in
     *        input array
     */
    public static byte[] longToByte(long[] in)
    {
        return longToByte(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' float array as byte array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output byte array which is used to receive result
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of <b>bytes</b> to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static byte[] floatToByte(float[] in, int inOffset, byte[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length);
        final byte[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len / 4; i++)
        {
            writeFloat(result, outOff, in[inOff++], little);
            outOff += 4;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' float array as byte array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static byte[] floatToByte(float[] in, boolean little)
    {
        return floatToByte(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' float array as byte array
     * 
     * @param in
     *        input array
     */
    public static byte[] floatToByte(float[] in)
    {
        return floatToByte(in, 0, null, 0, -1, false);
    }

    /**
     * Bit transform and return the 'in' double array as byte array
     * 
     * @param in
     *        input array
     * @param inOffset
     *        position where we start read data from
     * @param out
     *        output byte array which is used to receive result
     * @param outOffset
     *        position where we start to write data to
     * @param length
     *        number of <b>bytes</b> to compute (-1 means we will use the maximum possible)
     * @param little
     *        little endian order
     */
    public static byte[] doubleToByte(double[] in, int inOffset, byte[] out, int outOffset, int length, boolean little)
    {
        final int len = getCopyLengthInBytes(in, inOffset, out, outOffset, length);
        final byte[] result = ArrayUtil.allocIfNull(out, outOffset + len);

        int inOff = inOffset;
        int outOff = outOffset;

        for (int i = 0; i < len / 8; i++)
        {
            writeDouble(result, outOff, in[inOff++], little);
            outOff += 8;
        }

        return result;
    }

    /**
     * Bit transform and return the 'in' double array as byte array
     * 
     * @param in
     *        input array
     * @param little
     *        little endian order
     */
    public static byte[] doubleToByte(double[] in, boolean little)
    {
        return doubleToByte(in, 0, null, 0, -1, little);
    }

    /**
     * Bit transform and return the 'in' double array as byte array
     * 
     * @param in
     *        input array
     */
    public static byte[] doubleToByte(double[] in)
    {
        return doubleToByte(in, 0, null, 0, -1, false);
    }
}
