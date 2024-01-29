/**
 * 
 */
package sgdk.xgm2tool.format;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * VGM Sample (time is always expressed in 1/44100 of second)
 * 
 * @author Stephane Dallongeville
 */
public class VGMSample
{
    static class VGMGapCounter
    {
        public int count;

        public VGMGapCounter()
        {
            super();

            count = 1;
        }

        public void inc()
        {
            count++;
        }
    }

    public final List<VGMSampleData> sampleDataList;

    public VGMSample()
    {
        super();

        sampleDataList = new ArrayList<>();
    }

    public void addSampleData(int offset, byte data)
    {
        sampleDataList.add(new VGMSampleData(offset, data));
    }

    public void sort()
    {
        // sort samples on time
        Collections.sort(sampleDataList);
    }

    public int getMeanSampleRate()
    {
        double sumDelta = 0d;

        for (int i = 0; i < (sampleDataList.size() - 1); i++)
        {
            final VGMSampleData s0 = sampleDataList.get(i + 0);
            final VGMSampleData s1 = sampleDataList.get(i + 1);

            sumDelta += s1.time - s0.time;
        }

        if (sampleDataList.size() > 1)
            sumDelta /= (sampleDataList.size() - 1);

        return (int) Math.round(44100d / sumDelta);
    }

    public int getWantedSampleRate()
    {
        final Map<Long, VGMGapCounter> gapHisto = new HashMap<>();

        for (int i = 0; i < (sampleDataList.size() - 1); i++)
        {
            final VGMSampleData s0 = sampleDataList.get(i + 0);
            final VGMSampleData s1 = sampleDataList.get(i + 1);

            // key is time gap
            final Long key = Long.valueOf(s1.time - s0.time);
            // get number of occurrence for this gap
            final VGMGapCounter count = gapHisto.get(key);

            // create new count for this gap
            if (count == null)
                gapHisto.put(key, new VGMGapCounter());
            // just increment it
            else
                count.inc();
        }

        // find max count
        int maxCount = 0;
        for (VGMGapCounter gap : gapHisto.values())
        {
            final int cnt = gap.count;

            if (cnt > maxCount)
                maxCount = cnt;
        }

        // allowed gap minimum count
        final int minimumGapCount = maxCount / 3;

        double countedGap = 0d;
        double timeSum = 0d;
        for (Entry<Long, VGMGapCounter> entry : gapHisto.entrySet())
        {
            final int cnt = entry.getValue().count;

            // enough count ?
            if (cnt >= minimumGapCount)
            {
                // time sum
                timeSum += (double) entry.getKey().longValue() * (double) cnt;
                // counted gap
                countedGap += cnt;
            }
        }

        // return wanted sample rate (estimation)
        return (int) Math.round(timeSum / countedGap);
    }
}
