package sgdk.rescomp.tool;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Stack;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import sgdk.rescomp.type.CellGrid;
import sgdk.rescomp.type.SpriteCell;
import sgdk.rescomp.type.SpriteCell.OptimizationType;
import sgdk.tool.ImageUtil;
import sgdk.tool.Random;
import sgdk.tool.SystemUtil;
import sgdk.tool.ThreadUtil;

public class SpriteCutter
{
    private static List<Solution> getFastOptimizedSolutions(byte[] image8bpp, Dimension imageDim, Rectangle frameBounds,
            OptimizationType optimizationType)
    {
        final List<Solution> result = new ArrayList<>();
        final SpriteCutter spriteCutter = new SpriteCutter(image8bpp, imageDim, frameBounds);

        // no optimization ? --> get default solution covering the whole sprite frame
        if (optimizationType.equals(OptimizationType.NONE))
            result.add(spriteCutter.getDefaultSolution());
        else
        {
            for (int gridSize = 8; gridSize <= 32; gridSize += 8)
            {
                // get best grid (minimum number of tile for region image covering)
                final CellGrid grid = spriteCutter.getBestGrid(gridSize, optimizationType);
                // quick tiles merging where possible
                if (gridSize == 8)
                    grid.mergeCells(optimizationType);
                // build the solution from the grid
                final Solution solution = spriteCutter.getSolution(grid, optimizationType);
                // fast optimization
                solution.fastOptimize();
                // fix positions
                solution.fixPos();

                // add the solution
                if (!solution.cells.isEmpty())
                    result.add(solution);
            }
        }

        return result;
    }

    /**
     * Fast method for cutting frame and retrieve optimized sprite list.
     * 
     * @param optimizationType
     *        Indicate if we prefer to minimize the number of Sprite, Tile or a mix of both.
     * @see #startOptimization(Solution, int)
     * @see #getOptimizedSolution()
     */
    public static List<SpriteCell> getFastOptimizedSpriteList(byte[] image8bpp, Dimension imageDim,
            OptimizationType optimizationType)
    {
        return getFastOptimizedSpriteList(image8bpp, imageDim, new Rectangle(imageDim), optimizationType);
    }

    /**
     * Fast method for cutting frame and retrieve optimized sprite list.
     * 
     * @param optimizationType
     *        Indicate if we prefer to minimize the number of Sprite, Tile or a mix of both.
     * @see #startOptimization(Solution, int)
     * @see #getOptimizedSolution()
     */
    public static List<SpriteCell> getFastOptimizedSpriteList(byte[] image8bpp, Dimension imageDim,
            Rectangle frameBounds, OptimizationType optimizationType)
    {
        final List<Solution> solutions = getFastOptimizedSolutions(image8bpp, imageDim, frameBounds, optimizationType);

        if (solutions.isEmpty())
            return new ArrayList<>();

        // sort on score
        Collections.sort(solutions);

        List<SpriteCell> result = null;

        for (int s = 0; s < solutions.size(); s++)
        {
            // get cells
            final List<SpriteCell> currentCells = solutions.get(s).cells;

            // is it valid (16 sprites max) ? --> return it directly
            if (currentCells.size() <= 16)
                return currentCells;

            // keep the ones with the less number of sprite
            if ((result == null) || (currentCells.size() < result.size()))
                result = currentCells;
        }

        return result;
    }

    /**
     * Slow method for cutting frame and retrieve optimized sprite list (genetic algorithm).
     * <b>WARNING:</b> this methods can take a very long time to execute depending the wanted number
     * of iteration.
     * 
     * @param optIteration
     *        Number of iteration for the genetic algorithm (about 100000 it/s on core i5@2Ghz with
     *        a 128x96 sprite)
     * @param optimizationType
     *        Indicate if we prefer to minimize the number of Sprite, Tile or a mix of both.
     * @see SpriteCutter#startOptimization(Solution, int)
     * @see SpriteCutter#getOptimizedSolution()
     */
    public static List<SpriteCell> getSlowOptimizedSpriteList(byte[] image8bpp, Dimension imageDim, long optIteration,
            OptimizationType optimizationType)
    {
        return getSlowOptimizedSpriteList(image8bpp, imageDim, new Rectangle(imageDim), optIteration, optimizationType);
    }

