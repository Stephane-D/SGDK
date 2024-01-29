package sgdk.xgm2tool.struct;

/**
 * Sample (signed 8 bits data)
 * 
 * @author Stephane Dallongeville
 */
public class Sample
{
    public final byte[] data;
    public final int sampleRate;

    public Sample(byte[] data, int sampleRate)
    {
        super();

        this.data = data;
        this.sampleRate = sampleRate;
    }

    public int getSample(int index)
    {
        return data[index];
    }

    public int getSize()
    {
        return data.length;
    }

    public boolean match(Sample sample, int startIndex)
    {
        if (startIndex >= data.length)
            return false;

        final byte[] sdata = sample.data;

        for (int i = startIndex, si = 0; (i < data.length) && (si < sdata.length); i++, si++)
            if (data[i] != sdata[si])
                return false;

        return true;
    }

    public boolean isValid()
    {
        // 0 value in sampleRate means sample in invalid
        return sampleRate != 0;
    }
}
