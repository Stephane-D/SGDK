package org.sgdk.resourcemanager.ui.utils.indexedimage;

import java.awt.Color;
import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;
import java.io.File;
import java.io.IOException;
import java.util.HashMap;

import javax.imageio.ImageIO;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class ImageUtil {

	private static final Logger logger = LogManager.getLogger("UILogger");

	private static final int BPP = 8;
	private static final int PALETTE_SIZE = 16;

	public static void validateAndCreateIndexedImage(String path) throws IOException {
		File file = new File(path);
		BufferedImage image = ImageIO.read(file);

		if (!(image.getColorModel() instanceof IndexColorModel)) {
			logger.info("Image is not an indexed image");
			logger.info("Starting conversion ...");

			image = convertToIndexedImage(image, BPP, PALETTE_SIZE);
			ImageIO.write(image, "GIF", file);
			
		}
	}

	private static BufferedImage convertToIndexedImage(BufferedImage image, int bpp, int paletteSize)
			throws IOException {
		HashMap<Color, Integer> hashColors = new HashMap<>();
		
		for (int x = 0; x < image.getWidth(); x++) {
			for (int y = 0; y < image.getHeight(); y++) {
				int[] rgb = getPixelData(image, x, y);
				Color color = new Color(rgb[0], rgb[1], rgb[2]);
				if (hashColors.get(color) == null) {
					hashColors.put(color, 1);
				} else {
					hashColors.put(color, hashColors.get(color) + 1);
				}
			}
		}

		Color[] paletteColors = configurePalette(hashColors, PALETTE_SIZE);
		
		byte[] r = new byte[PALETTE_SIZE];
		byte[] g = new byte[PALETTE_SIZE];
		byte[] b = new byte[PALETTE_SIZE];
		
		int i = 0;
		for(Color c : paletteColors) {
			r[i]=(byte)c.getRed();
			g[i]=(byte)c.getGreen();
			b[i]=(byte)c.getBlue();
			i++;
		}
		
		BufferedImage indexedImage = new BufferedImage(image.getWidth(), image.getHeight(),
				BufferedImage.TYPE_BYTE_INDEXED, new IndexColorModel(8, PALETTE_SIZE, r,g,b));
		
		indexedImage.getGraphics().drawImage(image, 0, 0, null);
		return indexedImage;
	}

	private static Color[] configurePalette(HashMap<Color, Integer> hashColors, int paletteSize) {
		Color[] palette = new Color[paletteSize];
		if(hashColors.size() > paletteSize) {
			int[] colorFreq = new int[paletteSize];
			int minFreq = 0;
			for(Color c : hashColors.keySet()) {
				int myColorFreq = hashColors.get(c).intValue();
				if( myColorFreq > minFreq) {
					int i = 0;
					while(colorFreq[i] > myColorFreq) {
						i++;
					}
					for(int j = colorFreq.length -1; j>i;j--) {
						colorFreq[j] = colorFreq[j-1];
						palette[j] = palette[j-1];
					}
					colorFreq[i] = myColorFreq;
					palette[i] = c;
					minFreq = colorFreq[colorFreq.length - 1];
				}
			}
		}else {
			int i = 0;
			for(Color c:hashColors.keySet()) {
				palette[i++]=c;
			}
			while(i<paletteSize) {
				palette[i++]=new Color(0, 0, 0);
			}
		}
		return palette;
	}

	private static int[] getPixelData(BufferedImage img, int x, int y) {
		int argb = img.getRGB(x, y);

		int rgb[] = new int[] {
//			(argb >> 24) & 0xff, //alpha
		    (argb >> 16) & 0xff, //red
		    (argb >>  8) & 0xff, //green
		    (argb      ) & 0xff  //blue
		};
		
		return rgb;
	}
}