    /**
     * Slow method for cutting frame and retrieve optimized sprite list (genetic algorithm).<br>
     * <b>WARNING:</b> this methods can take a very long time to execute depending the wanted number
     * of iteration.
     * 
     * @param optIteration
     *        Number of iteration for the genetic algorithm (about 100000 it/s on core i5@2Ghz with
     *        a 128x96 sprite)
     * @param optimizationType
     *        Indicate if we prefer to minimize the number of Sprite, Tile or a mix of both.
     * @see SpriteCutter#startOptimization(Solution, int)
     * @see SpriteCutter#getOptimizedSolution()
     */
    public static List<SpriteCell> getSlowOptimizedSpriteList(byte[] image8bpp, Dimension imageDim,
            Rectangle frameBounds, long optIteration, OptimizationType optimizationType)
    {
        // get fast solution
        final List<Solution> baseSolutions = getFastOptimizedSolutions(image8bpp, imageDim, frameBounds,
                optimizationType);

        // nothing more to do..
        if (baseSolutions.isEmpty())
            return new ArrayList<>();

        // build the solution
        final SpriteCutter spriteCutter = new SpriteCutter(image8bpp, imageDim, frameBounds);
        // start optimization
        spriteCutter.startOptimization(baseSolutions, optIteration);

        try
        {
            do
            {
                Thread.sleep(100);
            }
            while (!spriteCutter.isOptimizationDone());
        }
        catch (InterruptedException e)
        {
            // don't wait more...
        }

        return spriteCutter.getOptimizedSolution().cells;
    }

    class Solution implements Comparable<Solution>
    {
        final List<SpriteCell> cells;
        final byte[] coverageImage;
        int remainingPixToCover;
        int numTile;
        double cachedScore;

        public Solution(byte[] coverageImage)
        {
            super();

            this.coverageImage = coverageImage;
            cells = new ArrayList<>();
            cachedScore = -1d;

            reset();
        }

        public Solution()
        {
            this(new byte[image.length]);
        }

        public Solution(CellGrid grid, OptimizationType opt)
        {
            this();

            for (SpriteCell sc : grid.getSpriteCells())
                addCell(new SpriteCell(sc, opt));
        }

        public Solution(List<? extends Rectangle> sprites, OptimizationType opt)
        {
            this();

            for (Rectangle r : sprites)
                addCell(new SpriteCell(r, opt));
        }

        public void reset()
        {
            // fastest way to restore origin pixels
            System.arraycopy(image, 0, coverageImage, 0, dim.width * dim.height);

            cells.clear();
            remainingPixToCover = numOpaquePixel;
            numTile = 0;
            cachedScore = -1d;
        }

        public boolean addCell(SpriteCell cell)
        {
            final Rectangle adjRect = cell.intersection(new Rectangle(dim));
            final int w = adjRect.width;
            final int h = adjRect.height;

            final byte[] data = coverageImage;
            final int pitch = dim.width;
            final int pitchAdj = pitch - w;

            int offset = (adjRect.y * pitch) + adjRect.x;
            int numCoveredPixel = 0;

            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    // new pixel covered ?
                    if (data[offset] != 0)
                    {
                        numCoveredPixel++;
                        // erase pixel
                        data[offset] = 0;
                    }

                    offset++;
                }

                offset += pitchAdj;
            }

            if (numCoveredPixel > 0)
            {
                remainingPixToCover -= numCoveredPixel;
                // num covered pixel by this sprite cell
                cell.coveredPix = numCoveredPixel;
                numTile += cell.numTile;
                cells.add(cell);
                return true;
            }

