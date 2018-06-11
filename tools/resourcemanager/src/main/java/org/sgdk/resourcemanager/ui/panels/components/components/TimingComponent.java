package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JComboBox;
import javax.swing.JPanel;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;

public class TimingComponent extends JPanel{
	
	/**
	 *  -1 (default) = AUTO (NTSC or PAL depending the information in source VGM/XGM file)
        0            = NTSC (XGM is generated for NTSC system)
        1            = PAL (XGM is generated for PAL system)
	 */	
	
	private static final long serialVersionUID = 1L;
	private SGDKEnvironmentSound environmentSound = null;
	private JComboBox<SGDKEnvironmentSound.Timing> options = new JComboBox<SGDKEnvironmentSound.Timing>(SGDKEnvironmentSound.Timing.values());

	public  TimingComponent() {
		super(new GridLayout(1,1));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Timing",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		options.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(environmentSound != null){
					environmentSound.setTiming((SGDKEnvironmentSound.Timing)options.getSelectedItem());
				}
			}
		});
		add(options);
	}
	
	public void setSGDKEnvironmentSound(SGDKEnvironmentSound environmentSound) {
		this.environmentSound = environmentSound;
		options.setSelectedIndex(environmentSound.getTiming().ordinal());
	}
	
}
