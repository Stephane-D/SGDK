package org.sgdk.resourcemanager.ui.menubar.filemenu;

import java.awt.Menu;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class FileMenu extends Menu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public FileMenu(ResourceManagerFrame parent) {
		super("File");
		add(new NewMenuItem(parent));
	}
}
