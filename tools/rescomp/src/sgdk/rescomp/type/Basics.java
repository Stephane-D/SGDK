package sgdk.rescomp.type;

import java.awt.Rectangle;

public class Basics
{
    public static enum SoundDriver
    {
        PCM, DPCM2, PCM4, XGM
    }

    public static enum Compression
    {
        AUTO, NONE, APLIB, LZ4W
    }

    public static enum TileOptimization
    {
        NONE, ALL, DUPLICATE_ONLY
    }

    public static enum TileEquality
    {
        NONE, EQUAL, VFLIP(false, true), HFLIP(true, false), HVFLIP(true, true);

        public final boolean hflip;
        public final boolean vflip;

        private TileEquality(boolean h, boolean v)
        {
            this.hflip = h;
            this.vflip = v;
        }

        private TileEquality()
        {
            this(false, false);
        }
    }

    public static enum CollisionType
    {
        NONE, CIRCLE, BOX
    }

    public static interface CollisionBase
    {

    }

    public static class Box implements CollisionBase
    {
        public final int x;
        public final int y;
        public final int w;
        public final int h;

        public Box(int x, int y, int w, int h)
        {
            super();

            this.x = x;
            this.y = y;
            this.w = w;
            this.h = h;
        }

        public Box(Rectangle r)
        {
            this(r.x, r.y, r.width, r.height);
        }
    }

    public static class Circle implements CollisionBase
    {
        public final int x;
        public final int y;
        public final int ray;

        public Circle(int x, int y, int ray)
        {
            super();

            this.x = x;
            this.y = y;
            this.ray = ray;
        }
    }

    public static class PackedData
    {
        public final byte[] data;
        public final Compression compression;

        public PackedData(byte[] data, Compression compression)
        {
            super();

            this.data = data;
            this.compression = compression;
        }
    }
}