package sgdk.rescomp.processor;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Sprite;
import sgdk.rescomp.resource.internal.SpriteFrame;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SpriteCell.OptimizationLevel;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class SpriteProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "SPRITE";
    }

    @Override
    public Resource execute(String[] fields) throws Exception
    {
        if (fields.length < 5)
        {
            System.out.println("Wrong SPRITE definition");
            System.out.println("SPRITE name \"file\" width height [compression [time [collision [opt_type [opt_level [opt_duplicate]]]]]]");
            System.out.println("  name          Sprite variable name");
            System.out.println("  file          the image file to convert to SpriteDefinition structure (BMP or PNG image)");
            System.out.println("  width         width of a single sprite frame in tile");
            System.out.println("  height        height of a single sprite frame in tile");
            System.out.println("  compression   compression type, accepted values:");
            System.out.println("                   -1 / BEST / AUTO = use best compression");
            System.out.println("                    0 / NONE        = no compression (default)");
            System.out.println("                    1 / APLIB       = aplib library (good compression ratio but slow)");
            System.out.println("                    2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)");
            System.out.println("  time          display frame time in 1/60 of second (time between each animation frame)");
            System.out.println("                    If this value is set to 0 (default) then auto animation is disabled");
            System.out.println("                    It can be set globally (single value) or independently for each frame of each animation");
            System.out.println("                    Example for a sprite sheet of 3 animations x 5 frames:");
            System.out.println("                    [[3,3,3,4,4][4,5,5][2,3,3,4]]");
            System.out.println("                    As you can see you can have empty value for empty frame");
            System.out.println("  collision     collision type: CIRCLE, BOX or NONE (BOX by default)");
            System.out.println("  opt_type      sprite cutting optimization strategy, accepted values:");
            System.out.println("                    0 / BALANCED  = balance between used tiles and hardware sprites (default)");
            System.out.println("                    1 / SPRITE    = reduce the number of hardware sprite (using bigger sprite) at the expense of more used tiles");
            System.out.println("                    2 / TILE      = reduce the number of tiles at the expense of more hardware sprite (using smaller sprite)");
            System.out.println("                    3 / NONE      = no optimization (cover the whole sprite frame)");
            System.out.println("  opt_level     optimization level for the sprite cutting operation:");
            System.out.println("                    FAST      = fast optimisation, good enough in general (default)");
            System.out.println("                    MEDIUM    = intermediate optimisation level, provide better results than FAST but ~5 time slower");
            System.out.println("                    SLOW      = advanced optimisation level using a genetic algorithm (80000 iterations), ~20 time slower than FAST");
            System.out.println("                    MAX       = maximum optimisation level, genetic algorithm (500000 iterations), ~100 time slower than FAST");
            System.out.println("  opt_duplicate enabled optimization of consecutive duplicated frames by removing them and increasing animation time to compensante.");
            System.out.println("                    FALSE     = no optimization (default)");
            System.out.println("                                Note that duplicated frames pixel data are still removed by rescomp binary blob optimizer");
            System.out.println("                    TRUE      = only the first instance of consecutive duplicated frames is kept and 'timer' value is increased to compensate the removed frames time.");
            System.out.println("                                Note that it *does* change the 'animation.numFrame' information so beware of that when enabling this optimization.");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get frame size (in tile)
        final int wf = StringUtil.parseInt(fields[3], 0);
        final int hf = StringUtil.parseInt(fields[4], 0);

        if ((wf < 1) || (hf < 1))
        {
            System.out.println("Wrong SPRITE definition");
            System.out.println("SPRITE name \"file\" width height [compression [time [collision [opt_type [opt_level]]]]]");
            System.out.println("  width and height (size of sprite frame) should be > 0");

            return null;
        }

        // frame size over limit (we need VDP sprite offset to fit into u8 type)
        if ((wf >= 32) || (hf >= 32))
        {
            System.out.println("Wrong SPRITE definition");
            System.out.println("SPRITE name \"file\" width height [compression [time [collision [opt_type [opt_level]]]]]");
            System.out.println("  width and height (size of sprite frame) should be < 32");

            return null;
        }

        // get packed value
        Compression compression = Compression.NONE;
        if (fields.length >= 6)
            compression = Util.getCompression(fields[5]);
        // get frame time
        int[][] time = new int[][] {{ 0 }};
        if (fields.length >= 7)
            time = StringUtil.parseIntArray2D(fields[6], new int[][] {{ 0 }});
        // get collision value
        CollisionType collision = CollisionType.NONE;
        if (fields.length >= 8)
            collision = Util.getCollision(fields[7]);
        // get optimization value
        OptimizationType opt = OptimizationType.BALANCED;
        if (fields.length >= 9)
            opt = Util.getSpriteOptType(fields[8]);
        // get max number of iteration
        OptimizationLevel optLevel = OptimizationLevel.FAST;
        boolean showCut = false;
        if (fields.length >= 10)
        {
            optLevel = Util.getSpriteOptLevel(fields[9]);
            showCut = true;
        }
        boolean optDuplicate = false;
        if (fields.length >= 11)
            optDuplicate = Boolean.parseBoolean(fields[10]);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        return new Sprite(id, fileIn, wf, hf, compression, time, collision, opt, optLevel, showCut, optDuplicate);
    }
}
