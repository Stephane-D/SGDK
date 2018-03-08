package org.sgdk.resourcemanager.ui;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.IOException;

import javax.swing.JFrame;
import javax.swing.JOptionPane;

import org.sgdk.resourcemanager.ui.menubar.ResourceManagerMenuBar;
import org.sgdk.resourcemanager.ui.panels.components.ComponentsContainerPanel;
import org.sgdk.resourcemanager.ui.panels.console.ConsolePanel;
import org.sgdk.resourcemanager.ui.panels.preview.PreviewContainerPanel;
import org.sgdk.resourcemanager.ui.panels.properties.PropertiesContainerPanel;
import org.sgdk.resourcemanager.ui.panels.projectexplorer.ProjectExplorerPanel;

public class ResourceManagerFrame extends JFrame {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private static final int minimizeWidth = 800;
	private static final int minimizeHeight = 600;
	
	private ProjectExplorerPanel projectExplorer = null;
	private PreviewContainerPanel previewContainerPanel = null;
	private ConsolePanel consolePanel = null;
	private PropertiesContainerPanel propertiesContainerPanel = null;
	private ComponentsContainerPanel componentsContainerPanel = null;
	
	public ResourceManagerFrame(String workingDirectory) throws IOException {
		super("SGDK Resource Manager");
		
		
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		int screenWidth = new Long(Math.round(screenSize.getWidth())).intValue();
		int screenHeight = new Long(Math.round(screenSize.getHeight())).intValue();
		
		setBounds(new Rectangle(screenWidth/2 - minimizeWidth/2, screenHeight/2 - minimizeHeight/2, minimizeWidth, minimizeHeight));
		setMaximizedBounds(new Rectangle(0, 0, screenWidth, screenHeight));
		
		setMenuBar(new ResourceManagerMenuBar(this));
		
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		
		c.weightx = 1d/6d;
		c.weighty = 1.0;		
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		projectExplorer = new ProjectExplorerPanel(this, workingDirectory);
		add(projectExplorer, c);
		
		c.weightx = 1d/2d;
		c.weighty = 4d/5d;
		c.gridx = 1;
		c.gridy = 0;
		c.gridwidth = 3;
		c.gridheight = 4;
		previewContainerPanel = new PreviewContainerPanel(this);
		add(previewContainerPanel, c);
		
		c.weightx = 1d/2d;
		c.weighty = 1d/5d;
		c.gridx = 1;
		c.gridy = 4;
		c.gridwidth = 3;
		c.gridheight = GridBagConstraints.REMAINDER;
		consolePanel = new ConsolePanel(this);
		add(consolePanel, c);
		
		c.weightx = 1d/6d;
		c.weighty = 1.0;	
		c.gridx = 4;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		propertiesContainerPanel = new PropertiesContainerPanel(this);
		add(propertiesContainerPanel, c);
		
		c.weightx = 1d/6d;
		c.weighty = 1.0;	
		c.gridx = 5;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		componentsContainerPanel = new ComponentsContainerPanel(this);
		add(componentsContainerPanel, c);
		
		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		addWindowListener(new WindowAdapter(){
            public void windowClosing(WindowEvent e){
                int i=JOptionPane.showConfirmDialog(null, "Do you want save the projects?");
                if(i < 2) {
                	if(i == 0) {                		
                		projectExplorer.getProjectExplorerTree().saveProjects();
                	}
                	System.exit(0);//cierra aplicacion
                }
            }
        });
		
		setVisible(true);
	}

	public ProjectExplorerPanel getProjectExplorer() {
		return projectExplorer;
	}

	public void setProjectExplorer(ProjectExplorerPanel projectExplorer) {
		this.projectExplorer = projectExplorer;
	}

	public PreviewContainerPanel getPreviewContainerPanel() {
		return previewContainerPanel;
	}

	public void setPreviewContainerPanel(PreviewContainerPanel previewContainerPanel) {
		this.previewContainerPanel = previewContainerPanel;
	}

	public ConsolePanel getConsolePanel() {
		return consolePanel;
	}

	public void setConsolePanel(ConsolePanel consolePanel) {
		this.consolePanel = consolePanel;
	}

	public PropertiesContainerPanel getPropertiesContainerPanel() {
		return propertiesContainerPanel;
	}

	public void setPropertiesContainerPanel(PropertiesContainerPanel propertiesContainerPanel) {
		this.propertiesContainerPanel = propertiesContainerPanel;
	}

	public ComponentsContainerPanel getComponentsContainerPanel() {
		return componentsContainerPanel;
	}

	public void setComponentsContainerPanel(ComponentsContainerPanel componentsContainerPanel) {
		this.componentsContainerPanel = componentsContainerPanel;
	}	
	
}
