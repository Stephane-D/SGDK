package sgdk.rescomp.resource.internal;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.resource.Tileset;
import sgdk.rescomp.tool.SpriteCutter;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics;
import sgdk.rescomp.type.Basics.CollisionBase;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SpriteCell;
import sgdk.rescomp.type.SpriteCell.OptimizationLevel;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.ImageUtil;

public class SpriteFrame extends Resource
{
    public final List<VDPSprite> vdpSprites;
    public final Collision collision;
    public final Tileset tileset;
    public final int timer;

    final int hc;

    // just for pre-equal test
    final byte[] frameImage;
    final Dimension frameDim;
    final CollisionType collisionType;
    final Compression compression;
    final int fhc;

    public SpriteFrame(String id, byte[] frameImage8bpp, int wf, int hf, int timer, CollisionType collisionType, Compression compression, List<SpriteCell> sprites)
    {
        super(id);

        vdpSprites = new ArrayList<>();
        this.timer = timer;
        this.collisionType = collisionType;
        this.compression = compression;
        this.frameImage = frameImage8bpp;
        this.frameDim = new Dimension(wf * 8, hf * 8);
        this.fhc = computeFastHashcode(frameImage8bpp, frameDim, timer, collisionType, compression);

        // empty frame --> empty tileset
        if (sprites.isEmpty())
        {
            // shot info about this sprite frame
            System.out.println("Sprite frame '" + id + "' is empty");

            tileset = (Tileset) addInternalResource(new Tileset(id + "_tileset", false));
        }
        else
        {
            int optNumTile = 0;
            for (SpriteCell spr : sprites)
                optNumTile += spr.numTile;

            // shot info about this sprite frame
            System.out.println("Sprite frame '" + id + "' - " + sprites.size() + " VDP sprites and " + optNumTile + " tiles");

            // build tileset
            tileset = (Tileset) addInternalResource(new Tileset(id + "_tileset", frameImage, wf * 8, hf * 8, sprites, compression, false));
        }

        final Collision coll;

        // define collision
        if (collisionType == CollisionType.NONE)
            coll = null;
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

            coll = new Collision(id + "_collision", c);
        }

        // need to check that as it can be null
        if (coll != null)
            collision = (Collision) addInternalResource(coll);
        else
            collision = null;

        int ind = 0;
        for (SpriteCell sprite : sprites)
            vdpSprites.add(new VDPSprite(id + "_sprite" + ind++, sprite, wf, hf));

