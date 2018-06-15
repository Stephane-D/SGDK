package org.sgdk.resourcemanager.ui.panels.preview.toolbar;

import java.awt.Color;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JPanel;
import javax.swing.colorchooser.AbstractColorChooserPanel;

import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.panels.preview.PreviewPanel;
import org.sgdk.resourcemanager.ui.panels.preview.toolbar.spriteplayer.SpritePlayerDialog;
import org.sgdk.resourcemanager.ui.utils.svg.SVGUtils;

public class ImageToolbar extends JPanel{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private JButton zoomUpButton;
	private JButton zoomDownButton;
	private JButton backgroundColorButton;
	private JButton gridColorButton;
	private JButton spritePlayerButton;
	
	private PreviewPanel previewPanel;
	private SpritePlayerDialog spritePlayerDialog = null;

	
	public ImageToolbar (ResourceManagerFrame parent, PreviewPanel previewPanel) {
		super(new GridBagLayout());
		this.previewPanel = previewPanel;
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;		
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 1.0;
		c.weighty = 1.0;
		c.gridwidth = 1;
		c.gridheight = 1;
		c.gridy = 0;
		
		zoomUpButton = new JButton("+");
		zoomUpButton.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				previewPanel.setZoom(previewPanel.getZoom() + 0.1f);
			}
		});

		c.gridx = 0;
		add(zoomUpButton, c);
		
		zoomDownButton = new JButton("-");	
		zoomDownButton.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				previewPanel.setZoom(previewPanel.getZoom() - 0.1f);	
			}
		});
		c.gridx = 1;
		add(zoomDownButton, c);
		
		backgroundColorButton = new JButton("");
		backgroundColorButton.setBackground(previewPanel.getBackgroundColor());
		backgroundColorButton.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				JColorChooser cc = new JColorChooser();
				cc.setColor(previewPanel.getBackgroundColor());
				AbstractColorChooserPanel defaultPanels[] = cc.getChooserPanels();
				cc.removeChooserPanel(defaultPanels[4]); // CMYK
				cc.removeChooserPanel(defaultPanels[2]); // HSL
				cc.removeChooserPanel(defaultPanels[1]); // other tab
				cc.removeChooserPanel(defaultPanels[0]); // palettes
				cc.setPreviewPanel(new JPanel());
				JColorChooser.createDialog(null, "Choose a background Color", true, cc, new ActionListener() {

					@Override
					public void actionPerformed(ActionEvent e) {
						Color newColor = cc.getColor();
						backgroundColorButton.setBackground(newColor);
						previewPanel.setBackgroundColor(newColor);
					}
				}, null).setVisible(true);
			}
		});
		c.gridx = 2;
		add(backgroundColorButton, c);
		
		gridColorButton = new JButton("");
		gridColorButton.setBackground(new Color(0,0,0));
		gridColorButton.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				JColorChooser cc = new JColorChooser();
				cc.setColor(gridColorButton.getBackground());
				AbstractColorChooserPanel defaultPanels[] = cc.getChooserPanels();
				cc.removeChooserPanel(defaultPanels[4]); // CMYK
				cc.removeChooserPanel(defaultPanels[2]); // HSL
				cc.removeChooserPanel(defaultPanels[1]); // other tab
				cc.removeChooserPanel(defaultPanels[0]); // palettes
				cc.setPreviewPanel(new JPanel());
				JColorChooser.createDialog(null, "Choose a grid Color", true, cc, new ActionListener() {

					@Override
					public void actionPerformed(ActionEvent e) {
						Color newColor = cc.getColor();
						gridColorButton.setBackground(newColor);
						previewPanel.setGridColor(newColor);
					}
				}, null).setVisible(true);
			}
		});
		c.gridx = 3;
		add(gridColorButton, c);
		try {
			spritePlayerButton = new JButton(SVGUtils.load(
					getClass().getResource("/icons/043-play-button.svg").toURI(),
					16,
					16));
			spritePlayerButton.addActionListener(new ActionListener() {				
				@Override
				public void actionPerformed(ActionEvent e) {
					if(previewPanel.getElement()!= null && previewPanel.getElement() instanceof SGDKSprite) {						
						if(spritePlayerDialog == null) {							
							spritePlayerDialog = new SpritePlayerDialog(parent);
						}
						spritePlayerDialog.setSprite((SGDKSprite)previewPanel.getElement());
						spritePlayerDialog.setVisible(true);
					}
				}
			});
		} catch (Exception e1) {
			e1.printStackTrace();
		}
		c.gridx = 4;
		add(spritePlayerButton, c);
	}

	public JButton getBackgroundColorButton() {
		return backgroundColorButton;
	}

	public void showGridButtton(boolean enableGrid) {
		gridColorButton.setVisible(enableGrid);
		if(!enableGrid) {
			previewPanel.setGridColor(null);
		}else {
			previewPanel.setGridColor(gridColorButton.getBackground());
		}
	}
	
	public void showSpritePlayerButton(boolean enablePlayerButton) {
		spritePlayerButton.setVisible(enablePlayerButton);
	}

}
