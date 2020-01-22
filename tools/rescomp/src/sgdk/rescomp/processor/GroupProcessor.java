package sgdk.rescomp.processor;

import java.io.IOException;

import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Group;

public class GroupProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "GROUP";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        if (fields.length < 1)
        {
            System.out.println("Wrong GROUP definition");
            System.out.println("GROUP");
            System.out.println("  Allow to group all binary resource export by type (may improve compression ratio)");

            return null;
        }

        // build GROUP resource
        return new Group("group");
    }
}