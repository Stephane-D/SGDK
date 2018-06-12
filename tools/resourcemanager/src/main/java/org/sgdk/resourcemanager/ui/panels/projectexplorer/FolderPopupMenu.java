package org.sgdk.resourcemanager.ui.panels.projectexplorer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.JFileChooser;
import javax.swing.JMenu;
import javax.swing.JMenuItem;

import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;
import org.sgdk.resourcemanager.entities.SGDKFXSound;
import org.sgdk.resourcemanager.entities.SGDKFolder;
import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.entities.factory.SGDKEntityFactory;
import org.sgdk.resourcemanager.ui.ResourceManagerFrame;
import org.sgdk.resourcemanager.ui.filechooser.BackgroundFileChooser;
import org.sgdk.resourcemanager.ui.filechooser.EnvironmentFileChooser;
import org.sgdk.resourcemanager.ui.filechooser.SFXFileChooser;
import org.sgdk.resourcemanager.ui.filechooser.SpriteFileChooser;
import org.sgdk.resourcemanager.ui.panels.projectexplorer.modals.AddFolderDialog;

public class FolderPopupMenu extends FilePopupMenu {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	private BackgroundFileChooser backgroundFileChooser = null;
	private SpriteFileChooser spriteFileChooser = null;
	private EnvironmentFileChooser environmentFileChooser = null;
	private SFXFileChooser sFXFileChooser = null;
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
				if(backgroundFileChooser == null) {
					backgroundFileChooser = new BackgroundFileChooser();
				}
				int returnVal = backgroundFileChooser.showDialog(parent, "New Background");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					for(File f : backgroundFileChooser.getSelectedFiles()) {
						SGDKBackground background = SGDKEntityFactory.createSGDKSprite(f.getAbsolutePath(), (SGDKFolder)getParentNode());
						parent.getProjectExplorer().getProjectExplorerTree().addElement(background,getParentNode());
					}
		        }
			}
		});
		sprite.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(spriteFileChooser == null) {
					spriteFileChooser = new SpriteFileChooser();
				}
				int returnVal = spriteFileChooser.showDialog(parent, "New Sprite");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					for(File f : spriteFileChooser.getSelectedFiles()) {
						SGDKSprite sprite = SGDKEntityFactory.createSGDKSprite(f.getAbsolutePath(), (SGDKFolder)getParentNode());
						parent.getProjectExplorer().getProjectExplorerTree().addElement(sprite,getParentNode());
					}
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
				if(environmentFileChooser == null) {
					environmentFileChooser = new EnvironmentFileChooser();
				}
				int returnVal = environmentFileChooser.showDialog(parent, "New Environment Sound");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					for(File f : environmentFileChooser.getSelectedFiles()) {
						SGDKEnvironmentSound environmentSound = SGDKEntityFactory.createSGDKEnvironmentSound(f.getAbsolutePath(), (SGDKFolder)getParentNode());
						parent.getProjectExplorer().getProjectExplorerTree().addElement(environmentSound,getParentNode());
					}
		        }
			}
		});
		sfx.addActionListener(new ActionListener() {			
			@Override
			public void actionPerformed(ActionEvent e) {
				if(sFXFileChooser == null) {
					sFXFileChooser = new SFXFileChooser();
				}
				int returnVal = sFXFileChooser.showDialog(parent, "New FX Sound");
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					for(File f : sFXFileChooser.getSelectedFiles()) {
						SGDKFXSound fxSound = SGDKEntityFactory.createSGDKFXSound(f.getAbsolutePath(), (SGDKFolder)getParentNode());
						parent.getProjectExplorer().getProjectExplorerTree().addElement(fxSound,getParentNode());
					}
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
