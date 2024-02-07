package sgdk.xgm2tool.format;

/**
 * Single byte of VGM sample data (time is expressed in 1/44100 of second)
 * 
 * @author steph
 */
public class VGMSampleData implements Comparable<VGMSampleData>
{
    public long time;
    public byte data;

    public VGMSampleData(long time, byte data)
    {
        super();

        this.time = time;
        this.data = data;
    }

    @Override
    public int compareTo(VGMSampleData sample)
    {
        return Long.compare(time, sample.time);
    }
}
