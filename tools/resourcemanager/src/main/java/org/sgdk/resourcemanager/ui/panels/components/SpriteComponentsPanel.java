package org.sgdk.resourcemanager.ui.panels.components;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.ui.panels.components.components.CollisionComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.CompressionComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.SpriteDimensionComponent;
import org.sgdk.resourcemanager.ui.panels.components.components.TimeComponent;
import org.sgdk.resourcemanager.ui.panels.preview.PreviewContainerPanel;

public class SpriteComponentsPanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private CompressionComponent compressionComponent = null;
	private TimeComponent timeComponent = null;
	private CollisionComponent collisionComponent = null;
	private SpriteDimensionComponent spriteDimensionsComponent = null;
	
	public SpriteComponentsPanel(PreviewContainerPanel previewContainerPanel){
		super();
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		
		compressionComponent = new CompressionComponent();
		timeComponent = new TimeComponent();
		collisionComponent = new CollisionComponent();
		spriteDimensionsComponent = new SpriteDimensionComponent(previewContainerPanel);
		
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
