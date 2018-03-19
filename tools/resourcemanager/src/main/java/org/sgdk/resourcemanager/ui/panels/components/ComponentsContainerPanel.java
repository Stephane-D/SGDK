package org.sgdk.resourcemanager.ui.panels.components;

import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKElement;

public class ComponentsContainerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private ImageComponentsPanel imageComponentsPanel;
	private SoundComponentsPanel soundComponentsPanel;
	
	public ComponentsContainerPanel() throws IOException {
		setBorder(BorderFactory.createTitledBorder("Components"));
		
		imageComponentsPanel = new ImageComponentsPanel();
		soundComponentsPanel = new SoundComponentsPanel();
		
		clean();
		
		add(imageComponentsPanel);
		add(soundComponentsPanel);		
	}
	
	public void clean() {
		imageComponentsPanel.setVisible(false);
		soundComponentsPanel.setVisible(false);
	}
	
	public void showImageComponents(SGDKElement e) {
		imageComponentsPanel.setComponents(e);
		imageComponentsPanel.setVisible(true);
	}

	public void showSoundComponents(SGDKElement e) {
		soundComponentsPanel.setComponents(e);
		soundComponentsPanel.setVisible(true);
	}

	public void setSGDKElement(SGDKElement element) {
		clean();
		switch(element.getType()) {
		case SGDKBackground:
		case SGDKSprite:
			showImageComponents(element);
			break;
		case SGDKEnvironmentSound:
		case SGDKFXSound:
			showSoundComponents(element);
			break;
		default:
			break;
		
		}
	}

}
