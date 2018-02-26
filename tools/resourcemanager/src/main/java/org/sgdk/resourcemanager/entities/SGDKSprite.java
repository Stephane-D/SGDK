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

public class SGDKSprite extends SGDKElement{

	public enum ValidFormat{
		png
	}
	
	public enum Compression{
		BEST,
		NONE,
		APLIB,
		FAST
	}
	
	public enum Collision{
		NONE,
		BOX,
		CIRCLE
	}
	private int width;
	private int heigth;
	private Compression compression = Compression.BEST;
	private int time = 0;
	private Collision collision = Collision.NONE;
	
	public SGDKSprite() {};

	public SGDKSprite(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKSprite);
		BufferedImage img;
		try {
			img = ImageIO.read(new File(path));
		} catch (IOException e) {
			throw new SGDKInvalidFormatException(e.getMessage(), e);
		}
		width = img.getWidth();
		heigth = img.getHeight();
		if(width % 8 != 0) {
			throw new SGDKInvalidFormatException("Sprite width is not a multiple of 8 " + toString());
		}
		if(heigth % 8 != 0) {
			throw new SGDKInvalidFormatException("Sprite heigth is not a multiple of 8 " + toString());
		}
		width = width/8;
		heigth = heigth/8;
	}
	
	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
				getClass().getResource("/icons/097-people-1.svg").toURI(),
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

	public int getWidth() {
		return width;
	}

	public void setWidth(int width) {
		this.width = width;
	}

	public int getHeigth() {
		return heigth;
	}

	public void setHeigth(int heigth) {
		this.heigth = heigth;
	}

	public Compression getCompression() {
		return compression;
	}

	public void setCompression(Compression compression) {
		this.compression = compression;
	}

	public int getTime() {
		return time;
	}

	public void setTime(int time) {
		this.time = time;
	}

	public Collision getCollision() {
		return collision;
	}

	public void setCollision(Collision collision) {
		this.collision = collision;
	}

	
}
