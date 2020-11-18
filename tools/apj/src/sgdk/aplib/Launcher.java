package sgdk.aplib;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;

class Launcher
{
    public static void main(String[] args) throws Exception
    {
        if (!ValidInput(args))
        {
            PrintUsage();
            System.exit(-1);
        }

        final String command = args[0];
        final String loadname = args[1];
        final String savename = args[2];
        final boolean silent = args.length > 3;
        final byte[] input = LoadFile(loadname);
        final byte[] output;

        final long start = System.currentTimeMillis();

        if (command.equals("p") | command.equals("pp"))
        {
            // compress
            output = APJ.pack(input, command.equals("pp"), silent);

            final long time = System.currentTimeMillis() - start;

            // verify compression
            if (!Arrays.equals(input, APJ.unpack(output, input)))
            {
                System.err.println("Error while verifying compression, result data mismatch input data !");
                System.exit(1);
            }

            if (!silent)
            {
                System.out.println("Initial size " + input.length + " --> packed to " + output.length + " ("
                        + ((100 * output.length) / input.length) + "%) in " + time + " ms");
            }
        }
        else
        {
            output = APJ.unpack(input, null);

            final long time = System.currentTimeMillis() - start;
            if (!silent)
            {
                System.out.println(
                        "Initial size " + input.length + " --> unpacked to " + output.length + " in " + time + " ms");
            }
        }

        // save result file
        SaveFile(output, savename);
        // and success exit
        System.exit(0);
    }

    private static byte[] LoadFile(String filename) throws IOException
    {
        Path path = Paths.get(filename);
        return Files.readAllBytes(path);
    }

    private static void SaveFile(byte[] data, String filename) throws IOException
    {
        Path path = Paths.get(filename);
        Files.write(path, data);
    }

    private static void PrintUsage()
    {
        System.out.println("APJ (ApLib for Java) packer v1.3 by Stephane Dallongeville (Copyright 2020)");
        System.out.println("  Pack:       java -jar apj.jar p <input_file> <output_file>");
        System.out.println("  Pack max:   java -jar apj.jar pp <input_file> <output_file>");
        System.out.println("  Unpack:     java -jar apj.jar u <input_file> <output_file>");
        System.out.println();
        System.out.println("Tip: using an extra parameter after <output_file> will act as 'silent mode' switch");
    }

    private static boolean ValidInput(String[] input)
    {
        return (input.length >= 3) && (input[0].equals("p") || input[0].equals("pp") || input[0].equals("u"))
                && new File(input[1]).exists() && !input[1].equals(input[2]);
    }
}
