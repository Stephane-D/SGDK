package org.sgdk.resourcemanager.ui.panels.projectexplorer;

import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JComponent;
import javax.swing.JTree;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeSelectionModel;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.constants.Constants;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKFolder;
import org.sgdk.resourcemanager.entities.SGDKProject;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

public class ProjectExplorerTree extends JTree {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private static final Logger logger = LogManager.getLogger("UILogger");
	
	private FolderPopupMenu popupFolder = null;
	private FilePopupMenu popupFile = null;	
	private ProjectPopupMenu popupProject = null;	

	private String workingDirectory;
	
	public ProjectExplorerTree(ResourceManagerFrame parent, String workingDirectory) throws IOException{
		super(new DefaultTreeModel(new DefaultMutableTreeNode("root")));
		getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
		setRootVisible(false);
		setCellRenderer(new ProjectCellRender());

		popupFolder = new FolderPopupMenu(parent);
		popupFile = new FilePopupMenu(parent);
		popupProject = new ProjectPopupMenu(parent);
		
		this.workingDirectory = workingDirectory;
		File f = new File(workingDirectory);
		if(!f.exists()) {
			f.mkdirs();
		}
		f = new File(workingDirectory+File.separator+Constants.PROJECT_SETTINGS_FILE);
		if(!f.exists()) {
			try (FileWriter writer = new FileWriter(f)){
				ObjectMapper mapper = new ObjectMapper();
				f.createNewFile();
				List<SGDKProject> projects = new ArrayList<>();
				writer.write(mapper.writeValueAsString(projects));
			}catch (IOException e) {
				logger.error(e);
			}			
		}
		try (BufferedReader br = new BufferedReader(new FileReader(f))){
			ObjectMapper mapper = new ObjectMapper();
			
			StringBuffer text = new StringBuffer();
			String sCurrentLine;
			while ((sCurrentLine = br.readLine()) != null) {
				text.append(sCurrentLine);
			}
			
			List<SGDKProject> projects = mapper.readValue(text.toString(), new TypeReference<List<SGDKProject>>(){});			
			
			for(SGDKProject p : projects) {
				initialize(p,null);
			}
		}catch (Exception e) {
			e.printStackTrace();
		}
				
	    addMouseListener(new MouseAdapter() {
			public void mouseReleased(MouseEvent e) {
				if (e.isPopupTrigger()) {
					DefaultMutableTreeNode node = (DefaultMutableTreeNode) getLastSelectedPathComponent();
				    if(node!=null) {				    	
				    	SGDKElement selected = (SGDKElement)node.getUserObject();
				    	switch(selected.getType()) {
				    	case SGDKProject:
				    		popupProject.setParentNode(selected);
				    		popupProject.show((JComponent) e.getSource(), e.getX(), e.getY());
				    		break;
				    	case SGDKFolder:
				    		popupFolder.setParentNode(selected);
				    		popupFolder.show((JComponent) e.getSource(), e.getX(), e.getY());
				    		break;
				    	default:
				    		popupFile.setParentNode(selected);
				    		popupFile.show((JComponent) e.getSource(), e.getX(), e.getY());
				    	}
				    }
				}
			}
			
			public void mouseClicked(MouseEvent e) {
	            if (e.getClickCount() == 2) {
	                DefaultMutableTreeNode node = (DefaultMutableTreeNode)getLastSelectedPathComponent();
	                if (node == null) return;
	                SGDKElement selected = (SGDKElement)node.getUserObject();	
	                logger.debug("Loading Preview");
					parent.getPreviewContainerPanel().setPreview(selected);
					logger.debug("Loading Properties");
					parent.getPropertiesContainerPanel().setSGDKElement(selected);
					logger.debug("Loading Components");
					parent.getComponentsContainerPanel().setSGDKElement(selected);
					logger.debug("End Load");
	            }
	        }
	    });
	}
	
	private void initialize(SGDKElement e, SGDKElement parent) {
		addElement(e, parent);		
		if(e.getType().equals(SGDKElement.Type.SGDKFolder)
			|| e.getType().equals(SGDKElement.Type.SGDKProject)) {
			SGDKFolder folder = (SGDKFolder)e;
			for(SGDKElement sgdkChild : folder.getChilds()) {
				initialize(sgdkChild, e);
			}
		}
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();	
		model.reload(root);
	}

	public void addElement(SGDKElement element, SGDKElement parent) {
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();			
		if(parent == null) {
			DefaultMutableTreeNode child = new DefaultMutableTreeNode(element);
			model.insertNodeInto(child, root, root.getChildCount());
		}else {
			boolean inserted = false;
			for(int i = 0; i < root.getChildCount() && !inserted; i++) {
				DefaultMutableTreeNode node = (DefaultMutableTreeNode)root.getChildAt(i);
				inserted = insert(element, parent, node);
			}
		}
	}

	private boolean insert(SGDKElement element, SGDKElement parent, DefaultMutableTreeNode node) {
		SGDKElement myElement = (SGDKElement)node.getUserObject();
		if(myElement == parent) {
			DefaultMutableTreeNode child = new DefaultMutableTreeNode(element);
			((DefaultTreeModel)getModel()).insertNodeInto(child, node, node.getChildCount());
			return true;
		}else {
			boolean inserted = false;
			for(int i = 0; i < node.getChildCount() && !inserted; i++) {
				DefaultMutableTreeNode node2 = (DefaultMutableTreeNode)node.getChildAt(i);
				inserted = insert(element, parent, node2);
			}
			return inserted;
		}
	}

	public void saveProjects() {

    	logger.info("Saving Projects...");
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();
		try (FileWriter writer = new FileWriter(new File(workingDirectory+File.separator+Constants.PROJECT_SETTINGS_FILE))) {
			List<SGDKProject> projects = new ArrayList<>();
			ObjectMapper mapper = new ObjectMapper();
			for(int i = 0; i < root.getChildCount(); i++) {
				DefaultMutableTreeNode child = (DefaultMutableTreeNode)root.getChildAt(i);
				SGDKProject project = (SGDKProject)child.getUserObject();
				projects.add(project);
			}
			writer.write(mapper.writeValueAsString(projects));
		} catch (IOException e) {
			logger.error("", e);
			e.printStackTrace();
		}
    	logger.info("End Program");
	}

	public void deleteElement(SGDKElement element) {
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();	
		deleteElement(element, root);
	}

	private boolean deleteElement(SGDKElement element, DefaultMutableTreeNode node) {
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		boolean deleted = false;
		for(int i = 0; i < node.getChildCount() && !deleted; i++) {
			DefaultMutableTreeNode child = (DefaultMutableTreeNode)node.getChildAt(i);
			if((SGDKElement)child.getUserObject() == element) {				
				model.removeNodeFromParent(child);
				deleted = true;
			}else {
				deleted = deleteElement(element, child);
			}
		}
		return deleted;
	}
}
