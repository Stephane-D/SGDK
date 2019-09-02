package sgdk.rescomp.resource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
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

        // frame size over limit (we need VDP sprite offset to fit into s8 type)
        if ((wf > 16) || (hf > 16))
            throw new IllegalArgumentException(
                    "SPRITE '" + id + "' has frame width or frame height > 16 (not supported)");

        // retrieve basic infos about the image
        final BasicImageInfo imgInfo = ImageUtil.getBasicInfo(imgFile);

        // check BPP is correct
        if ((imgInfo.bpp != 8) && (imgInfo.bpp != 4))
            throw new IllegalArgumentException(
                    "'" + imgFile + "' is in " + imgInfo.bpp + " bpp format, only 8bpp or 4bpp image supported.");

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

        // 4 bpp image ? --> convert to 8bpp
        if (imgInfo.bpp == 4)
            imgData = ImageUtil.convert4bppTo8bpp(imgData);

        // find max color index
        final int maxIndex = ArrayMath.max(imgData, false);
        // 16 colors (1 palette) max for a sprite
        if (maxIndex >= 16)
            throw new IllegalArgumentException("'" + imgFile
                    + "' uses color index >= 16, SPRITE resource requires image with a maximum of 16 colors, use 4bpp image instead if you are unsure.");

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
        palette = (Palette) addInternalResource(new Palette(id + "_palette", imgFile));

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
        hc = maxNumTile ^ (maxNumSprite << 16) ^ animations.hashCode() ^ palette.hashCode();
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
            return (maxNumTile == sprite.maxNumTile) && (maxNumSprite == sprite.maxNumSprite)
                    && animations.equals(sprite.animations) && palette.equals(sprite.palette);
        }

        return false;
    }

    @Override
    public String toString()
    {
        return id + ": numAnim=" + animations.size() + " maxNumTile=" + maxNumSprite + " maxNumSprite=" + maxNumSprite;
    }

    @Override
    public int shallowSize()
    {
        return (animations.size() * 4) + 4 + 2 + 4 + 2 + 2;
    }

    @Override
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // animations pointer table
        Util.decl(outS, outH, null, id + "_animations", 2, false);
        for (SpriteAnimation animation : animations)
            outS.println("    dc.l    " + animation.id);

        outS.println();

        // SpriteDefinition structure
        Util.decl(outS, outH, "SpriteDefinition", id, 2, global);
        // set palette pointer
        outS.println("    dc.l    " + palette.id);
        // set number of animation
        outS.println("    dc.w    " + animations.size());
        // set animations pointer
        outS.println("    dc.l    " + id + "_animations");
        // set maximum number of tile used by a single animation frame (used for VRAM tile space
        // allocation)
        outS.println("    dc.w    " + maxNumTile);
        // set maximum number of VDP sprite used by a single animation frame (used for VDP sprite
        // allocation)
        outS.println("    dc.w    " + maxNumSprite);

        outS.println();
    }

}