package sgdk.aplib;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

class Launcher
{
    public static void main(String[] args) throws Exception
    {
        if (!ValidInput(args))
        {
            PrintUsage();
            System.exit(-1);
        }

        String command = args[0];
        String loadname = args[1];
        String savename = args[2];
        boolean silent = args.length > 3;

        byte[] input = LoadFile(loadname);
        byte[] output = new byte[0];

        if (command.equals("p"))
            output = Cap.encode(input, silent);
        else if (command.equals("u"))
            output = Cap.decode(input, silent);

        SaveFile(output, savename);        
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
        System.out.println("APJ packer v1.00 by Stephane Dallongeville (Copyright 2020)");
        System.out.println(
                "This is a simple port (originally in C#) of CAP aplib compressor tool made by Svendahl (https://github.com/svendahl/cap)");
        System.out.println("  Pack:     java -jar apj.jar p <input_file> <output_file>");
        System.out.println("  Unpack:   java -jar apj.jar u <input_file> <output_file>");
        System.out.println();
        System.out.println("Tip: using an extra parameter after <output_file> will act as 'silent mode' switch");
    }

    private static boolean ValidInput(String[] input)
    {
        return (input.length >= 3 && input[0].length() == 1 && (input[0].equals("p") || input[0].equals("u"))
                && new File(input[1]).exists() && !input[1].equals(input[2]));
    }
}
