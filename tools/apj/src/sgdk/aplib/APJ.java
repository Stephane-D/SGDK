package sgdk.aplib;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;

/**
 * aPLib compression is a LZSS based lossless compression algorithm by Jorgen Ibsen - http://www.ibsensoftware.com
 * ApLib packer/unpacker ported in java for SGDK.
 * 
 * @author Stephane Dallongeville
 */
public class APJ
{
    static class Match
    {
        final private byte[] data;
        private int index;

        int offset;
        int length;

        private int rawCost;
        private int cost;
        private int saved;

        public Match(byte[] data, int index, int offset, int length)
        {
            super();

            this.data = data;
            this.index = index;

            this.offset = offset;
            this.length = length;

            rawCost = -1;
            cost = -1;
            saved = -1;
        }

        public void incLength(int value)
        {
            index -= value;
            length += value;

            // need to reset costs and saved
            rawCost = -1;
            cost = -1;
            saved = -1;
        }

        public boolean isShortOrLong()
        {
            if ((length >= 2) && (length <= 3) && (offset > 0) && (offset < 128))
                return true;
            if (length >= 3)
                return true;

            return false;
        }

        private int computeRawCost()
        {
            int result = 0;

            for (int i = index; i < index + length; i++)
            {
                if (data[i] == 0)
                    result += 7;
                else
                    result += 9;
            }

            return result;
        }

        private int computeCost()
        {
            if ((length == 1) && (offset > 0) && (offset < 16))
                return 3 + 4;

            if ((length >= 2) && (length <= 3) && (offset > 0) && (offset < 128))
                return 3 + 8;

            if (length >= 3)
            {
                int c = 2;

                // can re-use last offset ?
                if (!wasMatch && (lastOffset == offset))
                    // minimal variable encoding cost
                    c += 2;
                else
                {
                    // cost for offset high bits (estimation)
                    c += ((getHighBitNum(offset >> 8)) * 2) + 2;
                    // cost for offset low bit
                    c += 8;
                }

                // cost for length
                c += getHighBitNum(length) * 2;

                return c;
            }

            return getRawCost();
        }

        private void updateRawCost()
        {
            rawCost = computeRawCost();
        }

        private void updateCost()
        {
            cost = computeCost();
        }

        public void updateSaved()
        {
            saved = getRawCost() - getCost();
        }

        public int getRawCost()
        {
            if (rawCost == -1)
                updateRawCost();
            return rawCost;
        }

        public int getCost()
        {
            if (cost == -1)
                updateCost();
            return cost;
        }

        public int getSaved()
        {
            if (saved == -1)
                updateSaved();
            return saved;
        }

        @Override
        public String toString()
        {
            return "Offset= " + offset + " - Length= " + length;
        }

    }

    @SuppressWarnings("serial")
    static class ByteMatchList extends ArrayList<ByteMatch>
    {
        ByteMatchList()
        {
            super();
        }
    }

    static class ByteMatch implements Comparable<ByteMatch>
    {
        final int offset;
        final int repeat;

        public ByteMatch(int offset, int repeat)
        {
            super();

            this.offset = offset;
            this.repeat = repeat;
        }

        @Override
        public String toString()
        {
            return "Off=" + offset + " - Repeat=" + repeat;
        }

        @Override
        public int compareTo(ByteMatch bm)
        {
            return Integer.compare(offset, bm.offset);
        }
    }

    // stats
    static long statMiss;
    static long statLiteral;
    static long statShortZero;
    static long statRLE1;
    static long statRLEShort;
    static long statRLELong;
    static long statTinyMatch;
    static long statShortMatch;
    static long statLongMatch;
    static long statRepeatMatch;

    static boolean wasMatch;
    static int lastOffset;

