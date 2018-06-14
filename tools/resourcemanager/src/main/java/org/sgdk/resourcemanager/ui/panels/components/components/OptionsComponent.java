package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;

public class OptionsComponent extends JPanel{
		
	private static final long serialVersionUID = 1L;
	private SGDKEnvironmentSound environmentSound = null;
	private JTextField time = new JTextField();
	
	public OptionsComponent() {
		super(new GridLayout(1,1));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Options",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		time.addKeyListener(new KeyListener() {

			@Override
			public void keyTyped(KeyEvent e) {}

			@Override
			public void keyPressed(KeyEvent e) {}

			@Override
			public void keyReleased(KeyEvent e) {
				if(environmentSound != null){
					environmentSound.setOptions(time.getText());
				}
			}
		});
		add(time);
	}
	
	public void setSGDKEnvironmentSound(SGDKEnvironmentSound environmentSound) {
		this.environmentSound = environmentSound;
		time.setText(environmentSound.getOptions());
	}
	
}
