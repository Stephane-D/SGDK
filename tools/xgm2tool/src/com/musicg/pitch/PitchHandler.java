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

package com.musicg.pitch;

import java.util.Arrays;

/**
 * Methods for handling the pitches
 * 
 * @author Jacquet Wong
 *
 */
public class PitchHandler{
	
	/**
	 * Get the tone changed by comparing two tones
	 * return 1.0 is a semi-tone, 2.0 is a tone, etc...
	 * 
	 * @param f1	original tone
	 * @param f2	new tone
	 * @return	tone changed
	 */
	public double getToneChanged(double f1, double f2){		
		return Math.log(f1/f2)/Math.log(2)*12;		
	}
	
	/**
	 * Determine the harmonic probability of a list of frequencies
	 * 
	 * @param list of frequencies
	 * @return	probability of harmonic 
	 */
	public double getHarmonicProbability(double[] frequencies){
		
		int harmonicCount=0;
		int count=0;
		Arrays.sort(frequencies);
		
		for (int i=0; i<frequencies.length; i++){
			for (int j=i+1; j<frequencies.length; j++){
				if (isHarmonic(frequencies[i],frequencies[j])) harmonicCount++;
				count++;
			}
		}
		
		return (double)harmonicCount/count;
	}
	
	/**
	 * Determine the tones are in harmonic relationship or not
	 * 
	 * @param f1	frequency
	 * @param f2	another frequency
	 * @return
	 */
	public boolean isHarmonic(double f1, double f2){
		
		if (Math.abs(getToneChanged(f1,f2))>=1){		
			double minF0=100;
			int minDivisor=(int)(f1/minF0);
			
			for (int i=1; i<=minDivisor; i++){
				double f0=f1/i;
				int maxMultiplier=(int)(f2/f0+1);
				for (int j=2; j<=maxMultiplier; j++){
					double f=f0*j;
					double diff=Math.abs(getToneChanged(f,f2)%12);
					if (diff>6) diff=12-diff;
					if (diff<=1) return true;
				}
			}
		}
		
		return false;
	}
}