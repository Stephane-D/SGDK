package sgdk.rescomp.processor;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.FileUtil;

public class Xgm2Processor implements Processor
{
    @Override
    public String getId()
    {
        return "XGM2";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong XGM2 definition");
            System.out.println("Syntax for single track XGM:");
            System.out.println("XGM2 name file(s) [options]");
            System.out.println("  name      XGM2 music variable name");
            System.out.println("  file      path of the .vgm music file to convert in compiled XGM2 file");
            System.out.println("  options   optional(s) parameter(s) for xgm2Tool");
            System.out.println("                Should always start with '-' character otherwise it won't be recognized");
            System.out.println("                ex: \"-dr -di\" to disable some sample auto process (see xgm2tool to get more info)");
            System.out.println();
            System.out.println("Syntax for multi tracks XGM2 - allow PCM sharing:");
            System.out.println("XGM2 name file1 [file2] [...] [options]");
            System.out.println("  name      XGM2 music variable name");
            System.out.println("  file(s)   path(s) of the .vgm music file(s) to convert to compiled XGM2 file");
            System.out.println("                ex1: \"music.vgm\"");
            System.out.println("                ex2: \"music1.vgm\" \"music2.vgm\" ...");
            System.out.println("                ex3: \"*.vgm\"");
            System.out.println("  options   optional(s) parameter(s) for xgm2Tool");
            System.out.println("                Should always start with '-' character otherwise it won't be recognized");
            System.out.println("                ex: \"-dr -di\" to disable some sample auto process (see xgm2tool to get more info)");
            
            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file and options
        final List<String> fileIns = new ArrayList<>();
        String options = "";
        int f = 2;
        while (f < fields.length)
        {
            final String field = fields[f++];
            // check if this is the options parameter
            if ((f == fields.length) && field.startsWith("-")) options = field;
            else fileIns.add(FileUtil.adjustPath(Compiler.resDir, field));
        }

        // convert VGM to bin
        if (!Util.xgm2tool(fileIns, "out.xgc", options))
            throw new IOException("Error while compiling file(s) '" + String.join(" ", fileIns) + "' to BIN format");

        // read data from binary file
        final byte[] data = Util.in("out.xgc");
        // clean temp file
        FileUtil.delete("out.xgc", false);

        // error while reading data
        if (data == null)
            throw new IOException("Can't read data from result file !");

        // add resource file (used for deps generation)
        fileIns.forEach((fileIn) -> Compiler.addResourceFile(fileIn));

        // build BIN resource
        return new Bin(id, data, 256, 256, 0, Compression.NONE, true);
    }
}
