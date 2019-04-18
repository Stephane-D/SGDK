package sgdk.rescomp.processor;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class BinProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "BIN";
    }

    @Override
    public Resource execute(String[] fields)
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong BIN definition");
            System.out.println("BIN name file [align [salign [fill]]]");
            System.out.println("  name\t\tBIN data variable name");
            System.out.println("  file\tpath of the data file to convert to binary data array");
            System.out.println("  align\tmemory address alignment for generated data array (default is 2)");
            System.out.println("  salign\tsize alignment for the generated data array (default is 2)");
            System.out.println("  fill\tfill value for the size alignment (default is 0)");

            return null;
        }

        int align = 2;
        int salign = 2;
        int fill = 0;

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get address alignment
        if (fields.length >= 4)
            align = StringUtil.parseInt(fields[3], align);
        // get size alignment
        if (fields.length >= 5)
            salign = StringUtil.parseInt(fields[4], salign);
        // get fill value
        if (fields.length >= 6)
            fill = StringUtil.parseInt(fields[5], fill);

        // read data from BIN file
        final byte[] data = FileUtil.load(fileIn, true);
        // error while reading data
        if (data == null)
            return null;

        return new Bin(id, data, align, salign, fill);

        // EXPORT BIN
        // outBIN(data, align, outS, outH, id, true);
        //
        // return true;
    }

    // public static void outBIN(byte[] data, int align, PrintWriter outS, PrintWriter outH, String
    // id, boolean global)
    // {
    // // declare
    // Util.declArray(outS, outH, "u8", id, data.length, align, true);
    // // output data
    // Util.outS(data, outS, 1);
    // outS.println();
    // }
}
