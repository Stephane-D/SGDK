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
        if (fields.length < 2)
        {
            System.out.println("Wrong ALIGN definition");
            System.out.println("ALIGN alignment");
            System.out.println("  alignment     specifies the minimum data alignment in bytes");

            return null;
        }

        // get alignment
        final int alignment = StringUtil.parseInt(fields[1], 0);

        // build ALIGN resource
        return new Align("align", alignment);
    }
}