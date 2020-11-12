package sgdk.rescomp.resource.internal;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.SpriteCutter;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics;
import sgdk.rescomp.type.Basics.CollisionBase;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SpriteCell;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.ImageUtil;

public class SpriteFrame extends Resource
{
    public static final int DEFAULT_SPRITE_OPTIMIZATION_NUM_ITERATION = 500000;

    public final SpriteFrameInfo frameInfo;
    public final SpriteFrameInfo frameInfoH;
    public final SpriteFrameInfo frameInfoV;
    public final SpriteFrameInfo frameInfoHV;
    public final Tileset tileset;
    public final int w; // width of frame in tile
    public final int h; // height of frame in tile
    public final int timer;

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
    public SpriteFrame(String id, byte[] image8bpp, int w, int h, int frameIndex, int animIndex, int wf, int hf,
            int timer, CollisionType collisionType, Compression compression, OptimizationType opt, long optIteration)
    {
        super(id);

        this.w = wf;
        this.h = hf;
        this.timer = timer;

        final int numTile = w * h;
        // define frame bounds
        final Rectangle frameBounds = new Rectangle((frameIndex * wf) * 8, (animIndex * hf) * 8, wf * 8, hf * 8);
        // get image for this frame
        final byte[] frameImage = ImageUtil.getSubImage(image8bpp, new Dimension(w * 8, h * 8), frameBounds);
        // get optimized sprite list from the image frame
        List<SpriteCell> sprites;

        // always start with the fast optimization first
        sprites = SpriteCutter.getFastOptimizedSpriteList(frameImage, frameBounds.getSize(), opt);

        // too many sprites used for this sprite ? prefer better (but slower) sprite optimization
        if ((sprites.size() > 16) || ((numTile > 64) && (sprites.size() > (numTile / 8))))
            sprites = SpriteCutter.getSlowOptimizedSpriteList(frameImage, frameBounds.getSize(), optIteration, opt);

        // above the limit of internal sprite ? force alternative optimization strategy (minimize the number of sprite)
        if ((sprites.size() > 16) && (opt != OptimizationType.MIN_SPRITE))
            sprites = SpriteCutter.getSlowOptimizedSpriteList(frameImage, frameBounds.getSize(), optIteration,
                    OptimizationType.MIN_SPRITE);

        // still above the limit (shouldn't be possible as max sprite size is 128x128) ? --> stop here :-(
        if (sprites.size() > 16)
            throw new IllegalArgumentException("Sprite frame '" + id + "' uses " + sprites.size()
                    + " internal sprites, that is above the limit (16), try to reduce the sprite size or split it...");

        // empty frame ?
        if (sprites.isEmpty())
        {
            // we can exit now, frame will be discarded anyway
            tileset = null;
            frameInfo = null;
            frameInfoH = null;
            frameInfoV = null;
            frameInfoHV = null;
            hc = 0;

            return;
        }

        int optNumTile = 0;
        for (SpriteCell spr : sprites)
            optNumTile += spr.numTile;

        // shot info about this sprite frame
        System.out
                .println("Sprite frame '" + id + "' - " + sprites.size() + " VDP sprites and " + optNumTile + " tiles");

        // build tileset
        tileset = (Tileset) addInternalResource(
                new Tileset(id + "_tileset", frameImage, wf * 8, hf * 8, sprites, compression));

        final Collision collision;

        // define collision
        if (collisionType == CollisionType.NONE)
            collision = null;
        else
        {
            CollisionBase c = null;

            switch (collisionType)
            {
                case BOX:
                    // use 75% the size of the frame for the collision
                    c = new Basics.Box(((wf * 8) * 1) / 4, ((hf * 8) * 1) / 4, ((wf * 8) * 3) / 4, ((hf * 8) * 3) / 4);
                    break;
                case CIRCLE:
                    // use 75% the size of the frame for the collision
                    c = new Basics.Circle((wf * 8) / 2, (hf * 8) / 2, ((wf * 8) * 3) / 8);
                    break;

                default:
                    break;
            }

            collision = new Collision(id + "collision", c);
        }

        // build frameInfo structures
        frameInfo = (SpriteFrameInfo) addInternalResource(new SpriteFrameInfo(id + "_base", sprites, collision));
        frameInfoH = (SpriteFrameInfo) addInternalResource(
                SpriteFrameInfo.getSpriteFrameInfo(id + "_hflip", frameInfo, wf, hf, true, false, opt));
        frameInfoV = (SpriteFrameInfo) addInternalResource(
                SpriteFrameInfo.getSpriteFrameInfo(id + "_vflip", frameInfo, wf, hf, false, true, opt));
        frameInfoHV = (SpriteFrameInfo) addInternalResource(
                SpriteFrameInfo.getSpriteFrameInfo(id + "_hvflip", frameInfo, wf, hf, true, true, opt));

        hc = (h << 0) ^ (w << 8) ^ (timer << 16) ^ tileset.hashCode() ^ frameInfo.hashCode() ^ frameInfoH.hashCode()
                ^ frameInfoV.hashCode() ^ frameInfoHV.hashCode();
    }

