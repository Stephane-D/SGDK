package org.sgdk.resourcemanager.ui.panels.preview.toolbar.spriteplayer;

import java.awt.Graphics;
import java.awt.Image;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;

import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKSprite;

import ij.ImagePlus;

public class SpritePlayerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private SGDKSprite sprite = null;
	private SpritePlayerThread thread;

	private int animationIndex = -1;
	private int frame = -1;
	private int numFrames = -1;
	private int numAnimations = -1;
	private float zoom = 1f;

	private Image[][] frames = null;

	private int w;
	private int h;
	
	public SpritePlayerPanel() {
		super();
		addMouseWheelListener(new MouseWheelListener() {
			
			@Override
			public void mouseWheelMoved(MouseWheelEvent e) {
				if(sprite!=null) {
					zoom += e.getWheelRotation()/10f;
					if(zoom < 0)zoom = 0;
					repaint();
				}				
			}
		});
	}

	public void runAnimation(SGDKSprite sprite, int animationIndex) {
		this.sprite = sprite;
		ImagePlus image = new ImagePlus(sprite.getPath());
		w = sprite.getWidth() * SGDKSprite.SCALE_MULTIPLICATOR;
		h = sprite.getHeight() * SGDKSprite.SCALE_MULTIPLICATOR;
		numFrames  = new Long(image.getWidth()/(w)).intValue();
		numAnimations = new Long(image.getHeight()/(h)).intValue();
		if(animationIndex < numAnimations) {
			frames = new Image[numAnimations][numFrames];
			for(int i = 0; i<numAnimations; i++) {
				for(int j = 0; j<numFrames; j++) {
					frames[i][j] = image.getBufferedImage().getSubimage(j * w, i * h, w, h);
				}
			}
			
			thread = new SpritePlayerThread(this, animationIndex, numFrames);
			thread.start();
		}		
	}	
	
	public void stopAnimation() {
		thread.stopSafely();
		animationIndex = -1;
		frame = -1;
		numFrames = -1;
		numAnimations = -1;
		frames = null;
		zoom = 1f;
	}
	
	public SGDKSprite getSprite() {
		return sprite;
	}

	public void paintFrame(int animationIndex, int frame) {
		this.animationIndex = animationIndex;
		this.frame = frame;
		repaint();
	}
	
	@Override
	public void paint(Graphics g) {
		super.paint(g);
		if(frames != null && animationIndex>=0 && frame>=0) {
			Image img = frames[animationIndex][frame].getScaledInstance(Math.round(w*zoom), Math.round(h*zoom), Image.SCALE_FAST);
			g.drawImage(img, Math.round(getWidth()/2f-(w*zoom)/2f), Math.round(getHeight()/2f-(h*zoom)/2f), null);
		}
	}

	public float getZoom() {
		return zoom;
	}

	public void setZoom(float zoom) {
		this.zoom = zoom;
	}	

}
