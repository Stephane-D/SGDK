package org.sgdk.resourcemanager.ui.filechooser;

import java.io.File;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;

import org.apache.commons.lang3.StringUtils;
import org.sgdk.resourcemanager.entities.SGDKFXSound;

public class SFXFileChooser extends JFileChooser{

	private static final long serialVersionUID = 1L;

	public SFXFileChooser() {
		super(System.getProperty("user.home"));
		
		addChoosableFileFilter(new FileFilter() {			
			@Override
			public String getDescription() {
				return StringUtils.join(SGDKFXSound.ValidFormat.values(), ", ");
			}
			
			@Override
			public boolean accept(File f) {
				return SGDKFXSound.isValidFormat(f.getAbsolutePath()) || f.isDirectory();
			}
		});
		setAcceptAllFileFilterUsed(false);
		setMultiSelectionEnabled(true);		
	}
	
}
