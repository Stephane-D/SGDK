package org.sgdk.resourcemanager.ui.menubar.filemenu;

import java.awt.MenuItem;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class NewMenuItem extends MenuItem {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private NewMenuDialog newMenuDialog = null;
	public NewMenuItem(ResourceManagerFrame parent) {
		super("New");
		
		addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(newMenuDialog == null) {
					newMenuDialog = new NewMenuDialog(parent);
				}else {
					newMenuDialog.setVisible(true);
				}
			}
		});
	}
}
