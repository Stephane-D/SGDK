package org.sgdk.resourcemanager.ui.utils.indexedimage;

import java.awt.Color;
import java.awt.image.DataBuffer;
import java.awt.image.IndexColorModel;
import java.io.IOException;
import java.util.HashMap;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ij.IJ;
import ij.ImagePlus;
import ij.io.FileInfo;
import ij.process.ImageConverter;
import ij.process.ImageProcessor;

public class ImageUtil {

	private static final Logger logger = LogManager.getLogger("UILogger");

	private static final int PALETTE_SIZE = 16;
	private static final int BITS8 = 8;

	public static void validateAndCreateIndexedImage(String path) throws IOException {
		ImagePlus ip = new ImagePlus(path);

		if (!is8BitsColorImageIndexed(ip)) {
			logger.info("Image is not an indexed image");
			logger.info("Starting conversion ...");
			
		    ImageConverter ic = new ImageConverter(ip);
		    ic.convertRGBtoIndexedColor(PALETTE_SIZE);
		    IJ.saveAs(ip, "PNG", path);
		}
	}
	
	public static boolean is8BitsColorImageIndexed(ImagePlus ip) {
		FileInfo fileInfo = ip.getFileInfo();
		return fileInfo.fileType == FileInfo.COLOR8;
	}
	
	public static int getPaletteSize(ImagePlus ip) {
		HashMap<Color, Color> colorMap = getPalette(ip);
		return colorMap.size() < PALETTE_SIZE? PALETTE_SIZE: colorMap.size();
	}
	
	public static boolean exist(ImagePlus ip, Color color) {
		HashMap<Color, Color> colorMap = getPalette(ip);
		return colorMap.containsKey(color);
	}
	
	private static HashMap<Color, Color> getPalette(ImagePlus ip) {
		HashMap<Color, Color> colorMap = new HashMap<>();
		IndexColorModel indexColorModel = (IndexColorModel) ip.getBufferedImage().getColorModel();
		FileInfo fileInfo = ip.getFileInfo();
		for(int i = 0; i< fileInfo.lutSize; i++) {
			Color color = new Color(indexColorModel.getRGB(i));
			if(!colorMap.containsKey(color)) {
				colorMap.put(color, color);
			}			
		}
		return colorMap;
	}

	public static int getHeight(ImagePlus ip) {
		FileInfo fileInfo = ip.getFileInfo();
		return fileInfo.height;
	}
	
	public static int getWidth(ImagePlus ip) {
		FileInfo fileInfo = ip.getFileInfo();
		return fileInfo.width;
	}
	
	public static Color getColorFromIndex(ImagePlus ip, int i) {
		IndexColorModel indexColorModel = (IndexColorModel) ip.getBufferedImage().getColorModel();
		if(i > getPaletteSize(ip)) {
			return new Color(0);
		}
		return new Color(indexColorModel.getRGB(i));
	}

	public static void changeImagePalette(String path, Color[] newPalette){
		
		byte[] r = new byte[PALETTE_SIZE];
		byte[] g = new byte[PALETTE_SIZE];
		byte[] b = new byte[PALETTE_SIZE];
		
		int i = 0;
		for(Color c : newPalette) {
			r[i]=(byte)c.getRed();
			g[i]=(byte)c.getGreen();
			b[i]=(byte)c.getBlue();
			i++;
		}
		ImagePlus imp = new ImagePlus(path);

		IndexColorModel cm = new IndexColorModel(BITS8, PALETTE_SIZE, r, g, b);
        ImageProcessor ip = imp.getProcessor();
        ip.setColorModel(cm);
        if (imp.getStackSize()>1)
            imp.getStack().setColorModel(cm);
        IJ.saveAs(imp, "PNG", path);
	}
	
	public static void switchColorsImagePalette(String path, Color[] newPalette, int index1, int index2){
		changeImagePalette(path, newPalette);
		ImagePlus imp = new ImagePlus(path);
		DataBuffer db = imp.getBufferedImage().getRaster().getDataBuffer();
		for(int i = 0; i< db.getSize(); i++) {
			int val = db.getElem(i);
			if(val == index1) {
				db.setElem(i, index2);
			}else if(val == index2) {
				db.setElem(i, index1);
			}
		}
		IJ.saveAs(imp, "PNG", path);
	}
}
