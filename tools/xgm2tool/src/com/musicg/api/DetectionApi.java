/*
 * Copyright (C) 2011 Jacquet Wong
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.musicg.api;

import com.musicg.math.rank.ArrayRankDouble;
import com.musicg.math.statistics.StandardDeviation;
import com.musicg.math.statistics.ZeroCrossingRate;
import com.musicg.wave.Wave;
import com.musicg.wave.WaveHeader;
import com.musicg.wave.extension.Spectrogram;

/**
 * Api for detecting different sounds
 *
 * @author Jacquet Wong
 *
 */
public class DetectionApi {

	protected WaveHeader waveHeader;
	protected int fftSampleSize;
	protected int numFrequencyUnit;
	protected double unitFrequency;
	protected double minFrequency, maxFrequency;
	protected double minIntensity, maxIntensity;
	protected double minStandardDeviation, maxStandardDeviation;
	protected int highPass, lowPass;
	protected int minNumZeroCross, maxNumZeroCross;
	protected int lowerBoundary, upperBoundary;
	protected int numRobust;

	/**
	 * Constructor, support mono Wav only, 4096 sample byte size for 44100Hz
	 * 16bit mono wav
	 *
	 * @param sampleRate
	 *            Sample rate of the input audio byte
	 * @param bitsPerSample
	 *            Bit size of a sample of the input audio byte
	 */
	public DetectionApi(WaveHeader waveHeader) {
		if (waveHeader.getChannels() == 1) {
			this.waveHeader = waveHeader;
			init();
		} else {
			System.err.println("DetectionAPI supports mono Wav only");
		}
	}

	/**
	 * Initiate the settings for specific sound detection
	 */
	protected void init(){
		// do nothing, needed to be overrided
	}

	/**
	 * Determine the audio bytes contains a specific sound or not
	 *
	 * @param audioBytes
	 *            input audio byte
	 * @return
	 */
	public boolean isSpecificSound(byte[] audioBytes) {

		int bytesPerSample = waveHeader.getBitsPerSample() / 8;
		int numSamples = audioBytes.length / bytesPerSample;

		// numSamples required to be a power of 2
		if (numSamples <= 0 || Integer.bitCount(numSamples) != 1) {
			System.out.println("The sample size must be a power of 2");
			return false;
		}

		fftSampleSize = numSamples;
		numFrequencyUnit = fftSampleSize / 2;

		// frequency could be caught within the half of nSamples according to Nyquist theory
		unitFrequency = (double) waveHeader.getSampleRate() / 2 / numFrequencyUnit;

		// set boundary
		lowerBoundary = (int) (highPass / unitFrequency);
		upperBoundary = (int) (lowPass / unitFrequency);
		// end set boundary

		Wave wave = new Wave(waveHeader, audioBytes);	// audio bytes of this frame
		short[] amplitudes = wave.getSampleAmplitudes();

		// spectrum for the clip
		Spectrogram spectrogram = wave.getSpectrogram(fftSampleSize, 0);

		double[][] spectrogramData = spectrogram.getAbsoluteSpectrogramData();

		// since fftSampleSize==numSamples, there're only one spectrum which is thisFrameSpectrogramData[0]
		double[] spectrum = spectrogramData[0];

		int frequencyUnitRange = upperBoundary - lowerBoundary + 1;
		if (spectrum.length < frequencyUnitRange) {
			System.err.println("is error: the wave needed to be higher sample rate");
			return false;
		}

		double[] rangedSpectrum = new double[frequencyUnitRange];
		System.arraycopy(spectrum, lowerBoundary, rangedSpectrum, 0, rangedSpectrum.length);

		/*
		// run all checking for debug
		boolean isPassedChecking = true;
		// rule 1: check the intensity of this frame
		isPassedChecking &= isPassedIntensity(spectrum);
		// rule 2: check the frequency of this frame
		isPassedChecking &= isPassedFrequency(rangedSpectrum);
		// rule 3: check the zero crossing rate of this frame
		isPassedChecking &= isPassedZeroCrossingRate(amplitudes);
		// rule 4: check the standard deviation of this frame with reference of previous frames
		isPassedChecking &= isPassedStandardDeviation(spectrogramData);
		System.out.println("Result: " + isPassedChecking + "\n");
		return isPassedChecking;
		// end run all checking for debug
		*/

		return isPassedIntensity(spectrum) &&
				isPassedStandardDeviation(spectrogramData) &&
				isPassedZeroCrossingRate(amplitudes) &&
				isPassedFrequency(rangedSpectrum);
	}

