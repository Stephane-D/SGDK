package sgdk.rescomp.type;

import sgdk.tool.StringUtil;

public class SField extends SFieldDef
{
    static private int genId = 0;

    static synchronized private int nextId()
    {
        return genId++;
    }

    final public String value;
    final public Long longValue;
    final int internalId;
    boolean padding;

    public SField(String name, SGDKObjectType type, String value) throws Exception
    {
        super(name, type);

        this.value = value;
        padding = false;
        longValue = computeLongValue();

        // used to make string label uniq
        internalId = nextId();
    }

    private Long computeLongValue() throws Exception
    {
        if (!isIntData())
            return null;

        double v;

        try
        {
            v = Double.parseDouble(value);
        }
        catch (NumberFormatException e)
        {
            return null;
        }

        if (type == SGDKObjectType.F16)
        {
            // type checking (2^9)
            if ((v > 511.4d) || (v < -512.4d))
                throw new Exception("Value " + StringUtil.toString(v, 2) + " for field '" + name + "' overflows the specified fix16 type");

            v *= Math.pow(2, 6);
        }
        else if (type == SGDKObjectType.F32)
        {
            // type checking (2^21)
            if ((v > 2097151.4d) || (v < -2097152.4d))
                throw new Exception("Value " + StringUtil.toString(v, 3) + " for field '" + name + "' overflows the specified fix32 type");

            v *= Math.pow(2, 10);
        }

        final long result = Math.round(v);

        switch (type)
        {
            case S8:
                // type checking
                if ((result > Byte.MAX_VALUE) || (result < Byte.MIN_VALUE))
                    throw new Exception("Value " + StringUtil.toString(result) + " for field '" + name + "' overflows the specified " + type + " type");
                break;

            case U8:
                // type checking
                if ((result > 255) || (result < 0))
                    throw new Exception("Value " + StringUtil.toString(result) + " for field '" + name + "' overflows the specified " + type + " type");
                break;

            case S16:
                // type checking
                if ((result > Short.MAX_VALUE) || (result < Short.MIN_VALUE))
                    throw new Exception("Value " + StringUtil.toString(result) + " for field '" + name + "' overflows the specified " + type + " type");
                break;

            case U16:
                // type checking
                if ((result > 65535) || (result < 0))
                    throw new Exception("Value " + StringUtil.toString(result) + " for field '" + name + "' overflows the specified " + type + " type");
                break;

            case S32:
                // type checking
                if ((result > Integer.MAX_VALUE) || (result < -Integer.MIN_VALUE))
                    throw new Exception("Value " + StringUtil.toString(result) + " for field '" + name + "' overflows the specified " + type + " type");
                break;

            case U32:
                // type checking
                if ((result > 4294967295L) || (result < 0))
                    throw new Exception("Value " + StringUtil.toString(result) + " for field '" + name + "' overflows the specified " + type + " type");
                break;

            default:
                break;
        }

        return Long.valueOf(result);
    }

    public int totalSize()
    {
        return size() + (isString() ? (value.length() + 1) : 0);
    }

    public int size()
    {
        return type.size() + (padding ? 1 : 0);
    }

    @Override
    public boolean equals(Object obj)
    {
        if (obj instanceof SField)
        {
            final SField field = (SField) obj;
            return (type == field.type) && StringUtil.equals(value, field.value) && (padding == field.padding);
        }

        return false;
    }

    public void outPre(StringBuilder outS)
    {
        // only for string
        if (!isString())
            return;

        // align
        outS.append("    .align 2\n");
        // declare name
        outS.append(name + "_" + internalId + ":\n");
        // declare string
        outS.append("    .asciz \"" + value + "\"\n");
        outS.append("\n");
    }

    public void outNonIntData(StringBuilder outS)
    {
        // only for string or pointer
        if (isIntData())
            return;

        // object pointer ?
        if (isPointer())
            outS.append("    dc.l    " + (StringUtil.isEmpty(value) ? "0" : value) + "\t\t// " + name + "\n");
        else
            // string
            outS.append("    dc.l    " + name + "_" + internalId + "\t\t// " + name + "\n");
    }

    public void outIntData(StringBuilder outS)
    {
        if (!isIntData())
            return;

        final String sVal = (longValue == null) ? value : longValue.toString();

        switch (type)
        {
            case F32:
            case S32:
            case U32:
                outS.append("    dc.l    " + sVal + "\t\t// " + name + "\n");
                break;

            case F16:
            case S16:
            case U16:
                outS.append("    dc.w    " + sVal + "\t\t// " + name + "\n");
                break;

            case S8:
            case U8:
            case BOOL:
                outS.append("    dc.b    " + sVal + "\t\t// " + name + "\n");
                break;

            default:
                break;
        }

        // padding ?
        if (padding)
            outS.append("    dc.b    0\t\t// padding\n");
    }

    // public void outIntData(ByteArrayOutputStream out)
    // {
    // if (!isIntData())
    // return;
    //
    // final int v1 = (int) ((longValue >> 0) & 0xFF);
    // final int v2 = (int) ((longValue >> 8) & 0xFF);
    // final int v3 = (int) ((longValue >> 16) & 0xFF);
    // final int v4 = (int) ((longValue >> 24) & 0xFF);
    //
    // switch (type)
    // {
    // case F32:
    // case S32:
    // case U32:
    // out.write(v4);
    // out.write(v3);
    // //$FALL-THROUGH$
    // case F16:
    // case S16:
    // case U16:
    // out.write(v2);
    // //$FALL-THROUGH$
    // case S8:
    // case U8:
    // case BOOL:
    // out.write(v1);
    // break;
    //
    // default:
    // break;
    // }
    //
    // // padding ?
    // if (padding)
    // out.write(0);
    // }

    @Override
    public String toString()
    {
        return name + ":" + type + " = " + value;
    }
}
