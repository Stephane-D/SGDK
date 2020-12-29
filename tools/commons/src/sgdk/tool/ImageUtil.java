package sgdk.tool;

import java.awt.AlphaComposite;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.Toolkit;
import java.awt.Transparency;
import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.IndexColorModel;
import java.awt.image.Raster;
import java.awt.image.RenderedImage;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

import javax.imageio.ImageIO;

/**
 * Image utilities class.
 * 
 * @author stephane
 */
public class ImageUtil
{
    public static class BasicImageInfo
    {
        public final int w;
        public final int h;
        public final int bpp;

        public BasicImageInfo(int w, int h, int bpp)
        {
            super();

            this.w = w;
            this.h = h;
            this.bpp = bpp;
        }
    }

    public static String getImageTypeString(int type)
    {
        switch (type)
        {
            case BufferedImage.TYPE_CUSTOM:
                return "TYPE_CUSTOM";
            case BufferedImage.TYPE_INT_RGB:
                return "TYPE_INT_RGB";
            case BufferedImage.TYPE_INT_ARGB:
                return "TYPE_INT_ARGB";
            case BufferedImage.TYPE_INT_ARGB_PRE:
                return "TYPE_INT_ARGB_PRE";
            case BufferedImage.TYPE_INT_BGR:
                return "TYPE_INT_BGR";
            case BufferedImage.TYPE_3BYTE_BGR:
                return "TYPE_3BYTE_BGR";
            case BufferedImage.TYPE_4BYTE_ABGR:
                return "TYPE_4BYTE_ABGR";
            case BufferedImage.TYPE_4BYTE_ABGR_PRE:
                return "TYPE_4BYTE_ABGR_PRE";
            case BufferedImage.TYPE_USHORT_565_RGB:
                return "TYPE_USHORT_565_RGB";
            case BufferedImage.TYPE_USHORT_555_RGB:
                return "TYPE_USHORT_555_RGB";
            case BufferedImage.TYPE_BYTE_GRAY:
                return "TYPE_BYTE_GRAY";
            case BufferedImage.TYPE_USHORT_GRAY:
                return "TYPE_USHORT_GRAY";
            case BufferedImage.TYPE_BYTE_BINARY:
                return "TYPE_BYTE_BINARY";
            case BufferedImage.TYPE_BYTE_INDEXED:
                return "TYPE_BYTE_INDEXED";
            default:
                return "UNKNOWN TYPE";
        }
    }

    public static String getTransparencyString(int transparency)
    {
        switch (transparency)
        {
            case Transparency.OPAQUE:
                return "OPAQUE";
            case Transparency.BITMASK:
                return "BITMASK";
            case Transparency.TRANSLUCENT:
                return "TRANSLUCENT";
            default:
                return "UNKNOWN TRANSPARENCY";
        }
    }

    /**
     * Wait for dimension information of specified image being loaded.
     * 
     * @param image
     *        image we are waiting informations for.
     */
    public static void waitImageReady(Image image)
    {
        if (image != null)
        {
            final long st = System.currentTimeMillis();

            // wait 2 seconds max
            while ((image.getWidth(null) == -1) && ((System.currentTimeMillis() - st) < 2000))
                ThreadUtil.sleep(1);
        }
    }

    /**
     * Create a 8 bits indexed buffered image from specified <code>IndexColorModel</code><br>
     * and byte array data.
     */
    public static BufferedImage createIndexedImage(int w, int h, IndexColorModel cm, byte[] data)
    {
        final WritableRaster raster = Raster.createInterleavedRaster(new DataBufferByte(data, w * h, 0), w, h, w, 1,
                new int[] {0}, null);

        return new BufferedImage(cm, raster, false, null);
    }

    /**
     * Load an image from specified path
     */
    public static BufferedImage load(String path, boolean displayError)
    {
        return load(URLUtil.getURL(path), displayError);
    }

    /**
     * Load an image from specified path
     */
    public static BufferedImage load(String path)
    {
        return load(path, true);
    }

    /**
     * Load an image from specified url
     */
    public static BufferedImage load(URL url, boolean displayError)
    {
        if (url != null)
        {
            try
            {
                return ImageIO.read(url);
            }
            catch (IOException e)
            {
                if (displayError)
                    System.err.println("Can't load image from " + url);
            }
        }

        return null;
    }

