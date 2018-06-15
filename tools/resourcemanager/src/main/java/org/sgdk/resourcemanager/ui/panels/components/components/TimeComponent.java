package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.apache.commons.lang3.StringUtils;
import org.sgdk.resourcemanager.entities.SGDKSprite;

public class TimeComponent extends JPanel{
		
	private static final long serialVersionUID = 1L;
	private SGDKSprite sprite = null;
	private JTextField time = new JTextField();
	
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
		
		time.addKeyListener(new KeyListener() {

			@Override
			public void keyTyped(KeyEvent e) {}

			@Override
			public void keyPressed(KeyEvent e) {}

			@Override
			public void keyReleased(KeyEvent e) {
				if(sprite != null && !StringUtils.isEmpty(time.getText())){
					sprite.setTime(Math.round(Float.valueOf(time.getText()) * SGDKSprite.TIME_MULTIPLICATOR));
				}
			}
		});
		add(time);
	}
	
	public void setSGDKSprite(SGDKSprite sprite) {
		this.sprite = sprite;
		time.setText(""+ (sprite.getTime() / SGDKSprite.TIME_MULTIPLICATOR));
	}
	
}
