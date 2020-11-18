package sgdk.rescomp.tool;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;

import sgdk.aplib.APJ;
import sgdk.lz4w.LZ4W;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.PackedData;
import sgdk.rescomp.type.Basics.SoundDriver;
import sgdk.rescomp.type.Basics.TileOptimization;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;
import sgdk.tool.SystemUtil;
import sgdk.tool.TypeUtil;

public class Util
{
    final static String[] formatAsm = {"b", "b", "w", "w", "d"};

    public static short toVDPColor(byte b, byte g, byte r)
    {
        // genesis only define on 3 bits (but shifted to 1 left)
        final int ri = (r >> 4) & 0xE;
        final int gi = (g >> 4) & 0xE;
        final int bi = (byte) ((b >> 4) & 0xE);

        return (short) ((ri << 0) | (gi << 4) | (bi << 8));
    }

    public static SoundDriver getSoundDriver(String text)
    {
        final String upText = text.toUpperCase();

        if (StringUtil.isEmpty(upText) || StringUtil.equals(upText, "PCM") || StringUtil.equals(upText, "0"))
            return SoundDriver.PCM;
        if (StringUtil.equals(upText, "2ADPCM") || StringUtil.equals(upText, "1"))
            return SoundDriver.DPCM2;
        if (StringUtil.equals(upText, "4PCM") || StringUtil.equals(upText, "2") || StringUtil.equals(upText, "3"))
            return SoundDriver.PCM4;
        if (StringUtil.equals(upText, "XGM") || StringUtil.equals(upText, "4") || StringUtil.equals(upText, "5"))
            return SoundDriver.XGM;

        throw new IllegalArgumentException("Unrecognized sound driver: '" + text + "'");
    }

    public static Compression getCompression(String text)
    {
        final String upText = text.toUpperCase();

        if (StringUtil.equals(upText, "AUTO") || StringUtil.equals(upText, "BEST") || StringUtil.equals(upText, "-1"))
            return Compression.AUTO;
        if (StringUtil.isEmpty(upText) || StringUtil.equals(upText, "NONE") || StringUtil.equals(upText, "0"))
            return Compression.NONE;
        if (StringUtil.equals(upText, "APLIB") || StringUtil.equals(upText, "1"))
            return Compression.APLIB;
        if (StringUtil.equals(upText, "LZ4W") || StringUtil.equals(upText, "2") || StringUtil.equals(upText, "FAST"))
            return Compression.LZ4W;

        throw new IllegalArgumentException("Unrecognized compression: '" + text + "'");
    }

    public static CollisionType getCollision(String text)
    {
        final String upText = text.toUpperCase();

        if (StringUtil.isEmpty(upText) || StringUtil.equals(upText, "NONE"))
            return CollisionType.NONE;
        if (StringUtil.equals(upText, "BOX"))
            return CollisionType.BOX;
        if (StringUtil.equals(upText, "CIRCLE"))
            return CollisionType.CIRCLE;

        throw new IllegalArgumentException("Unrecognized collision: '" + text + "'");
    }

    public static TileOptimization getTileOpt(String text)
    {
        final String upText = text.toUpperCase();

        if (StringUtil.equals(upText, "NONE") || StringUtil.equals(upText, "0"))
            return TileOptimization.NONE;
        if (StringUtil.isEmpty(upText) || StringUtil.equals(upText, "ALL") || StringUtil.equals(upText, "1"))
            return TileOptimization.ALL;
        if (StringUtil.equals(upText, "DUPLICATE") || StringUtil.equals(upText, "2"))
            return TileOptimization.DUPLICATE_ONLY;

        throw new IllegalArgumentException("Unrecognized tilemap optimization: '" + text + "'");
    }

    public static OptimizationType getSpriteOpt(String text)
    {
        final String upText = text.toUpperCase();

        if (StringUtil.isEmpty(upText) || StringUtil.equals(upText, "BALANCED") || StringUtil.equals(upText, "0"))
            return OptimizationType.BALANCED;
        if (StringUtil.equals(upText, "SPRITE") || StringUtil.equals(upText, "1"))
            return OptimizationType.MIN_SPRITE;
        if (StringUtil.equals(upText, "TILE") || StringUtil.equals(upText, "2"))
            return OptimizationType.MIN_TILE;
        if (StringUtil.equals(upText, "NONE") || StringUtil.equals(upText, "3"))
            return OptimizationType.NONE;

        throw new IllegalArgumentException("Unrecognized sprite optimization: '" + text + "'");
    }

    public static byte[] sizeAlign(byte[] data, int align, byte fill)
    {
        // compute alignment
        int newSize = (data.length + (align - 1));
        newSize /= align;
        newSize *= align;

        // create result array
        final byte[] result = new byte[newSize];

        // copy original data
        System.arraycopy(data, 0, result, 0, data.length);
        // fill align part with given fill value
        Arrays.fill(result, data.length, result.length, fill);

        return result;
    }

