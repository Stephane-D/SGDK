package sgdk.rescomp.type;

import java.awt.Dimension;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.geom.Area;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import sgdk.tool.ImageUtil;
import sgdk.tool.Random;

public class SpriteCell extends Rectangle implements Comparable<SpriteCell>
{
    public static enum OptimizationType
    {
        BALANCED, MIN_SPRITE, MIN_TILE, NONE
    };

    public static enum OptimizationLevel
    {
        FAST, MEDIUM, SLOW, MAX
    };

    public static final Comparator<SpriteCell> sizeAndCoverageComparator = new Comparator<SpriteCell>()
    {
        @Override
        public int compare(SpriteCell o1, SpriteCell o2)
        {
            int result = Integer.compare(o1.numTile, o2.numTile);

            if (result == 0)
                result = java.lang.Double.compare(o1.getCoverage(), o2.getCoverage());

            // we want ascending order
            return -result;
        }
    };

    public final OptimizationType opt;
    public final int numTile;
    public int coveredPix;

    public SpriteCell(Rectangle r, OptimizationType opt)
    {
        super(r);

        this.opt = opt;
        numTile = (width * height) / 64;
        coveredPix = -1;
    }

    public SpriteCell(int x, int y, int width, int height, OptimizationType opt)
    {
        super(x, y, width, height);

        this.opt = opt;
        numTile = (width * height) / 64;
        coveredPix = -1;
    }

    public boolean isSingleTile()
    {
        return (width == 8) && (height == 8);
    }

    public List<SpriteCell> mutate()
    {
        final List<SpriteCell> result = new ArrayList<>(4);

        // switch (Random.nextInt() % 3)
        // {
        // default:
        // case 0:
        // // small move mutation
        // result.add(mutateMove(1));
        // break;
        //
        // case 1:
        // // size mutation
        // result.add(mutateSize(false));
        // break;
        //
        // case 2:
        // // split mutation
        // result.addAll(mutateSplit());
        // break;
        // }

        switch (Random.nextInt() & 7)
        {
            default:
            case 0:
                // small move mutation
                result.add(mutateMove(1));
                break;

            case 1:
                // big move mutation
                result.add(mutateMove(4));
                break;

            case 2:
                // size mutation
                result.add(mutateSize(false));
                break;

            case 3:
                // multi size mutation
                result.add(mutateSize(true));
                break;

            case 4:
                // move + size mutation
                result.add(mutateMove(1).mutateSize(false));
                break;

            case 5:
                // big move + multi size mutation
                result.add(mutateMove(4).mutateSize(true));
                break;

            case 6:
                // split mutation
                result.addAll(mutateSplit());
                break;

            case 7:
                // move + split mutation
                result.addAll(mutateMove(1).mutateSplit());
                break;
        }

        return result;
    }

    private SpriteCell mutateMove(int move)
    {
        final Rectangle newRegion = new Rectangle(this);

        switch (Random.nextInt() & 3)
        {
            default:
            case 0:
                newRegion.x += move;
                break;
            case 1:
                newRegion.x -= move;
                break;
            case 2:
                newRegion.y += move;
                break;
            case 3:
                newRegion.y -= move;
                break;
        }

        return new SpriteCell(newRegion, opt);
    }

    private SpriteCell mutateSize(boolean multi)
    {
        final Rectangle newRegion = new Rectangle(this);
        int it = 1;

        if (multi)
            it += Random.nextInt() & 3;

        while (it > 0)
        {
            switch (Random.nextInt() & 3)
            {
                default:
                case 0:
                    if (newRegion.width < 32)
                    {
                        newRegion.width += 8;
                        it--;
                    }
                    break;
                case 1:
                    if (newRegion.width > 8)
                    {
                        newRegion.width -= 8;
                        it--;
                    }
                    break;
                case 2:
                    if (newRegion.height < 32)
                    {
                        newRegion.height += 8;
                        it--;
                    }
                    break;
                case 3:
                    if (newRegion.height > 8)
                    {
                        newRegion.height -= 8;
                        it--;
                    }
                    break;
            }
        }

        return new SpriteCell(newRegion, opt);
    }

