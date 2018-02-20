package org.sgdk.resourcemanager.ui.panels.proyectexplorer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenuItem;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals.ExportProyectDialog;

public class ProyectPopupMenu extends FolderPopupMenu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private ExportProyectDialog exportProyectDialog = null;
	
	public ProyectPopupMenu(ResourceManagerFrame parent) {
		super(parent);
		JMenuItem exportProyect = new JMenuItem("ExportProyect");		
		addSeparator();
		exportProyect.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(exportProyectDialog == null) {
					exportProyectDialog = new ExportProyectDialog(parent, getParentNode());
				}else {
					exportProyectDialog.setVisible(true);
				}
			}
		});
		add(exportProyect);
	}
}
