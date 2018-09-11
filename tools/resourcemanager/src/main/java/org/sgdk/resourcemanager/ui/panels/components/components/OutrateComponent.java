package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKFXSound;

public class OutrateComponent extends JPanel{
		
	private static final long serialVersionUID = 1L;
	private SGDKFXSound fxSound = null;
	private JTextField outrate = new JTextField();
	
	public OutrateComponent() {
		super(new GridLayout(1,1));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Outrate",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		outrate.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(fxSound != null){
					fxSound.setOutrate(Math.round(Float.valueOf(outrate.getText())));
				}
			}
		});
		add(outrate);
	}
	
	public void setSGDKFXSound(SGDKFXSound fxSound) {
		this.fxSound = fxSound;
		outrate.setText(""+ fxSound.getOutrate());
	}
	
}
