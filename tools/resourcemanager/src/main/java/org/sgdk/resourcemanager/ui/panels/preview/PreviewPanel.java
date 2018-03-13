package org.sgdk.resourcemanager.ui.panels.preview;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Image;

import javax.swing.JPanel;

public class PreviewPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private Color backgroundColor = new Color(229, 9, 127);
	private float zoom = 1f;
	
	private Image preview = null;
	
	public PreviewPanel() {
		super();
		repaint();
	}

	public void paintImage(Image image) {
		zoom = 1f;
		preview = image;
		repaint();
	}
	
	@Override
	public void paint(Graphics g) {
		Dimension size = getSize();
		int screenWidth = size.width;
		int screenHeight = size.height;
		g.setColor(backgroundColor);
		g.fillRect(0, 0, screenWidth, screenHeight);
		if(preview!=null) {
			int imageWidth = Math.round(preview.getWidth(null) * zoom);
			int imageHeight = Math.round(preview.getHeight(null) * zoom);
			g.drawImage(preview,
					screenWidth/2 - imageWidth/2, screenHeight/2 - imageHeight/2,
					screenWidth/2 + imageWidth/2, screenHeight/2 + imageHeight/2,
					0, 0,
					preview.getWidth(null), preview.getHeight(null),
					null
			);
		}
	}

	public void clean() {
		preview = null;
		repaint();		
	}

	public void setZoom(float zoom) {
		this.zoom = zoom;
		repaint();
	}
	
	public float getZoom() {
		return zoom;
	}

	public Color getBackgroundColor() {
		return backgroundColor;
	}

	public void setBackgroundColor(Color backgroundColor) {
		this.backgroundColor = backgroundColor;
		repaint();
	}
	
	

}
