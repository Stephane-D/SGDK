package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.tool.FileUtil;

public class TilesetProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "TILESET";
    }

    @Override
    public Resource execute(String[] fields) throws IllegalArgumentException, IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong TILESET definition");
            System.out.println("TILESET name \"file\" [packed]");
            System.out.println("  name      Tileset variable name");
            System.out
                    .println("  file      the image to convert to TileSet structure (should be a 8bpp .bmp or .png)");
            System.out.println("  packed    compression type, accepted values:");
            System.out.println("              -1 / BEST / AUTO = use best compression");
            System.out.println("               0 / NONE        = no compression");
            System.out.println("               1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println(
                    "               2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");

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

        // TILESET resource never optimize tiles and is limited to a single palette
        return Tileset.getTileset(id, fileIn, compression, TileOptimization.NONE, 16);
    }
}