    private static Match findBestMatch(ByteMatchList[] byteMatches, byte[] data, int ind)
    {
        // nothing we can do
        if (ind < 1)
            return null;

        // get byte matches for current byte
        final ByteMatchList bml = byteMatches[data[ind] & 0xFF];
        // get number of repeat for current byte
        final int curRepeat = getRepeat(data, ind);

        // byte match index
        int mInd = 0;
        Match best = null;
        // minimum save
        int saved = 1;

        // test all byte matches
        while (mInd < bml.size())
        {
            // get byte match
            final ByteMatch bm = bml.get(mInd);

            int off = bm.offset;
            // raised current offset ? --> stop now
            if (off >= ind)
                break;

            int repeat = bm.repeat;

            // less repeat on match
            if (repeat < curRepeat)
            {
                final Match match = new Match(data, ind, ind - off, repeat + 1);

                // we use >= as we always prefer shorter offset
                if (match.getSaved() >= saved)
                {
                    best = match;
                    saved = match.getSaved();
                }
            }
            else
            {
                // more repeat on match ?
                if (repeat > curRepeat)
                {
                    // bypass extras repeats on match
                    int delta = repeat - curRepeat;
                    // fix maximum delta to not raise ind
                    if ((off + delta) >= ind)
                        delta = (ind - off) - 1;

                    off += delta;
                    repeat -= delta;
                }

                final Match match;

                // still some repeat ?
                if (repeat > 0)
                {
                    // we raised ind ? --> limit start offset to (ind - 1)
                    if ((off + repeat) >= ind)
                        repeat = (ind - off) - 1;

                    // easy optimization
                    match = getMatch(data, off + repeat, ind + repeat);
                    // adjust match length
                    match.incLength(repeat);
                }
                else
                    match = getMatch(data, off, ind);

                // we use >= as we always prefer shorter offset
                if (match.getSaved() >= saved)
                {
                    best = match;
                    saved = match.getSaved();
                }
            }

            // next byte match
            mInd++;
        }

        return best;
    }

    private static Match getMatch(byte[] data, int from, int ind)
    {
        int refOffset;
        int curOffset;
        int len;

        // test on simple copy
        refOffset = from;
        curOffset = ind;
        len = 0;
        while ((curOffset < data.length) && (data[refOffset++] == data[curOffset++]))
            len++;

        return new Match(data, ind, ind - from, len);
    }

    private static int getRepeat(byte[] data, int ind)
    {
        final byte value = data[ind];

        int off = ind + 1;
        while ((off < data.length) && (data[off] == value))
            off++;

        return (off - ind) - 1;
    }

    private static int getLengthDelta(int offset)
    {
        if ((offset < 0x80) || (offset >= 0x7D00))
            return 2;
        if (offset >= 0x500)
            return 1;
        return 0;
    }

    static int getHighBitNum(int value)
    {
        int result = 0;
        int v = value;

        if (v >= 0x10000)
        {
            result += 16;
            v >>= 16;
        }
        if (v >= 0x100)
        {
            result += 8;
            v >>= 8;
        }
        if (v >= 0x10)
        {
            result += 4;
            v >>= 4;
        }
        if (v >= 4)
        {
            result += 2;
            v >>= 2;
        }
        if (v >= 2)
            result++;

        // // find highest bit
        // while (v > 1)
        // {
        // v >>= 1;
        // result++;
        // }

        return result;
    }

    private static int readVariableNumber(BitReader stream)
    {
        int result = 1;

        do
            result = (result << 1) + stream.readBit();
        while (stream.readBit() != 0);

        // value is obtained using each even bit and end when we meet 0 on odd bit:
        // x0 = 1 bit encoded value
        // x1x0 = 2 bit encoded value
        // x1x1x1x1x0 = 5 bit encoded value corresponding to xxxxx
        // result = xxx + 2;

        return result;
    }

    private static int readFixedNumber(BitReader stream, int nbit)
    {
        int result = 0;
        int i = nbit;

        while (i-- > 0)
        {
            result <<= 1;
            result |= stream.readBit();
        }

        return result;
    }

