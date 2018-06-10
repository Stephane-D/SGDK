package org.sgdk.resourcemanager.entities;

import java.io.IOException;
import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.apache.commons.io.FilenameUtils;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.indexedimage.ImageUtil;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

import com.fasterxml.jackson.databind.JsonNode;

import ij.ImagePlus;

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
	
	public SGDKBackground(JsonNode node) throws SGDKInvalidFormatException, IOException {
		super(node);		
		this.compression = Compression.valueOf(node.get("compression").asText());
	};
	
	public SGDKBackground(String path) throws SGDKInvalidFormatException, IOException {
		super(path);		
	}
	
	@Override
	protected void init() throws SGDKInvalidFormatException {
		setType(Type.SGDKBackground);
		ImagePlus ip = new ImagePlus(getPath());
		
		int width = ImageUtil.getWidth(ip);
		int heigth = ImageUtil.getHeight(ip);
		if(width % 8 != 0) {
			throw new SGDKInvalidFormatException("Image width is not a multiple of 8 "+ toString());
		}
		if(heigth % 8 != 0) {
			throw new SGDKInvalidFormatException("Image heigth is not a multiple of 8 "+ toString());
		}
		if(!ImageUtil.is8BitsColorImageIndexed(ip)) {
			throw new SGDKInvalidFormatException("Image is not Indexed "+ toString());
		}
		int paletteSize = ImageUtil.getPaletteSize(ip);
		if(paletteSize != PALETTE_SIZE) {
			throw new SGDKInvalidFormatException("Palette Size is not 16. Palette size is " + paletteSize +" " + toString());
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
