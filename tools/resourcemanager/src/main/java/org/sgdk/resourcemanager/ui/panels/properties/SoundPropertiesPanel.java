package org.sgdk.resourcemanager.ui.panels.properties;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.GregorianCalendar;
import java.util.List;
import java.util.TimeZone;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;

import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.ui.panels.preview.SoundPlayer;
import org.sgdk.resourcemanager.ui.utils.vgm.VGMPlayer;

public class SoundPropertiesPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private SimpleDateFormat df = new SimpleDateFormat("mm:ss");
	
	private JPanel soundProperties;
//	private JPanel soundFXProperties;
//	private JPanel soundEnvironmentProperties;
	
	private List<JLabel> labels = new ArrayList<>();
	
	public SoundPropertiesPanel(){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		df.setTimeZone(TimeZone.getTimeZone("UTC"));
		
		soundProperties = new JPanel();
		soundProperties.setLayout(new GridBagLayout());
		
//		soundFXProperties = new JPanel();
//		soundFXProperties.setLayout(new GridBagLayout());
//		
//		soundEnvironmentProperties = new JPanel();
//		soundEnvironmentProperties.setLayout(new GridBagLayout());

		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.HORIZONTAL;	
		c.anchor = GridBagConstraints.CENTER;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = 1;
		c.weightx = 1.0;
		c.weighty = 0.0;
		c.gridy = 0;
		c.gridx = 0;
		
		soundProperties.setBorder(
			BorderFactory.createTitledBorder(
				BorderFactory.createEtchedBorder(EtchedBorder.LOWERED),
				"Sound Properties",
				TitledBorder.RIGHT,
				TitledBorder.ABOVE_TOP
			)
		);
		
		c.gridy = 0;
		labels.add(new JLabel(""));
		soundProperties.add(labels.get(0), c);
		
		c.gridy = 1;
		labels.add(new JLabel(""));
		soundProperties.add(labels.get(1), c);
		
		c.gridy = 2;
		labels.add(new JLabel(""));
		soundProperties.add(labels.get(2), c);
		
		c.gridy = 3;
		labels.add(new JLabel(""));
		soundProperties.add(labels.get(3), c);
		
		c.gridy = 4;
		labels.add(new JLabel(""));
		soundProperties.add(labels.get(4), c);
		
		add(soundProperties);		
	}

	public void setProperties(SGDKElement e) {
		clean();
		
		String duration = "-";
		String format = "-";
		AudioFormat audioFormat = null;
		File file = new File(e.getPath());
		
		try {
			if(e.getType().equals(SGDKElement.Type.SGDKEnvironmentSound)) {
				VGMPlayer vgmPlayer = new VGMPlayer(SoundPlayer.SEGA_SOUND_RATE); 
				vgmPlayer.loadFile(file.toURI().toURL(), e.getPath());
				audioFormat = vgmPlayer.getAudioFormat();
				
				double durationSec = file.length() / audioFormat.getSampleRate() / (audioFormat.getSampleSizeInBits() / 8.0) / audioFormat.getChannels();
				GregorianCalendar gc = new GregorianCalendar(TimeZone.getTimeZone("UTC"));
//				gc.setTimeInMillis((int)(vgmPlayer.getVgmDuration()));				
				gc.setTimeInMillis((int)(durationSec * 1000));
				duration = df.format(gc.getTime());
				format = "VGM-" +audioFormat.getEncoding().toString();
					
				labels.get(2).setText("Version: " + vgmPlayer.getVgmVersion());
				labels.get(3).setText("");
				labels.get(4).setText("");	
			}else {
				AudioInputStream audioStream = AudioSystem.getAudioInputStream(file);
				audioFormat = audioStream.getFormat();
				
				double durationSec = file.length() / audioFormat.getSampleRate() / (audioFormat.getSampleSizeInBits() / 8.0) / audioFormat.getChannels();
				GregorianCalendar gc = new GregorianCalendar(TimeZone.getTimeZone("UTC"));
				gc.setTimeInMillis((int)(durationSec * 1000));
				duration = df.format(gc.getTime());
				format = audioFormat.getEncoding().toString();				
				
				labels.get(2).setText("Channels: " + audioFormat.getChannels());
				labels.get(3).setText("Frame Rate: " + audioFormat.getFrameRate());
				labels.get(4).setText("Frame Size: " + audioFormat.getFrameSize());	
			}
			labels.get(0).setText("Format: " +format);		
			labels.get(1).setText("Duration: " + duration);
		} catch (UnsupportedAudioFileException e1) {
			e1.printStackTrace();
		} catch (IOException e1) {
			e1.printStackTrace();
		} catch (Exception e1) {
			e1.printStackTrace();
		}		
	}

	private void clean() {
		for(JLabel label : labels) {
			label.setText("");
		}
	}

}
