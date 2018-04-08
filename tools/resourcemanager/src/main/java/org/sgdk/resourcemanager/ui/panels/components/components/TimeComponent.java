package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKSprite;

public class TimeComponent extends JPanel{
		
	private static final long serialVersionUID = 1L;
	private SGDKSprite sprite = null;
	private JTextField time = new JTextField();

	private static final int TIME_MULTIPLICATOR = 60;
	
	public TimeComponent() {
		super(new GridLayout(1,1));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Time",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		time.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(sprite != null){
					sprite.setTime(Math.round(Float.valueOf(time.getText()) * TIME_MULTIPLICATOR));
				}
			}
		});
		add(time);
	}
	
	public void setSGDKSprite(SGDKSprite sprite) {
		this.sprite = sprite;
		time.setText(""+ (sprite.getTime() / TIME_MULTIPLICATOR));
	}
	
}
