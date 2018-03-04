package org.sgdk.resourcemanager.ui.panels.preview;

import java.awt.ComponentOrientation;
import java.awt.FlowLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JPanel;

public class PreviewToolbar extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public PreviewToolbar(PreviewPanel previewPanel) {
		super(new FlowLayout ());
		setComponentOrientation(ComponentOrientation.LEFT_TO_RIGHT);
		
		JButton zoomUpButton = new JButton("+");
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
		add(zoomUpButton);
		
		JButton zoomDownButton = new JButton("-");		
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
		add(zoomDownButton);
		
		JButton backgroundColorButton = new JButton("");	
		backgroundColorButton.setBackground(previewPanel.getBackgroundColor());
		backgroundColorButton.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent e) {
				backgroundColorButton.setBackground(
						 JColorChooser.showDialog(
								 null,
								 "Choose a Color",
								 backgroundColorButton.getBackground()
						 )
				 );
				previewPanel.setBackgroundColor(backgroundColorButton.getBackground());
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
		add(backgroundColorButton);
	}

}
