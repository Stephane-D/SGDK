package sgdk.rescomp.type;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.w3c.dom.Element;
import org.w3c.dom.Node;

import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.TSX.TSXTileset;
import sgdk.tool.StringUtil;
import sgdk.tool.XMLUtil;

public class TMX
{
    static final String ID_MAP = "map";
    static final String ID_TILESET = "tileset";
    static final String ID_LAYER = "layer";
    static final String ID_DATA = "data";
    static final String ID_OBJECTGROUP = "objectgroup";
    static final String ID_OBJECT = "object";
    static final String ID_TEXT = "text";

    static final String ATTR_ID = "id";
    static final String ATTR_NAME = "name";
    static final String ATTR_VERSION = "version";
    static final String ATTR_TILEDVERSION = "tiledversion";
    static final String ATTR_ORIENTATION = "orientation";
    static final String ATTR_RENDERORDER = "renderorder";
    static final String ATTR_WIDTH = "width";
    static final String ATTR_HEIGHT = "height";
    static final String ATTR_TILEWIDTH = "tilewidth";
    static final String ATTR_TILEHEIGHT = "tileheight";
    static final String ATTR_INFINITE = "infinite";
    static final String ATTR_SOURCE = "source";
    static final String ATTR_FIRSTGID = "firstgid";
    static final String ATTR_ENCODING = "encoding";
    static final String ATTR_VISIBLE = "visible";
    static final String ATTR_TYPE = "type";
    static final String ATTR_X = "x";
    static final String ATTR_Y = "y";

    static final String ATTR_VALUE_ORTHOGONAL = "orthogonal";
    static final String ATTR_VALUE_RIGHT_DOWN = "right-down";
    static final String ATTR_VALUE_CSV = "csv";
    static final String ATTR_VALUE_TEXT = "text";

    static final String SUFFIX_PRIORITY = " priority";
    static final String SUFFIX_LOW_PRIORITY = " low";
    static final String SUFFIX_HIGH_PRIORITY = " high";

    static final int TMX_HFLIP = (1 << 31);
    static final int TMX_VFLIP = (1 << 30);
    static final int TMX_AXE_FLIP = (1 << 29);

    // <map version="1.8" tiledversion="1.8.4" orientation="orthogonal" renderorder="right-down" width="40" height="14"
    // tilewidth="16" tileheight="16" infinite="0" backgroundcolor="#e00080" nextlayerid="15" nextobjectid="6">
    // <tileset firstgid="1" source="../tsx/Forest.tsx"/>
    // <tileset firstgid="392" source="../tsx/Bathing_Spot.tsx"/>
    // <layer id="1" name="B Background" width="40" height="14">
    // <data encoding="csv">
    // 715,715,715,715,715,715,715,604,604,604,604,604,715,715,715,604,604,604,744,124,125,126,2147483774,2147483773,2147483774,125,126,126,2147483774,2147483773,2147483772,124,125,126,2147483774,2147483773,2147483772,124,125,126
    // </data>
    // </layer>
    // <objectgroup id="7" name="_DEV_Comments" visible="0" locked="1">
    // <object id="4" name="Beast" type="Text" x="239.333" y="-172.167" width="147" height="30">
    // <text wrap="1">Beasts</text>
    // </object>
    // </objectgroup>

    public static class TMXMap
    {
        public final String file;
        public final String layerName;
        public final int tileSize;
        public final int w;
        public final int h;
        public final List<TSXTileset> tsxTilesets;
        public final List<TSXTileset> usedTilesets;
        public final int[] map;

