package sgdk.rescomp;

import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import sgdk.rescomp.processor.AlignProcessor;
import sgdk.rescomp.processor.BinProcessor;
import sgdk.rescomp.processor.BitmapProcessor;
import sgdk.rescomp.processor.ImageProcessor;
import sgdk.rescomp.processor.MapProcessor;
import sgdk.rescomp.processor.PaletteProcessor;
import sgdk.rescomp.processor.SpriteProcessor;
import sgdk.rescomp.processor.TilesetProcessor;
import sgdk.rescomp.processor.UngroupProcessor;
import sgdk.rescomp.processor.WavProcessor;
import sgdk.rescomp.processor.XgmProcessor;
import sgdk.rescomp.resource.Align;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.resource.Bitmap;
import sgdk.rescomp.resource.Image;
import sgdk.rescomp.resource.Palette;
import sgdk.rescomp.resource.Sprite;
import sgdk.rescomp.resource.Tilemap;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.resource.Ungroup;
import sgdk.rescomp.resource.internal.Collision;
import sgdk.rescomp.resource.internal.SpriteAnimation;
import sgdk.rescomp.resource.internal.SpriteFrame;
import sgdk.rescomp.resource.internal.VDPSprite;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;

public class Compiler
{
    // private final static String REGEX_LETTERS = "[a-zA-Z]";
    // private final static String REGEX_ID = "\\b([A-Za-z][A-Za-z0-9_]*)\\b";

    private final static List<Processor> resourceProcessors = new ArrayList<>();

    static
    {
        // function processors
        resourceProcessors.add(new AlignProcessor());
        resourceProcessors.add(new UngroupProcessor());

        // resource processors
        resourceProcessors.add(new BinProcessor());
        resourceProcessors.add(new PaletteProcessor());
        resourceProcessors.add(new BitmapProcessor());
        resourceProcessors.add(new TilesetProcessor());
        resourceProcessors.add(new MapProcessor());
        resourceProcessors.add(new ImageProcessor());
        resourceProcessors.add(new SpriteProcessor());
        resourceProcessors.add(new WavProcessor());
        resourceProcessors.add(new XgmProcessor());
    }

    // shared directory informations
    public static String currentDir;
    public static String resDir;

    // map storing all resources (same object for key and value as we want fast 'contains' check)
    public static Map<Resource, Resource> resources = new HashMap<>();
    // list to preserve order
    public static List<Resource> resourcesList = new ArrayList<>();

    // set storing all resource paths
    public static Set<String> resourcesFile = new HashSet<>();

