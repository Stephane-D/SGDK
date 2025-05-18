package sgdk.rescomp.type;

import sgdk.rescomp.type.SFieldDef.SGDKObjectType;
import sgdk.tool.StringUtil;

public class TField extends TFieldDef
{
    final String value;

    public TField(String name, TiledObjectType type, String value)
    {
        super(name, type);

        this.value = value;
    }

    public SField toSField(String baseObjectName, SGDKObjectType sgdkType) throws Exception
    {
        if (!sgdkType.accept(type))
            throw new Exception("Error: cannot convert Tiled type '" + type + " to SGDK '" + sgdkType + "' type (not compatible)");

        String adjVal = value;

        // object type ? --> use the base object name + value (id) to get initial object name reference
        if (sgdkType == SGDKObjectType.OBJECT)
        {
            // NULL object pointer ? --> use special empty value string
            if (StringUtil.parseInt(value, 0) == 0)
                adjVal = "";
            else
                // temporarily save only the id of the reference object, since its base_name is not yet known for sure if it is an external object
                adjVal = value;            
        }

        return new SField(name, sgdkType, adjVal);
    }
    
    @Override
    public String toString()
    {
        return name + ":" + type + " = " + value;
    }
}
