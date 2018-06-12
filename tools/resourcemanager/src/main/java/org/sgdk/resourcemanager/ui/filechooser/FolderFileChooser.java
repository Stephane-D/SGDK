package org.sgdk.resourcemanager.ui.filechooser;

import javax.swing.JFileChooser;

public class FolderFileChooser extends JFileChooser{

	private static final long serialVersionUID = 1L;

	public FolderFileChooser() {
		super(System.getProperty("user.home"));
		
//		addChoosableFileFilter(new FileFilter() {			
//			@Override
//			public String getDescription() {
//				return StringUtils.join(SGDKSprite.ValidFormat.values(), ", ");
//			}
//			
//			@Override
//			public boolean accept(File f) {
//				return SGDKSprite.isValidFormat(f.getAbsolutePath()) || f.isDirectory();
//			}
//		});
		setAcceptAllFileFilterUsed(false);
		setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY );
	}

}
