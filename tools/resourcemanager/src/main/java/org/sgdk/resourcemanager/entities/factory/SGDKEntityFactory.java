package org.sgdk.resourcemanager.entities.factory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
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

public class SGDKEntityFactory {

	private static final Logger logger = LogManager.getLogger("UILogger");
	private static final String RES_FILE_EXTENSION = ".res";
	
	public static SGDKProject createSGDKProject(String path) {
		logger.debug("Creating Project Element...");
		try {
			logger.debug("Created Project Element");
			return new SGDKProject(path);
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKFolder createSGDKFolder(String path, SGDKFolder parentNode) {
		logger.debug("Creating Folder Element...");
		try {
			File directory = new File(parentNode.getPath()+File.separator+path);
			directory.mkdir();
			logger.debug("Created Folder Element");
			SGDKFolder folder =  new SGDKFolder(directory.getAbsolutePath());
			parentNode.addChild(folder);
			folder.setParent(parentNode);
			return folder;
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKBackground createSGDKBackground(String path, SGDKFolder parentNode) {
		logger.debug("Creating Background Element...");
		try {
			SGDKBackground backgroundOrigen = new SGDKBackground(path);						
			Files.copy(
					Paths.get(backgroundOrigen.getPath()),
					Paths.get(parentNode.getPath()+File.separator+backgroundOrigen.toString()),
					StandardCopyOption.REPLACE_EXISTING);
			
			SGDKBackground backgroundDest = new SGDKBackground(parentNode.getPath()+File.separator+backgroundOrigen.toString());
			logger.debug("Created Background Element");
			parentNode.addChild(backgroundDest);
			backgroundDest.setParent(parentNode);
			return backgroundDest;
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKFXSound createSGDKFXSound(String path, SGDKFolder parentNode) {
		logger.debug("Creating FX Sound Element...");
		try {
			SGDKFXSound fxSoundOrigen = new SGDKFXSound(path);
			SGDKFXSound fxSoundDest = new SGDKFXSound(parentNode.getPath()+File.separator+fxSoundOrigen.toString());							
			Files.copy(
					Paths.get(fxSoundOrigen.getPath()),
					Paths.get(fxSoundDest.getPath()),
					StandardCopyOption.REPLACE_EXISTING);
			
			logger.debug("Created FX Sound Element");
			parentNode.addChild(fxSoundDest);
			fxSoundDest.setParent(parentNode);
			return fxSoundDest;
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKSprite createSGDKSprite(String path, SGDKFolder parentNode) {
		logger.debug("Creating Sprite Element...");
		try {
			SGDKSprite spriteOrigen = new SGDKSprite(path);							
			Files.copy(
					Paths.get(spriteOrigen.getPath()),
					Paths.get(parentNode.getPath()+File.separator+spriteOrigen.toString()),
					StandardCopyOption.REPLACE_EXISTING);
			
			SGDKSprite spriteDest = new SGDKSprite(parentNode.getPath()+File.separator+spriteOrigen.toString());
			logger.debug("Created Sprite Element");
			parentNode.addChild(spriteDest);
			spriteDest.setParent(parentNode);
			return spriteDest;
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static SGDKEnvironmentSound createSGDKEnvironmentSound(String path, SGDKFolder parentNode) {
		logger.debug("Creating Environment Sound Element...");
		try {
			SGDKEnvironmentSound environmentSoundOrigen = new SGDKEnvironmentSound(path);
			SGDKEnvironmentSound environmentSoundDest = new SGDKEnvironmentSound(parentNode.getPath()+File.separator+environmentSoundOrigen.toString());
			//Los sonidos pueden aceptarse VGM o VGZ. En caso de ser VGZ se 
			//descomprimirá previamente para ser un vgm
			switch(SGDKEnvironmentSound.ValidFormat.valueOf(FilenameUtils.getExtension(environmentSoundDest.getPath()).toLowerCase())) {				
			case vgm:				
				Files.copy(
						Paths.get(environmentSoundOrigen.getPath()),
						Paths.get(environmentSoundDest.getPath()),
						StandardCopyOption.REPLACE_EXISTING);
				break;
			case vgz:
				environmentSoundDest.setPath(
					environmentSoundDest.getPath().substring(
							0, environmentSoundDest.getPath().lastIndexOf(".") + 1
					).concat(SGDKEnvironmentSound.ValidFormat.vgm.toString())
				);
				File f = new File(environmentSoundDest.getPath());
				if(f.exists()) {
					f.delete();
				}
				logger.info("Unzipping VGZ File to VGM");
				GZIPInputStream unzipFile = new GZIPInputStream(new FileInputStream(new File(environmentSoundOrigen.getPath())));
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
			logger.debug("Created Environment Sound Element");
			parentNode.addChild(environmentSoundDest);
			environmentSoundDest.setParent(parentNode);
			return environmentSoundDest;
		} catch (SGDKInvalidFormatException e) {
			logger.error(e.getMessage(),  e);
		} catch (IOException e) {
			logger.error(e.getMessage(),  e);
		}
		return null;
	}

	public static void deleteSGDKElement(SGDKElement element) {
		File f = new File(element.getPath());
		f.delete();		
		((SGDKFolder)(element.getParent())).getChilds().remove(element);
	}

	public static void export(SGDKProject project, String path) throws IOException {
		int step = 1;
		logger.debug("Exporting project ...");
		logger.debug("Step "+step+" Creating Project folder " + project.toString());
		File f = new File(path + File.separator + project.toString());
		f.mkdir();
		logger.debug("Step "+step+" Success");		
		step++;
		logger.debug("Step "+step+" Creating res folder");
		f = new File(f.getAbsolutePath() + File.separator + "res");
		f.mkdir();
		logger.debug("Step "+step+" Success");	
		step++;
		logger.debug("Step "+step+" Coping Resources Tree ...");
		StringBuilder sb = new StringBuilder();
		for(SGDKElement child : project.getChilds()) {
			switch(child.getType()) {
			case SGDKBackground:
				sb.append(exportSGDKBackground((SGDKBackground)child, f.getAbsolutePath(), ""));
				break;
			case SGDKEnvironmentSound:
				sb.append(exportSGDKEnvironmentSound((SGDKEnvironmentSound)child, f.getAbsolutePath(), ""));
				break;
			case SGDKFXSound:
				sb.append(exportSGDKFXSound((SGDKFXSound)child, f.getAbsolutePath(), ""));
				break;
			case SGDKFolder:
				StringBuilder sbFolder = new StringBuilder();
				sbFolder.append(exportSGDKFolder((SGDKFolder)child, f.getAbsolutePath(), ""));
				File fresFolder = new File(f.getAbsolutePath()+File.separator+child.toString()+RES_FILE_EXTENSION);
				try(FileWriter writer = new FileWriter(fresFolder)) {
					fresFolder.createNewFile();
					writer.write(sbFolder.toString());
				} catch (IOException e) {
					logger.error("Step "+step+" Error", e);
					throw e;
				}
				break;
			case SGDKSprite:
				sb.append(exportSGDKSprite((SGDKSprite)child, f.getAbsolutePath(), ""));
				break;
			default:
				break;			
			}
		}
		File fmainFolder = new File(f.getAbsolutePath()+File.separator+"main"+RES_FILE_EXTENSION);
		try(FileWriter writer = new FileWriter(fmainFolder)) {
			fmainFolder.createNewFile();
			writer.write(sb.toString());
		} catch (IOException e) {
			logger.error("Step "+step+" Error", e);
			throw e;
		}
		logger.debug("Step "+step+" Success");
		step++;
		logger.debug("Exporting success");
	}
	
	private static StringBuilder exportSGDKBackground(SGDKBackground background, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(background.getPath()),
				Paths.get(resPath+File.separator+background.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("IMAGE " + background.getName() + " \"" + relativePath+background.toString() + "\" " + background.getCompression().toString());		
		return sb;		
	}
	
	private static StringBuilder exportSGDKSprite(SGDKSprite sprite, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(sprite.getPath()),
				Paths.get(resPath+File.separator+sprite.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("SPRITE " + sprite.getName() + " \"" + relativePath+sprite.toString() + "\" " +
				sprite.getWidth() + " " +sprite.getHeigth() + " " +
				sprite.getCompression().toString() + " " +
				sprite.getTime() + " " +
				sprite.getCollision().toString());		
		return sb;	
	}

	private static StringBuilder exportSGDKEnvironmentSound(SGDKEnvironmentSound environmentSound, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(environmentSound.getPath()),
				Paths.get(resPath+File.separator+environmentSound.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("XGM " + environmentSound.getName() + " \"" + relativePath+environmentSound.toString() + "\" " );		
		return sb;
	}

	private static StringBuilder exportSGDKFXSound(SGDKFXSound fxSound, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(fxSound.getPath()),
				Paths.get(resPath+File.separator+fxSound.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("WAV " + fxSound.getName() + " \"" + relativePath+fxSound.toString() + "\" ");		
		return sb;
	}
	
	private static StringBuilder exportSGDKFolder(SGDKFolder folder, String resPath, String relativePath) throws IOException {
		File f = new File(resPath+File.separator+folder.toString());
		f.mkdir();
		relativePath = relativePath + folder.toString() + "/";
		StringBuilder sb = new StringBuilder();
		for(SGDKElement child : folder.getChilds()) {
			switch(child.getType()) {
			case SGDKBackground:
				sb.append(exportSGDKBackground((SGDKBackground)child, f.getAbsolutePath(), relativePath));
				break;
			case SGDKEnvironmentSound:
				sb.append(exportSGDKEnvironmentSound((SGDKEnvironmentSound)child, f.getAbsolutePath(), relativePath));
				break;
			case SGDKFXSound:
				sb.append(exportSGDKFXSound((SGDKFXSound)child, f.getAbsolutePath(), relativePath));
				break;
			case SGDKFolder:
				StringBuilder sbFolder = new StringBuilder();
				sbFolder.append(exportSGDKFolder((SGDKFolder)child, f.getAbsolutePath(), relativePath));
				File fresFolder = new File(f.getAbsolutePath()+File.separator+child.toString()+RES_FILE_EXTENSION);
				try(FileWriter writer = new FileWriter(fresFolder)) {
					fresFolder.createNewFile();
					writer.write(sbFolder.toString());
				} catch (IOException e) {
					throw e;
				}
				break;
			case SGDKSprite:
				sb.append(exportSGDKSprite((SGDKSprite)child, f.getAbsolutePath(), relativePath));
				break;
			default:
				break;			
			}
		}	
		return sb;	
	}
}
