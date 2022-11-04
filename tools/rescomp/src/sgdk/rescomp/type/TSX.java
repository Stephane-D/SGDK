package sgdk.rescomp.type;

import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;

import org.w3c.dom.Element;
import org.w3c.dom.Node;

import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.tool.FileUtil;
import sgdk.tool.ImageUtil;
import sgdk.tool.StringUtil;
import sgdk.tool.XMLUtil;

public class TSX
{
    static final String ID_TILESET = "tileset";
    static final String ID_IMAGE = "image";

    static final String ATTR_VERSION = "version";
    static final String ATTR_TILEDVERSION = "tiledversion";
    static final String ATTR_NAME = "name";
    static final String ATTR_TILEWIDTH = "tilewidth";
    static final String ATTR_TILEHEIGHT = "tileheight";
    static final String ATTR_WIDTH = "width";
    static final String ATTR_HEIGHT = "height";
    static final String ATTR_TILECOUNT = "tilecount";
    static final String ATTR_COLUMNS = "columns";
    static final String ATTR_SOURCE = "source";
    static final String ATTR_TRANS = "trans";

    // <?xml version="1.0" encoding="UTF-8"?>
    // <tileset version="1.2" tiledversion="2019.02.10" name="Bathing_Spot" tilewidth="16" tileheight="16"
    // tilecount="368" columns="23">
    // <image source="../tileset/Bathing_Spot.png" trans="e00080" width="368" height="256"/>
    // </tileset>

    public static class TSXTileset implements Comparable<TSXTileset>
    {
        public final String file;
        public final int startTileIndex;
        public int tileSize;
        public int imageTileWidth;
        public int imageTileHeigt;
        public int numTile;
        public int transparentColor;
        public String tilesetImagePath;

        public TSXTileset(String tmxFile, int startInd, Element tilesetElement) throws Exception
        {
            super();

            file = tmxFile;
            startTileIndex = startInd;

            load(tilesetElement);
        }

        public TSXTileset(String tsxFile, int startInd) throws Exception
        {
            if (!FileUtil.exists(tsxFile))
                throw new FileNotFoundException("TSX file '" + tsxFile + " not found !");

            file = tsxFile;
            startTileIndex = startInd;

            load(XMLUtil.getRootElement(XMLUtil.loadDocument(file)));
        }

        private void load(Element tilesetElement) throws Exception
        {
            // check this is the tileset node
            if (!tilesetElement.getNodeName().toLowerCase().equals(ID_TILESET))
                throw new Exception("Expected " + ID_TILESET + " root node in TSX file: " + file + ", " + tilesetElement.getNodeName() + " found.");

            final int tw = XMLUtil.getAttributeIntValue(tilesetElement, ATTR_TILEWIDTH, 0);
            final int th = XMLUtil.getAttributeIntValue(tilesetElement, ATTR_TILEHEIGHT, 0);

            if (tw != th)
                throw new Exception("Non square tile not supported (" + tw + " x " + th + ") in TSX file: " + file);
            if ((tw & 7) != 0)
                throw new Exception("Unsuported tile size (should be a multiple of 8) in TSX file: " + file);
            if ((tw < 8) || (tw > 32))
                throw new Exception(tw + " x " + th + " tile size not supported (only 8x8 to 32x32 allowed) in TSX file: " + file);

            tileSize = tw;

            final int w = XMLUtil.getAttributeIntValue(tilesetElement, ATTR_COLUMNS, 0);
            if (w == 0)
                throw new Exception("Tileset 'columns' information not found in TSX file: " + file);

            final Element imageElement = getElement(tilesetElement, ID_IMAGE, file);

            if (XMLUtil.getAttributeIntValue(imageElement, ATTR_WIDTH, 0) != (w * tileSize))
                throw new Exception("Image width (" + XMLUtil.getAttributeIntValue(imageElement, ATTR_WIDTH, 0) + ") do not match tileset size ("
                        + (w * tileSize) + ") in TSX file: " + file);

            imageTileWidth = w;
            imageTileHeigt = XMLUtil.getAttributeIntValue(imageElement, ATTR_HEIGHT, 0) / tileSize;

            // if ((imageTileWidth * imageTileHeigt) != XMLUtil.getAttributeIntValue(tilesetElement, ATTR_TILECOUNT, 0))
            // System.out.print("Warning: Tileset 'tilecount' do not match 'image size' / 'tile size' in TSX file: " +
            // file);

            numTile = imageTileWidth * imageTileHeigt;

            transparentColor = XMLUtil.getAttributeIntValue(imageElement, ATTR_TRANS, 0);
            tilesetImagePath = XMLUtil.getAttributeValue(imageElement, ATTR_SOURCE, "");
        }

