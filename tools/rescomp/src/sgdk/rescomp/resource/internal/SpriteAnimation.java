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
    public SpriteAnimation(String id, byte[] image8bpp, int w, int h, int animIndex, int wf, int hf, int[] time, CollisionType collision, Compression compression,
            OptimizationType optType, OptimizationLevel optLevel, boolean optDuplicate)
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

            int duplicate = 0;
            // duplicate optimization enabled ?
            if (optDuplicate)
            {
                // search for duplicate consecutive frames
                for (int j = i + 1; j < numFrame; j++)
                {
                    final Rectangle nextBounds = new Rectangle((j * wf) * 8, (animIndex * hf) * 8, wf * 8, hf * 8);
                    final byte[] nextImage = ImageUtil.getSubImage(image8bpp, new Dimension(w * 8, h * 8), nextBounds);
    
                    // different ? --> stop here
                    if (!Arrays.equals(frameImage, nextImage))
                        break;
                    
                    // found duplicate
                    duplicate++;
                }
            }
            
            // try to search for a duplicated sprite mask so we can re-use the previous sprite cutting
            SpriteFrame frame = findMatchingSpriteFrameMask(frameImage, frameBounds.getSize());
            // found it ?
            if (frame != null)
            {
            	// create sprite frame ('timer' is augmented by number of duplicate) and re-use previous sprite cutting
            	frame = new SpriteFrame(id + "_frame" + i, frameImage, wf, hf, time[Math.min(time.length - 1, i)] * (duplicate + 1), collision, compression, frame.getSprites());
            }
            else
            {
            	// create sprite frame ('timer' is augmented by number of duplicate)
            	frame = new SpriteFrame(id + "_frame" + i, frameImage, wf, hf, time[Math.min(time.length - 1, i)] * (duplicate + 1), collision, compression, optType, optLevel);
            }
            // add as internal resource (get duplicate if exist)
            frame = (SpriteFrame) addInternalResource(frame);
            // bypass duplicates
            i += duplicate;

            // add the new sprite frame
            frames.add(frame);
            frameSet.add(frame);
        }

        if (frames.size() > 255)
            throw new IllegalArgumentException("Sprite animation '" + id + "' has " + frames.size() + " frames (max = 255)");

        // compute hash code
        hc = loopIndex ^ frames.hashCode();
    }

    private SpriteFrame findMatchingSpriteFrameMask(byte[] frameImage, Dimension dimension)
    {
        for (Resource res : Compiler.getResources(SpriteFrame.class))
        {
            final SpriteFrame spriteFrame = (SpriteFrame) res;

            if (checkMaskEqual(spriteFrame, frameImage, dimension))
                return spriteFrame;
        }

        return null;
    }

    private boolean checkMaskEqual(SpriteFrame spriteFrame, byte[] frameImage, Dimension dimension)
    {
    	if (!spriteFrame.frameDim.equals(dimension))
    		return false;

    	final byte[] frame1 = spriteFrame.frameImage;
		final byte[] frame2 = frameImage;
    	
		if (frame1.length != frame2.length)
    		return false;
    	
    	for(int i = 0; i < frame1.length; i++)
    	{
    		boolean p1 = frame1[i] != 0;
    		boolean p2 = frame2[i] != 0;
    		if (p1 != p2) return false;
    	}
    	
    	return true;
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