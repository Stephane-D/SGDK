package sgdk.xgm2tool.format;

import sgdk.tool.StringUtil;
import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.tool.Util;

public class GD3
{
    final static String XGMTOOL_PRINT = "Optimized with XGMTool";

    int version;

    String trackName_EN;
    String trackName_JP;
    String gameName_EN;
    String gameName_JP;
    String systemName_EN;
    String systemName_JP;
    String authorName_EN;
    String authorName_JP;
    String date;
    String vgmConversionAuthor;
    String notes;

    public GD3()
    {
        trackName_EN = "";
        trackName_JP = "";
        gameName_EN = "";
        gameName_JP = "";
        systemName_EN = "";
        systemName_JP = "";
        authorName_EN = "";
        authorName_JP = "";
        date = "";
        vgmConversionAuthor = "";
        notes = "";

        version = 0x100;
    }

    public GD3(XD3 xd3)
    {
        trackName_EN = xd3.trackName;
        trackName_JP = xd3.trackName;
        gameName_EN = xd3.gameName;
        gameName_JP = xd3.gameName;
        systemName_EN = "SEGA Mega Drive";
        systemName_JP = "SEGA Mega Drive";
        authorName_EN = xd3.authorName;
        authorName_JP = xd3.authorName;
        date = xd3.date;
        vgmConversionAuthor = xd3.conversionAuthor;
        notes = xd3.notes;

        version = 0x100;
    }

    public GD3(byte[] data, int baseOffset)
    {
        int offset = baseOffset;

        if (!Launcher.silent)
            System.out.println("Parsing GD3...\n");

        final String id = Util.getASCIIString(data, offset, 4);

        if (!StringUtil.equals(id, "Gd3 "))
            System.out.println("Error: GD3 header not recognized !\n");
        offset += 4;

        // get version info
        version = Util.getInt32(data, offset);
        offset += 4;

        // get total size for GD3 data (we can ignore this field)
        Util.getInt32(data, offset);
        offset += 4;

        trackName_EN = Util.getWideString(data, offset);
        offset += (trackName_EN.length() + 1) * 2;
        trackName_JP = Util.getWideString(data, offset);
        offset += (trackName_JP.length() + 1) * 2;
        gameName_EN = Util.getWideString(data, offset);
        offset += (gameName_EN.length() + 1) * 2;
        gameName_JP = Util.getWideString(data, offset);
        offset += (gameName_JP.length() + 1) * 2;
        systemName_EN = Util.getWideString(data, offset);
        offset += (systemName_EN.length() + 1) * 2;
        systemName_JP = Util.getWideString(data, offset);
        offset += (systemName_JP.length() + 1) * 2;
        authorName_EN = Util.getWideString(data, offset);
        offset += (authorName_EN.length() + 1) * 2;
        authorName_JP = Util.getWideString(data, offset);
        offset += (authorName_JP.length() + 1) * 2;
        date = Util.getWideString(data, offset);
        offset += (date.length() + 1) * 2;
        vgmConversionAuthor = Util.getWideString(data, offset);
        offset += (vgmConversionAuthor.length() + 1) * 2;
        notes = Util.getWideString(data, offset);
        offset += (notes.length() + 1) * 2;

        // not ending with our signature ?
        if (!vgmConversionAuthor.endsWith(XGMTOOL_PRINT))
        {
            if (!StringUtil.isEmpty(vgmConversionAuthor))
                vgmConversionAuthor += " - ";

            // add XGMTool signature
            vgmConversionAuthor += XGMTOOL_PRINT;
        }
    }

    private int computeDataSize()
    {
        return (trackName_EN.length() * 2) + (trackName_JP.length() * 2) + (gameName_EN.length() * 2)
                + (gameName_JP.length() * 2) + (systemName_EN.length() * 2) + (systemName_JP.length() * 2)
                + (authorName_EN.length() * 2) + (authorName_JP.length() * 2) + (date.length() * 2)
                + (vgmConversionAuthor.length() * 2) + (notes.length() * 2) + (11 * 2);
    }

    public int getTotalDataSize()
    {
        return computeDataSize() + 12;
    }

    public byte[] asByteArray()
    {
        // align size on 2 bytes
        int size = computeDataSize();
        final byte[] result = new byte[size + 12];
        int offset = 0;

        // header
        System.arraycopy(Util.getBytes("Gd3 "), 0, result, offset, 4);
        offset += 4;
        // version
        Util.setInt32(result, offset, version);
        offset += 4;
        // size
        Util.setInt32(result, offset, size);
        offset += 4;

        // fields
        size = trackName_EN.length() * 2;
        System.arraycopy(Util.getBytes(trackName_EN, true), 0, result, offset, size);
        // +2 for 0 ending character in C
        offset += size + 2;
        size = trackName_JP.length() * 2;
        System.arraycopy(Util.getBytes(trackName_JP, true), 0, result, offset, size);
        offset += size + 2;
        size = gameName_EN.length() * 2;
        System.arraycopy(Util.getBytes(gameName_EN, true), 0, result, offset, size);
        offset += size + 2;
        size = gameName_JP.length() * 2;
        System.arraycopy(Util.getBytes(gameName_JP, true), 0, result, offset, size);
        offset += size + 2;
        size = systemName_EN.length() * 2;
        System.arraycopy(Util.getBytes(systemName_EN, true), 0, result, offset, size);
        offset += size + 2;
        size = systemName_JP.length() * 2;
        System.arraycopy(Util.getBytes(systemName_JP, true), 0, result, offset, size);
        offset += size + 2;
        size = authorName_EN.length() * 2;
        System.arraycopy(Util.getBytes(authorName_EN, true), 0, result, offset, size);
        offset += size + 2;
        size = authorName_JP.length() * 2;
        System.arraycopy(Util.getBytes(authorName_JP, true), 0, result, offset, size);
        offset += size + 2;
        size = date.length() * 2;
        System.arraycopy(Util.getBytes(date, true), 0, result, offset, size);
        offset += size + 2;
        size = vgmConversionAuthor.length() * 2;
        System.arraycopy(Util.getBytes(vgmConversionAuthor, true), 0, result, offset, size);
        offset += size + 2;
        size = notes.length() * 2;
        System.arraycopy(Util.getBytes(notes, true), 0, result, offset, size);

        return result;
    }
}
