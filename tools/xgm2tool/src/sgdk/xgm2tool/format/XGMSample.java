package sgdk.xgm2tool.format;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Arrays;

import com.musicg.fingerprint.FingerprintManager;
import com.musicg.fingerprint.FingerprintSimilarity;
import com.musicg.fingerprint.FingerprintSimilarityComputer;
import com.musicg.wave.Wave;
import com.musicg.wave.WaveHeader;

import sgdk.tool.MathUtil;
import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.format.SampleBank.InternalSample;
import sgdk.xgm2tool.tool.Util;

/**
 * XGM Sample have fixed sample rate of 13.3 or 6.65 Khz.<br>
 * Size and Offset are 64 bytes boundary.
 * 
 * @author Stephane
 */
public class XGMSample
{
    final static WaveHeader xgmWaveHeader = new WaveHeader();

    {
        xgmWaveHeader.setBitsPerSample(8);
        xgmWaveHeader.setSampleRate(13300);
        xgmWaveHeader.setChannels(1);
    }

    final static int XGM_FULL_RATE = 13300;
    final static int XGM_HALF_RATE = (XGM_FULL_RATE / 2);

    int id;
    final byte[] data;

    // used only for VGM to XGM conversion
    final boolean halfRate;
    final int originId;
    final int originAddr;

    public XGMSample(int id, byte[] data, boolean halfRate, int originId, int originAddr)
    {
        super();

        this.id = id;
        this.data = data;

        // only used for VGM to XGM conversion
        this.halfRate = halfRate;
        this.originId = originId;
        this.originAddr = originAddr;
    }

    public XGMSample(int id, byte[] data)
    {
        this(id, data, false, -1, 0);
    }

    public XGMSample(int id, byte[] data, int offset, int len)
    {
        this(id, Arrays.copyOfRange(data, offset, offset + len));
    }

    public static XGMSample createFromVGMSample(int id, InternalSample sample) throws IOException
    {
        final boolean half = (sample.rate < 10000);

        return new XGMSample(id, Util.resample(sample.getBank().data, sample.addr, sample.len, sample.rate, half ? XGM_HALF_RATE : XGM_FULL_RATE, 256), half,
                sample.id, sample.addr);
    }

    public int getLength()
    {
        return data.length;
    }

    public double getSimilarityScore(XGMSample originSample)
    {
        final int deltaSize = originSample.data.length - data.length;
        // origin sample is longer ? cannot use a shorter sample to replace it...
        if (deltaSize > 150)
            return 0d;

        final int minSize = Math.min(data.length, originSample.data.length);

        // advanced sample compare using fingerprint
        if (Launcher.sampleAdvancedCompare)
        {
            int minSizePadded = (minSize < 2048) ? (int) MathUtil.nextPow2(minSize) : minSize;

            final FingerprintManager fingerprintManager = new FingerprintManager();

            try
            {
                Field sampleSizePerFrameF = FingerprintManager.class.getDeclaredField("sampleSizePerFrame");
                sampleSizePerFrameF.setAccessible(true);

                if (minSize < 512)
                    sampleSizePerFrameF.set(fingerprintManager, Integer.valueOf(256));
                else if (minSize < 1024)
                    sampleSizePerFrameF.set(fingerprintManager, Integer.valueOf(512));
                else if (minSize < 2048)
                    sampleSizePerFrameF.set(fingerprintManager, Integer.valueOf(1024));
                else if (minSize < 4096)
                    sampleSizePerFrameF.set(fingerprintManager, Integer.valueOf(2048));
                else
                    // default
                    sampleSizePerFrameF.set(fingerprintManager, Integer.valueOf(2048));
            }
            catch (Exception e)
            {
                // pad to 2048
                minSizePadded = Math.max(minSize, 2048);
            }

            final byte[] sample1 = new byte[minSizePadded];
            Arrays.fill(sample1, (byte) 128);
            for (int i = 0; i < minSize; i++)
                sample1[i] = (byte) (data[i] + 128);

            final byte[] sample2 = new byte[minSizePadded];
            Arrays.fill(sample2, (byte) 128);
            for (int i = 0; i < minSize; i++)
                sample2[i] = (byte) (originSample.data[i] + 128);

            final Wave wave1 = new Wave(xgmWaveHeader, sample1);
            final Wave wave2 = new Wave(xgmWaveHeader, sample2);
            final byte[] fp1 = fingerprintManager.extractFingerprint(wave1);
            final byte[] fp2 = fingerprintManager.extractFingerprint(wave2);
            final FingerprintSimilarity similarity = new FingerprintSimilarityComputer(fp2, fp1).getFingerprintsSimilarity();

            if (Launcher.verbose)
                System.out.println("similarity between #" + id + " (" + originAddr + ") and #" + originSample.id + " (" + originSample.originAddr + ") = "
                    + similarity.getSimilarity() + " - score = " + similarity.getScore());

            return similarity.getSimilarity() * similarity.getScore();
        }

        // simple sample compare (exact match)
        return Arrays.equals(Arrays.copyOf(data, minSize), Arrays.copyOf(originSample.data, minSize)) ? 1d : 0d;
    }
}
