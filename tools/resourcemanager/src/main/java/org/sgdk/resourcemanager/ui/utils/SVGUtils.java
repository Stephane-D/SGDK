package org.sgdk.resourcemanager.ui.utils;

import java.awt.image.BufferedImage;
import java.net.URI;

import javax.swing.Icon;
import javax.swing.ImageIcon;

import org.apache.batik.dom.svg.SVGDOMImplementation;
import org.apache.batik.transcoder.TranscoderException;
import org.apache.batik.transcoder.TranscoderInput;
import org.apache.batik.transcoder.TranscodingHints;
import org.apache.batik.transcoder.image.ImageTranscoder;
import org.apache.batik.util.SVGConstants;

public class SVGUtils {
	
	public static Icon load(URI uri, int width, int height) throws TranscoderException {
		MyTranscoder transcoder = new MyTranscoder();
	    TranscodingHints hints = new TranscodingHints();
	    hints.put(ImageTranscoder.KEY_WIDTH, new Float(width));
	    hints.put(ImageTranscoder.KEY_HEIGHT,  new Float(height));
	    hints.put(ImageTranscoder.KEY_DOM_IMPLEMENTATION, SVGDOMImplementation.getDOMImplementation());
	    hints.put(ImageTranscoder.KEY_DOCUMENT_ELEMENT_NAMESPACE_URI,SVGConstants.SVG_NAMESPACE_URI);
	    hints.put(ImageTranscoder.KEY_DOCUMENT_ELEMENT_NAMESPACE_URI,SVGConstants.SVG_NAMESPACE_URI);
	    hints.put(ImageTranscoder.KEY_DOCUMENT_ELEMENT, SVGConstants.SVG_SVG_TAG);
	    hints.put(ImageTranscoder.KEY_XML_PARSER_VALIDATING, false);
	    transcoder.setTranscodingHints(hints);
		transcoder.transcode(new TranscoderInput(uri.toString()), null);
	    BufferedImage image = transcoder.getImage();
	    return new ImageIcon(image);
	}

}