package org.sgdk.resourcemanager.ui.panels.projectexplorer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenuItem;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.panels.projectexplorer.modals.ExportProjectDialog;

public class ProjectPopupMenu extends FolderPopupMenu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private ExportProjectDialog exportProjectDialog = null;
	
	public ProjectPopupMenu(ResourceManagerFrame parent) {
		super(parent);
		JMenuItem exportProject = new JMenuItem("ExportProject");		
		addSeparator();
		exportProject.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(exportProjectDialog == null) {
					exportProjectDialog = new ExportProjectDialog(parent, getParentNode());
				}else {
					exportProjectDialog.setVisible(true);
				}
			}
		});
		add(exportProject);
	}
}
