package org.sgdk.resourcemanager.ui.menubar;

import java.awt.MenuBar;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.menubar.filemenu.FileMenu;
import org.sgdk.resourcemanager.ui.menubar.helpmenu.HelpMenu;

public class ResourceManagerMenuBar extends MenuBar {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public ResourceManagerMenuBar(ResourceManagerFrame parent) {
		super();
		add(new FileMenu(parent));
		add(new HelpMenu());
	}

}