    public static boolean compile(String fileName, String fileNameOut, boolean header, String depTarget)
    {
        // get application directory
        // currentDir = new File("").getAbsolutePath();
        currentDir = FileUtil.getApplicationDirectory();
        // get input file directory
        resDir = FileUtil.getDirectory(fileName);

        // reset resources / files lists
        resources.clear();
        resourcesList.clear();
        resourcesFile.clear();

        List<String> lines = null;

        try
        {
            // get all lines from resource file
            lines = Files.readAllLines(Paths.get(fileName), Charset.defaultCharset());
        }
        catch (IOException e)
        {
            System.err.println("Couldn't open input file " + fileName + ":");
            System.err.println(e.getMessage());
            return false;
        }

        int lineCnt = 1;
        int align = -1;
        boolean group = true;

        // process input resource file line by line
        for (String l : lines)
        {
            // cleanup the text
            final String line = l.trim().replaceAll("\t", " ").replaceAll(" +", " ");
            // count line
            lineCnt++;

            // ignore empty line
            if (StringUtil.isEmpty(line))
                continue;
            // ignore comment
            if (line.startsWith("//") || line.startsWith("#"))
                continue;

            // execute and get resource
            final Resource resource = execute(line);

            // can't get resource ? --> error happened, stop here..
            if (resource == null)
            {
                System.err.println(fileName + ": error on line " + lineCnt);
                return false;
            }

            // ALIGN function (not a real resource so handle it specifically)
            if (resource instanceof Align)
            {
                align = ((Align) resource).align;
                System.out.println();
            }
            // UNGROUP function (not a real resource so handle it specifically)
            else if (resource instanceof Ungroup)
            {
                // disable resource export grouping by type
                group = false;
                System.out.println();
            }
            // just store resource
            else
            {
                addResource(resource);
                System.out.println("'" + resource.id + "' raw size: " + resource.totalSize() + " bytes");
            }
        }

        // separate output
        System.out.println();

        // define output files
        final StringBuilder outS = new StringBuilder(1024);
        final StringBuilder outH = new StringBuilder(1024);

        try
        {
            final ByteArrayOutputStream outB = new ByteArrayOutputStream();

            // build header name from resource parent folder name
            String headerName = FileUtil.getFileName(FileUtil.getDirectory(resDir, false), false);
            if (StringUtil.isEmpty(headerName))
                headerName = "RES";
            headerName += "_" + FileUtil.getFileName(fileNameOut, false);
            headerName = headerName.toUpperCase();

            outH.append("#ifndef _" + headerName + "_H_\n");
            outH.append("#define _" + headerName + "_H_\n\n");

            // -- BINARY SECTION --

            // get BIN resources
            final List<Resource> binResources = getResources(Bin.class);
            final List<Resource> binResourcesOfPalette = getBinResourcesOf(Palette.class);
            final List<Resource> binResourcesOfTilemap;
            final List<Resource> binResourcesOfTileset;
            final List<Resource> binResourcesOfBitmap;
            final List<Resource> binResourcesOfMap;

            // keep raw BIN resources only
            binResources.removeAll(binResourcesOfPalette);

            if (group)
            {
                // get BIN resources grouped by type for better compression
                binResourcesOfTilemap = getBinResourcesOf(Tilemap.class);
                binResourcesOfTileset = getBinResourcesOf(Tileset.class);
                binResourcesOfBitmap = getBinResourcesOf(Bitmap.class);
                binResourcesOfMap = getBinResourcesOf(sgdk.rescomp.resource.Map.class);

                // keep raw BIN resources only
                binResources.removeAll(binResourcesOfTilemap);
                binResources.removeAll(binResourcesOfTileset);
                binResources.removeAll(binResourcesOfBitmap);
                binResources.removeAll(binResourcesOfMap);
            }
            else
            {
                binResourcesOfTilemap = new ArrayList<>();
                binResourcesOfTileset = new ArrayList<>();
                binResourcesOfBitmap = new ArrayList<>();
                binResourcesOfMap = new ArrayList<>();
            }

            // get "far" BIN resources (palette BIN data are never far)
            final List<Resource> farBinResources = getFarBinResourcesOf(binResources);
            final List<Resource> farBinResourcesOfTilemap;
            final List<Resource> farBinResourcesOfTileset;
            final List<Resource> farBinResourcesOfBitmap;
            final List<Resource> farBinResourcesOfMap;

            // keep "non far" BIN resources only
            binResources.removeAll(farBinResources);

            if (group)
            {
                // get "far" BIN resources grouped by type for better compression
                farBinResourcesOfTilemap = getFarBinResourcesOf(binResourcesOfTilemap);
                farBinResourcesOfTileset = getFarBinResourcesOf(binResourcesOfTileset);
                farBinResourcesOfBitmap = getFarBinResourcesOf(binResourcesOfBitmap);
                farBinResourcesOfMap = getFarBinResourcesOf(binResourcesOfMap);

                // keep "non far" BIN resources only
                binResourcesOfTilemap.removeAll(farBinResourcesOfTilemap);
                binResourcesOfTileset.removeAll(farBinResourcesOfTileset);
                binResourcesOfBitmap.removeAll(farBinResourcesOfBitmap);
                binResourcesOfMap.removeAll(farBinResourcesOfMap);
            }
            else
            {
                farBinResourcesOfTilemap = new ArrayList<>();
                farBinResourcesOfTileset = new ArrayList<>();
                farBinResourcesOfBitmap = new ArrayList<>();
                farBinResourcesOfMap = new ArrayList<>();
            }

            // export binary data first !! very important !!
            // otherwise metadata structures can't properly get BIN.doneCompression field value

            // Read Only Data section
            outS.append(".section .rodata\n\n");
            // reset binary buffer
            outB.reset();

            // export simple and safe "metadata" resources which can be compressed (binary data without reference)
            exportResources(getResources(VDPSprite.class), outB, outS, outH);
            exportResources(getResources(Collision.class), outB, outS, outH);

            // BIN Read Only Data section
            outS.append(".section .rodata_bin\n\n");
            // need to reset binary buffer
            outB.reset();

            // then export "not far" BIN resources by type for better compression
            exportResources(binResourcesOfPalette, outB, outS, outH);
            exportResources(binResources, outB, outS, outH);
            exportResources(binResourcesOfTilemap, outB, outS, outH);
            exportResources(binResourcesOfTileset, outB, outS, outH);
            exportResources(binResourcesOfBitmap, outB, outS, outH);
            exportResources(binResourcesOfMap, outB, outS, outH);

            // FAR BIN Read Only Data section
            outS.append(".section .rodata_binf\n\n");
            // need to reset binary buffer
            outB.reset();

            // do alignment for FAR BIN data
            if (align != -1)
                outS.append("    .align  " + align + "\n\n");

            // then export "far" BIN resources by type for better compression
            exportResources(farBinResources, outB, outS, outH);
            exportResources(farBinResourcesOfTilemap, outB, outS, outH);
            exportResources(farBinResourcesOfTileset, outB, outS, outH);
            exportResources(farBinResourcesOfBitmap, outB, outS, outH);
            exportResources(farBinResourcesOfMap, outB, outS, outH);

            // Read Only Data section
            outS.append(".section .rodata\n\n");
            // need to reset binary buffer
            outB.reset();

            // and finally export "metadata" *after* binary data (no compression possible)
            exportResources(getResources(Palette.class), outB, outS, outH);
            exportResources(getResources(Tileset.class), outB, outS, outH);
            exportResources(getResources(Tilemap.class), outB, outS, outH);
            exportResources(getResources(SpriteFrame.class), outB, outS, outH);
            exportResources(getResources(SpriteAnimation.class), outB, outS, outH);
            exportResources(getResources(Sprite.class), outB, outS, outH);
            exportResources(getResources(Image.class), outB, outS, outH);
            exportResources(getResources(Bitmap.class), outB, outS, outH);
            exportResources(getResources(sgdk.rescomp.resource.Map.class), outB, outS, outH);

            outH.append("\n");
            outH.append("#endif // _" + headerName + "_H_\n");

            int unpackedSize = 0;
            int packedRawSize = 0;
            int packedSize = 0;

            // compute global BIN sizes
            for (Resource res : getResources(Bin.class))
            {
                final Bin bin = (Bin) res;

                // compressed ?
                if (bin.doneCompression != Compression.NONE)
                {
                    packedRawSize += bin.data.length + (bin.data.length & 1);
                    packedSize += bin.packedData.data.length + (bin.packedData.data.length & 1);
                }
                else
                    unpackedSize += bin.data.length + (bin.data.length & 1);
            }

            System.out.println();
            System.out.println(fileName + " summary:");
            System.out.println("-------------");

            System.out.println("Binary data: " + (unpackedSize + packedSize) + " bytes");
            if (unpackedSize > 0)
                System.out.println("  Unpacked: " + unpackedSize + " bytes");
            if (packedSize > 0)
                System.out.println(
                        "  Packed: " + packedSize + " bytes (" + Math.round((packedSize * 100f) / packedRawSize)
                                + "% - origin size: " + packedRawSize + " bytes)");

            int spriteMetaSize = 0;
            int miscMetaSize = 0;

            // has sprites ?
            if (!getResources(Sprite.class).isEmpty())
            {
                // compute SPRITE structures size
                for (Resource res : getResources(VDPSprite.class))
                    spriteMetaSize += res.shallowSize();
                for (Resource res : getResources(Collision.class))
                    spriteMetaSize += res.shallowSize();
                for (Resource res : getResources(SpriteFrame.class))
                    spriteMetaSize += res.shallowSize();
                for (Resource res : getResources(SpriteAnimation.class))
                    spriteMetaSize += res.shallowSize();
                for (Resource res : getResources(Sprite.class))
                    spriteMetaSize += res.shallowSize();

                System.out.println("Sprite metadata (all but tiles and palette data): " + spriteMetaSize + " bytes");
            }

            // compute misc structures size
            for (Resource res : getResources(Bitmap.class))
                miscMetaSize += res.shallowSize();
            for (Resource res : getResources(Image.class))
                miscMetaSize += res.shallowSize();
            for (Resource res : getResources(Tilemap.class))
                miscMetaSize += res.shallowSize();
            for (Resource res : getResources(Tileset.class))
                miscMetaSize += res.shallowSize();
            for (Resource res : getResources(Palette.class))
                miscMetaSize += res.shallowSize();

            if (miscMetaSize > 0)
                System.out.println(
                        "Misc metadata (bitmap, image, tilemap, tileset, palette..): " + miscMetaSize + " bytes");

            final int totalSize = unpackedSize + packedSize + spriteMetaSize + miscMetaSize;
            System.out.println("Total: " + totalSize + " bytes (" + Math.round(totalSize / 1024d) + " KB)");
        }
        catch (Throwable t)
        {
            System.err.println(t.getMessage());
            t.printStackTrace();
            return false;
        }

        BufferedWriter out;

        try
        {
            // save .s file
            out = new BufferedWriter(new FileWriter(fileNameOut));
            out.write(outS.toString());
            out.close();
            // save .h file
            if (header)
            {
                out = new BufferedWriter(new FileWriter(FileUtil.setExtension(fileNameOut, ".h")));
                out.write(outH.toString());
                out.close();
            }
            // generate deps file if asked
            if (depTarget != null)
            {
                // save .d file
                out = new BufferedWriter(new FileWriter(FileUtil.setExtension(fileNameOut, ".d")));
                out.write(generateDependency(fileName, depTarget));
                out.close();
            }
        }
        catch (IOException e)
        {
            System.err.println("Couldn't create output file:");
            System.err.println(e.getMessage());
            return false;
        }

        return true;
    }