    public static void decl(StringBuilder outS, StringBuilder outH, String type, String name, int align, boolean global)
    {
        // asm declaration
        outS.append("    .align " + ((align < 2) ? 2 : align) + "\n");
        if (global)
            outS.append("    .global " + name + "\n");
        outS.append(name + ":\n");

        // include declaration
        if (global)
            outH.append("extern const " + type + " " + name + ";\n");
    }

    public static void declArray(StringBuilder outS, StringBuilder outH, String type, String name, int size, int align,
            boolean global)
    {
        // asm declaration
        outS.append("    .align  " + ((align < 2) ? 2 : align) + "\n");
        if (global)
            outS.append("    .global " + name + "\n");
        outS.append(name + ":\n");

        // include declaration
        if (global)
            outH.append("extern const " + type + " " + name + "[" + size + "];\n");
    }

    public static void outS(StringBuilder out, byte[] data, int intSize)
    {
        int offset = 0;
        // align remain on word
        int remain = ((data.length + 1) / 2) * 2;
        int adjIntSize = (intSize < 2) ? 2 : intSize;

        while (remain > 0)
        {
            out.append("    dc." + formatAsm[adjIntSize] + "    ");

            for (int i = 0; i < Math.min(16, remain) / adjIntSize; i++)
            {
                if (i > 0)
                    out.append(", ");

                out.append("0x");

                if (intSize == 1)
                {
                    // we cannot use byte data because of GCC bugs with -G parameter
                    out.append(StringUtil.toHexaString(TypeUtil.unsign(data[offset + 0]), 2));

                    if ((offset + 1) >= data.length)
                        out.append("00");
                    else
                        out.append(StringUtil.toHexaString(TypeUtil.unsign(data[offset + 1]), 2));

                    offset += adjIntSize;
                }
                else
                {
                    offset += adjIntSize;

                    for (int j = 0; j < adjIntSize; j++)
                        out.append(StringUtil.toHexaString(TypeUtil.unsign(data[offset - (j + 1)]), 2));
                }
            }

            out.append("\n");
            remain -= 16;
        }
    }

    public static byte[] in(String fin)
    {
        return FileUtil.load(fin, true);
    }

    public static boolean out(byte[] data, int intSize, boolean swap, String fout)
    {
        final byte[] outData = new byte[data.length];

        int iOff = 0;
        int oOff = 0;
        int remain = data.length;

        while (remain > 0)
        {
            switch (intSize)
            {
                default:
                    outData[oOff++] = data[iOff++];
                    break;

                case 2:
                    if (swap)
                    {
                        outData[oOff + 0] = data[iOff + 1];
                        outData[oOff + 1] = data[iOff + 0];
                    }
                    else
                    {
                        outData[oOff + 0] = data[iOff + 0];
                        outData[oOff + 1] = data[iOff + 1];
                    }
                    iOff += 2;
                    oOff += 2;
                    break;

                case 4:
                    if (swap)
                    {
                        outData[oOff + 0] = data[iOff + 3];
                        outData[oOff + 1] = data[iOff + 2];
                        outData[oOff + 2] = data[iOff + 1];
                        outData[oOff + 3] = data[iOff + 0];
                    }
                    else
                    {
                        outData[oOff + 0] = data[iOff + 0];
                        outData[oOff + 1] = data[iOff + 1];
                        outData[oOff + 2] = data[iOff + 2];
                        outData[oOff + 3] = data[iOff + 3];
                    }
                    iOff += 4;
                    oOff += 4;
                    break;
            }

            remain -= intSize;
        }

        return FileUtil.save(fout, outData, true) && (remain == 0);
    }

    public static boolean out(byte[] data, String fout)
    {
        return out(data, 1, false, fout);
    }

    public static void outB(ByteArrayOutputStream out, short data, boolean swap)
    {
        // then write data
        if (swap)
        {
            out.write(data >> 8);
            out.write(data >> 0);
        }
        else
        {
            out.write(data >> 0);
            out.write(data >> 8);
        }
    }

    public static void outB(ByteArrayOutputStream out, short data)
    {
        outB(out, data, false);
    }

    public static void outB(ByteArrayOutputStream out, int data, boolean swap)
    {
        // then write data
        if (swap)
        {
            out.write(data >> 24);
            out.write(data >> 16);
            out.write(data >> 8);
            out.write(data >> 0);
        }
        else
        {
            out.write(data >> 0);
            out.write(data >> 8);
            out.write(data >> 16);
            out.write(data >> 24);
        }
    }

    public static void outB(ByteArrayOutputStream out, int data)
    {
        outB(out, data, false);
    }

