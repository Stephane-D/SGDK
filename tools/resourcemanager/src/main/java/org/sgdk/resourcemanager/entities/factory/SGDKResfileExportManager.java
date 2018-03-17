package org.sgdk.resourcemanager.entities.factory;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKBackground;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.entities.SGDKEnvironmentSound;
import org.sgdk.resourcemanager.entities.SGDKFXSound;
import org.sgdk.resourcemanager.entities.SGDKFolder;
import org.sgdk.resourcemanager.entities.SGDKProject;
import org.sgdk.resourcemanager.entities.SGDKSprite;

public class SGDKResfileExportManager {
	
	private static final Logger logger = LogManager.getLogger("UILogger");
	private static final String RES_FILE_EXTENSION = ".res";

	public static void export(SGDKProject project, String path) throws IOException {
		int step = 1;
		logger.info("Exporting project ...");
		logger.info("Step "+step+" Creating Project folder " + project.toString());
		File f = new File(path + File.separator + project.toString());
		f.mkdir();
		logger.info("Step "+step+" Success");		
		step++;
		logger.info("Step "+step+" Creating res folder");
		f = new File(f.getAbsolutePath() + File.separator + "res");
		f.mkdir();
		logger.info("Step "+step+" Success");	
		step++;
		logger.info("Step "+step+" Coping Resources Tree ...");
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
		logger.info("Step "+step+" Success");
		step++;
		logger.info("Exporting success");
	}
	
	private static StringBuilder exportSGDKBackground(SGDKBackground background, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(background.getPath()),
				Paths.get(resPath+File.separator+background.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("IMAGE " + background.getName() + " \"" + relativePath+background.toString() + "\" " + background.getCompression().toString()+"\n");		
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
				sprite.getCollision().toString()+"\n");		
		return sb;	
	}

	private static StringBuilder exportSGDKEnvironmentSound(SGDKEnvironmentSound environmentSound, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(environmentSound.getPath()),
				Paths.get(resPath+File.separator+environmentSound.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("XGM " + environmentSound.getName() + " \"" + relativePath+environmentSound.toString() + "\"\n" );		
		return sb;
	}

	private static StringBuilder exportSGDKFXSound(SGDKFXSound fxSound, String resPath, String relativePath) throws IOException {
		Files.copy(
				Paths.get(fxSound.getPath()),
				Paths.get(resPath+File.separator+fxSound.toString()),
				StandardCopyOption.REPLACE_EXISTING);
		
		StringBuilder sb = new StringBuilder();
		sb.append("WAV " + fxSound.getName() + " \"" + relativePath+fxSound.toString() + "\"\n");		
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
