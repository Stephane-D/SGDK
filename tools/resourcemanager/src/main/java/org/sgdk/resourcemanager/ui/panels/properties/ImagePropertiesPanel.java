package org.sgdk.resourcemanager.ui.panels.properties;

import java.awt.Color;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.imageio.ImageIO;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKElement;

public class ImagePropertiesPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private static final int LABEL_INDEX_COLOR_0 = 3;
	
	private JPanel paletteProperties;
	
	private List<JLabel> labels = new ArrayList<>();
	
	public ImagePropertiesPanel(){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));		
		
		JPanel imageProperties = new JPanel();
		imageProperties.setLayout(new GridBagLayout());

		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.HORIZONTAL;	
		c.anchor = GridBagConstraints.CENTER;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = 1;
		c.weightx = 1.0;
		c.weighty = 0.0;
		c.gridy = 0;
		c.gridx = 0;
		
		imageProperties.setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Image Properties",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		c.gridy = 0;
		labels.add(new JLabel(""));
		imageProperties.add(labels.get(0), c);
		
		c.gridy = 1;
		labels.add(new JLabel(""));
		imageProperties.add(labels.get(1), c);
		
		c.gridy = 2;
		c.gridheight = GridBagConstraints.REMAINDER;
		labels.add(new JLabel(""));
		imageProperties.add(labels.get(2), c);

		add(imageProperties);

		paletteProperties = new JPanel();
		paletteProperties.setLayout(new GridLayout(4, 4));
		paletteProperties.setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Palette",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		for(int i = 0; i < 16; i++) {
			JLabel label = new JLabel(" ");
			label.setHorizontalTextPosition(SwingConstants.LEFT);
			label.setVerticalTextPosition(SwingConstants.BOTTOM);
			label.setFont(new Font(label.getFont().getName(), label.getFont().getStyle(), 10));
			labels.add(label);
			labels.get(LABEL_INDEX_COLOR_0+i).setOpaque(true);
			paletteProperties.add(labels.get(LABEL_INDEX_COLOR_0+i));
		}
		
		add(paletteProperties);
	}

	public void setProperties(SGDKElement e) {
		clean();	
		try {
			BufferedImage image = ImageIO.read(new File(e.getPath()));
			labels.get(0).setText("width: " + image.getWidth(null) + "");
			labels.get(1).setText("height: " + image.getHeight(null) + "");		
			
			IndexColorModel indexColorModel = (IndexColorModel) image.getColorModel();
			labels.get(2).setText("Type: Indexed");
			for (int i = 0; i < indexColorModel.getMapSize(); i++) {
				labels.get(LABEL_INDEX_COLOR_0 + i).setBackground(new Color(indexColorModel.getRGB(i)));
				labels.get(LABEL_INDEX_COLOR_0 + i).setText(i + "");
			}
			
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
	}

	private void clean() {
		for(JLabel label : labels) {
			label.setText("");
		}		
	}
}
