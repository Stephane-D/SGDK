package org.sgdk.resourcemanager.ui.panels.projectexplorer;

import java.awt.Component;
import javax.swing.JTree;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeCellRenderer;

import org.sgdk.resourcemanager.entities.SGDKElement;

public class ProjectCellRender extends DefaultTreeCellRenderer {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	@Override
	public Component getTreeCellRendererComponent(JTree tree, Object value, boolean sel, boolean expanded, boolean leaf,
			int row, boolean hasFocus) {
		super.getTreeCellRendererComponent(tree, value, sel, expanded, leaf, row, hasFocus);
		
		DefaultMutableTreeNode node = (DefaultMutableTreeNode) value;
		if(node.getUserObject() instanceof SGDKElement) {			
			SGDKElement elem = (SGDKElement) node.getUserObject();
			setIcon(elem.getIcon());
		}
		return this;
	}
}
