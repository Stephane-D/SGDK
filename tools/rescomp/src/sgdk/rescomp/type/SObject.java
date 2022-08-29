package sgdk.rescomp.type;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

import sgdk.tool.StringUtil;

/**
 * Single SGDK (custom) Object
 * 
 * @author Stephane
 */
public class SObject
{
    final int id;
    final String baseName;
    final String type;
    final double x;
    final double y;

    // internal
    private int off;

    final public List<SField> fields;

    public SObject(int id, String baseName, String type, double x, double y)
    {
        super();

        this.id = id;
        this.baseName = baseName;
        this.type = type;
        this.x = x;
        this.y = y;

        fields = new ArrayList<>();
        off = 0;
    }

    public int getFieldIndex(String fieldName)
    {
        int result = 0;

        for (SField field : fields)
        {
            if (StringUtil.equals(fieldName, field.name))
                return result;
            
            result++;
        }

        // not found
        return -1;
    }

    public boolean hasField(String fieldName)
    {
        return getFieldIndex(fieldName) != -1;
    }

    public void addField(SField sField)
    {
        if (hasField(sField.name))
            System.out.println("Warning: Object '" + getName() + "' already has a field named '" + sField.name + "', new field ignored...");

        // set padding to last added field
        if (((off & 1) != 0) && (sField.type.size() > 1))
        {
            fields.get(fields.size() - 1).padding = true;
            off++;
        }

        fields.add(sField);
        off += sField.size();
    }

    public String getName()
    {
        return baseName + "_" + id;
    }

    public long getFieldLongValue(String fieldName)
    {
        final int ind = getFieldIndex(fieldName);
        if (ind == -1)
            return 0L;

        final Long result = fields.get(ind).longValue;
        if (result == null)
            return 0L;

        return result.longValue();
    }

    public int size()
    {
        int result = 0;

        for (SField field : fields)
            result += field.totalSize();

        return result;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (obj instanceof SObject)
        {
            final SObject object = (SObject) obj;
            return StringUtil.equals(type, object.type) && (x == object.x) && (y == object.y) && fields.equals(object.fields);
        }

        return false;
    }

    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
    {
        // object can contain pointer so reset 'outB' buffer
        outB.reset();

        // output pre data (for String fields)
        for (SField field : fields)
            field.outPre(outS);

        // declare object
        outS.append("    .align 2\n");
        outS.append(getName() + ":\n");

        for (SField field : fields)
        {
            if (field.isIntData())
                field.outIntData(outS);
            else
                field.outNonIntData(outS);
        }

        // for (SField field : fields)
        // {
        // // write int data to 'outB' buffer
        // if (field.isIntData())
        // field.outIntData(outB);
        // else
        // {
        // if (outB.size() > 0)
        // {
        // // just for safety
        // if ((outB.size() & 1) != 0)
        // throw new RuntimeException("Error: unexpected odd size of binary buffer !");
        //
        // // output data
        // Util.outS(outS, outB.toByteArray(), 1);
        // // reset binary buffer
        // outB.reset();
        // }
        //
        // // then export pointer
        // field.outNonIntData(outS);
        // }
        // }
        //
        // // export remaining binary data
        // if (outB.size() > 0)
        // {
        // // output data
        // Util.outS(outS, outB.toByteArray(), 1);
        // // reset binary b)uffer
        // outB.reset();
        // }

        outS.append("\n");
    }

    @Override
    public String toString()
    {
        String result = "id=" + id + " type=" + type + " - ";

        for (SField field : fields)
            result += field.toString() + "; ";

        return result;
    }
}
