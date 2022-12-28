package sgdk.rescomp.type;

import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SFieldDef.SGDKObjectType;
import sgdk.rescomp.type.TFieldDef.TiledObjectType;
import sgdk.rescomp.type.TSX.TSXTileset;
import sgdk.tool.FileUtil;
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
    static final String ID_PROPERTIES = "properties";
    static final String ID_PROPERTY = "property";
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
    static final String ATTR_CLASS = "class";
    static final String ATTR_X = "x";
    static final String ATTR_Y = "y";
    static final String ATTR_PROPERTYTYPE = "propertytype";
    static final String ATTR_VALUE = "value";

    static final String ATTR_VALUE_ORTHOGONAL = "orthogonal";
    static final String ATTR_VALUE_RIGHT_DOWN = "right-down";
    static final String ATTR_VALUE_CSV = "csv";
    static final String ATTR_VALUE_TEXT = "text";
    static final String ATTR_VALUE_CLASS = "class";
    static final String ATTR_VALUE_EXPORT_NAME = "exportname";
    static final String ATTR_VALUE_EXPORT_POSITION = "exportposition";
    static final String ATTR_VALUE_EXPORT_TILE_INDEX = "exporttileindex";
    static final String ATTR_VALUE_EXPORT_SIZE = "exportsize";

    // keep it lower case
    static final String FIELD_TILE_INDEX = "tileindex";
    static final String FIELD_INDEX = "index";
    static final String FIELD_IND = "ind";
    static final String FIELD_ID = "id";

    static final String SUFFIX_PRIORITY_SHORT = " prio";
    static final String SUFFIX_PRIORITY = " priority";
    static final String SUFFIX_LOW_PRIORITY = " low";
    static final String SUFFIX_HIGH_PRIORITY = " high";

    static final int TMX_HFLIP = (1 << 31);
    static final int TMX_VFLIP = (1 << 30);
    static final int TMX_AXE_FLIP = (1 << 29);
    static final int TMX_TILE_IND_MASK = 0x0FFFFFFF;

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
        // priority map is always 8x78 tile based
        public final boolean[] prioMap;

        // used internally
        private final Map<TSXTileset, byte[]> prioTilesets;

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
            boolean singleLayer = hasElementNamed(layers, layerName);

            if (singleLayer)
            {
                final Element mainLayer = getElementNamed(layers, layerName);
                // try to get priority layer
                Element priorityLayer = getElementNamed(layers, layerName + SUFFIX_PRIORITY);
                // try to get priority layer (with short id this time)
                if (priorityLayer == null)
                    priorityLayer = getElementNamed(layers, layerName + SUFFIX_PRIORITY_SHORT);

                // get map data in mapData1
                mapData1 = getMapData(mainLayer, w, h, file);
                // get priority data in mapData2
                mapData2 = (priorityLayer != null) ? getMapData(priorityLayer, w, h, file) : new int[mapData1.length];
            }
            else
            {
                // use alternate priority definition
                final Element lowPriorityLayer = getElementNamed(layers, layerName + SUFFIX_LOW_PRIORITY);
                final Element highPriorityLayer = getElementNamed(layers, layerName + SUFFIX_HIGH_PRIORITY);

                if ((lowPriorityLayer == null) || (highPriorityLayer == null))
                    throw new Exception("No layer '" + layerName + "' found in TMX file: " + file);

                // get low priority map data in mapData1
                mapData1 = getMapData(lowPriorityLayer, w, h, file);
                // get high priority map data in mapData2
                mapData2 = getMapData(lowPriorityLayer, w, h, file);
            }

            final Set<TSXTileset> usedTilesetsSet = new HashSet<>();
            final Map<Integer, Integer> prioIndexesMap = new HashMap<>();

            prioTilesets = new HashMap<>();

            // initialize prio indexes map (tile index --> priority info in integer format)
            prioIndexesMap.put(Integer.valueOf(0), Integer.valueOf(0));
            prioIndexesMap.put(Integer.valueOf(-1), Integer.valueOf(-1));

            // base tile (8x8) size
            final int baseTileSize = (tileSize / 8);

            map = new int[mapData1.length];
            // priority map is always 8x8 tile based
            prioMap = new boolean[map.length * (baseTileSize * baseTileSize)];

            int ind = 0;

            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    final int v;
                    final int prio;

                    // single layer for map data
                    if (singleLayer)
                    {
                        // get map data
                        v = mapData1[ind];
                        // get prio data
                        prio = mapData2[ind];
                    }
                    // 2 layers (low and high prio)
                    else
                    {
                        // data in high prio map ? --> use it
                        if (mapData2[ind] != 0)
                        {
                            v = mapData2[ind];
                            prio = -1;
                        }
                        // use low prio map data
                        else
                        {
                            v = mapData1[ind];
                            prio = 0;
                        }
                    }

                    final boolean hflip = (v & TMX_HFLIP) != 0;
                    boolean vflip = (v & TMX_VFLIP) != 0;
                    final boolean axeflip = (v & TMX_AXE_FLIP) != 0;

                    if (axeflip)
                    {
                        if (!Compiler.DAGame)
                            System.out.println(
                                    "WARNING: unsupported rotated tile found at [" + x + "," + y + "] in layer '" + layerName + "' of TMX file: " + file);

                        // assume vflip for DA
                        if (Compiler.DAGame)
                            vflip = true;
                    }

                    final int tileInd = v & TMX_TILE_IND_MASK;

                    // we check that tile index is not higher than 2^16 (we store extra attributes in upper word)
                    if (tileInd > 0xFFFF)
                        throw new Exception("Tile [" + x + "," + y + "] of layer '" + layerName + "' has an index > 65535 (" + v
                                + "), maximum supported tile index is 65535");

                    // define used TSX tileset
                    if (tileInd != 0)
                    {
                        final TSXTileset tileset = getTSXTilesetFor(tileInd);

                        if (tileset == null)
                            throw new Exception("Tile [" + x + "," + y + "] of layer '" + layerName + "' is referencing an invalid index (" + tileInd
                                    + " is outside available tiles range)");

                        // common mistake so better to check
                        if (Compiler.DAGame)
                        {
                            if (tileset.file.contains("Gamefield"))
                                throw new Exception("Tile [" + x + "," + y + "]  of layer '" + layerName + "' is referencing the Gamefield overlay tileset !)");
                            if (tileset.file.contains("DMA"))
                                throw new Exception("Tile [" + x + "," + y + "]  of layer '" + layerName + "' is referencing the Gamefield overlay tileset !)");
                        }

                        usedTilesetsSet.add(tileset);
                    }

                    // store attributes in upper word
                    map[ind] = (Tile.TILE_ATTR_FULL(0, false, vflip, hflip, 0) << 16) | tileInd;

                    // get priority data
                    Integer prioData = prioIndexesMap.get(Integer.valueOf(prio));
                    // not yet in the map ?
                    if (prioData == null)
                    {
                        // get result
                        prioData = Integer.valueOf(getPrio(prio, baseTileSize));
                        // store it in map for next time
                        prioIndexesMap.put(Integer.valueOf(prio), prioData);
                    }

                    // get priority data
                    int data = prioData.intValue();

                    // something to set ?
                    if (data != 0)
                    {
                        // convert to 8x8 tile index
                        int prioInd = ((y * w * baseTileSize) * baseTileSize) + (x * baseTileSize);
                        // get mask to test next tile prio
                        int mask = 1 << (baseTileSize * baseTileSize);

                        // set priority data in priority map
                        for (int j = 0; j < baseTileSize; j++)
                        {
                            for (int i = 0; i < baseTileSize; i++)
                            {
                                // set prio
                                prioMap[prioInd] = (data & mask) != 0;

                                // next tile
                                mask >>= 1;
                                prioInd++;
                            }

                            // next row
                            prioInd += (w * baseTileSize) - baseTileSize;
                        }
                    }

                    // next (meta) tile
                    ind++;
                }
            }

            // keep trace of tilesets which are really used
            usedTilesets = new ArrayList<>(usedTilesetsSet);
            // sort on startTileIndex
            if (usedTilesets.size() > 1)
                Collections.sort(usedTilesets);

            if (Compiler.DAGame)
            {
                System.out.println("TMX map '" + file + "' layer '" + layerName + "' is using " + usedTilesetsSet.size() + " tilesets:");
                for (TSXTileset tileset : usedTilesetsSet)
                    System.out.println(tileset);
            }
        }

        private int getPrio(int ind, int baseTileSize) throws Exception
        {
            // special case of blank tile --> all LOW priority
            if (ind == 0)
                return 0;

            final TSXTileset tsxTileset = getTSXTilesetFor(ind);

            // not tileset found ? --> assume all HIGH priority
            if (tsxTileset == null)
                return -1;

            // get tileset image
            byte[] prioTileset = prioTilesets.get(tsxTileset);

            // not yet set in map ?
            if (prioTileset == null)
            {
                // load tileset
                prioTileset = tsxTileset.getTilesetImage8bpp(Compiler.DAGame ? false : true);
                // add it in map
                prioTilesets.put(tsxTileset, prioTileset);
            }

            final int imgW = tsxTileset.imageTileWidth * tileSize;
            final int imgH = tsxTileset.imageTileHeigt * tileSize;
            final byte[] imageTile = new byte[8 * 8];

            // get relative tile index
            int adjTileInd = ind - tsxTileset.startTileIndex;
            // get X / Y tile position
            final int yt = adjTileInd / tsxTileset.imageTileWidth;
            final int xt = adjTileInd % tsxTileset.imageTileWidth;
            // convert to 8x8 tile index
            adjTileInd = ((yt * tsxTileset.imageTileWidth * baseTileSize) * baseTileSize) + (xt * baseTileSize);
            int result = 0;

            // build prio data (8x8 tile based)
            for (int j = 0; j < baseTileSize; j++)
            {
                for (int i = 0; i < baseTileSize; i++)
                {
                    // get tile
                    Tile.getImageTile(prioTileset, imgW, imgH, adjTileInd, 8, imageTile);

                    // check that tile is empty
                    for (int d : imageTile)
                    {
                        // not empty ? --> set prio and stop
                        if (d != 0)
                        {
                            result |= 1;
                            break;
                        }
                    }

                    // next tile
                    result <<= 1;
                    adjTileInd++;
                }

                // next row
                adjTileInd += (imgW / 8) - baseTileSize;
            }

            return result;
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
                tilesets.put(tileset, tileset.getTilesetImage8bpp(Compiler.DAGame ? false : true));

            final byte[] baseTile = new byte[tileSize * tileSize];
            final byte[] transformedTile = new byte[tileSize * tileSize];
            // base tile (8x8) size
            final int baseTileSize = (tileSize / 8);

            int ind = 0;
            for (int yt = 0; yt < h; yt++)
            {
                for (int xt = 0; xt < w; xt++)
                {
                    final int tile = map[ind];

                    final int tileInd = tile & 0xFFFFFF;
                    final short tileAttr = (short) ((tile >> 16) & 0xFFFF);

                    final boolean hflip = (tileAttr & Tile.TILE_HFLIP_MASK) != 0;
                    final boolean vflip = (tileAttr & Tile.TILE_VFLIP_MASK) != 0;

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
                    }

                    // need to transform ?
                    if (hflip || vflip)
                        imageTile = Tile.transformTile(imageTile, tileSize, hflip, vflip, false, transformedTile);

                    // convert to 8x8 tile index
                    int prioInd = ((yt * w * baseTileSize) * baseTileSize) + (xt * baseTileSize);

                    // set priority
                    for (int j = 0; j < baseTileSize; j++)
                    {
                        for (int i = 0; i < baseTileSize; i++)
                        {
                            // priority set on this tile ?
                            if (prioMap[prioInd])
                                Tile.setPrioTile(imageTile, tileSize, i * 8, j * 8, 8);

                            // next tile
                            prioInd++;
                        }

                        // next row
                        prioInd += (w * baseTileSize) - baseTileSize;
                    }

                    // then copy tile
                    Tile.copyTile(mapImage, mapImageW, imageTile, xt * tileSize, yt * tileSize, tileSize);

                    // next
                    ind++;
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

    public static class TMXObjects
    {
        public final String file;
        public final String layerName;
        public final List<SObject> objects;

        public TMXObjects(String baseId, String file, String layerName, LinkedHashMap<String, SGDKObjectType> fieldDefs, String typeFilter) throws Exception
        {
            if (!FileUtil.exists(file))
                throw new FileNotFoundException("TMX file '" + file + " not found !");

            this.file = file;
            this.layerName = layerName;
            objects = new ArrayList<>();

            final Document doc = XMLUtil.loadDocument(file);
            final Element mapElement = XMLUtil.getRootElement(doc);

            // check this is the map node
            if (!mapElement.getNodeName().toLowerCase().equals(ID_MAP))
                throw new Exception("Expected " + ID_MAP + " root node in TMX file: " + file + ", " + mapElement.getNodeName() + " found.");

            final int tw = XMLUtil.getAttributeIntValue(mapElement, ATTR_TILEWIDTH, 0);
            final int th = XMLUtil.getAttributeIntValue(mapElement, ATTR_TILEHEIGHT, 0);

            if (tw != th)
                throw new Exception("Non square tile not supported (" + tw + " x " + th + ") in TMX file: " + file);
            if ((tw & 7) != 0)
                throw new Exception("Unsuported tile size (should be a multiple of 8) in TMX file: " + file);
            if ((tw < 8) || (tw > 32))
                throw new Exception(tw + " x " + th + " tile size not supported (only 8x8 to 32x32 allowed) in TMX file: " + file);

            final int tileSize = tw;
            final int mapWidth = XMLUtil.getAttributeIntValue(mapElement, ATTR_WIDTH, 0);

            if (mapWidth == 0)
                throw new Exception("Null map width in TMX file: " + file);

            final List<Element> objectGroups = XMLUtil.getElements(mapElement, ID_OBJECTGROUP);
            if (objectGroups.isEmpty())
                throw new Exception("No object layer found in TMX file: " + file);

            final Element group = getElementNamed(objectGroups, layerName);

            if (group == null)
                throw new Exception("No object layer '" + layerName + "' found in TMX file: " + file);

            // base object file name
            final String baseObjectName = baseId + "_object";
            final Map<String, TField> tFields = new HashMap<>();
            final List<Element> objectElements = XMLUtil.getElements(group, ID_OBJECT);

            // get all objects
            for (Element objectElement : objectElements)
            {
                // Tiled <= 1.8 uses 'type' for object type
                String objectType = getAttribute(objectElement, ATTR_TYPE, "");
                // Tiled > 1.8 uses 'class' for object type
                if (StringUtil.isEmpty(objectType))
                    objectType = getAttribute(objectElement, ATTR_CLASS, "");

                // type filter enabled and not matching type ? --> next object
                if (!StringUtil.isEmpty(typeFilter) && !StringUtil.equals(objectType, typeFilter))
                    continue;

                // clear field list
                tFields.clear();

                final int id = XMLUtil.getAttributeIntValue(objectElement, ATTR_ID, 0);
                final String objectName = getAttribute(objectElement, ATTR_NAME, "object_" + id);
                final double x = XMLUtil.getAttributeDoubleValue(objectElement, ATTR_X, 0d);
                final double y = XMLUtil.getAttributeDoubleValue(objectElement, ATTR_Y, 0d);
                final double w = XMLUtil.getAttributeDoubleValue(objectElement, ATTR_WIDTH, 0d);
                final double h = XMLUtil.getAttributeDoubleValue(objectElement, ATTR_HEIGHT, 0d);

                // get all properties
                final List<Element> propertyElements = getProperties(objectElement, new ArrayList<Element>());

                // build all fields from the properties
                for (Element property : propertyElements)
                {
                    // always take the lower case version for name
                    final String name = getAttribute(property, ATTR_NAME, "").toLowerCase();
                    final String type = getAttribute(property, ATTR_TYPE, "");
                    final String propertyType = getAttribute(property, ATTR_PROPERTYTYPE, "");
                    final String value = getAttribute(property, ATTR_VALUE, "");

                    // export name field ?
                    if (StringUtil.equals(name, ATTR_VALUE_EXPORT_NAME))
                    {
                        // set to true ? --> add 'name' field
                        if (StringUtil.equals(value.toLowerCase(), "true"))
                            addField(objectName, tFields, new TField(ATTR_NAME, TiledObjectType.STRING, objectName));
                    }
                    // export position field ?
                    else if (StringUtil.equals(name, ATTR_VALUE_EXPORT_POSITION))
                    {
                        // set to true ? --> add 'x' and 'y' fields
                        if (StringUtil.equals(value.toLowerCase(), "true"))
                        {
                            addField(objectName, tFields, new TField(ATTR_X, TiledObjectType.FLOAT, Double.toString(x)));
                            addField(objectName, tFields, new TField(ATTR_Y, TiledObjectType.FLOAT, Double.toString(y)));
                        }
                    }
                    // export size field ?
                    else if (StringUtil.equals(name, ATTR_VALUE_EXPORT_SIZE))
                    {
                        // set to true ? --> add 'width' and 'height' field
                        if (StringUtil.equals(value.toLowerCase(), "true"))
                        {
                            addField(objectName, tFields, new TField(ATTR_WIDTH, TiledObjectType.FLOAT, Double.toString(w)));
                            addField(objectName, tFields, new TField(ATTR_HEIGHT, TiledObjectType.FLOAT, Double.toString(h)));
                        }
                    }
                    // export tile index field ?
                    else if (StringUtil.equals(name, ATTR_VALUE_EXPORT_TILE_INDEX))
                    {
                        // set to true ? --> add 'tileIndex' field
                        if (StringUtil.equals(value.toLowerCase(), "true"))
                        {
                            // compute tile index
                            final int tileIndex = (((int) x) / tileSize) + ((((int) y) / tileSize) * mapWidth);
                            // add field
                            addField(objectName, tFields, new TField(FIELD_TILE_INDEX, TiledObjectType.INT, Double.toString(tileIndex)));
                        }
                    }
                    // empty type ?
                    else if (StringUtil.isEmpty(type))
                    {
                        // defined property type ? --> enum type
                        if (!StringUtil.isEmpty(propertyType))
                            addField(objectName, tFields, new TField(name, TiledObjectType.ENUM, value));
                        // missing type ? --> assume string (Tiled does not set type for string)
                        else
                            addField(objectName, tFields, new TField(name, TiledObjectType.STRING, value));
                    }
                    else
                        addField(objectName, tFields, new TField(name, TiledObjectType.fromString(type), value));
                }

                // create object
                final SObject object = new SObject(id, baseObjectName, objectType, x, y);
                // iterate over field definitions (allow good ordering of fields)
                for (String fieldName : fieldDefs.keySet())
                {
                    // find field declaration
                    final TField field = tFields.get(fieldName);

                    // we have the field ? --> add it to the object
                    if (field != null)
                        object.addField(field.toSField(baseObjectName, fieldDefs.get(fieldName)));
                    else
                    // field not found ?
                    {
                        // patch for DA game to fix Tiled export bug
                        if (Compiler.DAGame)
                        {
                            final int numField = object.fields.size();
                            // current field is 'next' and we had a previous 'waitId' field ?
                            // --> add 'next' field with NULL value
                            if (StringUtil.equals(fieldName, "next") && (numField > 0) && StringUtil.equals(object.fields.get(numField - 1).name, "waitid"))
                                object.addField(new SField("next", SGDKObjectType.OBJECT, ""));
                        }
                    }
                }

                // finally add the object
                objects.add(object);
            }

            // sort objects on tile index (if it exists)
            Collections.sort(objects, new ObjectComparator());
        }

        private static boolean addField(String objectName, Map<String, TField> fields, TField field)
        {
            if (fields.containsKey(field.name))
            {
                System.out.println("Warning: Object '" + objectName + "' already has a field named '" + field.name + "', new field ignored...");
                return false;
            }

            // add field
            fields.put(field.name, field);

            return true;
        }

        private List<Element> getProperties(Element objectElement, List<Element> result)
        {
            final Element properties = XMLUtil.getElement(objectElement, ID_PROPERTIES);

            // no more properties
            if (properties == null)
                return result;

            for (Element propertyElement : XMLUtil.getElements(properties, ID_PROPERTY))
            {
                // is it a class (object) property ? --> get its properties
                if (StringUtil.equals(getAttribute(propertyElement, ATTR_TYPE, "").toLowerCase(), ATTR_VALUE_CLASS))
                    getProperties(propertyElement, result);
                else
                    // just add the property
                    result.add(propertyElement);
            }

            return result;
        }

        @Override
        public String toString()
        {
            return "Object layer=" + layerName + " number of object=" + objects.size();
        }

        static class ObjectComparator implements Comparator<SObject>
        {
            @Override
            public int compare(SObject o1, SObject o2)
            {
                int result;

                // sort first on 'index' field
                result = Long.compare(o1.getFieldLongValue(FIELD_INDEX), o2.getFieldLongValue(FIELD_INDEX));
                // then sort on 'ind' field
                if (result == 0)
                    result = Long.compare(o1.getFieldLongValue(FIELD_IND), o2.getFieldLongValue(FIELD_IND));
                // then sort on 'tileindex' field
                if (result == 0)
                    result = Long.compare(o1.getFieldLongValue(FIELD_TILE_INDEX), o2.getFieldLongValue(FIELD_TILE_INDEX));
                // then sort on 'id' field
                if (result == 0)
                    result = Long.compare(o1.getFieldLongValue(FIELD_ID), o2.getFieldLongValue(FIELD_ID));

                return result;
            }
        }

    }

    static String getAttribute(Element element, String attrName, String def)
    {
        return XMLUtil.getAttributeValue(element, attrName, def);
    }

    static Element getElementNamed(List<Element> layers, String name)
    {
        for (Element element : layers)
            if (StringUtil.equals(getAttribute(element, ATTR_NAME, "").toLowerCase(), name.toLowerCase()))
                return element;

        return null;
    }

    static boolean hasElementNamed(List<Element> layers, String name)
    {
        return getElementNamed(layers, name) != null;
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
        final String attrValue = getAttribute(element, attrName, def).toLowerCase();

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
