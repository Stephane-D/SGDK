package org.sgdk.resourcemanager.entities;

import java.io.IOException;
import java.util.Iterator;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

public class SGDKElementDeserializer extends StdDeserializer<SGDKElement> {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private static final ObjectMapper mapper = new ObjectMapper();

	public SGDKElementDeserializer() { 
        this(null); 
    } 
 
    public SGDKElementDeserializer(Class<?> vc) { 
        super(vc); 
    }
 
	@SuppressWarnings("deprecation")
	@Override
    public SGDKElement deserialize(JsonParser jp, DeserializationContext ctxt) 
      throws IOException, JsonProcessingException {
        JsonNode node = jp.getCodec().readTree(jp);
//        int id = (Integer) ((IntNode) node.get("id")).numberValue();
        String path = node.get("path").asText();
        SGDKElement.Type type = SGDKElement.Type.valueOf(node.get("type").asText());        
        
        SGDKElement element = null;
        
        try {
        	switch (type) {
    		case SGDKBackground:
    			SGDKBackground b = new SGDKBackground(path);
    			element = b;
    			break;
    		case SGDKSprite:
    			SGDKSprite s = new SGDKSprite(path);
    			element = s;
    			break;
    		case SGDKFXSound:
    			SGDKFXSound fx = new SGDKFXSound(path);
    			element = fx;
    			break;
    		case SGDKEnvironmentSound:
    			SGDKEnvironmentSound environmentSound = new SGDKEnvironmentSound(path);
    			element = environmentSound;
    			break;
    		case SGDKProject:
    			SGDKProject p = new SGDKProject(path);
    			Iterator<JsonNode> childsProjectIterator = node.get("childs").elements();
    			while(childsProjectIterator.hasNext()) {
    				JsonNode childNode = childsProjectIterator.next();
    				SGDKElement e = mapper.readValue(childNode.toString(), SGDKElement.class);
    				e.setParent(p);
    				p.addChild(e);
    			}
    			element = p;
    			break;
    		case SGDKFolder:
    			SGDKFolder f = new SGDKFolder(path);
    			Iterator<JsonNode> childsFolderIterator = node.get("childs").elements();
    			while(childsFolderIterator.hasNext()) {
    				JsonNode childNode = childsFolderIterator.next();
    				SGDKElement e = mapper.readValue(childNode.toString(), SGDKElement.class);
    				e.setParent(f);
    				f.addChild(e);
    			}
    			element = f;
    			break;
    		}
        }catch (Exception e) {
			throw new JsonGenerationException(e.getMessage());
		} 
        return element;
    }

}
