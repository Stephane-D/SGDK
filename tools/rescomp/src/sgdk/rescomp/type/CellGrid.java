package sgdk.rescomp.type;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import sgdk.rescomp.type.SpriteCell.OptimizationType;

public class CellGrid
{
    final SpriteCell[][] grid;
    int offsetX;
    int offsetY;

    public CellGrid(int offsetX, int offsetY, int sx, int sy)
    {
        super();

        grid = new SpriteCell[sy][sx];
        this.offsetX = offsetX;
        this.offsetY = offsetY;

        for (int j = 0; j < sy; j++)
            for (int i = 0; i < sx; i++)
                grid[j][i] = null;
    }

    public void set(int x, int y, SpriteCell value)
    {
        grid[y][x] = value;
    }

    public boolean isUsed(int x, int y, boolean allowFused)
    {
        if ((x < 0) || (y < 0))
            return false;
        if ((y >= grid.length) || (x >= grid[0].length))
            return false;

        return (grid[y][x] != null) && (allowFused || grid[y][x].isSingleTile());
    }

    public int getUsedCells(boolean singleTileOnly)
    {
        return getUsedCells(0, 0, grid[0].length, grid.length, singleTileOnly);
    }

    public int getUsedCells(int x, int y, int w, int h, boolean singleTileOnly)
    {
        int result = 0;

        for (int j = y; j < y + h; j++)
        {
            if (j >= grid.length)
                return 0;

            final SpriteCell[] row = grid[j];

            for (int i = x; i < x + w; i++)
            {
                if (i >= row.length)
                    return 0;

                final SpriteCell cell = row[i];

                // occupied ?
                if (cell != null)
                {
                    // check if not yet merge
                    if (!singleTileOnly || cell.isSingleTile())
                        result++;
                }
            }
        }

        return result;
    }

    public void mergeCells(OptimizationType opt)
    {
        for (int j = 0; j < grid.length; j++)
        {
            SpriteCell[] row = grid[j];

            for (int i = 0; i < row.length; i++)
            {
                // occupied and not yet merged ?
                if ((row[i] != null) && row[i].isSingleTile())
                    mergeAt(i, j, opt);
            }
        }
    }

    public List<SpriteCell> getSpriteCells()
    {
        final Set<SpriteCell> result = new HashSet<>();

        for (int j = 0; j < grid.length; j++)
        {
            final SpriteCell[] row = grid[j];

            for (int i = 0; i < row.length; i++)
                if (row[i] != null)
                    result.add(row[i]);
        }

        return new ArrayList<>(result);
    }

    private void mergeAt(int x, int y, OptimizationType opt)
    {
        int w = 1;
        while ((w < 4) && isUsed(x + w, y, false))
            w++;

        int h = 1;
        while (h < 4)
        {
            boolean ok = true;

            for (int i = 0; i < w; i++)
            {
                if (!isUsed(x + i, y + h, false))
                {
                    ok = false;
                    break;
                }
            }

            // stop here
            if (!ok)
                break;

            // not at max width ?
            if (w != 4)
            {
                // check if we can enlarge on this line
                if (isUsed(x - 1, y + h, false) || isUsed(x + w, y + h, false))
                    break;
            }

            h++;
        }

        // merge to do ?
        if ((w > 1) || (h > 1))
        {
            final SpriteCell cell = new SpriteCell(offsetX + (x * 8), offsetY + (y * 8), w * 8, h * 8, opt);

            for (int j = y; j < y + h; j++)
            {
                final SpriteCell[] row = grid[j];

                for (int i = x; i < x + w; i++)
                    row[i] = cell;
            }
        }
    }

    // Solution getSolution(Image img)
    // {
    // final Solution result = new Solution(img);
    //
    // for (SpriteCell sp : getSpriteParts())
    // result.addPart(sp);
    //
    // return result;
    // }

}
