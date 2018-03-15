package org.sgdk.resourcemanager.ui.panels.properties;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKElement;

public class PropertiesContainerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private ImagePropertiesPanel imagePropertiesPanel;
	private SoundPropertiesPanel soundPropertiesPanel;
	
	public PropertiesContainerPanel() throws IOException {
		super(new GridBagLayout());
		setBorder(BorderFactory.createTitledBorder("Properties"));
		
		imagePropertiesPanel = new ImagePropertiesPanel();
		soundPropertiesPanel = new SoundPropertiesPanel();
		
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
		
		add(imagePropertiesPanel, c);
		add(soundPropertiesPanel, c);		
	}
	
	public void clean() {
		imagePropertiesPanel.setVisible(false);
		soundPropertiesPanel.setVisible(false);
	}
	
	public void showImageProperties(SGDKElement e) {
		imagePropertiesPanel.setProperties(e);
		imagePropertiesPanel.setVisible(true);
	}

	public void showSoundProperties(SGDKElement e) {
		soundPropertiesPanel.setProperties(e);
		soundPropertiesPanel.setVisible(true);
	}

	public void setSGDKElement(SGDKElement element) {
		clean();
		switch(element.getType()) {
		case SGDKBackground:
		case SGDKSprite:
			showImageProperties(element);
			break;
		case SGDKEnvironmentSound:
		case SGDKFXSound:
			showSoundProperties(element);
			break;
		default:
			break;
		
		}
	}

}
