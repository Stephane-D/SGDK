package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

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
		
		time.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
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