    public static boolean align(ByteArrayOutputStream out, int align)
    {
        // no way to know actual fill size so we reset binary data
        if (align > 2)
        {
            out.reset();
            return false;
        }

        // we can only handle align 2
        if (align == 2)
        {
            // do alignment
            while ((out.size() % align) != 0)
                out.write(0);
        }

        return true;
    }

    public static void outB(ByteArrayOutputStream out, byte[] data, int align) throws IOException
    {
        // align ok ? --> then write data
        if (align(out, align))
            out.write(data);
    }

    public static void outB(ByteArrayOutputStream out, byte[] data) throws IOException
    {
        outB(out, data, 0);
    }

    public static PackedData pack(byte[] data, Compression compression, ByteArrayOutputStream bin)
    {
        // nothing to do
        if (compression == Compression.NONE)
            return new PackedData(data, Compression.NONE);

        // LZ4W compression with byte size ? do it quickly :)
        if (compression == Compression.LZ4W)
        {
            final byte[] result = lz4wpack((bin != null) ? bin.toByteArray() : null, data);

            // error or no compression possible ? return origin data
            if ((result == null) || (result.length >= data.length))
                return new PackedData(data, Compression.NONE);

            // return compressed result
            return new PackedData(result, Compression.LZ4W);
        }

        // we don't count AUTO
        final byte[][] results = new byte[Compression.values().length - 1][];
        final int[] sizes = new int[Compression.values().length - 1];
        final byte[] prevData;

        // init no compression info
        results[0] = data;
        sizes[0] = data.length;

        // create prev data from bin stream (for LZ4W compression)
        if ((bin != null) && (bin.size() > 1))
            prevData = bin.toByteArray();
        else
            prevData = null;

        // if ((bin != null) && (bin.size() > 1))
        // {
        // if (!out(bin.toByteArray(), "prev.in"))
        // return null;
        // }
        // // create out file from data input
        // if (!out(data, "pack.in"))
        // return null;

        // init results
        for (int i = 1; i < results.length; i++)
            results[i] = null;

        // select compression scheme
        for (Compression comp : Compression.values())
        {
            // ignore AUTO and NONE
            if ((comp == Compression.AUTO) || (comp == Compression.NONE))
                continue;

            if ((compression == Compression.AUTO) || (compression == comp))
            {
                final int compIndex = comp.ordinal() - 1;
                byte[] out;

                switch (comp)
                {
                    case APLIB:
                        out = appack(data);
                        break;

                    case LZ4W:
                        out = lz4wpack(prevData, data);
                        break;

                    default:
                        out = null;
                        break;
                }

                // correctly packed ? --> store results
                if (out != null)
                {
                    results[compIndex] = out;
                    sizes[compIndex] = out.length;
                }

                // boolean packed;
                // switch (comp)
                // {
                // case APLIB:
                // packed = appack("pack.in", "pack.out");
                // break;
                //
                // case LZ4W:
                // packed = lz4wpack((bin != null) && (bin.size() > 1) ? "prev.in" : null, "pack.in", "pack.out");
                // break;
                //
                // default:
                // packed = false;
                // break;
                // }
                //
                // // correctly packed ? --> store results
                // if (packed)
                // {
                // results[compIndex] = in("pack.out");
                // sizes[compIndex] = results[compIndex].length;
                // }
            }
        }

        // find best compression
        int minSize = sizes[0];
        byte[] result = results[0];
        Compression bestComp = Compression.NONE;

        for (Compression comp : Compression.values())
        {
            // ignore AUTO and NONE
            if ((comp == Compression.AUTO) || (comp == Compression.NONE))
                continue;

            final int compIndex = comp.ordinal() - 1;

            if (results[compIndex] != null)
            {
                if (sizes[compIndex] < minSize)
                {
                    minSize = sizes[compIndex];
                    result = results[compIndex];
                    bestComp = comp;
                }
            }
        }

        // // clean
        // if ((bin != null) && (bin.size() > 0))
        // FileUtil.delete("prev.in", false);
        // FileUtil.delete("pack.in", false);
        // FileUtil.delete("pack.out", false);

        return new PackedData(result, bestComp);
    }

    public static byte[] appack(byte[] data)
    {
        try
        {
            return APJ.pack(data, false, true);
        }
        catch (Exception e)
        {
            System.err.println(e.getMessage());
            return null;
        }
    }

    public static boolean appack(String fin, String fout)
    {
        // better to remove output file for appack
        FileUtil.delete(fout, false);

        // build complete command line
        final String[] cmd = new String[] {"java", "-jar",
                FileUtil.adjustPath(sgdk.rescomp.Compiler.currentDir, "apj.jar"), "p", fin, fout, "-s"};

        String cmdLine = "";
        for (String s : cmd)
            cmdLine += s + " ";
        System.out.println("Executing " + cmdLine);

        // execute
        final Process p = SystemUtil.exec(cmd, true);

        try
        {
            // wait for execution
            p.waitFor();
        }
        catch (InterruptedException e)
        {
            // ignore
        }

        // file exist --> ok
        return FileUtil.exists(fout);
    }

