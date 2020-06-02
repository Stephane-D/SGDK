package sgdk.rescomp;

import sgdk.tool.FileUtil;

public class Launcher
{
    public static void main(String[] args)
    {
        // default
        String fileName = null;
        String fileNameOut = null;
        String depsFileName = null;
        boolean header = true;
        boolean generateDeps = false;

        // parse parameters
        for (int i = 0; i < args.length; i++)
        {
            final String param = args[i];

            if (param.equalsIgnoreCase("-noheader"))
                header = false;
            else if (param.equalsIgnoreCase("-d"))
                generateDeps = true;
            else if (generateDeps == true && depsFileName == null)
                depsFileName = param;
            else if (fileName == null)
                fileName = param;
            else if (fileNameOut == null)
                fileNameOut = param;
        }

        System.out.println("ResComp 2.75 - SGDK Resource Compiler - Copyright 2020 (Stephane Dallongeville)");

        if (fileName == null)
        {
            System.out.println("Error: missing the input file.");
            System.out.println();
            System.out.println("Usage:");
            System.out.println("  rescomp input [output] [-noheader] [-d]");
            System.out.println("    input: the input resource file (.res)");
            System.out.println("    output: the asm output filename (same name is used for the include file)");
            System.out.println("    -noheader: specify that we don't want to generate the header file (.h)");
            System.out.println("    -d: generate dependencies for make (experimental)");
            System.out.println("    deps_output: dependencies output filename. If not specified, input/output will be used");
            System.out.println("  Ex: rescomp resources.res outres.s");
            System.out.println("  Ex: rescomp resources.res -d outres.d");

            // stop here with error code 1
            System.exit(1);
        }

        // if not defined we use input file name as output file name
        if (fileNameOut == null)
            fileNameOut = FileUtil.setExtension(fileName, "");

        // If deps required but name not defined, uses the name from out
        if(generateDeps && depsFileName == null) {
            depsFileName = FileUtil.setExtension(fileNameOut, ".d");
        }

        // compile resources
        boolean result = Compiler.compile(fileName, fileNameOut, header, generateDeps, depsFileName);

        if (result)
            System.exit(0);
        else
            System.exit(-1);
    }
}
