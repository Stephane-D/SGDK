package org.sgdk.resourcemanager.ui.panels.preview;

import java.io.File;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.SourceDataLine;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.sgdk.resourcemanager.entities.SGDKElement;
import org.sgdk.resourcemanager.ui.utils.vgm.VGMPlayer;

public class SoundPlayer {
	
	private static final Logger logger = LogManager.getLogger("UILogger");

	private static final int BUFFER_SIZE = 8192;
	public static final int SEGA_SOUND_RATE = 44100;
	
	
	private File soundFile;
	private AudioInputStream audioStream;
	private AudioFormat audioFormat;
	private SourceDataLine sourceLine;
	private Thread threadSoundFx;
	
	private VGMPlayer vgmPlayer;	
	private Thread threadSoundVGM;
	
	private Boolean isRunning = false;
	private Boolean waitingToPlay = false;
	
	private SGDKElement sgdkElement;
	private boolean paused = false;

	public void playSound(SGDKElement sgdkElement) throws Exception {
		isPaused(false);
		this.sgdkElement = sgdkElement;
		switch (sgdkElement.getType()) {
		case SGDKEnvironmentSound:
			vgmPlayer = new VGMPlayer(SEGA_SOUND_RATE);
			playVGMSound(sgdkElement);
			break;
		case SGDKFXSound:
			vgmPlayer = null;
			playFXSound(sgdkElement);
			break;
		default:
			throw new Exception("Invalid SGDKElement Type");
		}
	}

	private void playFXSound(SGDKElement sgdkElement) throws InterruptedException {
		SoundPlayer that = this;
		if ((threadSoundFx != null && threadSoundFx.isAlive()) || (threadSoundVGM != null && threadSoundVGM.isAlive())) {
			thereAreWaitingToPlay(true);
			synchronized (that) {
				wait();
			}
		}
		if (!isRunning()) {
			thereAreWaitingToPlay(false);
			isRunning(true);
			threadSoundFx = new Thread(new Runnable() {

				@Override
				public void run() {
					try {
						logger.info("Playing fx sound ...");
						String strFilename = sgdkElement.getPath();
						soundFile = new File(strFilename);
						audioStream = AudioSystem.getAudioInputStream(soundFile);

						audioFormat = audioStream.getFormat();

						DataLine.Info info = new DataLine.Info(SourceDataLine.class, audioFormat);

						sourceLine = (SourceDataLine) AudioSystem.getLine(info);
						sourceLine.open(audioFormat);

						sourceLine.start();

						int nBytesRead = 0;
						byte[] abData = new byte[BUFFER_SIZE];
						while (nBytesRead != -1 && isRunning()) {
							if(!isPaused()) {								
								nBytesRead = audioStream.read(abData, 0, abData.length);
								if (nBytesRead >= 0) {
									sourceLine.write(abData, 0, nBytesRead);
								}
							}
						}

						sourceLine.drain();
						sourceLine.close();
					} catch (Exception e) {
					}
					logger.info("Finish fx sound");
					if (thereAreWaitingToPlay()) {
						synchronized (that) {
							that.notifyAll();
						}
					}
				}
			});
			threadSoundFx.start();
		}
	}

	private void playVGMSound(SGDKElement sgdkElement) throws InterruptedException {
		SoundPlayer that = this;
		if ((threadSoundFx != null && threadSoundFx.isAlive()) || (threadSoundVGM != null && threadSoundVGM.isAlive())) {
			thereAreWaitingToPlay(true);
			synchronized (that) {
				wait();
			}
		}
		if (!isRunning()) {
			thereAreWaitingToPlay(false);
			isRunning(true);
			threadSoundVGM = new Thread(new Runnable() {

				@Override
				public void run() {
					try {
						logger.info("Playing environmet sound ...");
//						Path pathFile = Paths.get(sgdkElement.getPath());
						vgmPlayer.loadFile(new File(sgdkElement.getPath()).toURI().toURL(), sgdkElement.getPath());
						vgmPlayer.startTrack(0, -1);			
					} catch (Exception e) {
						e.printStackTrace();
					}
					if (thereAreWaitingToPlay()) {
						synchronized (that) {
							that.notifyAll();
						}
					}
				}
			});
			threadSoundVGM.start();
		}
	}
	
	synchronized private void thereAreWaitingToPlay(boolean b) {
		waitingToPlay = b;
	}

	synchronized private boolean thereAreWaitingToPlay() {
		return waitingToPlay;
	}

	public void stopSound() throws Exception {
		isRunning(false);
		if(vgmPlayer != null) {
			vgmPlayer.stop();
			vgmPlayer = null;
		}
	}

	synchronized public Boolean isRunning() {
		return isRunning;
	}

	synchronized public void isRunning(Boolean isRunning) {
		this.isRunning = isRunning;
	}

	public void playSound() throws Exception {
		playSound(sgdkElement);
	}

	synchronized public boolean isPaused() {
		return paused ;
	}

	synchronized public void isPaused(boolean paused) throws Exception {
		if(vgmPlayer != null) {
			if(paused && vgmPlayer.isPlaying()) {
				vgmPlayer.pause();
			}else {
				vgmPlayer.play();
			}
		}
		this.paused = paused;
	}
}