    public static byte[] lz4wpack(byte[] prev, byte[] data)
    {
        final int prevLen = (prev != null) ? prev.length : 0;
        final byte[] buf = new byte[prevLen + data.length];

        if (prev != null)
            System.arraycopy(prev, 0, buf, 0, prevLen);
        System.arraycopy(data, 0, buf, prevLen, data.length);

        try
        {
            try
            {
                return LZ4W.pack(buf, prevLen, true);
            }
            catch (IllegalArgumentException e1)
            {
                // try to pack without previous data block then
                return LZ4W.pack(data, 0, true);
            }
        }
        catch (Exception e)
        {
            System.err.println(e.getMessage());
            return null;
        }
    }

    public static boolean lz4wpack(String prev, String fin, String fout)
    {
        // better to remove output file for lz4w
        FileUtil.delete(fout, false);

        // build complete command line
        final String[] cmd = new String[] {"java", "-jar",
                FileUtil.adjustPath(sgdk.rescomp.Compiler.currentDir, "lz4w.jar"), "p",
                (!StringUtil.isEmpty(prev) ? prev + "@" : "") + fin, fout, "-s"};
        // final String[] cmd = new String[] {"java", "-jar",
        // FileUtil.adjustPath(sgdk.rescomp.Compiler.currentDir, "lz4w.jar"), "p", fin, fout, "-s"};

        String cmdLine = "";
        for (String s : cmd)
            cmdLine += s + " ";
        System.out.println("Executing " + cmdLine);

        // execute
        final Process p = SystemUtil.exec(cmd, true);

        try
        {
            // wait for execution
            p.waitFor();
        }
        catch (InterruptedException e)
        {
            // ignore
        }

        // file exist --> ok
        return FileUtil.exists(fout);
    }

    public static boolean xgmtool(String fin, String fout, int timing, String options)
    {
        // better to remove output file for lz4w
        FileUtil.delete(fout, false);

        // build complete command line
        final String[] cmd = new String[] {FileUtil.adjustPath(sgdk.rescomp.Compiler.currentDir, "xgmtool"), fin, fout,
                "-s", (timing == 0) ? "-n" : ((timing == 1) ? "-p" : ""), (options != null) ? options : ""};

        String cmdLine = "";
        for (String s : cmd)
            cmdLine += s + " ";
        System.out.println("Executing " + cmdLine);

        // execute
        final Process p = SystemUtil.exec(cmd, true);

        try
        {
            // wait for execution
            p.waitFor();
        }
        catch (InterruptedException e)
        {
            // ignore
        }

        // file exist --> ok
        return FileUtil.exists(fout);
    }

    // used by DPCM compression
    static final int delta_tab[] = {-34, -21, -13, -8, -5, -3, -2, -1, 0, 1, 2, 3, 5, 8, 13, 21};

    static int getBestDeltaIndex(int wantedLevel, int curLevel)
    {
        final int wdelta = wantedLevel - curLevel;

        int ind = 0;
        int mindiff = Math.abs(wdelta - delta_tab[ind]);

        for (int i = 1; i < 16; i++)
        {
            final int diff = Math.abs(wdelta - delta_tab[i]);

            if (diff < mindiff)
            {
                mindiff = diff;
                ind = i;
            }
        }

        final int newLevel = delta_tab[ind] + curLevel;

        // check for overflow (8 bits signed)
        if (newLevel > 127)
            return ind - 1;
        if (newLevel < -128)
            return ind + 1;

        return ind;
    }

    /**
     * DPCM compression
     * 
     * @param input
     *        PCM 8 bits audio input data (should be signed)
     * @return DPCM 4 bits compressed audio data
     */
    public static byte[] dpcmPack(byte[] input)
    {
        final byte[] result = new byte[(input.length / 2) + (input.length & 1)];

        int curLevel = 0;
        int offDst = 0;
        for (int off = 0; off < input.length; off += 2)
        {
            int ind, out;

            // input is 8 bits signed
            ind = getBestDeltaIndex(input[off + 0], curLevel);
            curLevel += delta_tab[ind];
            // first nibble
            out = ind;

            // input is 8 bits signed
            ind = getBestDeltaIndex(((off + 1) < input.length) ? input[off + 1] : 0, curLevel);
            curLevel += delta_tab[ind];
            // second nibble
            out |= (ind << 4);

            // write in result
            result[offDst++] = (byte) out;
        }

        return result;
    }
}