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

package com.musicg.math.statistics;

/**
 * Evaluate the zero crossing rate of a signal
 * 
 * @author Jacquet Wong
 *
 */
public class ZeroCrossingRate{
	
	private short[] signals;
	private double lengthInSecond;
	
	/**
	 * Constructor
	 * 
	 * @param signals	input signal array
	 * @param lengthInSecond	length of the signal (in second)
	 */
	public ZeroCrossingRate(short[] signals, double lengthInSecond){
		setSignals(signals,1);
	}
	
	/**
	 * set the signals
	 * 
	 * @param signals	input signal array
	 * @param lengthInSecond	length of the signal (in second)
	 */
	public void setSignals(short[] signals, double lengthInSecond){		
		this.signals=signals;
		this.lengthInSecond=lengthInSecond;
	}
	
	public double evaluate(){
		int numZC=0;
		int size=signals.length;
		
		for (int i=0; i<size-1; i++){
			if((signals[i]>=0 && signals[i+1]<0) || (signals[i]<0 && signals[i+1]>=0)){
				numZC++;
			}
		}			

		return numZC/lengthInSecond;
	}
}