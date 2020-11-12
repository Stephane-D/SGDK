package sgdk.rescomp;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

public abstract class Resource
{
    public final String id;
    public boolean global;

    public Resource(String id)
    {
        super();

        this.id = id;
        global = true;
    }

    /**
     * Add this resource as internal resource
     */
    public static Resource addInternalResource(Resource resource)
    {
        // add this resource as 'internal' resource
        return Compiler.addResource(resource, true);
    }

    public abstract int internalHashCode();

    public abstract boolean internalEquals(Object obj);

    @Override
    public int hashCode()
    {
        return internalHashCode() ^ Boolean.valueOf(global).hashCode();
    }

    @Override
    public boolean equals(Object obj)
    {
        if (obj instanceof Resource)
        {
            final Resource res = (Resource) obj;
            // fast discard from hash code and global state
            return (hashCode() == res.hashCode()) && (global == res.global) && internalEquals(obj);
        }

        return false;
    }

    /**
     * Return size in byte used to store current data structure without counting referenced objects
     */
    public abstract int shallowSize();

    /**
     * Return total size in byte used to store current data structure (counting referenced objects)
     */
    public abstract int totalSize();

    public abstract void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException;

}
