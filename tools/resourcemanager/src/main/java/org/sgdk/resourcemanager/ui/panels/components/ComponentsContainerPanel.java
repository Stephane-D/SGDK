package org.sgdk.resourcemanager.ui.panels.components;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;
import org.sgdk.resourcemanager.entities.SGDKFXSound;
import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.ui.panels.preview.PreviewContainerPanel;

public class ComponentsContainerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private BackgroundComponentsPanel backgroundComponentsPanel;
	private FXSoundComponentsPanel fxComponentsPanel;
	private SpriteComponentsPanel spriteComponentsPanel;
	private EnvironmentSoundComponentsPanel environmentSoundComponentsPanel;
	
	public ComponentsContainerPanel(PreviewContainerPanel previewContainerPanel) throws IOException {
		super(new GridBagLayout());
		setBorder(BorderFactory.createTitledBorder("Components"));
		
		backgroundComponentsPanel = new BackgroundComponentsPanel();
		spriteComponentsPanel = new SpriteComponentsPanel(previewContainerPanel);
		fxComponentsPanel = new FXSoundComponentsPanel();
		environmentSoundComponentsPanel = new EnvironmentSoundComponentsPanel();
		
		clean();
		
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.HORIZONTAL;		
		c.anchor = GridBagConstraints.PAGE_START;
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = 1;
		c.weightx = 1.0;
		c.weighty = 1.0;
		
		add(backgroundComponentsPanel, c);
		add(fxComponentsPanel, c);		
		add(spriteComponentsPanel, c);
		add(environmentSoundComponentsPanel, c);	
	}
	
	public void clean() {
		backgroundComponentsPanel.setVisible(false);
		fxComponentsPanel.setVisible(false);
		spriteComponentsPanel.setVisible(false);
		environmentSoundComponentsPanel.setVisible(false);
	}
	
	public void showBackgroundComponents(SGDKBackground e) {
		backgroundComponentsPanel.setComponents(e);
		backgroundComponentsPanel.setVisible(true);
	}
	
	public void showSpriteComponents(SGDKSprite e) {
		spriteComponentsPanel.setComponents(e);
		spriteComponentsPanel.setVisible(true);
	}

	public void showFXSoundComponents(SGDKFXSound e) {
		fxComponentsPanel.setComponents(e);
		fxComponentsPanel.setVisible(true);
	}
	
	public void showEnvironmentSoundComponents(SGDKEnvironmentSound e) {
		environmentSoundComponentsPanel.setComponents(e);
		environmentSoundComponentsPanel.setVisible(true);
	}

	public void setSGDKElement(SGDKElement element) {
		clean();
		switch(element.getType()) {
		case SGDKBackground:
			showBackgroundComponents((SGDKBackground)element);
			break;
		case SGDKSprite:
			showSpriteComponents((SGDKSprite)element);
			break;
		case SGDKEnvironmentSound:
			showEnvironmentSoundComponents((SGDKEnvironmentSound)element);
			break;
		case SGDKFXSound:
			showFXSoundComponents((SGDKFXSound)element);
			break;
		default:
			break;
		
		}
	}

}
