package sgdk.rescomp.processor;

import java.util.LinkedHashMap;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Objects;
import sgdk.rescomp.type.SFieldDef.SGDKObjectType;
import sgdk.rescomp.type.TMX.TMXObjects;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class ObjectsProcessor implements Processor
{
    /**
     * Build the field definition map from a field definition string.
     * 
     * @return the field definition map (ordered)
     * @throws Exception
     */
    public static LinkedHashMap<String, SGDKObjectType> buildFieldDefsMap(String fieldDefs) throws Exception
    {
        // create the field definition map
        final LinkedHashMap<String, SGDKObjectType> result = new LinkedHashMap<>();
        final String[] defs = fieldDefs.split(";");

        for (String def : defs)
        {
            final String[] values = def.split(":");

            if (values.length != 2)
                throw new Exception("Error: invalid field definition '" + def + "', name and type should be separated by ':' character");

            final String fieldName = values[0].trim().toLowerCase();
            final String fieldType = values[1].trim().toLowerCase();

            if (StringUtil.isEmpty(fieldName))
                throw new Exception("Error: invalid field definition '" + def + "', field name is empty");
            if (StringUtil.isEmpty(fieldType))
                throw new Exception("Error: invalid field definition '" + def + "', field type is empty");
            if (!SGDKObjectType.isValid(fieldType))
                throw new Exception("Error: invalid field definition '" + def + "', unknow field type");

            // add field definition
            result.put(fieldName, SGDKObjectType.fromString(fieldType));
        }

        if (result.isEmpty())
            throw new Exception("Error: field definition cannot be empty");

        return result;
    }

    @Override
    public String getId()
    {
        return "OBJECTS";
    }

    @Override
    public Resource execute(String[] fields) throws Exception
    {
        if (fields.length < 5)
        {
            System.out.println("Wrong OBJECTS definition");
            System.out.println("OBJECTS name tmx_file layer_id field_defs [decl_type [type_filter]]");
            System.out.println("name            name of the output array Objects structure");
            System.out.println("tmx_file        path of the input TMX file (TMX Tiled file)");
            System.out.println("layer_id        object group/layer name we want to extract object from.");
            System.out.println("field_defs      name and type of fields we want to extract (also define the order of extraction) in <name>:<type> format.");
            System.out.println("                    Field type can be any of the usual SGDK base type: u8, s8, u16, s16, u32, s32, f16, f32, bool, string, object");
            System.out.println("                    Some examples of 'field_defs' declaration:");
            System.out.println("                        \"type:u16;name:string;x:f32;y:f32;tileindex:u32\"");
            System.out.println("                        \"type:u16;power:s8;visible:bool\"");
            System.out.println("                    Not found fields are just ignored so you can extract different object types easily.");
            System.out.println("decl_type       declaration type for objects (default is \"void\").");
            System.out.println("                    By default object array is untyped (void) but you can specify your own type if you want (\"Object\", \"Entity\", ...)");
            System.out.println("type_filter     define a type filter if we only want to extract Tiled objects of a specific type.");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input TMX file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get layer name
        final String layerName = fields[3];
        // get fields definition value
        final String fieldDefs = fields[4];
        String declType = "";
        if (fields.length >= 6)
            declType = fields[5];
        String typeFilter = "";
        if (fields.length >= 7)
            typeFilter = fields[6];

        final LinkedHashMap<String, SGDKObjectType> fieldDefsMap = buildFieldDefsMap(fieldDefs);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        // build TMX objects
        final TMXObjects tmxObjects = new TMXObjects(id, fileIn, layerName, fieldDefsMap, typeFilter);
        // build OBJECTS from TMX objects
        return new Objects(id, declType, tmxObjects.objects);
    }
}
