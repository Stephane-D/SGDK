package org.sgdk.resourcemanager.ui.panels.components.components;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JComboBox;
import javax.swing.JPanel;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKFXSound;
import org.sgdk.resourcemanager.entities.SGDKFXSound.Driver;

public class DriverComponent extends JPanel{
	
	/**
	 *  0 / PCM (default)
	        Single channel 8 bits signed sample driver.
	        It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.
	        Method to use: SND_startPlay_PCM(..)
	    1 / 2ADPCM
	        2 channels 4 bits ADPCM sample driver.
	        It can mix up to 2 ADCPM samples at a fixed 22050 Hz rate.
	        Method to use: SND_startPlay_2ADPCM(..)
	    2 / 3 / 4PCM
	        4 channels 8 bits signed sample driver with volume support.
	        It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.
	        with volume support (16 levels du to memory limitation).
	        Method to use: SND_startPlay_4PCM_ENV(..)
	    4 / VGM
	        VGM music driver with 8 bits PCM SFX support.
	        It supports single PCM SFX at a fixed ~9 Khz rate while playing VGM music.
	        Method to use: SND_playSfx_VGM(..)
	    5 / XGM
	        XGM music with 4 channels 8 bits samples driver.
	        It supports 4 PCM SFX at a fixed 14 Khz rate while playing XGM music.
	        Methods to use: SND_setPCM_XGM(..) and SND_startPlayPCM_XGM(..)
	 */	
	
	private static final long serialVersionUID = 1L;
	private SGDKFXSound fxSound = null;
	private JComboBox<SGDKFXSound.Driver> options = new JComboBox<SGDKFXSound.Driver>(SGDKFXSound.Driver.values());

	private OutrateComponent outrateComponent;
	
	public  DriverComponent(OutrateComponent outrateComponent) {
		super(new GridLayout(1,1));
		this.outrateComponent = outrateComponent;
		setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Driver",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		options.addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(fxSound != null){
					fxSound.setDriver((SGDKFXSound.Driver)options.getSelectedItem());
					if(fxSound.getDriver().equals(Driver.PCM)) {
						outrateComponent.setVisible(true);
					}else {
						outrateComponent.setVisible(false);
					}
				}
			}
		});
		add(options);
	}
	
	public void setSGDKFXSound(SGDKFXSound fxSound) {
		this.fxSound = fxSound;
		options.setSelectedIndex(fxSound.getDriver().ordinal());

		if(fxSound.getDriver().equals(SGDKFXSound.Driver.PCM)) {			
			outrateComponent.setSGDKFXSound(fxSound);
			outrateComponent.setVisible(true);
		}else {
			outrateComponent.setVisible(false);
		}
	}
	
}
