package org.sgdk.resourcemanager.ui.menubar.filemenu;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import javax.swing.filechooser.FileFilter;

public class SGDKFileFilter extends FileFilter{	

	private enum ValidTypes{
		vgm,wav,bmp,png
	}
    
	@Override
	public boolean accept(File f) {
		if (f.isDirectory()) {
			return true;
		}
		String extension = SGDKFileFilter.getExtension(f);
		if (extension != null) {
			boolean valid = false;
			for(int i = 0; i<ValidTypes.values().length && !valid ; i++) {
				valid = valid || ValidTypes.values()[i].name().equals(extension);
			}
			return valid;
		} else {
			return false;
		}
	}

	@Override
	public String getDescription() {
		List<String> extensions = new ArrayList<String>();
		for(ValidTypes v: ValidTypes.values()) {
			extensions.add(v.name()+".");
		}
		return String.join(", ", extensions);
	}
	

    /*
     * Get the extension of a file.
     */  
    public static String getExtension(File f) {
        String ext = null;
        String s = f.getName();
        int i = s.lastIndexOf('.');

        if (i > 0 &&  i < s.length() - 1) {
            ext = s.substring(i+1).toLowerCase();
        }
        return ext;
    }
	

}
