package org.sgdk.resourcemanager.ui.panels.components;

import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKElement;

public class ImageComponentsPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public ImageComponentsPanel(){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		
		  for (int i = 0; i < 3; i++) {
            add(new JLabel(" ImgC Label n°" + i));
        }
	}

	public void setComponents(SGDKElement e) {
		clean();
		// TODO Auto-generated method stub
		
	}

	private void clean() {
		// TODO Auto-generated method stub
		
	}

}
