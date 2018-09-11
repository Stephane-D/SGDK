package org.sgdk.resourcemanager.ui.panels.preview;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;
import javax.swing.BorderFactory;
import javax.swing.ImageIcon;
import javax.swing.JPanel;
import javax.swing.JScrollPane;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.panels.preview.toolbar.PreviewContainerToolbar;

public class PreviewContainerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private SoundPlayer soundPlayer;
	private PreviewPanel previewPanel;
	private PreviewContainerToolbar toolBar;
	
	private static final Logger logger = LogManager.getLogger("UILogger");
	
	public PreviewContainerPanel(ResourceManagerFrame parent) throws IOException {
		super(new GridBagLayout());		
		setBorder(BorderFactory.createTitledBorder("Preview"));
		
		soundPlayer = new SoundPlayer();
		previewPanel = new PreviewPanel();
		toolBar = new PreviewContainerToolbar(previewPanel, soundPlayer);
		
		GridBagConstraints c = new GridBagConstraints();
		
		JScrollPane scrollPanePreview = new JScrollPane(previewPanel);
		
		c.fill = GridBagConstraints.BOTH;		
		c.anchor = GridBagConstraints.CENTER;

		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = 1;
		c.weightx = 1.0;
		c.weighty = 1.0/12.0;
		add(toolBar, c);
		
		c.gridx = 0;
		c.gridy = 1;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.REMAINDER;
		c.weightx = 1.0;
		c.weighty = 11.0/12.0;
		add(scrollPanePreview, c);
	}
	
	public void setPreview(SGDKElement sgdkElement) {
		cleanPanel();
		stopSound();
		toolBar.clean();
		try {
			switch(sgdkElement.getType()) {
			case SGDKBackground:
			case SGDKSprite:
				paintImage(sgdkElement);
				toolBar.showImageButtons();
				break;
			case SGDKEnvironmentSound:
			case SGDKFXSound:
				paintImage(sgdkElement);
				playSound(sgdkElement);
				toolBar.showSoundButtons();
				break;
			case SGDKFolder:
			case SGDKProject:
				break;
			default:
				break;				
			}
		} catch (IOException e) {
			logger.error("", e);	
		}
	}

	private void stopSound() {
		try {
			soundPlayer.stopSound();
		}catch (Exception e) {
			logger.error("", e);
		}
	}

	private void playSound(SGDKElement sgdkElement) {			
		try {
			soundPlayer.playSound(sgdkElement);
        } catch (Exception e) {
        	logger.error("", e);
        }	     
	}

	private void paintImage(SGDKElement sgdkElement) throws IOException {
		switch (sgdkElement.getType()) {
		case SGDKBackground:
		case SGDKSprite:
			previewPanel.paintImage(ImageIO.read(new File(sgdkElement.getPath())));
			break;
		case SGDKEnvironmentSound:
		case SGDKFXSound:
		case SGDKFolder:
		case SGDKProject:
			previewPanel.paintImage(((ImageIcon)sgdkElement.getIcon()).getImage());
			previewPanel.setZoom(previewPanel.getZoom() * 5);
			break;
		default:
			break;		
		}
	}

	public void cleanPanel() {
		previewPanel.clean();	
		toolBar.getImageToolbar().getBackgroundColorButton().setBackground(previewPanel.getBackgroundColor());
	}

}
