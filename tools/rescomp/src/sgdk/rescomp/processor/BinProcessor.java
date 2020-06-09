package sgdk.rescomp.processor;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class BinProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "BIN";
    }

    @Override
    public Resource execute(String[] fields)
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong BIN definition");
            System.out.println("BIN name file [align [salign [fill [compression [far]]]]]");
            System.out.println("  name          BIN data variable name");
            System.out.println("  file          path of the data file to convert to binary data array");
            System.out.println("  align         memory address alignment for generated data array (default is 2)");
            System.out.println("  salign        size alignment for the generated data array (default is 2)");
            System.out.println("  fill          fill value for the size alignment (default is 0)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                 -1 / BEST / AUTO = use best compression");
            System.out.println("                  0 / NONE        = no compression (default)");
            System.out.println("                  1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println(
                    "                  2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println(
                    "  far           'far' binary data flag to put it at the end of the ROM (useful for bank switch, default = TRUE)");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get address alignment
        int align = 2;
        if (fields.length >= 4)
            align = StringUtil.parseInt(fields[3], align);
        // get size alignment
        int salign = 2;
        if (fields.length >= 5)
            salign = StringUtil.parseInt(fields[4], salign);
        // get fill value
        int fill = 0;
        if (fields.length >= 6)
            fill = StringUtil.parseInt(fields[5], fill);
        // get compression value
        Compression compression = Compression.NONE;
        if (fields.length >= 7)
            compression = Util.getCompression(fields[6]);
        // get far value
        boolean far = true;
        if (fields.length >= 8)
            far = StringUtil.parseBoolean(fields[7], far);

        // read data from BIN file
        final byte[] data = FileUtil.load(fileIn, true);
        // error while reading data
        if (data == null)
            return null;

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        return new Bin(id, data, align, salign, fill, compression, far);
    }
}
