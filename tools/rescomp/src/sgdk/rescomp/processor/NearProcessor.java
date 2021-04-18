package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Near;

public class NearProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "NEAR";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        // build ALIGN resource
        return new Near("near");
    }
}