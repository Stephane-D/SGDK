package sgdk.rescomp.processor;

import java.security.InvalidParameterException;
import java.util.List;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Tilemap;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.TMX.TMXMap;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class TilemapProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "TILEMAP";
    }

    @Override
    public Resource execute(String[] fields) throws Exception
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong TILEMAP definition");
            System.out.println("TILEMAP name \"img_file\" tileset_id [compression [map_opt [map_base]]]");
            System.out.println("  name          Tilemap variable name");
            System.out.println("  file          path of the input image file (BMP or PNG image file)");
            System.out.println("  tileset_id    base tileset resource to use (allow to share tileset along several maps)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                   -1 / BEST / AUTO = use best compression");
            System.out.println("                    0 / NONE        = no compression (default)");
            System.out.println("                    1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                    2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  map_opt       define the map optimisation level, accepted values:");
            System.out.println("                    0 / NONE        = no optimisation, each tile is unique");
            System.out.println("                    1 / ALL         = find duplicate and flipped tile (default)");
            System.out.println("                    2 / DUPLICATE   = find duplicate tile only");
            System.out.println("  map_base      define the base tilemap value, useful to set a default priority, palette and base tile index offset");
            System.out.println();
            System.out.println("TILEMAP name \"tmx_file\" \"layer_id\" [ts_compression [map_compression [map_base]]]");
            System.out.println("  name              Tilemap variable name");
            System.out.println("  tmx_file          path of the input TMX file (TMX Tiled file)");
            System.out.println("  layer_id          layer name we want to extract map data from.");
            System.out.println("  ts_compression    compression type for tileset, accepted values:");
            System.out.println("                       -1 / BEST / AUTO = use best compression");
            System.out.println("                        0 / NONE        = no compression (default)");
            System.out.println("                        1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                        2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  map_compression   compression type for map (same accepted values then 'ts_compression')");
            System.out.println("  map_base          define the base tilemap value, useful to set a default priority, palette and base tile index offset.");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        // TMX file
        if (FileUtil.getFileExtension(fileIn, false).toLowerCase().equals("tmx"))
        {
            // get layer name
            final String layerName = fields[3];
            // get compression value
            Compression tileSetCompression = Compression.NONE;
            if (fields.length >= 5)
                tileSetCompression = Util.getCompression(fields[4]);
            Compression mapCompression = Compression.NONE;
            if (fields.length >= 6)
                mapCompression = Util.getCompression(fields[5]);
            // get map base
            int mapBase = 0;
            if (fields.length >= 7)
                mapBase = StringUtil.parseInt(fields[6], 0);

            // build TMX map
            final TMXMap tmxMap = new TMXMap(fileIn, layerName);
            // get tilesets for this TMX map
            final List<Tileset> tilesets = tmxMap.getTilesets(id, tileSetCompression, false);

            // then build TileMap from TMX Map
            return Tilemap.getTilemap(id, new Tileset(tilesets), mapBase, tmxMap.getMapImage(), (tmxMap.w * tmxMap.tileSize) / 8,
                    (tmxMap.h * tmxMap.tileSize) / 8, TileOptimization.ALL, mapCompression);
        }
        else
        // image file
        {
            // get packed value
            Compression compression = Compression.NONE;
            if (fields.length >= 5)
                compression = Util.getCompression(fields[4]);
            // get map optimization value
            TileOptimization tileOpt = TileOptimization.ALL;
            if (fields.length >= 6)
                tileOpt = Util.getTileOpt(fields[5]);
            // get map base
            int mapBase = 0;
            if (fields.length >= 7)
                mapBase = StringUtil.parseInt(fields[6], 0);

            // get tileset
            final Tileset tileset = (Tileset) Compiler.getResourceById(fields[3]);
            // check tileset correctly found
            if (tileset == null)
                throw new InvalidParameterException("TILEMAP resource definition error: Tileset '" + fields[3] + "' not found !");

            return Tilemap.getTilemap(id, tileset, mapBase, fileIn, tileOpt, compression);
        }
    }
}
