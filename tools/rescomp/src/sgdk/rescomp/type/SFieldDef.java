package sgdk.rescomp.type;

import sgdk.rescomp.type.TFieldDef.TiledObjectType;

public class SFieldDef
{
    public static enum SGDKObjectType
    {
        U8, S8, U16, S16, U32, S32, F16, F32, BOOL, STRING, OBJECT;

        public static boolean isValid(String sgdkType)
        {
            try
            {
                fromString(sgdkType);
                return true;
            }
            catch (IllegalArgumentException e)
            {
                return false;
            }
        }

        public static SGDKObjectType fromString(String sgdkType)
        {
            final String t = sgdkType.toLowerCase();

            for (SGDKObjectType value : values())
                if (t.equals(value.toString().toLowerCase()))
                    return value;

            throw new IllegalArgumentException("Error: invalid SGDK field type: " + sgdkType);
        }

        public int size()
        {
            switch (this)
            {
                default:
                    return 1;
                case F16:
                case S16:
                case U16:
                    return 2;

                case F32:
                case S32:
                case U32:
                case STRING:
                case OBJECT:
                    return 4;
            }
        }

        public boolean accept(TiledObjectType type)
        {
            switch (this)
            {
                case U8:
                    return (type == TiledObjectType.INT) || (type == TiledObjectType.ENUM);
                case U16:
                    return (type == TiledObjectType.FLOAT) || (type == TiledObjectType.INT) || (type == TiledObjectType.ENUM);
                case U32:
                    return (type == TiledObjectType.FLOAT) || (type == TiledObjectType.INT) || (type == TiledObjectType.COLOR)
                            || (type == TiledObjectType.ENUM);
                case S8:
                    return (type == TiledObjectType.INT);
                case S16:
                    return (type == TiledObjectType.FLOAT) || (type == TiledObjectType.INT);
                case S32:
                    return (type == TiledObjectType.FLOAT) || (type == TiledObjectType.INT);
                case F16:
                    return (type == TiledObjectType.FLOAT) || (type == TiledObjectType.INT);
                case F32:
                    return (type == TiledObjectType.FLOAT) || (type == TiledObjectType.INT);
                case BOOL:
                    return (type == TiledObjectType.BOOL);
                case STRING:
                    return (type == TiledObjectType.STRING);
                case OBJECT:
                    return (type == TiledObjectType.OBJECT);
                default:
                    return false;
            }
        }

        @Override
        public String toString()
        {
            // important to keep everything lower case
            switch (this)
            {
                case U8:
                    return "u8";
                case U16:
                    return "u16";
                case U32:
                    return "u32";
                case S8:
                    return "s8";
                case S16:
                    return "s16";
                case S32:
                    return "s32";
                case F16:
                    return "f16";
                case F32:
                    return "f32";
                case BOOL:
                    return "bool";
                case STRING:
                    return "string";
                case OBJECT:
                    return "object";
                default:
                    return "";
            }
        }
    }

    final public String name;
    final public SGDKObjectType type;

    public SFieldDef(String name, SGDKObjectType type)
    {
        super();

        this.name = name;
        this.type = type;
    }

    public boolean isString()
    {
        return (type == SGDKObjectType.STRING);
    }

    public boolean isPointer()
    {
        return (type == SGDKObjectType.OBJECT);
    }

    public boolean isIntData()
    {
        return !isString() && !isPointer();
    }

    @Override
    public String toString()
    {
        return name + ":" + type;
    }
}
