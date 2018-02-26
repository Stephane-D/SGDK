package org.sgdk.resourcemanager.ui.menubar.helpmenu;

import java.awt.Menu;

public class HelpMenu extends Menu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public HelpMenu() {
		super("Help");
		add(new AboutMenuItem());
	}
}
