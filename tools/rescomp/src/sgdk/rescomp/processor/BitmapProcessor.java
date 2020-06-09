package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bitmap;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.FileUtil;

public class BitmapProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "BITMAP";
    }

    @Override
    public Resource execute(String[] fields) throws IllegalArgumentException, IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong BITMAP definition");
            System.out.println("BITMAP name \"file\" [compression]");
            System.out.println("  name          Bitmap variable name");
            System.out.println("  file          the image to convert to Bitmap structure (should be a 8bpp .bmp or .png)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                 -1 / BEST / AUTO = use best compression");
            System.out.println("                  0 / NONE        = no compression (default)");
            System.out.println("                  1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                  2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");

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

        return new Bitmap(id, fileIn, compression);
    }
}
