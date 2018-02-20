package org.sgdk.resourcemanager.ui.panels.proyectexplorer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPopupMenu;

import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.factory.SGDKEntityFactory;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class FilePopupMenu extends JPopupMenu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private SGDKElement parentNode;
	
	public FilePopupMenu(ResourceManagerFrame parent) {
		JMenuItem deleteItem = new JMenuItem("Delete Item");	
		deleteItem.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				int i=JOptionPane.showConfirmDialog(null, "Are you sure?", "Delete Item", JOptionPane.YES_NO_OPTION);
                if(i==0) {
                	SGDKEntityFactory.deleteSGDKElement(getParentNode());        
                	parent.getProyectExplorer().getProyectExplorerTree().deleteElement(getParentNode());
                }
				
			}
		});
		add(deleteItem);
	}

	public SGDKElement getParentNode() {
		return parentNode;
	}

	public void setParentNode(SGDKElement parentNode) {
		this.parentNode = parentNode;
	}

}
