package org.sgdk.resourcemanager.ui.panels.properties;

import java.awt.Color;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.util.ArrayList;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JColorChooser;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;
import javax.swing.colorchooser.AbstractColorChooserPanel;

import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.utils.indexedimage.ImageUtil;

import ij.ImagePlus;

public class ImagePropertiesPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private static final int LABEL_INDEX_COLOR_0 = 3;
	
	private JPanel paletteProperties;
	
	private SGDKElement element = null;
	
	private List<JLabel> labels = new ArrayList<>();
	
	public ImagePropertiesPanel(ResourceManagerFrame resourceManagerFrame){
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
		for(int i = 0; i < SGDKBackground.PALETTE_SIZE; i++) {
			JLabel label = new JLabel(" ");
			label.setHorizontalTextPosition(SwingConstants.LEFT);
			label.setVerticalTextPosition(SwingConstants.BOTTOM);
			label.setFont(new Font(label.getFont().getName(), label.getFont().getStyle(), 10));
			label.setOpaque(true);
			label.addMouseListener(new MouseListener() {
				
				@Override
				public void mouseReleased(MouseEvent e) {
					JColorChooser cc = new JColorChooser();
					Color oldColor = label.getBackground();
					cc.setColor(label.getBackground());
					AbstractColorChooserPanel defaultPanels[] = cc.getChooserPanels();
					cc.removeChooserPanel( defaultPanels[4] ); // CMYK
					cc.removeChooserPanel( defaultPanels[2] );  // HSL
					cc.removeChooserPanel( defaultPanels[1] ); // other tab
					cc.removeChooserPanel( defaultPanels[0] ); //palettes
					cc.setPreviewPanel(new JPanel());
					JDialog dialog = JColorChooser.createDialog(
						    null,
						    "Choose a Color",
						    true,
						    cc,
						    null,
						    null);
					dialog.setVisible(true);
					Color newColor =  cc.getColor();
					if (!newColor.equals(oldColor) && element != null) {
						ImagePlus ip = new ImagePlus(element.getPath());
						List<Color> mapColor = new ArrayList<>();
						for(int i = 0; i < ImageUtil.getPaletteSize(ip); i++) {
							mapColor.add(ImageUtil.getColorFromIndex(ip, i));
						}
						Color[] newPalette = new Color[mapColor.size()];
			        	//Convert Image Palete Colors
						if(ImageUtil.exist(ip, newColor)) {
							//Si el color existe en otro indice, intecambiamos indices de la paleta
							int oldIndex = 0;
							while( oldIndex < mapColor.size()) {
								if(mapColor.get(oldIndex).equals(newColor)) {
									break;
								}
								oldIndex++;
							}
							int newIndex = 0;
							while( newIndex < mapColor.size()) {
								if(mapColor.get(newIndex).equals(oldColor)) {
									break;
								}
								newIndex++;
							}
							for(int i = 0; i< newPalette.length; i++) {
								if(i!=oldIndex && i!=newIndex) {
									newPalette[i] = mapColor.get(i);
								}
							}
							newPalette[oldIndex] = oldColor;
							newPalette[newIndex] = newColor;	
							ImageUtil.switchColorsImagePalette(element.getPath(), newPalette, oldIndex, newIndex);
						}else {
							//Si el color no exite pintamos la imagen con el nuevo color
							int newIndex = 0;
							while( newIndex < mapColor.size()) {
								if(mapColor.get(newIndex).equals(oldColor)) {
									break;
								}
								newIndex++;
							}
							for(int i = 0; i< newPalette.length; i++) {
								if(i!=newIndex) {
									newPalette[i] = mapColor.get(i);
								}
							}
							newPalette[newIndex] = newColor;
							ImageUtil.changeImagePalette(element.getPath(), newPalette);
						}						
						
						resourceManagerFrame.loadElement(element);
			        }
				}
				
				@Override
				public void mousePressed(MouseEvent e) {
				}
				
				@Override
				public void mouseExited(MouseEvent e) {
				}
				
				@Override
				public void mouseEntered(MouseEvent e) {
				}
				
				@Override
				public void mouseClicked(MouseEvent e) {
				}
			});
			paletteProperties.add(label);
			labels.add(label);
		}
		
		add(paletteProperties);
	}

	public void setProperties(SGDKElement e) {
		clean();	
		ImagePlus ip = new ImagePlus(e.getPath());
		labels.get(0).setText("width: " + ImageUtil.getWidth(ip) + "");
		labels.get(1).setText("height: " + ImageUtil.getHeight(ip) + "");		
		
		
		labels.get(2).setText("Type: Indexed");
		int paletteSize = ImageUtil.getPaletteSize(ip);
		for (int i = 0; i < paletteSize; i++) {
			labels.get(LABEL_INDEX_COLOR_0 + i).setBackground(ImageUtil.getColorFromIndex(ip, i));
			labels.get(LABEL_INDEX_COLOR_0 + i).setText(i + "");
		}
		element = e;
	}

	private void clean() {
		element = null;
		for(JLabel label : labels) {
			label.setText("");
		}		
	}
}
