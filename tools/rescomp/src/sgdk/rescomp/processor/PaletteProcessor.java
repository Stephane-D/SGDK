package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Palette;
import sgdk.tool.FileUtil;

public class PaletteProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "PALETTE";
    }

    @Override
    public Resource execute(String[] fields) throws IllegalArgumentException, IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong PALETTE definition");
            System.out.println("PALETTE name file");
            System.out.println("  name       Palette variable name");
            System.out.println("  file       path of the .pal or image file to convert to Palette structure (should be a 8bpp / 4bpp .bmp or .png)");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        return new Palette(id, fileIn, 64, true);
    }
}
