package org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.filechooser.FileFilter;

import org.apache.commons.lang3.StringUtils;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;
import org.sgdk.resourcemanager.entities.SGDKFolder;
import org.sgdk.resourcemanager.entities.factory.SGDKEntityFactory;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class CreateEnvironmentDialog extends JDialog{

	private static final int minimizeWidth = 340;
	private static final int minimizeHeight = 220;
	
	private JTextField environmentSoundPathText = new JTextField();
	private JFileChooser environmentSoundPath = new JFileChooser(System.getProperty("user.home"));
	private JButton acceptButon = new JButton("Ok");
	private static final long serialVersionUID = 1L;
	
	public CreateEnvironmentDialog(ResourceManagerFrame parent, SGDKElement parentNode) {
		super(parent, "New Environment Sound");
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
		
		environmentSoundPath.addChoosableFileFilter(new FileFilter() {			
			@Override
			public String getDescription() {
				return StringUtils.join(SGDKEnvironmentSound.ValidFormat.values(), ", ");
			}
			
			@Override
			public boolean accept(File f) {
				return SGDKEnvironmentSound.isValidFormat(f.getAbsolutePath())  || f.isDirectory();
			}
		});
		environmentSoundPath.setAcceptAllFileFilterUsed(false);
		
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
			
		c.fill = GridBagConstraints.NONE;
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 1d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 1;
		c.gridheight = 1;
		add(new JLabel("Environment Sound Path: "), c);
		
		c.fill = GridBagConstraints.HORIZONTAL;	
		c.anchor = GridBagConstraints.LINE_START;
		c.weightx = 5d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 1;
		c.gridy = 0;
		c.gridwidth = 5;
		c.gridheight = 1;
		add(environmentSoundPathText, c);		
		
		environmentSoundPathText.addMouseListener(new MouseAdapter() {
			@Override
            public void mouseClicked(MouseEvent e){				
				int returnVal = environmentSoundPath.showDialog(parent, "New Environment Sound");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					environmentSoundPathText.setText(environmentSoundPath.getSelectedFile().getAbsolutePath());
		        }
            }
		});
		
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
				
				if(validForm && (environmentSoundPathText.getText() == null || environmentSoundPathText.getText().isEmpty())) {
					validForm = false;
					JPanel panel = new JPanel();
					JOptionPane.showMessageDialog(panel,
							 "Invalid Environment Sound File",
							 "Error",
					        JOptionPane.ERROR_MESSAGE);
				}
				
				if (validForm) {
					SGDKEnvironmentSound environmentSound = SGDKEntityFactory.createSGDKEnvironmentSound(environmentSoundPathText.getText(), (SGDKFolder)parentNode);
					parent.getProyectExplorer().getProyectExplorerTree().addElement(environmentSound, parentNode);
					clean();
					parent.setEnabled(true);
					setVisible(false);
				}
			}
		});
		
		setVisible(true);
	}

	protected void clean() {
		environmentSoundPathText.setText("");
	}
}
