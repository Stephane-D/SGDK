package org.sgdk.resourcemanager.ui.panels.preview.toolbar;

import javax.swing.JPanel;

import org.sgdk.resourcemanager.ui.panels.preview.PreviewPanel;
import org.sgdk.resourcemanager.ui.panels.preview.SoundPlayer;

public class PreviewContainerToolbar extends JPanel {

	
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;	
	
	private ImageToolbar imageToolbar;
	private SoundToolbar soundToolbar;	
	
	public PreviewContainerToolbar(PreviewPanel previewPanel, SoundPlayer soundPlayer) {
		super();
		
		imageToolbar = new ImageToolbar(previewPanel);
		soundToolbar = new SoundToolbar(soundPlayer);			
		clean();		
		add(imageToolbar);
		add(soundToolbar);
	}

	public void clean() {
		imageToolbar.setVisible(false);
		soundToolbar.setVisible(false);
	}

	public void showImageButtons() {
		imageToolbar.setVisible(true);
	}

	public void showSoundButtons() {
		soundToolbar.setVisible(true);
	}

	public ImageToolbar getImageToolbar() {
		return imageToolbar;
	}	

}
