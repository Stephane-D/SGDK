package sgdk.rescomp;

import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

import sgdk.rescomp.processor.AlignProcessor;
import sgdk.rescomp.processor.BinProcessor;
import sgdk.rescomp.processor.BitmapProcessor;
import sgdk.rescomp.processor.ImageProcessor;
import sgdk.rescomp.processor.MapProcessor;
import sgdk.rescomp.processor.NearProcessor;
import sgdk.rescomp.processor.ObjectsProcessor;
import sgdk.rescomp.processor.PaletteProcessor;
import sgdk.rescomp.processor.SpriteProcessor;
import sgdk.rescomp.processor.TilemapProcessor;
import sgdk.rescomp.processor.TilesetProcessor;
import sgdk.rescomp.processor.UngroupProcessor;
import sgdk.rescomp.processor.WavProcessor;
import sgdk.rescomp.processor.XgmProcessor;
import sgdk.rescomp.resource.Align;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.resource.Bitmap;
import sgdk.rescomp.resource.Near;
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
    private final static String EXT_JAR_NAME = "rescomp_ext.jar";
    // private final static String REGEX_LETTERS = "[a-zA-Z]";
    // private final static String REGEX_ID = "\\b([A-Za-z][A-Za-z0-9_]*)\\b";

    private final static List<Processor> resourceProcessors = new ArrayList<>();

    static
    {
        // function processors
        resourceProcessors.add(new AlignProcessor());
        resourceProcessors.add(new UngroupProcessor());
        resourceProcessors.add(new NearProcessor());

        // resource processors
        resourceProcessors.add(new BinProcessor());
        resourceProcessors.add(new PaletteProcessor());
        resourceProcessors.add(new BitmapProcessor());
        resourceProcessors.add(new TilesetProcessor());
        resourceProcessors.add(new TilemapProcessor());
        resourceProcessors.add(new MapProcessor());
        resourceProcessors.add(new ObjectsProcessor());
        resourceProcessors.add(new ImageProcessor());
        resourceProcessors.add(new SpriteProcessor());
        resourceProcessors.add(new WavProcessor());
        resourceProcessors.add(new XgmProcessor());
    }

    // shared directory informations
    public static String currentDir;
    public static String resDir;

    // map storing all resources (same object for key and value as we want fast 'contains' check)
    public final static Map<Resource, Resource> resources = new HashMap<>();
    // list to preserve order
    public final static List<Resource> resourcesList = new ArrayList<>();

    // set storing all resource paths
    public final static Set<String> resourcesFile = new HashSet<>();

    public static boolean extensionsLoaded = false;
    
    // TODO: set that to false on release 
    public static boolean DAGame = false;

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
            System.err.println("Couldn't open input file " + Paths.get(fileName).toAbsolutePath().toString() + ":");
            System.err.println(e.getMessage());
            return false;
        }

        int lineCnt = 1;
        int align = -1;
        boolean group = true;
        boolean near = false;

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
            // NEAR function (not a real resource so handle it specifically)
            else if (resource instanceof Near)
            {
                // enable forced near resource export
                near = true;
                System.out.println();
            }
            // just store resource
            else
            {
                addResource(resource);
                // show raw size
                System.out.println(" '" + resource.id + "' raw size: " + resource.totalSize() + " bytes");
                // show more infos for MAP type resource
                if (resource instanceof sgdk.rescomp.resource.Map)
                    System.out.println(resource.toString());
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

            // get "near" BIN resources
            final List<Bin> binResources = getBinResources(false);
            // get "far" BIN resources
            final List<Bin> farBinResources = getBinResources(true);

            // get "near" palette BIN resources (we always want them first and palette BIN data are never "far")
            final List<Bin> binResourcesOfPalette = getInternalBinResourcesOf(null, Palette.class, false);
            // grouped "near" internal bin resources
            final List<Bin> groupedInternalBinResources = new ArrayList<>();
            // grouped "far" internal bin resources
            final List<Bin> farGroupedInternalBinResources = new ArrayList<>();

            if (group)
            {
                // get "near" BIN resources grouped by type for better compression
                getInternalBinResourcesOf(groupedInternalBinResources, Tilemap.class, false);
                getInternalBinResourcesOf(groupedInternalBinResources, Tileset.class, false);
                getInternalBinResourcesOf(groupedInternalBinResources, Bitmap.class, false);
                getInternalBinResourcesOf(groupedInternalBinResources, sgdk.rescomp.resource.Map.class, false);

                // remove BIN palette resources (just for safety, we already had a tileset bin data = palette bin data)
                groupedInternalBinResources.removeAll(binResourcesOfPalette);

                // get "far" BIN resources grouped by type for better compression
                getInternalBinResourcesOf(farGroupedInternalBinResources, Tilemap.class, true);
                getInternalBinResourcesOf(farGroupedInternalBinResources, Tileset.class, true);
                getInternalBinResourcesOf(farGroupedInternalBinResources, Bitmap.class, true);
                getInternalBinResourcesOf(farGroupedInternalBinResources, sgdk.rescomp.resource.Map.class, true);
            }

            // remove grouped and Palette "near" BIN resources
            binResources.removeAll(binResourcesOfPalette);
            binResources.removeAll(groupedInternalBinResources);
            // remove grouped "far" BIN resources
            farBinResources.removeAll(farGroupedInternalBinResources);

            // get all non BIN resources
            final List<Resource> nonBinResources = getNonBinResources();
            // get non BIN resources for VDPSprite and Collision (they can be compressed as binary data doesn't store any pointer/reference)
            final List<Resource> vdpSpriteResources = getResources(VDPSprite.class);
            final List<Resource> collisionResources = getResources(Collision.class);

            // remove exported resources
            nonBinResources.removeAll(vdpSpriteResources);
            nonBinResources.removeAll(collisionResources);

            // export binary data first !! very important !!
            // otherwise metadata structures can't properly get BIN.doneCompression field value

            // Read Only Data section
            outS.append(".section .rodata\n\n");
            // reset binary buffer
            outB.reset();

            // export simple and safe "metadata" resources which can be compressed first
            exportResources(vdpSpriteResources, outB, outS, outH);
            exportResources(collisionResources, outB, outS, outH);

            // BIN Read Only Data section
            outS.append(".section .rodata_bin\n\n");
            // need to reset binary buffer
            outB.reset();

            // then export "not far" BIN resources by type for better compression
            exportResources(binResourcesOfPalette, outB, outS, outH);
            exportResources(binResources, outB, outS, outH);
            exportResources(groupedInternalBinResources, outB, outS, outH);

            // FAR BIN Read Only Data section if NEAR not enabled
            if (!near)
            {
                outS.append(".section .rodata_binf\n\n");
                // need to reset binary buffer
                outB.reset();
            }

            // do alignment for FAR BIN data
            if (align != -1)
            {
                outS.append("    .align  " + align + "\n\n");
                // need to reset binary buffer
                outB.reset();
            }

            // then export "far" BIN resources by type for better compression
            exportResources(farBinResources, outB, outS, outH);
            exportResources(farGroupedInternalBinResources, outB, outS, outH);

            // Read Only Data section
            outS.append(".section .rodata\n\n");
            // need to reset binary buffer
            outB.reset();

            // and finally export "metadata" (non BIN) *after* binary data (no compression possible)
            exportResources(nonBinResources, outB, outS, outH);

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
                System.out.println("  Packed: " + packedSize + " bytes (" + Math.round((packedSize * 100f) / packedRawSize) + "% - origin size: "
                        + packedRawSize + " bytes)");

            int spriteMetaSize = 0;

            // get all sprite related resources
            final List<Resource> allSpritesResources = getResources(Sprite.class);
            allSpritesResources.addAll(vdpSpriteResources);
            allSpritesResources.addAll(collisionResources);
            allSpritesResources.addAll(getResources(SpriteFrame.class));
            allSpritesResources.addAll(getResources(SpriteAnimation.class));

            // compute SPRITE structures size
            for (Resource res : allSpritesResources)
                spriteMetaSize += res.shallowSize();

            if (spriteMetaSize > 0)
                System.out.println("Sprite metadata (all but tiles and palette data): " + spriteMetaSize + " bytes");

            int miscMetaSize = 0;

            // keep all others non BIN resources (non sprite related)
            nonBinResources.removeAll(allSpritesResources);

            // compute misc structures size
            for (Resource res : nonBinResources)
                miscMetaSize += res.shallowSize();

            if (miscMetaSize > 0)
                System.out.println("Misc metadata (map, bitmap, image, tilemap, tileset, palette..): " + miscMetaSize + " bytes");

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

    private static void loadExtensions() throws IOException
    {
        final File rescompExt = StringUtil.isEmpty(resDir) ? new File(EXT_JAR_NAME) : new File(resDir, EXT_JAR_NAME);

        // found an extension ?
        if (rescompExt.exists())
        {
            // build the class loader
            final URLClassLoader classLoader = new URLClassLoader(new URL[] {rescompExt.toURI().toURL()}, Compiler.class.getClassLoader());

            try
            {
                // get all classes from JAR file
                for (String className : findClassNamesInJAR(rescompExt.getAbsolutePath()))
                {
                    try
                    {
                        // try to load class
                        final Class<?> clazz = classLoader.loadClass(className);

                        try
                        {
                            // is a processor class ?
                            final Class<? extends Processor> processorClass = clazz.asSubclass(Processor.class);
                            // create the processor
                            final Processor processor = processorClass.newInstance();

                            // and add to processor list
                            resourceProcessors.add(processor);

                            System.out.println("Extension '" + processor.getId() + "' loaded.");
                        }
                        catch (Throwable t)
                        {
                            // not a processor --> ignore
                        }
                    }
                    catch (UnsupportedClassVersionError e)
                    {
                        System.err.println("Class '" + className + "' cannot be loaded: newer java required.");
                    }
                    catch (Throwable t)
                    {
                        System.err.println("Class '" + className + "' cannot be loaded:" + t.getMessage());
                    }
                }
            }
            finally
            {
                classLoader.close();
            }
        }
    }

    /**
     * This method checks and transforms the filename of a potential {@link Class} given by <code>fileName</code>.<br>
     * 
     * @param fileName
     *        is the filename.
     * @return the according Java {@link Class#getName() class-name} for the given <code>fileName</code> if it is a
     *         class-file that is no anonymous {@link Class}, else <code>null</code>.
     */
    private static String fixClassName(String fileName)
    {
        // replace path separator by package separator
        String result = fileName.replace('/', '.');

        // handle inner classes...
        final int lastDollar = result.lastIndexOf('$');
        if (lastDollar > 0)
        {
            char innerChar = result.charAt(lastDollar + 1);
            // ignore anonymous inner class
            if ((innerChar >= '0') && (innerChar <= '9'))
                return null;
        }

        return result;
    }

    /**
     * This method checks and transforms the filename of a potential {@link Class} given by <code>fileName</code>.
     * 
     * @param fileName
     *        is the filename.
     * @return the according Java {@link Class#getName() class-name} for the given <code>fileName</code> if it is a
     *         class-file that is no anonymous {@link Class}, else <code>null</code>.
     */
    private static String filenameToClassname(String fileName)
    {
        // class file ?
        if (fileName.toLowerCase().endsWith(".class"))
            // remove ".class" extension and fix classname
            return fixClassName(fileName.substring(0, fileName.length() - 6));

        return null;
    }

    private static void addClassFileName(String fileName, Set<String> classSet, String prefix)
    {
        final String simpleClassName = filenameToClassname(fileName);

        if (simpleClassName != null)
            classSet.add(prefix + simpleClassName);
    }

    /**
     * Search for all classes in JAR file
     * 
     * @throws IOException
     */
    private static Set<String> findClassNamesInJAR(String fileName) throws IOException
    {
        final Set<String> classes = new HashSet<>();

        try (final JarFile jarFile = new JarFile(fileName))
        {
            final Enumeration<JarEntry> entries = jarFile.entries();

            while (entries.hasMoreElements())
            {
                final JarEntry jarEntry = entries.nextElement();

                if (!jarEntry.isDirectory())
                    addClassFileName(jarEntry.getName(), classes, "");
            }
        }

        return classes;
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

    public static List<Resource> getResources(Class<? extends Resource> resourceType)
    {
        final List<Resource> result = new ArrayList<>();

        for (Resource resource : resourcesList)
            if ((resourceType == null) || resourceType.isInstance(resource))
                result.add(resource);

        return result;
    }

    private static List<Resource> getNonBinResources()
    {
        final List<Resource> result = new ArrayList<>();

        for (Resource resource : resourcesList)
            if (!Bin.class.isInstance(resource))
                result.add(resource);

        return result;
    }

    private static List<Bin> getBinResources(boolean far)
    {
        final List<Bin> result = new ArrayList<>();

        for (Resource resource : resourcesList)
        {
            if (Bin.class.isInstance(resource) && (((Bin) resource).far == far))
                result.add((Bin) resource);
        }

        return result;
    }

    private static List<Bin> getInternalBinResourcesOf(List<Bin> dest, Class<? extends Resource> from, boolean far)
    {
        final List<Bin> result;

        if (dest != null)
            result = dest;
        else
            result = new ArrayList<>();

        // special case of Map resource which has several binary blob that we want to group :)
        if (from == sgdk.rescomp.resource.Map.class)
        {
            final List<Resource> mapResources = getResources(from);

            // metatiles bin
            for (Resource resource : mapResources)
            {
                final sgdk.rescomp.resource.Map map = (sgdk.rescomp.resource.Map) resource;

                if ((map.metatilesBin.far == far) && !result.contains(map.metatilesBin))
                    result.add(map.metatilesBin);
            }
            // mapBlockIndexes bin
            for (Resource resource : mapResources)
            {
                final sgdk.rescomp.resource.Map map = (sgdk.rescomp.resource.Map) resource;

                if ((map.mapBlockIndexesBin.far == far) && !result.contains(map.mapBlockIndexesBin))
                    result.add(map.mapBlockIndexesBin);
            }
            // mapBlockRowOffsets bin
            for (Resource resource : mapResources)
            {
                final sgdk.rescomp.resource.Map map = (sgdk.rescomp.resource.Map) resource;

                if ((map.mapBlockRowOffsetsBin.far == far) && !result.contains(map.mapBlockRowOffsetsBin))
                    result.add(map.mapBlockRowOffsetsBin);
            }
            // mapBlocks bin
            for (Resource resource : mapResources)
            {
                final sgdk.rescomp.resource.Map map = (sgdk.rescomp.resource.Map) resource;

                if ((map.mapBlocksBin.far == far) && !result.contains(map.mapBlocksBin))
                    result.add(map.mapBlocksBin);
            }
        }
        else
        {
            for (Resource resource : getResources(from))
            {
                for (Bin bin : resource.getInternalBinResources())
                {
                    if ((bin.far == far) && !result.contains(bin))
                        result.add(bin);
                }
            }
        }

        return result;
    }

    private static void exportResources(Collection<? extends Resource> resourceCollection, ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH)
            throws IOException
    {
        for (Resource res : resourceCollection)
            exportResource(res, outB, outS, outH);
    }

    private static void exportResource(Resource resource, ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        resource.out(outB, outS, outH);
    }

    public static Resource findResource(Resource resource)
    {
        return resources.get(resource);
    }

    public static Resource addResource(Resource resource, boolean internal)
    {
        // internal resource ?
        if (internal)
        {
            // check if we already have this resource
            final Resource result = findResource(resource);

            // return it if already exists
            if (result != null)
            {
                System.out.println("Info: '" + resource.id + "' has same content as '" + result.id + "'");
                return result;
            }

            // mark as not global (internal)
            resource.global = false;
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
        if (StringUtil.equals(id, "NULL"))
            return null;

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
        catch (Exception e)
        {
            System.err.println();
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

        // not found ? --> try to load extensions
        if (!extensionsLoaded)
        {
            extensionsLoaded = true;

            try
            {
                loadExtensions();
            }
            catch (IOException e)
            {
                System.err.println("Cannot load extension:" + e.getMessage());
            }

            // and try again
            for (Processor rp : resourceProcessors)
                if (resourceType.equalsIgnoreCase(rp.getId()))
                    return rp;
        }
        
        return null;
    }
}
