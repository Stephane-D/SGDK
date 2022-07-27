package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.SObject;
import sgdk.tool.StringUtil;

public class Objects extends Resource
{
    final String type;
    final List<SObject> objects;

    final int hc;

    public Objects(String id, String type, List<SObject> objects)
    {
        super(id);

        this.type = type;
        this.objects = objects;

        // compute hash code
        hc = type.hashCode() ^ objects.hashCode();
    }
    
    public int size()
    {
        return objects.size();
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Objects)
        {
            final Objects o = (Objects) obj;
            return StringUtil.equals(type, o.type) && objects.equals(o.objects);
        }

        return false;
    }

    @Override
    public List<Bin> getInternalBinResources()
    {
        return new ArrayList<>();
    }

    @Override
    public int shallowSize()
    {
        return objects.size() * 4;
    }

    @Override
    public int totalSize()
    {
        int result = 0;

        for (SObject object : objects)
            result += object.size();

        return result + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // export objects first
        for (SObject object : objects)
            object.out(outB, outS, outH);

        // declare
        Util.declArray(outS, outH, (StringUtil.isEmpty(type) ? "void" : type) + "*", id, objects.size(), 2, global);

        // set object pointers
        for (SObject object : objects)
            outS.append("    dc.l    " + object.getName() + "\n");

        outS.append("\n");
    }
}
