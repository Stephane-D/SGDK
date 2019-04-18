package sgdk.tool;

/**
 * Random utilities class.
 * 
 * @author Stephane
 */
public class Random
{
    private static final java.util.Random generator = new java.util.Random();

    /**
     * @see java.util.Random#nextInt()
     */
    public static int nextInt()
    {
        return generator.nextInt();
    }

    /**
     * @see java.util.Random#nextInt(int)
     */
    public static int nextInt(int n)
    {
        return generator.nextInt(n);
    }

    /**
     * @see java.util.Random#nextBoolean()
     */
    public static boolean nextBoolean()
    {
        return generator.nextBoolean();
    }

    /**
     * @see java.util.Random#nextDouble()
     */
    public static double nextDouble()
    {
        return generator.nextDouble();
    }

    /**
     * @see java.util.Random#nextFloat()
     */
    public static float nextFloat()
    {
        return generator.nextFloat();
    }

    /**
     * @see java.util.Random#nextLong()
     */
    public static long nextLong()
    {
        return generator.nextLong();
    }
}
