package sgdk.rescomp.type;

public class TFieldDef
{
    public static enum TiledObjectType
    {
        INT, FLOAT, BOOL, STRING, COLOR, FILE, OBJECT, ENUM;

        public static boolean isValid(String tiledType)
        {
            try
            {
                fromString(tiledType);
                return true;
            }
            catch (IllegalArgumentException e)
            {
                return false;
            }
        }

        public static TiledObjectType fromString(String tiledType)
        {
            final String t = tiledType.toLowerCase();

            for (TiledObjectType value : values())
                if (t.equals(value.toString().toLowerCase()))
                    return value;

            throw new IllegalArgumentException("Error: invalid Tiled field type: " + tiledType);
        }

        @Override
        public String toString()
        {
            switch (this)
            {
                case INT:
                    return "int";
                case FLOAT:
                    return "float";
                case BOOL:
                    return "bool";
                case STRING:
                    return "string";
                case COLOR:
                    return "color";
                case FILE:
                    return "file";
                case OBJECT:
                    return "object";
                case ENUM:
                    return "enum";
                default:
                    return "";
            }
        }
    }

    final String name;
    final TiledObjectType type;

    public TFieldDef(String name, TiledObjectType type)
    {
        super();

        this.name = name;
        this.type = type;
    }

    @Override
    public String toString()
    {
        return name + ":" + type;
    }
}