        public boolean containsTile(int tileInd)
        {
            return (tileInd >= startTileIndex) && (tileInd < (startTileIndex + numTile));
        }

        public String getTilesetPath()
        {
            return Util.getAdjustedPath(tilesetImagePath, file);
        }

        // return optimized tilset
        public Tileset getTileset(String id, Compression compression, boolean temp) throws Exception
        {
            // always add a blank tile if not present for TSX tileset (Tiled does not count it)
            return Tileset.getTileset(id, getTilesetPath(), compression, TileOptimization.ALL, true, temp);
        }

        public byte[] getTilesetImage8bpp(boolean cropPalette) throws Exception
        {
            final byte[] result = ImageUtil.getImageAs8bpp(getTilesetPath(), true, cropPalette);

            // happen when we couldn't retrieve palette data from RGB image
            if (result == null)
                throw new IllegalArgumentException(
                        "RGB image '" + getTilesetPath() + "' does not contains palette data (see 'Important note about image format' in the rescomp.txt file");

            return result;
        }

        @Override
        public int hashCode()
        {
            return getTilesetPath().hashCode() ^ (numTile << 16) ^ (startTileIndex << 0) ^ tileSize ^ imageTileHeigt ^ imageTileWidth;
        }

        @Override
        public boolean equals(Object obj)
        {
            if (obj instanceof TSXTileset)
            {
                final TSXTileset tsxTileset = (TSXTileset) obj;

                return tsxTileset.getTilesetPath().equals(getTilesetPath()) && (tsxTileset.numTile == numTile) && (tsxTileset.startTileIndex == startTileIndex)
                        && (tsxTileset.tileSize == tileSize) && (tsxTileset.imageTileHeigt == imageTileHeigt) && (tsxTileset.imageTileWidth == imageTileWidth);
            }

            return super.equals(obj);
        }

        @Override
        public String toString()
        {
            return "Tileset '" + FileUtil.getFileName(tilesetImagePath) + " - tileSize=" + tileSize + " - startInd=" + startTileIndex + " - numTile=" + numTile;
        }

        @Override
        public int compareTo(TSXTileset t)
        {
            return Integer.compare(startTileIndex, t.startTileIndex);
        }
    }

    public static List<Tileset> getTilesets(List<TSXTileset> tsxTilesets, String baseId, Compression compression, boolean temp) throws Exception
    {
        final List<Tileset> tilesets = new ArrayList<>();

        int ind = 0;
        // special case where we have empty tileset
        if (tsxTilesets.isEmpty())
        {
            // create a tileset containing only a blank tile
            tilesets.add(new Tileset(baseId + "_tileset"));
        }
        else
        {
            for (TSXTileset tsxTileset : tsxTilesets)
                tilesets.add(tsxTileset.getTileset(baseId + "_tileset" + ind++, compression, temp));
        }

        return tilesets;
    }

    public static String getTSXTilesetPath(String file) throws Exception
    {
        return new TSXTileset(file, 0).getTilesetPath();
    }

    static Element getElement(Node node, String name, String file) throws Exception
    {
        final Element result = XMLUtil.getElement(node, name);

        if (result == null)
            throw new Exception("Cannot find " + name + " XML node in TSX file: " + file);

        return result;
    }

    static String getAttribute(Element element, String attrName, String def)
    {
        return XMLUtil.getAttributeValue(element, attrName, def).toLowerCase();
    }

    static void checkAttributValue(Element element, String attrName, String value, String def, String file) throws Exception
    {
        final String attrValue = getAttribute(element, attrName, def);

        if (!StringUtil.equals(attrValue, value))
            throw new Exception("'" + attrValue + "' " + attrName + " not supported (" + def + " expected) in TSX file: " + file);
    }
}