    private List<SpriteCell> mutateSplit()
    {
        final List<SpriteCell> result = new ArrayList<>(4);
        final Rectangle newRegion = new Rectangle(this);

        final int sw = Random.nextInt(newRegion.width / 8) * 8;
        final int sh = Random.nextInt(newRegion.height / 8) * 8;

        if ((sw > 32) || (sh > 32))
            System.out.println("error");

        final Rectangle r1 = new Rectangle(x, y, sw, sh);
        final Rectangle r2 = new Rectangle(x + sw, y, width - sw, sh);
        final Rectangle r3 = new Rectangle(x, y + sh, sw, height - sh);
        final Rectangle r4 = new Rectangle(x + sw, y + sh, width - sw, height - sh);

        if (!r1.isEmpty())
            result.add(new SpriteCell(r1, opt));
        if (!r2.isEmpty())
            result.add(new SpriteCell(r2, opt));
        if (!r3.isEmpty())
            result.add(new SpriteCell(r3, opt));
        if (!r4.isEmpty())
            result.add(new SpriteCell(r4, opt));

        return result;
    }

    public double getScore()
    {
        return getBaseScore() + (getCoveragePenalty() / 10d);
    }

    public double getBaseScore()
    {
        switch (opt)
        {
            default:
            case BALANCED:
                return 8d + (numTile * 2.5d) + (getWidth() / 32d);

            case MIN_SPRITE:
                return 12d + (numTile * 1d) + (getWidth() / 32d);

            case MIN_TILE:
                return 6d + (numTile * 4d) + (getWidth() / 32d);
        }
    }

    public double getCoverage()
    {
        if (coveredPix == -1)
            return 0d;

        return (double) coveredPix / (double) (numTile * 64);
    }

    public double getCoveragePenalty()
    {
        return 1d - getCoverage();
    }

    public boolean optimizeOverdraw(Dimension imageDim, List<SpriteCell> allSpr)
    {
        final Area area = new Area();

        // build area corresponding to others sprites
        for (SpriteCell sc : allSpr)
            if (sc != this)
                area.add(new Area(sc));

        // get intersection with others sprites
        area.intersect(new Area(this));

        // no overdraw --> ok
        if (area.isEmpty())
            return false;

        final Point initialPos = getLocation();
        final Rectangle bounds = new Rectangle(imageDim);
        final Rectangle rectH = new Rectangle(0, 0, width, 1);
        final Rectangle rectV = new Rectangle(0, 0, 1, height);

        while (canMoveLeft(area, x, y, rectH, rectV))
        {
            x--;
            // outside image bounds ? --> stop
            if (!bounds.contains(this))
            {
                x++;
                break;
            }
        }
        while (canMoveRight(area, x, y, rectH, rectV))
        {
            x++;
            // outside image bounds ? --> stop
            if (!bounds.contains(this))
            {
                x--;
                break;
            }
        }
        while (canMoveTop(area, x, y, rectH, rectV))
        {
            y--;
            // outside image bounds ? --> stop
            if (!bounds.contains(this))
            {
                y++;
                break;
            }
        }
        while (canMoveBottom(area, x, y, rectH, rectV))
        {
            y++;
            // outside image bounds ? --> stop
            if (!bounds.contains(this))
            {
                y--;
                break;
            }
        }
        
        return !getLocation().equals(initialPos);
    }

    @Override
    public int compareTo(SpriteCell o)
    {
        return java.lang.Double.compare(getScore(), o.getScore());
    }

    @Override
    public String toString()
    {
        return "[" + x + "," + y + "-" + width + "," + height + "] cov= " + (int) (getCoverage() * 100d) + "%";
    }

    public static SpriteCell optimizePosition(byte[] image, Dimension imageDim, SpriteCell spr)
    {
        final Rectangle imageBounds = new Rectangle(imageDim);
        Rectangle region = new Rectangle(spr);

        // optimize on right
        while (!ImageUtil.hasOpaquePixelOnEdge(image, imageDim, region, false, false, true, false)
                && region.intersects(imageBounds))
            region.x--;
        // optimize on bottom
        while (!ImageUtil.hasOpaquePixelOnEdge(image, imageDim, region, false, false, false, true)
                && region.intersects(imageBounds))
            region.y--;
        // optimize on left
        while (!ImageUtil.hasOpaquePixelOnEdge(image, imageDim, region, true, false, false, false)
                && region.intersects(imageBounds))
            region.x++;
        // optimize on top
        while (!ImageUtil.hasOpaquePixelOnEdge(image, imageDim, region, false, true, false, false)
                && region.intersects(imageBounds))
            region.y++;

        return new SpriteCell(region, spr.opt);
    }

