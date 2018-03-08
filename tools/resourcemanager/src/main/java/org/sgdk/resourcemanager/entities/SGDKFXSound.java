package org.sgdk.resourcemanager.entities;

import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.apache.commons.io.FilenameUtils;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

public class SGDKFXSound extends SGDKElement{

	public enum ValidFormat{
		wav
	}
	
	public SGDKFXSound() {};

	public SGDKFXSound(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKFXSound);
	}
	
	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
				getClass().getResource("/icons/028-ebook-1.svg").toURI(),
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
}
