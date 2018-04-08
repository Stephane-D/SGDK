package org.sgdk.resourcemanager.entities;

import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;
import java.io.File;
import java.net.URISyntaxException;

import javax.imageio.ImageIO;
import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.apache.commons.io.FilenameUtils;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

import com.fasterxml.jackson.databind.JsonNode;

public class SGDKBackground extends SGDKElement{
	
	public static final int PALETTE_SIZE = 16;
//	private static final int BPP = 8;

	public enum ValidFormat{
		png, bmp
	}
	
	public enum Compression{
		BEST(-1), NONE(0), APLIB(1), FAST(2);
		
		private int value;
		
		private Compression(int value) {
			this.value = value;
		}
		
		public int getValue() {
			return value;
		}
	}
	
	private Compression compression = Compression.BEST;
	
	public SGDKBackground() {};
	
	public SGDKBackground(JsonNode node) throws SGDKInvalidFormatException {
		super(node);
		setType(Type.SGDKBackground);
		BufferedImage img;
		IndexColorModel icm;
		try {
			img = ImageIO.read(new File(getPath()));
			icm = (IndexColorModel)img.getColorModel();
		} catch (Exception e) {
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
		if(icm.getMapSize() != PALETTE_SIZE) {
			throw new SGDKInvalidFormatException("Palette Size is not 16. Palette size is " + icm.getMapSize() +" " + toString());
		}
		this.compression = Compression.valueOf(node.get("compression").asText());
	};
	
	public SGDKBackground(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKBackground);
		BufferedImage img;
		IndexColorModel icm;
		try {
			img = ImageIO.read(new File(path));
			icm = (IndexColorModel)img.getColorModel();
		} catch (Exception e) {
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
		if(icm.getMapSize() != PALETTE_SIZE) {
			throw new SGDKInvalidFormatException("Palette Size is not 16. Palette size is " + icm.getMapSize() +" " + toString());
		}
//		if(icm.getPixelSize() != BPP) {
//			throw new SGDKInvalidFormatException("bpp is not 8. Palette size is " + icm.getPixelSize() +" " + toString());
//		}
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