        public TMXMap(String file, String layerName) throws Exception
        {
            if (!FileUtil.exists(file))
                throw new FileNotFoundException("TMX file '" + file + " not found !");

            this.file = file;
            this.layerName = layerName;

            final Document doc = XMLUtil.loadDocument(file);
            final Element mapElement = XMLUtil.getRootElement(doc);

            // check this is the map node
            if (!mapElement.getNodeName().toLowerCase().equals(ID_MAP))
                throw new Exception("Expected " + ID_MAP + " root node in TMX file: " + file + ", " + mapElement.getNodeName() + " found.");

            checkAttributValue(mapElement, ATTR_ORIENTATION, ATTR_VALUE_ORTHOGONAL, ATTR_VALUE_ORTHOGONAL, file);
            checkAttributValue(mapElement, ATTR_RENDERORDER, ATTR_VALUE_RIGHT_DOWN, ATTR_VALUE_RIGHT_DOWN, file);

            final int tw = XMLUtil.getAttributeIntValue(mapElement, ATTR_TILEWIDTH, 0);
            final int th = XMLUtil.getAttributeIntValue(mapElement, ATTR_TILEHEIGHT, 0);

            if (tw != th)
                throw new Exception("Non square tile not supported (" + tw + " x " + th + ") in TMX file: " + file);
            if ((tw & 7) != 0)
                throw new Exception("Unsuported tile size (should be a multiple of 8) in TMX file: " + file);
            if ((tw < 8) || (tw > 32))
                throw new Exception(tw + " x " + th + " tile size not supported (only 8x8 to 32x32 allowed) in TMX file: " + file);

            tileSize = tw;

            w = XMLUtil.getAttributeIntValue(mapElement, ATTR_WIDTH, 0);
            h = XMLUtil.getAttributeIntValue(mapElement, ATTR_HEIGHT, 0);

            if ((w == 0) || (h == 0))
                throw new Exception("Null map size in TMX file: " + file);

            final List<Element> tilesetElements = XMLUtil.getElements(mapElement, ID_TILESET);
            if (tilesetElements.isEmpty())
                throw new Exception("No tileset reference(s) found in TMX file: " + file);

            // add TSX tilesets
            tsxTilesets = new ArrayList<>();
            for (Element tilesetElement : tilesetElements)
            {
                // start ind with dummy blank tile correction
                final int startId = XMLUtil.getAttributeIntValue(tilesetElement, ATTR_FIRSTGID, 0);
                final String source = XMLUtil.getAttributeValue(tilesetElement, ATTR_SOURCE, "");

                // has a source ? --> external TSX
                if (!StringUtil.isEmpty(source))
                    // load from file
                    tsxTilesets.add(new TSXTileset(Util.getAdjustedPath(source, file), startId));
                else
                    // directly load TSX from node
                    tsxTilesets.add(new TSXTileset(file, startId, tilesetElement));
            }

            for (TSXTileset tileset : tsxTilesets)
            {
                if (tileset.tileSize != tileSize)
                    throw new Exception("One of the referenced tileset has a different tile size than map tile size in TMX file: " + file);
            }

            // sort tilesets on startTileIndex
            if (tsxTilesets.size() > 1)
                Collections.sort(tsxTilesets);

            final List<Element> layers = XMLUtil.getElements(mapElement, ID_LAYER);
            if (layers.isEmpty())
                throw new Exception("No layer found in TMX file: " + file);

            final int[] mapData1;
            final int[] mapData2;

            // single layer definition ?
            boolean singleLayer = hasLayer(layers, layerName);

            if (singleLayer)
            {
                final Element mainLayer = getLayer(layers, layerName);
                // try to get priority layer
                final Element priorityLayer = getLayer(layers, layerName + SUFFIX_PRIORITY);

                // get map data in mapData1
                mapData1 = getMapData(mainLayer, w, h, file);
                // get priority data in mapData2
                mapData2 = (priorityLayer != null) ? getMapData(priorityLayer, w, h, file) : new int[mapData1.length];
            }
            else
            {
                // use alternate priority definition
                final Element lowPriorityLayer = getLayer(layers, layerName + SUFFIX_LOW_PRIORITY);
                final Element highPriorityLayer = getLayer(layers, layerName + SUFFIX_HIGH_PRIORITY);

                if ((lowPriorityLayer == null) || (highPriorityLayer == null))
                    throw new Exception("No layer '" + layerName + "' found in TMX file: " + file);

                // get low priority map data in mapData1
                mapData1 = getMapData(lowPriorityLayer, w, h, file);
                // get high priority map data in mapData2
                mapData2 = getMapData(lowPriorityLayer, w, h, file);
            }

            final Set<TSXTileset> usedTilesetsSet = new HashSet<>();

            map = new int[mapData1.length];
            int ind = 0;

            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    final int v;
                    final boolean prio;

                    // single layer for map data
                    if (singleLayer)
                    {
                        // get map data
                        v = mapData1[ind];
                        // get prio data
                        prio = mapData2[ind] != 0;
                    }
                    // 2 layers (low and high prio)
                    else
                    {
                        // data in high prio map ? --> use it
                        if (mapData2[ind] != 0)
                        {
                            v = mapData2[ind];
                            prio = true;
                        }
                        // use low prio map data
                        else
                        {
                            v = mapData1[ind];
                            prio = false;
                        }
                    }

                    // final boolean prio = prioData[ind] != 0;
                    final boolean hflip = (v & TMX_HFLIP) != 0;
                    boolean vflip = (v & TMX_VFLIP) != 0;
                    final boolean axeflip = (v & TMX_AXE_FLIP) != 0;

                    if (axeflip)
                    {
                        // TODO: re-enable warning for non DA game
                        System.out.println("WARNING: unsupported rotated tile found at [" + x + "," + y + "] in TMX file: " + file);
                        //vflip = true;
                    }

                    // we can guess that tile index is never higher than 2^24
                    final int tileInd = v & 0xFFFFFF;

                    // define used TSX tileset
                    if (tileInd != 0)
                    {
                        final TSXTileset tileset = getTSXTilesetFor(tileInd);

                        if (tileset == null)
                            throw new Exception(
                                    "Tile [" + x + "," + y + "] is referencing an invalid index (" + tileInd + " is outside available tiles range)");

                        usedTilesetsSet.add(tileset);
                    }

                    // store attributes in upper word
                    map[ind++] = (Tile.TILE_ATTR_FULL(0, prio, vflip, hflip, 0) << 16) | tileInd;
                }
            }

