package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Ungroup;

public class UngroupProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "UNGROUP";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        if (fields.length < 1)
        {
            System.out.println("Wrong UNGROUP definition");
            System.out.println("UNGROUP");
            System.out.println("  Disable grouping of binary resource export by type (may lower a bit compression ratio)");

            return null;
        }

        // build UNGROUP resource
        return new Ungroup("ungroup");
    }
}