    public static SpriteCell optimizeSize(byte[] image, Dimension imageDim, SpriteCell spr)
    {
        Rectangle region = new Rectangle(spr);

        final int coveredPixel = ImageUtil.getOpaquePixelCount(image, imageDim, region);
        // don't need sprite here
        if (coveredPixel == 0)
            return null;

        // optimize on left
        do
        {
            region.x += 8;
            region.width -= 8;
        }
        while ((region.width > 0) && (ImageUtil.getOpaquePixelCount(image, imageDim, region) == coveredPixel));
        // restore last change
        region.x -= 8;
        region.width += 8;

        // optimize on top
        do
        {
            region.y += 8;
            region.height -= 8;
        }
        while ((region.height > 0) && (ImageUtil.getOpaquePixelCount(image, imageDim, region) == coveredPixel));
        // restore last change
        region.y -= 8;
        region.height += 8;

        // optimize on right
        do
            region.width -= 8;
        while ((region.width > 0) && (ImageUtil.getOpaquePixelCount(image, imageDim, region) == coveredPixel));
        region.width += 8;

        // optimize on bottom
        do
            region.height -= 8;
        while ((region.height > 0) && (ImageUtil.getOpaquePixelCount(image, imageDim, region) == coveredPixel));
        region.height += 8;

        return new SpriteCell(region, spr.opt);
    }

    private static boolean canMoveTop(Area area, int x, int y, Rectangle rectH, Rectangle rectV)
    {
        // move rect to bottom
        rectH.setLocation(x, y + (rectV.height - 1));
        // overdraw on bottom line ?
        if (area.contains(rectH))
        {
            // move rect to top-1
            rectH.setLocation(x, y - 1);
            // nothing on top ?
            if (!area.intersects(rectH))
                return true;
        }

        return false;
    }

    private static boolean canMoveBottom(Area area, int x, int y, Rectangle rectH, Rectangle rectV)
    {
        // move rect to top
        rectH.setLocation(x, y);
        // overdraw on top line ?
        if (area.contains(rectH))
        {
            // move rect to bottom+1
            rectH.setLocation(x, y + rectV.height);
            // nothing on bottom ?
            if (!area.intersects(rectH))
                return true;
        }

        return false;
    }

    private static boolean canMoveLeft(Area area, int x, int y, Rectangle rectH, Rectangle rectV)
    {
        // move rect to right
        rectV.setLocation(x + (rectH.width - 1), y);
        // overdraw on right line ?
        if (area.contains(rectV))
        {
            // move rect to left-1
            rectV.setLocation(x - 1, y);
            // nothing on left ?
            if (!area.intersects(rectV))
                return true;
        }

        return false;
    }

    private static boolean canMoveRight(Area area, int x, int y, Rectangle rectH, Rectangle rectV)
    {
        // move rect to left
        rectV.setLocation(x, y);
        // overdraw on left line ?
        if (area.contains(rectV))
        {
            // move rect to right+1
            rectV.setLocation(x + rectH.width, y);
            // nothing on right ?
            if (!area.intersects(rectV))
                return true;
        }

        return false;
    }

    /**
     * Try to optimize the input position / size part depending the given intersecting parts list
     */
    public static SpriteCell optimizeSpritePart(SpriteCell part, List<SpriteCell> intersectingParts)
    {
        final Area area = new Area(part);

        for (SpriteCell sp : intersectingParts)
            area.subtract(new Area(sp));

        // get area bounds
        final Rectangle bounds = area.getBounds();
        // align size on 8
        bounds.setBounds(bounds.x, bounds.y, ((bounds.width + 7) / 8) * 8, ((bounds.height + 7) / 8) * 8);

        return new SpriteCell(bounds, part.opt);
    }

    /**
     * Fix cell position (cannot be negative)
     */
    public static SpriteCell fixPosition(Dimension imageDim, SpriteCell spr)
    {
        Rectangle region = new Rectangle(spr);

        // fix on right
        if (region.getMaxX() > imageDim.getWidth())
            region.x -= region.getMaxX() - imageDim.getWidth();
        // fix on bottom
        if (region.getMaxY() > imageDim.getHeight())
            region.y -= region.getMaxY() - imageDim.getHeight();
        // fix on left
        if (region.x < 0)
            region.x = 0;
        // fix on top
        if (region.y < 0)
            region.y = 0;

        return new SpriteCell(region, spr.opt);
    }
}
