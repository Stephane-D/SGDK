package org.sgdk.resourcemanager.ui.panels.preview;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKSprite;

public class PreviewPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public static final Color DEFAULT_BACKGROUND_COLOR = new Color(229, 9, 127);
	
	private Color backgroundColor = DEFAULT_BACKGROUND_COLOR;
	private float zoom = 1f;
	
	private SGDKElement element = null;
	private Image preview = null;

	private Color gridColor = null;
	
	public PreviewPanel() {
		super();
		addMouseWheelListener(new MouseWheelListener() {
			
			@Override
			public void mouseWheelMoved(MouseWheelEvent e) {
				if(element!=null && element instanceof SGDKBackground) {
					zoom += e.getWheelRotation()/10f;
					if(zoom < 0)zoom = 0;
					repaint();
				}				
			}
		});
	}

	public void paintImage(SGDKElement element) throws IOException {
		this.element = element;
		switch (element.getType()) {
		case SGDKBackground:
		case SGDKSprite:
			this.preview = ImageIO.read(new File(element.getPath()));
			break;
		default:
			this.preview = ((ImageIcon)element.getIcon()).getImage();
			break;
		}
		zoom = 1f;
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
					Math.round(screenWidth/2f - imageWidth/2f), Math.round(screenHeight/2f - imageHeight/2f),
					Math.round(screenWidth/2f + imageWidth/2f), Math.round(screenHeight/2f + imageHeight/2f),
					0, 0,
					preview.getWidth(null), preview.getHeight(null),
					null
			);
			if(gridColor!=null && element instanceof SGDKSprite) {
				SGDKSprite sprite = (SGDKSprite)element;
				g.setColor(gridColor);
				int xSize = new Long(Math.round(((screenWidth/2f + imageWidth/2f)-(screenWidth/2f - imageWidth/2f))/(sprite.getWidth() * zoom * 8))).intValue();
				float gridw = ((screenWidth/2f + imageWidth/2f)-(screenWidth/2f - imageWidth/2f))/xSize;
				for(int x = 0; x<xSize; x++) {
					int auxX = Math.round((screenWidth/2f - imageWidth/2f) + gridw * x);
					g.drawLine(auxX, Math.round(screenHeight/2f - imageHeight/2f), auxX, Math.round(screenHeight/2f + imageHeight/2f));
				}
				int ySize = new Long(Math.round(((screenHeight/2f + imageHeight/2f)-(screenHeight/2f - imageHeight/2f))/(sprite.getHeight() * zoom * 8))).intValue();
				float gridy = ((screenHeight/2f + imageHeight/2f)-(screenHeight/2f - imageHeight/2f))/ySize;
				for(int y = 0; y<ySize; y++) {					
					int auxY = Math.round((screenHeight/2f - imageHeight/2f) + gridy * y);
					g.drawLine(Math.round(screenWidth/2f - imageWidth/2f), auxY, Math.round(screenWidth/2f + imageWidth/2f), auxY);
				}
			}
		}		
	}

	public void clean() {
		preview = null;
		setBackgroundColor(DEFAULT_BACKGROUND_COLOR);
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

	public void setGridColor(Color color) {
		gridColor  = color;
		repaint();
	}	

}
