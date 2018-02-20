package org.sgdk.resourcemanager.ui;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.ui.menubar.ResourceManagerMenuBar;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.ProyectExplorerPanel;

public class ResourceManagerFrame extends JFrame {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private static final int minimizeWidth = 800;
	private static final int minimizeHeight = 600;
	
	private ProyectExplorerPanel proyectExplorer = null;
	
	public ResourceManagerFrame(String workingDirectory) throws IOException {
		super("SGDK Resource Manager");
		
		
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		int screenWidth = new Long(Math.round(screenSize.getWidth())).intValue();
		int screenHeight = new Long(Math.round(screenSize.getHeight())).intValue();
		
		setBounds(new Rectangle(screenWidth/2 - minimizeWidth/2, screenHeight/2 - minimizeHeight/2, minimizeWidth, minimizeHeight));
		setMaximizedBounds(new Rectangle(0, 0, screenWidth, screenHeight));
		
		setMenuBar(new ResourceManagerMenuBar(this));
		
		JPanel aux;
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		
		c.weightx = 1d/6d;
		c.weighty = 1.0;		
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		proyectExplorer = new ProyectExplorerPanel(this, workingDirectory);
		add(proyectExplorer, c);
		
		c.weightx = 1d/2d;
		c.weighty = 4d/5d;
		c.gridx = 1;
		c.gridy = 0;
		c.gridwidth = 3;
		c.gridheight = 4;
		aux = new JPanel();
		aux.setBorder(BorderFactory.createTitledBorder("Preview"));
		add(aux, c);
		
		c.weightx = 1d/2d;
		c.weighty = 1d/5d;
		c.gridx = 1;
		c.gridy = 4;
		c.gridwidth = 3;
		c.gridheight = GridBagConstraints.REMAINDER;
		aux = new JPanel();
		aux.setBorder(BorderFactory.createTitledBorder("Console"));
		add(aux, c);
		
		c.weightx = 1d/6d;
		c.weighty = 1.0;	
		c.gridx = 4;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		aux = new JPanel();
		aux.setBorder(BorderFactory.createTitledBorder("Properties"));
		add(aux, c);
		
		c.weightx = 1d/6d;
		c.weighty = 1.0;	
		c.gridx = 5;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = GridBagConstraints.REMAINDER;
		aux = new JPanel();
		aux.setBorder(BorderFactory.createTitledBorder("Components"));
		add(aux, c);
		
		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		addWindowListener(new WindowAdapter(){
            public void windowClosing(WindowEvent e){
                int i=JOptionPane.showConfirmDialog(null, "Seguro que quiere salir?");
                if(i==0) {
                	proyectExplorer.getProyectExplorerTree().saveProyects();
                    System.exit(0);//cierra aplicacion
                }
            }
        });
		
		setVisible(true);
	}

	public ProyectExplorerPanel getProyectExplorer() {
		return proyectExplorer;
	}

	public void setProyectExplorer(ProyectExplorerPanel proyectExplorer) {
		this.proyectExplorer = proyectExplorer;
	}	
	
}