            return false;
        }

        public boolean isComplete()
        {
            return remainingPixToCover <= 0;
        }

        public double getScore()
        {
            if (!isComplete())
                return Double.MAX_VALUE;

            if (cachedScore == -1d)
            {
                double result = 0d;

                for (SpriteCell cell : cells)
                    result += cell.getScore();

                cachedScore = Math.round(result * 100000d) / 100000d;
            }

            return cachedScore;
        }

        public boolean rebuildFrom(List<SpriteCell> sprCells)
        {
            // rebuild solution and coverage
            reset();
            for (SpriteCell cell : sprCells)
                addCell(cell);

            return isComplete();
        }

        private boolean rebuildCoverage(List<SpriteCell> sprcells)
        {
            // rebuild solution and coverage
            reset();
            for (SpriteCell sc : sprcells)
                addCell(sc);

            return isComplete();
        }

        private boolean rebuildCoverage()
        {
            return rebuildCoverage(new ArrayList<>(cells));
        }

        private void rebuildWithout(SpriteCell cell)
        {
            final List<SpriteCell> cellsCopy = new ArrayList<>(cells);

            cellsCopy.remove(cell);

            rebuildFrom(cellsCopy);
        }

        private double getBaseScore(List<SpriteCell> coveredcells)
        {
            double result = 0d;

            for (SpriteCell sc : coveredcells)
                result += sc.getBaseScore();

            return result;
        }

        List<SpriteCell> getCoveredcells(SpriteCell cell)
        {
            final List<SpriteCell> result = new ArrayList<>();

            for (SpriteCell sc : cells)
                if (cell.contains(sc))
                    result.add(sc);

            return result;
        }

        void optimizeMergeForPart(SpriteCell cell)
        {
            int x, y;
            int w, h;

            double bestGain = 0d;
            SpriteCell bestGainCell = null;
            List<SpriteCell> bestCovered = null;

            x = cell.x;
            y = cell.y;
            w = cell.width / 8;
            h = cell.height / 8;

            while (h < 4)
            {
                while (w < 4)
                {
                    final SpriteCell sc = new SpriteCell(x, y, w * 8, h * 8, cell.opt);
                    final List<SpriteCell> coveredcells = getCoveredcells(sc);
                    final double coveredScore = getBaseScore(coveredcells);

                    if (coveredScore > 0d)
                    {
                        final double gain = coveredScore - sc.getBaseScore();

                        if (gain > bestGain)
                        {
                            bestGain = gain;
                            bestGainCell = sc;
                            bestCovered = coveredcells;
                        }
                    }

                    w++;
                }

                w = 1;
                h++;
            }

            w = cell.width / 8;
            h = cell.height / 8;
            x = cell.x + (w * 8);
            y = cell.y + (h * 8);

            while (h < 4)
            {
                while (w < 4)
                {
                    final SpriteCell sc = new SpriteCell(x - (w * 8), y - (h * 8), w * 8, h * 8, cell.opt);
                    final List<SpriteCell> coveredcells = getCoveredcells(sc);
                    final double coveredScore = getBaseScore(coveredcells);

                    if (coveredScore > 0d)
                    {
                        final double gain = coveredScore - sc.getBaseScore();
                        if (gain > bestGain)
                        {
                            bestGain = gain;
                            bestGainCell = sc;
                            bestCovered = coveredcells;
                        }
                    }

                    w++;
                }

                w = 1;
                h++;
            }

            if (bestGainCell != null)
            {
                cells.add(bestGainCell);
                cells.removeAll(bestCovered);
            }
        }

        public void optimizeMerge()
        {
            // sort cells on their size and coverage
            Collections.sort(cells, SpriteCell.sizeAndCoverageComparator);

            final List<SpriteCell> cellsCopy = new ArrayList<>(cells);

            // optimize each cell independently (starting from largest one)
            for (SpriteCell cell : cellsCopy)
                optimizeMergeForPart(cell);
        }

        private void optimizePos()
        {
            // sort cells on their size and coverage
            Collections.sort(cells, SpriteCell.sizeAndCoverageComparator);

            final List<SpriteCell> cellsCopy = new ArrayList<>(cells);

            // rebuild solution while optimize cell position for better coverage of origin image
            reset();
            // start from largest cell
            for (SpriteCell cell : cellsCopy)
                addCell(SpriteCell.optimizePosition(image, dim, cell));
        }

        private void optimizeSizeForPart(SpriteCell cell)
        {
            rebuildWithout(cell);

            // get optimized cell
            final SpriteCell newCell = SpriteCell.optimizeSize(image, dim, cell);
            // add optimized cell
            if (newCell != null)
                addCell(newCell);
        }

        private void optimizeSize()
        {
            double score;
            double newScore = getScore();
            double conv = 1d;

            do
            {
                score = newScore;

                // sort cells on their size and coverage
                Collections.sort(cells, SpriteCell.sizeAndCoverageComparator);

                // optimize each cell independently (starting from smallest cell)
                for (int i = cells.size() - 1; i >= 0; i--)
                    optimizeSizeForPart(cells.get(i));

                newScore = getScore();

                // compute convergence (to avoid death lock with oscillating score)
                conv /= 2;
                conv += score - newScore;
            }
            while ((newScore != score) && (Math.abs(conv) > 0.0005d));
        }

        public void fixPos()
        {
            final List<SpriteCell> cellsCopy = new ArrayList<>(cells);

            // rebuild solution while fixing cell position (no negative position)
            reset();
            for (SpriteCell cell : cellsCopy)
                addCell(SpriteCell.fixPosition(dim, cell));
        }

        public void fastOptimize()
        {
            optimizeMerge();
            optimizePos();
            optimizeSize();
        }

        public void showInfo()
        {
            System.out.println("Solution: " + this);
        }

        @Override
        public String toString()
        {
            return "Nb sprite=" + cells.size() + " numTile=" + numTile + " - Score=" + getScore();
        }

        @Override
        public int compareTo(Solution o)
        {
            return Double.compare(getScore(), o.getScore());
        }
    }

    class SolutionOptimizer extends Thread
    {
        class SolutionBranch implements Comparable<SolutionBranch>
        {
            public final Integer id;
            // sorted (best solution first)
            final List<Solution> solutions;
            double bestScore;
            long gen;

            public SolutionBranch(Integer id, Solution base)
            {
                super();

                this.id = id;
                solutions = new LinkedList<>();
                bestScore = Double.MAX_VALUE;
                gen = 0;

                addSolution(base);
            }

            public Solution getRandomSolution()
            {
                synchronized (solutions)
                {
                    return solutions.get(Random.nextInt(solutions.size()));
                }
            }

            void addSolution(Solution solution)
            {
                gen++;

                synchronized (solutions)
                {
                    // add solution
                    final int index = Collections.binarySearch(solutions, solution);

                    if (index >= 0)
                        solutions.add(index, solution);
                    else
                        solutions.add(-(index + 1), solution);

                    // then remove worst element from list
                    if (solutions.size() > solutionPoolSize)
                        solutions.remove(solutions.size() - 1);
                }

                final double score = solution.getScore();

                if (score < bestScore)
                {
                    bestScore = score;

                    if (score < globalBestScore)
                    {
                        globalBestScore = score;
                        // System.out.println("Iteration " + executor.getCompletedTaskCount() + " -
                        // branch #" + id
                        // + " generation #" + gen + " Solution: " + solution);
                    }
                }
            }

            @Override
            public int compareTo(SolutionBranch sb)
            {
                // return Double.compare(bestScore, sb.bestScore);

                // same score ? compare on generation number
                if (bestScore == sb.bestScore)
                    return Double.compare(gen, sb.gen);

                // keep current best score whatever is other branch
                if (bestScore == globalBestScore)
                    return -1;
                if (sb.bestScore == globalBestScore)
                    return 1;

                // about same number of generation ? --> compare on respective score
                if ((gen / 1000L) == (sb.gen / 1000L))
                    return Double.compare(bestScore, sb.bestScore);

                // compare on generation number to avoid to throw up young generation
                return Double.compare(gen, sb.gen);
            }

            @Override
            public String toString()
            {
                return "Branch #" + id + " - generation #" + gen + " (bestScore=" + bestScore + ")";
            }
        }

        class SolutionPartMutationBuilder implements Runnable
        {
            final Integer branchId;
            final Solution source;
            final int numMutation;

            public SolutionPartMutationBuilder(Integer branchId, Solution src, int num)
            {
                super();

                this.branchId = branchId;
                source = src;
                numMutation = num;
            }

            @Override
            public void run()
            {
                final List<SpriteCell> cells = new ArrayList<>(source.cells);
                final int size = cells.size();

                // build the solution using source
                final Solution result = new Solution(getImage());

                try
                {
                    for (int i = 0; i < Math.min(size, numMutation); i++)
                    {
                        final SpriteCell partToMutate = cells.remove(Random.nextInt(cells.size()));

                        for (SpriteCell newPart : partToMutate.mutate())
                            result.addCell(newPart);
                    }

                    // remaining cells
                    for (SpriteCell cell : cells)
                    {
                        result.addCell(cell);

                        // stop as soon solution is complete
                        if (result.isComplete())
                            break;
                    }
                }
                finally
                {
                    releaseImage(result.coverageImage);
                }

                addSolution(branchId, result);
            }
        }

        class SolutionMixMutationBuilder implements Runnable
        {
            final Integer branchId;
            final Solution source1;
            final Solution source2;

            public SolutionMixMutationBuilder(Integer branchId, Solution src1, Solution src2)
            {
                super();

                this.branchId = branchId;
                source1 = src1;
                source2 = src2;
            }

            @Override
            public void run()
            {
                final List<SpriteCell> cells1 = new ArrayList<>(source1.cells);
                final List<SpriteCell> cells2 = new ArrayList<>(source2.cells);

                final Solution result = new Solution(getImage());

                try
                {
                    // build the solution mixing the 2 sources
                    while (!cells1.isEmpty() && !cells2.isEmpty() && !result.isComplete())
                    {
                        final SpriteCell cell;

                        if (Random.nextBoolean())
                            cell = cells1.remove(Random.nextInt(cells1.size()));
                        else
                            cell = cells2.remove(Random.nextInt(cells2.size()));

                        result.addCell(cell);
                    }
                }
                finally
                {
                    releaseImage(result.coverageImage);
                }

                addSolution(branchId, result);
            }
        }

        static final int DEFAULT_SOLUTION_POOL_SIZE = 64;
        static final int DEFAULT_MAX_BRANCH = 1024;

        final int maxBranch;
        final int solutionPoolSize;
        final long maxIteration;
        final int numWorker;

        int curBranchId;
        int curBranchTask;
        double globalBestScore;

        final ThreadPoolExecutor executor;
        final Stack<byte[]> workImagePool;
        // list of branch (easier for fast sorting)
        final List<SolutionBranch> branches;
        final Map<Integer, SolutionBranch> branchMap;

        /**
         * @param bases
         *        input solutions to optimize (should be valid)
         * @param numIteration
         *        maximum number of iteration (0 = no maximum)
         * @param maxBranch
         *        maximum number of alive branch
         * @param solutionPoolSize
         *        maximum number of solution per branch
         */
        public SolutionOptimizer(List<Solution> bases, long numIteration, int maxBranch, int solutionPoolSize)
        {
            super();

            this.maxBranch = maxBranch;
            this.solutionPoolSize = solutionPoolSize;
            this.maxIteration = numIteration;

            globalBestScore = Double.MAX_VALUE;
            numWorker = SystemUtil.getNumberOfCPUs();
            executor = new ThreadPoolExecutor(numWorker, numWorker * 2, 5L, TimeUnit.SECONDS,
                    new LinkedBlockingQueue<Runnable>());
            workImagePool = new Stack<>();
            branches = new ArrayList<>();
            branchMap = new HashMap<>();

            // build the work image pool (should never need more than numWorker * 2 but just for
            // safety)
            for (int i = 0; i < numWorker * 4; i++)
                workImagePool.push(new byte[image.length]);

            curBranchTask = 0;
            curBranchId = 1;

            // create firsts branches
            for (Solution base : bases)
                createNewBranch(base);

            // start process
            start();
        }

        /**
         * @param bases
         *        input solutions to optimize (should be valid)
         * @param numIteration
         *        maximum number of iteration (0 = no maximum)
         */
        public SolutionOptimizer(List<Solution> bases, long numIteration)
        {
            this(bases, numIteration, DEFAULT_MAX_BRANCH, DEFAULT_SOLUTION_POOL_SIZE);
        }

        public byte[] getImage()
        {
            final byte[] result;

            synchronized (workImagePool)
            {
                result = workImagePool.pop();
            }

            return result;
        }

        public void releaseImage(byte[] img)
        {
            synchronized (workImagePool)
            {
                workImagePool.push(img);
            }
        }

        @Override
        public void run()
        {
            while (!interrupted())
            {
                while (executor.getQueue().size() < (numWorker * 2))
                    addNewTask();

                // reached maximum number of iteration ? stop here
                if ((maxIteration > 0) && (getIterationCount() >= maxIteration))
                    break;

                ThreadUtil.sleep(0);
            }

            executor.shutdown();
        }

        private SolutionBranch createNewBranch(Solution base)
        {
            final SolutionBranch result;

            synchronized (branchMap)
            {
                synchronized (branches)
                {
                    // need to remove a branch ?
                    if (branches.size() >= maxBranch)
                    {
                        // sort branches by best score
                        Collections.sort(branches);
                        // remove worst score branch
                        final SolutionBranch branchToRemove = branches.remove(branches.size() - 1);
                        // then remove it from map
                        branchMap.remove(branchToRemove.id);
                    }

                    final Integer id = Integer.valueOf(curBranchId);

                    result = new SolutionBranch(id, base);

                    // add to list and to map
                    branches.add(result);
                    branchMap.put(id, result);

                    curBranchId++;
                }
            }

            return result;
        }

        public SolutionBranch getBranch(Integer id)
        {
            synchronized (branchMap)
            {
                return branchMap.get(id);
            }
        }

        public Solution getBestSolution()
        {
            synchronized (branchMap)
            {
                return branches.get(0).solutions.get(0);
            }
        }

        void addSolution(Integer branchId, Solution solution)
        {
            // add only complete solution
            if (solution.isComplete())
            {
                final SolutionBranch branch = getBranch(branchId);

                // branch don't exist yet ? --> create it
                if (branch == null)
                    createNewBranch(solution);
                else
                    branch.addSolution(solution);
            }
        }

        private void addNewTask()
        {
            final Runnable task;
            final int size = branches.size();

            // branch mutation
            if ((Random.nextInt() & 0x1F) != 0)
            {
                final SolutionBranch branch;
                final Solution solution;

                synchronized (branches)
                {
                    branch = branches.get(curBranchTask);
                    solution = branch.getRandomSolution();
                }

                // cell mutation
                task = new SolutionPartMutationBuilder(branch.id, solution, (Random.nextInt() & 3) + 1);
                // so we process each branch
                curBranchTask = (curBranchTask + 1) % size;
            }
            // new branch
            else
            {
                final SolutionBranch branch1;
                final SolutionBranch branch2;
                final Solution solution1;
                final Solution solution2;

                synchronized (branches)
                {
                    branch1 = branches.get(Random.nextInt(size));
                    branch2 = branches.get(Random.nextInt(size));
                    solution1 = branch1.getRandomSolution();
                    solution2 = branch2.getRandomSolution();
                }

                // mix mutation
                task = new SolutionMixMutationBuilder(Integer.valueOf(curBranchId), solution1, solution2);
                curBranchId++;
            }

            executor.execute(task);
        }
    }

    final byte[] image;
    final Dimension dim;
    final int numOpaquePixel;

    SolutionOptimizer optimizer;
    long optimizerStart;

    public SpriteCutter(byte[] image8bpp, Dimension imageDim)
    {
        super();

        image = image8bpp;
        dim = imageDim;
        numOpaquePixel = ImageUtil.getOpaquePixelCount(image, dim, new Rectangle(dim));
        optimizer = null;
    }

    public SpriteCutter(byte[] image8bpp, Dimension imageDim, Rectangle frameBounds)
    {
        this(ImageUtil.getSubImage(image8bpp, imageDim, frameBounds), frameBounds.getSize());
    }

    /**
     * Return default solution (just covering the whole sprite frame using plain VDP sprites)
     */
    public Solution getDefaultSolution()
    {
        final Rectangle imageCellBounds = new Rectangle(dim.width / 8, dim.height / 8);

        // default using 32x32 cell
        final int numCellW = (imageCellBounds.width + 3) / 4;
        final int numCellH = (imageCellBounds.height + 3) / 4;

        // last cell size
        int lastCellW = imageCellBounds.width & 3;
        int lastCellH = imageCellBounds.height & 3;
        // adjust
        if (lastCellW == 0)
            lastCellW = 4;
        if (lastCellH == 0)
            lastCellH = 4;

        final List<Rectangle> sprites = new ArrayList<>();

        // build the list of sprite
        for (int xc = 0; xc < numCellW; xc++)
        {
            for (int yc = 0; yc < numCellH; yc++)
            {
                sprites.add(new Rectangle(xc * 32, yc * 32, (xc == (numCellW - 1)) ? lastCellW * 8 : 32,
                        (yc == (numCellH - 1)) ? lastCellH * 8 : 32));
            }
        }

        return new Solution(sprites, OptimizationType.NONE);
    }

    public CellGrid getBestGrid(int cellSize, OptimizationType opt)
    {
        final Rectangle imageBounds = new Rectangle(dim);
        final int cellMask = cellSize - 1;
        int bestScore = Integer.MAX_VALUE;
        CellGrid bestGrid = null;

        for (int offX = -cellMask; offX <= 0; offX++)
        {
            for (int offY = -cellMask; offY <= 0; offY++)
            {
                final CellGrid grid = new CellGrid(offX, offY, (dim.width + (cellSize * 2)) / cellSize,
                        (dim.height + (cellSize * 2)) / cellSize);

                for (int x = offX, xc = 0; x < (dim.width + cellSize); x += cellSize, xc++)
                {
                    for (int y = offY, yc = 0; y < (dim.height + cellSize); y += cellSize, yc++)
                    {
                        final SpriteCell tileRect = new SpriteCell(x, y, cellSize, cellSize, opt);

                        if (!ImageUtil.isTransparent(image, dim, tileRect.intersection(imageBounds)))
                            grid.set(xc, yc, tileRect);
                    }
                }

                final int usedCells = grid.getUsedCells(false);
                // we find a better score ?
                if (usedCells < bestScore)
                {
                    bestScore = usedCells;
                    bestGrid = grid;
                }
            }
        }

        return bestGrid;
    }

    public Solution getSolution(CellGrid grid, OptimizationType opt)
    {
        return new Solution(grid, opt);
    }

    public Solution getSolution(List<? extends Rectangle> sprites, OptimizationType opt)
    {
        return new Solution(sprites, opt);
    }

    /**
     * Start a new optimization from given solution (genetic algorithm).<br>
     * The method returns immediately, you should retrieve the result using
     * {@link #getOptimizedSolution()}
     * 
     * @param solutions
     *        input solution< to optimize (should be valid)
     * @param numIteration
     *        Number of iteration for the genetic algorithm (0 = no limit, about 100000 it/s on core
     *        i5@2Ghz with a 128x96 sprite)
     * @see #isOptimizationDone()
     * @see #getOptimizedSolution()
     */
    public void startOptimization(List<Solution> solutions, long numIteration)
    {
        if (optimizer != null)
            optimizer.interrupt();

        optimizerStart = System.currentTimeMillis();
        optimizer = new SolutionOptimizer(solutions, numIteration);
    }

    public boolean isOptimizationDone()
    {
        if (optimizer == null)
            return true;

        return optimizer.executor.isTerminated();
    }

    public long getIterationCount()
    {
        if (optimizer == null)
            return 0L;

        return optimizer.executor.getCompletedTaskCount();
    }

    /**
     * @return the best sprite list solution found by the genetic algorithm (interrupt the
     *         optimization process if it was still running)
     * @see #startOptimization(Solution, int)
     * @see #isOptimizationDone()
     */
    public Solution getOptimizedSolution()
    {
        if (optimizer == null)
            return null;

        // stop optimizer
        optimizer.interrupt();
        try
        {
            optimizer.executor.awaitTermination(500, TimeUnit.MILLISECONDS);
        }
        catch (InterruptedException e)
        {
            // ignore
        }

        final long time = System.currentTimeMillis() - optimizerStart;
        final long it = getIterationCount();

        System.out.println(it + " iterations in " + time + " ms (" + ((it * 1000) / time) + " it/s)");

        // get best solution from optimizer
        final Solution result = optimizer.getBestSolution();
        // fix positions
        result.fixPos();
        // done
        optimizer = null;

        // return result
        return result;
    }
}