    /**
     * Asynchronously load an image from specified url.<br/>
     * Use {@link #waitImageReady(Image)} to know if width and height property
     */
    public static Image loadAsync(URL url)
    {
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    /**
     * Asynchronously load an image from specified path.<br/>
     * Use {@link #waitImageReady(Image)} to know if width and height property
     */
    public static Image loadAsync(String path)
    {
        return Toolkit.getDefaultToolkit().createImage(path);
    }

    /**
     * Load an image from specified url
     */
    public static BufferedImage load(URL url)
    {
        return load(url, true);
    }

    /**
     * Load an image from specified file
     */
    public static BufferedImage load(File file, boolean displayError)
    {
        if (file != null)
        {
            try
            {
                return ImageIO.read(file);
            }
            catch (IOException e)
            {
                if (displayError)
                    System.err.println("Can't load image from " + file);
            }
        }

        return null;
    }

    /**
     * Load an image from specified file
     */
    public static BufferedImage load(File file)
    {
        return load(file, true);
    }

    /**
     * Load an image from specified InputStream
     */
    public static BufferedImage load(InputStream input, boolean displayError)
    {
        if (input != null)
        {
            try
            {
                return ImageIO.read(input);
            }
            catch (Exception e)
            {
                if (displayError)
                    System.err.println("Can't load image from stream " + input);
            }
        }

        return null;
    }

    /**
     * Load an image from specified InputStream
     */
    public static BufferedImage load(InputStream input)
    {
        return load(input, true);
    }

    /**
     * Save an image to specified path in specified format
     */
    public static boolean save(RenderedImage image, String format, String path)
    {
        if (path != null)
        {
            try
            {
                return ImageIO.write(image, format, new FileOutputStream(path));
            }
            catch (IOException e)
            {
                System.err.println("Can't save image to " + path);
            }
        }

        return false;
    }

    /**
     * Save an image to specified file in specified format
     */
    public static boolean save(RenderedImage image, String format, File file)
    {
        if (file != null)
        {
            try
            {
                return ImageIO.write(image, format, file);
            }
            catch (IOException e)
            {
                System.err.println("Can't save image to " + file);
            }
        }

        return false;
    }

    /**
     * Return a RenderedImage from the given Image object.
     */
    public static RenderedImage toRenderedImage(Image image)
    {
        return toBufferedImage(image);
    }

    /**
     * Return a ARGB BufferedImage from the given Image object.
     * If the image is already a BufferedImage image then it's directly returned
     */
    public static BufferedImage toBufferedImage(Image image)
    {
        if (image instanceof BufferedImage)
            return (BufferedImage) image;

        // be sure image data are ready
        waitImageReady(image);
        final BufferedImage bufImage = new BufferedImage(image.getWidth(null), image.getHeight(null),
                BufferedImage.TYPE_INT_ARGB);

        final Graphics2D g = bufImage.createGraphics();
        g.drawImage(image, 0, 0, null);
        g.dispose();

        return bufImage;
    }

    /**
     * Scale an image with specified size.
     */
    public static BufferedImage scale(Image image, int width, int height)
    {
        if (image != null)
        {
            final BufferedImage result = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
            final Graphics2D g = result.createGraphics();

            g.setComposite(AlphaComposite.Src);
            g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BILINEAR);
            g.drawImage(image, 0, 0, width, height, null);
            g.dispose();

            return result;
        }

        return null;
    }

    /**
     * Scale an image with specified size (try to keep best quality).
     */
    public static BufferedImage scaleQuality(Image image, int width, int height)
    {
        if (image != null)
        {
            Image current = image;

            // be sure image data are ready
            waitImageReady(image);

            int w = image.getWidth(null);
            int h = image.getHeight(null);

            do
            {
                if (w > width)
                {
                    w /= 2;
                    if (w < width)
                        w = width;
                }
                else
                    w = width;

                if (h > height)
                {
                    h /= 2;
                    if (h < height)
                        h = height;
                }
                else
                    h = height;

                final BufferedImage result = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
                final Graphics2D g = result.createGraphics();

                g.setComposite(AlphaComposite.Src);
                g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BILINEAR);
                g.drawImage(current, 0, 0, w, h, null);

                g.dispose();

                current = result;
            }
            while (w != width || h != height);

            return (BufferedImage) current;
        }

