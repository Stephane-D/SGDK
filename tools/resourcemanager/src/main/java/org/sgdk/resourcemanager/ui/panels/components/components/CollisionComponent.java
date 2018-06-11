package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JComboBox;
import javax.swing.JPanel;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKSprite;

public class CollisionComponent extends JPanel{
	
	/**
	 * CIRCLE, BOX or NONE (NONE by default)
	 */	
	
	private static final long serialVersionUID = 1L;
	private SGDKSprite sprite = null;
	private JComboBox<SGDKSprite.Collision> options = new JComboBox<SGDKSprite.Collision>(SGDKSprite.Collision.values());

	public  CollisionComponent() {
		super(new GridLayout(1,1));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Collision",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		options.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(sprite != null){
					sprite.setCollision((SGDKSprite.Collision)options.getSelectedItem());
				}
			}
		});
		add(options);
	}
	
	public void setSGDKSprite(SGDKSprite sprite) {
		this.sprite = sprite;
		options.setSelectedIndex(sprite.getCollision().ordinal());
	}
	
}
