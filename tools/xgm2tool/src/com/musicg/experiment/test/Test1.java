package com.musicg.experiment.test;

import com.musicg.graphic.GraphicRender;
import com.musicg.math.rank.ArrayRankDouble;
import com.musicg.math.statistics.SpectralCentroid;
import com.musicg.math.statistics.StandardDeviation;
import com.musicg.pitch.PitchHandler;
import com.musicg.wave.Wave;
import com.musicg.wave.extension.Spectrogram;

public class Test1 {

	public static void main(String[] args) {		
		
		String filename = "audio_work/lala.wav";

		// create a wave object
		Wave wave = new Wave(filename);

		// TimeDomainRepresentations
		int fftSampleSize=1024;
		int overlapFactor=1;
		Spectrogram spectrogram=new Spectrogram(wave,fftSampleSize,overlapFactor);
		
		int fps=spectrogram.getFramesPerSecond();
		double unitFrequency=spectrogram.getUnitFrequency();

		// set boundary
		int highPass=100;
		int lowerBoundary=(int)(highPass/unitFrequency);
		int lowPass=4000;
		int upperBoundary=(int)(lowPass/unitFrequency);
		// end set boundary
		
		double[][] spectrogramData=spectrogram.getNormalizedSpectrogramData();
		double[][] absoluteSpectrogramData=spectrogram.getAbsoluteSpectrogramData();
		double[][] boundedSpectrogramData=new double[spectrogramData.length][];
		
		SpectralCentroid sc=new SpectralCentroid();
		StandardDeviation sd=new StandardDeviation();		
		ArrayRankDouble arrayRankDouble=new ArrayRankDouble();
		
		// zrc
		short[] amps=wave.getSampleAmplitudes();
		int numFrame=amps.length/1024;
		double[] zcrs=new double[numFrame];
		
		for (int i=0; i<numFrame; i++){
			short[] temp=new short[1024];
			System.arraycopy(amps, i*1024, temp, 0, temp.length);
			
			int numZC=0;
			int size=temp.length;
			
			for (int j=0; j<size-1; j++){
				if((temp[j]>=0 && temp[j+1]<0) || (temp[j]<0 && temp[j+1]>=0)){
					numZC++;
				}
			}	
			
			zcrs[i]=numZC;
		}
		
		// end zcr
		
		for (int i=0; i<spectrogramData.length; i++){
			double[] temp=new double[upperBoundary-lowerBoundary+1];
			System.arraycopy(spectrogramData[i], lowerBoundary, temp, 0, temp.length);			
			
			int maxIndex=arrayRankDouble.getMaxValueIndex(temp);			
			//sc.setValues(temp);
			sd.setValues(temp);			
			double sdValue=sd.evaluate();
			
			System.out.println(i+" "+(double)i/fps+"s\t"+maxIndex+"\t"+sdValue+"\t"+zcrs[i]);
			boundedSpectrogramData[i]=temp;
		}
		
		// Graphic render		
		GraphicRender render=new GraphicRender();
		render.setHorizontalMarker(61);
		render.setVerticalMarker(200);
		render.renderSpectrogramData(boundedSpectrogramData, filename+".jpg");
		
		PitchHandler ph=new PitchHandler();

		for (int frame=0; frame<absoluteSpectrogramData.length; frame++){
			
			System.out.print("frame "+frame+": ");
						
			double[] temp=new double[upperBoundary-lowerBoundary+1];
			sd.setValues(temp);
			double sdValue=sd.evaluate();
			double passSd=0.1;
			
			if (sdValue<passSd){
				System.arraycopy(spectrogramData[frame], lowerBoundary, temp, 0, temp.length);
				double maxFrequency=arrayRankDouble.getMaxValueIndex(temp)*unitFrequency;
				
				double passFrequency=400;
				int numRobust=2;
				
				double[] robustFrequencies=new double[numRobust];
				double nthValue=arrayRankDouble.getNthOrderedValue(temp, numRobust, false);
				int count=0;
				for (int b=lowerBoundary; b<=upperBoundary; b++){
					if (spectrogramData[frame][b]>=nthValue){
						robustFrequencies[count++]=b*unitFrequency;
						if (count>=numRobust){
							break;
						}
					}
				}
				
				double passIntensity=1000;
				double intensity=0;
				for (int i=0; i<absoluteSpectrogramData[frame].length; i++){
					intensity+=absoluteSpectrogramData[frame][i];
				}
				intensity/=absoluteSpectrogramData[frame].length;
				System.out.print(" intensity: "+intensity+" pitch: "+maxFrequency);
				if (intensity>passIntensity && maxFrequency>passFrequency){				
					double p=ph.getHarmonicProbability(robustFrequencies);				
					System.out.print(" P: "+p);
				}
			}
			System.out.print(" zcr:"+zcrs[frame]);
			System.out.println();
		}
	}
}