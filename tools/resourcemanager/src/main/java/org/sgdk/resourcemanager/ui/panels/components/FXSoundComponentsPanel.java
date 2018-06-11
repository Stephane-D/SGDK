package org.sgdk.resourcemanager.ui.panels.components;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKFXSound;
import org.sgdk.resourcemanager.ui.panels.components.components.DriverComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.OutrateComponent;

public class FXSoundComponentsPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private DriverComponent driverComponent;
	private OutrateComponent outrateComponent = new OutrateComponent();
	
	public FXSoundComponentsPanel(){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		
		driverComponent = new DriverComponent(outrateComponent);
		add(driverComponent);
		add(outrateComponent);
	}

	public void setComponents(SGDKFXSound e) {
		clean();
		driverComponent.setSGDKFXSound(e);
	}

	private void clean() {
		// TODO Auto-generated method stub
		
	}

}
