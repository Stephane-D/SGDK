import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;

public class LZ4W
{
    final static boolean ENABLE_STATS_EXT = true;

    final static int LITERAL_LENGTH_MASK = 0xF000;
    final static int LITERAL_LENGTH_SFT = 12;
    final static int MATCH_LENGTH_MASK = 0x0F00;
    final static int MATCH_LENGTH_SFT = 8;
    final static int MATCH_OFFSET_MASK = 0x00FF;
    final static int MATCH_OFFSET_SFT = 0;
    final static int MATCH_LONG_LENGTH_MASK = MATCH_OFFSET_MASK;
    final static int MATCH_LONG_LENGTH_SFT = MATCH_OFFSET_SFT;
    final static int MATCH_LONG_OFFSET_MASK = 0xFFFE;
    final static int MATCH_LONG_OFFSET_SFT = 1;

    final static int LITERAL_MAX_SIZE = 0xF;
    final static int MATCH_MIN_SIZE = 1;
    final static int MATCH_LONG_MIN_SIZE = 2;
    final static int MATCH_MAX_SIZE = 0xF + MATCH_MIN_SIZE;
    final static int MATCH_LONG_MAX_SIZE = 0xFF + MATCH_LONG_MIN_SIZE;
    final static int MATCH_MIN_OFFSET = 1;
    final static int MATCH_LONG_MIN_OFFSET = 1;
    final static int MATCH_OFFSET_MAX = 0x0FF + MATCH_MIN_OFFSET;
    final static int MATCH_LONG_OFFSET_MAX = 0x7FFF + MATCH_LONG_MIN_OFFSET;

    // stats
    static int[] statLiteralSize;
    static int[] statMatchSize;
    static int statNumSeg;
    static long statMiss;
    static long statLiteralLen;
    static long statMatchLen;
    static long statLongMatch;

