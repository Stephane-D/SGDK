package org.sgdk.resourcemanager.entities;

import java.io.IOException;
import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

import com.fasterxml.jackson.databind.JsonNode;

public class SGDKProject extends SGDKFolder{		

	public SGDKProject() {};
	
	public SGDKProject(JsonNode node) throws SGDKInvalidFormatException, IOException {
		super(node);
	};
	
	public SGDKProject(String path) throws SGDKInvalidFormatException, IOException {
		super(path);
	}
	
	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
				getClass().getResource("/icons/053-folder-24.svg").toURI(),
				16,
				16);
	}
	
	@Override
	protected void init() throws SGDKInvalidFormatException {
		super.init();
		setType(Type.SGDKProject);
	}
}
