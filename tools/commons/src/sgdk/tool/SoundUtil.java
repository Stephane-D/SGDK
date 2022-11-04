package sgdk.tool;

import java.io.File;
import java.io.IOException;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

public class SoundUtil
{
    public static byte[] getRawDataFromWAV(String wavPath, int bits, int rate, boolean mono, boolean signed, boolean bigEndian)
            throws UnsupportedAudioFileException, IOException
    {
        final File wavFile = new File(wavPath);
        final AudioInputStream wavInput = AudioSystem.getAudioInputStream(wavFile);
        // get resampled input stream
        final AudioInputStream resampledInput = resample(wavInput, bits, rate, mono, signed, bigEndian);

        // cannot convert ?
        if (resampledInput == null)
        {
            wavInput.close();
            throw new UnsupportedAudioFileException("Error: couldn't resample '" + wavPath + "' WAV file.");
        }

        try
        {
            return NetworkUtil.download(resampledInput);
        }
        finally
        {
            resampledInput.close();
            wavInput.close();
        }
    }

    public static AudioInputStream resample(AudioInputStream audioIn, int bits, int rate, boolean mono, boolean signed, boolean bigEndian)
    {
        final AudioFormat audioInputFormat = audioIn.getFormat();
        final int channel = mono ? 1 : audioInputFormat.getChannels();
        final int adjRate = ((rate <= 0) ? (int) audioInputFormat.getSampleRate() : rate);

        // define target format
        final AudioFormat dstFormat = new AudioFormat(signed ? Encoding.PCM_SIGNED : Encoding.PCM_UNSIGNED, adjRate, bits, channel, ((bits + 7) / 8) * channel,
                adjRate, bigEndian);

        if (!AudioSystem.isConversionSupported(dstFormat, audioInputFormat))
            return null;

        return AudioSystem.getAudioInputStream(dstFormat, audioIn);
    }
}