    private static String getFixedPath(String path)
    {
        return FileUtil.getGenericPath(path).replace(" ", "\\ ");
    }

    /**
     * Generate the content of the dependency list file.
     *
     * @param resFileName
     *        Name and proper path of current resource file
     * @param targetFileName
     *        Name of the file we want to generate a dependency list
     * @return String containing generated dependency list file
     */
    private static String generateDependency(String resFileName, String targetFileName)
    {
        String result = getFixedPath(resFileName);

        for (String fileName : resourcesFile)
            result += " \\\n" + getFixedPath(fileName);

        return getFixedPath(targetFileName) + ": " + result;
    }

    private static List<Resource> getFarBinResourcesOf(List<Resource> resourceList)
    {
        final List<Resource> result = new ArrayList<>();

        for (Resource resource : resourceList)
            if (resource instanceof Bin)
                if (((Bin) resource).far)
                    result.add(resource);

        return result;
    }

    private static List<Resource> getBinResourcesOf(Class<? extends Resource> resourceType)
    {
        final List<Resource> result = new ArrayList<>();
        final List<Resource> typeResources = getResources(resourceType);

        if (resourceType.equals(Palette.class))
        {
            for (Resource resource : typeResources)
            {
                final Bin binResource = ((Palette) resource).bin;

                if (!result.contains(binResource))
                    result.add(binResource);
            }
        }
        else if (resourceType.equals(Bitmap.class))
        {
            for (Resource resource : typeResources)
            {
                final Bin binResource = ((Bitmap) resource).bin;

                if (!result.contains(binResource))
                    result.add(binResource);
            }
        }
        else if (resourceType.equals(Tileset.class))
        {
            for (Resource resource : typeResources)
            {
                final Bin binResource = ((Tileset) resource).bin;

                if (!result.contains(binResource))
                    result.add(binResource);
            }
        }
        else if (resourceType.equals(Tilemap.class))
        {
            for (Resource resource : typeResources)
            {
                final Bin binResource = ((Tilemap) resource).bin;

                if (!result.contains(binResource))
                    result.add(binResource);
            }
        }
        else if (resourceType.equals(sgdk.rescomp.resource.Map.class))
        {
            for (Resource resource : typeResources)
            {
                final Bin binResourceMT = ((sgdk.rescomp.resource.Map) resource).metatilesBin;
                final Bin binResourceMB = ((sgdk.rescomp.resource.Map) resource).mapBlocksBin;
                final Bin binResourceMBI = ((sgdk.rescomp.resource.Map) resource).mapBlockIndexesBin;
                final Bin binResourceMBRO = ((sgdk.rescomp.resource.Map) resource).mapBlockRowOffsetsBin;

                if (!result.contains(binResourceMT))
                    result.add(binResourceMT);
                if (!result.contains(binResourceMB))
                    result.add(binResourceMB);
                if (!result.contains(binResourceMBI))
                    result.add(binResourceMBI);
                if (!result.contains(binResourceMBRO))
                    result.add(binResourceMBRO);
            }
        }
        else
            throw new IllegalArgumentException(
                    "getBinResourcesOf(..) error: " + resourceType.getName() + " class type not expected !");

        return result;
    }

