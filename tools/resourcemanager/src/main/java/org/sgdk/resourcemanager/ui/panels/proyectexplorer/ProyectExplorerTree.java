package org.sgdk.resourcemanager.ui.panels.proyectexplorer;

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
import org.sgdk.resourcemanager.entities.SGDKProyect;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

public class ProyectExplorerTree extends JTree {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private static final Logger logger = LogManager.getLogger(ProyectExplorerTree.class);
	
	private FolderPopupMenu popupFolder = null;
	private FilePopupMenu popupFile = null;	
	private ProyectPopupMenu popupProyect = null;	

	private String workingDirectory;
	
	public ProyectExplorerTree(ResourceManagerFrame parent, String workingDirectory) throws IOException{
		super(new DefaultTreeModel(new DefaultMutableTreeNode("root")));
		getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
		setRootVisible(false);
		setCellRenderer(new ProyectCellRender());
		
		popupFolder = new FolderPopupMenu(parent);
		popupFile = new FilePopupMenu(parent);
		popupProyect = new ProyectPopupMenu(parent);
		
		this.workingDirectory = workingDirectory;
		File f = new File(workingDirectory);
		if(!f.exists()) {
			f.mkdirs();
		}
		f = new File(workingDirectory+File.separator+Constants.PROYECT_SETTINGS_FILE);
		if(!f.exists()) {
			try (FileWriter writer = new FileWriter(f)){
				ObjectMapper mapper = new ObjectMapper();
				f.createNewFile();
				List<SGDKProyect> proyects = new ArrayList<>();
				writer.write(mapper.writeValueAsString(proyects));
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
			
			List<SGDKProyect> proyects = mapper.readValue(text.toString(), new TypeReference<List<SGDKProyect>>(){});			
			
			for(SGDKProyect p : proyects) {
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
				    	case SGDKProyect:
				    		popupProyect.setParentNode(selected);
				    		popupProyect.show((JComponent) e.getSource(), e.getX(), e.getY());
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
				//TODO
	            if (e.getClickCount() == 2) {
	                DefaultMutableTreeNode node = (DefaultMutableTreeNode)getLastSelectedPathComponent();
	                if (node == null) return;
	                SGDKElement selected = (SGDKElement)node.getUserObject();
	                switch(selected.getType()) {
					case SGDKBackground:
						break;
					case SGDKSprite:
						break;
					case SGDKFXSound:
						break;
					case SGDKEnvironmentSound:
						break;
					default:
						break;	                
	                }
	            }
	        }
	    });
	}
	
	private void initialize(SGDKElement e, SGDKElement parent) {
		addElement(e, parent);		
		if(e.getType().equals(SGDKElement.Type.SGDKFolder)
			|| e.getType().equals(SGDKElement.Type.SGDKProyect)) {
			SGDKFolder folder = (SGDKFolder)e;
			for(SGDKElement sgdkChild : folder.getChilds()) {
				initialize(sgdkChild, e);
			}
		}
	}

	public void addElement(SGDKElement element, SGDKElement parent) {
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();			
		if(parent == null) {
			DefaultMutableTreeNode child = new DefaultMutableTreeNode(element);
			root.add(child);
		}else {
			boolean inserted = false;
			for(int i = 0; i < root.getChildCount() && !inserted; i++) {
				DefaultMutableTreeNode node = (DefaultMutableTreeNode)root.getChildAt(i);
				inserted = insert(element, parent, node);
			}
		}
		model.reload(root);
	}

	private boolean insert(SGDKElement element, SGDKElement parent, DefaultMutableTreeNode node) {
		SGDKElement myElement = (SGDKElement)node.getUserObject();
		if(myElement == parent) {
			DefaultMutableTreeNode child = new DefaultMutableTreeNode(element);
			node.add(child);
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

	public void saveProyects() {

    	logger.debug("Saving Proyects...");
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();
		try (FileWriter writer = new FileWriter(new File(workingDirectory+File.separator+Constants.PROYECT_SETTINGS_FILE))) {
			List<SGDKProyect> proyects = new ArrayList<>();
			ObjectMapper mapper = new ObjectMapper();
			for(int i = 0; i < root.getChildCount(); i++) {
				DefaultMutableTreeNode child = (DefaultMutableTreeNode)root.getChildAt(i);
				SGDKProyect proyect = (SGDKProyect)child.getUserObject();
				proyects.add(proyect);
			}
			writer.write(mapper.writeValueAsString(proyects));
		} catch (IOException e) {
			logger.error("", e);
			e.printStackTrace();
		}
    	logger.debug("End Program");
	}

	public void deleteElement(SGDKElement element) {
		DefaultTreeModel model = (DefaultTreeModel)getModel();
		DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();	
		deleteElement(element, root);
		model.reload(root);
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
