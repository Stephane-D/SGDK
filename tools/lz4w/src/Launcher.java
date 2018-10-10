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
        if (args.length < 2)
        {
            showUsage();
            System.exit(2);
        }

        final String cmd = args[0].toLowerCase();

        // invalid command
        if (!cmd.equals("p") && !cmd.equals("u"))
        {
            showUsage();
            System.exit(2);
        }

        String inputFile = args[1];
        String prevFile = "";

        final int sep = inputFile.indexOf('*');
        // file separator character found ?
        if (sep != -1)
        {
            prevFile = inputFile.substring(0, sep);
            inputFile = inputFile.substring(sep + 1, inputFile.length());
        }

        final String outputFile;
        boolean silent;

        if (args.length > 2)
            outputFile = args[2];
        else
        {
            if (cmd.equals("p"))
                outputFile = "packed.dat";
            else
                outputFile = "unpacked.dat";
        }

        // assume the fourth argument as silent
        silent = (args.length > 3);

        try
        {
            final byte[] data1 = pad((prevFile.isEmpty() ? new byte[0] : readBinaryFile(prevFile)));
            final byte[] data2 = readBinaryFile(inputFile);
            final byte[] data = new byte[data1.length + data2.length];
            final byte[] result;

            // concatenate the 2 arrays
            System.arraycopy(data1, 0, data, 0, data1.length);
            System.arraycopy(data2, 0, data, data1.length, data2.length);

            // compress
            if (cmd.equals("p"))
            {
                if (!silent)
                    System.out.println("Packing " + inputFile + "...");

                result = LZ4W.pack(data, data1.length, silent);

                final byte[] resultFull = new byte[data1.length + result.length];

                // concatenate the 2 arrays
                System.arraycopy(data1, 0, resultFull, 0, data1.length);
                System.arraycopy(result, 0, resultFull, data1.length, result.length);

                // verify compression
                LZ4W.unpack(resultFull, data1.length, data, silent);

                if (!silent)
                {
                    System.out.println("Initial size " + data2.length + " --> packed to " + result.length + " ("
                            + ((100 * result.length) / data2.length) + "%)");
                }
            }
            // unpack
            else
            {
                if (!silent)
                    System.out.println("Unpacking " + inputFile + "...");

                result = LZ4W.unpack(data, data1.length, null, silent);

                if (!silent)
                    System.out.println("Initial size " + data2.length + " --> unpacked to " + result.length);
            }

            writeBinaryFile(result, outputFile);
        }
        catch (IOException e)
        {
            System.out.println(e.getMessage());
        }
    }

    /**
     * Align size to word
     */
    static byte[] pad(byte[] buffer)
    {
        // need to pad ?
        if ((buffer.length & 1) == 1)
        {
            final byte[] result = new byte[buffer.length + 1];
            System.arraycopy(buffer, 0, result, 0, buffer.length);
            return result;
        }

        return buffer;
    }

    static void showUsage()
    {
        System.out.println("LZ4W packer v1.3 by Stephane Dallongeville (Copyright 2018)");
        System.out.println("  Pack:          lz4w p <input_file> <output_file>");
        System.out.println("                 lz4w p <prev_file>*<input_file> <output_file>");
        System.out.println("  Pack (best):   lz4w pp <input_file> <output_file>");
        System.out.println("                 lz4w pp <prev_file>*<input_file> <output_file>");
        System.out.println("  Unpack:        lz4w u <input_file> <output_file>");
        System.out.println("                 lz4w u <prev_file>*<input_file> <output_file>");
        System.out.println();
        System.out.println("Tip: using an extra parameter after <output_file> will act as 'silent mode' switch");
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
}