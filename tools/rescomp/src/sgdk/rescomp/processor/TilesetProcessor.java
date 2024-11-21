package sgdk.rescomp.processor;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.Basics.TileOrdering;
import sgdk.rescomp.type.TSX;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class TilesetProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "TILESET";
    }

    @Override
    public Resource execute(String[] fields) throws Exception
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong TILESET definition");
            System.out.println("TILESET name \"file\" [compression [opt [ordering [export]]]]");
            System.out.println("  name          Tileset variable name");
            System.out.println("  file          path of the input file (BMP, PNG image file or TSX Tiled file)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                   -1 / BEST / AUTO = use best compression");
            System.out.println("                    0 / NONE        = no compression (default)");
            System.out.println("                    1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                    2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  opt           define the optimisation level, accepted values:");
            System.out.println("                    0 / NONE        = no optimisation, each tile is unique (default for TSX file)");
            System.out.println("                    1 / ALL         = ignore duplicated and flipped tile (default for image file)");
            System.out.println("                    2 / DUPLICATE   = ignore duplicated tile only");
            System.out.println("  ordering      define the tile process order, accepted values:");
            System.out.println("                    ROW             = process per row (default)");
            System.out.println("                    COLUMN          = process per column");
            System.out.println("  export        enable the tileset PNG export, accept values:");
            System.out.println("                    0 / FALSE       = no PNG export (default)");
            System.out.println("                    1 / TRUE        = optimized tileset is export to PNG format '<file>-tileset-export.png'");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);

        // TSX file
        final boolean tsx = FileUtil.getFileExtension(fileIn, false).toLowerCase().equals("tsx");

        // get packed value
        Compression compression = Compression.NONE;
        if (fields.length >= 4)
            compression = Util.getCompression(fields[3]);
        // get optimization value
        TileOptimization opt = TileOptimization.ALL;
        // force ALL for TSX format
        if (!tsx && (fields.length >= 5))
            opt = Util.getTileOpt(fields[4]);
        // get tile ordering value
        TileOrdering order = TileOrdering.ROW;
        if (fields.length >= 6)
            order = Util.getTileOrdering(fields[5]);
        // PNG export
        boolean export = false;
        if (fields.length >= 7)
            export = StringUtil.parseBoolean(fields[6], false);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        return Tileset.getTileset(id, tsx ? TSX.getTSXTilesetPath(fileIn) : fileIn, compression, opt, tsx, false, order, export);
    }
}
