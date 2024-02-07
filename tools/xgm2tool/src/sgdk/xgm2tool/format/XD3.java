package sgdk.xgm2tool.format;

import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.tool.Util;

public class XD3
{
    String trackName;
    String gameName;
    String authorName;
    String date;
    String conversionAuthor;
    String notes;
    int duration; // duration in frame
    int loopDuration; // duration in frame (0 if no loop)

    public XD3()
    {
        super();

        trackName = "";
        gameName = "";
        authorName = "";
        date = "";
        conversionAuthor = "";
        notes = "";
        duration = 0;
        loopDuration = 0;
    }

    public XD3(byte[] data, int baseOffset)
    {
        int offset = baseOffset;

        if (!Launcher.silent)
            System.out.println("Parsing XD3...\n");

        // total size for XD3 data (we can ignore this field)
        Util.getInt32(data, offset);
        offset += 4;

        // fields
        trackName = Util.getASCIIString(data, offset);
        // +1 for 0 ending character
        offset += trackName.length() + 1;
        gameName = Util.getASCIIString(data, offset);
        // +1 for 0 ending character
        offset += gameName.length() + 1;
        authorName = Util.getASCIIString(data, offset);
        // +1 for 0 ending character
        offset += authorName.length() + 1;
        date = Util.getASCIIString(data, offset);
        // +1 for 0 ending character
        offset += date.length() + 1;
        conversionAuthor = Util.getASCIIString(data, offset);
        // +1 for 0 ending character
        offset += conversionAuthor.length() + 1;
        notes = Util.getASCIIString(data, offset);
        // +1 for 0 ending character
        offset += notes.length() + 1;

        // duration and loop
        duration = Util.getInt32(data, offset + 0);
        loopDuration = Util.getInt32(data, offset + 4);
    }

    public XD3(GD3 gd3, int duration, int loopDuration)
    {
        trackName = gd3.trackName_EN;
        gameName = gd3.gameName_EN;
        authorName = gd3.authorName_EN;
        date = gd3.date;
        conversionAuthor = gd3.vgmConversionAuthor;
        notes = gd3.notes;

        this.duration = duration;
        this.loopDuration = loopDuration;
    }

    private int computeDataSize()
    {
        return trackName.length() + gameName.length() + authorName.length() + date.length() + conversionAuthor.length()
                + notes.length() + (6 * 1) + 8; // +8 for durations informations
    }

    public int getTotalDataSize()
    {
        return computeDataSize() + 4;
    }

    public byte[] asByteArray()
    {
        // align size on 2 bytes
        int size = (computeDataSize() + 1) & 0xFFFFFFFE;
        final byte[] result = new byte[size + 4];
        int offset = 0;

        // size of XD3 infos
        Util.setInt32(result, offset, size);
        offset += 4;

        // fields
        size = trackName.length();
        System.arraycopy(Util.getBytes(trackName), 0, result, offset, size);
        // +1 for 0 ending character in C
        offset += size + 1;
        size = gameName.length();
        System.arraycopy(Util.getBytes(gameName), 0, result, offset, size);
        // +1 for 0 ending character in C
        offset += size + 1;
        size = authorName.length();
        System.arraycopy(Util.getBytes(authorName), 0, result, offset, size);
        // +1 for 0 ending character in C
        offset += size + 1;
        size = date.length();
        System.arraycopy(Util.getBytes(date), 0, result, offset, size);
        // +1 for 0 ending character in C
        offset += size + 1;
        size = conversionAuthor.length();
        System.arraycopy(Util.getBytes(conversionAuthor), 0, result, offset, size);
        // +1 for 0 ending character in C
        offset += size + 1;
        size = notes.length();
        System.arraycopy(Util.getBytes(notes), 0, result, offset, size);
        // +1 for 0 ending character in C
        offset += size + 1;

        // duration and loop
        Util.setInt32(result, offset + 0, duration);
        Util.setInt32(result, offset + 4, loopDuration);

        return result;
    }
}
