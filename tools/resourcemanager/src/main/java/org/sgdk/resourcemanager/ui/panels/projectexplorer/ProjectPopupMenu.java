package org.sgdk.resourcemanager.ui.panels.projectexplorer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JFileChooser;
import javax.swing.JMenuItem;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKProject;
import org.sgdk.resourcemanager.entities.factory.SGDKResfileExportManager;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.filechooser.FolderFileChooser;

public class ProjectPopupMenu extends FolderPopupMenu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private static final Logger logger = LogManager.getLogger("UILogger");

	private FolderFileChooser folderFileChooser = null;
	
	public ProjectPopupMenu(ResourceManagerFrame parent) {
		super(parent);
		JMenuItem exportProject = new JMenuItem("Export Project");		
		addSeparator();
		exportProject.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(folderFileChooser == null) {
					folderFileChooser = new FolderFileChooser();
				}
				
				int returnVal = folderFileChooser.showDialog(parent, "Export Project");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					try {
						SGDKResfileExportManager.export((SGDKProject)getParentNode(), folderFileChooser.getSelectedFile().getAbsolutePath());
					} catch (Exception e1) {
						logger.error(e1);
					}
		        }	
				
			}
		});
		add(exportProject);
	}
}
