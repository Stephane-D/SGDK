package org.sgdk.resourcemanager.ui.panels.preview.toolbar.spriteplayer;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.apache.commons.lang3.StringUtils;
import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class SpritePlayerDialog extends JDialog {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private static final int minimizeWidth = 340;
	private static final int minimizeHeight = 220;

	private SpritePlayerPanel spritePlayer;
	private JTextField animationIndexText = new JTextField();
	private ResourceManagerFrame parent = null;

	public SpritePlayerDialog(ResourceManagerFrame parent) {
		super(parent, "Sprite Player");
		JPanel panel = new JPanel();
		this.parent = parent;
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		int screenWidth = new Long(Math.round(screenSize.getWidth())).intValue();
		int screenHeight = new Long(Math.round(screenSize.getHeight())).intValue();

		setBounds(new Rectangle(screenWidth / 2 - minimizeWidth / 2, screenHeight / 2 - minimizeHeight / 2,
				minimizeWidth, minimizeHeight));
		setResizable(false);

		panel.setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints(0,0,GridBagConstraints.RELATIVE,GridBagConstraints.REMAINDER,0.85d,1d,GridBagConstraints.CENTER,GridBagConstraints.BOTH, new Insets(0, 0, 0, 0),0,0);
		
		addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent evt) {
				clean();
				parent.setEnabled(true);
			}
		});
		spritePlayer = new SpritePlayerPanel();
		panel.add(spritePlayer, c);
		animationIndexText.setText("0");
		animationIndexText.addKeyListener(new KeyListener() {
			
			@Override
			public void keyTyped(KeyEvent e) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void keyReleased(KeyEvent e) {
				if(!StringUtils.isEmpty(animationIndexText.getText())) {
					try {						
						int i = Integer.valueOf(animationIndexText.getText());
						float zoom = spritePlayer.getZoom();
						spritePlayer.stopAnimation();
						spritePlayer.setZoom(zoom);
						spritePlayer.runAnimation(spritePlayer.getSprite(), i);
					}catch (Exception ex) {
					}
				}
			}
			
			@Override
			public void keyPressed(KeyEvent e) {
				// TODO Auto-generated method stub
				
			}
		});
		c.gridx = 5;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.RELATIVE;
		c.weightx = 0.15d;
		c.anchor = GridBagConstraints.SOUTH;
		c.fill = GridBagConstraints.NONE;
		panel.add(new JLabel("Animation Index"),c);
		
		c.gridy = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		c.anchor = GridBagConstraints.NORTH;
		panel.add(animationIndexText,c);
		add(panel);
	}

	private void clean() {
		spritePlayer.stopAnimation();
	}

	public void setSprite(SGDKSprite sprite) {
		animationIndexText.setText("0");
		spritePlayer.runAnimation(sprite, 0);
	}
	
	@Override
	public void setVisible(boolean b) {
		super.setVisible(b);
		parent.setEnabled(!b);
	}

	
}