        hc = (timer << 16) ^ ((tileset != null) ? tileset.hashCode() : 0) ^ vdpSprites.hashCode() ^ ((collision != null) ? collision.hashCode() : 0);
    }

    /**
     * @param w
     *        width of image in tile
     * @param h
     *        height of image in tile
     * @param wf
     *        width of frame in tile
     * @param hf
     *        height of frame in tile
     * @param showCut
     */
	public SpriteFrame(String id, byte[] frameImage8bpp, int wf, int hf, int timer, CollisionType collisionType, Compression compression,
            OptimizationType optType, OptimizationLevel optLevel)
    {
        this(id, frameImage8bpp, wf, hf, timer, collisionType, compression, computeSpriteCutting(id, frameImage8bpp, wf, hf, optType, optLevel));
    }

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
    public SpriteFrame(String id, byte[] image8bpp, int w, int h, int frameIndex, int animIndex, int wf, int hf, int timer, CollisionType collisionType,
            Compression compression, OptimizationType optType, OptimizationLevel optLevel)
    {
        this(id, ImageUtil.getSubImage(image8bpp, new Dimension(w * 8, h * 8), new Rectangle((frameIndex * wf) * 8, (animIndex * hf) * 8, wf * 8, hf * 8)), wf,
                hf, timer, collisionType, compression, optType, optLevel);
    }
    
    static List<SpriteCell> computeSpriteCutting(String id, byte[] frameImage8bpp, int wf, int hf, OptimizationType optType, OptimizationLevel optLevel) throws UnsupportedOperationException
    {
        List<SpriteCell> sprites;
        final Dimension frameDim = new Dimension(wf * 8, hf * 8);

        // special case of no optimization ? --> use default solution covering the whole sprite frame
        if (optType == OptimizationType.NONE)
            sprites = SpriteCutter.getFastOptimizedSpriteList(frameImage8bpp, frameDim, OptimizationType.NONE, false);
        else
        {
            // slow optimization ?
            if ((optLevel == OptimizationLevel.SLOW) || (optLevel == OptimizationLevel.MAX))
            {
                final int iteration = (optLevel == OptimizationLevel.SLOW) ? 500000 : 5000000;

                sprites = SpriteCutter.getSlowOptimizedSpriteList(frameImage8bpp, frameDim, iteration, optType);

                // above the limit of internal sprite ? force MIN_SPRITE optimization strategy
                if ((sprites.size() > 16) && (optType != OptimizationType.MIN_SPRITE))
                    sprites = SpriteCutter.getSlowOptimizedSpriteList(frameImage8bpp, frameDim, iteration, OptimizationType.MIN_SPRITE);
            }
            else
            {
                final boolean optBetter = optLevel == OptimizationLevel.MEDIUM;

                // always start with the fast optimization first
                sprites = SpriteCutter.getFastOptimizedSpriteList(frameImage8bpp, frameDim, optType, optBetter);

                // too many sprites used for this sprite ? try MIN_SPRITE opt strategy
                if ((sprites.size() > 16) && (optType != OptimizationType.MIN_SPRITE))
                    sprites = SpriteCutter.getFastOptimizedSpriteList(frameImage8bpp, frameDim, OptimizationType.MIN_SPRITE, optBetter);

                // still too many sprites used for this sprite ? try MIN_SPRITE with optBetter option
                if ((sprites.size() > 16) && !optBetter)
                    sprites = SpriteCutter.getFastOptimizedSpriteList(frameImage8bpp, frameDim, OptimizationType.MIN_SPRITE, true);

                // still too many sprites used for this sprite ? try better (but slower) sprite optimization method
                if (sprites.size() > 16)
                    sprites = SpriteCutter.getSlowOptimizedSpriteList(frameImage8bpp, frameDim, 100000, OptimizationType.MIN_SPRITE);
            }
        }

        // still above the limit ? --> stop here :-(
        if (sprites.size() > 16)
            throw new UnsupportedOperationException("Sprite frame '" + id + "' uses " + sprites.size()
                    + " internal sprites, that is above the limit (16), try to reduce the sprite size or split it.");

        // special case of NONE optimization type
        if ((!sprites.isEmpty()) && (optType == OptimizationType.NONE))
        {
            // check if frame is empty or not
            boolean empty = true;
            for (byte b : frameImage8bpp)
            {
                if ((b & 0xF) != 0)
                {
                    empty = false;
                    break;
                }
            }

            // empty frame ? --> clear sprite list
            if (empty)
                sprites.clear();
        }
        
        return sprites;
    }

    static int computeFastHashcode(byte[] frameImage8bpp, Dimension frameDim, int timer, CollisionType collision, Compression compression)
    {
        return (timer << 16) ^ ((collision != null) ? collision.hashCode() : 0) ^ Arrays.hashCode(frameImage8bpp) ^ frameDim.hashCode()
                ^ compression.hashCode();
    }
    
    public List<SpriteCell> getSprites()
    {
        List<SpriteCell> result = new ArrayList<SpriteCell>();
        
        for(VDPSprite sprite: vdpSprites)
        	result.add(new SpriteCell(sprite.offsetX, sprite.offsetY, sprite.wt * 8, sprite.ht * 8, OptimizationType.BALANCED));

        return result;
    }

    public int getNumSprite()
    {
        return isEmpty() ? 0 : vdpSprites.size();
    }

    public boolean isEmpty()
    {
        return tileset.isEmpty();
    }

    public boolean isOptimisable()
    {
        if (vdpSprites.size() == 1)
        {
            final VDPSprite vdpSprite = vdpSprites.get(0);
            return ((vdpSprite.wt * 8) == frameDim.width) && ((vdpSprite.ht * 8) == frameDim.height) && (vdpSprite.offsetX == 0) && (vdpSprite.offsetY == 0);
        }
        
        return false;
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
            return (timer == spriteFrame.timer) && tileset.equals(spriteFrame.tileset) && vdpSprites.equals(spriteFrame.vdpSprites)
                    && ((collision == spriteFrame.collision) || ((collision != null) && collision.equals(spriteFrame.collision)));
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
        return id + ": numTile=" + getNumTile() + " numSprite=" + getNumSprite();
    }

    @Override
    public int shallowSize()
    {
        return (vdpSprites.size() * 6) + 1 + 1 + 4 + 4;
    }

    @Override
    public int totalSize()
    {
        if (isEmpty())
            return shallowSize();

        return tileset.totalSize() + ((collision != null) ? collision.totalSize() : 0) + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // AnimationFrame structure
        Util.decl(outS, outH, "AnimationFrame", id, 2, global);
        // number of sprite / timer info
        int numSprite = isOptimisable() ? 0x81 : getNumSprite(); 
        outS.append("    dc.w    " + (((numSprite << 8) & 0xFF00) | ((timer << 0) & 0xFF)) + "\n");
        // set tileset pointer
        outS.append("    dc.l    " + tileset.id + "\n");
        // set collision pointer
        if (collision == null)
            outS.append("    dc.l    " + 0 + "\n");
        else
            outS.append("    dc.l    " + collision.id + "\n");

        // array of VDPSprite
        for (VDPSprite sprite : vdpSprites)
            sprite.internalOutS(outS);

        outS.append("\n");
    }
}
