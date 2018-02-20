package org.sgdk.resourcemanager.ui.panels.proyectexplorer;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JScrollPane;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class ProyectExplorerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private ProyectExplorerTree proyectExplorerTree = null;
	
	public ProyectExplorerPanel(ResourceManagerFrame parent, String workingDirectory) throws IOException {
		super(new GridBagLayout());
		proyectExplorerTree = new ProyectExplorerTree(parent, workingDirectory);
		
		setBorder(BorderFactory.createTitledBorder("Proyect Explorer"));
		
		GridBagConstraints c = new GridBagConstraints();
		
		JScrollPane scrollPaneProyectExplorerTree = new JScrollPane(proyectExplorerTree);
		
		c.fill = GridBagConstraints.BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;	
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.REMAINDER;
		add(scrollPaneProyectExplorerTree, c);
	}

	public ProyectExplorerTree getProyectExplorerTree() {
		return proyectExplorerTree;
	}

	public void setProyectExplorerTree(ProyectExplorerTree proyectExplorerTree) {
		this.proyectExplorerTree = proyectExplorerTree;
	}

}