        return null;
    }

    /**
     * Convert an image to a BufferedImage.<br>
     * If <code>out</out> is null, by default a <code>BufferedImage.TYPE_INT_ARGB</code> is created.
     */
    public static BufferedImage convert(Image in, BufferedImage out)
    {
        final BufferedImage result;

        // be sure image data are ready
        waitImageReady(in);

        // no output type specified ? use ARGB
        if (out == null)
            result = new BufferedImage(in.getWidth(null), in.getHeight(null), BufferedImage.TYPE_INT_ARGB);
        else
            result = out;

        final Graphics g = result.getGraphics();
        g.drawImage(in, 0, 0, null);
        g.dispose();

        return result;
    }

    /**
     * Returns <code>true</code> if the specified image is a grayscale image whatever is the image
     * type (GRAY, RGB, ARGB...)
     */
    public static boolean isGray(BufferedImage image)
    {
        if (image == null)
            return false;

        if (image.getType() == BufferedImage.TYPE_BYTE_GRAY)
            return true;
        if (image.getType() == BufferedImage.TYPE_USHORT_GRAY)
            return true;

        final int[] rgbArray = image.getRGB(0, 0, image.getWidth(), image.getHeight(), null, 0, image.getWidth());

        for (int value : rgbArray)
        {
            final int c0 = (value >> 0) & 0xFF;
            final int c1 = (value >> 8) & 0xFF;
            if (c0 != c1)
                return false;

            final int c2 = (value >> 16) & 0xFF;
            if (c0 != c2)
                return false;
        }

        return true;
    }

    /**
     * Convert an image to grey image (<code>BufferedImage.TYPE_BYTE_GRAY</code>).
     */
    public static BufferedImage toGray(Image image)
    {
        if (image != null)
        {
            // be sure image data are ready
            waitImageReady(image);
            return convert(image,
                    new BufferedImage(image.getWidth(null), image.getHeight(null), BufferedImage.TYPE_BYTE_GRAY));
        }

        return null;
    }

    /**
     * Convert an image to RGB image (<code>BufferedImage.TYPE_INT_RGB</code>).
     */
    public static BufferedImage toRGBImage(Image image)
    {
        if (image != null)
        {
            // be sure image data are ready
            waitImageReady(image);
            return convert(image,
                    new BufferedImage(image.getWidth(null), image.getHeight(null), BufferedImage.TYPE_INT_RGB));
        }

        return null;
    }

    /**
     * Convert an image to ARGB image (<code>BufferedImage.TYPE_INT_ARGB</code>).
     */
    public static BufferedImage toARGBImage(Image image)
    {
        if (image != null)
        {
            // be sure image data are ready
            waitImageReady(image);
            return convert(image,
                    new BufferedImage(image.getWidth(null), image.getHeight(null), BufferedImage.TYPE_INT_ARGB));
        }

        return null;
    }

    /**
     * Create a copy of the input image.<br>
     * Result is always a <code>BufferedImage.TYPE_INT_ARGB</code> type image.
     */
    public static BufferedImage getCopy(Image in)
    {
        return convert(in, null);
    }

    /**
     * Return true if image has the same size
     */
    public static boolean sameSize(BufferedImage im1, BufferedImage im2)
    {
        return (im1.getWidth() == im2.getWidth()) && (im1.getHeight() == im2.getHeight());
    }

    /**
     * Get the list of tiles to fill the given XY plan size.
     * 
     * @param sizeX
     *        plan sizeX
     * @param sizeY
     *        plan sizeY
     * @param tileW
     *        tile width
     * @param tileH
     *        tile height
     */
    public static List<Rectangle> getTileList(int sizeX, int sizeY, int tileW, int tileH)
    {
        final List<Rectangle> result = new ArrayList<>();
        int x, y;

        for (y = 0; y < (sizeY - tileH); y += tileH)
        {
            for (x = 0; x < (sizeX - tileW); x += tileW)
                result.add(new Rectangle(x, y, tileW, tileH));
            // last tile column
            result.add(new Rectangle(x, y, sizeX - x, tileH));
        }

        // last tiles row
        for (x = 0; x < (sizeX - tileW); x += tileW)
            result.add(new Rectangle(x, y, tileW, sizeY - y));
        // last column/row tile
        result.add(new Rectangle(x, y, sizeX - x, sizeY - y));

        return result;
    }

    /**
     * Apply simple color filter with specified alpha factor to the image
     */
    public static void applyColorFilter(Image image, Color color, float alpha)
    {
        if (image != null)
        {
            // be sure image data are ready
            waitImageReady(image);

            // should be Graphics2D compatible
            final Graphics2D g = (Graphics2D) image.getGraphics();
            final Rectangle rect = new Rectangle(image.getWidth(null), image.getHeight(null));

            g.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, alpha));
            g.setColor(color);
            g.fill(rect);
            g.dispose();
        }
    }

    public static BasicImageInfo getBasicInfo(BufferedImage image)
    {
        return new BasicImageInfo(image.getWidth(), image.getHeight(), image.getColorModel().getPixelSize());
    }

    public static BasicImageInfo getBasicInfo(String filename) throws IOException
    {
        final BufferedImage image;

        try
        {
            image = ImageIO.read(new File(filename));
        }
        catch (IOException e)
        {
            throw new IOException("Can't open image '" + filename + "'.", e);
        }

        if (image == null)
            throw new IOException("Can't open image '" + filename + "'.");

        return getBasicInfo(image);
    }

    /**
     * Return pixels (indexed color) array from the specified image file
     * 
     * @param filename
     * @return
     * @throws IOException
     */
    public static byte[] getIndexedPixels(String filename) throws IOException
    {
        final BufferedImage image = ImageIO.read(new File(filename));
        if (image == null)
            throw new IOException("Can't open image '" + filename + "'.");

        // not a gray scale image ?
        if (image.getType() != BufferedImage.TYPE_BYTE_GRAY)
        {
            final ColorModel cm = image.getColorModel();
            if (!(cm instanceof IndexColorModel))
                throw new IllegalArgumentException("Image '" + filename
                        + "' is RGB, only indexed images (8bpp, 4bpp, 2bpp and 1bpp) are supported !");
        }

        final DataBuffer db = image.getRaster().getDataBuffer();
        if (!(db instanceof DataBufferByte))
            throw new IllegalArgumentException("Image '" + filename + "' error: unexpected data buffer format !");

        return ((DataBufferByte) db).getData();
    }

    public static byte[] getSubImage(byte[] image8bpp, Dimension imageDim, Rectangle region)
    {
        final int sw = imageDim.width;
        final int dw = region.width;
        final int h = region.height;
        final byte[] result = new byte[dw * h];

        int offsetSrc = (region.y * sw) + region.x;
        int offsetDst = 0;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < dw; x++)
                result[offsetDst++] = image8bpp[offsetSrc++];

            offsetSrc += sw - dw;
        }

        return result;
    }

    public static boolean isTransparent(byte[] image8bpp, Dimension imageDim, Rectangle region)
    {
        final Rectangle adjRegion = region.intersection(new Rectangle(imageDim));

        int offset = (adjRegion.y * imageDim.width) + adjRegion.x;
        for (int y = 0; y < adjRegion.height; y++)
        {
            for (int x = 0; x < adjRegion.width; x++)
                if (image8bpp[offset++] != 0)
                    return false;

            offset += imageDim.width - adjRegion.width;
        }

        return true;
    }

    public static int getOpaquePixelCount(byte[] image8bpp, Dimension imageDim, Rectangle region)
    {
        final Rectangle adjRegion = region.intersection(new Rectangle(imageDim));

        int result = 0;
        int offset = (adjRegion.y * imageDim.width) + adjRegion.x;
        for (int y = 0; y < adjRegion.height; y++)
        {
            for (int x = 0; x < adjRegion.width; x++)
                if (image8bpp[offset++] != 0)
                    result++;

            offset += imageDim.width - adjRegion.width;
        }

        return result;
    }

    public static boolean hasOpaquePixelOnEdge(byte[] image8bpp, Dimension imageDim, Rectangle region, boolean left,
            boolean top, boolean right, boolean bottom)
    {
        final Rectangle adjRegion = region.intersection(new Rectangle(imageDim));
        final int w = imageDim.width;

        if (left)
        {
            int offset = (adjRegion.y * imageDim.width) + adjRegion.x;
            for (int i = adjRegion.y; i < adjRegion.y + adjRegion.height; i++)
            {
                if (image8bpp[offset] != 0)
                    return true;
                offset += w;
            }
        }
        if (top)
        {
            int offset = (adjRegion.y * imageDim.width) + adjRegion.x;
            for (int i = adjRegion.x; i < adjRegion.x + adjRegion.width; i++)
            {
                if (image8bpp[offset] != 0)
                    return true;
                offset++;
            }
        }
        if (right)
        {
            int offset = (adjRegion.y * imageDim.width) + adjRegion.x + (adjRegion.width - 1);
            for (int i = adjRegion.y; i < adjRegion.y + adjRegion.height; i++)
            {

                if (image8bpp[offset] != 0)
                    return true;
                offset += w;
            }
        }
        if (bottom)
        {
            int offset = ((adjRegion.y + (adjRegion.height - 1)) * imageDim.width) + adjRegion.x;
            for (int i = adjRegion.x; i < adjRegion.x + adjRegion.width; i++)
            {
                if (image8bpp[offset] != 0)
                    return true;
                offset++;
            }
        }

        return false;
    }

    /**
     * Returns used palette index
     */
    public static int getSpritePaletteIndex(byte[] image8bpp, int imgW, int imgH) throws IllegalArgumentException
    {
        int pal = -1;
        int srcOff = 0;
        for (int y = 0; y < imgH; y++)
        {
            for (int x = 0; x < imgW; x++)
            {
                final int pixel = TypeUtil.unsign(image8bpp[srcOff++]);
                final int color = pixel & 0xF;

                // not a transparent pixel ?
                if (color != 0)
                {
                    final int curPal = (pixel >> 4) & 0xF;

                    // set palette
                    if (pal == -1)
                        pal = curPal;
                    else if (pal != curPal)
                        throw new IllegalArgumentException("Error: pixel at [" + x + "," + y
                                + "] reference a different palette (" + curPal + " != " + pal + ").");
                }
            }
        }

        return (pal == -1) ? 0 : pal;
    }

    /**
     * Returns RGBA palette (A in high bits and R in low bits)
     */
    public static int[] getRGBA8888Palette(String filename) throws IOException, IllegalArgumentException
    {
        final BufferedImage image = ImageIO.read(new File(filename));
        if (image == null)
            throw new IOException("Can't open image '" + filename + "'.");

        // special case of 8bit gray scale image
        if (image.getType() == BufferedImage.TYPE_BYTE_GRAY)
        {
            final int[] result = new int[256];

            for (int i = 0; i < result.length; i++)
                result[i] = 0xFF000000 | (i << 16) | (i << 8) | (i << 0);

            return result;
        }

        final ColorModel cm = image.getColorModel();
        if (!(cm instanceof IndexColorModel))
            throw new IllegalArgumentException(
                    "Image '" + filename + "' is RGB, only indexed images (8bpp, 4bpp, 2bpp and 1bpp) are supported !");

        final IndexColorModel icm = (IndexColorModel) cm;
        final int[] result = new int[icm.getMapSize()];

        for (int i = 0; i < result.length; i++)
            result[i] = (icm.getAlpha(i) << 24) | (icm.getBlue(i) << 16) | (icm.getGreen(i) << 8)
                    | (icm.getRed(i) << 0);

        return result;
    }

    /**
     * Returns RGBA4444 palette (A in high bits and R in low bits)
     */
    public static short[] getRGBA4444Palette(String filename, int mask) throws IOException
    {
        return convertRGBA8888toRGBA4444(getRGBA8888Palette(filename), mask);
    }

    public static short[] convertRGBA8888toRGBA4444(int[] pixels, int mask)
    {
        final short[] result = new short[pixels.length];

        for (int i = 0; i < pixels.length; i++)
        {
            final int p = pixels[i];
            final int a = (p & 0xF0000000) >> (24 + 4);
            final int b = (p & 0x00F00000) >> (16 + 4);
            final int g = (p & 0x0000F000) >> (8 + 4);
            final int r = (p & 0x000000F0) >> (0 + 4);

            result[i] = (short) (((a << 12) | (b << 8) | (g << 4) | (r << 0)) & mask);
        }

        return result;
    }

    public static short[] convertRGBA8888toRGBA4444(int[] pixels)
    {
        return convertRGBA8888toRGBA4444(pixels, 0xFFFF);
    }

    static byte[] convert1bppTo8bpp(byte[] data)
    {
        final byte[] result = new byte[data.length * 8];

        for (int i = 0; i < data.length; i++)
        {
            result[(i * 8) + 0] = (byte) ((data[i] >> 7) & 0x01);
            result[(i * 8) + 1] = (byte) ((data[i] >> 6) & 0x01);
            result[(i * 8) + 2] = (byte) ((data[i] >> 5) & 0x01);
            result[(i * 8) + 3] = (byte) ((data[i] >> 4) & 0x01);
            result[(i * 8) + 4] = (byte) ((data[i] >> 3) & 0x01);
            result[(i * 8) + 5] = (byte) ((data[i] >> 2) & 0x01);
            result[(i * 8) + 6] = (byte) ((data[i] >> 1) & 0x01);
            result[(i * 8) + 7] = (byte) ((data[i] >> 0) & 0x01);
        }

        return result;
    }

    static byte[] convert2bppTo8bpp(byte[] data)
    {
        final byte[] result = new byte[data.length * 4];

        for (int i = 0; i < data.length; i++)
        {
            result[(i * 4) + 0] = (byte) ((data[i] >> 6) & 0x03);
            result[(i * 4) + 1] = (byte) ((data[i] >> 4) & 0x03);
            result[(i * 4) + 2] = (byte) ((data[i] >> 3) & 0x03);
            result[(i * 4) + 3] = (byte) ((data[i] >> 0) & 0x03);
        }

        return result;
    }

    static byte[] convert4bppTo8bpp(byte[] data)
    {
        final byte[] result = new byte[data.length * 2];

        for (int i = 0; i < data.length; i++)
        {
            result[(i * 2) + 0] = (byte) ((data[i] >> 4) & 0x0F);
            result[(i * 2) + 1] = (byte) ((data[i] >> 0) & 0x0F);
        }

        return result;
    }

    static byte[] convert1bppTo4bpp(byte[] data)
    {
        final byte[] result = new byte[data.length * 4];

        for (int i = 0; i < data.length; i++)
        {
            result[(i * 4) + 0] = (byte) (((data[i] >> 7) & 0x01) << 4);
            result[(i * 4) + 0] |= (byte) (((data[i] >> 6) & 0x01) << 0);
            result[(i * 4) + 1] = (byte) (((data[i] >> 5) & 0x01) << 4);
            result[(i * 4) + 1] |= (byte) (((data[i] >> 4) & 0x01) << 0);
            result[(i * 4) + 2] = (byte) (((data[i] >> 3) & 0x01) << 4);
            result[(i * 4) + 2] |= (byte) (((data[i] >> 2) & 0x01) << 0);
            result[(i * 4) + 3] = (byte) (((data[i] >> 1) & 0x01) << 4);
            result[(i * 4) + 3] |= (byte) (((data[i] >> 0) & 0x01) << 0);
        }

        return result;
    }

    static byte[] convert2bppTo4bpp(byte[] data)
    {
        final byte[] result = new byte[data.length * 2];

        for (int i = 0; i < data.length; i++)
        {
            result[(i * 2) + 0] = (byte) (((data[i] >> 6) & 0x03) << 4);
            result[(i * 2) + 0] |= (byte) (((data[i] >> 4) & 0x03) << 0);
            result[(i * 2) + 1] = (byte) (((data[i] >> 2) & 0x03) << 4);
            result[(i * 2) + 1] |= (byte) (((data[i] >> 0) & 0x03) << 0);
        }

        return result;
    }

    static byte[] convert8bppTo4bpp(byte[] data)
    {
        final byte[] result = new byte[data.length / 2];

        for (int i = 0; i < data.length / 2; i++)
        {
            result[i] = (byte) ((data[(i * 2) + 0] & 0x0F) << 4);
            result[i] |= (byte) ((data[(i * 2) + 1] & 0x0F) << 0);
        }

        return result;
    }

    public static byte[] convertTo4bpp(byte[] data, int inputBpp)
    {
        switch (inputBpp)
        {
            default:
                throw new IllegalArgumentException("Not supported image data format (" + inputBpp + " bpp)");
            case 1:
                return convert1bppTo4bpp(data);
            case 2:
                return convert2bppTo4bpp(data);
            case 4:
                return data;
            case 8:
                return convert8bppTo4bpp(data);
        }
    }

    public static byte[] convertTo8bpp(byte[] data, int inputBpp)
    {
        switch (inputBpp)
        {
            default:
                throw new IllegalArgumentException("Not supported image data format (" + inputBpp + " bpp)");
            case 1:
                return convert1bppTo8bpp(data);
            case 2:
                return convert2bppTo8bpp(data);
            case 4:
                return convert4bppTo8bpp(data);
            case 8:
                return data;
        }
    }
}
