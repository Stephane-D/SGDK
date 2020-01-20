package sgdk.rescomp.resource.internal;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.SpriteCell;
import sgdk.rescomp.type.SpriteCell.OptimizationType;

public class SpriteFrameInfo extends Resource
{
    public static SpriteFrameInfo getSpriteFrameInfo(String id, SpriteFrameInfo base, int wf, int hf, boolean hflip,
            boolean vflip, OptimizationType opt)
    {
        final List<SpriteCell> spriteCells = new ArrayList<>();

        for (VDPSprite sprite : base.vdpSprites)
            spriteCells.add(getSpriteCell(sprite, wf, hf, hflip, vflip, opt));

        return new SpriteFrameInfo(id, spriteCells, base.collision);
    }

    static SpriteCell getSpriteCell(VDPSprite base, int wf, int hf, boolean hflip, boolean vflip, OptimizationType opt)
    {
        final int x;
        final int y;

        if (hflip)
            x = (wf * 8) - (base.offsetX + (base.wt * 8));
        else
            x = base.offsetX;
        if (vflip)
            y = (hf * 8) - (base.offsetY + (base.ht * 8));
        else
            y = base.offsetY;

        return new SpriteCell(x, y, base.wt * 8, base.ht * 8, opt);
    }

    public final List<VDPSprite> vdpSprites;
    public final Collision collision;

    final int hc;

    public SpriteFrameInfo(String id, List<SpriteCell> sprites, Collision collision)
    {
        super(id);

        vdpSprites = new ArrayList<>();
        // need to check that as it can be null
        if (collision != null)
            this.collision = (Collision) addInternalResource(collision);
        else
            this.collision = null;

        int ind = 0;
        for (SpriteCell sprite : sprites)
            vdpSprites.add((VDPSprite) addInternalResource(new VDPSprite(id + "_sprite" + ind++, sprite)));

        hc = vdpSprites.hashCode() ^ ((collision != null) ? collision.hashCode() : 0);
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof SpriteFrameInfo)
        {
            final SpriteFrameInfo frameInfo = (SpriteFrameInfo) obj;
            return (collision == frameInfo.collision) && vdpSprites.equals(frameInfo.vdpSprites);
        }

        return false;
    }

    @Override
    public String toString()
    {
        return id + ":" + vdpSprites.size() + " sprites";
    }

    @Override
    public int shallowSize()
    {
        // pointer for each vdp sprite
        return vdpSprites.size() * 4;
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // can't store pointer so we just reset binary stream here (used for compression only)
        outB.reset();

        // VDP sprites pointer table
        Util.decl(outS, outH, null, id + "_sprites", 2, false);
        for (VDPSprite sprite : vdpSprites)
            outS.append("    dc.l    " + sprite.id + "\n");

        outS.append("\n");
    }

}