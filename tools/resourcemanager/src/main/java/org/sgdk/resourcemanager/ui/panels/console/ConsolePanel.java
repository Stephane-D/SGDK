package org.sgdk.resourcemanager.ui.panels.console;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class ConsolePanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
//	PreviewPanel previewPanel = new PreviewPanel();
	
	public ConsolePanel(ResourceManagerFrame parent) throws IOException {
		super(new GridBagLayout());		
		setBorder(BorderFactory.createTitledBorder("Console"));
		
		GridBagConstraints c = new GridBagConstraints();
		
//		JScrollPane scrollPaneProyectExplorerTree = new JScrollPane(previewPanel);
		
		c.fill = GridBagConstraints.BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;	
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.REMAINDER;
//		add(scrollPaneProyectExplorerTree, c);
	}

//	public ProyectExplorerTree getProyectExplorerTree() {
//		return proyectExplorerTree;
//	}
//
//	public void setProyectExplorerTree(ProyectExplorerTree proyectExplorerTree) {
//		this.proyectExplorerTree = proyectExplorerTree;
//	}

}
