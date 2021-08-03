package sgdk.sizebnd;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class Launcher
{
    /**
     * Launch the application.
     */
    public static void main(String[] args)
    {
        if (args.length < 3)
        {
            showUsage();
            System.exit(2);
        }

        String file = "";
        int align = 256;
        byte fill = 0;
        boolean checksum = false;

        for (String arg : args)
        {
            final String param = arg.toLowerCase();

            // next param should be align value
            if (align == -1)
                align = Integer.parseInt(param);
            // next param should be fill value
            else if (fill == -1)
                fill = (byte) Integer.parseInt(param);
            else if (param.equals("-sizealign"))
                align = -1;
            else if (param.equals("-fill"))
                fill = -1;
            else if (param.equals("-checksum"))
                checksum = true;
            else if (!param.startsWith("-"))
                file = param;
        }

        // invalid command
        if (file.isEmpty())
        {
            showUsage();
            System.exit(2);
        }

        // always set a minimum alignment of 4 bytes
        if (align < 4)
            align = 4;

        try
        {
            execute(file, align, fill, checksum);
            System.exit(0);
        }
        catch (Throwable t)
        {
            System.err.println(t.getMessage());
            t.printStackTrace();
            System.exit(1);
        }
    }

    private static void execute(String file, int align, byte fill, boolean checksum) throws IOException
    {
        // read file
        byte[] data = readBinaryFile(file);
        // pad it
        if (align != 0)
            data = pad(data, align, fill);
        // set checksum if needed
        if (checksum)
            setChecksum(data);
        // write back result
        writeBinaryFile(data, file);
    }

    private static void showUsage()
    {
        System.out.println("Sizebnd tool v1.1 by Stephane Dallongeville (Copyright 2021)");
        System.out.println();
        System.out.println("Usage: java -jar sizebnd.jar <file> -sizealign <size>");
        System.out.println();
        System.out.println("Options");
        System.out.println(" -fill <value>: set fill value");
        System.out.println(" -checksum: set ROM checksum (SGDK checksum format)");
    }

    private static byte[] readBinaryFile(String fileName) throws IOException
    {
        Path path = Paths.get(fileName);
        return Files.readAllBytes(path);
    }

    private static void writeBinaryFile(byte[] data, String fileName) throws IOException
    {
        Path path = Paths.get(fileName);
        Files.write(path, data);
    }

    /**
     * Align size to word
     */
    private static byte[] pad(byte[] buffer, int align, byte fill)
    {
        // calculate how many extra byte are needed
        int len = buffer.length;
        int needed = len & (align - 1);
        if (needed != 0) needed = align - needed;
        final byte[] result = new byte[len + needed];

        // copy to destination
        System.arraycopy(buffer, 0, result, 0, buffer.length);

        // pad destination
        while (needed-- > 0)
            result[len++] = fill;

        return result;
    }

    private static int getInt(byte[] data, int offset)
    {
        int off = offset;
        int result = 0;

        result += (data[off++] & 0xFF) << 24;
        result += (data[off++] & 0xFF) << 16;
        result += (data[off++] & 0xFF) << 8;
        result += (data[off++] & 0xFF) << 0;

        return result;
    }

    private static short getChecksum(byte[] data)
    {
        int checksum = 0;

        for (int i = 0; i < data.length; i += 4)
            checksum ^= getInt(data, i);

        return (short) (checksum ^ (checksum >> 16));
    }

    private static void setChecksum(byte[] data)
    {
        final int offset = 0x18E;

        // reset checksum
        data[offset + 0] = 0;
        data[offset + 1] = 0;

        // get checksum
        final short checksum = getChecksum(data);

        // save checksum
        data[offset + 0] = (byte) ((checksum >> 8) & 0xFF);
        data[offset + 1] = (byte) ((checksum >> 0) & 0xFF);
    }
}