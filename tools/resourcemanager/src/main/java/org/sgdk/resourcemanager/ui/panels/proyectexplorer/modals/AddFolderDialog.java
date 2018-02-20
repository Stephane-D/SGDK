package org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKFolder;
import org.sgdk.resourcemanager.entities.factory.SGDKEntityFactory;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class AddFolderDialog extends JDialog{

	private static final int minimizeWidth = 340;
	private static final int minimizeHeight = 220;
	
	private JLabel textError = new JLabel();
	private JTextField folderPathText = new JTextField();
	private JButton acceptButon = new JButton("Ok");
	private static final long serialVersionUID = 1L;
	
	public AddFolderDialog(ResourceManagerFrame parent, SGDKElement parentNode) {
		super(parent, "Add Folder");
		parent.setEnabled(false);
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		int screenWidth = new Long(Math.round(screenSize.getWidth())).intValue();
		int screenHeight = new Long(Math.round(screenSize.getHeight())).intValue();
		
		setBounds(new Rectangle(screenWidth/2 - minimizeWidth/2, screenHeight/2 - minimizeHeight/2, minimizeWidth, minimizeHeight));
		setResizable(false);
//		setAlwaysOnTop(true);
		
		addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent evt) {
				clean();
				parent.setEnabled(true);
			}
		});
				
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		
		textError.setForeground (Color.red);
		
		c.fill = GridBagConstraints.NONE;
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 1d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 2;
		c.gridheight = 1;
		add(textError, c);
			
		c.fill = GridBagConstraints.NONE;
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 1d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 0;
		c.gridy = 1;
		c.gridwidth = 1;
		c.gridheight = 1;
		add(new JLabel("Folder Name: "), c);
		
		c.fill = GridBagConstraints.HORIZONTAL;	
		c.anchor = GridBagConstraints.LINE_START;
		c.weightx = 5d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 1;
		c.gridy = 1;
		c.gridwidth = 5;
		c.gridheight = 1;
		add(folderPathText, c);		
		
		c.fill = GridBagConstraints.NONE;
		c.anchor = GridBagConstraints.LINE_END;
		c.weightx = 1d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 5;
		c.gridy = 2;
		c.gridwidth = 1;
		c.gridheight = 1;
		add(acceptButon, c);
		
		acceptButon.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean validForm = true;
				
				if(validForm && (folderPathText.getText() == null || folderPathText.getText().isEmpty())) {
					validForm = false;
					JPanel panel = new JPanel();
					JOptionPane.showMessageDialog(panel,
							 "Invalid Folder Name",
							 "Error",
					        JOptionPane.ERROR_MESSAGE);
				}
				
				if (validForm) {
					File directory = new File(parentNode.getPath()+File.separator+folderPathText.getText());
					if(!directory.exists()) {
						SGDKFolder folder = SGDKEntityFactory.createSGDKFolder(folderPathText.getText(), (SGDKFolder)parentNode);
						parent.getProyectExplorer().getProyectExplorerTree().addElement(folder, parentNode);
						clean();
						parent.setEnabled(true);
						setVisible(false);
					}else {
						textError.setText("Carpeta ya existe");
					}
				}
			}
		});		
		setVisible(true);
	}

	protected void clean() {
		folderPathText.setText("");
		textError.setText("");
	}
}
