package org.sgdk.resourcemanager.ui.panels.projectexplorer;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JScrollPane;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class ProjectExplorerPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private ProjectExplorerTree projectExplorerTree = null;
	
	public ProjectExplorerPanel(ResourceManagerFrame parent) throws IOException {
		super(new GridBagLayout());
		projectExplorerTree = new ProjectExplorerTree(parent);
		
		setBorder(BorderFactory.createTitledBorder("Project Explorer"));
		
		GridBagConstraints c = new GridBagConstraints();
		
		JScrollPane scrollPaneProjectExplorerTree = new JScrollPane(projectExplorerTree);
		
		c.fill = GridBagConstraints.BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;	
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.REMAINDER;
		add(scrollPaneProjectExplorerTree, c);
	}

	public ProjectExplorerTree getProjectExplorerTree() {
		return projectExplorerTree;
	}

	public void setProjectExplorerTree(ProjectExplorerTree projectExplorerTree) {
		this.projectExplorerTree = projectExplorerTree;
	}
	
	public boolean load(String workingDirectory) {
		return this.projectExplorerTree.load(workingDirectory);
	}

}