	protected void normalizeSpectrogramData(double[][] spectrogramData) {

		// normalization of absoultSpectrogram
		// set max and min amplitudes
		double maxAmp = Double.MIN_VALUE;
		double minAmp = Double.MAX_VALUE;
		for (int i = 0; i < spectrogramData.length; i++) {
			for (int j = 0; j < spectrogramData[i].length; j++){
				if (spectrogramData[i][j] > maxAmp) {
					maxAmp = spectrogramData[i][j];
				} else if (spectrogramData[i][j] < minAmp) {
					minAmp = spectrogramData[i][j];
				}
			}
		}
		// end set max and min amplitudes

		// normalization
		// avoiding divided by zero
		double minValidAmp = 0.00000000001F;
		if (minAmp == 0) {
			minAmp = minValidAmp;
		}

		double diff = Math.log10(maxAmp / minAmp); // perceptual difference
		for (int i = 0; i < spectrogramData.length; i++) {
			for (int j = 0; j < spectrogramData[i].length; j++) {
				if (spectrogramData[i][j] < minValidAmp) {
					spectrogramData[i][j] = 0;
				} else {
					spectrogramData[i][j] = (Math.log10(spectrogramData[i][j] / minAmp)) / diff;
				}
			}
		}
		// end normalization
	}

	protected boolean isPassedStandardDeviation(double[][] spectrogramData){

		// normalize the spectrogramData (with all frames in the spectrogram)
		normalizeSpectrogramData(spectrogramData);

		// analyst data in this frame
		// since fftSampleSize==numSamples, there're only one spectrum which is spectrogramData[last]
		double[] spectrum = spectrogramData[spectrogramData.length - 1];
		// find top most robust frequencies in this frame
		double[] robustFrequencies = new double[numRobust];
		ArrayRankDouble arrayRankDouble = new ArrayRankDouble();
		double nthValue = arrayRankDouble.getNthOrderedValue(spectrum, numRobust, false);
		// end analyst data in this frame

		int count = 0;
		for (int i = 0; i < spectrum.length; i++) {
			if (spectrum[i] >= nthValue) {
				robustFrequencies[count++] = spectrum[i];
				if (count >= numRobust) {
					break;
				}
			}
		}
		// end find top most robust frequencies

		StandardDeviation standardDeviation = new StandardDeviation();
		standardDeviation.setValues(robustFrequencies);
		double sd = standardDeviation.evaluate();

		// range of standard deviation
		boolean result = (sd >= minStandardDeviation && sd <= maxStandardDeviation);
		//System.out.println("sd: " + sd + " " + result);
		return result;
	}

	protected boolean isPassedFrequency(double[] spectrum){
		// find the robust frequency
		ArrayRankDouble arrayRankDouble = new ArrayRankDouble();
		double robustFrequency = arrayRankDouble.getMaxValueIndex(spectrum) * unitFrequency;

		// frequency of the sound should not be too low or too high
		boolean result = (robustFrequency >= minFrequency && robustFrequency <= maxFrequency);
		//System.out.println("freq: " + robustFrequency + " " + result);
		return result;
	}

	protected boolean isPassedIntensity(double[] spectrum){
		// get the average intensity of the signal
		double intensity = 0;
		for (int i = 0; i < spectrum.length; i++) {
			intensity += spectrum[i];
		}
		intensity /= spectrum.length;
		// end get the average intensity of the signal

		// intensity of the whistle should not be too soft
		boolean result = (intensity > minIntensity && intensity <= maxIntensity);
		//System.out.println("intensity: " + intensity + " " + result);

		return result;
	}

	protected boolean isPassedZeroCrossingRate(short[] amplitudes){
		ZeroCrossingRate zcr = new ZeroCrossingRate(amplitudes, 1);
		int numZeroCrosses = (int) zcr.evaluate();

		// different sound has different range of zero crossing value
		// when lengthInSecond=1, zero crossing rate is the num
		// of zero crosses
		boolean result = (numZeroCrosses >= minNumZeroCross && numZeroCrosses <= maxNumZeroCross);
		//System.out.println("zcr: " + numZeroCrosses + " " +result);

		return result;
	}

}