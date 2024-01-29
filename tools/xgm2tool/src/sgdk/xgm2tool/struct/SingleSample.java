package sgdk.xgm2tool.struct;

/**
 * Single sample class (time is always expressed in 1/44100 of second)
 * 
 * @author Stephane Dallongeville
 */
public class SingleSample implements Comparable<SingleSample>
{
    public final static int BASE_RATE = 44100;

    final boolean newSample;
    final int sample;
    final long time;

    public SingleSample(int sample, long time, boolean newSample)
    {
        super();

        this.newSample = newSample;
        this.sample = sample;
        this.time = time;
    }

    public SingleSample(int sample, long time)
    {
        this(sample, time, false);
    }

    @Override
    public int compareTo(SingleSample ss)
    {
        return Long.compare(time, ss.time);
    }
}
