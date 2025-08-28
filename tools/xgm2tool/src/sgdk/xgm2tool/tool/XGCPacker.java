package sgdk.xgm2tool.tool;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import sgdk.xgm2tool.Launcher;

public class XGCPacker
{
    public final static int LITERAL_MAX_SIZE = 7;
    public final static int MATCH_MAX_SIZE = 31;
    public final static int MATCH_OFFSET_MAX = 0x100;

    public final static int FRAME_MIN_SIZE = 32;
    public final static int FRAME_MAX_SIZE = 256;
    // to lower unpacking time
    // public final static int FRAME_MAX_SIZE = 128;

    static class DynamicByteArray extends ByteArrayOutputStream
    {
        public DynamicByteArray()
        {
            super();
        }

        public DynamicByteArray(int size)
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

    static class Match
    {
        final static int MAX_SAVED = MATCH_MAX_SIZE - 1;

        // current offset (offset of what we want to compress)
        final int curOffset;
        // reference offset
        int refOffset;

        final int length;
        final int saved;
        final int cost;

        public Match(int curOff, int refOff, int len)
        {
            super();

            curOffset = curOff;
            refOffset = refOff;
            length = len;
            cost = 2;
            saved = len - cost;
        }

        /**
         * @return relative offset (positive)
         */
        public int getRelativeOffset()
        {
            return Math.abs(curOffset - refOffset);
        }

        public boolean fixForExtraByte(int offset)
        {
            final int relativeOffset = getRelativeOffset();

            // impacted ?
            if (offset < relativeOffset)
            {
                // get maximum allowed len for match
                final int maxLen = relativeOffset - offset;

                // above max len ? --> cannot be used
                if (length >= maxLen)
                {
                    if (Launcher.verbose)
                        System.out.println("Info: cannot use match (len = " + length + ") because of delay frame command.");

                    return false;
                }

                // fix offset
                refOffset--;
            }

            // not impacted
            return true;
        }

        @Override
        public String toString()
        {
            return String.format("off=%4X, len=%d saved=%d", Integer.valueOf(getRelativeOffset()), Integer.valueOf(length), Integer.valueOf(saved));
        }

    }

    static int[] literalLen = new int[LITERAL_MAX_SIZE + 1];
    static int[] matchLen = new int[MATCH_MAX_SIZE + 1];
    static int[] matchOffset = new int[MATCH_OFFSET_MAX + 2];

    private static int getRepeat(byte[] data, int ind)
    {
        final byte value = data[ind];

        int off = ind + 1;
        while ((off < data.length) && (data[off] == value))
            off++;

        return (off - ind) - 1;
    }

    private static Match findBestMatchInternal(byte[] data, int from, int ind)
    {
        int refOffset;
        int curOffset;
        int len;

        // test on simple copy
        refOffset = from;
        curOffset = ind;
        len = 0;
        while ((curOffset < data.length) && (data[refOffset++] == data[curOffset++]) && (len < MATCH_MAX_SIZE))
            len++;

        return new Match(ind, from, len);
    }

