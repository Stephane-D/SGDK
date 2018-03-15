package org.sgdk.resourcemanager.ui.utils.indexedimage;

import java.awt.image.BufferedImage;
import java.awt.image.ComponentColorModel;
import java.awt.image.IndexColorModel;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;

import org.apache.commons.io.FilenameUtils;
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
			BufferedImage indexedImage = new BufferedImage(
				image.getWidth(),
				image.getHeight(),
				BufferedImage.TYPE_BYTE_INDEXED,
				new IndexColorModel(BPP, PALETTE_SIZE, new byte[PALETTE_SIZE], new byte[PALETTE_SIZE], new byte[PALETTE_SIZE])
			);
			
			indexedImage.getGraphics().drawImage(image, 0, 0, null);
			ImageIO.write(indexedImage, FilenameUtils.getExtension(path), file);
			//TODO reescalado
//			BufferedImage imageNew = ImageIO.read(file);
//			
//			IndexColorModel icm = (IndexColorModel)imageNew.getColorModel();
//			if(icm.getMapSize() > 16){
//				indexedImage = reducePaletteColors(image, BPP, PALETTE_SIZE, path, file);
//			}
		}
	}

	//TODO reescalado
	@SuppressWarnings("unused")
	private static BufferedImage reducePaletteColors(BufferedImage image, int bpp, int paletteSize, String path, File file) throws IOException {
		logger.info("Resizing palette size...");
		// TODO Auto-generated method stub
		//Ordenamos en base los colores mas abundantes (mejor) y mas distantes de la media (mejor) 
		//para quedarnos con esos colores
		//El resto de colores se asemejarán al color más cercano según la tonalidad.
		
		ComponentColorModel ccm = (ComponentColorModel)image.getColorModel();
		
		BufferedImage indexedImage = new BufferedImage(
			image.getWidth(),
			image.getHeight(),
			BufferedImage.TYPE_BYTE_INDEXED,
			new IndexColorModel(BPP, PALETTE_SIZE, new byte[PALETTE_SIZE], new byte[PALETTE_SIZE], new byte[PALETTE_SIZE])
		);
		
		BufferedImage indexedImageHuge = new BufferedImage(
			image.getWidth(),
			image.getHeight(),
			BufferedImage.TYPE_BYTE_INDEXED,
			new IndexColorModel(BPP, PALETTE_SIZE, new byte[PALETTE_SIZE], new byte[PALETTE_SIZE], new byte[PALETTE_SIZE])
		);
		
		indexedImageHuge.getGraphics().drawImage(image, 0, 0, null);
		ImageIO.write(indexedImageHuge, FilenameUtils.getExtension(path), file);
		BufferedImage imageNew = ImageIO.read(file);
//		
//		byte[] reds = new byte[icm.getMapSize()];
//		byte[] greens = new byte[icm.getMapSize()];
//		byte[] blues = new byte[icm.getMapSize()];
//		
//		icm.getReds(reds);
//		icm.getGreens(greens);
//		icm.getBlues(blues);
		
		byte[] newReds = new byte[paletteSize];
		byte[] newGreens = new byte[paletteSize];
		byte[] newBlues = new byte[paletteSize];
		
		
//			for(icm.getColorSpace()) {
//				
//			}
		
//			indexedImage2.getGraphics().drawImage(indexedImage, 0, 0, null);
			

		
		
		return image;
	}

}
