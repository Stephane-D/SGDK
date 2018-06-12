package org.sgdk.resourcemanager.ui.filechooser;

import java.io.File;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;

import org.apache.commons.lang3.StringUtils;
import org.sgdk.resourcemanager.entities.SGDKBackground;

public class BackgroundFileChooser extends JFileChooser{

	private static final long serialVersionUID = 1L;	
	
	public BackgroundFileChooser() {
		super(System.getProperty("user.home"));

		addChoosableFileFilter(new FileFilter() {			
			@Override
			public String getDescription() {
				return StringUtils.join(SGDKBackground.ValidFormat.values(), ", ");
			}
			
			@Override
			public boolean accept(File f) {
				return SGDKBackground.isValidFormat(f.getAbsolutePath()) || f.isDirectory();
			}
		});
		setAcceptAllFileFilterUsed(false);
		setMultiSelectionEnabled(true);		
	}

}