    private static Match findBestMatch(ByteMatchList[] byteMatches, byte[] data, int ind)
    {
        // nothing we can do
        if (ind < 1)
            return null;

        // get word matches for current word
        final ByteMatchList bml = byteMatches[data[ind] & 0xFF];
        // window search size = 255
        final int offMin = Math.max(0, ind - MATCH_OFFSET_MAX);

        // find starting index in word match list
        int bmlInd = 0;
        while (bmlInd < bml.size())
        {
            final ByteMatch wm = bml.get(bmlInd);

            // accepted offset ? --> start here
            if ((wm.offset + wm.repeat) >= offMin)
                break;

            bmlInd++;
        }

        // get number of repeat for current byte
        final int curRepeat = getRepeat(data, ind);

        Match best = null;
        int saved = 0;

        // for all accepted matches
        while (bmlInd < bml.size())
        {
            final ByteMatch wm = bml.get(bmlInd);

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

            // can optimize repeat ?
            if ((off >= 0) || ((off + repeat) < 0))
            {
                // less repeat on match
                if (repeat < curRepeat)
                {
                    final Match match = new Match(ind, off, Math.min(MATCH_MAX_SIZE, repeat + 1));

                    // use >= as we always prefer match to literal
                    if (match.saved >= saved)
                    {
                        best = match;
                        saved = match.saved;

                        // maximum saved ? --> don't continue
                        if (saved >= Match.MAX_SAVED)
                            return best;
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

                    Match match;

                    // still some repeat ?
                    if (repeat > 0)
                    {
                        // we raised ind ? --> limit start offset to (ind - 1)
                        if ((off + repeat) >= ind)
                            repeat = (ind - off) - 1;

                        // easy optimization (start comparing after repeat)
                        match = findBestMatchInternal(data, off + repeat, ind + repeat);
                        // then fix the match
                        match = new Match(match.curOffset - repeat, match.refOffset - repeat, Math.min(MATCH_MAX_SIZE, match.length + repeat));
                    }
                    else
                        match = findBestMatchInternal(data, off, ind);

                    // use >= as we always prefer match to literal
                    if (match.saved >= saved)
                    {
                        best = match;
                        saved = match.saved;

                        // maximum saved ? --> don't continue
                        if (saved == Match.MAX_SAVED)
                            return best;
                    }
                }
            }
            else
            {
                // for each repeated byte
                while ((repeat-- >= 0) && (off < ind))
                {
                    final Match match = findBestMatchInternal(data, off, ind);

                    // use >= as we always prefer match to literal
                    if (match.saved >= saved)
                    {
                        best = match;
                        saved = match.saved;

                        // maximum saved ? --> don't continue
                        if (saved == Match.MAX_SAVED)
                            return best;
                    }

                    off++;
                }
            }

            // next word match
            bmlInd++;
        }

        return best;
    }

    private static int getMaxFrameOffsetFor(List<Integer> frameOffsets, int startInd, int curOffset)
    {
        int ind = startInd;
        final int maxOff = curOffset + FRAME_MAX_SIZE;

        while ((ind < frameOffsets.size()) && (frameOffsets.get(ind).intValue() < maxOff))
            ind++;

        return frameOffsets.get(Math.max(--ind, startInd)).intValue();
    }

    private static void addBlock(ByteArrayOutputStream result, byte[] literal, Match match, int baseAlign) throws IOException
    {
        byte[] literalData = literal;
        final int start = result.size();
        final int blockSize = literalData.length + ((match != null) ? 2 : 1);
        final int end = start + (blockSize - 1);

        literalLen[literalData.length]++;

        // crossing page (need to split) ?
        if (((start + baseAlign) & 0xFF00) != ((end + baseAlign) & 0xFF00))
        {
            // remaining byte in first page
            final int firstPageRemain = 0x100 - ((start + baseAlign) & 0xFF);

            // write first part of literal data
            if (firstPageRemain > 1)
            {
                final int len = firstPageRemain - 1;

                // write block header with page cross marker
                result.write((len << 5) | 1);
                // write literal data
                result.write(literalData, 0, len);

                // remaining literal data
                literalData = Arrays.copyOfRange(literalData, len, literalData.length);
            }
            else
                // just write cross page marker
                result.write(1);
        }

        // match type data block
        if (match != null)
        {
            final int off = match.getRelativeOffset();

            matchLen[match.length]++;
            matchOffset[off]++;

            // literal + match length
            result.write((literalData.length << 5) | match.length);
            // copy literal data if any
            result.write(literalData);
            // write match offset *after* literal data (simpler for unpacking)
            result.write((-off) & 0xFF);

        }
        // literal only data block
        else
        {
            matchLen[0]++;

            // write literal data only if not empty
            if (literalData.length > 0)
            {
                // literal header
                result.write(literalData.length << 5);
                // copy literal data
                result.write(literalData);
            }
        }
    }

    private static void addBlock(ByteArrayOutputStream result, ByteArrayOutputStream literal, Match match, int baseAlign) throws IOException
    {
        addBlock(result, literal.toByteArray(), match, baseAlign);
        // reset literal buffer
        literal.reset();
    }

    public static byte[] pack(byte[] data, List<Integer> frameOffsets, int baseAlign) throws IOException
    {
        if ((data == null) || (data.length == 0))
            return new byte[0];

        final ByteArrayOutputStream result = new ByteArrayOutputStream();
        final ByteArrayOutputStream literal = new ByteArrayOutputStream();

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

        // PASS 2: get best match for each source position using the word matches table
        final Match[] matches = new Match[data.length];

        for (int i = 0; i < matches.length; i++)
            matches[i] = findBestMatch(byteMatches, data, i);

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
        Arrays.fill(literalLen, 0);
        Arrays.fill(matchLen, 0);
        Arrays.fill(matchOffset, 0);

        literal.reset();
        int nextFrameOffset = frameOffsets.get(0).intValue();
        int maxNextFrameOffset = Math.max(getMaxFrameOffsetFor(frameOffsets, 0, 0), nextFrameOffset);
        int ind = 0;
        int lastFrameStart = 0;
        int frameInd = 1;
        while (ind < matches.length)
        {
            final int frameSize = ind - lastFrameStart;
            Match match = matches[ind];
            final int blockSize = (match != null) ? match.length : 1;

            // get next frame start offset
            while ((nextFrameOffset < ind) && (frameInd < frameOffsets.size()))
                nextFrameOffset = frameOffsets.get(frameInd++).intValue();

            // !! fatal error (should never happen) !!
            if (frameSize >= FRAME_MAX_SIZE)
                throw new RuntimeException("Error: max frame size reached at frame #" + (frameInd - 1));
            
            // aligned on the end of frame ?
            boolean accept = (ind == nextFrameOffset);
            if (accept) 
            {
                // frame size is big enough or we reached maximum allowed offset ?
                accept &= (frameSize >= FRAME_MIN_SIZE) || (ind == maxNextFrameOffset); 
                
                if (accept)
                {
                    if (frameSize > 128)
                    {
                        if (Launcher.verbose)
                            System.out.println("Warning: large frame at #" + (frameInd - 1) + " - size = " + frameSize);
                    }
    
                    // literal not empty ? --> add literal block first
                    if (literal.size() > 0)
                        addBlock(result, literal, null, baseAlign);
    
                    // add end frame unpack block marker
                    result.write(0);
                    lastFrameStart = ind;
    
                    // get max next end frame offset
                    maxNextFrameOffset = getMaxFrameOffsetFor(frameOffsets, frameInd, ind);
                    // get next end frame offset
                    while ((nextFrameOffset <= ind) && (frameInd < frameOffsets.size()))
                        nextFrameOffset = frameOffsets.get(frameInd++).intValue();
                }
            }
            
            // was not accepted ?
            if (!accept)
            {
                // above max frame offset ? --> cancel match so we can stop on next frame
                if ((ind + blockSize) > maxNextFrameOffset)
                {
                    if (Launcher.verbose)
                        System.out.println("Info: cannot use match at offset =" + ind + " (len = " + blockSize + ") because of frame end alignement");

                    match = null;
                }
            }

            // match found ?
            if (match != null)
            {
                // add a new block
                addBlock(result, literal, match, baseAlign);
                // next ind
                ind += match.length;
            }
            // no match found --> add to literal buffer
            else
            {
                // max size for literal ? --> flush it
                if (literal.size() >= LITERAL_MAX_SIZE)
                    addBlock(result, literal, null, baseAlign);

                // write to literal and pass to next ind
                literal.write(data[ind++]);
            }
        }

        // literal not empty ? --> add last literal block
        if (literal.size() > 0)
            addBlock(result, literal, null, baseAlign);

        // end marker
        result.write(0);

        if (Launcher.verbose)
        {
            System.out.println("XGC block compressed to " + result.size() + " bytes (original = " + data.length + ")");
            System.out.println("Compression stats:");
            System.out.print("\t\t");
            for (int i = 0; i < Math.max(Math.max(LITERAL_MAX_SIZE + 1, MATCH_MAX_SIZE + 1), MATCH_OFFSET_MAX + 2); i++)
                System.out.print(i + "\t");
            System.out.println();
            System.out.print("Literals:\t");
            for (int i = 0; i < LITERAL_MAX_SIZE + 1; i++)
                System.out.print(literalLen[i] + "\t");
            System.out.println();
            System.out.print("Matches:\t");
            for (int i = 0; i < MATCH_MAX_SIZE + 1; i++)
                System.out.print(matchLen[i] + "\t");
            System.out.println();
            System.out.print("Offsets:\t");
            for (int i = 0; i < MATCH_OFFSET_MAX + 2; i++)
                System.out.print(matchOffset[i] + "\t");
            System.out.println();
        }

        return result.toByteArray();
    }

    public static byte[] unpack(byte[] data, byte[] verif)
    {
        final DynamicByteArray result = new DynamicByteArray();

        boolean end = false;
        int ind = 0;
        while (ind < data.length)
        {
            final byte block = data[ind++];
            final int litSize = (block >> 5) & 0x07;
            final int matSize = (block >> 0) & 0x1F;

            // get offset
            final int matOff = (matSize > 1) ? (data[ind++] & 0xFF) : 0;

            // write literal
            for (int i = 0; i < litSize; i++)
            {
                result.write(data[ind++]);
                if ((verif != null) && (verif[result.size() - 1] != result.read(result.size() - 1)))
                    System.out.println("Error while unpacking - offset = " + (result.size() - 1));
            }

            if (matSize > 1)
            {
                int off = result.size() - matOff;
                for (int i = 0; i < matSize; i++)
                {
                    result.write(result.read(off++));
                    if ((verif != null) && (verif[result.size() - 1] != result.read(result.size() - 1)))
                        System.out.println("Error while unpacking - offset = " + (result.size() - 1));
                }
            }
        }

        if (Launcher.verbose)
            System.out.println("Unpacked = " + result.size() + " - packed = " + data.length);

        return result.toByteArray();
    }
}
