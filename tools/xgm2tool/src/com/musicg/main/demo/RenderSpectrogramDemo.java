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

import com.musicg.graphic.GraphicRender;
import com.musicg.wave.Wave;
import com.musicg.wave.extension.Spectrogram;

public class RenderSpectrogramDemo {
	public static void main(String[] args) {

		String inFolder = "audio_work";		
		String outFolder = "out";
		String filename = "cock_a_1.wav";

		// create a wave object
		Wave wave = new Wave(inFolder+"/"+filename);
		Spectrogram spectrogram = new Spectrogram(wave);

		// Graphic render
		GraphicRender render = new GraphicRender();
		// render.setHorizontalMarker(1);
		// render.setVerticalMarker(1);
		render.renderSpectrogram(spectrogram, outFolder + "/"+filename+".jpg");

		// change the spectrogram representation
		int fftSampleSize = 1024;
		int overlapFactor = 0;
		spectrogram = new Spectrogram(wave, fftSampleSize, overlapFactor);
		render.renderSpectrogram(spectrogram, outFolder + "/"+filename+"2.jpg");
	}
}