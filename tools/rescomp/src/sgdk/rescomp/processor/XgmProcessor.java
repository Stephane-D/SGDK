package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class XgmProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "XGM";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong XGM definition");
            System.out.println("XGM name file [timing [options]]");
            System.out.println("  name\t\tXGM music variable name");
            System.out.println("  file\tpath of the .vgm or .xgm music file to convert to binary data array");
            System.out.println("  timing\tdefine the XGM base timing");
            System.out
                    .println("      \t -1 (default) = AUTO (NTSC or PAL depending the information in source VGM file)");
            System.out.println("      \t  0 = NTSC (XGM is generated for NTSC system)");
            System.out.println("      \t  1 = PAL (XGM is generated for PAL system)");
            System.out.println("  options\toptionals parameters for xgmtool");
            System.out.println(
                    "      \t ex: \"-dr -di\" to disable some sample auto process (see xgmtool to get more info)");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);

        int timing = -1;
        String options = "";

        // get NTSC/PAL timing info
        if (fields.length >= 4)
            timing = StringUtil.parseInt(fields[3], -1);
        // get XGM compilation options
        if (fields.length >= 5)
            options = fields[4];

        final String fileOut = FileUtil.setExtension(fileIn, ".bin");

        // convert VGM/XGM to bin
        if (!Util.xgmtool(fileIn, fileOut, timing, options))
            throw new IOException("Error while compiling file '" + fileIn + "' to BIN format");

        // read data from binary file
        final byte[] data = Util.in(fileOut);
        // clean temp file
        FileUtil.delete(fileOut, false);

        // error while reading data
        if (data == null)
            throw new IOException("Can't read BIN data from file'" + fileOut + "' !");

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        // build BIN resource
        return new Bin(id, data, 256, 256, 0, Compression.NONE, true);
    }
}
