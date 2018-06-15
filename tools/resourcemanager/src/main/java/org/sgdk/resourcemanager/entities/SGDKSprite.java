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
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

import com.fasterxml.jackson.databind.JsonNode;

public class SGDKSprite extends SGDKBackground{
	
	public static final float TIME_MULTIPLICATOR = 60f;
	public static final int SCALE_MULTIPLICATOR = 8;

	public enum ValidFormat{
		png
	}	
	
	public enum Collision{
		NONE,
		BOX,
		CIRCLE
	}
	private int width;
	private int height;
	private int time = 0;
	private Collision collision = Collision.NONE;
	
	public SGDKSprite() {};
	
	public SGDKSprite(JsonNode node) throws SGDKInvalidFormatException, IOException {
		super(node);
		width = node.get("width").asInt();
		height = node.get("height").asInt();
		collision = Collision.valueOf(node.get("collision").asText());
		time = node.get("time").asInt();
	};

	public SGDKSprite(String path) throws SGDKInvalidFormatException, IOException {
		super(path);
		BufferedImage img;
		try {
			img = ImageIO.read(new File(path));
		} catch (IOException e) {
			throw new SGDKInvalidFormatException(e.getMessage(), e);
		}
		width = img.getWidth();
		height = img.getHeight();
		width = width/8;
		height = height/8;
	}
	
	@Override
	protected void init() throws SGDKInvalidFormatException {
		super.init();
		setType(Type.SGDKSprite);
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

	public int getHeight() {
		return height;
	}

	public void setHeight(int height) {
		this.height = height;
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
