package sgdk.rescomp.resource.internal;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SpriteCell.OptimizationLevel;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.ImageUtil;

public class SpriteAnimation extends Resource
{
    public final List<SpriteFrame> frames;
    public final Set<SpriteFrame> frameSet;
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
     * @param showCuttingResult
     */
    public SpriteAnimation(String id, byte[] image8bpp, int w, int h, int animIndex, int wf, int hf, int time, CollisionType collision, Compression compression,
            OptimizationType optType, OptimizationLevel optLevel)
    {
        super(id);

        // init
        frames = new ArrayList<>();
        frameSet = new HashSet<>();
        // default loop index
        loopIndex = 0;

        final Dimension imageDim = new Dimension(w * 8, h * 8);
        // get max number of frame
        final int maxFrame = w / wf;

        // find last non transparent frame
        int f = maxFrame - 1;
        while (f >= 0)
        {
            // define frame bounds
            final Rectangle frameBounds = new Rectangle((f * wf) * 8, (animIndex * hf) * 8, wf * 8, hf * 8);
            // not transparent ? --> stop here
            if (!ImageUtil.isTransparent(image8bpp, imageDim, frameBounds))
                break;
            
            f--;
        }
        
        // number of frame to process
        final int numFrame = f + 1;
        
        for (int i = 0; i < numFrame; i++)
        {
            // define frame bounds
            final Rectangle frameBounds = new Rectangle((i * wf) * 8, (animIndex * hf) * 8, wf * 8, hf * 8);
            // get image for this frame
            final byte[] frameImage = ImageUtil.getSubImage(image8bpp, new Dimension(w * 8, h * 8), frameBounds);
            
            // FIXME: why are we doing that ? duplicate resource optimization should be enough
            // // try to search for duplicated frame first
            // SpriteFrame frame = findExistingSpriteFrame(frameImage, frameBounds.getSize(), time, collision,
            // compression);
            //
            // // not found ? --> define new frame
            // if (frame == null)
            // {
            // frame = new SpriteFrame(id + "_frame" + i, frameImage, wf, hf, time, collision, compression, opt,
            // optIteration);
            // }
            // else
            // {
            // System.out.println("Sprite frame at anim #" + animIndex + " frame #" + i + " is a duplicate of " +
            // frame.id);
            // }
            //
            // // add frame
            // frames.add(frame);

            // create sprite frame
            SpriteFrame frame = new SpriteFrame(id + "_frame" + i, frameImage, wf, hf, time, collision, compression, optType, optLevel);
            // add as internal resource (get duplicate if exist)
            frame = (SpriteFrame) addInternalResource(frame);
            
            // add the new sprite frame
            frames.add(frame);
            frameSet.add(frame);
        }

        if (frames.size() > 255)
            throw new IllegalArgumentException("Sprite animation '" + id + "' has " + frames.size() + " frames (max = 255)");

        // compute hash code
        hc = loopIndex ^ frames.hashCode();
    }

    private SpriteFrame findExistingSpriteFrame(byte[] frameImage, Dimension dimension, int time, CollisionType collision, Compression compression)
    {
        for (Resource res : Compiler.getResources(SpriteFrame.class))
        {
            final SpriteFrame spriteFrame = (SpriteFrame) res;

            if (checkEqual(spriteFrame, frameImage, dimension, time, collision, compression))
                return spriteFrame;
        }

        return null;
    }

    private boolean checkEqual(SpriteFrame spriteFrame, byte[] frameImage, Dimension dimension, int timer, CollisionType collision, Compression compression)
    {
        return (SpriteFrame.computeFastHashcode(frameImage, dimension, timer, collision, compression) == spriteFrame.fhc)
                && Arrays.equals(frameImage, spriteFrame.frameImage) && compression.equals(spriteFrame.compression) && ((collision == spriteFrame.collisionType)
                        || ((collision != null) && (collision.equals(spriteFrame.collisionType))) && (timer == spriteFrame.timer));
    }

    public boolean isEmpty()
    {
        return frames.isEmpty();
    }

    public int getNumFrame()
    {
        return frames.size();
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
            return (loopIndex == spriteAnim.loopIndex) && frames.equals(spriteAnim.frames);
        }

        return false;
    }

    @Override
    public List<Bin> getInternalBinResources()
    {
        return new ArrayList<>();
    }

    @Override
    public String toString()
    {
        return id + ": numFrame=" + frames.size() + " maxNumTile=" + getMaxNumTile() + " maxNumSprite=" + getMaxNumSprite();
    }

    @Override
    public int shallowSize()
    {
        return (frames.size() * 4) + 1 + 1 + 4;
    }

    @Override
    public int totalSize()
    {
        int result = 0;

        for (SpriteFrame frame : frameSet)
            result += frame.totalSize();

        return result + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // frames pointer table
        Util.decl(outS, outH, null, id + "_frames", 2, false);
        for (SpriteFrame frame : frames)
            outS.append("    dc.l    " + frame.id + "\n");

        outS.append("\n");

        // Animation structure
        Util.decl(outS, outH, "Animation", id, 2, global);
        // set number of frame and loop info
        outS.append("    dc.w    " + ((frames.size() << 8) | ((loopIndex << 0) & 0xFF)) + "\n");
        // set frames pointer
        outS.append("    dc.l    " + id + "_frames\n");

        outS.append("\n");
    }
}