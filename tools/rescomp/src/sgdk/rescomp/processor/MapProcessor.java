package sgdk.rescomp.processor;

import java.security.InvalidParameterException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Map;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.TMX.TMXMap;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class MapProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "MAP";
    }

    @Override
    public Resource execute(String[] fields) throws Exception
    {
        if (fields.length < 4)
        {
            System.out.println("Wrong MAP definition");
            System.out.println("MAP name \"img_file\" tileset_id [compression [map_base]]");
            System.out.println("  name          Map variable name");
            System.out.println("  img_file      path of the input image file (BMP or PNG image file)");
            System.out.println("  tileset_id    base tileset resource to use (allow to share tileset along several maps)");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                   -1 / BEST / AUTO = use best compression");
            System.out.println("                    0 / NONE        = no compression (default)");
            System.out.println("                    1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                    2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  map_base      define the base tilemap value, useful to set a default priority, palette and base tile index offset");
            System.out.println(
                    "                    Using a base tile index offset (static tile allocation) allow to use faster MAP decoding function internally.");
            System.out.println();
            System.out.println("MAP name \"tmx_file\" \"layer_id\" [ts_compression [map_compression [map_base]]]");
            System.out.println("  name              Map variable name");
            System.out.println("  tmx_file          path of the input TMX file (TMX Tiled file)");
            System.out.println("  layer_id          layer name we want to extract map data from.");
            System.out.println("  ts_compression    compression type for tileset, accepted values:");
            System.out.println("                       -1 / BEST / AUTO = use best compression");
            System.out.println("                        0 / NONE        = no compression (default)");
            System.out.println("                        1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                        2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  map_compression   compression type for map (same accepted values then 'ts_compression')");
            System.out.println("  map_base          define the base tilemap value, useful to set a default priority, palette and base tile index offset");

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
            // then build MAP from TMX Map
            return new Map(id, tmxMap.getMapImage(), tmxMap.w * tmxMap.tileSize, tmxMap.h * tmxMap.tileSize, mapBase, 2, tmxMap.getTilesets(id, tileSetCompression, false),
                    mapCompression, true);
        }
        else
        // image file
        {
            // get tileset
            final Tileset tileset = (Tileset) Compiler.getResourceById(fields[3]);
            // check tileset correctly found
            if (tileset == null)
                throw new InvalidParameterException("MAP resource definition error: Tileset '" + fields[3] + "' not found !");

            // get packed value
            Compression compression = Compression.NONE;
            if (fields.length >= 5)
                compression = Util.getCompression(fields[4]);
            // get map base
            int mapBase = 0;
            if (fields.length >= 6)
                mapBase = StringUtil.parseInt(fields[5], 0);

            // build MAP from an image
            return Map.getMap(id, fileIn, mapBase, 2, Util.asList(tileset), compression, true);
        }
    }
}
