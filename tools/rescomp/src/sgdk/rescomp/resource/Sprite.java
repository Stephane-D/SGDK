package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.internal.SpriteAnimation;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.ArrayMath;
import sgdk.tool.ImageUtil;
import sgdk.tool.ImageUtil.BasicImageInfo;

public class Sprite extends Resource
{
    public final int wf; // width of frame cell in tile
    public final int hf; // height of frame cell in tile
    public final List<SpriteAnimation> animations;
    public int maxNumTile;
    public int maxNumSprite;

    final int hc;

    public final Palette palette;

    public Sprite(String id, String imgFile, int wf, int hf, Compression compression, int time, CollisionType collision,
            OptimizationType opt, long optIteration) throws IOException, IllegalArgumentException
    {
        super(id);

        // init
        maxNumTile = 0;
        maxNumSprite = 0;
        animations = new ArrayList<>();

        // frame size over limit (we need VDP sprite offset to fit into u8 type)
        if ((wf >= 32) || (hf >= 32))
            throw new IllegalArgumentException(
                    "SPRITE '" + id + "' has frame width or frame height >= 32 (not supported)");

        // set frame size
        this.wf = wf;
        this.hf = hf;

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);

        // check BPP is correct
        if (imgInfo.bpp > 8)
            throw new IllegalArgumentException("'" + imgFile + "' is in " + imgInfo.bpp
                    + " bpp format, only indexed images (8,4,2,1 bpp) are supported.");

        // set width and height
        final int w = imgInfo.w;
        final int h = imgInfo.h;

        // check size is correct
        if ((w & 7) != 0)
            throw new IllegalArgumentException("'" + imgFile + "' width is '" + w + ", should be a multiple of 8.");
        if ((h & 7) != 0)
            throw new IllegalArgumentException("'" + imgFile + "' height is '" + h + ", should be a multiple of 8.");

        // get image data
        byte[] imgData = ImageUtil.getIndexedPixels(imgFile);
        // convert to 8 bpp
        imgData = ImageUtil.convertTo8bpp(imgData, imgInfo.bpp);

        // find max color index
        final int maxIndex = ArrayMath.max(imgData, false);
        if (maxIndex >= 64)
            throw new IllegalArgumentException("'" + imgFile
                    + "' uses color index >= 64, SPRITE resource requires image with a maximum of 64 colors, use 4bpp image instead if you are unsure.");

        final int palIndex;
        try
        {
            // get palette index used (only 1 palette allowed for sprite)
            palIndex = ImageUtil.getSpritePaletteIndex(imgData, imgInfo.w, imgInfo.h);
        }
        catch (IllegalArgumentException e)
        {
            throw new IllegalArgumentException(
                    "'" + imgFile
                            + "' SPRITE resource use more than 1 palette (16 colors), use 4bpp image instead if you are unsure.",
                    e);
        }
        // get size in tile
        final int wt = w / 8;
        final int ht = h / 8;

        // check image size is correct
        if ((wt % wf) != 0)
            throw new IllegalArgumentException(
                    "Error: '" + imgFile + "' width (" + w + ") is not a multiple of cell width (" + (wf * 8) + ").");
        if ((ht % hf) != 0)
            throw new IllegalArgumentException(
                    "Error: '" + imgFile + "' height (" + h + ") is not a multiple of cell height (" + (hf * 8) + ").");

        // build PALETTE
        palette = (Palette) addInternalResource(new Palette(id + "_palette", imgFile, palIndex * 16, 16, true));

        // get number of animation
        final int numAnim = ht / hf;

        for (int i = 0; i < numAnim; i++)
        {
            // build sprite animation
            SpriteAnimation animation = new SpriteAnimation(id + "_animation" + i, imgData, wt, ht, i, wf, hf, time,
                    collision, compression, opt, optIteration);

            // check if empty
            if (!animation.isEmpty())
            {
                // add as internal resource (get duplicate if exist)
                animation = (SpriteAnimation) addInternalResource(animation);

                // update maximum number of tile and sprite
                maxNumTile = Math.max(maxNumTile, animation.getMaxNumTile());
                maxNumSprite = Math.max(maxNumSprite, animation.getMaxNumSprite());

                // add animation
                animations.add(animation);
            }
        }

        // compute hash code
        hc = (wf << 0) ^ (hf << 8) ^ (maxNumTile << 16) ^ (maxNumSprite << 24) ^ animations.hashCode()
                ^ palette.hashCode();
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Sprite)
        {
            final Sprite sprite = (Sprite) obj;
            return (wf == sprite.wf) && (hf == sprite.hf) && (maxNumTile == sprite.maxNumTile)
                    && (maxNumSprite == sprite.maxNumSprite) && animations.equals(sprite.animations)
                    && palette.equals(sprite.palette);
        }

        return false;
    }

    @Override
    public String toString()
    {
        return id + ": wf=" + wf + " hf=" + hf + " numAnim=" + animations.size() + " maxNumTile=" + maxNumSprite
                + " maxNumSprite=" + maxNumSprite;
    }

    @Override
    public int shallowSize()
    {
        return (animations.size() * 4) + 2 + 2 + 4 + 2 + 4 + 2 + 2;
    }

    @Override
    public int totalSize()
    {
        int result = 0;

        for (SpriteAnimation animation : animations)
            result += animation.totalSize();

        return result + palette.totalSize() + shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // animations pointer table
        Util.decl(outS, outH, null, id + "_animations", 2, false);
        for (SpriteAnimation animation : animations)
            outS.append("    dc.l    " + animation.id + "\n");

        outS.append("\n");

        // SpriteDefinition structure
        Util.decl(outS, outH, "SpriteDefinition", id, 2, global);
        // set frame cell size
        outS.append("    dc.w    " + (wf * 8) + "\n");
        outS.append("    dc.w    " + (hf * 8) + "\n");
        // set palette pointer
        outS.append("    dc.l    " + palette.id + "\n");
        // set number of animation
        outS.append("    dc.w    " + animations.size() + "\n");
        // set animations pointer
        outS.append("    dc.l    " + id + "_animations" + "\n");
        // set maximum number of tile used by a single animation frame (used for VRAM tile space
        // allocation)
        outS.append("    dc.w    " + maxNumTile + "\n");
        // set maximum number of VDP sprite used by a single animation frame (used for VDP sprite
        // allocation)
        outS.append("    dc.w    " + maxNumSprite + "\n");

        outS.append("\n");
    }
}