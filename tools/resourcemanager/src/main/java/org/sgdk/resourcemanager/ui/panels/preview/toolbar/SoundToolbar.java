package org.sgdk.resourcemanager.ui.panels.preview.toolbar;

import java.awt.ComponentOrientation;
import java.awt.FlowLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JButton;
import javax.swing.JPanel;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.ui.panels.preview.SoundPlayer;

public class SoundToolbar extends JPanel{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private static final Logger logger = LogManager.getLogger("UILogger");
	
	private JButton stopButton;
	private JButton playpauseButton;
	
	public SoundToolbar(SoundPlayer soundPlayer) {
		super(new FlowLayout ());
		setComponentOrientation(ComponentOrientation.LEFT_TO_RIGHT);
		
		stopButton = new JButton("Stop");	
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
	
	@Override
	public void setVisible(boolean aFlag) {
		super.setVisible(aFlag);		
		playpauseButton.setText("Pause");
	}

}
