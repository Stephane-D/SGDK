package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Align;
import sgdk.tool.StringUtil;

public class AlignProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "ALIGN";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        if (fields.length < 1)
        {
            System.out.println("Wrong ALIGN definition");
            System.out.println("ALIGN [alignment]");
            System.out.println(
                    "  alignment     specifies the minimum binary data alignment in bytes (default is 524288)");

            return null;
        }

        // get alignment (default alignment is bank size alignment = 512KB)
        int alignment = 524288;
        if (fields.length > 1)
            alignment = StringUtil.parseInt(fields[1], alignment);

        // build ALIGN resource
        return new Align("align", alignment);
    }
}