            // keep trace of tilesets which are really used
            usedTilesets = new ArrayList<>(usedTilesetsSet);
            // sort on startTileIndex
            if (usedTilesets.size() > 1)
                Collections.sort(usedTilesets);

            // System.out.println("TMX map '" + file + "' is using " + usedTilesetsSet.size() + " tilesets:");
            // for (TSXTileset tileset : usedTilesetsSet)
            // System.out.println(tileset);
        }

        private TSXTileset getTSXTilesetFor(int tileInd)
        {
            for (TSXTileset tileset : tsxTilesets)
                if (tileset.containsTile(tileInd))
                    return tileset;

            return null;
        }

        public List<Tileset> getTilesets(String baseId, Compression compression, boolean temp) throws Exception
        {
            return TSX.getTilesets(usedTilesets, baseId, compression, temp);
        }

        // public Tilemap getTilemap(int mapBase) throws Exception
        // {
        // final int ts = (tileSize / 8);
        // final Tilemap result = new Tilemap(w * ts, h * ts);
        // final short[] data = new short[result.w * result.h];
        //
        // final boolean mapBasePrio = (mapBase & Tile.TILE_PRIORITY_MASK) != 0;
        // final int mapBasePal = (mapBase & Tile.TILE_PALETTE_MASK) >> Tile.TILE_PALETTE_SFT;
        // final int mapBaseTileInd = mapBase & Tile.TILE_INDEX_MASK;
        // // we have a base offset --> we can use system plain tiles
        // final boolean useSystemTiles = mapBaseTileInd != 0;
        //
        // // need to have a blank tile
        // if (!useSystemTiles)
        // {
        // System.out.println("WARNING: you're using TMX map (file: " + file + ") with a null map base index.");
        // System.out.println("See note in rescomp.txt about the required dummy blank tile in the tileset.");
        // }
        //
        // int ind = 0;
        // for (int y = 0; y < h; y++)
        // {
        // for (int x = 0; x < w; x++)
        // {
        // final short tileValue = map[ind];
        // final short tileAttr = (short) (tileValue & Tile.TILE_ATTR_MASK);
        // final short tileInd = (short) (tileValue & Tile.TILE_INDEX_MASK);
        //
        // // transparent tile with base tile offset defined ?
        // if ((tileInd == 0) && (mapBaseTileInd != 0))
        // {
        // // convert to 8x8 tile index
        // int tmInd = ind * ts * ts;
        //
        // // simply use tile 0 (transparent)
        // for (int ty = 0; ty < ts; ty++)
        // {
        // for (int tx = 0; tx < ts; tx++)
        // {
        // data[tmInd] = tileAttr;
        // tmInd++;
        // }
        //
        // tmInd += result.w - ts;
        // }
        // }
        // else
        // {
        // final TSXTileset tileset = getTSXTilesetFor(tileInd);
        //
        // if (tileset == null)
        // throw new Exception("Cannot find tilset for tile #" + tileInd + " in TMX file: " + file);
        //
        // final int tileRelativeInd = (tileInd - tileset.startTileIndex);
        // final int tileIndX = tileRelativeInd % tileset.imageTileWidth;
        // final int tileIndY = tileRelativeInd / tileset.imageTileWidth;
        // final int imageSingleTileWidth = tileset.imageTileWidth * (tileSize / 8);
        //
        // // convert to 8x8 tile index
        // int tmInd = ind * ts * ts;
        // int tmTileInd = (tileIndX * ts) + ((tileIndY * ts) * imageSingleTileWidth);
        //
        // if (tmTileInd >= 2048)
        // throw new Exception("Can't have tile index >= 2048 in TMX file: " + file + ", try to reduce number of unique
        // tile.");
        //
        // for (int ty = 0; ty < ts; ty++)
        // {
        // for (int tx = 0; tx < ts; tx++)
        // {
        // data[tmInd] = (short) Tile.TILE_ATTR_FULL(mapBasePal, mapBasePrio | ((tileAttr & Tile.TILE_PRIORITY_MASK) !=
        // 0),
        // (tileAttr & Tile.TILE_VFLIP_MASK) != 0, (tileAttr & Tile.TILE_HFLIP_MASK) != 0,
        // mapBaseTileInd + (tmTileInd & Tile.TILE_INDEX_MASK));
        // tmInd++;
        // tmTileInd++;
        // }
        //
        // tmInd += result.w - ts;
        // tmTileInd += imageSingleTileWidth - ts;
        // }
        // }
        //
        // ind++;
        // }
        // }
        //
        // // set tilemap data
        // result.setData(data);
        //
        // return result;
        // }

        // public int getTilesetImageHeight()
        // {
        // int totalTile = 0;
        //
        // for (TSXTileset tileset : usedTilesets)
        // totalTile += tileset.numTile;
        //
        // final int tilePerRow = 256 / tileSize;
        //
        // // calculate image height for 256 pixels wide tileset image
        // return ((totalTile + (tilePerRow - 1)) / tilePerRow) * tileSize;
        // }

        // RAW tileset image (width = 256)
        // public byte[] getTilesetImage() throws IllegalArgumentException, FileNotFoundException, IOException
        // {
        // final int wt = 256 / tileSize;
        // final int ht = getTilesetImageHeight() / tileSize;
        //
        // if (ht == 0)
        // return new byte[0];
        //
        // final byte[] fullTilesetImage = new byte[(wt * tileSize) * (ht * tileSize)];
        //
        // int tsInd = 0;
        //
        // TSXTileset tileset = usedTilesets.get(tsInd++);
        // byte[] tilesetImage = tileset.getTilesetImage8bpp(false);
        // int tilesetImageW = tileset.imageTileWidth * tileset.tileSize;
        // int tilesetImageH = tileset.imageTileHeigt * tileset.tileSize;
        // int tileInd = 0;
        // int remaining = tileset.numTile;
        //
        // for (int yt = 0; yt < ht; yt++)
        // {
        // for (int xt = 0; xt < wt; xt++)
        // {
        // final byte[] imageTile = Tile.getImageTile(tilesetImage, tilesetImageW, tilesetImageH, tileInd++, tileSize);
        // // copy tile
        // Tile.copyTile(fullTilesetImage, 256, imageTile, xt * tileSize, yt * tileSize, tileSize);
        //
        // // tileset done ? --> next tileset
        // if (--remaining == 0)
        // {
        // // done --> can return image now
        // if (tsInd >= usedTilesets.size())
        // return fullTilesetImage;
        //
        // tileset = usedTilesets.get(tsInd++);
        // tilesetImage = tileset.getTilesetImage8bpp(false);
        // tilesetImageW = tileset.imageTileWidth * tileset.tileSize;
        // tilesetImageH = (tileset.numTile / tileset.imageTileWidth) * tileset.tileSize;
        // tileInd = 0;
        // remaining = tileset.numTile;
        // }
        // }
        // }
        //
        // return fullTilesetImage;
        // }

        public byte[] getMapImage() throws Exception
        {
            final int mapImageW = w * tileSize;
            final int mapImageH = h * tileSize;
            final byte[] mapImage = new byte[mapImageW * mapImageH];

            // build tileset image map
            final Map<TSXTileset, byte[]> tilesets = new HashMap<>();

            for (TSXTileset tileset : usedTilesets)
                tilesets.put(tileset, tileset.getTilesetImage8bpp(true));
            // TODO: disable crop palette for DA game dev
            // tilesets.put(tileset, tileset.getTilesetImage8bpp(false));

            final byte[] baseTile = new byte[tileSize * tileSize];
            final byte[] transformedTile = new byte[tileSize * tileSize];

            int off = 0;
            for (int yt = 0; yt < h; yt++)
            {
                for (int xt = 0; xt < w; xt++)
                {
                    final int tile = map[off++];

                    final int tileInd = tile & 0xFFFFFF;
                    final short tileAttr = (short) ((tile >> 16) & 0xFFFF);

                    final boolean hflip = (tileAttr & Tile.TILE_HFLIP_MASK) != 0;
                    final boolean vflip = (tileAttr & Tile.TILE_VFLIP_MASK) != 0;
                    final boolean prio = (tileAttr & Tile.TILE_PRIORITY_MASK) != 0;

                    byte[] imageTile;

                    // special case of blank tile
                    if (tileInd == 0)
                    {
                        Arrays.fill(baseTile, (byte) 0);
                        imageTile = baseTile;
                    }
                    else
                    {
                        // find tileset for this tile
                        final TSXTileset tsxTileset = getTSXTilesetFor(tileInd);
                        // get tile
                        imageTile = Tile.getImageTile(tilesets.get(tsxTileset), tsxTileset.imageTileWidth * tileSize, tsxTileset.imageTileHeigt * tileSize,
                                tileInd - tsxTileset.startTileIndex, tileSize, baseTile);
                        // need to transform ?
                        if (hflip || vflip || prio)
                            imageTile = Tile.transformTile(imageTile, tileSize, hflip, vflip, prio, transformedTile);
                    }

                    // then copy tile
                    Tile.copyTile(mapImage, mapImageW, imageTile, xt * tileSize, yt * tileSize, tileSize);
                }
            }

            return mapImage;
        }

        @Override
        public String toString()
        {
            return "tileSize=" + tileSize + " - w=" + w + " - h=" + h;
        }
    }

    static String getAttribute(Element element, String attrName, String def)
    {
        return XMLUtil.getAttributeValue(element, attrName, def).toLowerCase();
    }

    static Element getLayer(List<Element> layers, String name)
    {
        for (Element element : layers)
            if (StringUtil.equals(getAttribute(element, ATTR_NAME, ""), name.toLowerCase()))
                return element;

        return null;
    }

    static boolean hasLayer(List<Element> layers, String name)
    {
        return getLayer(layers, name) != null;
    }

    static Element getElement(Node node, String name, String file) throws Exception
    {
        final Element result = XMLUtil.getElement(node, name);

        if (result == null)
            throw new Exception("Cannot find " + name + " XML node in TMX file: " + file);

        return result;
    }

    static void checkAttributValue(Element element, String attrName, String value, String def, String file) throws Exception
    {
        final String attrValue = getAttribute(element, attrName, def);

        if (!StringUtil.equals(attrValue, value))
            throw new Exception("'" + attrValue + "' " + attrName + " not supported (" + def + " expected) in TMX file: " + file);
    }

    static int[] getMapData(Element layerElement, int w, int h, String file) throws Exception
    {
        if (XMLUtil.getAttributeIntValue(layerElement, ATTR_WIDTH, w) != w)
            throw new Exception("Layer width do not match map width in TMX file: " + file);
        if (XMLUtil.getAttributeIntValue(layerElement, ATTR_HEIGHT, h) != h)
            throw new Exception("Layer height do not match map height in TMX file: " + file);

        final Element dataElement = getElement(layerElement, ID_DATA, file);

        checkAttributValue(dataElement, ATTR_ENCODING, ATTR_VALUE_CSV, "", file);

        final String[] values = XMLUtil.getFirstValue(dataElement, "").split(",");

        if (values.length != (w * h))
            throw new Exception("Data size (" + values.length + ") does not match Layer dimension (" + w + " x " + h + ") in TMX file: " + file);

        final int[] result = new int[values.length];

        for (int i = 0; i < values.length; i++)
            result[i] = (int) Long.parseLong(values[i].trim());

        return result;
    }
}
