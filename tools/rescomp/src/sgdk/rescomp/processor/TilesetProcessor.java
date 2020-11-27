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
            System.out.println("TILESET name \"file\" [compression [opt [metatile]]]");
            System.out.println("  name          Tileset variable name");
            System.out.println(
                    "  file          the image to convert to TileSet structure (should be a 8bpp .bmp or .png)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                 -1 / BEST / AUTO = use best compression");
            System.out.println("                  0 / NONE        = no compression (default)");
            System.out.println("                  1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println(
                    "                  2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  opt           define the optimisation level, accepted values:");
            System.out.println("                  0 / NONE        = no optimisation, each tile is unique");
            System.out.println("                  1 / ALL         = ignore duplicated and flipped tile (default)");
            System.out.println("                  2 / DUPLICATE   = ignore duplicated tile only");

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
        // get optimization value
        TileOptimization opt = TileOptimization.ALL;
        if (fields.length >= 5)
            opt = Util.getTileOpt(fields[4]);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        // TILESET resource never optimize tiles and we ignore palette information
        return Tileset.getTileset(id, fileIn, compression, opt, 256);
    }
}
