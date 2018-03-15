package org.sgdk.resourcemanager.ui.panels.preview.toolbar;

import java.awt.Color;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.ui.panels.preview.PreviewPanel;

public class ImageToolbar extends JPanel{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private JButton zoomUpButton;
	private JButton zoomDownButton;
	private JButton backgroundColorButton;
	
	public ImageToolbar (PreviewPanel previewPanel) {
		super(new GridBagLayout());
		
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
				Color color = JColorChooser.showDialog(
						 null,
						 "Choose a Color",
						 backgroundColorButton.getBackground()
				);
				if(color != null) {					
					backgroundColorButton.setBackground(color);
					previewPanel.setBackgroundColor(color);
				}
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
	}

	public JButton getBackgroundColorButton() {
		return backgroundColorButton;
	}

}
