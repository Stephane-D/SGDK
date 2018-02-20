package org.sgdk.resourcemanager.entities;

import java.io.File;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.SVGUtils;

public class SGDKFolder extends SGDKElement{

	private List<SGDKElement> childs = new ArrayList<SGDKElement>();
	
	public SGDKFolder() {};

	public SGDKFolder(String path) throws SGDKInvalidFormatException {
		super(path);
		setType(Type.SGDKFolder);
	}
	
	@Override
	public String getName() {
		return toString();
	}
	
	public List<SGDKElement> getChilds() {
		return childs;
	}
	
	public void setChilds(List<SGDKElement> childs) {
		this.childs = childs;
	}
	
	public void addChild(SGDKElement child) {
		this.childs.add(child);
	}
	
	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
				getClass().getResource("/icons/020-folder-39.svg").toURI(),
				16,
				16);
	}

	@Override
	protected boolean validateFormat(String path) {
		return isValidFormat(path);
	}
	
	public static boolean isValidFormat(String path) {
		return new File(path).isDirectory();
	}

}