    private static void writeVariableNumber(BitWriter stream, int value)
    {
        if (value < 2)
        {
            System.out.println("Encoding error");
            return;
        }

        // find highest bit
        int nbit = getHighBitNum(value);
        // we don't need to write first high bit as it's always 1
        nbit--;

        // then write remaining bit
        stream.writeBit((value >> nbit) & 1);
        while (nbit-- > 0)
        {
            stream.writeBit(1);
            stream.writeBit((value >> nbit) & 1);
        }
        stream.writeBit(0);
    }

    private static void writeFixedNumber(BitWriter stream, int value, int nbit)
    {
        for (int i = nbit - 1; i >= 0; i--)
            stream.writeBit((value >> i) & 1);
    }

    private static void writeLiteral(BitWriter stream, int data)
    {
        stream.writeBit(0);
        stream.writeUByte(data);
        wasMatch = false;
        statLiteral++;
    }

    private static void writeTinyBlock(BitWriter stream, int offset)
    {
        // should not be the case
        if ((offset < 0) || (offset > 15))
        {
            System.out.println("Encoding error");
            return;
        }

        // tiny block signature
        stream.writeBit(1);
        stream.writeBit(1);
        stream.writeBit(1);

        // single byte data (111oooo)
        writeFixedNumber(stream, offset, 4);

        wasMatch = false;
        statTinyMatch++;
        if (offset == 0)
            statShortZero++;
        if (offset == 1)
            statRLE1++;
    }

    private static void writeShortBlock(BitWriter stream, int offset, int length)
    {
        // should not be the case
        if ((offset < 1) || (offset > 127))
        {
            System.out.println("Encoding error");
            return;
        }
        // should not be the case
        if ((length < 2) || (length > 3))
        {
            System.out.println("Encoding error");
            return;
        }

        // short block signature
        stream.writeBit(1);
        stream.writeBit(1);
        stream.writeBit(0);

        // single byte data (oooooool)
        stream.writeUByte((offset << 1) | (length - 2));

        lastOffset = offset;
        wasMatch = true;
        statShortMatch++;
        if (offset == 1)
            statRLEShort++;
    }

    private static void writeBlock(BitWriter stream, int offset, int length)
    {
        // block signature
        stream.writeBit(1);
        stream.writeBit(0);

        // if the last operations were literal or single byte
        // and the offset is unchanged since the last block copy
        // we can just store a 'null' offset and the length
        if (!wasMatch && (lastOffset == offset))
        {
            // minimal variable number (means 0)
            writeVariableNumber(stream, 2);
            writeVariableNumber(stream, length);
            statRepeatMatch++;
            if (offset == 1)
                statRLELong++;
        }
        else
        {
            int highOffset = (offset >> 8) + 2;
            int lowOffset = offset & 0xFF;

            if (!wasMatch)
                highOffset++;

            writeVariableNumber(stream, highOffset);
            stream.writeUByte(lowOffset);
            writeVariableNumber(stream, length - getLengthDelta(offset));
            statLongMatch++;
            if (offset == 1)
                statRLELong++;
        }

        lastOffset = offset;
        wasMatch = true;
    }

    private static void writeEndBlock(BitWriter stream)
    {
        // same as short block but with null offset
        stream.writeBit(1);
        stream.writeBit(1);
        stream.writeBit(0);

        // oooooool: offset == 0 --> end
        stream.writeUByte(0);
    }

