package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.BorderFactory;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.ui.panels.preview.PreviewContainerPanel;

public class SpriteDimensionComponent extends JPanel{
		
	private static final Logger logger = LogManager.getLogger("UILogger");
	
	private static final long serialVersionUID = 1L;
	private SGDKSprite sprite = null;
	private JTextField w = new JTextField();
	private JTextField h = new JTextField();
	
	public SpriteDimensionComponent(PreviewContainerPanel previewContainerPanel) {
		super(new GridLayout(1,4));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Sprite Dimensions",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		w.addKeyListener(new KeyListener() {

			@Override
			public void keyTyped(KeyEvent e) {}

			@Override
			public void keyPressed(KeyEvent e) {}

			@Override
			public void keyReleased(KeyEvent e) {
				if(sprite != null && !StringUtils.isEmpty(w.getText())){
					if(Float.valueOf(w.getText()).intValue() % SGDKSprite.SCALE_MULTIPLICATOR == 0) {						
						sprite.setWidth(Math.round(Float.valueOf(w.getText()) / SGDKSprite.SCALE_MULTIPLICATOR));
						previewContainerPanel.repaint();
						logger.info("Width was changed to "+w.getText());
					}else {
						logger.warn("The input("+w.getText()+") must to be multiple of 8");
					}
				}
			}
		});
		
		h.addKeyListener(new KeyListener() {

			@Override
			public void keyTyped(KeyEvent e) {}

			@Override
			public void keyPressed(KeyEvent e) {}

			@Override
			public void keyReleased(KeyEvent e) {
				if(sprite != null && !StringUtils.isEmpty(h.getText())){
					if(Float.valueOf(h.getText()).intValue() % SGDKSprite.SCALE_MULTIPLICATOR == 0) {	
						sprite.setHeight(Math.round(Float.valueOf(h.getText()) / SGDKSprite.SCALE_MULTIPLICATOR));
						previewContainerPanel.repaint();
						logger.info("Height was changed to "+h.getText());
					}else {
						logger.warn("The input("+h.getText()+") must to be multiple of 8");
					}
				}
			}
		});
		add(new JLabel("w:"));
		add(w);
		add(new JLabel("h:"));
		add(h);
	}
	
	public void setSGDKSprite(SGDKSprite sprite) {
		this.sprite = sprite;
		w.setText(""+ (sprite.getWidth() * SGDKSprite.SCALE_MULTIPLICATOR));
		h.setText(""+ (sprite.getHeight() * SGDKSprite.SCALE_MULTIPLICATOR));
	}
	
}
