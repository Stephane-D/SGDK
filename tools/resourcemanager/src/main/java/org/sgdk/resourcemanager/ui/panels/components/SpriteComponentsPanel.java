package org.sgdk.resourcemanager.ui.panels.components;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.ui.panels.components.components.CollisionComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.CompressionComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.SpriteDimensionComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.TimeComponent;

public class SpriteComponentsPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private CompressionComponent compressionComponent = new CompressionComponent();
	private TimeComponent timeComponent = new TimeComponent();
	private CollisionComponent collisionComponent = new CollisionComponent();
	private SpriteDimensionComponent spriteDimensionsComponent = new SpriteDimensionComponent();
	
	public SpriteComponentsPanel(){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		add(spriteDimensionsComponent);
		add(compressionComponent);
		add(timeComponent);
		add(collisionComponent);
	}

	public void setComponents(SGDKSprite e) {
		clean();
		compressionComponent.setSGDKBackground(e);
		timeComponent.setSGDKSprite(e);	
		collisionComponent.setSGDKSprite(e);	
		spriteDimensionsComponent.setSGDKSprite(e);	
	}

	private void clean() {
		// TODO Auto-generated method stub
		
	}

}
