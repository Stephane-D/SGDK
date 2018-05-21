package org.sgdk.resourcemanager.entities;

import java.io.IOException;
import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.apache.commons.io.FilenameUtils;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

import com.fasterxml.jackson.databind.JsonNode;

public class SGDKFXSound extends SGDKElement{

	public enum Driver {
		PCM("PCM"),
		ADPCM_2("2ADPCM"),
		PCM_4("4PCM"),
		VGM("VGM"),
		XGM("XGM");			
		
		private String value;
		
		private Driver(String value) {
			this.value = value;
		}
		
		public String getValue() {
			return value;
		}
	}

	public enum ValidFormat{
		wav
	}
	
	private Driver driver = Driver.PCM;	
	private int outrate = -1;	

	public SGDKFXSound() {};
	
	public SGDKFXSound(JsonNode node) throws SGDKInvalidFormatException, IOException {
		super(node);
		setType(Type.SGDKFXSound);
		this.driver = Driver.valueOf(node.get("driver").asText());
		this.outrate = node.get("outrate").asInt();
	};

	public SGDKFXSound(String path) throws SGDKInvalidFormatException, IOException {
		super(path);
		setType(Type.SGDKFXSound);
	}
	
	public Driver getDriver() {
		return driver;
	}

	public void setDriver(Driver driver) {
		this.driver = driver;
	}
		
	public int getOutrate() {
		return outrate;
	}

	public void setOutrate(int outrate) {
		this.outrate = outrate;
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
