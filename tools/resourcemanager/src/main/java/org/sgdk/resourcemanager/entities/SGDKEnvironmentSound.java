package org.sgdk.resourcemanager.entities;

import java.io.IOException;
import java.net.URISyntaxException;

import javax.swing.Icon;

import org.apache.batik.transcoder.TranscoderException;
import org.apache.commons.io.FilenameUtils;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

import com.fasterxml.jackson.databind.JsonNode;

public class SGDKEnvironmentSound extends SGDKElement{

	public enum Timing {
		AUTO(-1), NTSC(0), PAL(1);
		
		private int value;
		
		private Timing(int value) {
			this.value = value;
		}
		
		public int getValue() {
			return value;
		}
	}

	public enum ValidFormat{
		vgm, vgz
	}
	
	private Timing timing = Timing.AUTO;
	private String options = "";
	
	public SGDKEnvironmentSound() {};
	
	public SGDKEnvironmentSound(JsonNode node) throws SGDKInvalidFormatException, IOException {
		super(node);
		this.timing = Timing.valueOf(node.get("timing").asText());
		this.options = node.get("options").asText();
	};
	
	public SGDKEnvironmentSound(String path) throws SGDKInvalidFormatException, IOException {
		super(path);
	}	

	public Timing getTiming() {
		return timing;
	}

	public void setTiming(Timing timing) {
		this.timing = timing;
	}

	public String getOptions() {
		return options;
	}

	public void setOptions(String options) {
		this.options = options;
	}

	@Override
	public Icon calculateIcon() throws TranscoderException, URISyntaxException {
		return SVGUtils.load(
			getClass().getResource("/icons/029-ebook.svg").toURI(),
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

	@Override
	protected void init() throws SGDKInvalidFormatException {
		setType(Type.SGDKEnvironmentSound);
	}
}
