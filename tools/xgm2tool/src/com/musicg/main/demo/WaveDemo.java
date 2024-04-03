/*
 * Copyright (C) 2012 Jacquet Wong
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
package com.musicg.main.demo;

import com.musicg.wave.Wave;
import com.musicg.wave.WaveFileManager;

public class WaveDemo {

	public static void main(String[] args) {

		String filename = "audio_work/cock_a_1.wav";
		String outFolder="out";

		// create a wave object
		Wave wave = new Wave(filename);

		// print the wave header and info
		System.out.println(wave);

		// trim the wav
		wave.leftTrim(1);
		wave.rightTrim(0.5F);

		// save the trimmed wav
		WaveFileManager waveFileManager=new WaveFileManager(wave);
		waveFileManager.saveWaveAsFile(outFolder+"/out.wav");
	}
}