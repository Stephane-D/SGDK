package sgdk.rescomp;

import java.io.IOException;

public interface Processor
{
    public String getId();

    public Resource execute(String[] fields) throws IOException;
}
