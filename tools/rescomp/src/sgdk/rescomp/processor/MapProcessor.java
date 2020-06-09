package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.FileUtil;

public class MapProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "MAP";
    }

    @Override
    public Resource execute(String[] fields) throws IllegalArgumentException, IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong MAP definition");
            System.out.println("MAP name \"file\" [compression]");
            System.out.println("  name          Map variable name");
            System.out.println("  file          the map file to convert to Map structure (.tmx Tiled file)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                 -1 / BEST / AUTO  = use best compression");
            System.out.println("                  0 / NONE         = no compression (default)");
            System.out.println("                  1 / APLIB        = aplib library (good compression ratio but slow)");
            System.out.println("                  2 / FAST / LZ4W  = custom lz4 compression (average compression ratio but fast)");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get packed value
        Compression compression = Compression.NONE;
        if (fields.length >= 4)
            compression = Util.getCompression(fields[3]);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);
        
        System.err.println("MAP resource not yet supported !");

        return null;
    }
}
