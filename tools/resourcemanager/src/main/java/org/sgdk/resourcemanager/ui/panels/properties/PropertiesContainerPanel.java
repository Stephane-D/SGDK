package org.sgdk.resourcemanager.ui.panels.properties;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class PropertiesContainerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
//	PreviewPanel previewPanel = new PreviewPanel();
	
	public PropertiesContainerPanel(ResourceManagerFrame parent) throws IOException {
		super(new GridBagLayout());		
		setBorder(BorderFactory.createTitledBorder("Components"));
		
		GridBagConstraints c = new GridBagConstraints();
		
//		JScrollPane scrollPaneProjectExplorerTree = new JScrollPane(previewPanel);
		
		c.fill = GridBagConstraints.BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;	
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.REMAINDER;
//		add(scrollPaneProjectExplorerTree, c);
	}

//	public ProjectExplorerTree getProjectExplorerTree() {
//		return projectExplorerTree;
//	}
//
//	public void setProjectExplorerTree(ProjectExplorerTree projectExplorerTree) {
//		this.projectExplorerTree = projectExplorerTree;
//	}

}
