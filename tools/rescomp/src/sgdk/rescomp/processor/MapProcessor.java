package sgdk.rescomp.processor;

import java.io.IOException;
import java.security.InvalidParameterException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Map;
import sgdk.rescomp.resource.Tileset;
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
    public Resource execute(String[] fields) throws IllegalArgumentException, IOException
    {
        if (fields.length < 4)
        {
            System.out.println("Wrong MAP definition");
            System.out.println("MAP name \"file\" tileset_id [mapbase]");
            System.out.println("  name          Map variable name");
            System.out.println(
                    "  file          the map file to convert to Map structure (8bpp BMP or PNG image file, TMX Tiled file not yet supported)");
            System.out
                    .println("  tileset_id    base tileset resource to use (allow to share tileset with several map)");
            System.out.println(
                    "  mapbase       define the base tilemap value, useful to set a default priority, palette and base tile index offset");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get tileset
        final Tileset tileset = (Tileset) Compiler.getResourceById(fields[3]);
        // get map base
        int mapBase = 0;
        if (fields.length >= 5)
            mapBase = StringUtil.parseInt(fields[4], 0);

        // check tileset correctly found
        if (tileset == null)
            throw new InvalidParameterException(
                    "MAP resource definition error: Tileset '" + fields[3] + "' not found !");

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        return new Map(id, fileIn, mapBase, 2, tileset);
    }
}
