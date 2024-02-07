package sgdk.xgm2tool.tool;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.Date;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;

import sgdk.tool.SoundUtil;

public class Util
{
    public static int align(int value, int align)
    {
        return ((value + (align - 1)) / align) * align;
    }

    public static void align(ByteArrayOutputStream out, int align, int fill)
    {
        final int size = out.size();
        // compute padding size
        final int len = Util.align(size, align) - size;
        // pad data
        for (int i = 0; i < len; i++)
            out.write(fill);
    }

    public static byte[] align(byte[] data, int align, int fill) throws IOException
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream(data.length);

        result.write(data);

        align(result, align, fill);

        return result.toByteArray();
    }

    public static int getASCIIStringSize(byte[] in, int offset)
    {
        int off = offset;
        int result = 0;

        while (Util.getInt8(in, off) != 0)
        {
            result++;
            off++;
        }

        return result;
    }

    public static int getWideStringSize(byte[] in, int offset)
    {
        int off = offset;
        int result = 0;

        while (Util.getInt16(in, off) != 0)
        {
            result++;
            off += 2;
        }

        return result;
    }

    public static String getWideString(byte[] in, int offset)
    {
        int length = getWideStringSize(in, offset);
        return getWideString(in, offset, length);
    }

    public static String getASCIIString(byte[] in, int offset)
    {
        int length = getASCIIStringSize(in, offset);
        return getASCIIString(in, offset, length);
    }

    public static String getWideString(byte[] in, int offset, int length)
    {
        return new String(in, offset, length * 2, StandardCharsets.UTF_16LE);
    }

    public static String getASCIIString(byte[] in, int offset, int length)
    {
        return new String(in, offset, length, StandardCharsets.US_ASCII);
    }

    public static String getUTF8String(byte[] in, int offset, int length)
    {
        return new String(in, offset, length, StandardCharsets.UTF_8);
    }

    public static int getStringSize(String text)
    {
        return text.length();
    }

    public static byte[] getBytes(String text, boolean utf16)
    {
        return text.getBytes(utf16 ? StandardCharsets.UTF_16LE : StandardCharsets.US_ASCII);
    }

    public static byte[] getBytes(String text)
    {
        return getBytes(text, false);
    }

    public static boolean arrayEquals(byte[] array1, byte[] array2, int size)
    {
        for (int i = 0; i < size; i++)
        {
            if (array1[i] != array2[i])
            {
                return false;
            }
        }
        return true;
    }

    public static int swapNibbles(byte value)
    {
        return value >> 4 & 0xF | value << 4 & 0xF0;
    }

    public static String getBytesAsHexaString(byte[] data)
    {
        if (data == null)
            return "null";

        return getBytesAsHexaString(data, 0, data.length, 64);
    }

    public static String getBytesAsHexaString(byte[] data, int offset, int len, int maxlen)
    {
        if (data == null)
        {
            return "null";
        }
        String result = "";
        for (int i = 0; i < Math.min(maxlen, len); i++)
        {
            byte b = data[(offset + i)];
            int v = b & 0xFF;
            if (v < 16)
            {
                result = result + "0";
            }
            result = result + Integer.toHexString(v).toUpperCase();
        }
        if (len > maxlen)
        {
            result = result + "..";
        }
        return result;
    }

    public static String getHexaString(int value, int minSize)
    {
        String result = "";
        for (int i = 7; i >= 0; i--)
        {
            int v = value >> i * 4 & 0xF;
            if ((v != 0) || (i < minSize))
            {
                result = result + Integer.toHexString(v).toUpperCase();
            }
        }
        return result;
    }

    public static int getInt8(byte[] data, int offset)
    {
        long res = (data[(offset + 0)] & 0xFF) << 0;

        return (int) res;
    }

    public static int getInt16(byte[] data, int offset)
    {
        long res = (data[(offset + 0)] & 0xFF) << 0;
        res += ((data[(offset + 1)] & 0xFF) << 8);

        return (int) res;
    }

    public static int getInt24(byte[] data, int offset)
    {
        long res = (data[(offset + 0)] & 0xFF) << 0;
        res += ((data[(offset + 1)] & 0xFF) << 8);
        res += ((data[(offset + 2)] & 0xFF) << 16);

        return (int) res;
    }

    public static int getInt32(byte[] data, int offset)
    {
        long res = (data[(offset + 0)] & 0xFF) << 0;
        res += ((data[(offset + 1)] & 0xFF) << 8);
        res += ((data[(offset + 2)] & 0xFF) << 16);
        res += ((data[(offset + 3)] & 0xFF) << 24);

        return (int) res;
    }

    public static void setInt32(byte[] array, int offset, int value)
    {
        array[(offset + 0)] = ((byte) ((value >> 0) & 0xFF));
        array[(offset + 1)] = ((byte) ((value >> 8) & 0xFF));
        array[(offset + 2)] = ((byte) ((value >> 16) & 0xFF));
        array[(offset + 3)] = ((byte) ((value >> 24) & 0xFF));
    }

    public static void setInt24(byte[] array, int offset, int value)
    {
        array[(offset + 0)] = ((byte) ((value >> 0) & 0xFF));
        array[(offset + 1)] = ((byte) ((value >> 8) & 0xFF));
        array[(offset + 2)] = ((byte) ((value >> 16) & 0xFF));
    }

    public static void setInt16(byte[] array, int offset, int value)
    {
        array[(offset + 0)] = ((byte) ((value >> 0) & 0xFF));
        array[(offset + 1)] = ((byte) ((value >> 8) & 0xFF));
    }

    public static void setInt8(byte[] array, int offset, int value)
    {
        array[(offset + 0)] = ((byte) ((value >> 0) & 0xFF));
    }

    public static byte[] readBinaryFile(String fileName) throws IOException
    {
        Path path = Paths.get(fileName, new String[0]);
        if (!path.toFile().exists())
            return null;

        return Files.readAllBytes(path);
    }

    public static Path writeBinaryFile(byte[] data, String fileName) throws IOException
    {
        Path path = Paths.get(fileName, new String[0]);
        return Files.write(path, data, new OpenOption[0]);
    }

    public static AudioInputStream resample(AudioInputStream audioIn, int bits, int rate, boolean mono, boolean signed,
            boolean bigEndian)
    {
        final AudioFormat audioInputFormat = audioIn.getFormat();
        final int channel = mono ? 1 : audioInputFormat.getChannels();
        final int adjRate = ((rate <= 0) ? (int) audioInputFormat.getSampleRate() : rate);

        // define target format
        final AudioFormat dstFormat = new AudioFormat(signed ? Encoding.PCM_SIGNED : Encoding.PCM_UNSIGNED, adjRate,
                bits, channel, ((bits + 7) / 8) * channel, adjRate, bigEndian);

        if (!AudioSystem.isConversionSupported(dstFormat, audioInputFormat))
            return null;

        return AudioSystem.getAudioInputStream(dstFormat, audioIn);
    }

    public static byte[] resample(byte[] data, int offset, int len, int inputRate, int outputRate, int align)
            throws IOException
    {
        final AudioFormat audioFormat = new AudioFormat(inputRate, 8, 1, false, false);
        final AudioInputStream audioIS = new AudioInputStream(new ByteArrayInputStream(data, offset, len), audioFormat,
                len);
        final AudioInputStream resultIS = SoundUtil.resample(audioIS, 8, outputRate, true, false, false);

        final byte[] buf = new byte[8 * 1024];
        final ByteArrayOutputStream resultOS = new ByteArrayOutputStream(8 * 1024);
        int count;

        try
        {
            while ((count = resultIS.read(buf)) != -1)
                resultOS.write(buf, 0, count);
        }
        finally
        {
            resultIS.close();
            audioIS.close();
        }

        if (align > 1)
        {
            final int mask = align - 1;
            final int size = align - (resultOS.size() & mask);

            if (size != align)
            {
                for (int i = 0; i < size; i++)
                    resultOS.write(0x80);
            }
        }

        final byte[] byteResult = resultOS.toByteArray();
        return byteResult;
    }

    public static byte[] resample_old(byte[] data, int offset, int len, int inputRate, int outputRate, int align)
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream();
        final double step = (double) inputRate / (double) outputRate;

        double sample = 0d;
        double value = 0d;
        double lastSample = 0d;
        int off = 0;
        for (double dOff = 0d; dOff <= (len - step); dOff += step)
        {
            sample = 0d;
            if (step >= 1d)
            {
                if (value < 0d)
                    sample += lastSample * -value;

                value += step;

                while (value > 0d)
                {
                    lastSample = (data[(off + offset)] & 0xFF);
                    lastSample = data[(off + offset)];
                    off++;

                    if (value >= 1d)
                        sample += lastSample;
                    else
                        sample += lastSample * value;

                    value -= 1d;
                }

                sample /= step;
            }
            else if (Math.floor(dOff) == dOff)
            {
                // sample = (data[((int) dOff + offset)] & 0xFF) - 128d;
                sample = data[((int) dOff + offset)];
            }
            else
            {
                // double sample0 = (data[((int) Math.floor(dOff) + offset)] & 0xFF) - 128d;
                // double sample1 = (data[((int) Math.ceil(dOff) + offset)] & 0xFF) - 128d;
                double sample0 = data[((int) Math.floor(dOff) + offset)];
                double sample1 = data[((int) Math.ceil(dOff) + offset)];

                sample += sample0 * (Math.ceil(dOff) - dOff);
                sample += sample1 * (dOff - Math.floor(dOff));
            }

            result.write((int) Math.round(sample));
        }

        return result.toByteArray();
    }

    static final SimpleDateFormat timeFormatter = new SimpleDateFormat("mm:ss.SSS");

    public static String formatVGMTime(long vgmTime)
    {
        return timeFormatter.format(new Date(vgmTime * 1000L / 44100L));
    }

    public static boolean isDiffRate(int rate1, int rate2)
    {
        // obviously
        if (rate1 == rate2)
            return false;

        final int r1 = Math.max(100, rate1);
        final int r2 = Math.max(100, rate2);
        double f = (double) r1 / (double) r2;

        return Math.abs(100 - (f * 100d)) > 10d;
    }
}
