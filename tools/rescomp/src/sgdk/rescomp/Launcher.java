package sgdk.rescomp;

import sgdk.tool.FileUtil;

public class Launcher
{
    public static void main(String[] args)
    {
        // default
        String fileName = null;
        String fileNameOut = null;
        String depTarget = null;
        boolean header = true;
        boolean dep = false;

        // parse parameters
        for (int i = 0; i < args.length; i++)
        {
            final String param = args[i];

            if (param.equalsIgnoreCase("-noheader"))
                header = false;
            else if (param.equalsIgnoreCase("-dep"))
                dep = true;
            else if (fileName == null)
                fileName = param;
            else if (fileNameOut == null)
                fileNameOut = param;
            else if (dep && depTarget == null)
                depTarget = param;
        }

        System.out.println("ResComp 3.22 - SGDK Resource Compiler - Copyright 2021 (Stephane Dallongeville)");

        if (fileName == null)
        {
            System.out.println("Error: missing the input file.");
            System.out.println();
            System.out.println("Usage:");
            System.out.println("  rescomp input [output] [-noheader] [-dep <target_file>]");
            System.out.println("    input: the input resource file (.res)");
            System.out.println("    output: the asm output filename (same name is used for the include file)");
            System.out.println("    -noheader: specify that we don't want to generate the header file (.h)");
            System.out.println("    -dep: generate dependencies file (.d) for make (experimental)");
            System.out.println("      <target_file> allow to specify the target filename in the .d file (not the destination of the .d file itself)");
            System.out.println("  Ex: rescomp resources.res outres.s");
            System.out.println("  Ex: rescomp resources.res outres.s -noheader -dep");
            System.out.println("  Ex: rescomp resources.res outres.s -dep out/res/gfx.o");

            // stop here with error code 1
            System.exit(1);
        }

        // separate
        System.out.println();

        // if not defined we use input file name as output file name with .s extension
        if (fileNameOut == null)
            fileNameOut = FileUtil.setExtension(fileName, ".s");
        // define default dep target name
        if (dep && (depTarget == null))
            depTarget = fileNameOut;

        // compile resources
        boolean result = Compiler.compile(fileName, fileNameOut, header, depTarget);

        if (result)
            System.exit(0);
        else
            System.exit(-1);
    }
}
