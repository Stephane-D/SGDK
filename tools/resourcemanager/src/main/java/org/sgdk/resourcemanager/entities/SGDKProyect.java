package org.sgdk.resourcemanager.entities;

import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.SVGUtils;

public class SGDKProyect extends SGDKFolder{		

	public SGDKProyect() {};
	
	public SGDKProyect(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKProyect);
	}
	
	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
				getClass().getResource("/icons/053-folder-24.svg").toURI(),
				16,
				16);
	}
}
