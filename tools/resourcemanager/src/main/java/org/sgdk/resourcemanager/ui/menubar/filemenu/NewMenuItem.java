package org.sgdk.resourcemanager.ui.menubar.filemenu;

import java.awt.MenuItem;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JFileChooser;

import org.sgdk.resourcemanager.entities.SGDKProject;
import org.sgdk.resourcemanager.entities.factory.SGDKEntityFactory;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.filechooser.FolderFileChooser;

public class NewMenuItem extends MenuItem {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private FolderFileChooser folderFileChooser = null;
	public NewMenuItem(ResourceManagerFrame parent) {
		super("New Project");
		
		addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(folderFileChooser == null) {
					folderFileChooser = new FolderFileChooser();
				}
				int returnVal = folderFileChooser.showDialog(parent, "New Project");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					SGDKProject project = SGDKEntityFactory.createSGDKProject(folderFileChooser.getSelectedFile().getAbsolutePath());
					parent.getProjectExplorer().getProjectExplorerTree().addElement(project, null);
		        }				
			}
		});
	}
}
