package org.sgdk.resourcemanager.ui.panels.preview.toolbar.spriteplayer;

import org.sgdk.resourcemanager.entities.SGDKSprite;

public class SpritePlayerThread extends Thread {
	
	private boolean run = true;
	private SpritePlayerPanel spritePlayer = null;
	private int numFrames = 0;
	private int animationIndex = 0;
	
	public SpritePlayerThread(SpritePlayerPanel spritePlayer, int animationIndex, int numFrames) {
		this.spritePlayer = spritePlayer;
		this.numFrames = numFrames;
		run = spritePlayer.getSprite().getTime() > 0 && numFrames > 1;	
		this.animationIndex = animationIndex;
	}

	@Override
	public void run() {
		try {
			Thread.sleep(1000);
			int frame = 0;
			while (run) {
				spritePlayer.paintFrame(animationIndex, frame);
				Thread.sleep(Math.round(1000 * spritePlayer.getSprite().getTime() / SGDKSprite.TIME_MULTIPLICATOR));
				frame = (frame + 1) % numFrames;
			}
		} catch (InterruptedException e) {
			run = false;
		}
	}
	
	public void stopSafely() {
		run = false;
	}

}