    public int getNumSprite()
    {
        return isEmpty() ? 0 : frameInfo.vdpSprites.size();
    }

    public boolean isEmpty()
    {
        return tileset == null;
    }

    public int getNumTile()
    {
        return isEmpty() ? 0 : tileset.getNumTile();
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof SpriteFrame)
        {
            final SpriteFrame spriteFrame = (SpriteFrame) obj;
            return (w == spriteFrame.w) && (h == spriteFrame.h) && (timer == spriteFrame.timer)
                    && tileset.equals(spriteFrame.tileset) && frameInfo.equals(spriteFrame.frameInfo)
                    && frameInfoH.equals(spriteFrame.frameInfoH) && frameInfoV.equals(spriteFrame.frameInfoV)
                    && frameInfoHV.equals(spriteFrame.frameInfoHV);
        }

        return false;
    }

    @Override
    public String toString()
    {
        return id + ": numTile=" + getNumTile() + " numSprite=" + getNumSprite();
    }

    @Override
    public int shallowSize()
    {
        return 2 + 2 + (4 * 4 * 2) + 4;
    }

    @Override
    public int totalSize()
    {
        if (isEmpty())
            return shallowSize();

        int result = frameInfo.totalSize() + frameInfoH.totalSize() + frameInfoV.totalSize() + frameInfoHV.totalSize();

        if (frameInfo.collision != null)
            result += frameInfo.collision.totalSize();
        if (frameInfoH.collision != null)
            result += frameInfoH.collision.totalSize();
        if (frameInfoV.collision != null)
            result += frameInfoV.collision.totalSize();
        if (frameInfoHV.collision != null)
            result += frameInfoHV.collision.totalSize();

        return result + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // AnimationFrame structure
        Util.decl(outS, outH, "AnimationFrame", id, 2, global);
        // number of sprite / frame width
        outS.append("    dc.w    " + ((getNumSprite() << 8) | (((w * 8) << 0) & 0xFF)) + "\n");
        // frame height / timer info
        outS.append("    dc.w    " + (((h * 8) << 8) | ((timer << 0) & 0xFF)) + "\n");

        // set vdp sprites table and collision pointers (base)
        outS.append("    dc.l    " + frameInfo.id + "_sprites\n");
        outS.append("    dc.l    " + ((frameInfo.collision != null) ? frameInfo.collision.id : "0") + "\n");
        // set vdp sprites table and collision pointers (hflip)
        outS.append("    dc.l    " + frameInfoH.id + "_sprites\n");
        outS.append("    dc.l    " + ((frameInfoH.collision != null) ? frameInfoH.collision.id : "0") + "\n");
        // set vdp sprites table and collision pointers (vflip)
        outS.append("    dc.l    " + frameInfoV.id + "_sprites\n");
        outS.append("    dc.l    " + ((frameInfoV.collision != null) ? frameInfoV.collision.id : "0") + "\n");
        // set vdp sprites table and collision pointers (hvflip)
        outS.append("    dc.l    " + frameInfoHV.id + "_sprites\n");
        outS.append("    dc.l    " + ((frameInfoHV.collision != null) ? frameInfoHV.collision.id : "0") + "\n");

        // set tileset pointer
        outS.append("    dc.l    " + tileset.id + "\n");

        outS.append("\n");
    }

}
