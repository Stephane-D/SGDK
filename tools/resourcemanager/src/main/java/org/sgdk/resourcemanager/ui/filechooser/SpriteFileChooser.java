package org.sgdk.resourcemanager.ui.filechooser;

import java.io.File;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;

import org.apache.commons.lang3.StringUtils;
import org.sgdk.resourcemanager.entities.SGDKSprite;

public class SpriteFileChooser extends JFileChooser{

	private static final long serialVersionUID = 1L;

	public SpriteFileChooser() {
		super(System.getProperty("user.home"));
		
		addChoosableFileFilter(new FileFilter() {			
			@Override
			public String getDescription() {
				return StringUtils.join(SGDKSprite.ValidFormat.values(), ", ");
			}
			
			@Override
			public boolean accept(File f) {
				return SGDKSprite.isValidFormat(f.getAbsolutePath()) || f.isDirectory();
			}
		});
		setAcceptAllFileFilterUsed(false);
		setMultiSelectionEnabled(true);
	}

}
