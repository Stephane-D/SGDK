package org.sgdk.resourcemanager.ui.panels.proyectexplorer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenu;
import javax.swing.JMenuItem;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals.AddFolderDialog;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals.CreateBackgroundDialog;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals.CreateEnvironmentDialog;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals.CreateSFXDialog;
import org.sgdk.resourcemanager.ui.panels.proyectexplorer.modals.CreateSpriteDialog;

public class FolderPopupMenu extends FilePopupMenu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private CreateBackgroundDialog createBackgroundDialog = null;
	private CreateSpriteDialog createSpriteDialog = null;
	private CreateEnvironmentDialog createEnvironmentDialog = null;
	private CreateSFXDialog createSFXDialog = null;
	private AddFolderDialog addFolderDialog = null;
	
	public FolderPopupMenu(ResourceManagerFrame parent) {
		super(parent);
		JMenu addResource = new JMenu("Add Resource");
		JMenuItem addFolder = new JMenuItem("Create Folder");
		JMenu image = new JMenu("Image");
		JMenuItem bg = new JMenuItem("Background");
		JMenuItem sprite = new JMenuItem("Sprite");
		bg.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(createBackgroundDialog == null) {
					createBackgroundDialog = new CreateBackgroundDialog(parent, getParentNode());
				}else {
					createBackgroundDialog.setVisible(true);
				}
			}
		});
		sprite.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(createSpriteDialog == null) {
					createSpriteDialog = new CreateSpriteDialog(parent, getParentNode());
				}else {
					createSpriteDialog.setVisible(true);
				}
			}
		});
		image.add(bg);
		image.add(sprite);
		JMenu sound = new JMenu("Sound");
		JMenuItem environment = new JMenuItem("Environment");
		JMenuItem sfx = new JMenuItem("sfx");
		environment.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(createEnvironmentDialog == null) {
					createEnvironmentDialog = new CreateEnvironmentDialog(parent, getParentNode());
				}else {
					createEnvironmentDialog.setVisible(true);
				}
			}
		});
		sfx.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(createSFXDialog == null) {
					createSFXDialog = new CreateSFXDialog(parent, getParentNode());
				}else {
					createSFXDialog.setVisible(true);
				}
			}
		});
		sound.add(environment);
		sound.add(sfx);
		addResource.add(image);
		addResource.add(sound);
		addFolder.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(addFolderDialog == null) {
					addFolderDialog = new AddFolderDialog(parent, getParentNode());
				}else {
					addFolderDialog.setVisible(true);
				}
			}
		});
		add(addResource);
		addSeparator();
		add(addFolder);
	}
}
