package org.sgdk.resourcemanager.ui;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.IOException;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JSplitPane;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.ui.menubar.ResourceManagerMenuBar;
import org.sgdk.resourcemanager.ui.panels.components.ComponentsContainerPanel;
import org.sgdk.resourcemanager.ui.panels.console.ConsolePanel;
import org.sgdk.resourcemanager.ui.panels.preview.PreviewContainerPanel;
import org.sgdk.resourcemanager.ui.panels.projectexplorer.ProjectExplorerPanel;
import org.sgdk.resourcemanager.ui.panels.properties.PropertiesContainerPanel;

public class ResourceManagerFrame extends JFrame implements ComponentListener{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private static final Logger logger = LogManager.getLogger("UILogger");
	
	private static final int minimizeWidth = 800;
	private static final int minimizeHeight = 600;
	
	private ProjectExplorerPanel projectExplorer = null;
	private PreviewContainerPanel previewContainerPanel = null;
	private ConsolePanel consolePanel = null;
	private PropertiesContainerPanel propertiesContainerPanel = null;
	private ComponentsContainerPanel componentsContainerPanel = null;

	private String workingDirectory;
	private boolean loadedData = false;
	
	public ResourceManagerFrame(String workingDirectory) throws IOException {
		super("SGDK Resource Manager");
		
		this.workingDirectory = workingDirectory;
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		int screenWidth = new Long(Math.round(screenSize.getWidth())).intValue();
		int screenHeight = new Long(Math.round(screenSize.getHeight())).intValue();
		
		setBounds(new Rectangle(screenWidth/2 - minimizeWidth/2, screenHeight/2 - minimizeHeight/2, minimizeWidth, minimizeHeight));
		setMaximizedBounds(new Rectangle(0, 0, screenWidth, screenHeight));
		
		setMenuBar(new ResourceManagerMenuBar(this));

		previewContainerPanel = new PreviewContainerPanel(this);		
		previewContainerPanel.setPreferredSize(new Dimension(getWidth()/2, getHeight()*3/4));
		previewContainerPanel.setMinimumSize(new Dimension(minimizeWidth/4, minimizeHeight*2/3));
		
		propertiesContainerPanel = new PropertiesContainerPanel(this);
		propertiesContainerPanel.setPreferredSize(new Dimension(getWidth()/6, getHeight()*3/4));
		propertiesContainerPanel.setMinimumSize(new Dimension(minimizeWidth/8,minimizeHeight/3));
		
		JSplitPane preview_propsSP = new JSplitPane(
    			JSplitPane.HORIZONTAL_SPLIT,
    			true,
    	        previewContainerPanel, propertiesContainerPanel);
		preview_propsSP.resetToPreferredSizes();
		consolePanel = new ConsolePanel(this);
		consolePanel.setPreferredSize(new Dimension(getWidth()/2, getHeight()/4));
		consolePanel.setMinimumSize(new Dimension(minimizeWidth/4, minimizeHeight/6));
		
		JSplitPane preview_props_consoleSP = new JSplitPane(
    			JSplitPane.VERTICAL_SPLIT,
    			true,
    			preview_propsSP, consolePanel);
		preview_props_consoleSP.resetToPreferredSizes();
		componentsContainerPanel = new ComponentsContainerPanel();
		componentsContainerPanel.setPreferredSize(new Dimension(getWidth()/6, getHeight()));
		componentsContainerPanel.setMinimumSize(new Dimension(minimizeWidth/8, minimizeHeight/3));
		
		JSplitPane preview_props_console_compoSP = new JSplitPane(
    			JSplitPane.HORIZONTAL_SPLIT,
    			true,
    			preview_props_consoleSP, componentsContainerPanel);
		preview_props_console_compoSP.resetToPreferredSizes();
		projectExplorer = new ProjectExplorerPanel(this);
		projectExplorer.setPreferredSize(new Dimension(getWidth()/6, getHeight()));
		projectExplorer.setMinimumSize(new Dimension(minimizeWidth/4, minimizeHeight/3));
		
		JSplitPane all = new JSplitPane(
    			JSplitPane.HORIZONTAL_SPLIT,
    			true,
    			projectExplorer, preview_props_console_compoSP);
		all.resetToPreferredSizes();
		add(all);

		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		addWindowListener(new WindowAdapter(){
            public void windowClosing(WindowEvent e){
                int i=JOptionPane.showConfirmDialog(null, "Do you want to exit the Resource Manager?","Confirm Exit", JOptionPane.OK_CANCEL_OPTION);
                if(i < 2) {
                	if(i == 0) {              
                		if(loadedData) {                			
                			projectExplorer.getProjectExplorerTree().saveProjects(workingDirectory);
                		}
                	}
                	Thread t = new Thread(new Runnable() {						
						@Override
						public void run() {
							try {
								Thread.sleep(1500);
							} catch (InterruptedException e1) {
							}
							System.exit(0);//cierra aplicacion							
						}
					});
                	logger.info("Closing...");
                	t.start();
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
	
	public void load() {
		this.loadedData = this.projectExplorer.load(workingDirectory);
	}

	@Override
	public void componentResized(ComponentEvent e) {
		previewContainerPanel.setPreferredSize(new Dimension(getWidth()/2, getHeight()*3/4));
		previewContainerPanel.setMinimumSize(new Dimension(getWidth()/4, getHeight()*2/3));
		propertiesContainerPanel.setPreferredSize(new Dimension(getWidth()/6, getHeight()*3/4));
		propertiesContainerPanel.setMinimumSize(new Dimension(getWidth()/8,getHeight()/3));
		consolePanel.setPreferredSize(new Dimension(getWidth()/2, getHeight()/4));
		consolePanel.setMinimumSize(new Dimension(getWidth()/4, getHeight()/6));
		componentsContainerPanel.setPreferredSize(new Dimension(getWidth()/6, getHeight()));
		componentsContainerPanel.setMinimumSize(new Dimension(getWidth()/8, getHeight()/3));
		projectExplorer.setPreferredSize(new Dimension(getWidth()/6, getHeight()));
		projectExplorer.setMinimumSize(new Dimension(getWidth()/4, getHeight()/3));
	}

	@Override
	public void componentMoved(ComponentEvent e) {}

	@Override
	public void componentShown(ComponentEvent e) {}

	@Override
	public void componentHidden(ComponentEvent e) {}

	public void loadElement(SGDKElement selected) {
		logger.debug("Loading Preview");
		getPreviewContainerPanel().setPreview(selected);
		logger.debug("Loading Properties");
		getPropertiesContainerPanel().setSGDKElement(selected);
		logger.debug("Loading Components");
		getComponentsContainerPanel().setSGDKElement(selected);
		logger.debug("End Load");
	}
	
}