    /**
     * Pack data using the ApLib algorithm.
     *
     * @param data
     *        data to pack
     * @return ApLib compressed data
     * @throws IOException
     * @throws IllegalArgumentException
     *         Cannot be packed using previous data block (try to pack without previous data block)
     */
    public static byte[] pack(byte[] data, boolean ultra, boolean silent) throws IOException, IllegalArgumentException
    {
        // data length
        final int len = data.length;
        // not enough data to try compression..
        if (len < 2)
            return data;

        final BitWriter result = new BitWriter(data.length);

        // PASS 1: build the byte matches table
        final ByteMatchList[] byteMatches = new ByteMatchList[0x100];
        for (int i = 0; i < byteMatches.length; i++)
            byteMatches[i] = new ByteMatchList();

        // current offset to start counting matches
        int mOffset = 0;
        while (mOffset < data.length)
        {
            final int off = mOffset;
            final byte val = data[mOffset++];

            int repeat = 0;
            while ((mOffset < data.length) && (data[mOffset] == val))
            {
                repeat++;
                mOffset++;
            }

            // need to unsign val / add new byte match
            byteMatches[val & 0xFF].add(new ByteMatch(off, repeat));
        }

        // sort all ByteMatch list
        for (int i = 0; i < byteMatches.length; i++)
            Collections.sort(byteMatches[i]);

        // PASS 2: get best match for each source position using the matches table
        wasMatch = false;
        lastOffset = -1;
        final Match[] matches = new Match[data.length];

        if (ultra)
        {
            // ultra compression mode (very slow)
            for (int i = 1; i < matches.length; i++)
                matches[i] = findBestMatch(byteMatches, data, i);
        }
        else
        {
            // fast compression mode
            for (int i = 1; i < matches.length;)
            {
                final Match match = findBestMatch(byteMatches, data, i);

                matches[i] = match;

                if (match != null)
                {
                    i += matches[i].length;

                    if (match.isShortOrLong())
                    {
                        lastOffset = match.offset;
                        wasMatch = true;
                    }
                    else
                        wasMatch = false;
                }
                else
                {
                    i++;
                    wasMatch = false;
                }
            }
        }

        // meaningful only in ultra mode
        if (ultra)
        {
            // PASS 3: walk backward in matches and find optimal match length
            final int costs[] = new int[matches.length + 1];

            // initialize ending cost
            costs[matches.length] = 0;

            for (int i = matches.length - 1; i >= 0; i--)
            {
                // literal cost = next cost + 1
                final int literalCost = costs[i + 1] + ((data[i] == 0) ? 7 : 9);
                // default match cost
                int matchCost = Integer.MAX_VALUE;

                final Match match = matches[i];

                // we have a match ? its cost = current match cost + cost after match sequence
                if (match != null)
                    matchCost = match.getCost() + costs[i + match.length];

                // literal cost is cheaper than match cost ?
                if (literalCost < matchCost)
                {
                    // change the match to a literal as it is more efficient
                    costs[i] = literalCost;
                    matches[i] = null;
                }
                else
                    costs[i] = matchCost;
            }
        }

        // clear stats
        statMiss = 0;
        statLiteral = 0;
        statTinyMatch = 0;
        statShortMatch = 0;
        statLongMatch = 0;
        statRepeatMatch = 0;
        statRLE1 = 0;
        statRLEShort = 0;
        statRLELong = 0;
        statShortZero = 0;

        // PASS 4: build compressed data from optimal matches
        lastOffset = 0;
        wasMatch = false;

        // always write first byte as it
        result.writeUByte(data[0]);
        statLiteral++;

        int ind = 1;
        while (ind < matches.length)
        {
            boolean done = false;
            final Match match = matches[ind];

            // match found ?
            if (match != null)
            {
                final int off = match.offset;
                final int l = match.length;
                done = true;

                if ((l == 1) && (off > 0) && (off < 16))
                    writeTinyBlock(result, off);
                else if ((l >= 2) && (l <= 3) && (off > 0) && (off < 128))
                    writeShortBlock(result, off, l);
                else if (l >= 3)
                    writeBlock(result, off, l);
                else
                {
                    // long offset with very small match --> can't encode...
                    done = false;
                    System.out.println("Can't encode match: ind=" + ind + " - offset=" + off + " - len=" + l);
                    statMiss++;
                }

                if (done)
                    ind += l;
            }

            // no match found --> just add to literal buffer
            if (!done)
            {
                final byte v = data[ind++];

                if (v == 0)
                    writeTinyBlock(result, 0);
                else
                    writeLiteral(result, v);
            }
        }

        // end mark
        writeEndBlock(result);

        if (!silent)
        {
            System.out.println("Stats:");
            System.out.println("Literal: " + statLiteral);
            System.out.println(
                    "Tiny matches: " + statTinyMatch + " - short zero: " + statShortZero + " - RLE1: " + statRLE1);
            System.out.println("Short matches: " + statShortMatch + " - RLE short: " + statRLEShort);
            System.out.println("Long matches: " + statLongMatch + " - RLE long: " + statRLELong);
            System.out.println("Repeat matches: " + statRepeatMatch);
            System.out.println("Miss(es): " + statMiss);
        }

        return result.toByteArray();
    }