    private static List<Resource> getResources(Class<? extends Resource> resourceType)
    {
        final List<Resource> result = new ArrayList<>();

        for (Resource resource : resourcesList)
            if (resourceType.isInstance(resource))
                result.add(resource);

        return result;
    }

    private static void exportResources(Collection<Resource> resourceCollection, ByteArrayOutputStream outB,
            StringBuilder outS, StringBuilder outH) throws IOException
    {
        for (Resource res : resourceCollection)
            exportResource(res, outB, outS, outH);
    }

    private static void exportResource(Resource resource, ByteArrayOutputStream outB, StringBuilder outS,
            StringBuilder outH) throws IOException
    {
        resource.out(outB, outS, outH);
    }

    public static Resource addResource(Resource resource, boolean internal)
    {
        // internal resource ?
        if (internal)
        {
            // mark as not global
            resource.global = false;

            // check if we already have this resource
            final Resource result = resources.get(resource);

            // return it if already exists
            if (result != null)
            {
                // System.out.println("Duplicated resource found: " + resource.id + " = " + result.id);
                return result;
            }
        }

        // add resource
        resources.put(resource, resource);
        resourcesList.add(resource);

        return resource;
    }

    public static Resource addResource(Resource resource)
    {
        return addResource(resource, false);
    }

