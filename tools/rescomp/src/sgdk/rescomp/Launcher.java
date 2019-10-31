package sgdk.rescomp;

import sgdk.tool.FileUtil;

public class Launcher
{
    public static void main(String[] args)
    {
        // default
        String fileName = null;
        String fileNameOut = null;
        boolean header = true;

        // parse parameters
        for (int i = 0; i < args.length; i++)
        {
            final String param = args[i];

            if (param.equalsIgnoreCase("-noheader"))
                header = false;
            else if (fileName == null)
                fileName = param;
            else if (fileNameOut == null)
                fileNameOut = param;
        }

        // fileName = "gfx.res";

        System.out.println("ResComp 2.41 - SGDK Resource Compiler - Copyright 2019 (Stephane Dallongeville)");

        if (fileName == null)
        {
            System.out.println("Error: missing the input file.");
            System.out.println();
            System.out.println("Usage:");
            System.out.println("  rescomp input [output] [-noheader]");
            System.out.println("    input: the input resource file (.res)");
            System.out.println("    output: the asm output filename (same name is used for the include file)");
            System.out.println("    -noheader: specify that we don't want to generate the header file (.h)");
            System.out.println("  Ex: rescomp resources.res outres.s");

            // stop here with error code 1
            System.exit(1);
        }

        // if not defined we use input file name as output file name
        if (fileNameOut == null)
            fileNameOut = FileUtil.setExtension(fileName, "");

        // compile resources
        Compiler.compile(fileName, fileNameOut, header);
    }
}
