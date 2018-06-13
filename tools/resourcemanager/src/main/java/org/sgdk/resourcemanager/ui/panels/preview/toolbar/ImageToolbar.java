package org.sgdk.resourcemanager.ui.panels.preview.toolbar;

import java.awt.Color;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JPanel;
import javax.swing.colorchooser.AbstractColorChooserPanel;

import org.sgdk.resourcemanager.ui.panels.preview.PreviewPanel;

public class ImageToolbar extends JPanel{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private JButton zoomUpButton;
	private JButton zoomDownButton;
	private JButton backgroundColorButton;
	private JButton gridColorButton;
	
	PreviewPanel previewPanel;
	
	public ImageToolbar (PreviewPanel previewPanel) {
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
		zoomUpButton.addMouseListener(new MouseListener() {			
			@Override
			public void mouseReleased(MouseEvent e) {
				previewPanel.setZoom(previewPanel.getZoom() + 0.1f);
			}

			@Override
			public void mouseClicked(MouseEvent e) {
				
			}

			@Override
			public void mouseEntered(MouseEvent e) {
				
			}

			@Override
			public void mouseExited(MouseEvent e) {
				
			}

			@Override
			public void mousePressed(MouseEvent e) {
				
			}
		});

		c.gridx = 0;
		add(zoomUpButton, c);
		
		zoomDownButton = new JButton("-");	
		zoomDownButton.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent e) {
				previewPanel.setZoom(previewPanel.getZoom() - 0.1f);	
			}
			
			@Override
			public void mousePressed(MouseEvent e) {
				
			}
			
			@Override
			public void mouseExited(MouseEvent e) {
				
			}
			
			@Override
			public void mouseEntered(MouseEvent e) {
				
			}
			
			@Override
			public void mouseClicked(MouseEvent e) {
				
			}
		});
		c.gridx = 1;
		add(zoomDownButton, c);
		
		backgroundColorButton = new JButton("");
		backgroundColorButton.setBackground(previewPanel.getBackgroundColor());
		backgroundColorButton.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent e) {
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
			
			@Override
			public void mousePressed(MouseEvent e) {
				
			}
			
			@Override
			public void mouseExited(MouseEvent e) {
				
			}
			
			@Override
			public void mouseEntered(MouseEvent e) {
				
			}
			
			@Override
			public void mouseClicked(MouseEvent e) {
				
			}
		});
		c.gridx = 2;
		add(backgroundColorButton, c);
		
		gridColorButton = new JButton("");
		gridColorButton.setBackground(new Color(0,0,0));
		gridColorButton.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent e) {
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
			
			@Override
			public void mousePressed(MouseEvent e) {
				
			}
			
			@Override
			public void mouseExited(MouseEvent e) {
				
			}
			
			@Override
			public void mouseEntered(MouseEvent e) {
				
			}
			
			@Override
			public void mouseClicked(MouseEvent e) {
				
			}
		});
		c.gridx = 3;
		add(gridColorButton, c);
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

}
