package org.sgdk.resourcemanager.entities.factory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.util.zip.GZIPInputStream;

import org.apache.commons.io.FilenameUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;
import org.sgdk.resourcemanager.entities.SGDKFXSound;
import org.sgdk.resourcemanager.entities.SGDKFolder;
import org.sgdk.resourcemanager.entities.SGDKProject;
import org.sgdk.resourcemanager.entities.SGDKSprite;
import org.sgdk.resourcemanager.entities.exceptions.SGDKInvalidFormatException;
import org.sgdk.resourcemanager.ui.utils.indexedimage.ImageUtil;

public class SGDKEntityFactory {

	private static final Logger logger = LogManager.getLogger("UILogger");
	
	public static SGDKProject createSGDKProject(String path) {
		logger.info("Creating Project Element...");
		try {
			logger.info("Created Project Element");
			return new SGDKProject(path);
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKFolder createSGDKFolder(String path, SGDKFolder parentNode) {
		logger.info("Creating Folder Element...");
		try {
			File directory = new File(parentNode.getPath()+File.separator+path);
			directory.mkdir();
			logger.info("Created Folder Element");
			SGDKFolder folder =  new SGDKFolder(directory.getAbsolutePath());
			parentNode.addChild(folder);
			folder.setParent(parentNode);
			return folder;
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKBackground createSGDKBackground(String path, SGDKFolder parentNode) {
		logger.info("Creating Background Element...");
		SGDKBackground backgroundDest = null;
		String pathDest = parentNode.getPath()+File.separator+SGDKElement.toString(path);
		try {					
			Files.copy(
					Paths.get(path),
					Paths.get(pathDest),
					StandardCopyOption.REPLACE_EXISTING);
			
			ImageUtil.validateAndCreateIndexedImage(pathDest);
			backgroundDest = new SGDKBackground(pathDest);				
		} catch (SGDKInvalidFormatException|IOException e) {
			logger.error(e.getMessage(),  e);
			File f = new File(pathDest);
			f.delete();
		}
		if (backgroundDest != null) {
			logger.info("Created Background Element");
			parentNode.addChild(backgroundDest);
			backgroundDest.setParent(parentNode);
		}
		return backgroundDest;
	}

	public static SGDKFXSound createSGDKFXSound(String path, SGDKFolder parentNode) {
		logger.info("Creating FX Sound Element...");
		String pathDest = parentNode.getPath()+File.separator+SGDKElement.toString(path);
		SGDKFXSound fxSoundDest = null;
		try {
			Files.copy(
					Paths.get(path),
					Paths.get(pathDest),
					StandardCopyOption.REPLACE_EXISTING);
			fxSoundDest = new SGDKFXSound(pathDest);	
		} catch (SGDKInvalidFormatException|IOException e) {
			logger.error(e.getMessage(),  e);
			File f = new File(pathDest);
			f.delete();
		}
		if (fxSoundDest != null) {
			logger.info("Created FX Sound Element");
			parentNode.addChild(fxSoundDest);
			fxSoundDest.setParent(parentNode);
		}
		return fxSoundDest;
	}

	public static SGDKSprite createSGDKSprite(String path, SGDKFolder parentNode) {
		logger.info("Creating Sprite Element...");
		SGDKSprite spriteDest = null;
		String pathDest = parentNode.getPath()+File.separator+SGDKElement.toString(path);
		try {
			Files.copy(
					Paths.get(path),
					Paths.get(pathDest),
					StandardCopyOption.REPLACE_EXISTING);
			
			ImageUtil.validateAndCreateIndexedImage(pathDest);
			spriteDest = new SGDKSprite(pathDest);						
		} catch (SGDKInvalidFormatException|IOException e) {
			logger.error(e.getMessage(),  e);
			File f = new File(pathDest);
			f.delete();
		}
		if (spriteDest != null) {
			logger.info("Created Sprite Element");
			parentNode.addChild(spriteDest);
			spriteDest.setParent(parentNode);
		}
		return spriteDest;
	}

	public static SGDKEnvironmentSound createSGDKEnvironmentSound(String path, SGDKFolder parentNode) {
		logger.info("Creating Environment Sound Element...");
		String pathDest = parentNode.getPath()+File.separator+SGDKElement.toString(path);
		SGDKEnvironmentSound environmentSoundDest = null;
		try {			
			//Los sonidos pueden aceptarse VGM o VGZ. En caso de ser VGZ se 
			//descomprimirá previamente para ser un vgm
			switch(SGDKEnvironmentSound.ValidFormat.valueOf(FilenameUtils.getExtension(pathDest).toLowerCase())) {				
			case vgm:				
				Files.copy(
						Paths.get(path),
						Paths.get(pathDest),
						StandardCopyOption.REPLACE_EXISTING);
				break;
			case vgz:
				pathDest = pathDest.substring(
						0, pathDest.lastIndexOf(".") + 1
					).concat(SGDKEnvironmentSound.ValidFormat.vgm.toString());
				
				File f = new File(pathDest);
				if(f.exists()) {
					f.delete();
				}
				logger.info("Unzipping VGZ File to VGM");
				GZIPInputStream unzipFile = new GZIPInputStream(new FileInputStream(new File(path)));
				FileOutputStream fos = new FileOutputStream(f);
				byte[] buf = new byte[8000];
				int bytesReaded = 0;
				do{
					bytesReaded = unzipFile.read(buf , 0, buf.length);
					if(bytesReaded > 0) {						
						fos.write(buf, 0, bytesReaded);
					}
				}while(bytesReaded > 0);					
				fos.close();
				unzipFile.close();
				break;
			}
			environmentSoundDest = new SGDKEnvironmentSound(pathDest);			
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		if (environmentSoundDest != null) {
			logger.info("Created Environment Sound Element");
			parentNode.addChild(environmentSoundDest);
			environmentSoundDest.setParent(parentNode);
		}
		return environmentSoundDest;
	}

	public static void deleteSGDKElement(SGDKElement element) {
		File f = new File(element.getPath());
		f.delete();		
		if(element.getParent() != null && ((SGDKFolder)(element.getParent())).getChilds() != null) {			
			((SGDKFolder)(element.getParent())).getChilds().remove(element);
		}
	}
}
