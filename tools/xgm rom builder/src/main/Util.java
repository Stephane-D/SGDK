package main;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.zip.GZIPInputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class Util
{
    private static final int BUFFER_SIZE = 4096;

    public static String convertFrameToTime(long frames)
    {
        final int f = (int) (frames % 60);
        final int s = (int) ((frames / 60) % 60);
        final int m = (int) (frames / (60 * 60));
        return String.format("%d:%02d", Integer.valueOf(m), Integer.valueOf(s));
    }

    /**
     * Execute a system command and return the attached process.
     * 
     * @param cmd
     *        system command to execute.
     * @param dir
     *        the working directory of the subprocess, or null if the subprocess should inherit the
     *        working directory of the current process.
     */
    public static Process exec(String cmd, String dir)
    {
        try
        {
            return Runtime.getRuntime().exec(cmd, null, new File(dir));
        }
        catch (Exception e)
        {
            System.err.println("exec(" + cmd + ") error : " + e.getMessage());
            return null;
        }
    }

    /**
     * Transform any system specific path in java generic path form.<br>
     * Ex: "C:\windows" --> "C:/windows"
     */
    public static String getGenericPath(String path)
    {
        if (path != null)
            return path.replace('\\', '/');

        return null;
    }

    public static String getFileName(String path, boolean withExtension)
    {
        final String finalPath = getGenericPath(path);

        int index = finalPath.lastIndexOf('/');
        final String fileName;

        if (index != -1)
            fileName = finalPath.substring(index + 1);
        else
        {
            index = finalPath.lastIndexOf(':');

            if (index != -1)
                fileName = finalPath.substring(index + 1);
            else
                fileName = finalPath;
        }

        if (withExtension)
            return fileName;

        index = fileName.lastIndexOf('.');

        if (index == 0)
            return "";
        else if (index != -1)
            return fileName.substring(0, index);
        else
            return fileName;
    }

    public static String setFileExtension(String path, String extension)
    {
        final int len = path.length();
        String result = path;

        final int dotIndex = result.lastIndexOf(".");
        // ensure we are modifying an extension
        if (dotIndex >= 0 && (len - dotIndex) <= 5)
        {
            // we consider that an extension starting with a digit is not an extension
            if (((dotIndex + 1) == len) || !Character.isDigit(result.charAt(dotIndex + 1)))
                result = result.substring(0, dotIndex);
        }

        if (extension != null)
            result += extension;

        return result;
    }

    public static boolean arrayEquals(byte[] array1, byte[] array2, int size)
    {
        for (int i = 0; i < size; i++)
            if (array1[i] != array2[i])
                return false;

        return true;
    }

    public static int swapNibbles(byte value)
    {
        return ((value >> 4) & 0xF) | ((value << 4) & 0xF0);
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
            return "null";

        String result = "";

        for (int i = 0; i < Math.min(maxlen, len); i++)
        {
            byte b = data[offset + i];
            int v = (b) & 0xFF;

            if (v < 16)
                result += "0";
            result += Integer.toHexString(v).toUpperCase();
        }

        if (len > maxlen)
            result += "..";

        return result;
    }

    public static String getHexaString(int value, int minSize)
    {
        String result = "";

        for (int i = 7; i >= 0; i--)
        {
            final int v = (value >> (i * 4)) & 0xF;

            if ((v != 0) || (i < minSize))
                result += Integer.toHexString(v).toUpperCase();
        }

        return result;
    }

    public static int getInt16Swapped(byte[] data, int offset)
    {
        int res;

        res = ((data[offset + 1]) & 0xFF) << 0;
        res += ((data[offset + 0]) & 0xFF) << 8;

        return res;
    }

    public static int getInt(byte[] data, int offset)
    {
        long res;

        res = ((data[offset + 0]) & 0xFF) << 0;
        res += ((data[offset + 1]) & 0xFF) << 8;
        res += ((data[offset + 2]) & 0xFF) << 16;
        res += ((data[offset + 3]) & 0xFF) << 24;

        return (int) res;
    }

    public static void setIntSwapped(byte[] array, int offset, int value)
    {
        array[offset + 3] = (byte) ((value >> 0) & 0xFF);
        array[offset + 2] = (byte) ((value >> 8) & 0xFF);
        array[offset + 1] = (byte) ((value >> 16) & 0xFF);
        array[offset + 0] = (byte) ((value >> 24) & 0xFF);
    }

    public static void setInt16Swapped(byte[] array, int offset, int value)
    {
        array[offset + 1] = (byte) ((value >> 0) & 0xFF);
        array[offset + 0] = (byte) ((value >> 8) & 0xFF);
    }

    public static void deleteFile(String fileName) throws IOException
    {
        Path path = Paths.get(fileName);
        Files.delete(path);
    }

    public static byte[] readBinaryFile(String fileName) throws IOException
    {
        Path path = Paths.get(fileName);
        return Files.readAllBytes(path);
    }

    public static void writeBinaryFile(byte[] data, String fileName) throws IOException
    {
        Path path = Paths.get(fileName);
        Files.write(path, data);
    }

    public static byte[] resample(byte[] data, int offset, int len, int inputRate, int outputRate, int align)
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream();
        final double step = (double) inputRate / (double) outputRate;

        double value;
        double lastSample;
        double sample = 0;
        int off;

        value = 0;
        lastSample = 0;
        off = 0;
        for (double dOff = 0d; dOff < len; dOff += step)
        {
            sample = 0;

            // extrapolation
            if (step >= 1d)
            {
                if (value < 0)
                    sample += lastSample * -value;

                value += step;

                while (value > 0)
                {
                    lastSample = (data[off + offset] & 0xFF) - 0x80;
                    off++;

                    if (value >= 1)
                        sample += lastSample;
                    else
                        sample += lastSample * value;

                    value--;
                }

                sample /= step;
            }
            else
            {
                if (Math.floor(dOff) == dOff)
                    sample = (data[(int) dOff + offset] & 0xFF) - 0x80;
                else
                {
                    double sample0 = (data[(int) Math.floor(dOff) + offset] & 0xFF) - 0x80;
                    double sample1 = (data[(int) Math.ceil(dOff) + offset] & 0xFF) - 0x80;

                    sample += sample0 * (Math.ceil(dOff) - dOff);
                    sample += sample1 * (dOff - Math.floor(dOff));
                }
            }

            result.write((int) Math.round(sample));
        }

        // do alignment
        if (align > 1)
        {
            final int mask = align - 1;
            final int size = align - (result.size() & mask);

            if (size != align)
            {
                double reduce = sample / size;

                for (int i = 0; i < size; i++)
                {
                    sample -= reduce;
                    result.write((int) Math.round(sample));
                }
            }
        }

        return result.toByteArray();
    }

    public static int getXGCMusicDataOffset(byte[] xgc)
    {
        int result;

        // get sample table size
        result = ((xgc[0xFC] & 0xFF) << 8) | ((xgc[0xFD] & 0xFF) << 16);

        return result + 0x104;
    }

    public static int getXD3Offset(byte[] xgc)
    {
        // XD3 tag ?
        if ((xgc[0xFF] & 2) != 0)
        {
            int result;

            // get music size field offset
            result = getXGCMusicDataOffset(xgc) - 4;
            // add music data size
            result += ((xgc[result + 0] & 0xFF) << 0) | ((xgc[result + 1] & 0xFF) << 8)
                    | ((xgc[result + 2] & 0xFF) << 16);
            // add both size field len (music and xd3)
            return result + 8;
        }

        return -1;
    }

    public static int getXGCDuration(byte[] xgc)
    {
        int offset = getXD3Offset(xgc);

        if (offset != -1)
        {
            // track name
            while (xgc[offset++] != 0)
                ;
            // game name
            while (xgc[offset++] != 0)
                ;
            // author name
            while (xgc[offset++] != 0)
                ;
            // date
            while (xgc[offset++] != 0)
                ;
            // conversion author
            while (xgc[offset++] != 0)
                ;
            // notes
            while (xgc[offset++] != 0)
                ;

            return ((xgc[offset + 0] & 0XFF) << 0) + ((xgc[offset + 1] & 0XFF) << 8) + ((xgc[offset + 2] & 0XFF) << 16)
                    + ((xgc[offset + 3] & 0XFF) << 24);
            // offset += 4;
            // loopDuration = (xgm[offset + 0] << 0) + (xgm[offset + 1] << 8) + (xgm[offset + 2] <<
            // 16) + (xgm[offset + 3] << 24);
        }

        return -1;
    }

    /**
     * Extracts a gzip file specified by the zipFilePath to a directory specified by
     * destDirectory (will be created if does not exists)
     */
    public static void gunzipFile(String gzipFile, String destFile) throws IOException
    {
        final FileInputStream fis = new FileInputStream(gzipFile);
        final GZIPInputStream gis = new GZIPInputStream(fis);
        final FileOutputStream fos = new FileOutputStream(destFile);
        try
        {
            final byte[] buffer = new byte[1024];

            int len;
            while ((len = gis.read(buffer)) != -1)
                fos.write(buffer, 0, len);
        }
        finally
        {
            // close resources
            fos.close();
            gis.close();
        }
    }

    /**
     * Extracts a zip file specified by the zipFilePath to a directory specified by
     * destDirectory (will be created if does not exists)
     */
    public static boolean unzipSingle(String zipFilePath, String destFile) throws IOException
    {
        final ZipInputStream zipIn = new ZipInputStream(new FileInputStream(zipFilePath));

        try
        {
            final ZipEntry entry = zipIn.getNextEntry();
            if (entry != null)
            {
                // extract the file
                extractFile(zipIn, destFile);
                zipIn.closeEntry();
                return true;
            }
        }
        finally
        {
            zipIn.close();
        }

        return false;
    }

    /**
     * Extracts a zip entry (file entry)
     */
    private static void extractFile(ZipInputStream zipIn, String out) throws IOException
    {
        final byte[] bytesIn = new byte[BUFFER_SIZE];
        final BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(out));

        try
        {
            int read = 0;
            while ((read = zipIn.read(bytesIn)) != -1)
                bos.write(bytesIn, 0, read);
        }
        finally
        {
            bos.close();
        }
    }
}
