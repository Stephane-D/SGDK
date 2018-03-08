package org.sgdk.resourcemanager.ui.utils.svg;

import java.awt.image.BufferedImage;

import org.apache.batik.transcoder.TranscoderOutput;
import org.apache.batik.transcoder.image.ImageTranscoder;

class MyTranscoder extends ImageTranscoder {
	
	private BufferedImage image = null;

	public BufferedImage createImage(int w, int h) {
		image = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
		return image;
	}

	public void writeImage(BufferedImage img, TranscoderOutput out) {
	}

	public BufferedImage getImage() {
		return image;
	}
}
