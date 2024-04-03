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

import com.musicg.wave.WaveHeader;

/**
 * Api for detecting whistle
 * 
 * @author Jacquet Wong
 * 
 */
public class WhistleApi extends DetectionApi{
	
	public WhistleApi(WaveHeader waveHeader) {
		super(waveHeader);
	}

	protected void init(){
		// settings for detecting a whistle
		minFrequency = 600.0f;
		maxFrequency = Double.MAX_VALUE;
		
		minIntensity = 100.0f;
		maxIntensity = 100000.0f;
		
		minStandardDeviation = 0.1f;
		maxStandardDeviation = 1.0f;
		
		highPass = 100;
		lowPass = 10000;
		
		minNumZeroCross = 50;
		maxNumZeroCross = 200;
		
		numRobust = 10;
	}
		
	public boolean isWhistle(byte[] audioBytes){
		return isSpecificSound(audioBytes);
	}
}