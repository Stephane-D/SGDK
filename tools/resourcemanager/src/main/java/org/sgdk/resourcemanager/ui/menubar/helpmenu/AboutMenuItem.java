package org.sgdk.resourcemanager.ui.menubar.helpmenu;

import java.awt.MenuItem;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JOptionPane;
import javax.swing.JPanel;

public class AboutMenuItem extends MenuItem {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public AboutMenuItem() {
		super("About SGDK Resource Manager");
		
		addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				 JPanel panel = new JPanel();
				 JOptionPane.showMessageDialog(panel,
						 "SGDK Resource Manager v0.4.0\n"
						 + "This envairoment is used to manage rescomp file on SGDK\n"
						 + "Developed by Pachon89",
						 "About SGDK Resource Manager",
				        JOptionPane.INFORMATION_MESSAGE);
			}
		});
	}

}
