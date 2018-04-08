package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JComboBox;
import javax.swing.JPanel;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKBackground;

public class CompressionComponent extends JPanel{
	
	/**
	 *  -1 / BEST / AUTO = use best compression
     *   0 / NONE        = no compression
     *   1 / APLIB       = aplib library (good compression ratio but slow)
     *   2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)
	 */	
	
	private static final long serialVersionUID = 1L;
	private SGDKBackground background = null;
	private JComboBox<SGDKBackground.Compression> options = new JComboBox<SGDKBackground.Compression>(SGDKBackground.Compression.values());

	public  CompressionComponent() {
		super(new GridLayout(1,1));
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Compression",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		options.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(background != null){
					background.setCompression((SGDKBackground.Compression)options.getSelectedItem());
				}
			}
		});
		add(options);
	}
	
	public void setSGDKBackground(SGDKBackground background) {
		this.background = background;
		options.setSelectedIndex(background.getCompression().ordinal());
	}
	
}
