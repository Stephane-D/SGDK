package org.sgdk.resourcemanager.ui.panels.components;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;
import org.sgdk.resourcemanager.ui.panels.components.components.OptionsComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.TimingComponent;

public class EnvironmentSoundComponentsPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private TimingComponent timingComponent = new TimingComponent();
	private OptionsComponent optionsComponent = new OptionsComponent();
	
	public EnvironmentSoundComponentsPanel(){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		
		add(timingComponent);
		add(optionsComponent);
	}

	public void setComponents(SGDKEnvironmentSound e) {
		clean();
		timingComponent.setSGDKEnvironmentSound(e);
		optionsComponent.setSGDKEnvironmentSound(e);
	}

	private void clean() {
		// TODO Auto-generated method stub
		
	}

}