    /**
     * Pack data using the LZ4W algorithm.
     *
     * @param data
     *        data to pack
     * @param start
     *        offset (in byte) where to start packing
     * @return LZ4W compressed data
     * @throws IOException
     */
    public static byte[] pack(byte[] data, int start, boolean silent) throws IOException
    {
        final DymamicByteArray result = new DymamicByteArray(data.length);
        final DymamicByteArray literal = new DymamicByteArray(1024);

        statLiteralSize = new int[LITERAL_MAX_SIZE + 1];
        statMatchSize = new int[MATCH_LONG_MAX_SIZE + 1];
        statNumSeg = 0;
        statMiss = 0;
        statLiteralLen = 0;
        statMatchLen = 0;
        statLongMatch = 0;

        // data length (in byte)
        final int len = data.length - start;

        // enough data to try compression ?
        if (len >= 2)
        {
            // transform to word data
            final short[] wdata = byteToShort(data, false);
            // adjust offset to word data
            final int offset = start / 2;

            // PASS 1: build the word matches table
            final WordMatchList[] wordMatches = new WordMatchList[0x10000];
            for (int i = 0; i < wordMatches.length; i++)
                wordMatches[i] = new WordMatchList();

            // current offset to start counting word matches
            int wmOffset = Math.max(offset - MATCH_LONG_OFFSET_MAX, 0);
            while (wmOffset < wdata.length)
            {
                final int off = wmOffset;
                final short val = wdata[wmOffset++];

                int repeat = 0;
                while ((wmOffset < wdata.length) && (wdata[wmOffset] == val))
                {
                    repeat++;
                    wmOffset++;
                }

                // need to unsign val / add new word match
                wordMatches[val & 0xFFFF].add(new WordMatch(off, repeat));
            }

            // PASS 2: get best match for each source position using the word matches table
            final Match[] matches = new Match[wdata.length - offset];

            for (int i = 0; i < matches.length; i++)
                matches[i] = findBestMatch(wordMatches, wdata, offset + i);

            // PASS 3: walk backward in matches and find optimal match length
            final int costs[] = new int[matches.length + 1];

            // initialize ending cost
            costs[matches.length] = 0;

            for (int i = matches.length - 1; i >= 0; i--)
            {
                // literal cost = next cost + 1
                final int literalCost = costs[i + 1] + 1;
                // default match cost
                int matchCost = Integer.MAX_VALUE;

                final Match match = matches[i];

                // we have a match ? its cost = current match cost + cost after match sequence
                if (match != null)
                    matchCost = match.cost + costs[i + match.length];

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

            // PASS 4: build compressed data from optimal matches
            literal.reset();
            int ind = 0;
            while (ind < matches.length)
            {
                final Match match = matches[ind];

                // match found ?
                if (match != null)
                {
                    // add segment
                    addSegment(result, literal, match, data);
                    // adjust index
                    ind += match.length;
                    // and clear literal data
                    literal.reset();
                }
                // no match found --> just add to literal buffer
                else
                    writeWordLE(literal, wdata[offset + ind++]);
            }
        }

        // remaining literal data ?
        if (literal.size() > 0)
        {
            // last segment
            addSegment(result, literal, new Match(), data);
            // and clear literal data
            literal.reset();
        }

        // mark end with empty literal and empty match
        addSegment(result, literal, new Match(), data);

        // don't forget the last byte...
        if ((len & 1) != 0)
        {
            result.write(0x80);
            result.write(data[data.length - 1]);
        }
        else
        {
            result.write(0x00);
            result.write(0x00);
        }

        if (!silent)
        {
            System.out.println("Stats:");
            System.out.println("  Num segment = " + statNumSeg + " - " + statLongMatch + " long match(s) - " + statMiss
                    + " miss(es)");
            System.out.println("  Mean literal size = "
                    + String.format("%.2g", Double.valueOf((double) statLiteralLen / (double) statNumSeg)));
            System.out.println("  Mean match size = "
                    + String.format("%.2g", Double.valueOf((double) statMatchLen / (double) statNumSeg)));

            if (ENABLE_STATS_EXT)
            {
                System.out.print("  Literal size stat = [");
                for (int i = 0; i < statLiteralSize.length; i++)
                    System.out.print(statLiteralSize[i] + ",");
                System.out.println("]");
                System.out.print("  Match size stat = [");
                for (int i = 0; i < statMatchSize.length; i++)
                    System.out.print(statMatchSize[i] + ",");
                System.out.println("]");
            }
        }

        return result.toByteArray();
    }

    /**
     * Unpack data using the LZ4W algorithm.
     *
     * @param data
     *        data to unpack
     * @param start
     *        offset (in byte) where to start unpacking
     * @param verif
     *        data buffer to verify unpacking (optional, can be null)
     * @return LZ4W compressed data
     * @throws IOException
     */
    public static byte[] unpack(byte[] data, int start, byte[] verif, boolean silent)
    {
        final DymamicByteArray result = new DymamicByteArray(32 * 1024);

        // copy previous buffer
        result.write(data, 0, start);

        // transform to word data
        final short wdata[] = byteToShort(data, false);

        // bypass previous buffer and length
        int ind = (start / 2);
        // int ind = (start / 2) + 2;
        while (ind < (wdata.length - 1))
        {
            final int seg = swap(wdata[ind++]);

            int literalLength = (seg & LITERAL_LENGTH_MASK) >> LITERAL_LENGTH_SFT;
            int matchLength = (seg & MATCH_LENGTH_MASK) >> MATCH_LENGTH_SFT;
            int matchOffset = (seg & MATCH_OFFSET_MASK) >> MATCH_OFFSET_SFT;

            // empty data ? --> end
            if ((literalLength == 0) && (matchLength == 0) && (matchOffset == 0))
                break;

            // write literal data
            for (int i = 0; i < literalLength; i++)
                writeWordLE(result, verif, wdata[ind++]);

            // no match ?
            if (matchLength == 0)
            {
                // long match mode ?
                if (matchOffset > 0)
                {
                    // use offset as length
                    matchLength = matchOffset + MATCH_LONG_MIN_SIZE;
                    // match offset = next word
                    matchOffset = ((swap(wdata[ind++]) & MATCH_LONG_OFFSET_MASK) >> MATCH_LONG_OFFSET_SFT)
                            + MATCH_LONG_MIN_OFFSET;
                }
            }
            else
            {
                // adjust match legth and offset
                matchLength += MATCH_MIN_SIZE;
                matchOffset += MATCH_MIN_OFFSET;
            }

            // we have match data ?
            if (matchLength != 0)
            {
                int offset = result.size() - (matchOffset * 2);
                int value;

                for (int i = 0; i < matchLength; i++)
                {
                    value = (result.read(offset++) & 0xFF) << 0;
                    value |= (result.read(offset++) & 0xFF) << 8;
                    writeWordLE(result, verif, value);
                }
            }
        }

        // last byte
        final int value = swap(wdata[ind]);
        if ((value & 0x8000) != 0)
            result.write(value & 0xFF);

        final byte[] resultArray = result.toByteArray();
        final byte[] resultWanted = new byte[resultArray.length - start];

        // only keep unpacked part of result
        System.arraycopy(resultArray, start, resultWanted, 0, resultWanted.length);

        return resultWanted;
    }

    private static Match findBestMatch(WordMatchList[] wordMatches, short[] wdata, int ind)
    {
        // nothing we can do
        if (ind < 1)
            return null;

        // get word matches for current word
        final WordMatchList wml = wordMatches[wdata[ind] & 0xFFFF];
        final int offMin = Math.max(0, ind - MATCH_LONG_OFFSET_MAX);

        // not really useful (only for > 64KB file which rarely happen on Megadrive)
        // int wmlInd = Collections.binarySearch(wml, new WordMatch(offMin, 0));
        // if (wmlInd < 0)
        // wmlInd = (-wmlInd) - 1;

        // find starting index in word match list
        int wmlInd = 0;
        while (wmlInd < wml.size())
        {
            final WordMatch wm = wml.get(wmlInd);

            // accepted offset ? --> start here
            if ((wm.offset + wm.repeat) >= offMin)
                break;

            wmlInd++;
        }

        Match best = null;
        int savedWord = 0;

        // for all accepted word matches
        while (wmlInd < wml.size())
        {
            final WordMatch wm = wml.get(wmlInd);

            int off = wm.offset;
            int repeat = wm.repeat;

            // raised current offset ? --> stop now
            if (off >= ind)
                break;

            // need to adjust start ?
            if (off < offMin)
            {
                repeat -= offMin - off;
                off = offMin;
            }

            // for each repeated word
            while ((repeat-- >= 0) && (off < ind))
            {
                final Match match = findBestMatchInternal(wdata, off, ind);

                if (match.savedWord > savedWord)
                {
                    best = match;
                    savedWord = match.savedWord;

                    // maximum saved ? --> don't continue
                    if (savedWord == Match.MAX_SAVED_WORD)
                        return best;
                }

                off++;
            }

            // next word match
            wmlInd++;
        }

        return best;
    }

    private static Match findBestMatchInternal(short[] wdata, int from, int ind)
    {
        final int maxLen = MATCH_LONG_MAX_SIZE;
        int refOffset;
        int curOffset;
        int len;

        // test on simple copy
        refOffset = from;
        curOffset = ind;
        len = 0;
        while ((curOffset < wdata.length) && (wdata[refOffset++] == wdata[curOffset++]) && (len < maxLen))
            len++;

        return new Match(ind, from, len);
    }

    private static void addSegment(DymamicByteArray result, DymamicByteArray literal, Match match, byte[] src)
    {
        byte[] literalArray = literal.toByteArray();
        int literalLength = literal.size() / 2;
        int literalOffset = 0;

        // literal size overflow
        while (literalLength > LITERAL_MAX_SIZE)
        {
            statNumSeg++;
            statMiss++;
            statLiteralLen += LITERAL_MAX_SIZE;
            statLiteralSize[LITERAL_MAX_SIZE]++;
            statMatchSize[0]++;

            // add literal only segment
            result.write(((LITERAL_MAX_SIZE & 0xF) << 4) | 0);
            // offset = 0 (no match)
            result.write(0);
            // write literal data (as byte so need to multiply by 2)
            result.write(literalArray, literalOffset, LITERAL_MAX_SIZE * 2);

            literalOffset += LITERAL_MAX_SIZE * 2;
            literalLength -= LITERAL_MAX_SIZE;
        }

        int matchLength = match.length;
        int matchOffset = match.getRelativeOffset();

        if ((matchLength > 0) || (literalLength > 0))
        {
            statNumSeg++;

            if (match.longMatch)
            {
                // literal length and special match marker
                result.write(((literalLength & 0xF) << 4) | 0);
                // put match len here (minimum long match len = 3)
                result.write(matchLength - MATCH_LONG_MIN_SIZE);
                // we write extended offset after literal data block

                statMatchLen += matchLength;
                statMatchSize[matchLength - MATCH_LONG_MIN_SIZE]++;
                statLongMatch++;
            }
            else
            {
                // we have match data ?
                if (matchLength != 0)
                {
                    // add literal only segment
                    result.write(((literalLength & 0xF) << 4) | ((matchLength - MATCH_MIN_SIZE) & 0xF));
                    // write match offset only if we have match data
                    result.write(matchOffset - MATCH_MIN_OFFSET);
                }
                else
                {
                    // literal length and no match
                    result.write(((literalLength & 0xF) << 4) | 0);
                    result.write(0);
                }

                statMatchLen += matchLength;
                statMatchSize[matchLength]++;

                matchLength = 0;
            }

            statLiteralLen += literalLength;
            statLiteralSize[literalLength]++;

            // write literal data (as byte so need to multiply by 2)
            result.write(literalArray, literalOffset, literalLength * 2);

            // long match ?
            if (match.longMatch)
            {
                // need to write extended offset *after* literal data
                matchOffset = ((matchOffset - MATCH_LONG_MIN_OFFSET) << MATCH_LONG_OFFSET_SFT) & MATCH_LONG_OFFSET_MASK;

                result.write(matchOffset >> 8);
                result.write(matchOffset & 0xFF);
            }
        }
        else
        {
            // end marker
            result.write(0);
            result.write(0);
        }
    }

    private static short[] byteToShort(byte[] data, boolean swap)
    {
        final int len = data.length / 2;
        final short result[] = new short[len];

        // transform to word data + last byte
        for (int i = 0; i < len; i++)
        {
            if (swap)
                result[i] = (short) readWordBE(data, 2 * i);
            else
                result[i] = (short) readWordLE(data, 2 * i);
        }

        return result;
    }

    private static int swap(int value)
    {
        int result;

        result = (value >> 8) & 0xFF;
        result |= (value & 0xFF) << 8;

        return result;
    }

    private static int readWordLE(byte[] src, int offset)
    {
        int result;

        result = (src[offset + 0] & 0xFF) << 0;
        result |= (src[offset + 1] & 0xFF) << 8;

        return result;
    }

    private static int readWordBE(byte[] src, int offset)
    {
        int result;

        result = (src[offset + 0] & 0xFF) << 8;
        result |= (src[offset + 1] & 0xFF) << 0;

        return result;
    }

    private static void writeWordLE(DymamicByteArray dst, byte[] verif, int value)
    {
        writeWordLE(dst, value);

        if (verif != null)
        {
            int offset = dst.size() - 2;

            if ((verif[offset] != dst.read(offset)) || (verif[offset + 1] != dst.read(offset + 1)))
            {
                System.out.println("Error unpacking:");
                System.out.println("DST[" + offset + "] = " + Integer.toHexString(dst.read(offset) & 0xFF)
                        + Integer.toHexString(dst.read(offset + 1) & 0xFF) + " != "
                        + Integer.toHexString(verif[offset] & 0xFF) + Integer.toHexString(verif[offset + 1] & 0xFF));
            }
        }
    }

    private static void writeWordLE(DymamicByteArray dst, int value)
    {
        dst.write(value);
        dst.write(value >> 8);
    }

    static class Match
    {
        final static int MAX_SAVED_WORD = (MATCH_LONG_MAX_SIZE - 2);

        // current offset (offset of what we want to compress)
        final int curOffset;
        // reference offset
        final int refOffset;
        final int length;
        final int savedWord;
        final int cost;
        final boolean longMatch;

        public Match(int curOff, int refOff, int len)
        {
            super();

            this.curOffset = curOff;
            this.refOffset = refOff;
            this.length = len;

            // need long match
            if ((getRelativeOffset() > MATCH_OFFSET_MAX) || (len > MATCH_MAX_SIZE))
            {
                longMatch = true;
                cost = 2;
                // minimum wanted match size for long match is 3
                savedWord = Math.max(0, len - MATCH_LONG_MIN_SIZE);
            }
            else
            {
                longMatch = false;
                cost = 1;
                // minimum wanted match size for short match is 2
                // we can have better compression with match of 1 but
                // at the expense of much more segments (slower decompression)
                savedWord = len - MATCH_MIN_SIZE;
            }
        }

        public Match()
        {
            this(0, 0, 0);
        }

        /**
         * Return relative offset (positive)
         * 
         * @return
         */
        public int getRelativeOffset()
        {
            return Math.abs(curOffset - refOffset);
        }

        @Override
        public String toString()
        {
            return String.format("off=%4X, len=%d saved=%d", Integer.valueOf(getRelativeOffset()),
                    Integer.valueOf(length), Integer.valueOf(savedWord));
        }
    }

    @SuppressWarnings("serial")
    static class WordMatchList extends ArrayList<WordMatch>
    {
        WordMatchList()
        {
            super();
        }
    }

    static class WordMatch implements Comparable<WordMatch>
    {
        final int offset;
        final int repeat;

        public WordMatch(int offset, int repeat)
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
        public int compareTo(WordMatch wm)
        {
            return Integer.compare(offset, wm.offset);
        }
    }

    static class DymamicByteArray extends ByteArrayOutputStream
    {
        public DymamicByteArray()
        {
            super();
        }

        public DymamicByteArray(int size)
        {
            super(size);
        }

        public byte read(int off)
        {
            if ((off < 0) || (off >= count))
                throw new IndexOutOfBoundsException();

            return buf[off];
        }
    }
}