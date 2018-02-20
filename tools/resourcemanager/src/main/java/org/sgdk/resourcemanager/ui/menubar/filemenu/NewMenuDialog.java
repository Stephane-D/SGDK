package org.sgdk.resourcemanager.ui.menubar.filemenu;

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

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.sgdk.resourcemanager.entities.SGDKProyect;
import org.sgdk.resourcemanager.entities.factory.SGDKEntityFactory;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class NewMenuDialog extends JDialog{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private static final int minimizeWidth = 340;
	private static final int minimizeHeight = 220;
	
	private JTextField proyectPathText = new JTextField();
	private JFileChooser proyectPath = new JFileChooser(System.getProperty("user.home"));
	private JButton acceptButon = new JButton("Ok");
	
	public NewMenuDialog(ResourceManagerFrame parent) {
		super(parent, "New Proyect");
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
		
//		proyectPath.addChoosableFileFilter(new SGDKFileFilter());
		proyectPath.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
		proyectPath.setAcceptAllFileFilterUsed(false);
		
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
		add(new JLabel("Proyect Path: "), c);
		
		c.fill = GridBagConstraints.HORIZONTAL;	
		c.anchor = GridBagConstraints.LINE_START;
		c.weightx = 5d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 1;
		c.gridy = 0;
		c.gridwidth = 5;
		c.gridheight = 1;
		add(proyectPathText, c);
		
		proyectPathText.addMouseListener(new MouseAdapter() {
			@Override
            public void mouseClicked(MouseEvent e){				
				int returnVal = proyectPath.showDialog(parent, "New Proyect");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					proyectPathText.setText(proyectPath.getSelectedFile().getAbsolutePath());
		        }
            }
		});
		
		c.fill = GridBagConstraints.NONE;
		c.anchor = GridBagConstraints.LINE_END;
		c.weightx = 1d/6d;
		c.weighty = 1d/2d;		
		c.gridx = 5;
		c.gridy = 1;
		c.gridwidth = 1;
		c.gridheight = 1;
		add(acceptButon, c);
		
		acceptButon.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean validForm = true;
				
				if(validForm && (proyectPathText.getText() == null || proyectPathText.getText().isEmpty())) {
					validForm = false;
					JPanel panel = new JPanel();
					JOptionPane.showMessageDialog(panel,
							 "Invalid Proyect Path",
							 "Error",
					        JOptionPane.ERROR_MESSAGE);
				}
				
				if (validForm) {
					SGDKProyect proyect = SGDKEntityFactory.createSGDKProyect(proyectPathText.getText());
					parent.getProyectExplorer().getProyectExplorerTree().addElement(proyect, null);
					clean();
					parent.setEnabled(true);
					setVisible(false);
				}
			}
		});
		
		setVisible(true);
	}

	protected void clean() {
		proyectPathText.setText("");
	}

}