    public static void addResourceFile(String file)
    {
        resourcesFile.add(file);
    }

    public static Resource getResourceById(String id)
    {
        for (Resource resource : resourcesList)
            if (resource.id.equals(id))
                return resource;

        return null;
    }

    private static Resource execute(String input)
    {
        try
        {
            // regex pattern to properly separate quoted string
            final String pattern = " +(?=(([^\"]*\"){2})*[^\"]*$)";
            // split on space character and preserve quoted parts
            final String[] fields = input.split(pattern);
            // remove quotes
            for (int f = 0; f < fields.length; f++)
                fields[f] = fields[f].replaceAll("\"", "");

            // get resource type
            final String type = fields[0];
            // get resource processor for this type
            final Processor processor = getResourceProcessor(type);

            // can't find matching processor ? --> invalid resource type
            if (processor == null)
            {
                System.err.println("Error: unknown resource type '" + type + "'");
                System.out.println("Accepted resource types are:");

                for (Processor rp : resourceProcessors)
                    System.out.println("  " + rp.getId());

                return null;
            }

            System.out.println("Resource: " + input);
            System.out.print("--> executing plugin " + type + "...");

            return processor.execute(fields);
        }
        catch (IOException e)
        {
            System.out.println();
            System.err.println(e.getMessage());
            System.err.println("Error: cannot compile resource '" + input + "'");
            e.printStackTrace();
            return null;
        }
        catch (IllegalArgumentException e)
        {
            System.out.println();
            System.err.println(e.getMessage());
            System.err.println("Error: cannot compile resource '" + input + "'");
            e.printStackTrace();
            return null;
        }
    }

    private static Processor getResourceProcessor(String resourceType)
    {
        for (Processor rp : resourceProcessors)
            if (resourceType.equalsIgnoreCase(rp.getId()))
                return rp;

        return null;
    }
}