    /**
     * Unpack data using the ApLib algorithm.
     *
     * @param data
     *        data to unpack
     * @return unpacked data
     * @throws IOException
     */
    public static byte[] unpack(byte[] data, byte[] verif)
    {
        final BitReader source = new BitReader(data);
        final DynamicByteArray result = new DynamicByteArray();
        int offset, len;
        boolean done;

        lastOffset = 0;
        wasMatch = false;
        done = false;

        // first byte always unpacked
        result.write(source.readUByte());

        try
        {
            while (!done)
            {
                // 1xx
                if (source.readBit() == 1)
                {
                    // 11x
                    if (source.readBit() == 1)
                    {
                        // 111: 1 byte match with 4bit offset (0000 offset = 0 value)
                        if (source.readBit() == 1)
                        {
                            // 111oooo : 4 bit for offset
                            offset = readFixedNumber(source, 4);

                            // offset != 0 ? -->copy 1 byte from -offset
                            if (offset != 0)
                                result.writeCheck(result.readBackUByte(offset), verif);
                            // offset == 0 ? --> fill 1 byte to 0
                            else
                                result.writeCheck(0, verif);

                            // not a short/long match
                            wasMatch = false;
                        }
                        else
                        // 110: 2/3 bytes match with 7 bit offset (0000000 offset = end of stream)
                        {
                            // read 1 byte : oooooool
                            // offset = b7-b1 (7 bits)
                            // len = 2 + b0

                            offset = source.readUByte();
                            len = 2 + (offset & 1);
                            offset >>= 1;

                            // offset != 0 ?
                            if (offset != 0)
                            {
                                // copy len byte from offset
                                while (len-- > 0)
                                    result.writeCheck(result.readBackUByte(offset), verif);
                            }
                            // offset == 0 --> end of stream !
                            else
                                done = true;

                            // store last offset
                            lastOffset = offset;
                            // last was match
                            wasMatch = true;
                        }
                    }
                    else
                    // 10x: variable encoded offset / length
                    {
                        // get variable encoded offset
                        offset = readVariableNumber(source);

                        // last was not a match and minimal variable offset ? --> use stored offset
                        if (!wasMatch && (offset == 2))
                        {
                            // take saved offset
                            offset = lastOffset;

                            // take variable encoded len (2+x)
                            len = readVariableNumber(source);

                            // copy len byte from offset
                            while (len-- > 0)
                                result.writeCheck(result.readBackUByte(offset), verif);
                        }
                        // long offset and variable length
                        else
                        {
                            // last wasn't match ? offset -= 3 else offset -= 2
                            if (!wasMatch)
                                offset -= 3;
                            else
                                offset -= 2;

                            // offset = (offset << 8) | next byte
                            offset <<= 8;
                            offset |= source.readUByte();

                            // take variable encoded len (2+x)
                            len = readVariableNumber(source);

                            // adjust len depending offset size
                            if (offset >= 32000)
                                len++;
                            if (offset >= 1280)
                                len++;
                            if (offset < 128)
                                len += 2;

                            // copy len byte from offset
                            while (len-- > 0)
                                result.writeCheck(result.readBackUByte(offset), verif);

                            // store offset
                            lastOffset = offset;
                        }

                        // last was match
                        wasMatch = true;
                    }
                }
                else
                // 0xx
                {
                    // literal (single byte)
                    result.writeCheck(source.readUByte(), verif);
                    // short match or literal
                    wasMatch = false;
                }
            }
        }
        catch (Exception e)
        {
            System.out.println("Error at source[" + source.offset + "] / dest[" + result.size() + "]:");
            throw e;
        }

        return result.toByteArray();
    }
}
