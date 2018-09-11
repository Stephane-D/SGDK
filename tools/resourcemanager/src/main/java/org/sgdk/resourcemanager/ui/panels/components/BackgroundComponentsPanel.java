package org.sgdk.resourcemanager.ui.panels.components;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.ui.panels.components.components.CompressionComponent;

public class BackgroundComponentsPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private CompressionComponent compressionComponent = new CompressionComponent();
	
	public BackgroundComponentsPanel(){
		super();		
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		
		add(compressionComponent);
	}

	public void setComponents(SGDKBackground e) {
		clean();
		compressionComponent.setSGDKBackground(e);
		
	}

	private void clean() {
		// TODO Auto-generated method stub
		
	}

}
