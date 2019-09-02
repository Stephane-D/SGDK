package sgdk.rescomp.resource.internal;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SpriteCell.OptimizationType;

public class SpriteAnimation extends Resource
{
    public final List<SpriteFrame> frames;
    public final byte[] sequence;
    public int loopIndex;

    final int hc;

    /**
     * @param w
     *        width of image in tile
     * @param h
     *        height of image in tile
     * @param wf
     *        width of frame in tile
     * @param hf
     *        height of frame in tile
     */
    public SpriteAnimation(String id, byte[] image8bpp, int w, int h, int animIndex, int wf, int hf, int time,
            CollisionType collision, Compression compression, OptimizationType opt, long optIteration)
    {
        super(id);

        // init
        frames = new ArrayList<>();
        // default loop index
        loopIndex = 0;

        // get max number of frame
        final int numFrame = w / wf;
        final List<Integer> sequenceList = new ArrayList<>();

        for (int i = 0; i < numFrame; i++)
        {
            SpriteFrame frame = new SpriteFrame(id + "_frame" + i, image8bpp, w, h, i, animIndex, wf, hf, time,
                    collision, compression, opt, optIteration);

            // check if empty
            if (!frame.isEmpty())
            {
                // add as internal resource (get duplicate if exist)
                frame = (SpriteFrame) addInternalResource(frame);

                // search if this frame already exist in the animation
                final int ind = frames.indexOf(frame);

                // not found --> just add the frame
                if (ind == -1)
                {
                    // add frame index to sequence
                    sequenceList.add(Integer.valueOf(frames.size()));
                    // add frame
                    frames.add(frame);
                }
                else
                    // just add frame index to sequence
                    sequenceList.add(Integer.valueOf(ind));
            }
        }

        sequence = new byte[sequenceList.size()];
        for (int s = 0; s < sequence.length; s++)
            sequence[s] = sequenceList.get(s).byteValue();

        // compute hash code
        hc = loopIndex ^ Arrays.hashCode(sequence) ^ frames.hashCode();
    }

    public boolean isEmpty()
    {
        return frames.isEmpty();
    }

    public int getNumFrame()
    {
        return sequence.length;
    }

    public int getMaxNumTile()
    {
        int result = 0;

        for (SpriteFrame frame : frames)
            result = Math.max(result, frame.getNumTile());

        return result;
    }

    public int getMaxNumSprite()
    {
        int result = 0;

        for (SpriteFrame frame : frames)
            result = Math.max(result, frame.getNumSprite());

        return result;
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof SpriteAnimation)
        {
            final SpriteAnimation spriteAnim = (SpriteAnimation) obj;
            return (loopIndex == spriteAnim.loopIndex) && Arrays.equals(sequence, spriteAnim.sequence)
                    && frames.equals(spriteAnim.frames);
        }

        return false;
    }

    @Override
    public String toString()
    {
        return id + ": numFrame=" + frames.size() + " maxNumTile=" + getMaxNumTile() + " maxNumSprite="
                + getMaxNumSprite();
    }

    @Override
    public int shallowSize()
    {
        return (frames.size() * 4) + sequence.length + (sequence.length & 1);
    }

    @Override
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // frames pointer table
        Util.decl(outS, outH, null, id + "_frames", 2, false);
        for (SpriteFrame frame : frames)
            outS.println("    dc.l    " + frame.id);

        outS.println();

        // sequence data
        Util.decl(outS, outH, null, id + "_sequence", 2, false);
        // output sequence data
        Util.outS(outS, sequence, 1);

        outS.println();

        // Animation structure
        Util.decl(outS, outH, "Animation", id, 2, global);
        // set number of frame
        outS.println("    dc.w    " + frames.size());
        // set frames pointer
        outS.println("    dc.l    " + id + "_frames");
        // set size of sequence
        outS.println("    dc.w    " + sequence.length);
        // set sequence pointer
        outS.println("    dc.l    " + id + "_sequence");
        // loop info
        outS.println("    dc.w    " + loopIndex);

        outS.println();
    }
}