package org.sgdk.resourcemanager.entities;

import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

public class SGDKProject extends SGDKFolder{		

	public SGDKProject() {};
	
	public SGDKProject(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKProject);
	}
	
	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
				getClass().getResource("/icons/053-folder-24.svg").toURI(),
				16,
				16);
	}
}
