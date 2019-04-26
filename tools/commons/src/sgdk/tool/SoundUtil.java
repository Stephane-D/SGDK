package sgdk.tool;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import com.sun.media.sound.WaveFileReader;

public class SoundUtil
{
    public static byte[] getRawDataFromWAV(String wavPath, int bits, int rate, boolean mono, boolean signed,
            boolean bigEndian) throws UnsupportedAudioFileException, IOException
    {
        final File wavFile = new File(wavPath);
        final WaveFileReader waveReader = new WaveFileReader();
        final AudioInputStream wavInput = waveReader.getAudioInputStream(wavFile);

        // get resampled input stream
        final AudioInputStream resampledInput = resample(wavInput, bits, rate, mono, signed, bigEndian);
        // we will never need more than 128MB sample :p
        final byte[] result = new byte[128 * 1024 * 1024];

        int read;
        int off = 0;
        do
        {
            read = resampledInput.read(result, off, 256);
            if (read > 0)
                off += read;
        }
        while (read > 0);

        return Arrays.copyOf(result, off);
    }

    static AudioInputStream resample(AudioInputStream audioIn, int bits, int rate, boolean mono, boolean signed,
            boolean bigEndian)
    {
        final AudioFormat audioInputFormat = audioIn.getFormat();
        final int channel = mono ? 1 : audioInputFormat.getChannels();
        final int adjRate = ((rate <= 0) ? (int) audioInputFormat.getSampleRate() : rate);

        // define target format
        final AudioFormat dstFormat = new AudioFormat(signed ? Encoding.PCM_SIGNED : Encoding.PCM_UNSIGNED, adjRate, bits,
                channel, ((bits + 7) / 8) * channel, adjRate, bigEndian);

        if (!AudioSystem.isConversionSupported(dstFormat, audioInputFormat))
            return null;

        return AudioSystem.getAudioInputStream(dstFormat, audioIn);
    }
}
