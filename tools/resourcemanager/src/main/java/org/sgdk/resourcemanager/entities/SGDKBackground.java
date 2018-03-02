package org.sgdk.resourcemanager.entities;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;

import javax.imageio.ImageIO;
import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.apache.commons.io.FilenameUtils;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.SVGUtils;

public class SGDKBackground extends SGDKElement{

	public enum ValidFormat{
		png, bmp
	}
	
	public enum Compression{
		BEST,
		NONE,
		APLIB,
		FAST
	}
	
	private Compression compression = Compression.BEST;
	
	public SGDKBackground() {};
	
	public SGDKBackground(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKBackground);
		BufferedImage img;
		try {
			img = ImageIO.read(new File(path));
		} catch (IOException e) {
			throw new SGDKInvalidFormatException(e.getMessage(), e);
		}
		int width = img.getWidth();
		int heigth = img.getHeight();
		if(width % 8 != 0) {
			throw new SGDKInvalidFormatException("Image width is not a multiple of 8 "+ toString());
		}
		if(heigth % 8 != 0) {
			throw new SGDKInvalidFormatException("Image heigth is not a multiple of 8 "+ toString());
		}
	}

	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {	
		return SVGUtils.load(
				getClass().getResource("/icons/022-picture.svg").toURI(),
				16,
				16);
	}

	@Override
	protected boolean validateFormat(String path) {
		return isValidFormat(path);
	}
	
	public static boolean isValidFormat(String path) {
		boolean b = false;
		String myExtension = FilenameUtils.getExtension(path);
		for(ValidFormat f: ValidFormat.values()) {
			b = b || f.toString().equals(myExtension);
		}		
		return b;
	}

	public Compression getCompression() {
		return compression;
	}
	
	public void setCompression(Compression compression) {
		this.compression = compression;
	}

}

//final int[] colourMap = {   0x00000000, 0xff000000, 0xffffffff, 0xff353535, 0xff888888, 0xff969696, 0xff237fe9, 0xffff0000 };
//IndexColorModel colorModel = new IndexColorModel(8, colourMap.length, colourMap, 0, true, 0, DataBuffer.TYPE_BYTE );
//BufferedImage image = new BufferedImage(IMAGE_WIDTH, IMAGE_HEIGHT, BufferedImage.TYPE_BYTE_INDEXED, colorModel);
//
//// Do whatever with your image 
//// ...
//ImageIO.write(image, "PNG", imageFile);
