package org.sgdk.resourcemanager.ui.panels.preview;

import java.awt.Color;
import java.awt.Component;
import java.awt.ComponentOrientation;
import java.awt.FlowLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JPanel;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class PreviewToolbar extends JPanel {

	private static final Logger logger = LogManager.getLogger("UILogger");
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private JButton zoomUpButton;
	private JButton zoomDownButton;
	private JButton backgroundColorButton;

	private Component stopButton;

	private JButton playpauseButton;
	
	public PreviewToolbar(PreviewPanel previewPanel, SoundPlayer soundPlayer) {
		super(new FlowLayout ());
		setComponentOrientation(ComponentOrientation.LEFT_TO_RIGHT);
		
		zoomUpButton = new JButton("+");
		zoomUpButton.setVisible(false);
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
		
		zoomDownButton = new JButton("-");		
		zoomDownButton.setVisible(false);
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
		
		backgroundColorButton = new JButton("");	
		backgroundColorButton.setVisible(false);
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
		add(backgroundColorButton);
		
		stopButton = new JButton("Stop");	
		stopButton.setVisible(false);
		stopButton.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent e) {
				try {
					soundPlayer.stopSound();
				} catch (Exception e1) {
					logger.error(e1.getMessage());
				}
				playpauseButton.setText("Play");
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
		add(stopButton);
		
		playpauseButton = new JButton("Pause");	
		playpauseButton.setVisible(false);
		playpauseButton.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent e) {
				try {
					if(soundPlayer.isRunning()) {
						if(soundPlayer.isPaused()) {
							soundPlayer.isPaused(false);							
							playpauseButton.setText("Pause");
						}else {
							soundPlayer.isPaused(true);							
							playpauseButton.setText("Play");
						}
					}else {
						soundPlayer.playSound();
						playpauseButton.setText("Pause");
					}
				} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
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
		add(playpauseButton);
	}

	public void clean() {
		zoomUpButton.setVisible(false);
		zoomDownButton.setVisible(false);
		backgroundColorButton.setVisible(false);
		stopButton.setVisible(false);
		playpauseButton.setVisible(false);
		playpauseButton.setText("Pause");
	}

	public void showImageButtons() {
		zoomUpButton.setVisible(true);
		zoomDownButton.setVisible(true);
		backgroundColorButton.setVisible(true);
	}

	public void showSoundButtons() {
		stopButton.setVisible(true);
		playpauseButton.setVisible(true);
	}

}
