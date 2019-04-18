package sgdk.rescomp.resource.internal;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Palette;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.CollisionType;
import sgdk.rescomp.type.Basics.Compression;

public class SpriteDefinition extends Resource
{
    public final List<SpriteAnimation> animations;
    public final Palette palette;
    public int maxNumTile;
    public int maxNumSprite;

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
    public SpriteDefinition(String id, byte[] image8bpp, int w, int h, int wf, int hf, int time,
            CollisionType collision, Palette palette, Compression compression)
    {
        super(id);

        // init
        maxNumTile = 0;
        maxNumSprite = 0;
        animations = new ArrayList<>();
        // just a reference on palette for the export
        this.palette = palette;

        // get number of animation
        final int numAnim = h / hf;

        for (int i = 0; i < numAnim; i++)
        {
            // build sprite animation
            SpriteAnimation animation = new SpriteAnimation(id + "_animation" + i, image8bpp, w, h, i, wf, hf, time,
                    collision, compression);

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
        if (obj instanceof SpriteDefinition)
        {
            final SpriteDefinition spriteDef = (SpriteDefinition) obj;
            return (maxNumTile == spriteDef.maxNumTile) && (maxNumSprite == spriteDef.maxNumSprite)
                    && animations.equals(spriteDef.animations) && palette.equals(spriteDef.palette);
        }

        return false;
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